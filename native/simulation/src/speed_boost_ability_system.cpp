#include "session_internal.hpp"

#include <algorithm>
#include <cmath>
#include <string>
#include <utility>

namespace ninho::simulation {
namespace {

constexpr float canonical_scale = 100000.0F;

[[nodiscard]] SessionStatus speed_boost_failure(std::string message)
{
    return {{ContentErrorCode::InternalError, "/ability", std::move(message)}};
}

[[nodiscard]] SessionStatus missing_direction_failure()
{
    return {{ContentErrorCode::InvalidInvariant, "/ability/direction",
        "speed boost has no valid flight direction"}};
}

[[nodiscard]] float quantized(float value)
{
    return static_cast<float>(detail::canonical_quantize(value))
        / canonical_scale;
}

[[nodiscard]] ninho::physics::Vec3 quantized(ninho::physics::Vec3 value)
{
    return {quantized(value.x), quantized(value.y), quantized(value.z)};
}

}

void SimulationSession::Impl::observe_speed_boost_direction(
    ShotState& active_shot, SpeedBoostAbilityRuntime& runtime) noexcept
{
    const auto* projectile = active_shot.primary_projectile();
    if (projectile == nullptr) {
        return;
    }
    const auto state = physics.state(projectile->physics_handle);
    if (!state || !ninho::physics::is_finite(state->linear_velocity)) {
        return;
    }
    const float speed = ninho::physics::length(state->linear_velocity);
    if (!std::isfinite(speed)
        || speed <= speed_boost_direction_speed_threshold_m_s) {
        return;
    }
    const ninho::physics::Vec3 direction =
        ninho::physics::normalized_or_zero(state->linear_velocity);
    if (valid_speed_boost_direction(direction)) {
        runtime.last_valid_flight_direction = direction;
    }
}

SessionStatus SimulationSession::Impl::apply_before_step(ShotState& active_shot,
    const SpeedBoostAbilityDefinition& definition,
    SpeedBoostAbilityRuntime& runtime)
{
    if (!runtime.active || !runtime.start_tick
        || session_state.tick != *runtime.start_tick) {
        return {};
    }
    const auto* projectile = active_shot.primary_projectile();
    if (projectile == nullptr) {
        return speed_boost_failure(
            "active speed boost projectile is unavailable");
    }
    const auto state = physics.state(projectile->physics_handle);
    if (!state) {
        return speed_boost_failure(
            "active speed boost physics body is unavailable");
    }
    if (!ninho::physics::is_finite(state->linear_velocity)
        || !std::isfinite(state->mass) || state->mass <= 0.0F) {
        return speed_boost_failure(
            "active speed boost physics state is invalid");
    }
    if (!std::isfinite(definition.fallback_speed_m_s)
        || definition.fallback_speed_m_s <= 0.0
        || definition.fallback_speed_m_s
            > static_cast<double>(speed_boost_absolute_speed_cap_m_s)) {
        return speed_boost_failure("speed boost fallback speed is invalid");
    }

    const float current_speed =
        ninho::physics::length(state->linear_velocity);
    if (!std::isfinite(current_speed)) {
        return speed_boost_failure("speed boost velocity magnitude is invalid");
    }
    const bool moving =
        current_speed > speed_boost_direction_speed_threshold_m_s;
    ninho::physics::Vec3 direction{};
    if (moving) {
        direction = ninho::physics::normalized_or_zero(state->linear_velocity);
    } else if (runtime.last_valid_flight_direction) {
        direction = *runtime.last_valid_flight_direction;
    }
    if (!valid_speed_boost_direction(direction)) {
        return missing_direction_failure();
    }

    // In flight the specification multiplies the speed; stalled, there is
    // nothing to multiply, so the authored fallback speed is the magnitude and
    // the last valid flight direction is the direction.
    const float target_speed = moving
        ? std::min(current_speed * speed_boost_speed_multiplier,
            speed_boost_absolute_speed_cap_m_s)
        : std::min(static_cast<float>(definition.fallback_speed_m_s),
            speed_boost_absolute_speed_cap_m_s);
    const ninho::physics::Vec3 target_velocity = direction * target_speed;
    const ninho::physics::Vec3 delta_velocity = quantized(
        target_velocity - state->linear_velocity);
    const ninho::physics::Vec3 physical_impulse =
        delta_velocity * state->mass;
    // Canonical telemetry has its own 1e-5 quantum and may honestly resolve to
    // zero for tiny masses. The physical command keeps representable float
    // precision so a published SpeedChanged always accompanies a real boost.
    const ninho::physics::Vec3 telemetry_impulse = quantized(physical_impulse);
    if (!ninho::physics::is_finite(delta_velocity)
        || !ninho::physics::is_finite(physical_impulse)
        || !ninho::physics::is_finite(telemetry_impulse)) {
        return speed_boost_failure("speed boost impulse is invalid");
    }

    // Product-v2 projectile bodies are centered spheres. Therefore the body
    // transform origin is their center of mass and applying here cannot add torque.
    const ninho::physics::Status status = physics.apply_impulse(
        projectile->physics_handle, physical_impulse, state->transform.position);
    if (!status.ok()) {
        return speed_boost_failure(status.message);
    }
    publish_ability_event(DomainEventKind::SpeedChanged, nullptr, 0.0, {},
        telemetry_impulse, delta_velocity);
    return {};
}

SessionStatus SimulationSession::Impl::finish_after_step(ShotState&,
    const SpeedBoostAbilityDefinition&, SpeedBoostAbilityRuntime& runtime)
{
    if (runtime.active && runtime.end_tick
        && session_state.tick == *runtime.end_tick) {
        publish_ability_event(DomainEventKind::AbilityEnded);
    }
    return {};
}

}
