#include "session_internal.hpp"

#include <algorithm>
#include <cmath>
#include <limits>
#include <ranges>
#include <tuple>

namespace ninho::simulation {
namespace detail {

struct FractureAccess {
    using Impl = SimulationSession::Impl;
};

}
namespace {

using Impl = detail::FractureAccess::Impl;

Impl::BodyRecord* find_body(
    std::vector<Impl::BodyRecord>& bodies, EntityId entity, PartId part)
{
    const auto found = std::ranges::find_if(bodies, [&](const Impl::BodyRecord& body) {
        return body.entity_id == entity && body.part_id == part;
    });
    return found == bodies.end() ? nullptr : &*found;
}

const MaterialDefinition* find_material(const MaterialCatalog& catalog, MaterialId id)
{
    const auto found = std::ranges::find(catalog.materials, id, &MaterialDefinition::id);
    return found == catalog.materials.end() ? nullptr : &*found;
}

ninho::physics::Vec3 joint_midpoint(
    const Impl& session, const Impl::JointRecord& joint)
{
    const auto body_a = std::ranges::find_if(session.body_records, [&](const auto& body) {
        return body.entity_id == joint.snapshot.a.entity_id
            && body.part_id == joint.snapshot.a.part_id;
    });
    const auto body_b = std::ranges::find_if(session.body_records, [&](const auto& body) {
        return body.entity_id == joint.snapshot.b.entity_id
            && body.part_id == joint.snapshot.b.part_id;
    });
    if (body_a == session.body_records.end() || body_b == session.body_records.end()) {
        return {};
    }
    const auto state_a = session.physics.state(body_a->physics_handle);
    const auto state_b = session.physics.state(body_b->physics_handle);
    if (!state_a || !state_b) {
        return {};
    }
    return (state_a->transform.position + state_b->transform.position) * 0.5f;
}

double distance_squared(ninho::physics::Vec3 a, ninho::physics::Vec3 b)
{
    const ninho::physics::Vec3 delta = a - b;
    return static_cast<double>(ninho::physics::dot(delta, delta));
}

EventId nearest_cause(const Impl& session, ninho::physics::Vec3 point)
{
    const DomainEvent* best = nullptr;
    double best_distance = std::numeric_limits<double>::infinity();
    for (const DomainEvent& event : session.domain_events) {
        if (event.kind != DomainEventKind::DamageApplied
            && event.kind != DomainEventKind::EntityNeutralized) {
            continue;
        }
        const double candidate_distance = distance_squared(event.position_m, point);
        if (best == nullptr
            || std::tuple{candidate_distance, event.id}
                < std::tuple{best_distance, best->id}) {
            best = &event;
            best_distance = candidate_distance;
        }
    }
    return best == nullptr ? EventId{} : best->id;
}

bool already_pending(const Impl& session, JointId id)
{
    return std::ranges::any_of(session.pending_joint_breaks,
        [&](const Impl::PendingJointBreak& pending) { return pending.joint_id == id; });
}

JointId nearest_incident_joint(const Impl& session, EntityId entity, PartId part,
    ninho::physics::Vec3 position)
{
    const Impl::JointRecord* best = nullptr;
    double best_distance = std::numeric_limits<double>::infinity();
    for (const Impl::JointRecord& joint : session.joint_records) {
        const bool incident = joint.snapshot.active
            && (joint.snapshot.a == JointEndpoint{entity, part}
                || joint.snapshot.b == JointEndpoint{entity, part});
        if (!incident) {
            continue;
        }
        const double candidate_distance = std::sqrt(distance_squared(
            joint_midpoint(session, joint), position));
        const std::int64_t candidate_key = detail::canonical_quantize(candidate_distance);
        if (best == nullptr
            || std::tuple{candidate_key, joint.snapshot.id}
                < std::tuple{detail::canonical_quantize(best_distance), best->snapshot.id}) {
            best = &joint;
            best_distance = candidate_distance;
        }
    }
    return best == nullptr ? JointId{} : best->snapshot.id;
}

ninho::physics::Vec3 first_incident_tie_position(
    const Impl& session, EntityId entity, PartId part)
{
    std::vector<const Impl::JointRecord*> incident;
    for (const Impl::JointRecord& joint : session.joint_records) {
        if (joint.snapshot.active
            && (joint.snapshot.a == JointEndpoint{entity, part}
                || joint.snapshot.b == JointEndpoint{entity, part})) {
            incident.push_back(&joint);
        }
    }
    std::ranges::sort(incident, {}, [](const Impl::JointRecord* joint) {
        return joint->snapshot.id;
    });
    if (incident.size() < 2U) {
        return {};
    }
    const ninho::physics::Vec3 first = joint_midpoint(session, *incident[0]);
    const ninho::physics::Vec3 second = joint_midpoint(session, *incident[1]);
    const ninho::physics::Vec3 midpoint = (first + second) * 0.5f;
    const ninho::physics::Vec3 axis = second - first;
    const float axis_squared = ninho::physics::dot(axis, axis);
    ninho::physics::Vec3 away{};
    for (std::size_t index = 2; index < incident.size(); ++index) {
        ninho::physics::Vec3 direction = midpoint
            - joint_midpoint(session, *incident[index]);
        if (axis_squared > 0.0f) {
            direction = direction - axis
                * (ninho::physics::dot(direction, axis) / axis_squared);
        }
        away = away + ninho::physics::normalized_or_zero(direction);
    }
    return midpoint + ninho::physics::normalized_or_zero(away) * 10.0f;
}

void schedule_joint(Impl& session, JointId joint, EventId cause)
{
    if (joint == JointId{} || cause == EventId{} || already_pending(session, joint)) {
        return;
    }
    session.pending_joint_breaks.push_back({joint, cause});
}

void schedule_piece(Impl& session, EntityId entity, PartId part,
    MaterialId material, ninho::physics::Vec3 position, EventId cause)
{
    if (std::ranges::find(session.fractured_pieces, std::pair{entity, part})
            != session.fractured_pieces.end()
        || std::ranges::any_of(session.pending_piece_fractures,
            [&](const Impl::PendingPieceFracture& pending) {
                return pending.entity_id == entity && pending.part_id == part;
            })) {
        return;
    }
    if (cause == EventId{}) {
        return;
    }
    const JointId incident = nearest_incident_joint(session, entity, part, position);
    session.pending_piece_fractures.push_back(
        {entity, part, material, incident, cause, position});
    schedule_joint(session, incident, cause);
}

}

void SimulationSession::Impl::apply_pending_fractures_before_step()
{
    std::ranges::sort(pending_joint_breaks, {}, &PendingJointBreak::joint_id);
    for (const PendingJointBreak& pending : pending_joint_breaks) {
        const auto found = std::ranges::find_if(joint_records, [&](const JointRecord& joint) {
            return joint.snapshot.id == pending.joint_id;
        });
        if (found == joint_records.end() || !found->snapshot.active) {
            continue;
        }
        static_cast<void>(physics.destroy_joint(found->physics_handle));
        found->snapshot.active = false;
        found->consecutive_overload_ticks = 0U;
        found->overload_cause_event_id = {};
        domain_events.push_back({
            .id = EventId{next_event_sequence++},
            .tick = session_state.tick,
            .kind = DomainEventKind::JointBroken,
            .affected_entity_id = found->snapshot.a.entity_id,
            .affected_part_id = found->snapshot.a.part_id,
            .cause_event_id = pending.cause_event_id,
            .joint_id = pending.joint_id,
        });
    }
    pending_joint_breaks.clear();

    std::ranges::sort(pending_piece_fractures, [](const auto& lhs, const auto& rhs) {
        return std::pair{lhs.entity_id, lhs.part_id}
            < std::pair{rhs.entity_id, rhs.part_id};
    });
    for (const PendingPieceFracture& pending : pending_piece_fractures) {
        domain_events.push_back({
            .id = EventId{next_event_sequence++},
            .tick = session_state.tick,
            .kind = DomainEventKind::PieceFractured,
            .affected_entity_id = pending.entity_id,
            .affected_part_id = pending.part_id,
            .position_m = pending.position_m,
            .cause_event_id = pending.cause_event_id,
            .joint_id = pending.incident_joint_id,
            .material_id = pending.material_id,
        });
        fractured_pieces.emplace_back(pending.entity_id, pending.part_id);
    }
    pending_piece_fractures.clear();
    std::ranges::sort(fractured_pieces);
}

void SimulationSession::Impl::evaluate_fractures_after_step()
{
    for (JointRecord& joint : joint_records) {
        if (!joint.snapshot.active || already_pending(*this, joint.snapshot.id)) {
            continue;
        }
        double ratio = 0.0;
        EventId explicit_test_cause{};
#if defined(NINHO_ENABLE_TEST_FACADES)
        const auto override = joint_ratio_overrides_for_testing.find(joint.snapshot.id.value());
        if (override != joint_ratio_overrides_for_testing.end()) {
            ratio = override->second.ratio;
            const bool emit_cause = override->second.emit_cause;
            joint_ratio_overrides_for_testing.erase(override);
            if (emit_cause) {
                const ninho::physics::Vec3 position = joint_midpoint(*this, joint);
                explicit_test_cause = EventId{next_event_sequence++};
                domain_events.push_back({
                    .id = explicit_test_cause,
                    .tick = session_state.tick,
                    .kind = DomainEventKind::DamageApplied,
                    .affected_entity_id = joint.snapshot.a.entity_id,
                    .affected_part_id = joint.snapshot.a.part_id,
                    .position_m = position,
                });
            }
        } else
#endif
        if (const auto reaction = physics.joint_reaction(joint.physics_handle)) {
            ratio = std::max(
                static_cast<double>(ninho::physics::length(reaction->force))
                    / joint.snapshot.force_limit_n,
                static_cast<double>(ninho::physics::length(reaction->torque))
                    / joint.snapshot.torque_limit_nm);
        }
        if (ratio >= 1.5) {
            joint.consecutive_overload_ticks = 0U;
            const EventId cause = explicit_test_cause != EventId{}
                ? explicit_test_cause
                : nearest_cause(*this, joint_midpoint(*this, joint));
            if (cause != EventId{}) {
                joint.overload_cause_event_id = cause;
            }
            schedule_joint(*this, joint.snapshot.id, joint.overload_cause_event_id);
        } else if (ratio >= 1.0) {
            const EventId cause = explicit_test_cause != EventId{}
                ? explicit_test_cause
                : nearest_cause(*this, joint_midpoint(*this, joint));
            if (cause != EventId{}) {
                joint.overload_cause_event_id = cause;
            }
            ++joint.consecutive_overload_ticks;
            if (joint.consecutive_overload_ticks >= 2U) {
                joint.consecutive_overload_ticks = 0U;
                schedule_joint(
                    *this, joint.snapshot.id, joint.overload_cause_event_id);
            }
        } else {
            joint.consecutive_overload_ticks = 0U;
            joint.overload_cause_event_id = {};
        }
    }

    for (const DomainEvent& event : domain_events) {
        if (event.kind != DomainEventKind::DamageApplied) {
            continue;
        }
        BodyRecord* body = find_body(
            body_records, event.affected_entity_id, event.affected_part_id);
        if (body == nullptr || !body->material_id) {
            continue;
        }
        const auto damage = damage_system.state(body->entity_id, body->part_id);
        const auto state = physics.state(body->physics_handle);
        const MaterialDefinition* material = find_material(bundle.materials, *body->material_id);
        if (!damage || !state || material == nullptr) {
            continue;
        }
        const double fracture_energy = state->mass * 250.0 * material->toughness;
        if (damage->material_damage_energy_j >= fracture_energy) {
            schedule_piece(*this, body->entity_id, body->part_id,
                *body->material_id, event.position_m, event.id);
        }
    }

#if defined(NINHO_ENABLE_TEST_FACADES)
    for (const PieceFractureRequest& request : piece_fracture_requests_for_testing) {
        BodyRecord* body = find_body(body_records, request.entity_id, request.part_id);
        const ninho::physics::Vec3 position = request.tie_first_two_incident
            ? first_incident_tie_position(*this, request.entity_id, request.part_id)
            : request.position_m;
        const EventId cause{next_event_sequence++};
        domain_events.push_back({
            .id = cause,
            .tick = session_state.tick,
            .kind = DomainEventKind::DamageApplied,
            .affected_entity_id = request.entity_id,
            .affected_part_id = request.part_id,
            .position_m = position,
        });
        schedule_piece(*this, request.entity_id, request.part_id,
            body != nullptr && body->material_id ? *body->material_id : MaterialId{},
            position, cause);
    }
    piece_fracture_requests_for_testing.clear();
#endif
}

}
