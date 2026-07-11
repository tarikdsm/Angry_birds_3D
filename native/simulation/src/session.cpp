#include "ninho/simulation/session.hpp"

#include "session_internal.hpp"
#if defined(NINHO_ENABLE_TEST_FACADES)
#include "session_test_facade.hpp"
#endif

#include <algorithm>
#include <exception>
#include <ranges>
#include <utility>

namespace ninho::simulation {

SimulationSession::SimulationSession(std::unique_ptr<Impl> implementation) noexcept
    : impl_(std::move(implementation))
{
}

SimulationSession::~SimulationSession() = default;
SimulationSession::SimulationSession(SimulationSession&&) noexcept = default;
SimulationSession& SimulationSession::operator=(SimulationSession&&) noexcept = default;

SessionStatus SimulationSession::reconfigure(
    const MaterialCatalog& materials,
    const ArchetypeCatalog& archetypes,
    const LevelManifest& level)
{
    auto candidate = create(materials, archetypes, level);
    if (!candidate.ok()) {
        return {candidate.error};
    }
    impl_.swap(candidate.value->impl_);
    return {};
}

SessionStatus SimulationSession::restart()
{
    return reconfigure(impl_->bundle.materials, impl_->bundle.archetypes, impl_->bundle.level);
}

SessionStatus SimulationSession::tick()
{
    if (impl_->latched_fault) {
        return *impl_->latched_fault;
    }
    try {
        impl_->domain_events.clear();
        impl_->session_state.tick = TickIndex{impl_->session_state.tick.value() + 1U};
        impl_->apply_pending_fractures_before_step();
        const SessionStatus command_status = impl_->process_commands();
        if (!command_status.ok()) {
            impl_->session_state.phase = SessionPhase::Faulted;
            impl_->session_state.outcome = Outcome::None;
            impl_->latched_fault = command_status;
            return command_status;
        }
        impl_->update_fsm_before_step();
        const SessionStatus ability_status = impl_->apply_gravity_field_before_step();
        if (!ability_status.ok()) {
            impl_->session_state.phase = SessionPhase::Faulted;
            impl_->session_state.outcome = Outcome::None;
            impl_->latched_fault = ability_status;
            return ability_status;
        }
        impl_->physics.step();
        impl_->process_damage_after_step();
        impl_->evaluate_fractures_after_step();
        impl_->evaluate_objectives_after_step();
        impl_->finish_gravity_field_after_step();
        impl_->rebuild_snapshots();
        impl_->remove_confirmed_runtime_body_records();
        impl_->update_fsm_after_step();
        impl_->rebuild_snapshots();
#if defined(NINHO_ENABLE_TEST_FACADES)
        if (impl_->snapshot_mass_override_for_testing && !impl_->entity_snapshots.empty()) {
            impl_->entity_snapshots.front().mass_kg =
                *impl_->snapshot_mass_override_for_testing;
            impl_->snapshot_mass_override_for_testing.reset();
        }
#endif
        impl_->refresh_canonical_state();
        return {};
    } catch (const std::exception& error) {
        impl_->session_state.phase = SessionPhase::Faulted;
        impl_->session_state.outcome = Outcome::None;
        try {
            impl_->latched_fault = SessionStatus{{
                ContentErrorCode::InternalError, "", error.what()}};
        } catch (...) {
            impl_->latched_fault.swap(impl_->emergency_fault);
        }
        return *impl_->latched_fault;
    } catch (...) {
        impl_->session_state.phase = SessionPhase::Faulted;
        impl_->session_state.outcome = Outcome::None;
        impl_->latched_fault.swap(impl_->emergency_fault);
        return *impl_->latched_fault;
    }
}

std::span<const EntitySnapshot> SimulationSession::snapshots() const noexcept
{
    return impl_->entity_snapshots;
}

std::span<const StructuralJointSnapshot> SimulationSession::structural_joints() const noexcept
{
    return impl_->joint_snapshots;
}

std::span<const DomainEvent> SimulationSession::events() const noexcept
{
    return impl_->domain_events;
}

const SessionState& SimulationSession::state() const noexcept
{
    return impl_->session_state;
}

std::uint32_t SimulationSession::birds_remaining() const noexcept
{
    return impl_->remaining_birds;
}

bool SimulationSession::objectives_complete() const noexcept
{
    return impl_->objective_complete;
}

ninho::physics::WorldMetrics SimulationSession::physics_metrics() const noexcept
{
    return impl_->physics.metrics();
}

const std::vector<std::uint8_t>& SimulationSession::canonical_state_v1() const noexcept
{
    return impl_->canonical_bytes;
}

std::uint64_t SimulationSession::canonical_hash_v1() const noexcept
{
    return impl_->canonical_hash;
}

#if defined(NINHO_ENABLE_TEST_FACADES)
void detail::SessionTestFacade::fail_next_canonical_refresh(
    SimulationSession& session, std::string message)
{
    session.impl_->canonical_refresh_failure_for_testing = std::move(message);
}

void detail::SessionTestFacade::override_next_snapshot_mass(
    SimulationSession& session, double mass_kg)
{
    session.impl_->snapshot_mass_override_for_testing = mass_kg;
}

std::uint64_t detail::SessionTestFacade::last_processed_command_sequence(
    const SimulationSession& session)
{
    return session.impl_->last_processed_command_sequence;
}

bool detail::SessionTestFacade::projectile_is_bullet(const SimulationSession& session)
{
    return session.impl_->projectile && session.impl_->projectile->bullet;
}

void detail::SessionTestFacade::finish_projectile(SimulationSession& session)
{
    if (session.impl_->projectile) {
        session.impl_->projectile->finished = true;
        session.impl_->force_settled_for_testing = true;
    }
}

void detail::SessionTestFacade::complete_objective(SimulationSession& session)
{
    for (const ObjectiveDefinition& objective : session.impl_->bundle.level.objectives) {
        for (SimulationSession::Impl::BodyRecord& record : session.impl_->body_records) {
            if (record.entity_id == objective.target_entity_id) {
                record.neutralized = true;
            }
        }
    }
}

void detail::SessionTestFacade::set_ability_active(SimulationSession& session, bool active)
{
    if (session.impl_->projectile) {
        session.impl_->projectile->ability_active = active;
    }
}

void detail::SessionTestFacade::age_projectile(
    SimulationSession& session, std::uint32_t age_ticks)
{
    if (session.impl_->projectile) {
        session.impl_->projectile->age_ticks = age_ticks;
    }
}

bool detail::SessionTestFacade::ability_requested(const SimulationSession& session)
{
    return session.impl_->projectile && session.impl_->projectile->ability_requested;
}

bool detail::SessionTestFacade::ability_active(const SimulationSession& session)
{
    return session.impl_->projectile && session.impl_->projectile->ability_active;
}

bool detail::SessionTestFacade::impulse_entity(
    SimulationSession& session, EntityId entity, ninho::physics::Vec3 impulse)
{
    const auto found = std::ranges::find(
        session.impl_->body_records, entity, &SimulationSession::Impl::BodyRecord::entity_id);
    if (found == session.impl_->body_records.end()) {
        return false;
    }
    const auto state = session.impl_->physics.state(found->physics_handle);
    return state && session.impl_->physics.apply_impulse(
        found->physics_handle, impulse, state->transform.position).ok();
}

bool detail::SessionTestFacade::add_static_sphere(SimulationSession& session,
    EntityId entity, ninho::physics::Vec3 position, double radius_m)
{
    auto description = ninho::physics::BodyDesc::static_sphere(
        static_cast<float>(radius_m), {position, {}});
    description.name = "TEST_StaticPreviewTarget";
    const auto created = session.impl_->physics.create_body(description);
    if (!created) {
        return false;
    }
    session.impl_->body_records.push_back({0U, entity, PartId{1}, BodyType::Static,
        std::nullopt, SurfaceId{1002}, std::nullopt,
        {.type = ShapeType::Sphere, .radius_m = radius_m},
        "TEST_StaticPreviewTarget", created.value});
    return true;
}

bool detail::SessionTestFacade::add_dynamic_sphere(SimulationSession& session,
    EntityId entity, PartId part, ninho::physics::Vec3 position, double mass_kg,
    ninho::physics::Vec3 linear_velocity, double radius_m)
{
    const double sphere_volume_m3 = 4.0 / 3.0 * 3.14159265358979323846
        * radius_m * radius_m * radius_m;
    auto description = ninho::physics::BodyDesc::dynamic_sphere(
        static_cast<float>(radius_m), {position, {}},
        static_cast<float>(mass_kg / sphere_volume_m3));
    description.linear_velocity = linear_velocity;
    description.name = "TEST_AbilityCandidate";
    const auto created = session.impl_->physics.create_body(description);
    if (!created) {
        return false;
    }
    session.impl_->body_records.push_back({0U, entity, part, BodyType::Dynamic,
        std::nullopt, std::nullopt, std::nullopt,
        {.type = ShapeType::Sphere, .radius_m = radius_m},
        "TEST_AbilityCandidate", created.value});
    return true;
}

bool detail::SessionTestFacade::set_body_neutralized(SimulationSession& session,
    EntityId entity, PartId part, bool neutralized)
{
    const auto record = std::ranges::find_if(session.impl_->body_records,
        [&](const auto& value) {
            return value.entity_id == entity && value.part_id == part;
        });
    if (record == session.impl_->body_records.end()) {
        return false;
    }
    record->neutralized = neutralized;
    return true;
}

void detail::SessionTestFacade::override_joint_ratio_after_solver(
    SimulationSession& session, JointId joint, double ratio)
{
    session.impl_->joint_ratio_overrides_for_testing[joint.value()] = {ratio};
}

void detail::SessionTestFacade::override_joint_ratio_without_new_cause_after_solver(
    SimulationSession& session, JointId joint, double ratio)
{
    session.impl_->joint_ratio_overrides_for_testing[joint.value()] = {ratio};
}

void detail::SessionTestFacade::fracture_piece_after_solver(
    SimulationSession& session, EntityId entity, PartId part,
    ninho::physics::Vec3 position)
{
    session.impl_->piece_fracture_requests_for_testing.push_back(
        {entity, part, position, false});
}

void detail::SessionTestFacade::fracture_piece_at_incident_tie_after_solver(
    SimulationSession& session, EntityId entity, PartId part)
{
    session.impl_->piece_fracture_requests_for_testing.push_back(
        {entity, part, {}, true});
}

TickIndex detail::SessionTestFacade::projectile_launch_tick(const SimulationSession& session)
{
    return session.impl_->projectile ? session.impl_->projectile->launch_tick : TickIndex{};
}

TickIndex detail::SessionTestFacade::ability_end_tick(const SimulationSession& session)
{
    return session.impl_->projectile && session.impl_->projectile->ability_end_tick
        ? *session.impl_->projectile->ability_end_tick
        : TickIndex{};
}

BirdArchetypeId detail::SessionTestFacade::next_bird_archetype_id(
    const SimulationSession& session)
{
    const BirdArchetype* bird = session.impl_->next_bird_archetype();
    return bird == nullptr ? BirdArchetypeId{} : bird->id;
}

AbilityId detail::SessionTestFacade::ability_id(const SimulationSession& session)
{
    return session.impl_->projectile ? session.impl_->projectile->ability_id : AbilityId{};
}

void detail::SessionTestFacade::set_launch_ordinal(
    SimulationSession& session, std::uint32_t ordinal)
{
    session.impl_->launch_count = ordinal;
}

std::size_t detail::SessionTestFacade::body_record_count(const SimulationSession& session)
{
    return session.impl_->body_records.size();
}

std::int64_t detail::SessionTestFacade::quantize_canonical(double value)
{
    return canonical_quantize(value);
}
#endif

}
