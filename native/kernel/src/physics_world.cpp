#include <ninho/physics/physics_world.hpp>

#include "box3d_conversions.hpp"
#if defined(NINHO_ENABLE_TEST_FACADES)
#include "physics_world_test_facade.hpp"
#endif

#include <ninho/physics/radial_gravity.hpp>

#include <box3d/box3d.h>

#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <cstdint>
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
    if (!positive_finite(config.planet_radius)) {
        throw std::invalid_argument("planet_radius must be finite and positive");
    }
    if (!nonnegative_finite(config.surface_gravity)) {
        throw std::invalid_argument("surface_gravity must be finite and non-negative");
    }
    const double gravity_scale = static_cast<double>(config.surface_gravity)
        * static_cast<double>(config.planet_radius) * static_cast<double>(config.planet_radius);
    const double ejection_radius = 6.0 * static_cast<double>(config.planet_radius);
    if (gravity_scale > std::numeric_limits<float>::max()
        || ejection_radius > std::numeric_limits<float>::max()) {
        throw std::invalid_argument("radial gravity configuration exceeds finite float range");
    }
    constexpr std::size_t maximum_public_slots =
        static_cast<std::size_t>(std::numeric_limits<std::uint32_t>::max()) - 1U;
    if (config.max_bodies == 0 || config.max_bodies > maximum_public_slots) {
        throw std::invalid_argument("max_bodies is outside the supported handle range");
    }
    return config;
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

BodyDesc BodyDesc::static_sphere(float radius, Transform transform)
{
    BodyDesc body;
    body.type = BodyType::Static;
    body.transform = transform;
    body.radial_gravity = false;
    body.remove_beyond_six_r = false;
    body.shapes.push_back(make_shape(SphereShape{radius, {}}, 1.0f));
    return body;
}

BodyDesc BodyDesc::static_box(Vec3 half_extents, Transform transform)
{
    BodyDesc body;
    body.type = BodyType::Static;
    body.transform = transform;
    body.radial_gravity = false;
    body.remove_beyond_six_r = false;
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
        std::uint32_t generation{};
        SlotState state{SlotState::Free};
        b3BodyId native{};
        BodyType type{BodyType::Static};
        bool radial_gravity{};
        bool remove_beyond_six_r{};
        bool ejected{};
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
        ImpulseCommand>;

    static_assert(std::is_nothrow_move_assignable_v<Command>);
    static_assert(std::is_nothrow_destructible_v<Command>);

    explicit Impl(WorldConfig world_config)
        : config(validated_config(world_config))
        , gravity({.center = {},
                   .radius = config.planet_radius,
                   .surface_acceleration = config.surface_gravity})
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
            ejection_tracker.reset({index, slot.generation});
        }
        ++slot.generation;
        if (slot.generation == 0) {
            ++slot.generation;
        }
        slot.state = SlotState::PendingCreate;
        slot.native = {};
        slot.ejected = false;
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
        slot.type = BodyType::Static;
        slot.radial_gravity = false;
        slot.remove_beyond_six_r = false;
        slot.ejected = false;
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
        ejection_tracker.reset(handle);
        slot->native = {};
        slot->state = SlotState::Free;
        slot->type = BodyType::Static;
        slot->radial_gravity = false;
        slot->remove_beyond_six_r = false;
        slot->ejected = false;
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
        std::uint64_t material_id)
    {
        const NativeShapeResult created = create_native_primitive(native, shape_def, primitive);
        if (B3_IS_NULL(created.id) || !b3Shape_IsValid(created.id)) {
            fail_native_create(handle, native, created.operation);
        }
        shape_bindings.push_back({b3StoreShapeId(created.id), handle, material_id});
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

            if (const auto* compound = std::get_if<CompoundShape>(&shape.geometry)) {
                for (const PrimitiveShape& child : compound->children) {
                    attach_primitive(
                        native, command.handle, shape_def, child, shape.material_id);
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
                                shape.material_id);
                        }
                    },
                    shape.geometry);
            }
        }

        slot->native = native;
        slot->type = desc.type;
        slot->radial_gravity = desc.radial_gravity;
        slot->remove_beyond_six_r = desc.remove_beyond_six_r;
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
        std::vector<std::array<std::uint32_t, 3>> contact_ids;
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
            std::vector<b3ContactData> contacts(static_cast<std::size_t>(capacity));
            const int count = b3Body_GetContactData(slot.native, contacts.data(), capacity);
            for (int contact_index = 0; contact_index < count; ++contact_index) {
                const b3ContactData& contact = contacts[contact_index];
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
                contact_ids.push_back(key);
            }
        }
        std::ranges::sort(contact_ids);
        return static_cast<int>(std::ranges::unique(contact_ids).begin() - contact_ids.begin());
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
                .linear_velocity = detail::from_box3d_vector(b3Body_GetLinearVelocity(slot.native)),
                .angular_velocity = detail::from_box3d_vector(b3Body_GetAngularVelocity(slot.native)),
                .mass = b3Body_GetMass(slot.native),
                .awake = b3Body_IsAwake(slot.native),
                .ejected = slot.ejected,
            });
        }
    }

    void update_ejection_and_queue_removal()
    {
        const float removal_radius = 6.0f * config.planet_radius;
        for (BodyState& snapshot : snapshots) {
            Slot* slot = matching_slot(snapshot.handle);
            if (slot == nullptr || slot->state != SlotState::Live) {
                continue;
            }

            const float radius = length(snapshot.transform.position);
            const Vec3 radial_direction = normalized_or_zero(snapshot.transform.position);
            const float radial_speed = dot(snapshot.linear_velocity, radial_direction);
            if (!slot->ejected
                && ejection_tracker.update(
                    snapshot.handle, radius, radial_speed, config.time_step, config.planet_radius)) {
                slot->ejected = true;
            }
            snapshot.ejected = slot->ejected;

            if (slot->remove_beyond_six_r && radius >= removal_radius) {
                commands.push_back(DestroyCommand{snapshot.handle});
                transition_body_to_pending_destroy(snapshot.handle);
            }
        }
    }

    WorldConfig config;
    RadialGravity gravity;
    b3WorldId world{};
    std::vector<Slot> slots;
    std::vector<std::uint32_t> free_slots;
    std::vector<JointSlot> joint_slots;
    std::vector<std::uint32_t> free_joint_slots;
    std::vector<ShapeBinding> shape_bindings;
    std::vector<Command> commands;
    std::vector<BodyState> snapshots;
    std::vector<ContactHit> contact_storage;
    std::vector<JointReaction> joint_reaction_storage;
    std::size_t reserved_body_count{};
    std::size_t reserved_joint_count{};
    bool initial_state_committed{};
    bool initialization_faulted{};
    bool step_called{};
#if defined(NINHO_ENABLE_TEST_FACADES)
    std::optional<std::size_t> fail_initial_commit_after_for_testing;
#endif
    double step_ms{};
    EjectionTracker ejection_tracker;
};

PhysicsWorld::PhysicsWorld(WorldConfig config)
    : impl_(std::make_unique<Impl>(config))
{
}

PhysicsWorld::~PhysicsWorld() = default;
PhysicsWorld::PhysicsWorld(PhysicsWorld&&) noexcept = default;
PhysicsWorld& PhysicsWorld::operator=(PhysicsWorld&&) noexcept = default;

Result<BodyHandle> PhysicsWorld::create_body(const BodyDesc& desc)
{
    const Status validation = validate_body(desc);
    if (!validation.ok()) {
        return {{}, validation};
    }
    if (impl_->reserved_body_count >= impl_->config.max_bodies) {
        return {{}, {StatusCode::CapacityExceeded, "physics body capacity has been reached"}};
    }

    Impl::CreateCommand command{{}, desc};
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
            || !slot.radial_gravity || B3_IS_NULL(slot.native) || !b3Body_IsValid(slot.native)
            || !b3Body_IsAwake(slot.native)) {
            continue;
        }
        const Transform transform = detail::from_box3d_world(b3Body_GetTransform(slot.native));
        const Vec3 force = impl_->gravity.acceleration(transform.position) * b3Body_GetMass(slot.native);
        b3Body_ApplyForceToCenter(slot.native, detail::to_box3d_vector(force), false);
    }

    b3World_Step(impl_->world, impl_->config.time_step, impl_->config.substeps);
    impl_->copy_contact_hits();
    impl_->copy_joint_reactions();
    impl_->rebuild_snapshots();
    impl_->update_ejection_and_queue_removal();
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

const WorldConfig& PhysicsWorld::config() const
{
    return impl_->config;
}

#if defined(NINHO_ENABLE_TEST_FACADES)
void detail::PhysicsWorldTestFacade::fail_initial_commit_after(
    PhysicsWorld& world, std::size_t applied_command_count)
{
    world.impl_->fail_initial_commit_after_for_testing = applied_command_count;
}
#endif

}
