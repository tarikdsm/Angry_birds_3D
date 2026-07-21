#include "session_internal.hpp"

#include <algorithm>
#include <ranges>

namespace ninho::simulation {
namespace {

SessionStatus explosion_failure(std::string message)
{
    return {{ContentErrorCode::InternalError, "/ability/explosion",
        std::move(message)}};
}

}

SessionStatus SimulationSession::Impl::apply_before_step(ShotState& active_shot,
    const ExplosionAbilityDefinition& definition,
    ExplosionAbilityRuntime& runtime)
{
    if (runtime.detonated || runtime.burst_pending) {
        return {};
    }
    const bool manual = active_shot.activation_consumed
        && runtime.activation_event_id.value() != 0U;
    const bool fuse_due = runtime.fuse_armed && runtime.fuse_due_tick
        && session_state.tick >= *runtime.fuse_due_tick;
    if (!manual && !fuse_due) {
        return {};
    }
    const auto* projectile = active_shot.primary_projectile();
    if (projectile == nullptr || projectile->pending_destroy) {
        return explosion_failure("detonating projectile is unavailable");
    }
    const auto state = physics.state(projectile->physics_handle);
    if (!state) {
        return explosion_failure("detonating projectile body is unavailable");
    }
    pending_burst_requests.push_back({
        .origin_kind = manual
            ? detail::PressureBurstOriginKind::ManualAbility
            : detail::PressureBurstOriginKind::FuseAbility,
        .origin_identity = projectile->entity_id().value(),
        .source_entity_id = projectile->entity_id(),
        .source_part_id = PartId{1},
        .origin_m = state->world_center_of_mass,
        .definition = {definition.radius_m, definition.impulse_n_s,
            definition.energy_j, true, definition.max_bodies},
        .cause_event_id = manual
            ? runtime.activation_event_id : runtime.fuse_armed_event_id,
    });
    runtime.burst_pending = true;
    return {};
}

SessionStatus SimulationSession::Impl::process_after_step(ShotState& active_shot,
    const ExplosionAbilityDefinition&, ExplosionAbilityRuntime& runtime)
{
    if (runtime.fuse_armed || runtime.detonated) {
#if defined(NINHO_ENABLE_TEST_FACADES)
        explosion_contact_override_for_testing.reset();
#endif
        return {};
    }
    auto* projectile = active_shot.primary_projectile();
    if (projectile == nullptr || projectile->pending_destroy) {
        return {};
    }
    std::optional<ninho::physics::PhysicalContact> selected;
    const auto qualifies_contact = [](const auto& contact) {
        return detail::canonical_quantize(contact.approach_speed_m_s)
                >= detail::canonical_quantize(3.0)
            || detail::canonical_quantize(contact.total_normal_impulse_n_s)
                >= detail::canonical_quantize(250.0);
    };
#if defined(NINHO_ENABLE_TEST_FACADES)
    if (explosion_contact_override_for_testing) {
        selected = *explosion_contact_override_for_testing;
        explosion_contact_override_for_testing.reset();
    }
#endif
    if (!selected) {
        for (const auto& contact : physics.physical_contacts()) {
            if ((contact.a != projectile->physics_handle
                    && contact.b != projectile->physics_handle)
                || !qualifies_contact(contact)) {
                continue;
            }
            if (!selected
                || std::tuple{contact.total_normal_impulse_n_s,
                       contact.approach_speed_m_s, contact.a, contact.b}
                    > std::tuple{selected->total_normal_impulse_n_s,
                       selected->approach_speed_m_s, selected->a, selected->b}) {
                selected = contact;
            }
        }
    }
    if (!selected) {
        return {};
    }
    if (!qualifies_contact(*selected)) {
        return {};
    }
    runtime.fuse_armed = true;
    runtime.active = true;
    runtime.fuse_armed_tick = session_state.tick;
    runtime.fuse_due_tick = TickIndex{session_state.tick.value() + 39U};
    runtime.start_tick = session_state.tick;
    runtime.end_tick = runtime.fuse_due_tick;
    runtime.fuse_armed_event_id = publish_ability_event(
        DomainEventKind::ExplosionFuseArmed);
    ProjectileState replacement = *projectile;
    replacement.finished = true;
    static_cast<void>(active_shot.replace_projectile(std::move(replacement)));
    if (!session_state.last_impact_m) {
        session_state.last_impact_m = selected->point;
    }
    return {};
}

SessionStatus SimulationSession::Impl::finish_after_step(ShotState& active_shot,
    const ExplosionAbilityDefinition&, ExplosionAbilityRuntime& runtime)
{
    if (!runtime.detonated || !runtime.active) {
        return {};
    }
    publish_ability_event(DomainEventKind::AbilityEnded);
    runtime.active = false;
    if (auto* projectile = active_shot.primary_projectile()) {
        ProjectileState replacement = *projectile;
        replacement.finished = true;
        static_cast<void>(active_shot.replace_projectile(std::move(replacement)));
    }
    return {};
}

SessionStatus SimulationSession::Impl::collect_environmental_bursts_before_step()
{
    auto due = detail::EnvironmentalTriggerSystem::due_requests(
        bundle.level.triggers, environmental_trigger_runtimes,
        session_state.tick);
    pending_burst_requests.insert(pending_burst_requests.end(),
        due.begin(), due.end());
    return {};
}

bool SimulationSession::Impl::has_pending_environmental_burst() const noexcept
{
    return std::ranges::any_of(environmental_trigger_runtimes,
        [](const detail::EnvironmentalTriggerRuntime& runtime) {
            return runtime.armed && !runtime.detonated;
        });
}

SessionStatus SimulationSession::Impl::process_pending_pressure_bursts_before_step()
{
    if (pending_burst_requests.empty()) {
        return {};
    }
    std::vector<detail::PressureBurstBody> bodies;
    bodies.reserve(body_records.size());
    for (const BodyRecord& record : body_records) {
        const auto state = physics.state(record.physics_handle);
        if (!state) {
            continue;
        }
        double structural_mass = state->mass;
        if (structural_mass <= 0.0) {
            const auto authored_mass = physics.structural_mass(record.physics_handle);
            structural_mass = authored_mass.value_or(0.0f);
        }
        bodies.push_back({record.entity_id, record.part_id,
            record.physics_handle, record.body_type, structural_mass});
    }

    auto requests = pending_burst_requests;
    std::ranges::sort(requests, [](const auto& lhs, const auto& rhs) {
        return std::tuple{lhs.origin_kind, lhs.origin_identity,
                   lhs.source_entity_id, lhs.source_part_id,
                   lhs.environmental_trigger_id}
            < std::tuple{rhs.origin_kind, rhs.origin_identity,
                   rhs.source_entity_id, rhs.source_part_id,
                   rhs.environmental_trigger_id};
    });
    const auto planned = detail::PressureBurstSystem::plan(
        physics, bodies, requests);
    if (!planned.ok()) {
        return planned.status;
    }

    std::vector<DomainEvent> events;
    std::vector<detail::ExternalDamage> external;
    events.reserve(requests.size() * 2U);
    std::uint64_t assigned = next_event_sequence;
    for (std::size_t index = 0; index < planned.value.requests.size(); ++index) {
        const auto& burst = planned.value.requests[index];
        EventId direct_parent = burst.request.cause_event_id;
        if (burst.request.origin_kind
            == detail::PressureBurstOriginKind::EnvironmentalTrigger) {
            const EventId detonated{assigned++};
            const auto runtime = std::ranges::find(environmental_trigger_runtimes,
                burst.request.environmental_trigger_id,
                &detail::EnvironmentalTriggerRuntime::trigger_id);
            if (runtime == environmental_trigger_runtimes.end()) {
                return explosion_failure("environmental trigger runtime is unavailable");
            }
            events.push_back({.id = detonated, .tick = session_state.tick,
                .kind = DomainEventKind::EnvironmentalTriggerDetonated,
                .entity_id = burst.request.source_entity_id,
                .position_m = burst.request.origin_m,
                .cause_event_id = runtime->armed_event_id,
                .environmental_trigger_id = burst.request.environmental_trigger_id});
            direct_parent = detonated;
        }
        const EventId burst_event_id{assigned++};
        double total_energy = 0.0;
        ninho::physics::Vec3 total_impulse{};
        for (const auto& effect : burst.effects) {
            total_energy += effect.energy_j;
            total_impulse = total_impulse + effect.impulse_n_s;
            if (effect.energy_j > 0.0) {
                external.push_back({burst.request.source_entity_id,
                    burst.request.source_part_id, effect.target_entity_id,
                    effect.target_part_id, effect.position_m,
                    effect.normal_source_to_target, effect.energy_j,
                    burst_event_id});
            }
        }
        events.push_back({.id = burst_event_id, .tick = session_state.tick,
            .kind = DomainEventKind::PressureBurst,
            .entity_id = burst.request.source_entity_id,
            .impulse_n_s = total_impulse,
            .position_m = burst.request.origin_m,
            .energy_j = total_energy,
            .cause_event_id = direct_parent,
            .environmental_trigger_id = burst.request.environmental_trigger_id});
    }
    domain_events.reserve(domain_events.size() + events.size());
    pending_external_damage.reserve(
        pending_external_damage.size() + external.size());
#if defined(NINHO_ENABLE_TEST_FACADES)
    if (pressure_burst_failure_after_plan_for_testing) {
        pressure_burst_failure_after_plan_for_testing = false;
        return explosion_failure("injected failure after pressure burst planning");
    }
#endif
    const SessionStatus committed =
        detail::PressureBurstSystem::commit_impulses(physics, planned.value);
    if (!committed.ok()) {
        return committed;
    }
    domain_events.insert(domain_events.end(), events.begin(), events.end());
    pending_external_damage.insert(pending_external_damage.end(),
        external.begin(), external.end());
    next_event_sequence = assigned;
    for (const DomainEvent& event : events) {
        if (event.kind == DomainEventKind::EnvironmentalTriggerDetonated) {
            static_cast<void>(detail::EnvironmentalTriggerSystem::mark_detonated(
                environmental_trigger_runtimes, event.environmental_trigger_id,
                session_state.tick, event.id));
        }
        if (event.kind != DomainEventKind::PressureBurst) {
            continue;
        }
        if (shot) {
            if (auto* runtime = std::get_if<ExplosionAbilityRuntime>(&shot->runtime);
                runtime != nullptr && !runtime->detonated
                && event.entity_id == shot->primary_projectile()->entity_id()) {
                runtime->detonated = true;
                runtime->burst_pending = false;
                runtime->detonation_tick = session_state.tick;
                runtime->pressure_burst_event_id = event.id;
                shot->activation_consumed = true;
            }
        }
    }
    pending_burst_requests.clear();
    return {};
}

SessionStatus SimulationSession::Impl::observe_environmental_triggers_after_damage()
{
    if (bundle.level.triggers.empty()) {
        return {};
    }
    std::vector<detail::PressureBurstBody> bodies;
    for (const BodyRecord& record : body_records) {
        if (physics.state(record.physics_handle)) {
            bodies.push_back({record.entity_id, record.part_id,
                record.physics_handle, record.body_type});
        }
    }
    domain_events.reserve(domain_events.size() + bundle.level.triggers.size());
    const auto observed = detail::EnvironmentalTriggerSystem::observe_damage(
        physics, bundle.level.triggers, bodies, domain_events,
        session_state.tick, environmental_trigger_runtimes);
    if (!observed.ok()) {
        return observed.status;
    }
    for (const auto& action : observed.value) {
        DomainEvent event{.id = EventId{next_event_sequence++},
            .tick = session_state.tick,
            .kind = DomainEventKind::EnvironmentalTriggerArmed,
            .cause_event_id = action.initiating_damage_event_id,
            .environmental_trigger_id = action.trigger_id};
        domain_events.push_back(event);
        static_cast<void>(detail::EnvironmentalTriggerSystem::set_armed_event(
            environmental_trigger_runtimes, action.trigger_id, event.id));
    }
    return {};
}

}
