#include "session_internal.hpp"

#include <cmath>
#include <limits>
#include <string>
#include <utility>

namespace ninho::simulation {
namespace {

[[nodiscard]] SessionStatus mass_boost_failure(std::string message)
{
    return {{ContentErrorCode::InternalError, "/ability", std::move(message)}};
}

}

SessionStatus SimulationSession::Impl::apply_before_step(ShotState& active_shot,
    const MassBoostAbilityDefinition& definition,
    MassBoostAbilityRuntime& runtime)
{
    if (!runtime.active || !runtime.start_tick
        || session_state.tick != *runtime.start_tick) {
        return {};
    }
    const auto* projectile = active_shot.primary_projectile();
    if (projectile == nullptr) {
        return mass_boost_failure("active mass boost projectile is unavailable");
    }
    if (!std::isfinite(definition.mass_multiplier)
        || definition.mass_multiplier <= 0.0
        || definition.mass_multiplier > std::numeric_limits<float>::max()) {
        return mass_boost_failure("mass boost multiplier is invalid");
    }
    const ninho::physics::Status status = physics.set_body_mass_scale(
        projectile->physics_handle,
        static_cast<float>(definition.mass_multiplier));
    if (!status.ok()) {
        return mass_boost_failure(status.message);
    }
    publish_ability_event(
        DomainEventKind::MassChanged, nullptr, definition.mass_multiplier);
    return {};
}

SessionStatus SimulationSession::Impl::finish_after_step(ShotState&,
    const MassBoostAbilityDefinition&, MassBoostAbilityRuntime& runtime)
{
    if (runtime.active && runtime.end_tick
        && session_state.tick == *runtime.end_tick) {
        publish_ability_event(DomainEventKind::AbilityEnded);
    }
    return {};
}

}
