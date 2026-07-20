#include "session_internal.hpp"
#include "material_mapping.hpp"

#include <ninho/physics/radial_gravity.hpp>
#include <ninho/physics/world_bounds.hpp>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <numbers>
#include <optional>
#include <ranges>
#include <type_traits>

namespace ninho::simulation {
namespace {

constexpr std::size_t command_capacity = 128U;
constexpr std::uint32_t projectile_lifetime_ticks = 600U;
constexpr std::uint32_t watchdog_ticks = 1800U;
constexpr std::uint32_t rest_required_ticks = 60U;
constexpr float linear_rest_speed = 0.15f;
constexpr float angular_rest_speed = 0.20f;
constexpr std::uint32_t runtime_entity_bit = 0x80000000U;

[[nodiscard]] ContentError error(
    ContentErrorCode code, std::string pointer, std::string message)
{
    return {code, std::move(pointer), std::move(message)};
}

[[nodiscard]] float quantize(float value, float quantum) noexcept
{
    return std::round(value / quantum) * quantum;
}

[[nodiscard]] double quantize(double value, double quantum) noexcept
{
    return std::round(value / quantum) * quantum;
}

[[nodiscard]] std::uint64_t fnv_mix(std::uint64_t hash, std::int64_t value) noexcept
{
    const auto bits = static_cast<std::uint64_t>(value);
    for (unsigned byte = 0; byte < 8; ++byte) {
        hash ^= (bits >> (byte * 8U)) & 0xffU;
        hash *= 1099511628211ULL;
    }
    return hash;
}

constexpr double event_fraction_quantum = 1.0e-6;

[[nodiscard]] std::int64_t event_fraction_bucket(float fraction) noexcept
{
    return std::llround(std::clamp(
        static_cast<double>(fraction), 0.0, 1.0) / event_fraction_quantum);
}

[[nodiscard]] std::optional<float> first_aabb_exit_fraction(
    const ninho::physics::AabbWorldBounds& bounds,
    ninho::physics::Vec3 origin, ninho::physics::Vec3 translation) noexcept
{
    const std::array origins{origin.x, origin.y, origin.z};
    const std::array translations{translation.x, translation.y, translation.z};
    const std::array minimums{
        bounds.minimum_m.x, bounds.minimum_m.y, bounds.minimum_m.z};
    const std::array maximums{
        bounds.maximum_m.x, bounds.maximum_m.y, bounds.maximum_m.z};
    float first_exit = 1.0F;
    bool exits = false;
    for (std::size_t axis = 0; axis < origins.size(); ++axis) {
        if (origins[axis] < minimums[axis] || origins[axis] > maximums[axis]) {
            return 0.0F;
        }
        if (translations[axis] > 0.0F) {
            const float fraction =
                (maximums[axis] - origins[axis]) / translations[axis];
            if (fraction >= 0.0F && fraction <= first_exit) {
                first_exit = fraction;
                exits = fraction <= 1.0F;
            }
        } else if (translations[axis] < 0.0F) {
            const float fraction =
                (minimums[axis] - origins[axis]) / translations[axis];
            if (fraction >= 0.0F && fraction <= first_exit) {
                first_exit = fraction;
                exits = fraction <= 1.0F;
            }
        }
    }
    return exits ? std::optional<float>{first_exit} : std::nullopt;
}

[[nodiscard]] std::optional<float> first_sphere_exit_fraction(
    const ninho::physics::SphericalWorldBounds& bounds,
    ninho::physics::Vec3 origin, ninho::physics::Vec3 translation) noexcept
{
    const auto offset = origin - bounds.center_m;
    const double radius_squared = static_cast<double>(bounds.removal_radius_m)
        * bounds.removal_radius_m;
    const double distance_squared = static_cast<double>(offset.x) * offset.x
        + static_cast<double>(offset.y) * offset.y
        + static_cast<double>(offset.z) * offset.z;
    if (!std::isfinite(distance_squared) || distance_squared >= radius_squared) {
        return 0.0F;
    }
    const double a = static_cast<double>(translation.x) * translation.x
        + static_cast<double>(translation.y) * translation.y
        + static_cast<double>(translation.z) * translation.z;
    const double b = 2.0 * (static_cast<double>(offset.x) * translation.x
        + static_cast<double>(offset.y) * translation.y
        + static_cast<double>(offset.z) * translation.z);
    const double c = distance_squared - radius_squared;
    const double discriminant = b * b - 4.0 * a * c;
    if (a <= 0.0 || discriminant < 0.0 || !std::isfinite(discriminant)) {
        return std::nullopt;
    }
    const double fraction = (-b + std::sqrt(discriminant)) / (2.0 * a);
    if (!std::isfinite(fraction) || fraction < 0.0 || fraction > 1.0) {
        return std::nullopt;
    }
    return static_cast<float>(fraction);
}

[[nodiscard]] std::optional<float> first_bounds_exit_fraction(
    const ninho::physics::WorldBoundsConfig& bounds,
    ninho::physics::Vec3 origin, ninho::physics::Vec3 translation) noexcept
{
    return std::visit(
        [origin, translation](const auto& selected) -> std::optional<float> {
            using Bounds = std::decay_t<decltype(selected)>;
            if constexpr (std::is_same_v<Bounds, ninho::physics::NoWorldBounds>) {
                return std::nullopt;
            } else if constexpr (
                std::is_same_v<Bounds, ninho::physics::AabbWorldBounds>) {
                return first_aabb_exit_fraction(selected, origin, translation);
            } else {
                return first_sphere_exit_fraction(selected, origin, translation);
            }
        },
        bounds);
}

[[nodiscard]] const PhysicsSurfaceDefinition* find_surface(
    const MaterialCatalog& catalog, SurfaceId id) noexcept
{
    const auto found = std::ranges::find(catalog.surfaces, id, &PhysicsSurfaceDefinition::id);
    return found == catalog.surfaces.end() ? nullptr : &*found;
}

[[nodiscard]] bool is_projectile_contact(
    const ninho::physics::ContactHit& contact, ninho::physics::BodyHandle projectile) noexcept
{
    return contact.a == projectile || contact.b == projectile;
}

[[nodiscard]] ContentResult<AimState> quantize_aim_for_bundle(
    const ContentBundle& bundle, const AimState& source) noexcept
{
    try {
        if (!ninho::physics::is_finite(source.origin_m)
            || !ninho::physics::is_finite(source.tangent_direction)
            || !std::isfinite(source.speed_m_s)) {
            return {{}, error(ContentErrorCode::InvalidNumber, "/aim",
                            "aim components must be finite")};
        }
        AimState aim = source;
        aim.origin_m = {quantize(aim.origin_m.x, 0.001f),
            quantize(aim.origin_m.y, 0.001f), quantize(aim.origin_m.z, 0.001f)};
        aim.tangent_direction = ninho::physics::normalized_or_zero({
            quantize(aim.tangent_direction.x, 0.0001f),
            quantize(aim.tangent_direction.y, 0.0001f),
            quantize(aim.tangent_direction.z, 0.0001f)});
        if (ninho::physics::length(aim.tangent_direction) < 0.999f) {
            return {{}, error(ContentErrorCode::InvalidInvariant, "/aim/tangent_direction",
                            "aim direction must be non-zero")};
        }

        const auto& ring = bundle.level.launch_ring;
        const double shell_radius = bundle.level.planet.radius_m + ring.shell_offset_m;
        const double radius = ninho::physics::length(aim.origin_m);
        if (std::abs(radius - shell_radius) > 0.05 || std::abs(aim.origin_m.y) > 0.05f) {
            return {{}, error(ContentErrorCode::OutOfRange, "/aim/origin_m",
                            "aim origin must be on the launch shell within 5 cm")};
        }
        const double theta_deg = std::atan2(static_cast<double>(aim.origin_m.z),
                                     -static_cast<double>(aim.origin_m.x))
            * 180.0 / std::numbers::pi;
        if (theta_deg < ring.theta_min_deg || theta_deg > ring.theta_max_deg) {
            return {{}, error(ContentErrorCode::OutOfRange, "/aim/origin_m",
                            "aim origin is outside the launch arc")};
        }
        const auto radial = ninho::physics::normalized_or_zero(aim.origin_m);
        if (std::abs(ninho::physics::dot(radial, aim.tangent_direction)) > 0.01f) {
            return {{}, error(ContentErrorCode::InvalidInvariant,
                            "/aim/tangent_direction",
                            "aim direction must be tangent to the launch shell")};
        }
        if (source.speed_m_s < 8.0 || source.speed_m_s > 40.0) {
            return {{}, error(ContentErrorCode::OutOfRange, "/aim/speed_m_s",
                            "aim speed must be in the global 8-40 m/s range")};
        }
        aim.speed_m_s = std::clamp(quantize(source.speed_m_s, 0.01),
            ring.phase_speed_min_m_s, ring.phase_speed_max_m_s);
        return {aim, {}};
    } catch (...) {
        return {{}, error(ContentErrorCode::InternalError, "/aim",
                        "unexpected aim quantization failure")};
    }
}

}

SessionStatus SimulationSession::enqueue(PlayerCommand command)
{
    if (impl_->latched_fault) {
        return *impl_->latched_fault;
    }
    if (impl_->command_queue.size() >= command_capacity) {
        return {error(ContentErrorCode::ResourceLimit, "/commands",
            "player command queue capacity of 128 was reached")};
    }
    if (const auto* update = std::get_if<SetAimCommand>(&command)) {
        const auto within_envelope = [](double value) {
            return std::isfinite(value) && std::abs(value) <= 1000000.0;
        };
        if (!within_envelope(update->aim.origin_m.x)
            || !within_envelope(update->aim.origin_m.y)
            || !within_envelope(update->aim.origin_m.z)
            || !within_envelope(update->aim.tangent_direction.x)
            || !within_envelope(update->aim.tangent_direction.y)
            || !within_envelope(update->aim.tangent_direction.z)
            || !within_envelope(update->aim.speed_m_s)) {
            return {error(ContentErrorCode::InvalidNumber, "/commands/aim",
                "aim command numeric envelope must be finite and bounded")};
        }
    }
    if (const auto* begin = std::get_if<BeginGrabCommand>(&command)) {
        if (!ninho::physics::is_finite(begin->camera_right)) {
            return {error(ContentErrorCode::InvalidNumber, "/commands/camera_right",
                "camera right must be finite")};
        }
    }
    if (const auto* update = std::get_if<SetPullCommand>(&command)) {
        const auto within_envelope = [](double value) {
            return std::isfinite(value) && std::abs(value) <= 1000000.0;
        };
        if (!within_envelope(update->horizontal_m)
            || !within_envelope(update->vertical_m)) {
            return {error(ContentErrorCode::InvalidNumber, "/commands/pull",
                "pull command numeric envelope must be finite and bounded")};
        }
    }
    impl_->command_queue.push_back({impl_->next_command_sequence++, std::move(command)});
    try {
        impl_->refresh_canonical_state();
    } catch (const std::exception& exception) {
        impl_->session_state.phase = SessionPhase::Faulted;
        impl_->session_state.outcome = Outcome::None;
        impl_->latched_fault = SessionStatus{error(
            ContentErrorCode::InternalError, "/commands", exception.what())};
        return *impl_->latched_fault;
    } catch (...) {
        impl_->session_state.phase = SessionPhase::Faulted;
        impl_->session_state.outcome = Outcome::None;
        impl_->latched_fault = SessionStatus{error(ContentErrorCode::InternalError,
            "/commands", "unexpected command canonicalization failure")};
        return *impl_->latched_fault;
    }
    return {};
}

ContentResult<AimState> SimulationSession::quantize_aim(const AimState& source) const
{
    return quantize_aim_for_bundle(impl_->bundle, source);
}

TrajectoryPreview SimulationSession::preview(const AimState& source) const
{
    TrajectoryPreview result;
    const auto quantized = quantize_aim(source);
    if (!quantized.ok()) {
        result.status.error = quantized.error;
        return result;
    }
    result.quantized_aim = quantized.value;
    if (impl_->session_state.phase != SessionPhase::Inspection
        && impl_->session_state.phase != SessionPhase::Aim) {
        result.status.error = error(ContentErrorCode::InvalidInvariant, "/preview",
            "trajectory preview is available only during inspection or aim");
        return result;
    }
    const BirdArchetype* preview_bird = impl_->next_bird_archetype();
    if (preview_bird == nullptr) {
        result.status.error = error(ContentErrorCode::InvalidInvariant, "/bird_roster",
            "trajectory preview requires an available bird");
        return result;
    }

    try {
        const auto& world_config = impl_->physics.config();
        const float dt = world_config.time_step;
        const auto* spherical_bounds =
            std::get_if<ninho::physics::SphericalWorldBounds>(&world_config.bounds);
        if (spherical_bounds == nullptr) {
            result.status.error = error(ContentErrorCode::InternalError, "/preview",
                "legacy orbital preview requires spherical world bounds");
            return result;
        }
        const float removal_radius = spherical_bounds->removal_radius_m;
        if (!std::isfinite(dt) || dt <= 0.0f) {
            result.status.error = error(ContentErrorCode::InternalError, "/preview",
                "current physics timestep is not representable");
            return result;
        }
        auto position = result.quantized_aim.origin_m;
        auto velocity = result.quantized_aim.tangent_direction
            * static_cast<float>(result.quantized_aim.speed_m_s);
        result.samples.reserve(projectile_lifetime_ticks + 1U);
        result.samples.push_back(position);
        std::uint64_t hash = 14695981039346656037ULL;
        for (std::uint32_t tick = 0; tick < projectile_lifetime_ticks; ++tick) {
            velocity = velocity + impl_->physics.gravity_at(position) * dt;
            const auto translation = velocity * dt;
            if (!ninho::physics::is_finite(translation)
                || ninho::physics::length(translation) <= 0.0f) {
                result.status.error = error(ContentErrorCode::InternalError, "/preview",
                    "trajectory segment is not representable");
                return result;
            }
            const auto hit = impl_->physics.cast_sphere(position,
                static_cast<float>(preview_bird->radius_m), translation);
            if (hit) {
                const auto identity = impl_->domain_identity(hit->body);
                if (identity) {
                    result.first_hit = TrajectoryHit{identity->entity_id,
                        identity->part_id, hit->point, hit->normal};
                } else {
                    result.status.error = error(ContentErrorCode::InternalError, "/preview",
                        "trajectory hit has no domain identity");
                    return result;
                }
                position = position + translation * hit->fraction;
                result.samples.push_back(position);
                break;
            }
            position = position + translation;
            result.samples.push_back(position);
            if (ninho::physics::length(position) >= removal_radius) {
                break;
            }
        }
        for (const auto sample : result.samples) {
            hash = fnv_mix(hash, std::llround(static_cast<double>(sample.x) * 100000.0));
            hash = fnv_mix(hash, std::llround(static_cast<double>(sample.y) * 100000.0));
            hash = fnv_mix(hash, std::llround(static_cast<double>(sample.z) * 100000.0));
        }
        if (result.first_hit) {
            hash = fnv_mix(hash, result.first_hit->entity_id.value());
            hash = fnv_mix(hash, result.first_hit->part_id.value());
            hash = fnv_mix(hash,
                std::llround(static_cast<double>(result.first_hit->point_m.x) * 100000.0));
            hash = fnv_mix(hash,
                std::llround(static_cast<double>(result.first_hit->point_m.y) * 100000.0));
            hash = fnv_mix(hash,
                std::llround(static_cast<double>(result.first_hit->point_m.z) * 100000.0));
        }
        result.canonical_hash = hash;
    } catch (const std::exception& exception) {
        result.status.error = error(
            ContentErrorCode::InternalError, "/preview", exception.what());
    } catch (...) {
        result.status.error = error(ContentErrorCode::InternalError, "/preview",
            "unexpected trajectory preview failure");
    }
    return result;
}

TrajectoryPreview SimulationSession::preview() const
{
    TrajectoryPreview result;
    if (impl_->bundle.level.source_schema_version != 2U
        || impl_->session_state.phase != SessionPhase::Grabbed
        || !impl_->launcher_system) {
        result.status.error = error(ContentErrorCode::InvalidInvariant, "/preview",
            "launcher preview requires a grabbed schema v2 session");
        return result;
    }
    const auto solved = impl_->launcher_system->solution();
    if (!solved.ok() || !solved.value.launchable) {
        result.status.error = solved.ok()
            ? error(ContentErrorCode::InvalidInvariant, "/preview",
                "launcher preview requires a pull outside the deadzone")
            : solved.error;
        return result;
    }
    const BirdArchetype* preview_bird = impl_->next_bird_archetype();
    if (preview_bird == nullptr) {
        result.status.error = error(ContentErrorCode::InvalidInvariant, "/bird_queue",
            "launcher preview requires an available bird");
        return result;
    }
    result.quantized_aim = {
        solved.value.origin_m, solved.value.direction, solved.value.speed_m_s};

    try {
        constexpr double preview_seconds = 3.0;
        const auto& world_config = impl_->physics.config();
        const float dt = world_config.time_step;
        if (!std::isfinite(dt) || dt <= 0.0F) {
            result.status.error = error(ContentErrorCode::InternalError, "/preview",
                "current physics timestep is not representable");
            return result;
        }
        const auto maximum_ticks = static_cast<std::uint32_t>(
            std::floor(preview_seconds / static_cast<double>(dt) + 1.0e-6));
        auto position = solved.value.origin_m;
        auto velocity = solved.value.direction * static_cast<float>(solved.value.speed_m_s);
        result.samples.reserve(static_cast<std::size_t>(maximum_ticks) + 1U);
        result.samples.push_back(position);
        std::uint64_t hash = 14695981039346656037ULL;
        for (std::uint32_t tick = 0; tick < maximum_ticks; ++tick) {
            velocity = velocity + impl_->physics.gravity_at(position) * dt;
            const auto translation = velocity * dt;
            if (!ninho::physics::is_finite(translation)
                || ninho::physics::length(translation) <= 0.0F) {
                result.status.error = error(ContentErrorCode::InternalError, "/preview",
                    "trajectory segment is not representable");
                return result;
            }
            const auto hit = impl_->physics.cast_sphere(position,
                static_cast<float>(preview_bird->radius_m), translation);
            const auto bounds_exit = first_bounds_exit_fraction(
                world_config.bounds, position, translation);
            const bool hit_precedes_bounds = hit
                && (!bounds_exit
                    || event_fraction_bucket(hit->fraction)
                        < event_fraction_bucket(*bounds_exit));
            if (hit_precedes_bounds) {
                const auto identity = impl_->domain_identity(hit->body);
                if (!identity) {
                    result.status.error = error(ContentErrorCode::InternalError, "/preview",
                        "trajectory hit has no domain identity");
                    return result;
                }
                result.first_hit = TrajectoryHit{identity->entity_id,
                    identity->part_id, hit->point, hit->normal};
                position = position + translation * hit->fraction;
                result.samples.push_back(position);
                break;
            }
            if (bounds_exit) {
                position = position + translation * *bounds_exit;
                result.samples.push_back(position);
                break;
            }
            position = position + translation;
            result.samples.push_back(position);
        }
        for (const auto sample : result.samples) {
            hash = fnv_mix(hash, std::llround(static_cast<double>(sample.x) * 100000.0));
            hash = fnv_mix(hash, std::llround(static_cast<double>(sample.y) * 100000.0));
            hash = fnv_mix(hash, std::llround(static_cast<double>(sample.z) * 100000.0));
        }
        if (result.first_hit) {
            hash = fnv_mix(hash, result.first_hit->entity_id.value());
            hash = fnv_mix(hash, result.first_hit->part_id.value());
            hash = fnv_mix(hash,
                std::llround(static_cast<double>(result.first_hit->point_m.x) * 100000.0));
            hash = fnv_mix(hash,
                std::llround(static_cast<double>(result.first_hit->point_m.y) * 100000.0));
            hash = fnv_mix(hash,
                std::llround(static_cast<double>(result.first_hit->point_m.z) * 100000.0));
        }
        result.canonical_hash = hash;
    } catch (const std::exception& exception) {
        result.status.error = error(
            ContentErrorCode::InternalError, "/preview", exception.what());
    } catch (...) {
        result.status.error = error(ContentErrorCode::InternalError, "/preview",
            "unexpected trajectory preview failure");
    }
    return result;
}

const BirdArchetype* SimulationSession::Impl::next_bird_archetype() const noexcept
{
    if (bundle.level.source_schema_version == 2U) {
        if (remaining_birds == 0U
            || remaining_birds > bundle.level.bird_queue.size()) {
            return nullptr;
        }
        const std::size_t queue_index = bundle.level.bird_queue.size()
            - static_cast<std::size_t>(remaining_birds);
        const BirdArchetypeId next_id = bundle.level.bird_queue[queue_index];
        const auto found = std::ranges::find(
            bundle.archetypes.birds, next_id, &BirdArchetype::id);
        return found == bundle.archetypes.birds.end() ? nullptr : &*found;
    }
    for (const BirdRosterEntry& entry : roster_remaining) {
        if (entry.count == 0U) {
            continue;
        }
        const auto found = std::ranges::find(
            bundle.archetypes.birds, entry.bird_archetype_id, &BirdArchetype::id);
        if (found != bundle.archetypes.birds.end()) {
            return &*found;
        }
    }
    return nullptr;
}

const AbilityArchetype* SimulationSession::Impl::ability_archetype(AbilityId id) const noexcept
{
    const auto found = std::ranges::find(
        bundle.archetypes.abilities, id, &AbilityArchetype::id);
    return found == bundle.archetypes.abilities.end() ? nullptr : &*found;
}

std::optional<JointEndpoint> SimulationSession::Impl::domain_identity(
    ninho::physics::BodyHandle handle) const noexcept
{
    const auto found = std::ranges::find(body_records, handle, &BodyRecord::physics_handle);
    if (found == body_records.end()) {
        return std::nullopt;
    }
    return JointEndpoint{found->entity_id, found->part_id};
}

void SimulationSession::Impl::remove_confirmed_runtime_body_records()
{
    std::erase_if(body_records, [&](const BodyRecord& record) {
        return (record.entity_id.value() & runtime_entity_bit) != 0U
            && !physics.state(record.physics_handle).has_value();
    });
}

void SimulationSession::Impl::publish_event(
    DomainEventKind kind, EntityId entity, BirdArchetypeId archetype,
    CommandRejectionReason rejection)
{
    domain_events.push_back({EventId{next_event_sequence++}, session_state.tick,
        kind, entity, archetype, rejection});
}

SessionStatus SimulationSession::Impl::create_projectile(const AimState& launch_state)
{
    const BirdArchetype* bird = next_bird_archetype();
    if (bird == nullptr) {
        return {error(ContentErrorCode::InvalidInvariant, "/launch",
            "launch requires a valid aim and an available bird")};
    }
    const AbilityArchetype* ability = ability_archetype(bird->ability_id);
    if (ability == nullptr) {
        return {error(ContentErrorCode::MissingReference, "/launch/ability_id",
            "bird ability is unavailable")};
    }
    if (launch_count >= runtime_entity_bit) {
        return {error(ContentErrorCode::ResourceLimit, "/launch/entity_id",
            "runtime projectile entity ordinal capacity was reached")};
    }
    const auto* surface = find_surface(bundle.materials, bird->surface_id);
    if (surface == nullptr) {
        return {error(ContentErrorCode::MissingReference, "/launch/surface_id",
            "bird surface is unavailable")};
    }
    const bool legacy = bundle.level.source_schema_version == 1U;
    const double sphere_volume_m3 = 4.0 / 3.0 * std::numbers::pi
        * bird->radius_m * bird->radius_m * bird->radius_m;
    const double density_kg_m3 = legacy
        ? bird->density_kg_m3 : bird->mass_kg / sphere_volume_m3;
    ninho::physics::BodyDesc description = ninho::physics::BodyDesc::dynamic_sphere(
        static_cast<float>(bird->radius_m), {launch_state.origin_m, {}},
        static_cast<float>(density_kg_m3));
    description.linear_velocity = launch_state.tangent_direction
        * static_cast<float>(launch_state.speed_m_s);
    description.bullet = bird->bullet;
    // The simulation FSM owns projectile lifetime. In particular, an active
    // ability still needs its source body after crossing the world's 6R cleanup
    // boundary so it can deterministically publish its final pulse and end.
    description.world_exit_policy =
        ninho::physics::WorldExitPolicy::KeepOutsideBounds;
    const std::string visual_id = legacy
        ? "CHR_LaunchBird" : bird->projectile_visual_id;
    description.name = visual_id;
    description.shapes.front().friction = static_cast<float>(bird->friction);
    description.shapes.front().restitution = static_cast<float>(bird->restitution);
    description.shapes.front().material_id = detail::physics_surface_tag(bird->surface_id);
    const auto created = physics.create_body(description);
    if (!created) {
        return {error(ContentErrorCode::InternalError, "/launch", created.status.message)};
    }

    const std::uint32_t shot_ordinal = launch_count;
    const EntityId entity{runtime_entity_bit | shot_ordinal};
    ++launch_count;
    body_records.push_back({.entity_id = entity,
        .part_id = PartId{1},
        .body_type = BodyType::Dynamic,
        .surface_id = bird->surface_id,
        .shape = {.type = ShapeType::Sphere, .radius_m = bird->radius_m},
        .visual_id = visual_id,
        .physics_handle = created.value,
        .is_projectile = true,
        .affected_by_world_gravity = true});
    LockedLaunchPlane locked_plane;
    double pull_horizontal_m{};
    double pull_vertical_m{};
    if (session_state.launcher) {
        locked_plane = {
            .camera_right = session_state.launcher->camera_right,
            .up = session_state.launcher->up,
            .horizontal = session_state.launcher->horizontal,
            .plane_normal = session_state.launcher->plane_normal,
        };
        pull_horizontal_m = session_state.launcher->pull_horizontal_m;
        pull_vertical_m = session_state.launcher->pull_vertical_m;
    }
    ShotState next_shot{ProjectileState{entity, created.value, bird->bullet}};
    next_shot.shot_id = static_cast<std::uint64_t>(shot_ordinal) + 1U;
    next_shot.bird_archetype_id = bird->id;
    next_shot.ability_id = ability->id;
    next_shot.launch_tick = session_state.tick;
    next_shot.locked_plane = locked_plane;
    next_shot.pull_horizontal_m = pull_horizontal_m;
    next_shot.pull_vertical_m = pull_vertical_m;
    next_shot.runtime = make_ability_runtime(ability->kind_v2);
    shot.emplace(std::move(next_shot));
    if (legacy) {
        for (BirdRosterEntry& entry : roster_remaining) {
            if (entry.bird_archetype_id == bird->id && entry.count > 0U) {
                --entry.count;
                break;
            }
        }
    }
    --remaining_birds;
    session_state.phase = SessionPhase::FlightAbility;
    session_state.last_impact_m.reset();
    resolution_rest_ticks = 0U;
    publish_event(DomainEventKind::BirdLaunched, entity, bird->id);
    return {};
}

SessionStatus SimulationSession::Impl::process_commands()
{
    while (!command_queue.empty()) {
        if (std::holds_alternative<SetPullCommand>(command_queue.front().command)) {
            SetPullCommand latest = std::get<SetPullCommand>(command_queue.front().command);
            while (!command_queue.empty()
                && std::holds_alternative<SetPullCommand>(command_queue.front().command)) {
                latest = std::get<SetPullCommand>(command_queue.front().command);
                last_processed_command_sequence = command_queue.front().sequence;
                command_queue.pop_front();
            }
            if (session_state.phase != SessionPhase::Grabbed || !launcher_system) {
                publish_event(DomainEventKind::CommandRejected, {}, {},
                    CommandRejectionReason::InvalidPhase);
                continue;
            }
            const auto value = launcher_system->set_pull(
                latest.horizontal_m, latest.vertical_m);
            if (!value.ok()) {
                publish_event(DomainEventKind::CommandRejected, {}, {},
                    CommandRejectionReason::InvalidAim);
                continue;
            }
            session_state.launcher = value.value;
            continue;
        }
        if (std::holds_alternative<SetAimCommand>(command_queue.front().command)) {
            SetAimCommand latest = std::get<SetAimCommand>(command_queue.front().command);
            while (!command_queue.empty()
                && std::holds_alternative<SetAimCommand>(command_queue.front().command)) {
                latest = std::get<SetAimCommand>(command_queue.front().command);
                last_processed_command_sequence = command_queue.front().sequence;
                command_queue.pop_front();
            }
            if (session_state.phase != SessionPhase::Aim) {
                publish_event(DomainEventKind::CommandRejected, {}, {},
                    CommandRejectionReason::InvalidPhase);
                continue;
            }
            const auto value = quantize_aim_for_bundle(bundle, latest.aim);
            if (!value.ok()) {
                publish_event(DomainEventKind::CommandRejected, {}, {},
                    CommandRejectionReason::InvalidAim);
                continue;
            }
            session_state.aim = value.value;
            continue;
        }
        QueuedCommand queued = std::move(command_queue.front());
        command_queue.pop_front();
        last_processed_command_sequence = queued.sequence;
        if (std::holds_alternative<BeginAimCommand>(queued.command)) {
            if (bundle.level.source_schema_version == 1U
                && session_state.phase == SessionPhase::Inspection) {
                session_state.phase = SessionPhase::Aim;
                const double shell = bundle.level.planet.radius_m
                    + bundle.level.launch_ring.shell_offset_m;
                session_state.aim = AimState{{static_cast<float>(-shell), 0.0f, 0.0f},
                    {0.0f, 1.0f, 0.0f}, bundle.level.launch_ring.default_speed_m_s};
            } else {
                publish_event(DomainEventKind::CommandRejected, {}, {},
                    CommandRejectionReason::InvalidPhase);
            }
        } else if (std::holds_alternative<CancelAimCommand>(queued.command)) {
            if (session_state.phase == SessionPhase::Aim) {
                session_state.phase = SessionPhase::Inspection;
                session_state.aim.reset();
            } else {
                publish_event(DomainEventKind::CommandRejected, {}, {},
                    CommandRejectionReason::InvalidPhase);
            }
        } else if (std::holds_alternative<LaunchCommand>(queued.command)) {
            if (session_state.phase == SessionPhase::Aim) {
                const auto status = create_projectile(*session_state.aim);
                if (!status.ok()) {
                    return status;
                }
            } else {
                publish_event(DomainEventKind::CommandRejected, {}, {},
                    CommandRejectionReason::InvalidPhase);
            }
        } else if (const auto* begin = std::get_if<BeginGrabCommand>(&queued.command)) {
            const BirdArchetype* bird = next_bird_archetype();
            if (bundle.level.source_schema_version != 2U
                || session_state.phase != SessionPhase::Inspection
                || !launcher_system || bird == nullptr) {
                publish_event(DomainEventKind::CommandRejected, {}, {},
                    bird == nullptr ? CommandRejectionReason::NoBirdAvailable
                                    : CommandRejectionReason::InvalidPhase);
                continue;
            }
            const auto value = launcher_system->begin_grab(begin->camera_right,
                {.mass_kg = bird->mass_kg, .speed_cap_m_s = bird->launch_speed_cap_m_s});
            if (!value.ok()) {
                publish_event(DomainEventKind::CommandRejected, {}, {},
                    CommandRejectionReason::InvalidAim);
                continue;
            }
            session_state.phase = SessionPhase::Grabbed;
            session_state.aim.reset();
            session_state.launcher = value.value;
        } else if (std::holds_alternative<CancelGrabCommand>(queued.command)) {
            if (session_state.phase == SessionPhase::Grabbed && launcher_system) {
                launcher_system->cancel_grab();
                session_state.phase = SessionPhase::Inspection;
                session_state.launcher.reset();
            } else {
                publish_event(DomainEventKind::CommandRejected, {}, {},
                    CommandRejectionReason::InvalidPhase);
            }
        } else if (std::holds_alternative<ReleaseBirdCommand>(queued.command)) {
            if (session_state.phase != SessionPhase::Grabbed || !launcher_system) {
                publish_event(DomainEventKind::CommandRejected, {}, {},
                    CommandRejectionReason::InvalidPhase);
                continue;
            }
            const auto solved = launcher_system->solution();
            if (!solved.ok()) {
                publish_event(DomainEventKind::CommandRejected, {}, {},
                    CommandRejectionReason::InvalidAim);
                continue;
            }
            if (!solved.value.launchable) {
                launcher_system->cancel_grab();
                session_state.phase = SessionPhase::Inspection;
                session_state.launcher.reset();
                continue;
            }
            const AimState launch_state{solved.value.origin_m,
                solved.value.direction, solved.value.speed_m_s};
            const auto status = create_projectile(launch_state);
            if (!status.ok()) {
                return status;
            }
            launcher_system->complete_release();
            session_state.launcher = launcher_system->state();
        } else if (std::holds_alternative<ActivateAbilityCommand>(queued.command)) {
            auto* projectile = shot ? shot->primary_projectile() : nullptr;
            const AbilityArchetype* ability = shot
                ? ability_archetype(shot->ability_id)
                : nullptr;
            if (session_state.phase == SessionPhase::FlightAbility && shot
                && projectile && ability
                && session_state.tick.value()
                    >= shot->launch_tick.value() + ability->arm_ticks
                && !shot->activation_consumed && !projectile->finished) {
                shot->activation_consumed = true;
                set_ability_runtime_active(shot->runtime, true);
                set_ability_runtime_window(shot->runtime, session_state.tick,
                    TickIndex{session_state.tick.value()
                        + static_cast<std::uint64_t>(ability->duration_ticks) - 1U});
                publish_ability_event(DomainEventKind::AbilityStarted);
            } else {
                const auto reason = session_state.phase != SessionPhase::FlightAbility
                    || !shot || !projectile
                    ? CommandRejectionReason::InvalidPhase
                    : CommandRejectionReason::NotArmed;
                publish_event(DomainEventKind::CommandRejected, {}, {}, reason);
            }
        }
    }
    return {};
}

void SimulationSession::Impl::update_fsm_before_step()
{
    if (session_state.phase == SessionPhase::Evaluation) {
        if (objective_complete) {
            session_state.phase = SessionPhase::Result;
            session_state.outcome = Outcome::Victory;
        } else if (remaining_birds == 0U) {
            session_state.phase = SessionPhase::Result;
            session_state.outcome = Outcome::Defeat;
        } else {
            session_state.phase = SessionPhase::Inspection;
            session_state.outcome = Outcome::None;
            session_state.aim.reset();
            if (launcher_system) {
                launcher_system->cancel_grab();
            }
            session_state.launcher.reset();
            session_state.last_impact_m.reset();
            shot.reset();
        }
    }
    auto* projectile = shot ? shot->primary_projectile() : nullptr;
    if (projectile && !ability_runtime_active(shot->runtime)
        && projectile->finished
        && session_state.phase == SessionPhase::FlightAbility) {
        session_state.phase = SessionPhase::Resolution;
        if (!projectile->pending_destroy) {
            static_cast<void>(physics.destroy_body(projectile->physics_handle));
            projectile->pending_destroy = true;
        }
    }
}

void SimulationSession::Impl::update_fsm_after_step()
{
    if (session_state.phase == SessionPhase::Result
        || session_state.phase == SessionPhase::Faulted) {
        return;
    }
    auto* projectile = shot ? shot->primary_projectile() : nullptr;
    if (!projectile) {
        return;
    }
    ++projectile->age_ticks;
    if (objective_complete && !ability_runtime_active(shot->runtime)) {
        if (!projectile->pending_destroy) {
            static_cast<void>(physics.destroy_body(projectile->physics_handle));
            projectile->pending_destroy = true;
        }
        projectile->finished = true;
        session_state.phase = SessionPhase::Evaluation;
        resolution_rest_ticks = rest_required_ticks;
        return;
    }
    if (projectile->age_ticks >= watchdog_ticks) {
        if (!projectile->pending_destroy) {
            static_cast<void>(physics.destroy_body(projectile->physics_handle));
            projectile->pending_destroy = true;
        }
        set_ability_runtime_active(shot->runtime, false);
        projectile->finished = true;
        session_state.phase = SessionPhase::Evaluation;
        resolution_rest_ticks = rest_required_ticks;
        return;
    }

    if (!projectile->finished) {
        const auto state = physics.state(projectile->physics_handle);
        if (state) {
            if (state->ejected) {
                projectile->finished = true;
            }
            if (ninho::physics::length(state->linear_velocity) < linear_rest_speed
                && ninho::physics::length(state->angular_velocity) < angular_rest_speed) {
                ++projectile->rest_ticks;
                if (projectile->rest_ticks >= rest_required_ticks) {
                    projectile->finished = true;
                }
            } else {
                projectile->rest_ticks = 0U;
            }
        }
        const auto contact = std::ranges::find_if(physics.contact_hits(), [&](const auto& value) {
            return is_projectile_contact(value, projectile->physics_handle);
        });
        if (contact != physics.contact_hits().end()) {
            projectile->finished = true;
            session_state.last_impact_m = contact->point;
        }
        if (projectile->age_ticks >= projectile_lifetime_ticks) {
            projectile->finished = true;
        }
    }

    if (projectile->finished && !ability_runtime_active(shot->runtime)
        && session_state.phase == SessionPhase::FlightAbility) {
        session_state.phase = SessionPhase::Resolution;
        if (!projectile->pending_destroy) {
            static_cast<void>(physics.destroy_body(projectile->physics_handle));
            projectile->pending_destroy = true;
        }
    }
    if (session_state.phase == SessionPhase::Resolution) {
        bool settled = true;
        for (const ninho::physics::BodyState& state : physics.states()) {
            // Static bodies cannot carry velocity, so scanning the authoritative
            // physics states is equivalent to filtering the published snapshots.
            if (ninho::physics::length(state.linear_velocity) >= linear_rest_speed
                || ninho::physics::length(state.angular_velocity) >= angular_rest_speed) {
                settled = false;
                break;
            }
        }
#if defined(NINHO_ENABLE_TEST_FACADES)
        if (force_settled_for_testing) {
            settled = true;
        }
#endif
        resolution_rest_ticks = settled ? resolution_rest_ticks + 1U : 0U;
        if (resolution_rest_ticks >= rest_required_ticks) {
            session_state.phase = SessionPhase::Evaluation;
        }
    }
}

}
