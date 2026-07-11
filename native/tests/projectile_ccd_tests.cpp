#include "test_framework.hpp"

#include <ninho/physics/scenario.hpp>

#include <cstdint>
#include <ranges>

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
    NINHO_REQUIRE(result.joint_count == 0);
    NINHO_REQUIRE(result.peak_body_count == 123);
    NINHO_REQUIRE(result.peak_shape_count == 123);
    NINHO_REQUIRE(result.peak_joint_count == 0);
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

NINHO_TEST("projectile gate reports an unevaluated fallback without a false pass")
{
    const auto result = ScenarioRunner{}.run(ScenarioKind::ProjectilePile, 1, 4);
    const auto limit = std::ranges::find(
        result.limits, "fallback_seed_passes", &ScenarioLimit::name);
    const auto evaluated = std::ranges::find(
        result.metrics, "fallback_seed_evaluated", &ScenarioValue::name);
    NINHO_REQUIRE(limit != result.limits.end());
    NINHO_REQUIRE(evaluated != result.metrics.end());
    if (result.ccd_primary_pass_count == 20) {
        NINHO_REQUIRE(limit->comparison == "==");
        NINHO_REQUIRE(limit->value == 0.0);
        NINHO_REQUIRE(evaluated->value == 0.0);
    } else {
        NINHO_REQUIRE(limit->value == 20.0);
        NINHO_REQUIRE(evaluated->value == 1.0);
    }
}
