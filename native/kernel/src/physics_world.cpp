#include <ninho/physics/physics_world.hpp>

#include "box3d_conversions.hpp"
#if defined(NINHO_ENABLE_TEST_FACADES)
#include "physics_world_test_facade.hpp"
#endif

#include <ninho/physics/physics_limits.hpp>
#include <ninho/physics/radial_gravity.hpp>

#include <box3d/box3d.h>

#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <iterator>
#include <limits>
#include <stdexcept>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <variant>

namespace ninho::physics {
namespace {

[[nodiscard]] ShapeDesc make_shape(ShapeGeometry geometry, float density)
{
    ShapeDesc shape;
    shape.geometry = std::move(geometry);
    shape.density = density;
    return shape;
}

[[nodiscard]] b3BodyType to_box3d(BodyType type)
{
    switch (type) {
    case BodyType::Static:
        return b3_staticBody;
    case BodyType::Kinematic:
        return b3_kinematicBody;
    case BodyType::Dynamic:
        return b3_dynamicBody;
    }
    return b3_staticBody;
}

[[nodiscard]] Status invalid_handle_status()
{
    return {StatusCode::InvalidHandle, "body handle is not live"};
}

[[nodiscard]] Status invalid_joint_handle_status()
{
    return {StatusCode::InvalidHandle, "joint handle is not live"};
}

[[nodiscard]] Status invalid_argument_status(std::string message)
{
    return {StatusCode::InvalidArgument, std::move(message)};
}

[[nodiscard]] bool valid_quaternion(Quat value) noexcept
{
    if (!std::isfinite(value.x) || !std::isfinite(value.y) || !std::isfinite(value.z)
        || !std::isfinite(value.w)) {
        return false;
    }
    const float length_squared =
        value.x * value.x + value.y * value.y + value.z * value.z + value.w * value.w;
    return std::abs(length_squared - 1.0f) <= 20.0f * std::numeric_limits<float>::epsilon();
}

[[nodiscard]] bool valid_transform(Transform value) noexcept
{
    return is_finite(value.position) && valid_quaternion(value.rotation);
}

[[nodiscard]] bool positive_finite(float value) noexcept
{
    return std::isfinite(value) && value > 0.0f;
}

[[nodiscard]] bool nonnegative_finite(float value) noexcept
{
    return std::isfinite(value) && value >= 0.0f;
}

[[nodiscard]] Status validate_hull(const HullShape& hull)
{
    if (!valid_transform(hull.local)) {
        return invalid_argument_status("hull local transform must be valid");
    }
    if (hull.vertices.size() < 4 || hull.vertices.size() > B3_MAX_SHAPE_CAST_POINTS) {
        return invalid_argument_status("hull requires between 4 and 64 vertices");
    }

    std::vector<Vec3> unique_vertices;
    unique_vertices.reserve(hull.vertices.size());
    std::vector<b3Vec3> native_vertices;
    native_vertices.reserve(hull.vertices.size());
    for (const Vec3 vertex : hull.vertices) {
        if (!is_finite(vertex)) {
            return invalid_argument_status("hull vertices must be finite");
        }
        if (std::find(unique_vertices.begin(), unique_vertices.end(), vertex)
            == unique_vertices.end()) {
            unique_vertices.push_back(vertex);
        }
        native_vertices.push_back(detail::to_box3d_vector(vertex));
    }
    if (unique_vertices.size() < 4) {
        return invalid_argument_status("hull requires at least 4 unique vertices");
    }

    b3HullData* native_hull = b3CreateHull(
        native_vertices.data(),
        static_cast<int>(native_vertices.size()),
        static_cast<int>(native_vertices.size()));
    if (native_hull == nullptr) {
        return invalid_argument_status("vertices do not form a valid convex hull");
    }
    b3DestroyHull(native_hull);
    return {};
}

[[nodiscard]] Status validate_primitive(const PrimitiveShape& primitive)
{
    return std::visit(
        [](const auto& geometry) -> Status {
            using Geometry = std::decay_t<decltype(geometry)>;
            if constexpr (std::is_same_v<Geometry, SphereShape>) {
                if (!positive_finite(geometry.radius) || !valid_transform(geometry.local)) {
                    return invalid_argument_status(
                        "sphere radius and local transform must be valid");
                }
                return {};
            } else if constexpr (std::is_same_v<Geometry, BoxShape>) {
                if (!positive_finite(geometry.half_extents.x)
                    || !positive_finite(geometry.half_extents.y)
                    || !positive_finite(geometry.half_extents.z)
                    || !valid_transform(geometry.local)) {
                    return invalid_argument_status(
                        "box half extents and local transform must be valid");
                }
                return {};
            } else if constexpr (std::is_same_v<Geometry, CapsuleShape>) {
                if (!positive_finite(geometry.half_height)
                    || !positive_finite(geometry.radius)
                    || !valid_transform(geometry.local)) {
                    return invalid_argument_status(
                        "capsule dimensions and local transform must be valid");
                }
                return {};
            } else {
                return validate_hull(geometry);
            }
        },
        primitive);
}

[[nodiscard]] Status validate_geometry(const ShapeGeometry& geometry)
{
    return std::visit(
        [](const auto& value) -> Status {
            using Geometry = std::decay_t<decltype(value)>;
            if constexpr (std::is_same_v<Geometry, CompoundShape>) {
                if (value.children.empty()) {
                    return invalid_argument_status("compound shapes require at least one child");
                }
                for (const PrimitiveShape& child : value.children) {
                    const Status status = validate_primitive(child);
                    if (!status.ok()) {
                        return status;
                    }
                }
                return {};
            } else {
                return validate_primitive(PrimitiveShape{value});
            }
        },
        geometry);
}

[[nodiscard]] Status validate_joint_desc(const JointDesc& joint)
{
    return std::visit(
        [](const auto& desc) -> Status {
            if (desc.a == desc.b) {
                return invalid_argument_status("a joint requires two distinct bodies");
            }
            if (!valid_transform(desc.frame_a) || !valid_transform(desc.frame_b)) {
                return invalid_argument_status(
                    "joint frames must be finite with unit quaternions");
            }
            if (!nonnegative_finite(desc.hertz)
                || !nonnegative_finite(desc.damping_ratio)) {
                return invalid_argument_status("joint tuning must be finite and non-negative");
            }
            using Description = std::decay_t<decltype(desc)>;
            if constexpr (std::is_same_v<Description, DistanceJointDesc>) {
                if (!positive_finite(desc.length)) {
                    return invalid_argument_status(
                        "distance joint length must be finite and positive");
                }
            }
            return {};
        },
        joint);
}

[[nodiscard]] WorldConfig validated_config(WorldConfig config)
{
    if (!positive_finite(config.time_step)) {
        throw std::invalid_argument("time_step must be finite and positive");
    }
    if (config.substeps <= 0) {
        throw std::invalid_argument("substeps must be positive");
    }
    constexpr std::size_t maximum_public_slots =
        static_cast<std::size_t>(std::numeric_limits<std::uint32_t>::max()) - 1U;
    if (config.max_bodies == 0 || config.max_bodies > maximum_public_slots) {
        throw std::invalid_argument("max_bodies is outside the supported handle range");
    }
    static_cast<void>(GravityField{config.gravity});
    static_cast<void>(WorldBounds{config.bounds});
    return config;
}

[[nodiscard]] RadialGravityConfig radial_ejection_config(
    const GravityFieldConfig& config) noexcept
{
    if (const auto* radial = std::get_if<RadialGravityConfig>(&config)) {
        return *radial;
    }
    return {};
}

[[nodiscard]] Status validate_body(const BodyDesc& body)
{
    switch (body.type) {
    case BodyType::Static:
    case BodyType::Kinematic:
    case BodyType::Dynamic:
        break;
    default:
        return invalid_argument_status("body type is invalid");
    }
    if (!valid_transform(body.transform)) {
        return invalid_argument_status("body transform must be finite with a unit quaternion");
    }
    if (!is_finite(body.linear_velocity) || !is_finite(body.angular_velocity)) {
        return invalid_argument_status("body velocities must be finite");
    }
    if (body.type == BodyType::Dynamic && body.shapes.empty()) {
        return invalid_argument_status("dynamic bodies require at least one shape");
    }
    if (body.type == BodyType::Dynamic && !body.affected_by_world_gravity) {
        return invalid_argument_status(
            "dynamic bodies must be affected by world gravity");
    }

    for (const ShapeDesc& shape : body.shapes) {
        if (!positive_finite(shape.density)) {
            return invalid_argument_status("shape density must be finite and positive");
        }
        if (!nonnegative_finite(shape.friction) || !nonnegative_finite(shape.restitution)) {
            return invalid_argument_status("shape friction and restitution must be finite and non-negative");
        }

        const Status geometry_status = validate_geometry(shape.geometry);
        if (!geometry_status.ok()) {
            return geometry_status;
        }
    }
    return {};
}

}

WorldConfig make_legacy_radial_world_config(LegacyRadialWorldConfig legacy) noexcept
{
    return WorldConfig{
        .time_step = legacy.time_step,
        .substeps = legacy.substeps,
        .max_bodies = legacy.max_bodies,
        .gravity = RadialGravityConfig{
            .center_m = {},
            .reference_radius_m = legacy.planet_radius,
            .reference_acceleration_m_s2 = legacy.surface_gravity,
        },
        .bounds = SphericalWorldBounds{
            .center_m = {},
            .removal_radius_m = 6.0f * legacy.planet_radius,
        },
    };
}

BodyDesc BodyDesc::static_sphere(float radius, Transform transform)
{
    BodyDesc body;
    body.type = BodyType::Static;
    body.transform = transform;
    body.affected_by_world_gravity = false;
    body.world_exit_policy = WorldExitPolicy::KeepOutsideBounds;
    body.shapes.push_back(make_shape(SphereShape{radius, {}}, 1.0f));
    return body;
}

BodyDesc BodyDesc::static_box(Vec3 half_extents, Transform transform)
{
    BodyDesc body;
    body.type = BodyType::Static;
    body.transform = transform;
    body.affected_by_world_gravity = false;
    body.world_exit_policy = WorldExitPolicy::KeepOutsideBounds;
    body.shapes.push_back(make_shape(BoxShape{half_extents, {}}, 1.0f));
    return body;
}

BodyDesc BodyDesc::dynamic_sphere(float radius, Transform transform, float density)
{
    BodyDesc body;
    body.type = BodyType::Dynamic;
    body.transform = transform;
    body.shapes.push_back(make_shape(SphereShape{radius, {}}, density));
    return body;
}

BodyDesc BodyDesc::dynamic_box(Vec3 half_extents, Transform transform, float density)
{
    BodyDesc body;
    body.type = BodyType::Dynamic;
    body.transform = transform;
    body.shapes.push_back(make_shape(BoxShape{half_extents, {}}, density));
    return body;
}

struct PhysicsWorld::Impl {
    enum class SlotState { Free, PendingCreate, Live, PendingDestroy };
    static constexpr std::size_t max_joints = 250;
    static constexpr double cast_fraction_quantum = 1.0e-6;

    struct Slot {
        struct OwnedShape {
            b3ShapeId native{};
            float base_density{};
        };

        std::uint32_t generation{};
        SlotState state{SlotState::Free};
        b3BodyId native{};
        std::vector<OwnedShape> shapes;
        b3MassData base_mass_data{};
        float mass_scale{1.0f};
        bool mass_scale_valid{true};
        BodyType type{BodyType::Static};
        WorldExitPolicy world_exit_policy{WorldExitPolicy::KeepOutsideBounds};
        bool ejected{};
        bool exited_world{};
    };

    struct JointSlot {
        std::uint32_t generation{};
        SlotState state{SlotState::Free};
        b3JointId native{};
        BodyHandle a{};
        BodyHandle b{};
    };

    struct ShapeBinding {
        std::uint64_t native_key{};
        BodyHandle body{};
        std::uint64_t material_id{};
    };

    struct CreateCommand {
        BodyHandle handle;
        BodyDesc desc;
    };

    struct DestroyCommand {
        BodyHandle handle;
    };

    struct CreateJointCommand {
        JointHandle handle;
        JointDesc desc;
    };

    struct DestroyJointCommand {
        JointHandle handle;
    };

    struct ForceCommand {
        BodyHandle handle;
        Vec3 force;
        Vec3 point;
        bool wake;
    };

    struct ImpulseCommand {
        BodyHandle handle;
        Vec3 impulse;
        Vec3 point;
        bool wake;
    };

    struct CentralImpulseCommand {
        BodyHandle handle;
        Vec3 impulse;
        bool wake;
    };

    struct Reservation {
        BodyHandle handle;
        bool reused{};
    };

    struct JointReservation {
        JointHandle handle;
        bool reused{};
    };

    struct NativeShapeResult {
        b3ShapeId id{};
        const char* operation{};
    };

    using Command = std::variant<
        CreateCommand,
        DestroyCommand,
        CreateJointCommand,
        DestroyJointCommand,
        ForceCommand,
        ImpulseCommand,
        CentralImpulseCommand>;

    static_assert(std::is_nothrow_move_assignable_v<Command>);
    static_assert(std::is_nothrow_destructible_v<Command>);

    explicit Impl(WorldConfig world_config)
        : config(validated_config(world_config))
        , gravity(config.gravity)
        , world_exit_tracker(
              config.bounds,
              std::holds_alternative<RadialGravityConfig>(config.gravity)
                  ? detail::RadialEjectionPolicy::Enabled
                  : detail::RadialEjectionPolicy::Disabled,
              radial_ejection_config(config.gravity))
    {
        constexpr std::size_t initial_reserve_limit = 1024;
        const std::size_t initial_body_capacity =
            std::min(config.max_bodies, initial_reserve_limit);
        slots.reserve(initial_body_capacity + 1);
        free_slots.reserve(initial_body_capacity);
        snapshots.reserve(initial_body_capacity);
        slots.emplace_back();
        joint_slots.reserve(max_joints + 1);
        free_joint_slots.reserve(max_joints);
        joint_slots.emplace_back();
        shape_bindings.reserve(initial_body_capacity);
        contact_storage.reserve(initial_body_capacity);
        physical_contact_storage.reserve(initial_body_capacity);
        live_contact_id_scratch.reserve(initial_body_capacity);
        joint_reaction_storage.reserve(max_joints);

        b3WorldDef world_def = b3DefaultWorldDef();
        world_def.gravity = {};
        world_def.workerCount = 1;
        const b3WorldId created_world = b3CreateWorld(&world_def);
        if (B3_IS_NULL(created_world) || !b3World_IsValid(created_world)) {
            throw std::runtime_error("Box3DFault: b3CreateWorld");
        }
        world = created_world;
    }

    ~Impl()
    {
        if (B3_IS_NON_NULL(world) && b3World_IsValid(world)) {
            b3DestroyWorld(world);
        }
    }

    [[nodiscard]] bool accepts(BodyHandle handle) const
    {
        if (!handle.valid() || handle.index >= slots.size()) {
            return false;
        }
        const Slot& slot = slots[handle.index];
        return slot.generation == handle.generation
            && (slot.state == SlotState::PendingCreate || slot.state == SlotState::Live);
    }

    [[nodiscard]] bool accepts(JointHandle handle) const
    {
        if (!handle.valid() || handle.index >= joint_slots.size()) {
            return false;
        }
        const JointSlot& slot = joint_slots[handle.index];
        return slot.generation == handle.generation
            && (slot.state == SlotState::PendingCreate || slot.state == SlotState::Live);
    }

    [[nodiscard]] Slot* matching_slot(BodyHandle handle)
    {
        if (!handle.valid() || handle.index >= slots.size()) {
            return nullptr;
        }
        Slot& slot = slots[handle.index];
        return slot.generation == handle.generation ? &slot : nullptr;
    }

    [[nodiscard]] const Slot* matching_slot(BodyHandle handle) const
    {
        if (!handle.valid() || handle.index >= slots.size()) {
            return nullptr;
        }
        const Slot& slot = slots[handle.index];
        return slot.generation == handle.generation ? &slot : nullptr;
    }

    [[nodiscard]] JointSlot* matching_joint_slot(JointHandle handle)
    {
        if (!handle.valid() || handle.index >= joint_slots.size()) {
            return nullptr;
        }
        JointSlot& slot = joint_slots[handle.index];
        return slot.generation == handle.generation ? &slot : nullptr;
    }

    [[nodiscard]] const JointSlot* matching_joint_slot(JointHandle handle) const
    {
        if (!handle.valid() || handle.index >= joint_slots.size()) {
            return nullptr;
        }
        const JointSlot& slot = joint_slots[handle.index];
        return slot.generation == handle.generation ? &slot : nullptr;
    }

    [[nodiscard]] const ShapeBinding* find_shape_binding(b3ShapeId shape) const
    {
        const std::uint64_t key = b3StoreShapeId(shape);
        const auto found = std::find_if(
            shape_bindings.begin(),
            shape_bindings.end(),
            [key](const ShapeBinding& binding) { return binding.native_key == key; });
        return found == shape_bindings.end() ? nullptr : &*found;
    }

    [[nodiscard]] Reservation reserve_handle()
    {
        std::uint32_t index{};
        bool reused = false;
        if (!free_slots.empty()) {
            index = free_slots.back();
            reused = true;
        } else {
            const std::size_t required_free_capacity = slots.size() + 1;
            if (free_slots.capacity() < required_free_capacity) {
                free_slots.reserve(required_free_capacity);
            }
            index = static_cast<std::uint32_t>(slots.size());
            slots.emplace_back();
        }

        Slot& slot = slots[index];
        if (slot.generation != 0) {
            world_exit_tracker.reset({index, slot.generation});
        }
        ++slot.generation;
        if (slot.generation == 0) {
            ++slot.generation;
        }
        slot.state = SlotState::PendingCreate;
        slot.native = {};
        slot.shapes.clear();
        slot.base_mass_data = {};
        slot.mass_scale = 1.0f;
        slot.mass_scale_valid = true;
        slot.ejected = false;
        slot.exited_world = false;
        const BodyHandle handle{index, slot.generation};
        return {handle, reused};
    }

    [[nodiscard]] JointReservation reserve_joint_handle()
    {
        std::uint32_t index{};
        bool reused = false;
        if (!free_joint_slots.empty()) {
            index = free_joint_slots.back();
            reused = true;
        } else {
            index = static_cast<std::uint32_t>(joint_slots.size());
            joint_slots.emplace_back();
        }

        JointSlot& slot = joint_slots[index];
        ++slot.generation;
        if (slot.generation == 0) {
            ++slot.generation;
        }
        slot.state = SlotState::PendingCreate;
        slot.native = {};
        slot.a = {};
        slot.b = {};
        return {{index, slot.generation}, reused};
    }

    void commit_reservation(const Reservation& reservation) noexcept
    {
        if (reservation.reused) {
            free_slots.pop_back();
        }
    }

    void commit_reservation(const JointReservation& reservation) noexcept
    {
        if (reservation.reused) {
            free_joint_slots.pop_back();
        }
    }

    void rollback_reservation(const Reservation& reservation) noexcept
    {
        if (!reservation.reused) {
            slots.pop_back();
            return;
        }

        Slot& slot = slots[reservation.handle.index];
        slot.state = SlotState::Free;
        slot.native = {};
        slot.shapes.clear();
        slot.base_mass_data = {};
        slot.mass_scale = 1.0f;
        slot.mass_scale_valid = true;
        slot.type = BodyType::Static;
        slot.world_exit_policy = WorldExitPolicy::KeepOutsideBounds;
        slot.ejected = false;
        slot.exited_world = false;
    }

    void rollback_reservation(const JointReservation& reservation) noexcept
    {
        if (!reservation.reused) {
            joint_slots.pop_back();
            return;
        }
        JointSlot& slot = joint_slots[reservation.handle.index];
        slot.state = SlotState::Free;
        slot.native = {};
        slot.a = {};
        slot.b = {};
    }

    void release_joint_slot(JointHandle handle, b3JointId native)
    {
        JointSlot* slot = matching_joint_slot(handle);
        if (slot == nullptr || slot->state == SlotState::Free) {
            return;
        }

        // Constructor reservation covers the fixed 250-joint foundation capacity.
        free_joint_slots.push_back(handle.index);
        if (B3_IS_NON_NULL(native) && b3Joint_IsValid(native)) {
            b3DestroyJoint(native, true);
        }
        std::erase_if(
            joint_reaction_storage,
            [handle](const JointReaction& reaction) { return reaction.joint == handle; });
        slot->native = {};
        slot->state = SlotState::Free;
        slot->a = {};
        slot->b = {};
        --reserved_joint_count;
    }

    void invalidate_joint_handle(JointHandle handle) noexcept
    {
        JointSlot* slot = matching_joint_slot(handle);
        if (slot == nullptr || slot->state == SlotState::Free) {
            return;
        }

        // Fixed-capacity reservation makes this public invalidation non-allocating.
        free_joint_slots.push_back(handle.index);
        std::erase_if(
            joint_reaction_storage,
            [handle](const JointReaction& reaction) { return reaction.joint == handle; });
        slot->native = {};
        slot->state = SlotState::Free;
        slot->a = {};
        slot->b = {};
        --reserved_joint_count;
    }

    void transition_body_to_pending_destroy(BodyHandle body) noexcept
    {
        Slot* slot = matching_slot(body);
        if (slot == nullptr || slot->state == SlotState::Free
            || slot->state == SlotState::PendingDestroy) {
            return;
        }
        slot->state = SlotState::PendingDestroy;
        for (std::uint32_t index = 1; index < joint_slots.size(); ++index) {
            const JointSlot& joint_slot = joint_slots[index];
            if (joint_slot.state != SlotState::Free
                && (joint_slot.a == body || joint_slot.b == body)) {
                invalidate_joint_handle({index, joint_slot.generation});
            }
        }
    }

    void release_attached_joints(BodyHandle body)
    {
        for (std::uint32_t index = 1; index < joint_slots.size(); ++index) {
            JointSlot& joint_slot = joint_slots[index];
            if (joint_slot.state == SlotState::Free
                || (joint_slot.a != body && joint_slot.b != body)) {
                continue;
            }
            release_joint_slot({index, joint_slot.generation}, joint_slot.native);
        }
    }

    void release_slot(BodyHandle handle, b3BodyId native)
    {
        Slot* slot = matching_slot(handle);
        if (slot == nullptr || slot->state == SlotState::Free) {
            return;
        }

        free_slots.push_back(handle.index);
        release_attached_joints(handle);
        std::erase_if(shape_bindings, [handle](const ShapeBinding& binding) {
            return binding.body == handle;
        });
        if (B3_IS_NON_NULL(native) && b3Body_IsValid(native)) {
            b3DestroyBody(native);
        }
        world_exit_tracker.reset(handle);
        slot->native = {};
        slot->shapes.clear();
        slot->base_mass_data = {};
        slot->mass_scale = 1.0f;
        slot->mass_scale_valid = true;
        slot->state = SlotState::Free;
        slot->type = BodyType::Static;
        slot->world_exit_policy = WorldExitPolicy::KeepOutsideBounds;
        slot->ejected = false;
        slot->exited_world = false;
        --reserved_body_count;
    }

    [[noreturn]] void fail_native_create(
        BodyHandle handle,
        b3BodyId native,
        std::string_view operation)
    {
        release_slot(handle, native);
        throw std::runtime_error("Box3DFault: " + std::string(operation));
    }

    [[noreturn]] void fail_native_joint_create(
        JointHandle handle,
        b3JointId native,
        std::string_view operation)
    {
        release_joint_slot(handle, native);
        throw std::runtime_error("Box3DFault: " + std::string(operation));
    }

    [[nodiscard]] NativeShapeResult create_native_primitive(
        b3BodyId native,
        const b3ShapeDef& shape_def,
        const PrimitiveShape& primitive)
    {
        return std::visit(
            [&](const auto& geometry) -> NativeShapeResult {
                using Geometry = std::decay_t<decltype(geometry)>;
                if constexpr (std::is_same_v<Geometry, SphereShape>) {
                    const b3Sphere sphere{
                        detail::to_box3d_vector(geometry.local.position), geometry.radius};
                    return {b3CreateSphereShape(native, &shape_def, &sphere),
                            "b3CreateSphereShape"};
                } else if constexpr (std::is_same_v<Geometry, BoxShape>) {
                    const b3BoxHull box = b3MakeTransformedBoxHull(
                        geometry.half_extents.x,
                        geometry.half_extents.y,
                        geometry.half_extents.z,
                        detail::to_box3d_local(geometry.local));
                    return {b3CreateHullShape(native, &shape_def, &box.base),
                            "b3CreateHullShape(box)"};
                } else if constexpr (std::is_same_v<Geometry, CapsuleShape>) {
                    const b3Transform local = detail::to_box3d_local(geometry.local);
                    const b3Capsule capsule{
                        b3TransformPoint(local, {0.0f, -geometry.half_height, 0.0f}),
                        b3TransformPoint(local, {0.0f, geometry.half_height, 0.0f}),
                        geometry.radius,
                    };
                    return {b3CreateCapsuleShape(native, &shape_def, &capsule),
                            "b3CreateCapsuleShape"};
                } else {
                    std::array<b3Vec3, B3_MAX_SHAPE_CAST_POINTS> vertices{};
                    std::ranges::transform(
                        geometry.vertices,
                        vertices.begin(),
                        [](Vec3 vertex) { return detail::to_box3d_vector(vertex); });
                    b3HullData* source = b3CreateHull(
                        vertices.data(),
                        static_cast<int>(geometry.vertices.size()),
                        static_cast<int>(geometry.vertices.size()));
                    if (source == nullptr) {
                        return {{}, "b3CreateHull"};
                    }
                    const b3ShapeId shape = b3CreateTransformedHullShape(
                        native,
                        &shape_def,
                        source,
                        detail::to_box3d_local(geometry.local),
                        {1.0f, 1.0f, 1.0f});
                    b3DestroyHull(source);
                    return {shape, "b3CreateTransformedHullShape"};
                }
            },
            primitive);
    }

    void attach_primitive(
        b3BodyId native,
        BodyHandle handle,
        const b3ShapeDef& shape_def,
        const PrimitiveShape& primitive,
        float base_density,
        std::uint64_t material_id)
    {
        const NativeShapeResult created = create_native_primitive(native, shape_def, primitive);
        if (B3_IS_NULL(created.id) || !b3Shape_IsValid(created.id)) {
            fail_native_create(handle, native, created.operation);
        }
        shape_bindings.push_back({b3StoreShapeId(created.id), handle, material_id});
        Slot* slot = matching_slot(handle);
        if (slot == nullptr) {
            fail_native_create(handle, native, "body slot unavailable after shape creation");
        }
        slot->shapes.push_back({created.id, base_density});
    }

    void create_native_body(const CreateCommand& command)
    {
        Slot* slot = matching_slot(command.handle);
        if (slot == nullptr) {
            return;
        }
        if (slot->state == SlotState::PendingDestroy && B3_IS_NULL(slot->native)) {
            return;
        }

        const BodyDesc& desc = command.desc;
        std::size_t shape_count = 0;
        for (const ShapeDesc& shape : desc.shapes) {
            if (const auto* compound = std::get_if<CompoundShape>(&shape.geometry)) {
                shape_count += compound->children.size();
            } else {
                ++shape_count;
            }
        }
        try {
            shape_bindings.reserve(shape_bindings.size() + shape_count);
            slot->shapes.reserve(shape_count);
        } catch (...) {
            release_slot(command.handle, {});
            throw;
        }

        b3BodyDef body_def = b3DefaultBodyDef();
        body_def.type = to_box3d(desc.type);
        body_def.position = detail::to_box3d_position(desc.transform.position);
        body_def.rotation = detail::to_box3d(desc.transform.rotation);
        body_def.linearVelocity = detail::to_box3d_vector(desc.linear_velocity);
        body_def.angularVelocity = detail::to_box3d_vector(desc.angular_velocity);
        body_def.gravityScale = 0.0f;
        body_def.enableSleep = desc.enable_sleep;
        body_def.isBullet = desc.bullet;
        body_def.name = desc.name.empty() ? nullptr : desc.name.c_str();

        const b3BodyId native = b3CreateBody(world, &body_def);
        if (B3_IS_NULL(native) || !b3Body_IsValid(native)) {
            fail_native_create(command.handle, native, "b3CreateBody");
        }

        for (const ShapeDesc& shape : desc.shapes) {
            b3ShapeDef shape_def = b3DefaultShapeDef();
            shape_def.density = shape.density;
            shape_def.baseMaterial.friction = shape.friction;
            shape_def.baseMaterial.restitution = shape.restitution;
            shape_def.baseMaterial.userMaterialId = shape.material_id;
            shape_def.enableHitEvents = shape.hit_events;
            shape_def.filter.groupIndex = shape.collision_group;

            if (const auto* compound = std::get_if<CompoundShape>(&shape.geometry)) {
                for (const PrimitiveShape& child : compound->children) {
                    attach_primitive(
                        native, command.handle, shape_def, child,
                        shape.density, shape.material_id);
                }
            } else {
                std::visit(
                    [&](const auto& primitive) {
                        using Geometry = std::decay_t<decltype(primitive)>;
                        if constexpr (!std::is_same_v<Geometry, CompoundShape>) {
                            attach_primitive(
                                native,
                                command.handle,
                                shape_def,
                                PrimitiveShape{primitive},
                                shape.density,
                                shape.material_id);
                        }
                    },
                    shape.geometry);
            }
        }

        slot->native = native;
        slot->base_mass_data = b3Body_GetMassData(native);
        slot->mass_scale = 1.0f;
        slot->mass_scale_valid = true;
        slot->type = desc.type;
        slot->world_exit_policy = desc.world_exit_policy;
        if (slot->state == SlotState::PendingCreate) {
            slot->state = SlotState::Live;
        }
    }

    void destroy_native_body(const DestroyCommand& command)
    {
        Slot* slot = matching_slot(command.handle);
        if (slot == nullptr) {
            return;
        }
        release_slot(command.handle, slot->native);
    }

    void create_native_joint(const CreateJointCommand& command)
    {
        JointSlot* slot = matching_joint_slot(command.handle);
        if (slot == nullptr || slot->state == SlotState::Free) {
            return;
        }
        if (slot->state == SlotState::PendingDestroy && B3_IS_NULL(slot->native)) {
            return;
        }

        const BodyHandle a = std::visit([](const auto& desc) { return desc.a; }, command.desc);
        const BodyHandle b = std::visit([](const auto& desc) { return desc.b; }, command.desc);
        Slot* body_a = matching_slot(a);
        Slot* body_b = matching_slot(b);
        if (body_a == nullptr || body_b == nullptr || body_a->state != SlotState::Live
            || body_b->state != SlotState::Live || B3_IS_NULL(body_a->native)
            || B3_IS_NULL(body_b->native) || !b3Body_IsValid(body_a->native)
            || !b3Body_IsValid(body_b->native)) {
            fail_native_joint_create(command.handle, {}, "joint body unavailable");
        }

        const auto created = std::visit(
            [&](const auto& desc) -> std::pair<b3JointId, const char*> {
                using Description = std::decay_t<decltype(desc)>;
                if constexpr (std::is_same_v<Description, DistanceJointDesc>) {
                    b3DistanceJointDef def = b3DefaultDistanceJointDef();
                    def.base.bodyIdA = body_a->native;
                    def.base.bodyIdB = body_b->native;
                    def.base.localFrameA = detail::to_box3d_local(desc.frame_a);
                    def.base.localFrameB = detail::to_box3d_local(desc.frame_b);
                    def.base.collideConnected = desc.collide_connected;
                    def.length = desc.length;
                    def.enableSpring = desc.hertz > 0.0f;
                    def.hertz = desc.hertz;
                    def.dampingRatio = desc.damping_ratio;
                    return {b3CreateDistanceJoint(world, &def), "b3CreateDistanceJoint"};
                } else {
                    b3WeldJointDef def = b3DefaultWeldJointDef();
                    def.base.bodyIdA = body_a->native;
                    def.base.bodyIdB = body_b->native;
                    def.base.localFrameA = detail::to_box3d_local(desc.frame_a);
                    def.base.localFrameB = detail::to_box3d_local(desc.frame_b);
                    def.base.collideConnected = desc.collide_connected;
                    def.linearHertz = desc.hertz;
                    def.angularHertz = desc.hertz;
                    def.linearDampingRatio = desc.damping_ratio;
                    def.angularDampingRatio = desc.damping_ratio;
                    return {b3CreateWeldJoint(world, &def), "b3CreateWeldJoint"};
                }
            },
            command.desc);
        if (B3_IS_NULL(created.first) || !b3Joint_IsValid(created.first)) {
            fail_native_joint_create(command.handle, created.first, created.second);
        }
        slot->native = created.first;
        slot->a = a;
        slot->b = b;
        if (slot->state == SlotState::PendingCreate) {
            slot->state = SlotState::Live;
        }
    }

    void destroy_native_joint(const DestroyJointCommand& command)
    {
        JointSlot* slot = matching_joint_slot(command.handle);
        if (slot == nullptr) {
            return;
        }
        release_joint_slot(command.handle, slot->native);
    }

    void apply(const Command& command)
    {
        std::visit(
            [&](const auto& value) {
                using Value = std::decay_t<decltype(value)>;
                if constexpr (std::is_same_v<Value, CreateCommand>) {
                    create_native_body(value);
                } else if constexpr (std::is_same_v<Value, DestroyCommand>) {
                    destroy_native_body(value);
                } else if constexpr (std::is_same_v<Value, CreateJointCommand>) {
                    create_native_joint(value);
                } else if constexpr (std::is_same_v<Value, DestroyJointCommand>) {
                    destroy_native_joint(value);
                } else {
                    Slot* slot = matching_slot(value.handle);
                    if (slot == nullptr || B3_IS_NULL(slot->native)
                        || !b3Body_IsValid(slot->native)) {
                        return;
                    }
                    if constexpr (std::is_same_v<Value, ForceCommand>) {
                        b3Body_ApplyForce(
                            slot->native,
                            detail::to_box3d_vector(value.force),
                            detail::to_box3d_position(value.point),
                            value.wake);
                    } else if constexpr (std::is_same_v<Value, ImpulseCommand>) {
                        b3Body_ApplyLinearImpulse(
                            slot->native,
                            detail::to_box3d_vector(value.impulse),
                            detail::to_box3d_position(value.point),
                            value.wake);
                    } else if constexpr (std::is_same_v<Value, CentralImpulseCommand>) {
                        b3Body_ApplyLinearImpulseToCenter(
                            slot->native,
                            detail::to_box3d_vector(value.impulse),
                            value.wake);
                    }
                }
            },
            command);
    }

    void discard_command_prefix(std::size_t prefix_count) noexcept
    {
        const std::size_t suffix_count = commands.size() - prefix_count;
        for (std::size_t index = 0; index < suffix_count; ++index) {
            commands[index] = std::move(commands[prefix_count + index]);
        }
        while (commands.size() > suffix_count) {
            commands.pop_back();
        }
    }

    void apply_queued_commands()
    {
        // Command application never enqueues. Auto-removal is a later phase after Box3D stepping.
        for (std::size_t index = 0; index < commands.size(); ++index) {
            try {
                apply(commands[index]);
            } catch (...) {
                discard_command_prefix(index + 1);
                throw;
            }
        }
        commands.clear();
    }

    [[nodiscard]] float dynamic_mass(BodyHandle handle) const
    {
        const Slot* slot = matching_slot(handle);
        if (slot == nullptr || slot->state != SlotState::Live || slot->type != BodyType::Dynamic
            || B3_IS_NULL(slot->native) || !b3Body_IsValid(slot->native)) {
            return 0.0f;
        }
        return b3Body_GetMass(slot->native);
    }

    void copy_contact_hits()
    {
        contact_storage.clear();
        const b3ContactEvents events = b3World_GetContactEvents(world);
        if (events.hitCount <= 0) {
            return;
        }
        contact_storage.reserve(static_cast<std::size_t>(events.hitCount));
        for (int index = 0; index < events.hitCount; ++index) {
            const b3ContactHitEvent& event = events.hitEvents[index];
            const ShapeBinding* binding_a = find_shape_binding(event.shapeIdA);
            const ShapeBinding* binding_b = find_shape_binding(event.shapeIdB);
            if (binding_a == nullptr || binding_b == nullptr || binding_a->body == binding_b->body) {
                continue;
            }
            const Slot* slot_a = matching_slot(binding_a->body);
            const Slot* slot_b = matching_slot(binding_b->body);
            if (slot_a == nullptr || slot_b == nullptr || slot_a->state != SlotState::Live
                || slot_b->state != SlotState::Live) {
                continue;
            }

            ContactHit hit{
                .a = binding_a->body,
                .b = binding_b->body,
                .point = detail::from_box3d_position(event.point),
                .normal = detail::from_box3d_vector(event.normal),
                .approach_speed = event.approachSpeed,
                .material_a = event.userMaterialIdA,
                .material_b = event.userMaterialIdB,
            };
            if (hit.b < hit.a) {
                std::swap(hit.a, hit.b);
                std::swap(hit.material_a, hit.material_b);
                hit.normal = -hit.normal;
            }
            const float mass_a = dynamic_mass(hit.a);
            const float mass_b = dynamic_mass(hit.b);
            if (mass_a > 0.0f && mass_b > 0.0f) {
                hit.effective_mass = mass_a * mass_b / (mass_a + mass_b);
            } else {
                hit.effective_mass = std::max(mass_a, mass_b);
            }
            hit.derived_energy = derived_contact_energy(
                hit.effective_mass, hit.approach_speed);
            if (!is_finite(hit.point) || !is_finite(hit.normal)
                || !positive_finite(hit.approach_speed)
                || !positive_finite(hit.effective_mass)
                || !nonnegative_finite(hit.derived_energy)) {
                continue;
            }

            const auto duplicate = std::find_if(
                contact_storage.begin(),
                contact_storage.end(),
                [&](const ContactHit& current) {
                    return current.a == hit.a && current.b == hit.b;
                });
            if (duplicate == contact_storage.end()) {
                contact_storage.push_back(hit);
            } else if (stronger_contact_for_pair(hit, *duplicate)) {
                *duplicate = hit;
            }
        }
        std::ranges::sort(contact_storage, [](const ContactHit& lhs, const ContactHit& rhs) {
            return lhs.a != rhs.a ? lhs.a < rhs.a : lhs.b < rhs.b;
        });
    }

    void copy_physical_contacts()
    {
        physical_contact_storage.clear();
        for (std::uint32_t index = 1; index < slots.size(); ++index) {
            const Slot& slot = slots[index];
            if (slot.state != SlotState::Live || B3_IS_NULL(slot.native)
                || !b3Body_IsValid(slot.native)) {
                continue;
            }
            const int capacity = b3Body_GetContactCapacity(slot.native);
            if (capacity <= 0) {
                continue;
            }
            if (physical_contact_data_scratch.size() < static_cast<std::size_t>(capacity)) {
                physical_contact_data_scratch.resize(static_cast<std::size_t>(capacity));
            }
            const int count = b3Body_GetContactData(
                slot.native, physical_contact_data_scratch.data(), capacity);
            for (int contact_index = 0; contact_index < count; ++contact_index) {
                const b3ContactData& contact = physical_contact_data_scratch[contact_index];
                const ShapeBinding* binding_a = find_shape_binding(contact.shapeIdA);
                const ShapeBinding* binding_b = find_shape_binding(contact.shapeIdB);
                if (binding_a == nullptr || binding_b == nullptr
                    || binding_a->body == binding_b->body) {
                    continue;
                }
                const Slot* slot_a = matching_slot(binding_a->body);
                const Slot* slot_b = matching_slot(binding_b->body);
                if (slot_a == nullptr || slot_b == nullptr
                    || slot_a->state != SlotState::Live || slot_b->state != SlotState::Live
                    || B3_IS_NULL(slot_a->native) || !b3Body_IsValid(slot_a->native)) {
                    continue;
                }
                // b3Body_GetContactData exposes the same contact from both
                // bodies. Process it only while visiting the canonical public
                // handle so compound/shape-pair impulses are summed once.
                const BodyHandle visited{index, slot.generation};
                if (visited != std::min(binding_a->body, binding_b->body)) {
                    continue;
                }
                for (int manifold_index = 0; manifold_index < contact.manifoldCount;
                    ++manifold_index) {
                    const b3Manifold& manifold = contact.manifolds[manifold_index];
                    float total_impulse = 0.0f;
                    float approach_speed = 0.0f;
                    Vec3 point{};
                    int contributing_points = 0;
                    const Vec3 center_a = detail::from_box3d_position(
                        b3Body_GetWorldCenterOfMass(slot_a->native));
                    for (int point_index = 0; point_index < manifold.pointCount; ++point_index) {
                        const b3ManifoldPoint& manifold_point = manifold.points[point_index];
                        if (!std::isfinite(manifold_point.totalNormalImpulse)
                            || !std::isfinite(manifold_point.normalVelocity)) {
                            continue;
                        }
                        total_impulse += std::max(0.0f, manifold_point.totalNormalImpulse);
                        approach_speed = std::max(
                            approach_speed, std::max(0.0f, -manifold_point.normalVelocity));
                        point = point + center_a
                            + detail::from_box3d_vector(manifold_point.anchorA);
                        ++contributing_points;
                    }
                    if (contributing_points == 0 || total_impulse <= 0.0f) {
                        continue;
                    }
                    PhysicalContact value{
                        .a = binding_a->body,
                        .b = binding_b->body,
                        .point = point / static_cast<float>(contributing_points),
                        .normal = detail::from_box3d_vector(manifold.normal),
                        .approach_speed_m_s = approach_speed,
                        .total_normal_impulse_n_s = total_impulse,
                    };
                    if (value.b < value.a) {
                        std::swap(value.a, value.b);
                        value.normal = -value.normal;
                    }
                    if (!is_finite(value.point) || !is_finite(value.normal)
                        || !nonnegative_finite(value.approach_speed_m_s)
                        || !positive_finite(value.total_normal_impulse_n_s)) {
                        continue;
                    }
                    const auto existing = std::ranges::find_if(
                        physical_contact_storage, [&](const PhysicalContact& current) {
                            return current.a == value.a && current.b == value.b;
                        });
                    if (existing == physical_contact_storage.end()) {
                        physical_contact_storage.push_back(value);
                    } else {
                        existing->total_normal_impulse_n_s +=
                            value.total_normal_impulse_n_s;
                        existing->approach_speed_m_s = std::max(
                            existing->approach_speed_m_s,
                            value.approach_speed_m_s);
                    }
                }
            }
        }
        std::ranges::sort(
            physical_contact_storage, [](const PhysicalContact& lhs,
                                         const PhysicalContact& rhs) {
                return lhs.a != rhs.a ? lhs.a < rhs.a : lhs.b < rhs.b;
            });
    }

    void copy_joint_reactions()
    {
        joint_reaction_storage.clear();
        joint_reaction_storage.reserve(reserved_joint_count);
        for (std::uint32_t index = 1; index < joint_slots.size(); ++index) {
            const JointSlot& slot = joint_slots[index];
            if (slot.state != SlotState::Live || B3_IS_NULL(slot.native)
                || !b3Joint_IsValid(slot.native)) {
                continue;
            }
            const JointReaction reaction{
                .joint = {index, slot.generation},
                .force = detail::from_box3d_vector(b3Joint_GetConstraintForce(slot.native)),
                .torque = detail::from_box3d_vector(b3Joint_GetConstraintTorque(slot.native)),
                .linear_separation = b3Joint_GetLinearSeparation(slot.native),
                .angular_separation = b3Joint_GetAngularSeparation(slot.native),
            };
            if (is_finite(reaction.force) && is_finite(reaction.torque)
                && std::isfinite(reaction.linear_separation)
                && std::isfinite(reaction.angular_separation)) {
                joint_reaction_storage.push_back(reaction);
            }
        }
    }

    [[nodiscard]] int live_contact_count() const
    {
        live_contact_id_scratch.clear();
        for (std::uint32_t index = 1; index < slots.size(); ++index) {
            const Slot& slot = slots[index];
            if (slot.state != SlotState::Live || B3_IS_NULL(slot.native)
                || !b3Body_IsValid(slot.native)) {
                continue;
            }
            const int capacity = b3Body_GetContactCapacity(slot.native);
            if (capacity <= 0) {
                continue;
            }
            if (live_contact_data_scratch.size() < static_cast<std::size_t>(capacity)) {
                live_contact_data_scratch.resize(static_cast<std::size_t>(capacity));
            }
            const int count = b3Body_GetContactData(
                slot.native, live_contact_data_scratch.data(), capacity);
            for (int contact_index = 0; contact_index < count; ++contact_index) {
                const b3ContactData& contact = live_contact_data_scratch[contact_index];
                if (!b3Contact_IsValid(contact.contactId)) {
                    continue;
                }
                const ShapeBinding* binding_a = find_shape_binding(contact.shapeIdA);
                const ShapeBinding* binding_b = find_shape_binding(contact.shapeIdB);
                if (binding_a == nullptr || binding_b == nullptr) {
                    continue;
                }
                const Slot* body_a = matching_slot(binding_a->body);
                const Slot* body_b = matching_slot(binding_b->body);
                if (body_a == nullptr || body_b == nullptr || body_a->state != SlotState::Live
                    || body_b->state != SlotState::Live) {
                    continue;
                }
                std::array<std::uint32_t, 3> key{};
                b3StoreContactId(contact.contactId, key.data());
                live_contact_id_scratch.push_back(key);
            }
        }
        std::ranges::sort(live_contact_id_scratch);
        return static_cast<int>(
            std::ranges::unique(live_contact_id_scratch).begin()
            - live_contact_id_scratch.begin());
    }

    struct QueryProxyStorage {
        std::array<b3Vec3, B3_MAX_SHAPE_CAST_POINTS> points{};
        int count{};
        float radius{};

        [[nodiscard]] b3ShapeProxy proxy() const
        {
            return {points.data(), count, radius};
        }
    };

    struct QueryCandidate {
        QueryHit hit{};
        std::uint64_t shape_key{};
    };

    struct OverlapContext {
        const Impl* impl{};
        Vec3 origin{};
        std::vector<QueryCandidate> candidates;
    };

    struct CastContext {
        const Impl* impl{};
        std::span<const BodyHandle> ignored_handles{};
        std::vector<QueryCandidate> candidates;
    };

    [[nodiscard]] static QueryProxyStorage make_query_proxy(
        const QueryShape& shape, Transform transform)
    {
        QueryProxyStorage storage;
        const b3Transform query_rotation{{}, detail::to_box3d(transform.rotation)};
        std::visit(
            [&](const auto& geometry) {
                using Geometry = std::decay_t<decltype(geometry)>;
                const b3Transform local =
                    b3MulTransforms(query_rotation, detail::to_box3d_local(geometry.local));
                if constexpr (std::is_same_v<Geometry, SphereShape>) {
                    storage.points[0] = local.p;
                    storage.count = 1;
                    storage.radius = geometry.radius;
                } else if constexpr (std::is_same_v<Geometry, BoxShape>) {
                    for (int x = -1; x <= 1; x += 2) {
                        for (int y = -1; y <= 1; y += 2) {
                            for (int z = -1; z <= 1; z += 2) {
                                storage.points[storage.count++] = b3TransformPoint(
                                    local,
                                    {x * geometry.half_extents.x,
                                     y * geometry.half_extents.y,
                                     z * geometry.half_extents.z});
                            }
                        }
                    }
                } else if constexpr (std::is_same_v<Geometry, CapsuleShape>) {
                    storage.points[0] = b3TransformPoint(
                        local, {0.0f, -geometry.half_height, 0.0f});
                    storage.points[1] = b3TransformPoint(
                        local, {0.0f, geometry.half_height, 0.0f});
                    storage.count = 2;
                    storage.radius = geometry.radius;
                } else {
                    for (const Vec3 vertex : geometry.vertices) {
                        storage.points[storage.count++] =
                            b3TransformPoint(local, detail::to_box3d_vector(vertex));
                    }
                }
            },
            shape);
        return storage;
    }

    [[nodiscard]] static bool overlap_callback(b3ShapeId shape, void* raw_context)
    {
        auto& context = *static_cast<OverlapContext*>(raw_context);
        if (!b3Shape_IsValid(shape)) {
            return true;
        }
        const ShapeBinding* binding = context.impl->find_shape_binding(shape);
        if (binding == nullptr) {
            return true;
        }
        const Slot* slot = context.impl->matching_slot(binding->body);
        if (slot == nullptr || slot->state != SlotState::Live) {
            return true;
        }

        const Vec3 point = detail::from_box3d_vector(
            b3Shape_GetClosestPoint(shape, detail::to_box3d_vector(context.origin)));
        context.candidates.push_back({
            .hit = {.body = binding->body,
                    .point = point,
                    .normal = normalized_or_zero(context.origin - point),
                    .fraction = 0.0f,
                    .material_id = binding->material_id},
            .shape_key = binding->native_key,
        });
        return true;
    }

    [[nodiscard]] static float cast_callback(
        b3ShapeId shape,
        b3Pos point,
        b3Vec3 normal,
        float fraction,
        std::uint64_t material_id,
        int,
        int,
        void* raw_context)
    {
        auto& context = *static_cast<CastContext*>(raw_context);
        if (!b3Shape_IsValid(shape)) {
            return -1.0f;
        }
        const ShapeBinding* binding = context.impl->find_shape_binding(shape);
        if (binding == nullptr) {
            return -1.0f;
        }
        if (std::ranges::find(context.ignored_handles, binding->body)
            != context.ignored_handles.end()) {
            return -1.0f;
        }
        const Slot* slot = context.impl->matching_slot(binding->body);
        if (slot == nullptr || slot->state != SlotState::Live || !std::isfinite(fraction)
            || fraction < 0.0f || fraction > 1.0f) {
            return -1.0f;
        }

        const QueryHit hit{
            .body = binding->body,
            .point = detail::from_box3d_position(point),
            .normal = detail::from_box3d_vector(normal),
            .fraction = fraction,
            .material_id = material_id,
        };
        if (!is_finite(hit.point) || !is_finite(hit.normal)) {
            return -1.0f;
        }
        context.candidates.push_back({hit, binding->native_key});
        const auto bucket = static_cast<std::int64_t>(
            std::llround(static_cast<double>(fraction) / cast_fraction_quantum));
        const float bucket_upper = static_cast<float>(
            (static_cast<double>(bucket) + 0.5) * cast_fraction_quantum);
        return std::min(1.0f, std::max(fraction, bucket_upper));
    }

    [[nodiscard]] std::vector<QueryHit> overlap(
        const QueryShape& shape, Transform transform) const
    {
        const QueryProxyStorage storage = make_query_proxy(shape, transform);
        const b3ShapeProxy proxy = storage.proxy();
        OverlapContext context{.impl = this, .origin = transform.position};
        context.candidates.reserve(shape_bindings.size());
        b3World_OverlapShape(
            world,
            detail::to_box3d_position(transform.position),
            &proxy,
            b3DefaultQueryFilter(),
            overlap_callback,
            &context);
        std::ranges::sort(context.candidates, [](const QueryCandidate& lhs, const QueryCandidate& rhs) {
            if (lhs.hit.body != rhs.hit.body) {
                return lhs.hit.body < rhs.hit.body;
            }
            if (lhs.hit.material_id != rhs.hit.material_id) {
                return lhs.hit.material_id < rhs.hit.material_id;
            }
            return lhs.shape_key < rhs.shape_key;
        });

        std::vector<QueryHit> hits;
        hits.reserve(context.candidates.size());
        for (const QueryCandidate& candidate : context.candidates) {
            if (hits.empty() || hits.back().body != candidate.hit.body) {
                hits.push_back(candidate.hit);
            }
        }
        return hits;
    }

    [[nodiscard]] std::optional<QueryHit> cast(
        const QueryShape& shape, Transform transform, Vec3 translation) const
    {
        const QueryProxyStorage storage = make_query_proxy(shape, transform);
        const b3ShapeProxy proxy = storage.proxy();
        CastContext context{.impl = this};
        context.candidates.reserve(shape_bindings.size());
        b3World_CastShape(
            world,
            detail::to_box3d_position(transform.position),
            &proxy,
            detail::to_box3d_vector(translation),
            b3DefaultQueryFilter(),
            cast_callback,
            &context);

        // Fractions within one microunit are a deterministic tie, resolved by public handle.
        std::ranges::sort(context.candidates, [](const QueryCandidate& lhs, const QueryCandidate& rhs) {
            const auto lhs_fraction = static_cast<std::int64_t>(
                std::llround(static_cast<double>(lhs.hit.fraction) / cast_fraction_quantum));
            const auto rhs_fraction = static_cast<std::int64_t>(
                std::llround(static_cast<double>(rhs.hit.fraction) / cast_fraction_quantum));
            if (lhs_fraction != rhs_fraction) {
                return lhs_fraction < rhs_fraction;
            }
            if (lhs.hit.body != rhs.hit.body) {
                return lhs.hit.body < rhs.hit.body;
            }
            if (lhs.hit.material_id != rhs.hit.material_id) {
                return lhs.hit.material_id < rhs.hit.material_id;
            }
            return lhs.shape_key < rhs.shape_key;
        });
        return context.candidates.empty()
            ? std::nullopt
            : std::optional<QueryHit>{context.candidates.front().hit};
    }

    [[nodiscard]] std::optional<QueryHit> cast_segment(Vec3 origin, Vec3 target,
        std::span<const BodyHandle> ignored_handles) const
    {
        CastContext context{.impl = this, .ignored_handles = ignored_handles};
        context.candidates.reserve(shape_bindings.size());
        b3World_CastRay(world, detail::to_box3d_position(origin),
            detail::to_box3d_vector(target - origin), b3DefaultQueryFilter(),
            cast_callback, &context);
        std::ranges::sort(context.candidates, [](const QueryCandidate& lhs,
                                               const QueryCandidate& rhs) {
            const auto lhs_fraction = static_cast<std::int64_t>(std::llround(
                static_cast<double>(lhs.hit.fraction) / cast_fraction_quantum));
            const auto rhs_fraction = static_cast<std::int64_t>(std::llround(
                static_cast<double>(rhs.hit.fraction) / cast_fraction_quantum));
            if (lhs_fraction != rhs_fraction) {
                return lhs_fraction < rhs_fraction;
            }
            if (lhs.hit.body != rhs.hit.body) {
                return lhs.hit.body < rhs.hit.body;
            }
            if (lhs.hit.material_id != rhs.hit.material_id) {
                return lhs.hit.material_id < rhs.hit.material_id;
            }
            return lhs.shape_key < rhs.shape_key;
        });
        return context.candidates.empty()
            ? std::nullopt
            : std::optional<QueryHit>{context.candidates.front().hit};
    }

    void rebuild_snapshots()
    {
        snapshots.clear();
        for (std::uint32_t index = 1; index < slots.size(); ++index) {
            Slot& slot = slots[index];
            if (slot.state != SlotState::Live || B3_IS_NULL(slot.native)
                || !b3Body_IsValid(slot.native)) {
                continue;
            }
            const BodyHandle handle{index, slot.generation};
            snapshots.push_back({
                .handle = handle,
                .transform = detail::from_box3d_world(b3Body_GetTransform(slot.native)),
                .world_center_of_mass = detail::from_box3d_position(
                    b3Body_GetWorldCenterOfMass(slot.native)),
                .linear_velocity = detail::from_box3d_vector(b3Body_GetLinearVelocity(slot.native)),
                .angular_velocity = detail::from_box3d_vector(b3Body_GetAngularVelocity(slot.native)),
                .mass = b3Body_GetMass(slot.native),
                .awake = b3Body_IsAwake(slot.native),
                .ejected = slot.ejected,
                .exited_world = slot.exited_world,
            });
        }
    }

    void update_world_exit_and_queue_removal()
    {
        for (BodyState& snapshot : snapshots) {
            Slot* slot = matching_slot(snapshot.handle);
            if (slot == nullptr || slot->state != SlotState::Live) {
                continue;
            }

            const detail::WorldExitEvents exit = world_exit_tracker.update(
                snapshot.handle,
                snapshot.transform.position,
                snapshot.linear_velocity,
                config.time_step);
            if (exit.radial_ejection) {
                slot->ejected = true;
            }
            if (exit.bounds_exit) {
                slot->exited_world = true;
            }
            snapshot.ejected = slot->ejected;
            snapshot.exited_world = slot->exited_world;

            if (slot->exited_world
                && slot->world_exit_policy == WorldExitPolicy::RemoveOutsideBounds) {
                commands.push_back(DestroyCommand{snapshot.handle});
                transition_body_to_pending_destroy(snapshot.handle);
            }
        }
    }

    WorldConfig config;
    GravityField gravity;
    detail::WorldExitTracker world_exit_tracker;
    b3WorldId world{};
    std::vector<Slot> slots;
    std::vector<std::uint32_t> free_slots;
    std::vector<JointSlot> joint_slots;
    std::vector<std::uint32_t> free_joint_slots;
    std::vector<ShapeBinding> shape_bindings;
    std::vector<Command> commands;
    std::vector<BodyState> snapshots;
    std::vector<ContactHit> contact_storage;
    std::vector<PhysicalContact> physical_contact_storage;
    mutable std::vector<b3ContactData> physical_contact_data_scratch;
    mutable std::vector<b3ContactData> live_contact_data_scratch;
    mutable std::vector<std::array<std::uint32_t, 3>> live_contact_id_scratch;
    std::vector<JointReaction> joint_reaction_storage;
    std::size_t reserved_body_count{};
    std::size_t reserved_joint_count{};
    bool initial_state_committed{};
    bool initialization_faulted{};
    bool step_called{};
#if defined(NINHO_ENABLE_TEST_FACADES)
    std::optional<std::size_t> fail_initial_commit_after_for_testing;
    bool fail_next_mass_scale_postcondition_for_testing{};
#endif
    double step_ms{};
};

PhysicsWorld::PhysicsWorld(WorldConfig config)
    : impl_(std::make_unique<Impl>(config))
{
}

PhysicsWorld::~PhysicsWorld() = default;
PhysicsWorld::PhysicsWorld(PhysicsWorld&&) noexcept = default;
PhysicsWorld& PhysicsWorld::operator=(PhysicsWorld&&) noexcept = default;

Status PhysicsWorld::prepare_body_creations(std::size_t count)
{
    if (count > remaining_body_capacity()) {
        return {StatusCode::CapacityExceeded,
            "physics body capacity has been reached"};
    }
    const std::size_t reusable = std::min(count, impl_->free_slots.size());
    const std::size_t new_slots = count - reusable;
    impl_->commands.reserve(impl_->commands.size() + count + 1U);
    impl_->slots.reserve(impl_->slots.size() + new_slots);
    impl_->free_slots.reserve(std::max(
        impl_->free_slots.capacity(), impl_->slots.size() + new_slots));
    return {};
}

Result<BodyHandle> PhysicsWorld::create_body(BodyDesc desc)
{
    const Status validation = validate_body(desc);
    if (!validation.ok()) {
        return {{}, validation};
    }
    if (impl_->reserved_body_count >= impl_->config.max_bodies) {
        return {{}, {StatusCode::CapacityExceeded, "physics body capacity has been reached"}};
    }

    Impl::CreateCommand command{{}, std::move(desc)};
    const Impl::Reservation reservation = impl_->reserve_handle();
    command.handle = reservation.handle;
    try {
        impl_->commands.emplace_back(std::move(command));
    } catch (...) {
        impl_->rollback_reservation(reservation);
        throw;
    }
    impl_->commit_reservation(reservation);
    ++impl_->reserved_body_count;
    return {reservation.handle, {}};
}

Status PhysicsWorld::destroy_body(BodyHandle body)
{
    if (!impl_->accepts(body)) {
        return invalid_handle_status();
    }
    impl_->commands.emplace_back(Impl::DestroyCommand{body});
    impl_->transition_body_to_pending_destroy(body);
    std::erase_if(impl_->snapshots, [body](const BodyState& value) { return value.handle == body; });
    return {};
}

Result<JointHandle> PhysicsWorld::create_joint(const JointDesc& desc)
{
    const BodyHandle a = std::visit([](const auto& value) { return value.a; }, desc);
    const BodyHandle b = std::visit([](const auto& value) { return value.b; }, desc);
    if (!impl_->accepts(a) || !impl_->accepts(b)) {
        return {{}, {StatusCode::InvalidHandle, "joint body handle is not live"}};
    }
    const Status validation = validate_joint_desc(desc);
    if (!validation.ok()) {
        return {{}, validation};
    }
    if (impl_->reserved_joint_count >= Impl::max_joints) {
        return {{}, {StatusCode::CapacityExceeded, "physics joint capacity has been reached"}};
    }

    Impl::CreateJointCommand command{{}, desc};
    const Impl::JointReservation reservation = impl_->reserve_joint_handle();
    command.handle = reservation.handle;
    Impl::JointSlot& slot = impl_->joint_slots[reservation.handle.index];
    slot.a = a;
    slot.b = b;
    try {
        impl_->commands.emplace_back(std::move(command));
    } catch (...) {
        impl_->rollback_reservation(reservation);
        throw;
    }
    impl_->commit_reservation(reservation);
    ++impl_->reserved_joint_count;
    return {reservation.handle, {}};
}

Status PhysicsWorld::destroy_joint(JointHandle joint)
{
    if (!impl_->accepts(joint)) {
        return invalid_joint_handle_status();
    }
    impl_->commands.emplace_back(Impl::DestroyJointCommand{joint});
    Impl::JointSlot& slot = impl_->joint_slots[joint.index];
    slot.state = Impl::SlotState::PendingDestroy;
    std::erase_if(
        impl_->joint_reaction_storage,
        [joint](const JointReaction& reaction) { return reaction.joint == joint; });
    return {};
}

Status PhysicsWorld::apply_force(BodyHandle body, Vec3 force, Vec3 point, bool wake)
{
    if (!impl_->accepts(body)) {
        return invalid_handle_status();
    }
    if (!is_finite(force) || !is_finite(point)) {
        return invalid_argument_status("force and application point must be finite");
    }
    impl_->commands.push_back(Impl::ForceCommand{body, force, point, wake});
    return {};
}

Status PhysicsWorld::apply_impulse(BodyHandle body, Vec3 impulse, Vec3 point, bool wake)
{
    if (!impl_->accepts(body)) {
        return invalid_handle_status();
    }
    if (!is_finite(impulse) || !is_finite(point)) {
        return invalid_argument_status("impulse and application point must be finite");
    }
    impl_->commands.push_back(Impl::ImpulseCommand{body, impulse, point, wake});
    return {};
}

Status PhysicsWorld::apply_central_impulses(
    std::span<const CentralImpulse> impulses, bool wake)
{
    for (std::size_t index = 0; index < impulses.size(); ++index) {
        const CentralImpulse& value = impulses[index];
        const Impl::Slot* slot = impl_->matching_slot(value.body);
        if (slot == nullptr || slot->state != Impl::SlotState::Live) {
            return invalid_handle_status();
        }
        if (slot->type != BodyType::Dynamic) {
            return invalid_argument_status(
                "central impulse batch accepts dynamic bodies only");
        }
        if (!is_finite(value.impulse)) {
            return invalid_argument_status("central impulses must be finite");
        }
        for (std::size_t earlier = 0; earlier < index; ++earlier) {
            if (impulses[earlier].body == value.body) {
                return invalid_argument_status(
                    "central impulse batch body handles must be unique");
            }
        }
    }
    impl_->commands.reserve(impl_->commands.size() + impulses.size());
    for (const CentralImpulse& value : impulses) {
        impl_->commands.emplace_back(
            Impl::CentralImpulseCommand{value.body, value.impulse, wake});
    }
    return {};
}

Status PhysicsWorld::set_body_mass_scale(BodyHandle body, float scale)
{
    Impl::Slot* slot = impl_->matching_slot(body);
    if (slot == nullptr || slot->state != Impl::SlotState::Live
        || B3_IS_NULL(slot->native) || !b3Body_IsValid(slot->native)) {
        return invalid_handle_status();
    }
    if (slot->type != BodyType::Dynamic) {
        return invalid_argument_status("mass scale requires a dynamic body");
    }
    if (!positive_finite(scale)) {
        return invalid_argument_status("mass scale must be finite and positive");
    }
    if (!slot->mass_scale_valid) {
        return {StatusCode::Box3DFault,
            "body mass scale metadata is unavailable after a failed rollback"};
    }
    const b3MassData& base = slot->base_mass_data;
    if (slot->shapes.empty() || !positive_finite(base.mass)
        || !std::isfinite(base.center.x) || !std::isfinite(base.center.y)
        || !std::isfinite(base.center.z)) {
        return {StatusCode::Box3DFault, "dynamic body has no valid base mass data"};
    }
    const auto scaled_value = [scale](float value, bool positive)
        -> std::optional<float> {
        const double scaled =
            static_cast<double>(value) * static_cast<double>(scale);
        if (!std::isfinite(scaled)
            || std::abs(scaled) > std::numeric_limits<float>::max()) {
            return std::nullopt;
        }
        const float narrowed = static_cast<float>(scaled);
        if (!std::isfinite(narrowed) || (positive && narrowed <= 0.0F)) {
            return std::nullopt;
        }
        return narrowed;
    };
    const std::array base_inertia_values{
        base.inertia.cx.x, base.inertia.cx.y, base.inertia.cx.z,
        base.inertia.cy.x, base.inertia.cy.y, base.inertia.cy.z,
        base.inertia.cz.x, base.inertia.cz.y, base.inertia.cz.z,
    };
    if (!scaled_value(base.mass, true)
        || !std::ranges::all_of(base_inertia_values, [&](float value) {
            return scaled_value(value, false).has_value();
        })) {
        return invalid_argument_status(
            "mass scale exceeds the supported float range");
    }
    for (const Impl::Slot::OwnedShape& shape : slot->shapes) {
        if (B3_IS_NULL(shape.native) || !b3Shape_IsValid(shape.native)
            || !positive_finite(shape.base_density)
            || !scaled_value(shape.base_density, true)) {
            return invalid_argument_status(
                "mass scale cannot be applied to every owned shape");
        }
    }
    if (slot->mass_scale == scale) {
        return {};
    }
    const b3MassData previous_mass_data = b3Body_GetMassData(slot->native);
    const b3Vec3 previous_linear_velocity = b3Body_GetLinearVelocity(slot->native);
    const b3Vec3 previous_angular_velocity = b3Body_GetAngularVelocity(slot->native);
    const bool previous_awake = b3Body_IsAwake(slot->native);
    const float previous_scale = slot->mass_scale;
    std::vector<float> previous_densities;
    std::vector<float> target_densities;
    previous_densities.reserve(slot->shapes.size());
    target_densities.reserve(slot->shapes.size());
    for (const Impl::Slot::OwnedShape& shape : slot->shapes) {
        previous_densities.push_back(b3Shape_GetDensity(shape.native));
        target_densities.push_back(static_cast<float>(
            static_cast<double>(shape.base_density) * static_cast<double>(scale)));
    }
    for (std::size_t index = 0; index < slot->shapes.size(); ++index) {
        b3Shape_SetDensity(slot->shapes[index].native, target_densities[index], false);
    }
    b3Body_ApplyMassFromShapes(slot->native);
    b3Body_SetLinearVelocity(slot->native, previous_linear_velocity);
    b3Body_SetAngularVelocity(slot->native, previous_angular_velocity);
    b3Body_SetAwake(slot->native, previous_awake);
#if defined(NINHO_ENABLE_TEST_FACADES)
    if (impl_->fail_next_mass_scale_postcondition_for_testing) {
        impl_->fail_next_mass_scale_postcondition_for_testing = false;
        b3MassData corrupted = b3Body_GetMassData(slot->native);
        corrupted.center.x += 0.25F;
        b3Body_SetMassData(slot->native, corrupted);
    }
#endif
    const auto close_value = [](float actual, float expected, double tolerance) {
        return std::isfinite(actual)
            && std::abs(static_cast<double>(actual) - static_cast<double>(expected))
                <= tolerance;
    };
    const auto close_vector = [&](b3Vec3 actual, b3Vec3 expected, double tolerance) {
        return close_value(actual.x, expected.x, tolerance)
            && close_value(actual.y, expected.y, tolerance)
            && close_value(actual.z, expected.z, tolerance);
    };
    const auto inertia_values = [](const b3Matrix3& inertia) {
        return std::array{
            inertia.cx.x, inertia.cx.y, inertia.cx.z,
            inertia.cy.x, inertia.cy.y, inertia.cy.z,
            inertia.cz.x, inertia.cz.y, inertia.cz.z,
        };
    };
    const b3MassData updated = b3Body_GetMassData(slot->native);
    const auto updated_inertia = inertia_values(updated.inertia);
    const auto base_inertia = inertia_values(slot->base_mass_data.inertia);
    const auto close_scaled = [scale](float actual, float base) {
        const double expected = static_cast<double>(base) * static_cast<double>(scale);
        const double tolerance = std::max(1.0e-5, std::abs(expected) * 5.0e-5);
        return std::isfinite(actual)
            && std::abs(static_cast<double>(actual) - expected) <= tolerance;
    };
    bool densities_match = true;
    for (std::size_t index = 0; index < slot->shapes.size(); ++index) {
        densities_match = densities_match
            && b3Shape_GetDensity(slot->shapes[index].native) == target_densities[index];
    }
    const bool valid_result = positive_finite(updated.mass)
        && close_scaled(updated.mass, slot->base_mass_data.mass)
        && std::ranges::equal(updated_inertia, base_inertia, close_scaled)
        && close_vector(updated.center, previous_mass_data.center, 1.0e-5)
        && close_vector(updated.center, slot->base_mass_data.center, 1.0e-5)
        && close_vector(b3Body_GetLinearVelocity(slot->native),
            previous_linear_velocity, 1.0e-5)
        && close_vector(b3Body_GetAngularVelocity(slot->native),
            previous_angular_velocity, 1.0e-5)
        && b3Body_IsAwake(slot->native) == previous_awake && densities_match;
    if (!valid_result) {
        for (std::size_t index = 0; index < slot->shapes.size(); ++index) {
            b3Shape_SetDensity(slot->shapes[index].native,
                previous_densities[index], false);
        }
        b3Body_SetMassData(slot->native, previous_mass_data);
        b3Body_SetLinearVelocity(slot->native, previous_linear_velocity);
        b3Body_SetAngularVelocity(slot->native, previous_angular_velocity);
        b3Body_SetAwake(slot->native, previous_awake);
        const b3MassData rolled_back = b3Body_GetMassData(slot->native);
        const auto rolled_back_inertia = inertia_values(rolled_back.inertia);
        const auto previous_inertia = inertia_values(previous_mass_data.inertia);
        bool rollback_densities_match = true;
        for (std::size_t index = 0; index < slot->shapes.size(); ++index) {
            rollback_densities_match = rollback_densities_match
                && b3Shape_GetDensity(slot->shapes[index].native)
                    == previous_densities[index];
        }
        const bool rollback_valid =
            close_value(rolled_back.mass, previous_mass_data.mass, 1.0e-6)
            && std::ranges::equal(rolled_back_inertia, previous_inertia,
                [&](float actual, float expected) {
                    return close_value(actual, expected, 1.0e-6);
                })
            && close_vector(rolled_back.center, previous_mass_data.center, 1.0e-6)
            && close_vector(b3Body_GetLinearVelocity(slot->native),
                previous_linear_velocity, 1.0e-6)
            && close_vector(b3Body_GetAngularVelocity(slot->native),
                previous_angular_velocity, 1.0e-6)
            && b3Body_IsAwake(slot->native) == previous_awake
            && rollback_densities_match && slot->mass_scale == previous_scale;
        if (!rollback_valid) {
            slot->mass_scale_valid = false;
            impl_->rebuild_snapshots();
            return {StatusCode::Box3DFault,
                "FATAL: Box3D body mass rollback validation failed"};
        }
        impl_->rebuild_snapshots();
        return {StatusCode::Box3DFault,
            "Box3D produced invalid mass properties while scaling body mass"};
    }
    slot->mass_scale = scale;
    slot->mass_scale_valid = true;
    impl_->rebuild_snapshots();
    return {};
}

Status PhysicsWorld::set_body_collision_group(BodyHandle body, int collision_group)
{
    Impl::Slot* slot = impl_->matching_slot(body);
    if (slot == nullptr || slot->state != Impl::SlotState::Live
        || B3_IS_NULL(slot->native) || !b3Body_IsValid(slot->native)) {
        return invalid_handle_status();
    }
    if (slot->shapes.empty()
        || !std::ranges::all_of(slot->shapes, [](const Impl::Slot::OwnedShape& shape) {
            return B3_IS_NON_NULL(shape.native) && b3Shape_IsValid(shape.native);
        })) {
        return {StatusCode::Box3DFault,
            "body collision group cannot be applied to every owned shape"};
    }
    for (const Impl::Slot::OwnedShape& shape : slot->shapes) {
        b3Filter filter = b3Shape_GetFilter(shape.native);
        filter.groupIndex = collision_group;
        b3Shape_SetFilter(shape.native, filter, true);
    }
    return {};
}

Status PhysicsWorld::commit_pending_initial_state()
{
    if (impl_->initialization_faulted) {
        return {StatusCode::Box3DFault,
            "initialization previously failed after command application began"};
    }
    if (impl_->initial_state_committed || impl_->step_called) {
        return invalid_argument_status(
            "initial state can be committed exactly once before the first step");
    }
    const bool only_creations = std::ranges::all_of(impl_->commands, [](const auto& command) {
        return std::holds_alternative<Impl::CreateCommand>(command)
            || std::holds_alternative<Impl::CreateJointCommand>(command);
    });
    if (!only_creations) {
        return invalid_argument_status(
            "initial state accepts only pending body and joint creations");
    }
    try {
        std::size_t applied_count = 0;
        for (const Impl::Command& command : impl_->commands) {
#if defined(NINHO_ENABLE_TEST_FACADES)
            if (impl_->fail_initial_commit_after_for_testing == applied_count) {
                throw std::runtime_error("injected initial commit failure");
            }
#endif
            impl_->apply(command);
            ++applied_count;
        }
        impl_->commands.clear();
        impl_->rebuild_snapshots();
    } catch (const std::exception& error) {
        impl_->initialization_faulted = true;
        return {StatusCode::Box3DFault, error.what()};
    } catch (...) {
        impl_->initialization_faulted = true;
        return {StatusCode::Box3DFault, "unknown failure while committing initial state"};
    }
    impl_->contact_storage.clear();
    impl_->joint_reaction_storage.clear();
    impl_->initial_state_committed = true;
    return {};
}

void PhysicsWorld::step()
{
    if (impl_->initialization_faulted) {
        throw std::logic_error(
            "cannot step a world whose initial commit partially failed");
    }
    impl_->step_called = true;
    const auto step_start = std::chrono::steady_clock::now();
    impl_->apply_queued_commands();

    for (std::uint32_t index = 1; index < impl_->slots.size(); ++index) {
        Impl::Slot& slot = impl_->slots[index];
        if (slot.state != Impl::SlotState::Live || slot.type != BodyType::Dynamic
            || B3_IS_NULL(slot.native) || !b3Body_IsValid(slot.native)
            || !b3Body_IsAwake(slot.native)) {
            continue;
        }
        const Transform transform = detail::from_box3d_world(b3Body_GetTransform(slot.native));
        const Vec3 force = impl_->gravity.acceleration_at(transform.position) * b3Body_GetMass(slot.native);
        b3Body_ApplyForceToCenter(slot.native, detail::to_box3d_vector(force), false);
    }

    b3World_Step(impl_->world, impl_->config.time_step, impl_->config.substeps);
    impl_->copy_contact_hits();
    impl_->copy_physical_contacts();
    impl_->copy_joint_reactions();
    impl_->rebuild_snapshots();
    impl_->update_world_exit_and_queue_removal();
    impl_->step_ms = std::chrono::duration<double, std::milli>(
                         std::chrono::steady_clock::now() - step_start)
                         .count();
}

std::optional<BodyState> PhysicsWorld::state(BodyHandle body) const
{
    const Impl::Slot* slot = impl_->matching_slot(body);
    if (slot == nullptr || slot->state == Impl::SlotState::Free) {
        return std::nullopt;
    }
    const auto found = std::find_if(
        impl_->snapshots.begin(),
        impl_->snapshots.end(),
        [body](const BodyState& value) { return value.handle == body; });
    return found == impl_->snapshots.end() ? std::nullopt : std::optional<BodyState>{*found};
}

std::optional<float> PhysicsWorld::structural_mass(BodyHandle body) const
{
    const Impl::Slot* slot = impl_->matching_slot(body);
    if (slot == nullptr || slot->state == Impl::SlotState::Free
        || slot->shapes.empty()) {
        return std::nullopt;
    }
    double total_mass = 0.0;
    for (const Impl::Slot::OwnedShape& shape : slot->shapes) {
        if (B3_IS_NULL(shape.native) || !b3Shape_IsValid(shape.native)) {
            return std::nullopt;
        }
        const b3MassData mass_data = b3Shape_ComputeMassData(shape.native);
        if (!std::isfinite(mass_data.mass) || mass_data.mass < 0.0f) {
            return std::nullopt;
        }
        total_mass += mass_data.mass;
    }
    const float narrowed = static_cast<float>(total_mass);
    if (!std::isfinite(total_mass) || !std::isfinite(narrowed)) {
        return std::nullopt;
    }
    return narrowed;
}

std::span<const BodyState> PhysicsWorld::states() const
{
    return impl_->snapshots;
}

std::vector<QueryHit> PhysicsWorld::overlap_shape(
    const QueryShape& shape, Transform transform) const
{
    if (!valid_transform(transform) || !validate_primitive(shape).ok()) {
        return {};
    }
    return impl_->overlap(shape, transform);
}

std::optional<QueryHit> PhysicsWorld::cast_shape(
    const QueryShape& shape, Transform transform, Vec3 translation) const
{
    const float translation_length = length(translation);
    if (!valid_transform(transform) || !validate_primitive(shape).ok()
        || !is_finite(translation) || !positive_finite(translation_length)) {
        return std::nullopt;
    }
    return impl_->cast(shape, transform, translation);
}

std::vector<QueryHit> PhysicsWorld::overlap_sphere(Vec3 center, float radius) const
{
    return overlap_shape(SphereShape{.radius = radius}, {center, {}});
}

std::optional<QueryHit> PhysicsWorld::cast_sphere(
    Vec3 center, float radius, Vec3 translation) const
{
    return cast_shape(SphereShape{.radius = radius}, {center, {}}, translation);
}

std::optional<QueryHit> PhysicsWorld::cast_segment(Vec3 origin, Vec3 target,
    std::span<const BodyHandle> ignored_handles) const
{
    if (!is_finite(origin) || !is_finite(target)
        || !positive_finite(length(target - origin))) {
        return std::nullopt;
    }
    if (std::ranges::any_of(ignored_handles,
            [](BodyHandle handle) { return !handle.valid(); })) {
        return std::nullopt;
    }
    return impl_->cast_segment(origin, target, ignored_handles);
}

std::optional<Aabb> PhysicsWorld::body_bounds(BodyHandle body) const
{
    const Impl::Slot* slot = impl_->matching_slot(body);
    if (slot == nullptr || slot->state != Impl::SlotState::Live) {
        return std::nullopt;
    }

    std::optional<Aabb> result;
    for (const Impl::ShapeBinding& binding : impl_->shape_bindings) {
        if (binding.body != body) {
            continue;
        }
        const b3ShapeId shape = b3LoadShapeId(binding.native_key);
        if (!b3Shape_IsValid(shape)) {
            continue;
        }
        const b3AABB native_bounds = b3Shape_GetAABB(shape);
        const Aabb bounds{
            detail::from_box3d_vector(native_bounds.lowerBound),
            detail::from_box3d_vector(native_bounds.upperBound),
        };
        if (!is_finite(bounds.lower) || !is_finite(bounds.upper)) {
            continue;
        }
        if (!result.has_value()) {
            result = bounds;
            continue;
        }
        result->lower.x = std::min(result->lower.x, bounds.lower.x);
        result->lower.y = std::min(result->lower.y, bounds.lower.y);
        result->lower.z = std::min(result->lower.z, bounds.lower.z);
        result->upper.x = std::max(result->upper.x, bounds.upper.x);
        result->upper.y = std::max(result->upper.y, bounds.upper.y);
        result->upper.z = std::max(result->upper.z, bounds.upper.z);
    }
    return result;
}

std::span<const ContactHit> PhysicsWorld::contact_hits() const
{
    return impl_->contact_storage;
}

std::span<const PhysicalContact> PhysicsWorld::physical_contacts() const
{
    return impl_->physical_contact_storage;
}

std::span<const JointReaction> PhysicsWorld::joint_reactions() const
{
    return impl_->joint_reaction_storage;
}

std::optional<JointReaction> PhysicsWorld::joint_reaction(JointHandle joint) const
{
    if (!impl_->accepts(joint)) {
        return std::nullopt;
    }
    const auto found = std::find_if(
        impl_->joint_reaction_storage.begin(),
        impl_->joint_reaction_storage.end(),
        [joint](const JointReaction& reaction) { return reaction.joint == joint; });
    return found == impl_->joint_reaction_storage.end()
        ? std::nullopt
        : std::optional<JointReaction>{*found};
}

WorldMetrics PhysicsWorld::metrics() const
{
    WorldMetrics result{
        .contact_count = impl_->live_contact_count(),
        .step_ms = impl_->step_ms,
    };
    for (std::uint32_t index = 1; index < impl_->slots.size(); ++index) {
        const Impl::Slot& slot = impl_->slots[index];
        if (slot.state != Impl::SlotState::Live) {
            continue;
        }
        ++result.body_count;
        if (B3_IS_NON_NULL(slot.native) && b3Body_IsValid(slot.native)
            && b3Body_IsAwake(slot.native)) {
            ++result.awake_count;
        }
    }
    for (const Impl::ShapeBinding& binding : impl_->shape_bindings) {
        const Impl::Slot* slot = impl_->matching_slot(binding.body);
        if (slot != nullptr && slot->state == Impl::SlotState::Live) {
            ++result.shape_count;
        }
    }
    for (std::uint32_t index = 1; index < impl_->joint_slots.size(); ++index) {
        if (impl_->joint_slots[index].state == Impl::SlotState::Live) {
            ++result.joint_count;
        }
    }
    return result;
}

std::size_t PhysicsWorld::remaining_body_capacity() const noexcept
{
    return impl_->config.max_bodies - impl_->reserved_body_count;
}

Vec3 PhysicsWorld::gravity_at(Vec3 position) const noexcept
{
    return impl_->gravity.acceleration_at(position);
}

const WorldConfig& PhysicsWorld::config() const
{
    return impl_->config;
}

#if defined(NINHO_ENABLE_TEST_FACADES)
int detail::PhysicsWorldTestFacade::worker_count(const PhysicsWorld& world)
{
    return b3World_GetWorkerCount(world.impl_->world);
}

Vec3 detail::PhysicsWorldTestFacade::box3d_gravity(const PhysicsWorld& world)
{
    return detail::from_box3d_vector(b3World_GetGravity(world.impl_->world));
}

int detail::PhysicsWorldTestFacade::gravity_strategy(const PhysicsWorld& world)
{
    return world.impl_->gravity.evaluator_ == &GravityField::evaluate_uniform ? 0 : 1;
}

int detail::PhysicsWorldTestFacade::bounds_strategy(const PhysicsWorld& world)
{
    if (world.impl_->world_exit_tracker.bounds_.evaluator_
        == &WorldBounds::contains_everywhere) {
        return 0;
    }
    return world.impl_->world_exit_tracker.bounds_.evaluator_
            == &WorldBounds::contains_aabb
        ? 1
        : 2;
}

int detail::PhysicsWorldTestFacade::ejection_strategy(const PhysicsWorld& world)
{
    return world.impl_->world_exit_tracker.ejection_evaluator_
            == &detail::WorldExitTracker::disabled_ejection
        ? 0
        : 1;
}

std::array<float, 9> detail::PhysicsWorldTestFacade::local_inertia(
    const PhysicsWorld& world, BodyHandle handle)
{
    const PhysicsWorld::Impl::Slot* slot = world.impl_->matching_slot(handle);
    if (slot == nullptr || slot->state != PhysicsWorld::Impl::SlotState::Live
        || B3_IS_NULL(slot->native) || !b3Body_IsValid(slot->native)) {
        return {};
    }
    const b3Matrix3 value = b3Body_GetLocalRotationalInertia(slot->native);
    return {value.cx.x, value.cx.y, value.cx.z,
        value.cy.x, value.cy.y, value.cy.z,
        value.cz.x, value.cz.y, value.cz.z};
}

Vec3 detail::PhysicsWorldTestFacade::local_center(
    const PhysicsWorld& world, BodyHandle handle)
{
    const PhysicsWorld::Impl::Slot* slot = world.impl_->matching_slot(handle);
    if (slot == nullptr || slot->state != PhysicsWorld::Impl::SlotState::Live
        || B3_IS_NULL(slot->native) || !b3Body_IsValid(slot->native)) {
        return {};
    }
    return detail::from_box3d_vector(b3Body_GetLocalCenterOfMass(slot->native));
}

std::vector<std::uint64_t> detail::PhysicsWorldTestFacade::shape_keys(
    const PhysicsWorld& world, BodyHandle handle)
{
    const PhysicsWorld::Impl::Slot* slot = world.impl_->matching_slot(handle);
    std::vector<std::uint64_t> result;
    if (slot == nullptr || slot->state != PhysicsWorld::Impl::SlotState::Live) {
        return result;
    }
    result.reserve(slot->shapes.size());
    for (const PhysicsWorld::Impl::Slot::OwnedShape& shape : slot->shapes) {
        result.push_back(b3StoreShapeId(shape.native));
    }
    return result;
}

std::vector<float> detail::PhysicsWorldTestFacade::shape_densities(
    const PhysicsWorld& world, BodyHandle handle)
{
    const PhysicsWorld::Impl::Slot* slot = world.impl_->matching_slot(handle);
    std::vector<float> result;
    if (slot == nullptr || slot->state != PhysicsWorld::Impl::SlotState::Live) {
        return result;
    }
    result.reserve(slot->shapes.size());
    for (const PhysicsWorld::Impl::Slot::OwnedShape& shape : slot->shapes) {
        result.push_back(B3_IS_NON_NULL(shape.native) && b3Shape_IsValid(shape.native)
            ? b3Shape_GetDensity(shape.native) : 0.0f);
    }
    return result;
}

std::vector<float> detail::PhysicsWorldTestFacade::base_shape_densities(
    const PhysicsWorld& world, BodyHandle handle)
{
    const PhysicsWorld::Impl::Slot* slot = world.impl_->matching_slot(handle);
    std::vector<float> result;
    if (slot == nullptr || slot->state != PhysicsWorld::Impl::SlotState::Live) {
        return result;
    }
    result.reserve(slot->shapes.size());
    std::ranges::transform(slot->shapes, std::back_inserter(result),
        [](const PhysicsWorld::Impl::Slot::OwnedShape& shape) {
            return shape.base_density;
        });
    return result;
}

std::vector<int> detail::PhysicsWorldTestFacade::shape_collision_groups(
    const PhysicsWorld& world, BodyHandle handle)
{
    const PhysicsWorld::Impl::Slot* slot = world.impl_->matching_slot(handle);
    std::vector<int> result;
    if (slot == nullptr || slot->state != PhysicsWorld::Impl::SlotState::Live) {
        return result;
    }
    result.reserve(slot->shapes.size());
    for (const PhysicsWorld::Impl::Slot::OwnedShape& shape : slot->shapes) {
        result.push_back(B3_IS_NON_NULL(shape.native) && b3Shape_IsValid(shape.native)
            ? b3Shape_GetFilter(shape.native).groupIndex : 0);
    }
    return result;
}

std::vector<std::array<std::uint64_t, 2>>
detail::PhysicsWorldTestFacade::shape_collision_bits(
    const PhysicsWorld& world, BodyHandle handle)
{
    const PhysicsWorld::Impl::Slot* slot = world.impl_->matching_slot(handle);
    std::vector<std::array<std::uint64_t, 2>> result;
    if (slot == nullptr || slot->state != PhysicsWorld::Impl::SlotState::Live) {
        return result;
    }
    result.reserve(slot->shapes.size());
    for (const PhysicsWorld::Impl::Slot::OwnedShape& shape : slot->shapes) {
        if (B3_IS_NULL(shape.native) || !b3Shape_IsValid(shape.native)) {
            result.push_back({});
            continue;
        }
        const b3Filter filter = b3Shape_GetFilter(shape.native);
        result.push_back({filter.categoryBits, filter.maskBits});
    }
    return result;
}

float detail::PhysicsWorldTestFacade::base_mass(
    const PhysicsWorld& world, BodyHandle handle)
{
    const PhysicsWorld::Impl::Slot* slot = world.impl_->matching_slot(handle);
    return slot != nullptr && slot->state == PhysicsWorld::Impl::SlotState::Live
        ? slot->base_mass_data.mass : 0.0f;
}

float detail::PhysicsWorldTestFacade::mass_scale(
    const PhysicsWorld& world, BodyHandle handle)
{
    const PhysicsWorld::Impl::Slot* slot = world.impl_->matching_slot(handle);
    return slot != nullptr && slot->state == PhysicsWorld::Impl::SlotState::Live
            && slot->mass_scale_valid
        ? slot->mass_scale : 0.0f;
}

float detail::PhysicsWorldTestFacade::raw_total_normal_impulse_once(
    const PhysicsWorld& world, BodyHandle a, BodyHandle b)
{
    const PhysicsWorld::Impl::Slot* slot = world.impl_->matching_slot(a);
    if (slot == nullptr || slot->state != PhysicsWorld::Impl::SlotState::Live
        || B3_IS_NULL(slot->native) || !b3Body_IsValid(slot->native)) {
        return 0.0f;
    }
    const int capacity = b3Body_GetContactCapacity(slot->native);
    std::vector<b3ContactData> contacts(static_cast<std::size_t>(std::max(0, capacity)));
    const int count = capacity > 0
        ? b3Body_GetContactData(slot->native, contacts.data(), capacity) : 0;
    float total = 0.0f;
    for (int index = 0; index < count; ++index) {
        const b3ContactData& contact = contacts[index];
        const auto* binding_a = world.impl_->find_shape_binding(contact.shapeIdA);
        const auto* binding_b = world.impl_->find_shape_binding(contact.shapeIdB);
        if (binding_a == nullptr || binding_b == nullptr
            || !((binding_a->body == a && binding_b->body == b)
                || (binding_a->body == b && binding_b->body == a))) {
            continue;
        }
        for (int manifold_index = 0; manifold_index < contact.manifoldCount;
            ++manifold_index) {
            const b3Manifold& manifold = contact.manifolds[manifold_index];
            for (int point_index = 0; point_index < manifold.pointCount; ++point_index) {
                total += std::max(0.0f,
                    manifold.points[point_index].totalNormalImpulse);
            }
        }
    }
    return total;
}

void detail::PhysicsWorldTestFacade::set_base_density(
    PhysicsWorld& world, BodyHandle handle, std::size_t shape_index, float density)
{
    PhysicsWorld::Impl::Slot* slot = world.impl_->matching_slot(handle);
    if (slot != nullptr && slot->state == PhysicsWorld::Impl::SlotState::Live
        && shape_index < slot->shapes.size()) {
        slot->shapes[shape_index].base_density = density;
    }
}

void detail::PhysicsWorldTestFacade::fail_next_mass_scale_postcondition(
    PhysicsWorld& world)
{
    world.impl_->fail_next_mass_scale_postcondition_for_testing = true;
}

void detail::PhysicsWorldTestFacade::fail_initial_commit_after(
    PhysicsWorld& world, std::size_t applied_command_count)
{
    world.impl_->fail_initial_commit_after_for_testing = applied_command_count;
}
#endif

}
