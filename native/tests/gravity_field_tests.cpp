#include "test_framework.hpp"

#include <ninho/physics/gravity_field.hpp>
#include <ninho/physics/physics_limits.hpp>
#include <ninho/physics/radial_gravity.hpp>

#include <array>
#include <bit>
#include <cstdint>
#include <limits>
#include <stdexcept>
#include <variant>

using namespace ninho::physics;

namespace {

[[nodiscard]] bool rejects(GravityFieldConfig config)
{
    try {
        const GravityField field{config};
        static_cast<void>(field);
    } catch (const std::invalid_argument&) {
        return true;
    }
    return false;
}

[[nodiscard]] std::array<std::uint32_t, 3> vector_bits(Vec3 value) noexcept
{
    return {
        std::bit_cast<std::uint32_t>(value.x),
        std::bit_cast<std::uint32_t>(value.y),
        std::bit_cast<std::uint32_t>(value.z),
    };
}

}

NINHO_TEST("gravity field modes preserve their typed configuration")
{
    const UniformGravityConfig uniform_config{.acceleration_m_s2 = {1.0f, -9.81f, 2.0f}};
    const GravityField uniform{GravityFieldConfig{uniform_config}};
    NINHO_REQUIRE(std::holds_alternative<UniformGravityConfig>(uniform.config()));
    NINHO_REQUIRE(
        std::get<UniformGravityConfig>(uniform.config()).acceleration_m_s2
        == uniform_config.acceleration_m_s2);

    const RadialGravityConfig radial_config{
        .center_m = {1.0f, -2.0f, 3.0f},
        .reference_radius_m = 10.0f,
        .reference_acceleration_m_s2 = 9.0f,
    };
    const GravityField radial{GravityFieldConfig{radial_config}};
    NINHO_REQUIRE(std::holds_alternative<RadialGravityConfig>(radial.config()));
    const auto& retained = std::get<RadialGravityConfig>(radial.config());
    NINHO_REQUIRE(retained.center_m == radial_config.center_m);
    NINHO_REQUIRE(retained.reference_radius_m == radial_config.reference_radius_m);
    NINHO_REQUIRE(
        retained.reference_acceleration_m_s2
        == radial_config.reference_acceleration_m_s2);
}

NINHO_TEST("radial gravity adapts its legacy configuration type")
{
    const LegacyRadialGravityConfig config{
        .center = {1.0f, -2.0f, 3.0f},
        .radius = 10.0f,
        .surface_acceleration = 9.0f,
    };
    const RadialGravity gravity{config};
    NINHO_REQUIRE((gravity.acceleration({11.0f, -2.0f, 3.0f}) == Vec3{-9.0f, 0.0f, 0.0f}));
}

NINHO_TEST("gravity field modes return uniform acceleration exactly at finite positions")
{
    const Vec3 expected{1.25f, -9.81f, 0.5f};
    const GravityField gravity{GravityFieldConfig{
        UniformGravityConfig{.acceleration_m_s2 = expected},
    }};
    constexpr std::array positions{
        Vec3{},
        Vec3{1.0f, -2.0f, 3.0f},
        Vec3{-100000.0f, 0.125f, 99999.0f},
        Vec3{
            std::numeric_limits<float>::max(),
            std::numeric_limits<float>::lowest(),
            std::numeric_limits<float>::denorm_min(),
        },
    };

    for (const Vec3 position : positions) {
        NINHO_REQUIRE(gravity.acceleration_at(position) == expected);
    }
}

NINHO_TEST("gravity field modes preserve radial surface and inverse square goldens")
{
    const GravityField gravity{GravityFieldConfig{RadialGravityConfig{
        .center_m = {1.0f, -2.0f, 3.0f},
        .reference_radius_m = 10.0f,
        .reference_acceleration_m_s2 = 9.0f,
    }}};

    NINHO_REQUIRE((gravity.acceleration_at({11.0f, -2.0f, 3.0f}) == Vec3{-9.0f, 0.0f, 0.0f}));
    NINHO_REQUIRE((gravity.acceleration_at({-9.0f, -2.0f, 3.0f}) == Vec3{9.0f, 0.0f, 0.0f}));
    NINHO_REQUIRE((gravity.acceleration_at({1.0f, 8.0f, 3.0f}) == Vec3{0.0f, -9.0f, 0.0f}));
    NINHO_REQUIRE((gravity.acceleration_at({1.0f, -2.0f, -7.0f}) == Vec3{0.0f, 0.0f, 9.0f}));
    NINHO_REQUIRE((gravity.acceleration_at({21.0f, -2.0f, 3.0f}) == Vec3{-2.25f, 0.0f, 0.0f}));
    NINHO_REQUIRE((gravity.acceleration_at({-19.0f, -2.0f, 3.0f}) == Vec3{2.25f, 0.0f, 0.0f}));
}

NINHO_TEST("gravity field modes stay finite at the radial center")
{
    const GravityField gravity{GravityFieldConfig{RadialGravityConfig{
        .center_m = {4.0f, -5.0f, 6.0f},
        .reference_radius_m = 10.0f,
        .reference_acceleration_m_s2 = 9.0f,
    }}};
    const Vec3 acceleration = gravity.acceleration_at({4.0f, -5.0f, 6.0f});
    NINHO_REQUIRE(acceleration == Vec3{});
    NINHO_REQUIRE(is_finite(acceleration));
}

NINHO_TEST("gravity field modes reject unsafe configurations")
{
    const float nan = std::numeric_limits<float>::quiet_NaN();
    const float infinity = std::numeric_limits<float>::infinity();

    NINHO_REQUIRE(rejects(UniformGravityConfig{.acceleration_m_s2 = {nan, 0.0f, 0.0f}}));
    NINHO_REQUIRE(rejects(UniformGravityConfig{.acceleration_m_s2 = {0.0f, infinity, 0.0f}}));
    NINHO_REQUIRE(rejects(UniformGravityConfig{
        .acceleration_m_s2 = {maximum_radial_acceleration + 0.01f, 0.0f, 0.0f},
    }));
    NINHO_REQUIRE(rejects(UniformGravityConfig{
        .acceleration_m_s2 = {maximum_radial_acceleration, maximum_radial_acceleration, 0.0f},
    }));

    const auto radial = [](Vec3 center, float radius, float acceleration) {
        return RadialGravityConfig{
            .center_m = center,
            .reference_radius_m = radius,
            .reference_acceleration_m_s2 = acceleration,
        };
    };
    NINHO_REQUIRE(rejects(radial({nan, 0.0f, 0.0f}, 10.0f, 9.0f)));
    NINHO_REQUIRE(rejects(radial({}, 0.0f, 9.0f)));
    NINHO_REQUIRE(rejects(radial({}, -1.0f, 9.0f)));
    NINHO_REQUIRE(rejects(radial({}, infinity, 9.0f)));
    NINHO_REQUIRE(rejects(radial({}, 10.0f, nan)));
    NINHO_REQUIRE(rejects(radial({}, 10.0f, -0.01f)));
    NINHO_REQUIRE(rejects(radial({}, 10.0f, maximum_radial_acceleration + 0.01f)));
    NINHO_REQUIRE(rejects(radial({}, std::numeric_limits<float>::max(), 9.0f)));
}

NINHO_TEST("gravity field modes are bit deterministic under precise floating point")
{
    const GravityField uniform{GravityFieldConfig{UniformGravityConfig{
        .acceleration_m_s2 = {0.125f, -9.81f, 1.75f},
    }}};
    const RadialGravityConfig radial_config{
        .center_m = {1.25f, -2.5f, 0.75f},
        .reference_radius_m = 10.0f,
        .reference_acceleration_m_s2 = 9.0f,
    };
    const GravityField radial{GravityFieldConfig{radial_config}};
    const RadialGravity legacy_radial{radial_config};
    const Vec3 position{17.125f, 3.25f, -4.5f};
    const auto expected_uniform = vector_bits(uniform.acceleration_at(position));
    const auto expected_radial = vector_bits(radial.acceleration_at(position));

    for (int attempt = 0; attempt < 1024; ++attempt) {
        NINHO_REQUIRE(vector_bits(uniform.acceleration_at(position)) == expected_uniform);
        NINHO_REQUIRE(vector_bits(radial.acceleration_at(position)) == expected_radial);
        NINHO_REQUIRE(vector_bits(legacy_radial.acceleration(position)) == expected_radial);
    }
}
