#include <ninho/physics/physics_world.hpp>

#include "box3d_conversions.hpp"

#include <ninho/physics/radial_gravity.hpp>

#include <box3d/box3d.h>

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <limits>
#include <stdexcept>
#include <string>
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
        if (std::holds_alternative<HullShape>(shape.geometry)
            || std::holds_alternative<CompoundShape>(shape.geometry)) {
            return {StatusCode::Unsupported, "hull and compound shapes are reserved for a later task"};
        }
        if (!positive_finite(shape.density)) {
            return invalid_argument_status("shape density must be finite and positive");
        }
        if (!nonnegative_finite(shape.friction) || !nonnegative_finite(shape.restitution)) {
            return invalid_argument_status("shape friction and restitution must be finite and non-negative");
        }

        const Status geometry_status = std::visit(
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
                    return {StatusCode::Unsupported,
                            "hull and compound shapes are reserved for a later task"};
                }
            },
            shape.geometry);
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

    struct Slot {
        std::uint32_t generation{};
        SlotState state{SlotState::Free};
        b3BodyId native{};
        BodyType type{BodyType::Static};
        bool radial_gravity{};
        bool remove_beyond_six_r{};
        bool ejected{};
    };

    struct CreateCommand {
        BodyHandle handle;
        BodyDesc desc;
    };

    struct DestroyCommand {
        BodyHandle handle;
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

    using Command = std::variant<CreateCommand, DestroyCommand, ForceCommand, ImpulseCommand>;

    explicit Impl(WorldConfig world_config)
        : config(validated_config(world_config))
        , gravity({.center = {},
                   .radius = config.planet_radius,
                   .surface_acceleration = config.surface_gravity})
    {
        b3WorldDef world_def = b3DefaultWorldDef();
        world_def.gravity = {};
        world_def.workerCount = 1;
        world = b3CreateWorld(&world_def);
        if (B3_IS_NULL(world) || !b3World_IsValid(world)) {
            throw std::runtime_error("Box3D failed to create the physics world");
        }

        constexpr std::size_t initial_reserve_limit = 1024;
        slots.reserve(std::min(config.max_bodies + 1, initial_reserve_limit));
        slots.emplace_back();
        snapshots.reserve(std::min(config.max_bodies, initial_reserve_limit));
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

    [[nodiscard]] BodyHandle reserve_handle()
    {
        std::uint32_t index{};
        if (!free_slots.empty()) {
            index = free_slots.back();
            free_slots.pop_back();
        } else {
            index = static_cast<std::uint32_t>(slots.size());
            slots.emplace_back();
        }

        Slot& slot = slots[index];
        ++slot.generation;
        if (slot.generation == 0) {
            ++slot.generation;
        }
        slot.state = SlotState::PendingCreate;
        slot.native = {};
        slot.ejected = false;
        return {index, slot.generation};
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
        for (const ShapeDesc& shape : desc.shapes) {
            b3ShapeDef shape_def = b3DefaultShapeDef();
            shape_def.density = shape.density;
            shape_def.baseMaterial.friction = shape.friction;
            shape_def.baseMaterial.restitution = shape.restitution;
            shape_def.baseMaterial.userMaterialId = shape.material_id;
            shape_def.enableHitEvents = shape.hit_events;

            std::visit(
                [&](const auto& geometry) {
                    using Geometry = std::decay_t<decltype(geometry)>;
                    if constexpr (std::is_same_v<Geometry, SphereShape>) {
                        const b3Sphere sphere{
                            detail::to_box3d_vector(geometry.local.position), geometry.radius};
                        b3CreateSphereShape(native, &shape_def, &sphere);
                    } else if constexpr (std::is_same_v<Geometry, BoxShape>) {
                        const b3BoxHull box = b3MakeTransformedBoxHull(
                            geometry.half_extents.x,
                            geometry.half_extents.y,
                            geometry.half_extents.z,
                            detail::to_box3d_local(geometry.local));
                        b3CreateHullShape(native, &shape_def, &box.base);
                    } else if constexpr (std::is_same_v<Geometry, CapsuleShape>) {
                        const b3Transform local = detail::to_box3d_local(geometry.local);
                        const b3Capsule capsule{
                            b3TransformPoint(local, {0.0f, -geometry.half_height, 0.0f}),
                            b3TransformPoint(local, {0.0f, geometry.half_height, 0.0f}),
                            geometry.radius,
                        };
                        b3CreateCapsuleShape(native, &shape_def, &capsule);
                    }
                },
                shape.geometry);
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
        if (B3_IS_NON_NULL(slot->native) && b3Body_IsValid(slot->native)) {
            b3DestroyBody(slot->native);
        }
        slot->native = {};
        slot->state = SlotState::Free;
        slot->radial_gravity = false;
        slot->remove_beyond_six_r = false;
        slot->ejected = false;
        free_slots.push_back(command.handle.index);
        --reserved_body_count;
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

    void queue_ejected_bodies()
    {
        const float removal_radius = 6.0f * config.planet_radius;
        for (BodyState& snapshot : snapshots) {
            Slot* slot = matching_slot(snapshot.handle);
            if (slot == nullptr || slot->state != SlotState::Live || !slot->remove_beyond_six_r
                || length(snapshot.transform.position) < removal_radius) {
                continue;
            }
            snapshot.ejected = true;
            slot->ejected = true;
            slot->state = SlotState::PendingDestroy;
            commands.push_back(DestroyCommand{snapshot.handle});
        }
    }

    WorldConfig config;
    RadialGravity gravity;
    b3WorldId world{};
    std::vector<Slot> slots;
    std::vector<std::uint32_t> free_slots;
    std::vector<Command> commands;
    std::vector<BodyState> snapshots;
    std::size_t reserved_body_count{};
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
    const BodyHandle handle = impl_->reserve_handle();
    ++impl_->reserved_body_count;
    impl_->commands.push_back(Impl::CreateCommand{handle, desc});
    return {handle, {}};
}

Status PhysicsWorld::destroy_body(BodyHandle body)
{
    if (!impl_->accepts(body)) {
        return invalid_handle_status();
    }
    Impl::Slot& slot = impl_->slots[body.index];
    slot.state = Impl::SlotState::PendingDestroy;
    impl_->commands.push_back(Impl::DestroyCommand{body});
    std::erase_if(impl_->snapshots, [body](const BodyState& value) { return value.handle == body; });
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

void PhysicsWorld::step()
{
    std::vector<Impl::Command> commands = std::move(impl_->commands);
    impl_->commands.clear();
    for (const Impl::Command& command : commands) {
        impl_->apply(command);
    }

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
    impl_->rebuild_snapshots();
    impl_->queue_ejected_bodies();
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

const WorldConfig& PhysicsWorld::config() const
{
    return impl_->config;
}

}
