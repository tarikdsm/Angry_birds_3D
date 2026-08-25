#pragma once

#include "ninho/simulation/content.hpp"

#include <ninho/physics/physics_types.hpp>

#include <array>
#include <optional>
#include <stdexcept>
#include <type_traits>
#include <variant>

namespace ninho::simulation {

inline constexpr float speed_boost_direction_speed_threshold_m_s = 1.0e-4F;
/// The in-flight multiplier and the absolute post-ability ceiling of the
/// specification. Content validation measures the authored fallback speed
/// against the ceiling, so a manifest can never author a magnitude the kernel
/// would silently clamp away.
inline constexpr float speed_boost_speed_multiplier = 1.55F;
inline constexpr float speed_boost_absolute_speed_cap_m_s = 45.0F;

[[nodiscard]] inline bool valid_speed_boost_direction(
    ninho::physics::Vec3 direction) noexcept
{
    return ninho::physics::is_finite(direction)
        && std::abs(ninho::physics::length(direction) - 1.0F) <= 1.0e-4F;
}

// Runtime alternatives are intentionally append-only. Canonical serialization
// maps each type to an explicit tag and never depends on variant position.
struct GravityFieldAbilityRuntime {
    std::optional<TickIndex> start_tick;
    std::optional<TickIndex> end_tick;
    bool active{};

    bool operator==(const GravityFieldAbilityRuntime&) const = default;
};

struct MassBoostAbilityRuntime {
    std::optional<TickIndex> start_tick;
    std::optional<TickIndex> end_tick;
    bool active{};

    bool operator==(const MassBoostAbilityRuntime&) const = default;
};

struct SpeedBoostAbilityRuntime {
    std::optional<TickIndex> start_tick;
    std::optional<TickIndex> end_tick;
    std::optional<ninho::physics::Vec3> last_valid_flight_direction;
    bool active{};

    bool operator==(const SpeedBoostAbilityRuntime&) const = default;
};

struct ExplosionAbilityRuntime {
    std::optional<TickIndex> start_tick;
    std::optional<TickIndex> end_tick;
    std::optional<TickIndex> fuse_armed_tick;
    std::optional<TickIndex> fuse_due_tick;
    std::optional<TickIndex> detonation_tick;
    EventId activation_event_id{};
    EventId fuse_armed_event_id{};
    EventId pressure_burst_event_id{};
    bool fuse_armed{};
    bool burst_pending{};
    bool detonated{};
    bool active{};

    bool operator==(const ExplosionAbilityRuntime&) const = default;
};

struct SplitAbilityRuntime {
    std::optional<TickIndex> start_tick;
    std::optional<TickIndex> end_tick;
    EntityId source_entity_id{};
    std::array<EntityId, 3> child_ids{};
    std::optional<TickIndex> grace_end_tick;
    bool applied{};
    bool filters_restored{};
    bool active{};

    bool operator==(const SplitAbilityRuntime&) const = default;
};

using AbilityRuntime = std::variant<GravityFieldAbilityRuntime,
    MassBoostAbilityRuntime, SpeedBoostAbilityRuntime, ExplosionAbilityRuntime,
    SplitAbilityRuntime>;

[[nodiscard]] inline AbilityRuntime make_ability_runtime(AbilityKind kind)
{
    switch (kind) {
    case AbilityKind::LegacyGravityField:
    case AbilityKind::GravityField:
        return GravityFieldAbilityRuntime{};
    case AbilityKind::MassBoost:
        return MassBoostAbilityRuntime{};
    case AbilityKind::SpeedBoost:
        return SpeedBoostAbilityRuntime{};
    case AbilityKind::Explosion:
        return ExplosionAbilityRuntime{};
    case AbilityKind::Split:
        return SplitAbilityRuntime{};
    }
    throw std::invalid_argument("unknown ability kind for runtime");
}

[[nodiscard]] inline bool ability_runtime_active(const AbilityRuntime& runtime) noexcept
{
    return std::visit([](const auto& value) { return value.active; }, runtime);
}

inline void set_ability_runtime_active(AbilityRuntime& runtime, bool active) noexcept
{
    std::visit([active](auto& value) { value.active = active; }, runtime);
}

[[nodiscard]] inline std::optional<TickIndex> ability_runtime_start_tick(
    const AbilityRuntime& runtime) noexcept
{
    return std::visit([](const auto& value) { return value.start_tick; }, runtime);
}

[[nodiscard]] inline std::optional<TickIndex> ability_runtime_end_tick(
    const AbilityRuntime& runtime) noexcept
{
    return std::visit([](const auto& value) { return value.end_tick; }, runtime);
}

inline void set_ability_runtime_window(AbilityRuntime& runtime,
    TickIndex start_tick, TickIndex end_tick) noexcept
{
    std::visit([=](auto& value) {
        value.start_tick = start_tick;
        value.end_tick = end_tick;
    }, runtime);
}

}
