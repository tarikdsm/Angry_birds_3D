#pragma once

#include <ninho/physics/physics_types.hpp>

#include <variant>

namespace ninho::physics {

namespace detail {
#if defined(NINHO_ENABLE_TEST_FACADES)
class PhysicsWorldTestFacade;
#endif
}

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
    using Evaluator = bool (*)(const WorldBounds&, Vec3) noexcept;
    [[nodiscard]] static bool contains_everywhere(
        const WorldBounds&, Vec3) noexcept;
    [[nodiscard]] static bool contains_aabb(
        const WorldBounds&, Vec3) noexcept;
    [[nodiscard]] static bool contains_sphere(
        const WorldBounds&, Vec3) noexcept;

#if defined(NINHO_ENABLE_TEST_FACADES)
    friend class detail::PhysicsWorldTestFacade;
#endif
    WorldBoundsConfig config_;
    Vec3 minimum_m_{};
    Vec3 maximum_m_{};
    Vec3 center_m_{};
    float removal_radius_m_{};
    Evaluator evaluator_{};
};

}
