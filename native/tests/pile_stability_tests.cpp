#include "test_framework.hpp"

#include <ninho/physics/scenario.hpp>

#include "scenario_configuration.hpp"

#include <algorithm>
#include <array>
#include <string_view>

using namespace ninho::physics;

namespace {

std::string_view assessment_name(PrivateCommitStatus status)
{
    switch (status) {
    case PrivateCommitStatus::Pass:
        return "pass";
    case PrivateCommitStatus::Growth:
        return "growth";
    case PrivateCommitStatus::Unstable:
        return "unstable";
    case PrivateCommitStatus::Unavailable:
        return "unavailable";
    }
    return "unavailable";
}

void require_private_memory_contract(const ScenarioResult& result)
{
    NINHO_REQUIRE(result.private_commit_gate_status == "diagnostic");
    NINHO_REQUIRE(!result.private_commit_gate_applied);
    NINHO_REQUIRE(!result.private_commit_budget_qualified);
    if (!result.private_commit_available) {
        NINHO_REQUIRE(result.private_commit_assessment_status == "unavailable");
        NINHO_REQUIRE(result.private_commit_warmup_samples.empty());
        NINHO_REQUIRE(result.private_commit_cycle_samples.empty());
        NINHO_REQUIRE(!result.private_commit_stable);
        NINHO_REQUIRE(!result.private_commit_terminal_growth);
        return;
    }
    NINHO_REQUIRE(result.private_commit_warmup_samples.size() == 10);
    NINHO_REQUIRE(result.private_commit_cycle_samples.size() == 10);
    const PrivateCommitAssessment expected = assess_private_commit(
        result.private_commit_warmup_samples,
        result.private_commit_cycle_samples);
    NINHO_REQUIRE(result.private_commit_assessment_status == assessment_name(expected.status));
    NINHO_REQUIRE(result.private_commit_stable == expected.stable);
    NINHO_REQUIRE(result.private_commit_terminal_growth == expected.terminal_growth);
    NINHO_REQUIRE(
        result.private_commit_baseline_full_min_bytes
        == expected.baseline_full_min_bytes);
    NINHO_REQUIRE(
        result.private_commit_baseline_central_min_bytes
        == expected.baseline_central_min_bytes);
    NINHO_REQUIRE(
        result.private_commit_baseline_median_bytes
        == expected.baseline_median_bytes);
    NINHO_REQUIRE(
        result.private_commit_baseline_central_max_bytes
        == expected.baseline_central_max_bytes);
    NINHO_REQUIRE(
        result.private_commit_baseline_full_max_bytes
        == expected.baseline_full_max_bytes);
    NINHO_REQUIRE(
        result.private_commit_final_full_min_bytes == expected.final_full_min_bytes);
    NINHO_REQUIRE(
        result.private_commit_final_central_min_bytes
        == expected.final_central_min_bytes);
    NINHO_REQUIRE(
        result.private_commit_final_median_bytes == expected.final_median_bytes);
    NINHO_REQUIRE(
        result.private_commit_final_central_max_bytes
        == expected.final_central_max_bytes);
    NINHO_REQUIRE(
        result.private_commit_final_full_max_bytes == expected.final_full_max_bytes);
    NINHO_REQUIRE_NEAR(result.private_commit_growth_ratio, expected.growth_ratio, 1e-12);
    NINHO_REQUIRE_NEAR(
        result.private_commit_warmup_trimmed_span_ratio,
        expected.warmup_trimmed_span_ratio,
        1e-12);
    NINHO_REQUIRE_NEAR(
        result.private_commit_warmup_full_span_ratio,
        expected.warmup_full_span_ratio,
        1e-12);
    NINHO_REQUIRE_NEAR(
        result.private_commit_measured_trimmed_span_ratio,
        expected.measured_trimmed_span_ratio,
        1e-12);
    NINHO_REQUIRE_NEAR(
        result.private_commit_measured_full_span_ratio,
        expected.measured_full_span_ratio,
        1e-12);
}

void require_working_set_contract(const ScenarioResult& result)
{
    NINHO_REQUIRE(result.working_set_gate_status == "diagnostic");
    NINHO_REQUIRE(!result.working_set_gate_applied);
    NINHO_REQUIRE(!result.working_set_budget_qualified);
    if (!result.working_set_available) {
        NINHO_REQUIRE(result.working_set_assessment_status == "unavailable");
        NINHO_REQUIRE(result.working_set_warmup_samples.empty());
        NINHO_REQUIRE(result.working_set_cycle_samples.empty());
        NINHO_REQUIRE(!result.working_set_stable);
        NINHO_REQUIRE(!result.working_set_terminal_growth);
        return;
    }
    NINHO_REQUIRE(result.working_set_warmup_samples.size() == 10);
    NINHO_REQUIRE(result.working_set_cycle_samples.size() == 10);
    const PrivateCommitAssessment expected = assess_private_commit(
        result.working_set_warmup_samples,
        result.working_set_cycle_samples);
    NINHO_REQUIRE(result.working_set_assessment_status == assessment_name(expected.status));
    NINHO_REQUIRE(result.working_set_stable == expected.stable);
    NINHO_REQUIRE(result.working_set_terminal_growth == expected.terminal_growth);
    NINHO_REQUIRE(result.working_set_baseline_low_bytes == expected.baseline_full_min_bytes);
    NINHO_REQUIRE(
        result.working_set_baseline_central_low_bytes
        == expected.baseline_central_min_bytes);
    NINHO_REQUIRE(result.working_set_baseline_median_bytes == expected.baseline_median_bytes);
    NINHO_REQUIRE(
        result.working_set_baseline_central_high_bytes
        == expected.baseline_central_max_bytes);
    NINHO_REQUIRE(result.working_set_baseline_max_bytes == expected.baseline_full_max_bytes);
    NINHO_REQUIRE(result.working_set_final_low_bytes == expected.final_full_min_bytes);
    NINHO_REQUIRE(
        result.working_set_final_central_low_bytes == expected.final_central_min_bytes);
    NINHO_REQUIRE(result.working_set_final_median_bytes == expected.final_median_bytes);
    NINHO_REQUIRE(
        result.working_set_final_central_high_bytes == expected.final_central_max_bytes);
    NINHO_REQUIRE(result.working_set_final_max_bytes == expected.final_full_max_bytes);
    NINHO_REQUIRE_NEAR(result.working_set_growth_ratio, expected.growth_ratio, 1e-12);
    NINHO_REQUIRE_NEAR(
        result.working_set_warmup_trimmed_span_ratio,
        expected.warmup_trimmed_span_ratio,
        1e-12);
    NINHO_REQUIRE_NEAR(
        result.working_set_warmup_full_span_ratio,
        expected.warmup_full_span_ratio,
        1e-12);
    NINHO_REQUIRE_NEAR(
        result.working_set_measured_trimmed_span_ratio,
        expected.measured_trimmed_span_ratio,
        1e-12);
    NINHO_REQUIRE_NEAR(
        result.working_set_measured_full_span_ratio,
        expected.measured_full_span_ratio,
        1e-12);
}

}

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

NINHO_TEST("rolling energy detects an intermediate growing window")
{
    std::array<double, 360> energies{};
    energies.fill(1.0);
    for (std::size_t tick = 120; tick < 240; ++tick) {
        energies[tick] = 1.10;
    }
    NINHO_REQUIRE_NEAR(
        max_rolling_energy_growth(energies, 120, 1.0e-9), 0.10, 1.0e-12);
    NINHO_REQUIRE_NEAR(
        max_rolling_energy_growth(
            std::span<const double>{energies}.last(120), 120, 1.0e-9),
        0.0,
        1.0e-12);
}

NINHO_TEST("private commit assessment uses trimmed central tail protocol")
{
    const std::array<std::size_t, 10> baseline{
        90, 91, 92, 93, 94, 100, 100, 100, 100, 100};
    const auto status = [&](const std::array<std::size_t, 10>& measured) {
        return assess_private_commit(baseline, measured).status;
    };
    const std::array<std::size_t, 10> empirical{
        1, 1, 1, 1, 1, 4886528, 4296704, 4292608, 4276224, 4272128};
    const std::array<std::size_t, 10> empirical_baseline{
        1, 1, 1, 1, 1, 4263936, 4280320, 4268032, 4255744, 4263936};
    const auto empirical_result = assess_private_commit(empirical_baseline, empirical);
    NINHO_REQUIRE(empirical_result.status == PrivateCommitStatus::Pass);
    NINHO_REQUIRE(empirical_result.measured_trimmed_span_ratio <= 0.05);
    NINHO_REQUIRE(empirical_result.measured_full_span_ratio > 0.05);

    const std::array<std::size_t, 10> single_high{
        1, 1, 1, 1, 1, 100, 100, 100, 100, 140};
    const std::array<std::size_t, 10> single_low{
        1, 1, 1, 1, 1, 60, 100, 100, 100, 100};
    const std::array<std::size_t, 10> low_high{
        1, 1, 1, 1, 1, 60, 100, 100, 100, 140};
    NINHO_REQUIRE(status(single_high) == PrivateCommitStatus::Pass);
    NINHO_REQUIRE(status(single_low) == PrivateCommitStatus::Pass);
    NINHO_REQUIRE(status(low_high) == PrivateCommitStatus::Pass);

    const std::array<std::size_t, 10> two_highs{
        1, 1, 1, 1, 1, 106, 106, 100, 100, 100};
    const std::array<std::size_t, 10> two_lows{
        1, 1, 1, 1, 1, 94, 94, 100, 100, 100};
    NINHO_REQUIRE(status(two_highs) == PrivateCommitStatus::Unstable);
    NINHO_REQUIRE(status(two_lows) == PrivateCommitStatus::Unstable);

    const std::array<std::size_t, 10> median_106{
        1, 1, 1, 1, 1, 106, 106, 106, 106, 106};
    const std::array<std::size_t, 10> exactly_105{
        1, 1, 1, 1, 1, 105, 105, 105, 105, 105};
    NINHO_REQUIRE(status(median_106) == PrivateCommitStatus::Growth);
    NINHO_REQUIRE(status(exactly_105) == PrivateCommitStatus::Pass);

    const std::array<std::size_t, 10> terminal_two{
        1, 1, 1, 1, 1, 104, 104, 104, 106, 106};
    const std::array<std::size_t, 10> terminal_one{
        1, 1, 1, 1, 1, 104, 104, 104, 104, 106};
    NINHO_REQUIRE(status(terminal_two) == PrivateCommitStatus::Growth);
    NINHO_REQUIRE(status(terminal_one) == PrivateCommitStatus::Pass);

    const std::array<std::size_t, 10> unstable_warmup{
        1, 1, 1, 1, 1, 94, 94, 100, 100, 100};
    NINHO_REQUIRE(
        assess_private_commit(unstable_warmup, median_106).status
        == PrivateCommitStatus::Unstable);

    auto zero = baseline;
    zero.back() = 0;
    NINHO_REQUIRE(
        assess_private_commit(zero, exactly_105).status
        == PrivateCommitStatus::Unavailable);
    NINHO_REQUIRE(
        assess_private_commit(
            std::span<const std::size_t>{baseline}.first(9), exactly_105)
            .status
        == PrivateCommitStatus::Unavailable);
}

NINHO_TEST("Box3 allocator observation returns exactly to zero after a complete scenario")
{
    const ScenarioResult result =
        ScenarioRunner{}.run(ScenarioKind::RadialFall, 1, 4);
    NINHO_REQUIRE(result.box3d_allocator.baseline_bytes == 0);
    NINHO_REQUIRE(result.box3d_allocator.final_bytes == 0);
    NINHO_REQUIRE(result.box3d_allocator.max_abs_delta == 0);
    NINHO_REQUIRE(result.box3d_allocator.exact_return);
}

NINHO_TEST("runtime configuration evaluator follows the real CRT configuration")
{
    NINHO_REQUIRE(
        detail::evaluate_runtime_configuration(
            {.release_build = true, .mt_defined = true})
        == detail::RuntimeConfigurationStatus::ReleaseMt);
    NINHO_REQUIRE(
        detail::evaluate_runtime_configuration(
            {.release_build = true, .mt_defined = true, .dll_defined = true})
        == detail::RuntimeConfigurationStatus::ConfigurationMismatch);
    NINHO_REQUIRE(
        detail::evaluate_runtime_configuration(
            {.mt_defined = true, .debug_defined = true})
        == detail::RuntimeConfigurationStatus::Diagnostic);
}

NINHO_TEST("internal configuration dispatch blocks every scenario before execution")
{
    constexpr std::array mismatches{
        detail::RuntimeConfiguration{
            .release_build = true,
            .mt_defined = true,
            .dll_defined = true,
        },
        detail::RuntimeConfiguration{
            .release_build = true,
            .mt_defined = false,
        },
    };
    constexpr std::array kinds{
        ScenarioKind::RadialFall,
        ScenarioKind::ProjectilePile,
        ScenarioKind::RadialPile,
        ScenarioKind::MassRatio,
        ScenarioKind::Stress,
        ScenarioKind::CapabilityMatrix,
    };
    for (const detail::RuntimeConfiguration configuration : mismatches) {
        for (const ScenarioKind kind : kinds) {
            const auto result = detail::configuration_mismatch_result(
                kind, 9, 4, configuration);
            NINHO_REQUIRE(result.has_value());
            NINHO_REQUIRE(result->ticks == 0);
            NINHO_REQUIRE(result->violations.size() == 1);
            NINHO_REQUIRE(result->violations.front().code == "configuration_mismatch");
            NINHO_REQUIRE(scenario_exit_code(std::array{*result}, false) == 1);
        }
    }
}

NINHO_TEST("Debug static CRT configuration remains diagnostic")
{
    constexpr detail::RuntimeConfiguration debug_mtd{
        .release_build = false,
        .mt_defined = true,
        .dll_defined = false,
        .debug_defined = true,
    };
    NINHO_REQUIRE(
        detail::evaluate_runtime_configuration(debug_mtd)
        == detail::RuntimeConfigurationStatus::Diagnostic);
}

NINHO_TEST("unavailable footprint remains diagnostic with limits recommendation")
{
    ScenarioResult result;
    result.name = "unavailable_memory";
    result.warnings.push_back({
        .code = "private_commit_budget_unqualified",
        .message = "budget deferred",
    });
    require_private_memory_contract(result);
    require_working_set_contract(result);
    NINHO_REQUIRE(scenario_exit_code(std::array{result}, false) == 0);
    ScenarioReport report;
    report.scenarios.push_back(result);
    const std::string json = report.to_json();
    NINHO_REQUIRE(json.find("\"private_commit_budget_unqualified\"")
        != std::string::npos);
    NINHO_REQUIRE(json.find("\"recommendation\":\"prosseguir_com_limites\"")
        != std::string::npos);
}

NINHO_TEST("Debug CRT probe detects and then frees an intentional allocation")
{
    const auto clean = debug_crt_allocation_probe(false);
    const auto intentional = debug_crt_allocation_probe(true);
#if defined(_MSC_VER) && defined(_DEBUG)
    NINHO_REQUIRE(clean.applicable);
    NINHO_REQUIRE(clean.balanced);
    NINHO_REQUIRE(intentional.applicable);
    NINHO_REQUIRE(!intentional.balanced);
    NINHO_REQUIRE(intentional.normal_block_count_delta > 0);
    NINHO_REQUIRE(intentional.normal_block_bytes_delta > 0);
    NINHO_REQUIRE(debug_crt_allocation_probe(false).balanced);
#else
    NINHO_REQUIRE(!clean.applicable);
    NINHO_REQUIRE(clean.balanced);
    NINHO_REQUIRE(!intentional.applicable);
#endif
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
    const auto result = ScenarioRunner{}.run(ScenarioKind::Stress, 99, 2);
    NINHO_REQUIRE(result.dynamic_body_count == 500);
    NINHO_REQUIRE(result.shape_count == 800);
    NINHO_REQUIRE(result.joint_count == 250);
    NINHO_REQUIRE(result.warmup_ticks == 300);
    NINHO_REQUIRE(result.measurement_ticks == 1200);
    NINHO_REQUIRE(result.allocator_warmup_cycles == 10);
    NINHO_REQUIRE(result.stress_cycles == 10);
    const auto executed_substeps = std::find_if(
        result.metrics.begin(), result.metrics.end(), [](const ScenarioValue& value) {
            return value.name == "executed_substeps";
        });
    NINHO_REQUIRE(executed_substeps != result.metrics.end());
    NINHO_REQUIRE(executed_substeps->value == 2.0);
#ifdef _WIN32
    require_private_memory_contract(result);
    NINHO_REQUIRE(result.box3d_allocator.baseline_bytes == 0);
    NINHO_REQUIRE(result.box3d_allocator.final_bytes == 0);
    NINHO_REQUIRE(result.box3d_allocator.exact_return);
    NINHO_REQUIRE(result.box3d_allocator.warmup_post_teardown_bytes.size() == 10);
    NINHO_REQUIRE(result.box3d_allocator.measured_post_teardown_bytes.size() == 10);
    NINHO_REQUIRE(std::ranges::all_of(
        result.box3d_allocator.warmup_post_teardown_bytes,
        [](std::int64_t bytes) { return bytes == 0; }));
    NINHO_REQUIRE(std::ranges::all_of(
        result.box3d_allocator.measured_post_teardown_bytes,
        [](std::int64_t bytes) { return bytes == 0; }));
#if defined(_MSC_VER) && defined(_DEBUG)
    NINHO_REQUIRE(!result.private_commit_gate_applied);
    NINHO_REQUIRE(result.private_commit_gate_status == "diagnostic");
    NINHO_REQUIRE(result.crt.applicable);
    NINHO_REQUIRE(result.crt.balanced);
#endif
    require_working_set_contract(result);
#endif
    NINHO_REQUIRE(result.violations.empty());
}
