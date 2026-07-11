#include "ninho/simulation/session.hpp"

#include "session_internal.hpp"
#if defined(NINHO_ENABLE_TEST_FACADES)
#include "session_test_facade.hpp"
#endif

#include <exception>
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
        impl_->physics.step();
        impl_->session_state.tick = TickIndex{impl_->session_state.tick.value() + 1U};
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

std::int64_t detail::SessionTestFacade::quantize_canonical(double value)
{
    return canonical_quantize(value);
}
#endif

}
