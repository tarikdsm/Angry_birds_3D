#include "session_internal.hpp"

#include <algorithm>
#include <cmath>
#include <limits>
#include <ranges>
#include <utility>
#include <vector>

namespace ninho::simulation {
namespace {

[[nodiscard]] SessionStatus ability_failure(std::string message)
{
    return {{ContentErrorCode::InternalError, "/ability", std::move(message)}};
}

}

void SimulationSession::Impl::publish_ability_event(DomainEventKind kind,
    const BodyRecord* affected, double weight, ninho::physics::Vec3 force,
    ninho::physics::Vec3 impulse)
{
    DomainEvent event;
    event.id = EventId{next_event_sequence++};
    event.tick = session_state.tick;
    event.kind = kind;
    if (shot) {
        if (const auto* projectile = primary_projectile(*shot)) {
            event.entity_id = projectile->entity_id;
        }
        event.bird_archetype_id = shot->bird_archetype_id;
        event.ability_id = shot->ability_id;
    }
    if (affected != nullptr) {
        event.affected_entity_id = affected->entity_id;
        event.affected_part_id = affected->part_id;
    }
    event.weight = weight;
    event.force_n = force;
    event.impulse_n_s = impulse;
    domain_events.push_back(event);
}

SessionStatus SimulationSession::Impl::apply_gravity_field_before_step()
{
    if (!shot || !ability_runtime_active(shot->runtime)) {
        return {};
    }
    const auto* projectile = primary_projectile(*shot);
    if (projectile == nullptr) {
        return ability_failure("active gravity field projectile is unavailable");
    }
    const AbilityArchetype* ability = ability_archetype(shot->ability_id);
    if (ability == nullptr || ability->kind != "gravity_field") {
        return ability_failure("active gravity field archetype is unavailable");
    }
    const auto projectile_snapshot = std::ranges::find_if(entity_snapshots,
        [&](const EntitySnapshot& snapshot) {
            return snapshot.entity_id == projectile->entity_id;
        });
    if (projectile_snapshot == entity_snapshots.end()) {
        return ability_failure("active gravity field projectile is unavailable");
    }

    const auto overlap = physics.overlap_sphere(projectile_snapshot->transform.position,
        static_cast<float>(ability->radius_m));
    std::vector<JointEndpoint> overlap_membership;
    overlap_membership.reserve(overlap.size());
    for (const auto& hit : overlap) {
        if (const auto identity = domain_identity(hit.body)) {
            overlap_membership.push_back(*identity);
        }
    }
    std::ranges::sort(overlap_membership, [](const auto& lhs, const auto& rhs) {
        return std::pair{lhs.entity_id, lhs.part_id}
            < std::pair{rhs.entity_id, rhs.part_id};
    });
    struct Candidate {
        const EntitySnapshot* snapshot{};
        double weight{};
        ninho::physics::Vec3 direction;
    };
    std::vector<Candidate> candidates;
    candidates.reserve(std::min<std::size_t>(overlap.size(), ability->max_bodies));
    const float maximum_mass = std::nextafter(
        static_cast<float>(ability->max_body_mass_kg),
        std::numeric_limits<float>::infinity());
    for (const EntitySnapshot& snapshot : entity_snapshots) {
        if (candidates.size() == ability->max_bodies) {
            break;
        }
        if (snapshot.body_type != BodyType::Dynamic
            || snapshot.entity_id == projectile->entity_id || snapshot.ejected
            || !std::isfinite(snapshot.mass_kg) || snapshot.mass_kg <= 0.0
            || snapshot.mass_kg > maximum_mass) {
            continue;
        }
        const JointEndpoint identity{snapshot.entity_id, snapshot.part_id};
        if (!std::ranges::binary_search(overlap_membership, identity,
                [](const auto& lhs, const auto& rhs) {
                    return std::pair{lhs.entity_id, lhs.part_id}
                        < std::pair{rhs.entity_id, rhs.part_id};
                })) {
            continue;
        }
        const auto record = std::ranges::find_if(body_records, [&](const BodyRecord& value) {
            return value.entity_id == snapshot.entity_id && value.part_id == snapshot.part_id;
        });
        if (record == body_records.end() || record->neutralized) {
            continue;
        }
        const auto toward_center = projectile_snapshot->transform.position
            - snapshot.transform.position;
        const double distance = ninho::physics::length(toward_center);
        const double t = std::clamp(1.0 - distance / ability->radius_m, 0.0, 1.0);
        const double weight = t * t * (3.0 - 2.0 * t);
        if (weight <= 0.0) {
            continue;
        }
        candidates.push_back({&snapshot, weight,
            ninho::physics::normalized_or_zero(toward_center)});
    }

    const auto ability_end_tick = ability_runtime_end_tick(shot->runtime);
    const bool final_tick = ability_end_tick && session_state.tick == *ability_end_tick;
    for (const Candidate& candidate : candidates) {
        const auto record = std::ranges::find_if(body_records, [&](const BodyRecord& value) {
            return value.entity_id == candidate.snapshot->entity_id
                && value.part_id == candidate.snapshot->part_id;
        });
        if (record == body_records.end()) {
            return ability_failure("selected gravity field body is unavailable");
        }
        const auto force = candidate.direction * static_cast<float>(candidate.snapshot->mass_kg
            * ability->max_acceleration_m_s2 * candidate.weight);
        const auto force_status = physics.apply_force(record->physics_handle,
            force, candidate.snapshot->transform.position);
        if (!force_status.ok()) {
            return ability_failure(force_status.message);
        }
        ninho::physics::Vec3 impulse{};
        if (final_tick) {
            impulse = -candidate.direction * static_cast<float>(candidate.snapshot->mass_kg
                * ability->pulse_speed_m_s * candidate.weight);
            const auto impulse_status = physics.apply_impulse(record->physics_handle,
                impulse, candidate.snapshot->transform.position);
            if (!impulse_status.ok()) {
                return ability_failure(impulse_status.message);
            }
        }
        publish_ability_event(DomainEventKind::AbilityAffectedBody,
            &*record, candidate.weight, force, impulse);
    }
    return {};
}

void SimulationSession::Impl::finish_gravity_field_after_step()
{
    if (!shot || !ability_runtime_active(shot->runtime)) {
        return;
    }
    const auto ability_end_tick = ability_runtime_end_tick(shot->runtime);
    if (!ability_end_tick || session_state.tick != *ability_end_tick) {
        return;
    }
    publish_ability_event(DomainEventKind::AbilityPulse);
    publish_ability_event(DomainEventKind::AbilityEnded);
    set_ability_runtime_active(shot->runtime, false);
}

}
