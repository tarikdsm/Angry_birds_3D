#pragma once

#include "ninho/simulation/events.hpp"

#include <ninho/physics/physics_types.hpp>

#include <cstdint>
#include <optional>
#include <span>
#include <string>
#include <vector>

namespace ninho::simulation {

struct GestureShotInput {
    ninho::physics::Vec3 camera_right{};
    double pull_horizontal_m{};
    double pull_vertical_m{};
    std::optional<std::uint64_t> ability_tick_after_launch;

    bool operator==(const GestureShotInput&) const = default;
};

struct PlaythroughRouteDefinition {
    std::string world_id;
    std::string level_id;
    std::vector<GestureShotInput> shots;

    bool operator==(const PlaythroughRouteDefinition&) const = default;
};

[[nodiscard]] std::vector<std::uint8_t> ordered_events_v3(
    std::span<const DomainEvent> events);

[[nodiscard]] std::vector<std::uint8_t> canonical_playthrough_v5(
    const PlaythroughRouteDefinition& route,
    std::span<const std::uint8_t> ordered_events,
    std::span<const std::uint8_t> terminal_canonical_state);

[[nodiscard]] std::uint64_t fnv1a64(std::span<const std::uint8_t> bytes) noexcept;

}
