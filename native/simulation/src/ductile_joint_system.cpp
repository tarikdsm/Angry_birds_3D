#include "ductile_joint_system.hpp"
#include "session_internal.hpp"

#include <algorithm>
#include <cmath>
#include <ranges>

namespace ninho::simulation::detail {

std::vector<DuctileJointTransition> DuctileJointSystem::observe(
    TickIndex tick, std::span<const DuctileJointSample> samples)
{
    std::vector<DuctileJointSample> ordered{samples.begin(), samples.end()};
    std::ranges::sort(ordered, {}, &DuctileJointSample::joint_id);
    std::vector<DuctileJointTransition> transitions;
    for (const auto& sample : ordered) {
        if (!std::isfinite(sample.force_n) || !std::isfinite(sample.torque_nm)
            || sample.force_n < 0.0 || sample.torque_nm < 0.0) {
            continue;
        }
        auto found = std::ranges::find(states_, sample.joint_id,
            &DuctileJointRuntime::joint_id);
        if (found == states_.end()) {
            states_.push_back({.joint_id = sample.joint_id});
            std::ranges::sort(states_, {}, &DuctileJointRuntime::joint_id);
            found = std::ranges::find(states_, sample.joint_id,
                &DuctileJointRuntime::joint_id);
        }
        if (found->state == DuctileJointState::Elastic
            && (sample.force_n >= 3200.0 || sample.torque_nm >= 450.0)) {
            found->state = DuctileJointState::Yielded;
            found->pending_recreate = true;
            found->frame_a = sample.frame_a;
            found->frame_b = sample.frame_b;
            found->cause_event_id = sample.cause_event_id;
            found->yielded_tick = tick;
            found->solver_ran_after_recreate = false;
            transitions.push_back({sample.joint_id,
                DuctileJointTransitionKind::Yielded, sample.cause_event_id,
                sample.force_n, sample.torque_nm});
        } else if (found->state == DuctileJointState::Yielded
            && !found->pending_recreate
            && (sample.force_n >= 7500.0 || sample.torque_nm >= 900.0)) {
            found->solver_ran_after_recreate = true;
            found->state = DuctileJointState::Broken;
            transitions.push_back({sample.joint_id,
                DuctileJointTransitionKind::Broken, found->cause_event_id,
                sample.force_n, sample.torque_nm});
        } else if (found->state == DuctileJointState::Yielded
            && !found->pending_recreate) {
            found->solver_ran_after_recreate = true;
        }
    }
    return transitions;
}

} // namespace ninho::simulation::detail

namespace ninho::simulation {
namespace detail {
struct DuctileAccess { using Impl = SimulationSession::Impl; };
}
namespace {

using Impl = detail::DuctileAccess::Impl;

ninho::physics::Quat conjugate(ninho::physics::Quat value) noexcept
{
    return {-value.x, -value.y, -value.z, value.w};
}

ninho::physics::Quat multiply(
    ninho::physics::Quat a, ninho::physics::Quat b) noexcept
{
    return {
        a.w * b.x + a.x * b.w + a.y * b.z - a.z * b.y,
        a.w * b.y - a.x * b.z + a.y * b.w + a.z * b.x,
        a.w * b.z + a.x * b.y - a.y * b.x + a.z * b.w,
        a.w * b.w - a.x * b.x - a.y * b.y - a.z * b.z,
    };
}

ninho::physics::Vec3 rotate(
    ninho::physics::Quat rotation, ninho::physics::Vec3 value) noexcept
{
    const ninho::physics::Vec3 q{rotation.x, rotation.y, rotation.z};
    const auto twice_cross = 2.0f * ninho::physics::cross(q, value);
    return value + rotation.w * twice_cross
        + ninho::physics::cross(q, twice_cross);
}

ninho::physics::Transform current_local_frame(
    const ninho::physics::Transform& body,
    ninho::physics::Vec3 world_midpoint) noexcept
{
    const auto inverse = conjugate(body.rotation);
    return {rotate(inverse, world_midpoint - body.position),
        multiply(inverse, {})};
}

Impl::BodyRecord* endpoint_body(Impl& session, JointEndpoint endpoint)
{
    const auto found = std::ranges::find_if(session.body_records,
        [&](const auto& body) {
            return body.entity_id == endpoint.entity_id
                && body.part_id == endpoint.part_id;
        });
    return found == session.body_records.end() ? nullptr : &*found;
}

}

SessionStatus SimulationSession::Impl::apply_pending_ductile_recreations_before_step()
{
    const auto due = ductile_joint_system.recreate_due(session_state.tick);
    for (const auto& runtime : due) {
        const auto found = std::ranges::find_if(joint_records,
            [&](const auto& joint) {
                return joint.snapshot.id == runtime.joint_id
                    && joint.snapshot.kind == JointKind::SteelDuctile
                    && joint.snapshot.active;
            });
        if (found == joint_records.end()) {
            continue;
        }
        BodyRecord* a = endpoint_body(*this, found->snapshot.a);
        BodyRecord* b = endpoint_body(*this, found->snapshot.b);
        if (a == nullptr || b == nullptr) {
            return {{ContentErrorCode::InternalError, "",
                "ductile replacement endpoint is unavailable"}};
        }
        const ninho::physics::WeldJointDesc yielded{
            .a = a->physics_handle,
            .b = b->physics_handle,
            .frame_a = runtime.frame_a,
            .frame_b = runtime.frame_b,
            .hertz = 3.0f,
            .damping_ratio = 1.0f,
            .collide_connected = false,
        };
        const auto status = physics.replace_joint(found->physics_handle, yielded);
        if (!status.ok()) {
            return {{ContentErrorCode::InternalError, "", status.message}};
        }
        if (!ductile_joint_system.mark_recreated(runtime.joint_id)) {
            return {{ContentErrorCode::InternalError, "",
                "ductile replacement runtime could not be committed"}};
        }
        found->snapshot.frame_a = runtime.frame_a;
        found->snapshot.frame_b = runtime.frame_b;
    }
    return {};
}

void SimulationSession::Impl::evaluate_ductile_joints_after_step()
{
    std::vector<detail::DuctileJointSample> samples;
    for (JointRecord& joint : joint_records) {
        if (!joint.snapshot.active || joint.snapshot.kind != JointKind::SteelDuctile) {
            continue;
        }
        const BodyRecord* body_a = endpoint_body(*this, joint.snapshot.a);
        const BodyRecord* body_b = endpoint_body(*this, joint.snapshot.b);
        if (body_a == nullptr || body_b == nullptr) {
            continue;
        }
        const auto state_a = physics.state(body_a->physics_handle);
        const auto state_b = physics.state(body_b->physics_handle);
        if (!state_a || !state_b) {
            continue;
        }
        double force = 0.0;
        double torque = 0.0;
#if defined(NINHO_ENABLE_TEST_FACADES)
        const auto override = joint_ratio_overrides_for_testing.find(
            joint.snapshot.id.value());
        if (override != joint_ratio_overrides_for_testing.end()) {
            force = override->second.ratio * 3200.0;
            joint_ratio_overrides_for_testing.erase(override);
        } else
#endif
        if (const auto reaction = physics.joint_reaction(joint.physics_handle)) {
            force = ninho::physics::length(reaction->force);
            torque = ninho::physics::length(reaction->torque);
        }
        EventId cause{};
        for (auto event = domain_events.rbegin(); event != domain_events.rend(); ++event) {
            if (event->kind == DomainEventKind::DamageApplied
                || event->kind == DomainEventKind::EntityNeutralized) {
                cause = event->id;
                break;
            }
        }
        const auto midpoint = (state_a->transform.position
            + state_b->transform.position) * 0.5f;
        samples.push_back({joint.snapshot.id, force, torque,
            current_local_frame(state_a->transform, midpoint),
            current_local_frame(state_b->transform, midpoint), cause});
    }
    const auto transitions = ductile_joint_system.observe(session_state.tick, samples);
    for (const auto& transition : transitions) {
        auto joint = std::ranges::find_if(joint_records, [&](const auto& value) {
            return value.snapshot.id == transition.joint_id;
        });
        if (joint == joint_records.end()) {
            continue;
        }
        if (transition.kind == detail::DuctileJointTransitionKind::Yielded) {
            const EventId event_id{next_event_sequence++};
            joint->ductile_yield_event_id = event_id;
            const auto runtime = ductile_joint_system.state(transition.joint_id);
            if (runtime) {
                joint->snapshot.frame_a = runtime->frame_a;
                joint->snapshot.frame_b = runtime->frame_b;
            }
            domain_events.push_back({
                .id = event_id,
                .tick = session_state.tick,
                .kind = DomainEventKind::MaterialYielded,
                .affected_entity_id = joint->snapshot.a.entity_id,
                .affected_part_id = joint->snapshot.a.part_id,
                .cause_event_id = transition.cause_event_id,
                .joint_id = transition.joint_id,
                .joint_load_ratio = std::max(
                    transition.force_n / 3200.0,
                    transition.torque_nm / 450.0),
            });
        } else if (!std::ranges::any_of(pending_joint_breaks,
            [&](const auto& pending) {
                return pending.joint_id == transition.joint_id;
            })) {
            pending_joint_breaks.push_back({transition.joint_id,
                joint->ductile_yield_event_id});
        }
    }
}

}

namespace ninho::simulation::detail {

std::vector<DuctileJointRuntime> DuctileJointSystem::recreate_due(TickIndex tick)
{
    std::vector<DuctileJointRuntime> due;
    for (auto& runtime : states_) {
        if (runtime.state == DuctileJointState::Yielded
            && runtime.pending_recreate
            && tick.value() > runtime.yielded_tick.value()) {
            due.push_back(runtime);
        }
    }
    return due;
}

bool DuctileJointSystem::mark_recreated(JointId id) noexcept
{
    const auto found = std::ranges::find(states_, id, &DuctileJointRuntime::joint_id);
    if (found == states_.end() || found->state != DuctileJointState::Yielded
        || !found->pending_recreate) {
        return false;
    }
    found->pending_recreate = false;
    found->solver_ran_after_recreate = false;
    return true;
}

std::optional<DuctileJointRuntime> DuctileJointSystem::state(JointId id) const
{
    const auto found = std::ranges::find(states_, id, &DuctileJointRuntime::joint_id);
    return found == states_.end() ? std::nullopt : std::optional{*found};
}

}
