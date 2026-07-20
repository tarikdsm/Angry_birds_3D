#include "test_framework.hpp"

#include <ninho/physics/world_bounds.hpp>

#include <limits>
#include <stdexcept>
#include <utility>

using namespace ninho::physics;

NINHO_TEST("world bounds includes AABB faces and excludes an epsilon beyond them")
{
    const WorldBounds bounds{AabbWorldBounds{
        .minimum_m = {-24.0f, -12.0f, -12.0f},
        .maximum_m = {48.0f, 32.0f, 12.0f},
    }};

    NINHO_REQUIRE(bounds.contains({-24.0f, -12.0f, -12.0f}));
    NINHO_REQUIRE(bounds.contains({48.0f, 32.0f, 12.0f}));
    NINHO_REQUIRE(!bounds.contains({
        std::nextafter(48.0f, std::numeric_limits<float>::infinity()), 0.0f, 0.0f}));
    NINHO_REQUIRE(!bounds.contains({
        0.0f, std::nextafter(-12.0f, -std::numeric_limits<float>::infinity()), 0.0f}));
}

NINHO_TEST("world bounds preserves the legacy spherical six radius removal boundary")
{
    const WorldBounds bounds{SphericalWorldBounds{
        .center_m = {},
        .removal_radius_m = 60.0f,
    }};

    NINHO_REQUIRE(bounds.contains({0.0f, 59.999f, 0.0f}));
    NINHO_REQUIRE(!bounds.contains({0.0f, 60.0f, 0.0f}));
}

NINHO_TEST("world bounds rejects invalid and non finite configurations")
{
    const auto rejected = [](WorldBoundsConfig config) {
        try {
            static_cast<void>(WorldBounds{std::move(config)});
        } catch (const std::invalid_argument&) {
            return true;
        }
        return false;
    };

    NINHO_REQUIRE(rejected(AabbWorldBounds{{1.0f, 0.0f, 0.0f}, {0.0f, 1.0f, 1.0f}}));
    NINHO_REQUIRE(rejected(AabbWorldBounds{{}, {
        std::numeric_limits<float>::infinity(), 1.0f, 1.0f}}));
    NINHO_REQUIRE(rejected(SphericalWorldBounds{{}, 0.0f}));
    NINHO_REQUIRE(rejected(SphericalWorldBounds{
        {std::numeric_limits<float>::quiet_NaN(), 0.0f, 0.0f}, 1.0f}));
}

NINHO_TEST("world bounds results are independent of call order")
{
    const WorldBounds bounds{AabbWorldBounds{
        .minimum_m = {-1.0f, -1.0f, -1.0f},
        .maximum_m = {1.0f, 1.0f, 1.0f},
    }};

    const Vec3 inside{0.5f, -0.5f, 0.0f};
    const Vec3 outside{1.01f, 0.0f, 0.0f};
    const bool inside_first = bounds.contains(inside);
    const bool outside_second = bounds.contains(outside);
    const bool outside_first = bounds.contains(outside);
    const bool inside_second = bounds.contains(inside);

    NINHO_REQUIRE(inside_first && inside_second);
    NINHO_REQUIRE(!outside_first && !outside_second);
}
