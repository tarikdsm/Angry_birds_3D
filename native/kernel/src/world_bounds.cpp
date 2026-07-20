#include <ninho/physics/world_bounds.hpp>

#include <cmath>
#include <stdexcept>
#include <type_traits>
#include <utility>

namespace ninho::physics {
namespace {

[[nodiscard]] bool valid(const AabbWorldBounds& bounds) noexcept
{
    return is_finite(bounds.minimum_m) && is_finite(bounds.maximum_m)
        && bounds.minimum_m.x <= bounds.maximum_m.x
        && bounds.minimum_m.y <= bounds.maximum_m.y
        && bounds.minimum_m.z <= bounds.maximum_m.z;
}

[[nodiscard]] bool valid(const SphericalWorldBounds& bounds) noexcept
{
    return is_finite(bounds.center_m) && std::isfinite(bounds.removal_radius_m)
        && bounds.removal_radius_m > 0.0f;
}

}

WorldBounds::WorldBounds(WorldBoundsConfig config)
    : config_(std::move(config))
{
    const bool is_valid = std::visit(
        [](const auto& bounds) {
            using Bounds = std::decay_t<decltype(bounds)>;
            if constexpr (std::is_same_v<Bounds, NoWorldBounds>) {
                return true;
            } else {
                return valid(bounds);
            }
        },
        config_);
    if (!is_valid) {
        throw std::invalid_argument("world bounds must be finite and ordered");
    }
    std::visit(
        [this](const auto& selected) {
            using Bounds = std::decay_t<decltype(selected)>;
            if constexpr (std::is_same_v<Bounds, NoWorldBounds>) {
                evaluator_ = contains_everywhere;
            } else if constexpr (std::is_same_v<Bounds, AabbWorldBounds>) {
                minimum_m_ = selected.minimum_m;
                maximum_m_ = selected.maximum_m;
                evaluator_ = contains_aabb;
            } else {
                center_m_ = selected.center_m;
                removal_radius_m_ = selected.removal_radius_m;
                evaluator_ = contains_sphere;
            }
        },
        config_);
}

bool WorldBounds::contains(Vec3 position_m) const noexcept
{
    if (!is_finite(position_m)) {
        return false;
    }

    return evaluator_(*this, position_m);
}

bool WorldBounds::contains_everywhere(
    const WorldBounds&, Vec3) noexcept
{
    return true;
}

bool WorldBounds::contains_aabb(
    const WorldBounds& bounds, Vec3 position_m) noexcept
{
    return position_m.x >= bounds.minimum_m_.x && position_m.x <= bounds.maximum_m_.x
        && position_m.y >= bounds.minimum_m_.y && position_m.y <= bounds.maximum_m_.y
        && position_m.z >= bounds.minimum_m_.z && position_m.z <= bounds.maximum_m_.z;
}

bool WorldBounds::contains_sphere(
    const WorldBounds& bounds, Vec3 position_m) noexcept
{
    const Vec3 offset = position_m - bounds.center_m_;
    const double distance_squared = static_cast<double>(offset.x) * offset.x
        + static_cast<double>(offset.y) * offset.y
        + static_cast<double>(offset.z) * offset.z;
    const double removal_radius_squared =
        static_cast<double>(bounds.removal_radius_m_) * bounds.removal_radius_m_;
    return std::isfinite(distance_squared) && distance_squared < removal_radius_squared;
}

}
