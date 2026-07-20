#include <ninho/physics/radial_gravity.hpp>

#include <cmath>
#include <utility>

namespace ninho::physics {
namespace {

constexpr float ejection_radius_multiplier = 4.0f;
constexpr float minimum_ejection_speed = 2.0f;
constexpr float ejection_duration = 0.5f;

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
    return elapsed >= ejection_duration;
}

void EjectionTracker::reset(BodyHandle body)
{
    elapsed_.erase(body_key(body));
}

detail::WorldExitTracker::WorldExitTracker(WorldBoundsConfig bounds)
    : bounds_(std::move(bounds))
{
}

detail::WorldExitKind detail::WorldExitTracker::update(
    BodyHandle body,
    Vec3 position_m,
    Vec3 linear_velocity_m_s,
    float dt,
    float planet_radius_m)
{
    if (!bounds_.contains(position_m)) {
        radial_ejection_.reset(body);
        return WorldExitKind::BoundsExit;
    }

    const float radius = length(position_m);
    const Vec3 radial_direction = normalized_or_zero(position_m);
    const float radial_speed = dot(linear_velocity_m_s, radial_direction);
    return radial_ejection_.update(body, radius, radial_speed, dt, planet_radius_m)
        ? WorldExitKind::RadialEjection
        : WorldExitKind::None;
}

void detail::WorldExitTracker::reset(BodyHandle body)
{
    radial_ejection_.reset(body);
}

}
