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
}

bool WorldBounds::contains(Vec3 position_m) const noexcept
{
    if (!is_finite(position_m)) {
        return false;
    }

    return std::visit(
        [position_m](const auto& bounds) {
            using Bounds = std::decay_t<decltype(bounds)>;
            if constexpr (std::is_same_v<Bounds, NoWorldBounds>) {
                return true;
            } else if constexpr (std::is_same_v<Bounds, AabbWorldBounds>) {
                return position_m.x >= bounds.minimum_m.x && position_m.x <= bounds.maximum_m.x
                    && position_m.y >= bounds.minimum_m.y && position_m.y <= bounds.maximum_m.y
                    && position_m.z >= bounds.minimum_m.z && position_m.z <= bounds.maximum_m.z;
            } else {
                const Vec3 offset = position_m - bounds.center_m;
                const double distance_squared = static_cast<double>(offset.x) * offset.x
                    + static_cast<double>(offset.y) * offset.y
                    + static_cast<double>(offset.z) * offset.z;
                const double removal_radius_squared =
                    static_cast<double>(bounds.removal_radius_m) * bounds.removal_radius_m;
                return std::isfinite(distance_squared) && distance_squared < removal_radius_squared;
            }
        },
        config_);
}

}
