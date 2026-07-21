#pragma once

#include "pressure_burst_system.hpp"

#include <optional>
#include <span>
#include <vector>

namespace ninho::simulation::detail {

struct EnvironmentalTriggerRuntime {
    std::uint32_t trigger_id{};
    std::uint32_t cooldown_ticks{};
    double accumulated_damage{};
    std::optional<TickIndex> armed_tick;
    std::optional<TickIndex> due_tick;
    std::optional<TickIndex> cooldown_until_tick;
    ninho::physics::Vec3 captured_origin_m{};
    EventId initiating_damage_event_id{};
    EventId armed_event_id{};
    EventId detonated_event_id{};
    EventId last_observed_damage_event_id{};
    bool armed{};
    bool detonated{};
    bool operator==(const EnvironmentalTriggerRuntime&) const = default;
};

struct EnvironmentalTriggerArmedAction {
    std::uint32_t trigger_id{};
    EventId initiating_damage_event_id{};
};

struct EnvironmentalTriggerObservationResult {
    std::vector<EnvironmentalTriggerArmedAction> value;
    SessionStatus status;
    [[nodiscard]] bool ok() const noexcept { return status.ok(); }
};

class EnvironmentalTriggerSystem {
public:
    [[nodiscard]] static std::vector<EnvironmentalTriggerRuntime> initialize(
        std::span<const EnvironmentalTriggerDefinition>);
    [[nodiscard]] static EnvironmentalTriggerObservationResult observe_damage(
        const ninho::physics::PhysicsWorld&,
        std::span<const EnvironmentalTriggerDefinition>,
        std::span<const PressureBurstBody>,
        std::span<const DomainEvent>, TickIndex,
        std::vector<EnvironmentalTriggerRuntime>&);
    [[nodiscard]] static std::vector<PressureBurstRequest> due_requests(
        std::span<const EnvironmentalTriggerDefinition>,
        std::span<const EnvironmentalTriggerRuntime>, TickIndex);
    static bool set_armed_event(std::vector<EnvironmentalTriggerRuntime>&,
        std::uint32_t trigger_id, EventId armed_event_id);
    static bool mark_detonated(std::vector<EnvironmentalTriggerRuntime>&,
        std::uint32_t trigger_id, TickIndex detonation_tick,
        EventId detonated_event_id);
};

}
