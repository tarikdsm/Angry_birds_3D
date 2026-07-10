#include "test_framework.hpp"

#include <ninho/physics/scenario.hpp>

#include <algorithm>
#include <string_view>

using namespace ninho::physics;

NINHO_TEST("radial fall settles inside every fixed limit")
{
    const auto result = ScenarioRunner{}.run(ScenarioKind::RadialFall, 1, 4);
    NINHO_REQUIRE(result.ticks == 600);
    NINHO_REQUIRE(result.surface_separation >= -0.02);
    NINHO_REQUIRE(result.surface_separation <= 0.03);
    NINHO_REQUIRE(result.final_linear_speed < 0.05);
    NINHO_REQUIRE(result.final_angular_speed < 0.10);
    NINHO_REQUIRE(result.max_energy_growth_ratio <= 0.02);
    NINHO_REQUIRE(result.violations.empty());
}

NINHO_TEST("radial pile meets predetermined sleep limits")
{
    const auto result = ScenarioRunner{}.run(ScenarioKind::RadialPile, 7, 4);
    NINHO_REQUIRE(result.ticks == 1800);
    NINHO_REQUIRE(result.dynamic_body_count == 80);
    NINHO_REQUIRE(result.p95_linear_speed < 0.05);
    NINHO_REQUIRE(result.p95_angular_speed < 0.10);
    NINHO_REQUIRE(result.sleep_ratio >= 0.90);
    const auto metric = [&](std::string_view name) {
        const auto found = std::find_if(
            result.metrics.begin(), result.metrics.end(), [&](const ScenarioValue& value) {
                return value.name == name;
            });
        NINHO_REQUIRE(found != result.metrics.end());
        return found->value;
    };
    NINHO_REQUIRE_NEAR(metric("max_pair_penetration"), 0.0, 0.019999);
    NINHO_REQUIRE_NEAR(metric("max_platform_penetration"), 0.0, 0.019999);
    NINHO_REQUIRE_NEAR(result.max_penetration, 0.0, 0.019999);
    NINHO_REQUIRE(result.max_energy_growth_ratio <= 0.02);
    NINHO_REQUIRE(result.violations.empty());
}

NINHO_TEST("mass ratio pile keeps the fixed density range stable")
{
    const auto result = ScenarioRunner{}.run(ScenarioKind::MassRatio, 42, 4);
    NINHO_REQUIRE(result.minimum_density == 85.0);
    NINHO_REQUIRE(result.maximum_density == 3400.0);
    NINHO_REQUIRE(result.p95_linear_speed < 0.10);
    NINHO_REQUIRE(result.p95_angular_speed < 0.20);
    NINHO_REQUIRE(result.max_penetration < 0.025);
    NINHO_REQUIRE(result.sleep_ratio >= 0.80);
    NINHO_REQUIRE(result.violations.empty());
}

NINHO_TEST("stress uses the full fixed topology after allocator warmup")
{
    const auto result = ScenarioRunner{}.run(ScenarioKind::Stress, 99, 4);
    NINHO_REQUIRE(result.dynamic_body_count == 500);
    NINHO_REQUIRE(result.shape_count == 800);
    NINHO_REQUIRE(result.joint_count == 250);
    NINHO_REQUIRE(result.warmup_ticks == 300);
    NINHO_REQUIRE(result.measurement_ticks == 1200);
    NINHO_REQUIRE(result.stress_cycles == 10);
#ifdef _WIN32
    NINHO_REQUIRE(result.working_set_available);
    NINHO_REQUIRE(result.working_set_warmup_samples.size() == 3);
    NINHO_REQUIRE(result.working_set_cycle_samples.size() == 10);
    NINHO_REQUIRE(result.working_set_baseline_low_bytes > 0);
    NINHO_REQUIRE(result.working_set_final_low_bytes > 0);
    NINHO_REQUIRE(result.working_set_final_low_bytes <= result.working_set_peak_bytes);
#endif
    NINHO_REQUIRE(result.working_set_growth_ratio <= 0.05);
    NINHO_REQUIRE(result.violations.empty());
}
