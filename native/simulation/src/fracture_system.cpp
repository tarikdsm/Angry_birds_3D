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

Impl::JointRecord* find_joint(Impl& session, JointId id)
{
    const auto found = std::ranges::find_if(session.joint_records,
        [&](const Impl::JointRecord& joint) { return joint.snapshot.id == id; });
    return found == session.joint_records.end() ? nullptr : &*found;
}

bool piece_already_scheduled(const Impl& session, EntityId entity, PartId part)
{
    return std::ranges::find(session.fractured_pieces, std::pair{entity, part})
            != session.fractured_pieces.end()
        || std::ranges::any_of(session.pending_piece_fractures,
            [&](const Impl::PendingPieceFracture& pending) {
                return pending.entity_id == entity && pending.part_id == part;
            });
}

EventId publish_overload(Impl& session, Impl::JointRecord& joint,
    ninho::physics::Vec3 position, double ratio, EventId prior_cause)
{
    const EventId overload_id{session.next_event_sequence++};
    session.domain_events.push_back({
        .id = overload_id,
        .tick = session.session_state.tick,
        .kind = DomainEventKind::JointOverloaded,
        .affected_entity_id = joint.snapshot.a.entity_id,
        .affected_part_id = joint.snapshot.a.part_id,
        .position_m = position,
        .cause_event_id = prior_cause,
        .joint_id = joint.snapshot.id,
        .joint_load_ratio = ratio,
    });
    schedule_joint(session, joint.snapshot.id, overload_id);
    return overload_id;
}

EventId publish_piece_fracture_trigger(Impl& session, const Impl::JointRecord& joint,
    EntityId entity, PartId part, MaterialId material,
    ninho::physics::Vec3 position, double fracture_ratio, EventId prior_cause)
{
    const EventId trigger_id{session.next_event_sequence++};
    session.domain_events.push_back({
        .id = trigger_id,
        .tick = session.session_state.tick,
        .kind = DomainEventKind::PieceFractureTriggered,
        .affected_entity_id = entity,
        .affected_part_id = part,
        .position_m = position,
        .cause_event_id = prior_cause,
        .joint_id = joint.snapshot.id,
        .material_id = material,
        .fracture_ratio = fracture_ratio,
    });
    const auto pending = std::ranges::find_if(session.pending_joint_breaks,
        [&](const Impl::PendingJointBreak& value) {
            return value.joint_id == joint.snapshot.id;
        });
    if (pending == session.pending_joint_breaks.end()) {
        session.pending_joint_breaks.push_back({joint.snapshot.id, trigger_id});
    } else {
        pending->cause_event_id = trigger_id;
    }
    return trigger_id;
}

void schedule_piece(Impl& session, EntityId entity, PartId part,
    MaterialId material, JointId incident, ninho::physics::Vec3 position,
    EventId cause_event)
{
    if (piece_already_scheduled(session, entity, part)) {
        return;
    }
    if (cause_event == EventId{}) {
        return;
    }
    session.pending_piece_fractures.push_back(
        {entity, part, material, incident, cause_event, position});
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
#if defined(NINHO_ENABLE_TEST_FACADES)
        const auto override = joint_ratio_overrides_for_testing.find(joint.snapshot.id.value());
        if (override != joint_ratio_overrides_for_testing.end()) {
            ratio = override->second.ratio;
            joint_ratio_overrides_for_testing.erase(override);
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
            const ninho::physics::Vec3 position = joint_midpoint(*this, joint);
            static_cast<void>(publish_overload(
                *this, joint, position, ratio, nearest_cause(*this, position)));
        } else if (ratio >= 1.0) {
            ++joint.consecutive_overload_ticks;
            if (joint.consecutive_overload_ticks >= 2U) {
                joint.consecutive_overload_ticks = 0U;
                const ninho::physics::Vec3 position = joint_midpoint(*this, joint);
                static_cast<void>(publish_overload(
                    *this, joint, position, ratio, nearest_cause(*this, position)));
            }
        } else {
            joint.consecutive_overload_ticks = 0U;
        }
    }

    std::vector<DomainEvent> damage_events;
    for (const DomainEvent& event : domain_events) {
        if (event.kind == DomainEventKind::DamageApplied) {
            damage_events.push_back(event);
        }
    }
    for (const DomainEvent& event : damage_events) {
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
            if (piece_already_scheduled(*this, body->entity_id, body->part_id)) {
                continue;
            }
            const JointId incident = nearest_incident_joint(
                *this, body->entity_id, body->part_id, event.position_m);
            JointRecord* joint = find_joint(*this, incident);
            if (joint == nullptr) {
                continue;
            }
            const double fracture_ratio =
                damage->material_damage_energy_j / fracture_energy;
            const EventId trigger = publish_piece_fracture_trigger(*this, *joint,
                body->entity_id, body->part_id, *body->material_id,
                event.position_m, fracture_ratio, event.id);
            schedule_piece(*this, body->entity_id, body->part_id,
                *body->material_id, incident, event.position_m, trigger);
        }
    }

#if defined(NINHO_ENABLE_TEST_FACADES)
    for (const PieceFractureRequest& request : piece_fracture_requests_for_testing) {
        BodyRecord* body = find_body(body_records, request.entity_id, request.part_id);
        const ninho::physics::Vec3 position = request.tie_first_two_incident
            ? first_incident_tie_position(*this, request.entity_id, request.part_id)
            : request.position_m;
        const JointId incident = nearest_incident_joint(
            *this, request.entity_id, request.part_id, position);
        JointRecord* joint = find_joint(*this, incident);
        if (joint == nullptr
            || piece_already_scheduled(*this, request.entity_id, request.part_id)) {
            continue;
        }
        const MaterialId material = body != nullptr && body->material_id
            ? *body->material_id : MaterialId{};
        const EventId trigger = publish_piece_fracture_trigger(*this, *joint,
            request.entity_id, request.part_id, material, position, 1.0, {});
        schedule_piece(*this, request.entity_id, request.part_id,
            material, incident, position, trigger);
    }
    piece_fracture_requests_for_testing.clear();
#endif
}

}
