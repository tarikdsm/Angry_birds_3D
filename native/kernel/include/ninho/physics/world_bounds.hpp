#pragma once

#include <ninho/physics/physics_types.hpp>

#include <variant>

namespace ninho::physics {

struct NoWorldBounds {};

struct AabbWorldBounds {
    Vec3 minimum_m{};
    Vec3 maximum_m{};
};

struct SphericalWorldBounds {
    Vec3 center_m{};
    float removal_radius_m{};
};

using WorldBoundsConfig = std::variant<NoWorldBounds, AabbWorldBounds, SphericalWorldBounds>;

class WorldBounds {
public:
    explicit WorldBounds(WorldBoundsConfig config);

    [[nodiscard]] bool contains(Vec3 position_m) const noexcept;

private:
    WorldBoundsConfig config_;
};

}
