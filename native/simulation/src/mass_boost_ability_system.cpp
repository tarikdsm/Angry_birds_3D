#include "session_internal.hpp"

#include <cmath>
#include <iterator>
#include <limits>
#include <ranges>
#include <string>
#include <utility>
#include <vector>

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
    if (!std::isfinite(definition.mass_multiplier)
        || definition.mass_multiplier <= 0.0
        || definition.mass_multiplier > std::numeric_limits<float>::max()) {
        return mass_boost_failure("mass boost multiplier is invalid");
    }

    struct Target {
        ninho::physics::BodyHandle handle;
        const BodyRecord* record{};
    };
    std::vector<Target> targets;
    targets.reserve(active_shot.projectiles().size());
    for (const ProjectileState& projectile : active_shot.projectiles()) {
        if (projectile.finished || !physics.state(projectile.physics_handle)) {
            continue;
        }
        const auto record = std::ranges::find_if(body_records,
            [&](const BodyRecord& candidate) {
                return candidate.is_projectile
                    && candidate.entity_id == projectile.entity_id()
                    && candidate.physics_handle == projectile.physics_handle;
            });
        if (record == body_records.end()) {
            return mass_boost_failure(
                "active mass boost projectile record is unavailable");
        }
        targets.push_back({projectile.physics_handle, &*record});
    }
    if (targets.empty()) {
        return mass_boost_failure("active mass boost projectile is unavailable");
    }
    std::vector<ninho::physics::BodyHandle> handles;
    handles.reserve(targets.size());
    std::ranges::transform(targets, std::back_inserter(handles), &Target::handle);
    const ninho::physics::Status status = physics.set_body_mass_scales(
        handles, static_cast<float>(definition.mass_multiplier));
    if (!status.ok()) {
        return mass_boost_failure(status.message);
    }
    for (const Target& target : targets) {
        publish_ability_event(DomainEventKind::MassChanged,
            target.record, definition.mass_multiplier);
    }
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
