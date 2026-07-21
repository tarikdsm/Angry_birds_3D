#include "environmental_trigger_system.hpp"

#include <algorithm>
#include <cmath>
#include <ranges>

namespace ninho::simulation::detail {
namespace {

const EnvironmentalTriggerDefinition* definition_for(
    std::span<const EnvironmentalTriggerDefinition> definitions,
    std::uint32_t id)
{
    const auto found = std::ranges::find(
        definitions, id, &EnvironmentalTriggerDefinition::id);
    return found == definitions.end() ? nullptr : &*found;
}

EnvironmentalTriggerRuntime* runtime_for(
    std::vector<EnvironmentalTriggerRuntime>& runtimes, std::uint32_t id)
{
    const auto found = std::ranges::find(
        runtimes, id, &EnvironmentalTriggerRuntime::trigger_id);
    return found == runtimes.end() ? nullptr : &*found;
}

SessionStatus trigger_failure(std::uint32_t id, const char* message)
{
    return {{ContentErrorCode::InvalidInvariant,
        "/triggers/" + std::to_string(id), message}};
}

}

std::vector<EnvironmentalTriggerRuntime> EnvironmentalTriggerSystem::initialize(
    std::span<const EnvironmentalTriggerDefinition> definitions)
{
    std::vector<EnvironmentalTriggerRuntime> result;
    result.reserve(definitions.size());
    for (const auto& definition : definitions) {
        result.push_back({.trigger_id = definition.id,
            .cooldown_ticks = definition.cooldown_ticks});
    }
    std::ranges::sort(result, {}, &EnvironmentalTriggerRuntime::trigger_id);
    return result;
}

EnvironmentalTriggerObservationResult EnvironmentalTriggerSystem::observe_damage(
    const ninho::physics::PhysicsWorld& physics,
    std::span<const EnvironmentalTriggerDefinition> definitions,
    std::span<const PressureBurstBody> bodies,
    std::span<const DomainEvent> events, TickIndex current_tick,
    std::vector<EnvironmentalTriggerRuntime>& runtimes)
{
    EnvironmentalTriggerObservationResult result;
    auto next = runtimes;
    std::vector<const DomainEvent*> ordered_events;
    for (const DomainEvent& event : events) {
        if (event.kind == DomainEventKind::DamageApplied) {
            ordered_events.push_back(&event);
        }
    }
    std::ranges::sort(ordered_events, {}, &DomainEvent::id);
    for (EnvironmentalTriggerRuntime& runtime : next) {
        const auto* definition = definition_for(definitions, runtime.trigger_id);
        if (definition == nullptr) {
            result.status = trigger_failure(runtime.trigger_id,
                "trigger runtime has no definition");
            return result;
        }
        for (const DomainEvent* event : ordered_events) {
            if (event->id <= runtime.last_observed_damage_event_id) {
                continue;
            }
            runtime.last_observed_damage_event_id = event->id;
            if (event->affected_entity_id != definition->target_entity_id
                || runtime.detonated) {
                continue;
            }
            if (!std::isfinite(event->damage) || event->damage < 0.0) {
                result.status = trigger_failure(runtime.trigger_id,
                    "observed damage is not finite and non-negative");
                return result;
            }
            runtime.accumulated_damage += event->damage;
            if (!std::isfinite(runtime.accumulated_damage)) {
                result.status = trigger_failure(runtime.trigger_id,
                    "accumulated damage overflowed");
                return result;
            }
            if (runtime.armed
                || runtime.accumulated_damage < definition->damage_threshold) {
                continue;
            }
            const auto matching_count = std::ranges::count(
                bodies, definition->target_entity_id, &PressureBurstBody::entity_id);
            if (matching_count != 1) {
                result.status = trigger_failure(runtime.trigger_id,
                    "trigger target must resolve to exactly one authoritative body");
                return result;
            }
            const auto body = std::ranges::find(
                bodies, definition->target_entity_id, &PressureBurstBody::entity_id);
            const auto state = physics.state(body->physics_handle);
            if (!state) {
                result.status = trigger_failure(runtime.trigger_id,
                    "trigger target body is not live");
                return result;
            }
            runtime.armed = true;
            runtime.armed_tick = current_tick;
            runtime.due_tick = TickIndex{current_tick.value()
                + std::max(1U, definition->fuse_ticks)};
            runtime.captured_origin_m = state->world_center_of_mass;
            runtime.initiating_damage_event_id = event->id;
            result.value.push_back({runtime.trigger_id, event->id});
        }
    }
    std::ranges::sort(result.value, {}, &EnvironmentalTriggerArmedAction::trigger_id);
    runtimes.swap(next);
    return result;
}

std::vector<PressureBurstRequest> EnvironmentalTriggerSystem::due_requests(
    std::span<const EnvironmentalTriggerDefinition> definitions,
    std::span<const EnvironmentalTriggerRuntime> runtimes, TickIndex current_tick)
{
    std::vector<PressureBurstRequest> result;
    for (const EnvironmentalTriggerRuntime& runtime : runtimes) {
        if (!runtime.armed || runtime.detonated || !runtime.due_tick
            || current_tick < *runtime.due_tick) {
            continue;
        }
        const auto* definition = definition_for(definitions, runtime.trigger_id);
        if (definition == nullptr) {
            continue;
        }
        result.push_back({
            .origin_kind = PressureBurstOriginKind::EnvironmentalTrigger,
            .origin_identity = runtime.trigger_id,
            .source_entity_id = definition->target_entity_id,
            .origin_m = runtime.captured_origin_m,
            .definition = definition->pressure_burst,
            .cause_event_id = runtime.armed_event_id,
            .environmental_trigger_id = runtime.trigger_id,
        });
    }
    return result;
}

bool EnvironmentalTriggerSystem::set_armed_event(
    std::vector<EnvironmentalTriggerRuntime>& runtimes,
    std::uint32_t trigger_id, EventId armed_event_id)
{
    auto* runtime = runtime_for(runtimes, trigger_id);
    if (runtime == nullptr || !runtime->armed || runtime->detonated
        || runtime->armed_event_id.value() != 0U) {
        return false;
    }
    runtime->armed_event_id = armed_event_id;
    return true;
}

bool EnvironmentalTriggerSystem::mark_detonated(
    std::vector<EnvironmentalTriggerRuntime>& runtimes,
    std::uint32_t trigger_id, TickIndex detonation_tick,
    EventId detonated_event_id)
{
    auto* runtime = runtime_for(runtimes, trigger_id);
    if (runtime == nullptr || !runtime->armed || runtime->detonated) {
        return false;
    }
    const auto* due = runtime->due_tick ? &*runtime->due_tick : nullptr;
    if (due != nullptr && detonation_tick < *due) {
        return false;
    }
    runtime->detonated = true;
    runtime->detonated_event_id = detonated_event_id;
    runtime->cooldown_until_tick = TickIndex{
        detonation_tick.value() + runtime->cooldown_ticks};
    return true;
}

}
