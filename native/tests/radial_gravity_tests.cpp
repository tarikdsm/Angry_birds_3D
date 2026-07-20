#include "test_framework.hpp"

#include <ninho/physics/radial_gravity.hpp>

#include <limits>
#include <type_traits>

using namespace ninho::physics;

static_assert(std::is_trivially_copyable_v<Vec3> && std::is_standard_layout_v<Vec3>);
static_assert(std::is_trivially_copyable_v<Quat> && std::is_standard_layout_v<Quat>);
static_assert(std::is_trivially_copyable_v<Transform> && std::is_standard_layout_v<Transform>);
static_assert(Quat{}.w == 1.0f);
static_assert(Transform{}.rotation.w == 1.0f);
static_assert(Quat{1, 2, 3, 4}.w == 4.0f);
static_assert(Transform{{1, 2, 3}, {4, 5, 6, 7}}.rotation.w == 7.0f);
static_assert(!BodyHandle{}.valid());
static_assert(!BodyHandle{1, 0}.valid());
static_assert(!BodyHandle{0, 1}.valid());
static_assert(BodyHandle{1, 1}.valid());
static_assert(Vec3{1, 2, 3} + Vec3{4, 5, 6} == Vec3{5, 7, 9});
static_assert(Vec3{4, 5, 6} - Vec3{1, 2, 3} == Vec3{3, 3, 3});
static_assert(-Vec3{1, -2, 3} == Vec3{-1, 2, -3});
static_assert(Vec3{1, 2, 3} * 2.0f == Vec3{2, 4, 6});
static_assert(2.0f * Vec3{1, 2, 3} == Vec3{2, 4, 6});
static_assert(Vec3{2, 4, 6} / 2.0f == Vec3{1, 2, 3});
static_assert(dot(Vec3{1, 2, 3}, Vec3{4, 5, 6}) == 32.0f);
static_assert(cross(Vec3{1, 0, 0}, Vec3{0, 1, 0}) == Vec3{0, 0, 1});

NINHO_TEST("radial gravity adapts an explicit legacy configuration")
{
    const LegacyRadialGravityConfig legacy{
        .center = {1.0f, -2.0f, 3.0f},
        .radius = 10.0f,
        .surface_acceleration = 9.0f,
    };
    const RadialGravity gravity{legacy};
    NINHO_REQUIRE((gravity.acceleration({11.0f, -2.0f, 3.0f}) == Vec3{-9.0f, 0.0f, 0.0f}));
}

NINHO_TEST("radial gravity points toward center on every axis")
{
    const auto near_failure = [](double actual, double expected, double epsilon) {
        try {
            NINHO_REQUIRE_NEAR(actual, expected, epsilon);
        } catch (const std::runtime_error& error) {
            return std::string{error.what()};
        }
        return std::string{};
    };
    const double nan = std::numeric_limits<double>::quiet_NaN();
    const double infinity = std::numeric_limits<double>::infinity();
    NINHO_REQUIRE(!near_failure(nan, 0.0, 1e-4).empty());
    NINHO_REQUIRE(!near_failure(0.0, nan, 1e-4).empty());
    NINHO_REQUIRE(!near_failure(0.0, 0.0, nan).empty());
    NINHO_REQUIRE(!near_failure(infinity, 0.0, 1e-4).empty());
    NINHO_REQUIRE(!near_failure(0.0, infinity, 1e-4).empty());
    NINHO_REQUIRE(!near_failure(0.0, 0.0, infinity).empty());
    NINHO_REQUIRE(near_failure(0.0, 0.0, -1.0).find("epsilon") != std::string::npos);

    RadialGravity gravity({.center = {0, 0, 0}, .radius = 10.0f, .surface_acceleration = 9.0f});
    const Vec3 x = gravity.acceleration({10, 0, 0});
    const Vec3 y = gravity.acceleration({0, 10, 0});
    const Vec3 z = gravity.acceleration({0, 0, -10});
    NINHO_REQUIRE_NEAR(x.x, -9.0f, 1e-4f);
    NINHO_REQUIRE_NEAR(x.y, 0.0f, 1e-4f);
    NINHO_REQUIRE_NEAR(x.z, 0.0f, 1e-4f);
    NINHO_REQUIRE_NEAR(y.x, 0.0f, 1e-4f);
    NINHO_REQUIRE_NEAR(y.y, -9.0f, 1e-4f);
    NINHO_REQUIRE_NEAR(y.z, 0.0f, 1e-4f);
    NINHO_REQUIRE_NEAR(z.x, 0.0f, 1e-4f);
    NINHO_REQUIRE_NEAR(z.y, 0.0f, 1e-4f);
    NINHO_REQUIRE_NEAR(z.z, 9.0f, 1e-4f);
}

NINHO_TEST("radial gravity decays, caps near center, and is finite at center")
{
    RadialGravity gravity({.center = {0, 0, 0}, .radius = 10.0f, .surface_acceleration = 9.0f});
    NINHO_REQUIRE_NEAR(length(gravity.acceleration({20, 0, 0})), 2.25f, 1e-4f);
    NINHO_REQUIRE_NEAR(length(gravity.acceleration({6, 0, 0})), 18.0f, 1e-4f);
    const Vec3 center = gravity.acceleration({0, 0, 0});
    NINHO_REQUIRE(center == Vec3{});
    NINHO_REQUIRE(is_finite(center));
    const Vec3 normalized = normalized_or_zero({3, 4, 0});
    NINHO_REQUIRE_NEAR(normalized.x, 0.6f, 1e-6f);
    NINHO_REQUIRE_NEAR(normalized.y, 0.8f, 1e-6f);
    NINHO_REQUIRE(normalized_or_zero({}) == Vec3{});
    NINHO_REQUIRE(gravity.acceleration({0.0005f, 0, 0}) == Vec3{});
    NINHO_REQUIRE_NEAR(length(gravity.acceleration({0.001f, 0, 0})), 18.0f, 1e-4f);

    RadialGravity gentle({.center = {0, 0, 0}, .radius = 10.0f, .surface_acceleration = 1.0f});
    NINHO_REQUIRE_NEAR(length(gentle.acceleration({3, 0, 0})), 100.0f / 36.0f, 1e-4f);
}

NINHO_TEST("radial ejection predicate requires radius speed and duration")
{
    EjectionTracker tracker;
    const BodyHandle body{1, 1};
    for (int i = 0; i < 29; ++i) {
        NINHO_REQUIRE(!tracker.update(body, 41.0f, 2.1f, 1.0f / 60.0f, 10.0f));
    }
    NINHO_REQUIRE(tracker.update(body, 41.0f, 2.1f, 1.0f / 60.0f, 10.0f));
    tracker.reset(body);
    NINHO_REQUIRE(!tracker.update(body, 41.0f, 2.1f, 1.0f / 60.0f, 10.0f));
    NINHO_REQUIRE(!tracker.update(body, 39.9f, 3.0f, 1.0f, 10.0f));

    EjectionTracker generations;
    const BodyHandle first_generation{7, 1};
    const BodyHandle second_generation{7, 2};
    for (int i = 0; i < 29; ++i) {
        NINHO_REQUIRE(!generations.update(first_generation, 41.0f, 2.1f, 1.0f / 60.0f, 10.0f));
    }
    NINHO_REQUIRE(!generations.update(second_generation, 41.0f, 2.1f, 1.0f / 60.0f, 10.0f));

    EjectionTracker interrupted;
    const BodyHandle interrupted_body{9, 1};
    for (int i = 0; i < 29; ++i) {
        NINHO_REQUIRE(!interrupted.update(interrupted_body, 41.0f, 2.1f, 1.0f / 60.0f, 10.0f));
    }
    NINHO_REQUIRE(!interrupted.update(interrupted_body, 41.0f, 1.9f, 1.0f / 60.0f, 10.0f));
    NINHO_REQUIRE(!interrupted.update(interrupted_body, 41.0f, 2.1f, 1.0f / 60.0f, 10.0f));

    EjectionTracker boundary;
    NINHO_REQUIRE(boundary.update(BodyHandle{10, 1}, 40.0f, 2.0f, 0.5f, 10.0f));
}

NINHO_TEST("radial ejection tracker resets and recovers after a non finite sample")
{
    constexpr float valid_dt = 1.0f / 60.0f;
    const float nan = std::numeric_limits<float>::quiet_NaN();
    const auto require_reset = [&](float radius, float radial_speed, float dt,
                                   float planet_radius) {
        EjectionTracker tracker;
        const BodyHandle body{11, 1};
        for (int i = 0; i < 29; ++i) {
            NINHO_REQUIRE(!tracker.update(body, 41.0f, 2.1f, valid_dt, 10.0f));
        }

        NINHO_REQUIRE(!tracker.update(body, radius, radial_speed, dt, planet_radius));
        for (int i = 0; i < 29; ++i) {
            NINHO_REQUIRE(!tracker.update(body, 41.0f, 2.1f, valid_dt, 10.0f));
        }
        NINHO_REQUIRE(tracker.update(body, 41.0f, 2.1f, valid_dt, 10.0f));
    };

    require_reset(nan, 2.1f, valid_dt, 10.0f);
    require_reset(41.0f, nan, valid_dt, 10.0f);
    require_reset(41.0f, 2.1f, nan, 10.0f);
    require_reset(41.0f, 2.1f, valid_dt, nan);
}
