#include <ninho/physics/radial_gravity.hpp>

#include <cmath>

namespace ninho::physics {
namespace {

constexpr float ejection_radius_multiplier = 4.0f;
constexpr float minimum_ejection_speed = 2.0f;
constexpr float ejection_duration = 0.5f;

[[nodiscard]] constexpr detail::WorldExitKind radial_ejection_result(bool triggered) noexcept
{
    return triggered ? detail::WorldExitKind::RadialEjection : detail::WorldExitKind::None;
}

[[nodiscard]] constexpr std::uint64_t body_key(BodyHandle body) noexcept
{
    return (static_cast<std::uint64_t>(body.generation) << 32U) | body.index;
}

}

Vec3 RadialGravity::acceleration(Vec3 position) const noexcept
{
    return gravity_.acceleration_at(position);
}

bool EjectionTracker::update(
    BodyHandle body,
    float radius,
    float radial_speed,
    float dt,
    float planet_radius)
{
    const std::uint64_t key = body_key(body);
    if (!std::isfinite(radius) || !std::isfinite(radial_speed) || !std::isfinite(dt)
        || !std::isfinite(planet_radius)
        || radius < ejection_radius_multiplier * planet_radius
        || radial_speed < minimum_ejection_speed) {
        elapsed_.erase(key);
        return false;
    }

    float& elapsed = elapsed_[key];
    elapsed += dt;
    return radial_ejection_result(elapsed >= ejection_duration)
        == detail::WorldExitKind::RadialEjection;
}

void EjectionTracker::reset(BodyHandle body)
{
    elapsed_.erase(body_key(body));
}

}
