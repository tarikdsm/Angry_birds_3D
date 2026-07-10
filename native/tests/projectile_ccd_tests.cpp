#include "test_framework.hpp"

#include <ninho/physics/scenario.hpp>

#include <cstdint>

using namespace ninho::physics;

NINHO_TEST("bullet contacts pile in all fixed seeds")
{
    for (std::uint64_t seed = 1; seed <= 20; ++seed) {
        const auto result =
            ScenarioRunner{}.run(ScenarioKind::ProjectilePile, seed, 4);
        NINHO_REQUIRE(result.contact_before_pile_exit);
        NINHO_REQUIRE(result.violations.empty());
    }
}

NINHO_TEST("projectile gate uses only the approved fixture and fallback")
{
    const auto result = ScenarioRunner{}.run(ScenarioKind::ProjectilePile, 1, 4);
    NINHO_REQUIRE(result.dynamic_body_count == 121);
    NINHO_REQUIRE(result.shape_count == 123);
    NINHO_REQUIRE(result.ccd_primary_pass_count >= 0);
    NINHO_REQUIRE(result.ccd_primary_pass_count <= 20);
    if (result.ccd_primary_pass_count == 20) {
        NINHO_REQUIRE(result.fallback.empty());
        NINHO_REQUIRE(result.projectile_speed == 35.0);
        NINHO_REQUIRE(result.substeps == 4);
        NINHO_REQUIRE(result.ccd_fallback_pass_count == 0);
    } else {
        NINHO_REQUIRE(result.fallback == "speed30_substeps6");
        NINHO_REQUIRE(result.projectile_speed == 30.0);
        NINHO_REQUIRE(result.substeps == 6);
        NINHO_REQUIRE(result.ccd_fallback_pass_count == 20);
    }
    NINHO_REQUIRE(result.violations.empty());
}
