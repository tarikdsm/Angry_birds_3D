#include "session_internal.hpp"
#include "content_semantic_validation.hpp"
#include "material_mapping.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <numbers>
#include <ranges>
#include <string>
#include <utility>
#include <vector>

namespace ninho::simulation {
namespace {

constexpr std::uint32_t child_ordinal_mask =
    ~detail::split_child_entity_namespace_begin;
constexpr std::size_t split_child_count = 3U;
constexpr double split_angle_deg = 11.0;

[[nodiscard]] SessionStatus split_unavailable(std::string message)
{
    return {{ContentErrorCode::InvalidInvariant, "/ability/split",
        std::move(message)}};
}

[[nodiscard]] SessionStatus split_failure(std::string message)
{
    return {{ContentErrorCode::InternalError, "/ability/split",
        std::move(message)}};
}

[[nodiscard]] bool unit_vector(ninho::physics::Vec3 value) noexcept
{
    return ninho::physics::is_finite(value)
        && std::abs(ninho::physics::length(value) - 1.0F) <= 1.0e-3F;
}

[[nodiscard]] bool valid_rotation(ninho::physics::Quat value) noexcept
{
    const float length_squared = value.x * value.x + value.y * value.y
        + value.z * value.z + value.w * value.w;
    return std::isfinite(value.x) && std::isfinite(value.y)
        && std::isfinite(value.z) && std::isfinite(value.w)
        && std::abs(length_squared - 1.0F) <= 1.0e-3F;
}

[[nodiscard]] bool valid_locked_plane(const LockedLaunchPlane& plane) noexcept
{
    if (!unit_vector(plane.camera_right) || !unit_vector(plane.up)
        || !unit_vector(plane.horizontal) || !unit_vector(plane.plane_normal)) {
        return false;
    }
    if (std::abs(ninho::physics::dot(plane.horizontal, plane.up)) > 1.0e-3F
        || std::abs(ninho::physics::dot(plane.horizontal, plane.plane_normal)) > 1.0e-3F
        || std::abs(ninho::physics::dot(plane.up, plane.plane_normal)) > 1.0e-3F) {
        return false;
    }
    return ninho::physics::length(
        ninho::physics::cross(plane.horizontal, plane.up)
            - plane.plane_normal) <= 2.0e-3F;
}

[[nodiscard]] std::optional<std::array<EntityId, split_child_count>> child_ids_for(
    const ShotState& shot) noexcept
{
    if (shot.shot_id == 0U) {
        return std::nullopt;
    }
    const std::uint64_t shot_ordinal = shot.shot_id - 1U;
    if (shot_ordinal
        > (child_ordinal_mask - (split_child_count - 1U))
            / split_child_count) {
        return std::nullopt;
    }
    const std::uint32_t first = static_cast<std::uint32_t>(
        shot_ordinal * split_child_count);
    return std::array{
        EntityId{detail::split_child_entity_namespace_begin | first},
        EntityId{detail::split_child_entity_namespace_begin | (first + 1U)},
        EntityId{detail::split_child_entity_namespace_begin | (first + 2U)},
    };
}

[[nodiscard]] ninho::physics::Vec3 rotate_in_plane(
    ninho::physics::Vec3 value, ninho::physics::Vec3 normal,
    double angle_radians) noexcept
{
    const float cosine = static_cast<float>(std::cos(angle_radians));
    const float sine = static_cast<float>(std::sin(angle_radians));
    return value * cosine + ninho::physics::cross(normal, value) * sine;
}

}

SessionStatus SimulationSession::Impl::preflight_split_ability(
    ShotState& active_shot, const SplitAbilityDefinition& definition)
{
    if (definition.child_count != split_child_count
        || detail::canonical_quantize(definition.spread_angle_deg)
            != detail::canonical_quantize(split_angle_deg)) {
        return split_unavailable("split preset is unavailable");
    }
    if (physics.remaining_body_capacity() < split_child_count) {
        return split_unavailable("three physics body slots are required");
    }
    const auto* source = active_shot.primary_projectile();
    if (source == nullptr || source->finished || source->pending_destroy) {
        return split_unavailable("source projectile is unavailable");
    }
    const auto source_record = std::ranges::find_if(body_records,
        [&](const BodyRecord& record) {
            return record.entity_id == source->entity_id()
                && record.part_id == PartId{1}
                && record.physics_handle == source->physics_handle
                && record.is_projectile;
        });
    const auto source_snapshot = std::ranges::find_if(entity_snapshots,
        [&](const EntitySnapshot& snapshot) {
            return snapshot.entity_id == source->entity_id()
                && snapshot.part_id == PartId{1} && snapshot.is_projectile;
        });
    const auto source_state = physics.state(source->physics_handle);
    if (source_record == body_records.end()
        || source_snapshot == entity_snapshots.end() || !source_state
        || source_record->shape.type != ShapeType::Sphere
        || !source_record->surface_id
        || !source_record->affected_by_world_gravity
        || !ninho::physics::is_finite(source_state->transform.position)
        || !valid_rotation(source_state->transform.rotation)
        || !ninho::physics::is_finite(source_state->linear_velocity)
        || !ninho::physics::is_finite(source_state->angular_velocity)
        || !std::isfinite(source_state->mass) || source_state->mass <= 0.0F) {
        return split_unavailable("source physics and domain state is invalid");
    }
    if (!valid_locked_plane(active_shot.locked_plane)) {
        return split_unavailable("locked launch plane is invalid");
    }
    const auto child_ids = child_ids_for(active_shot);
    if (!child_ids) {
        return split_unavailable("split child entity namespace is exhausted");
    }
    for (const EntityId child : *child_ids) {
        if (std::ranges::any_of(body_records,
                [&](const BodyRecord& record) {
                    return record.entity_id == child;
                })
            || std::ranges::any_of(active_shot.projectiles(),
                [&](const ProjectileState& projectile) {
                    return projectile.entity_id() == child;
                })) {
            return split_unavailable("split child entity identity is already in use");
        }
    }
    const double child_mass_kg = static_cast<double>(source_state->mass) / 3.0;
    const double child_radius_m = source_record->shape.radius_m
        * std::cbrt(1.0 / 3.0);
    if (!detail::box3d_sphere_mass_is_safe(child_mass_kg, child_radius_m)) {
        return split_unavailable("split child physics is unsafe for Box3D");
    }
    if (std::ranges::find(bundle.archetypes.presentation_ids,
            std::string{"CHR_BlueChild"})
        == bundle.archetypes.presentation_ids.end()) {
        return split_unavailable("split child presentation is unavailable");
    }

    const ninho::physics::Status preparation =
        physics.prepare_body_creations(split_child_count);
    if (!preparation.ok()) {
        return split_unavailable(preparation.message);
    }
    body_records.reserve(body_records.size() + split_child_count);
    domain_events.reserve(domain_events.size() + 5U);
    return {};
}

SessionStatus SimulationSession::Impl::apply_before_step(
    ShotState& active_shot, const SplitAbilityDefinition& definition,
    SplitAbilityRuntime& runtime)
{
    if (!runtime.active || !runtime.start_tick
        || session_state.tick != *runtime.start_tick || runtime.applied) {
        return {};
    }
    const auto preflight = preflight_split_ability(active_shot, definition);
    if (!preflight.ok()) {
        return split_failure(preflight.error.message);
    }

    const ProjectileState source_projectile = *active_shot.primary_projectile();
    const auto source_record_position = std::ranges::find_if(body_records,
        [&](const BodyRecord& record) {
            return record.entity_id == source_projectile.entity_id()
                && record.physics_handle == source_projectile.physics_handle;
        });
    if (source_record_position == body_records.end()) {
        return split_failure("source body record vanished after preflight");
    }
    const BodyRecord source_record = *source_record_position;
    const auto source_state = physics.state(source_projectile.physics_handle);
    const auto child_ids = child_ids_for(active_shot);
    if (!source_state || !child_ids) {
        return split_failure("source state vanished after preflight");
    }
    const auto bird_position = std::ranges::find(
        bundle.archetypes.birds, active_shot.bird_archetype_id,
        &BirdArchetype::id);
    if (bird_position == bundle.archetypes.birds.end()) {
        return split_failure("source bird archetype vanished after preflight");
    }
    const BirdArchetype& bird = *bird_position;

    const double child_mass_kg = static_cast<double>(source_state->mass) / 3.0;
    const double child_radius_m = source_record.shape.radius_m
        * std::cbrt(1.0 / 3.0);
    const double child_volume_m3 = 4.0 / 3.0 * std::numbers::pi
        * child_radius_m * child_radius_m * child_radius_m;
    const float child_density_kg_m3 = static_cast<float>(
        child_mass_kg / child_volume_m3);
    const double angle_radians = split_angle_deg * std::numbers::pi / 180.0;
    const float planar_multiplier = static_cast<float>(
        3.0 / (1.0 + 2.0 * std::cos(angle_radians)));
    const ninho::physics::Vec3 normal = active_shot.locked_plane.plane_normal;
    const ninho::physics::Vec3 normal_velocity = normal
        * ninho::physics::dot(source_state->linear_velocity, normal);
    const ninho::physics::Vec3 planar_velocity =
        source_state->linear_velocity - normal_velocity;
    const ninho::physics::Vec3 scaled_planar =
        planar_velocity * planar_multiplier;
    const std::array<ninho::physics::Vec3, split_child_count> child_velocities{
        rotate_in_plane(scaled_planar, normal, -angle_radians) + normal_velocity,
        scaled_planar + normal_velocity,
        rotate_in_plane(scaled_planar, normal, angle_radians) + normal_velocity,
    };
    if (!std::ranges::all_of(child_velocities,
            ninho::physics::is_finite)) {
        return split_failure("derived split velocities are invalid");
    }

    const std::uint64_t shot_ordinal = active_shot.shot_id - 1U;
    const int collision_group = -static_cast<int>(shot_ordinal + 1U);
    std::array<ninho::physics::BodyDesc, split_child_count> descriptions;
    for (std::size_t index = 0; index < split_child_count; ++index) {
        auto& description = descriptions[index];
        description = ninho::physics::BodyDesc::dynamic_sphere(
            static_cast<float>(child_radius_m), source_state->transform,
            child_density_kg_m3);
        description.linear_velocity = child_velocities[index];
        description.angular_velocity = source_state->angular_velocity;
        description.bullet = source_projectile.bullet;
        description.enable_sleep = true;
        description.affected_by_world_gravity = source_record.affected_by_world_gravity;
        description.world_exit_policy =
            ninho::physics::WorldExitPolicy::KeepOutsideBounds;
        description.name = "CHR_BlueChild";
        description.shapes.front().friction = static_cast<float>(bird.friction);
        description.shapes.front().restitution = static_cast<float>(bird.restitution);
        description.shapes.front().material_id = source_record.material_id
            ? detail::physics_material_tag(*source_record.material_id)
            : detail::physics_surface_tag(*source_record.surface_id);
        description.shapes.front().collision_group = collision_group;
    }

    std::vector<ProjectileState> replacement_projectiles;
    replacement_projectiles.reserve(split_child_count);
    std::vector<BodyRecord> child_records;
    child_records.reserve(split_child_count);
    for (std::size_t index = 0; index < split_child_count; ++index) {
        const auto created = physics.create_body(std::move(descriptions[index]));
        if (!created) {
            return split_failure(created.status.message);
        }
        ProjectileState child{(*child_ids)[index], created.value,
            source_projectile.bullet};
        child.age_ticks = source_projectile.age_ticks;
        replacement_projectiles.push_back(child);
        child_records.push_back({
            .entity_id = (*child_ids)[index],
            .part_id = PartId{1},
            .body_type = BodyType::Dynamic,
            .material_id = source_record.material_id,
            .surface_id = source_record.surface_id,
            .shape = {.type = ShapeType::Sphere, .radius_m = child_radius_m},
            .visual_id = "CHR_BlueChild",
            .physics_handle = created.value,
            .is_projectile = true,
            .affected_by_world_gravity = source_record.affected_by_world_gravity,
        });
    }

    for (BodyRecord& record : child_records) {
        body_records.push_back(std::move(record));
    }
    if (!active_shot.replace_all_projectiles(
            std::move(replacement_projectiles))) {
        return split_failure("split projectile collection replacement failed");
    }
    const ninho::physics::Status destroyed =
        physics.destroy_body(source_projectile.physics_handle);
    if (!destroyed.ok()) {
        return split_failure(destroyed.message);
    }

    runtime.source_entity_id = source_projectile.entity_id();
    runtime.child_ids = *child_ids;
    runtime.grace_end_tick = TickIndex{runtime.start_tick->value() + 2U};
    runtime.applied = true;
    runtime.filters_restored = false;

    DomainEvent split_event;
    split_event.id = EventId{next_event_sequence++};
    split_event.tick = session_state.tick;
    split_event.kind = DomainEventKind::ProjectileSplit;
    split_event.entity_id = runtime.source_entity_id;
    split_event.bird_archetype_id = active_shot.bird_archetype_id;
    split_event.ability_id = active_shot.ability_id;
    domain_events.push_back(split_event);
    for (std::size_t index = 0; index < split_child_count; ++index) {
        DomainEvent spawned;
        spawned.id = EventId{next_event_sequence++};
        spawned.tick = session_state.tick;
        spawned.kind = DomainEventKind::ProjectileSpawned;
        spawned.entity_id = runtime.source_entity_id;
        spawned.bird_archetype_id = active_shot.bird_archetype_id;
        spawned.ability_id = active_shot.ability_id;
        spawned.affected_entity_id = runtime.child_ids[index];
        spawned.affected_part_id = PartId{1};
        spawned.position_m = source_state->transform.position;
        spawned.cause_event_id = split_event.id;
        domain_events.push_back(spawned);
    }
    return {};
}

SessionStatus SimulationSession::Impl::finish_after_step(
    ShotState& active_shot, const SplitAbilityDefinition&,
    SplitAbilityRuntime& runtime)
{
    if (!runtime.active || !runtime.applied || !runtime.grace_end_tick
        || session_state.tick != *runtime.grace_end_tick) {
        return {};
    }
    for (const ProjectileState& child : active_shot.projectiles()) {
        const ninho::physics::Status restored =
            physics.set_body_collision_group(child.physics_handle, 0);
        if (!restored.ok()) {
            return split_failure(restored.message);
        }
    }
    runtime.filters_restored = true;

    DomainEvent ended;
    ended.id = EventId{next_event_sequence++};
    ended.tick = session_state.tick;
    ended.kind = DomainEventKind::AbilityEnded;
    ended.entity_id = runtime.source_entity_id;
    ended.bird_archetype_id = active_shot.bird_archetype_id;
    ended.ability_id = active_shot.ability_id;
    domain_events.push_back(ended);
    return {};
}

}
