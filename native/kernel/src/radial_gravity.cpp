#include <ninho/physics/radial_gravity.hpp>

#include <algorithm>
#include <cmath>

namespace ninho::physics {
namespace {

constexpr float minimum_direction_radius = 0.001f;
constexpr float inner_radius_fraction = 0.6f;
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
    const Vec3 toward_center = config_.center - position;
    const float distance = length(toward_center);
    if (distance < minimum_direction_radius) {
        return {};
    }

    const float inner_radius = inner_radius_fraction * config_.radius;
    const float distance_squared = distance * distance;
    const float denominator = std::max(distance_squared, inner_radius * inner_radius);
    const float magnitude = std::min(
        config_.surface_acceleration * config_.radius * config_.radius / denominator,
        maximum_radial_acceleration);
    return toward_center * (magnitude / distance);
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

}
