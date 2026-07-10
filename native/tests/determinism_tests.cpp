#include "test_framework.hpp"

#include <ninho/physics/scenario.hpp>

#include <array>
#include <filesystem>
#include <limits>
#include <stdexcept>

using namespace ninho::physics;

NINHO_TEST("canonical state hash uses the pinned quantization and FNV mix")
{
    const BodyState state{
        .handle = {9, 3},
        .transform = {{1, -2, 3}, {0, 0, 0, 1}},
        .linear_velocity = {2, -3, 4},
        .angular_velocity = {0.5f, -0.25f, 0.125f},
        .awake = true,
        .ejected = false,
    };
    NINHO_REQUIRE(hash_states(std::array{state}) == 8671678994761298067ull);
}

NINHO_TEST("canonical state hash is independent of snapshot order")
{
    BodyState first{.handle = {7, 2}, .transform = {{1, 2, 3}, {}}};
    BodyState second{.handle = {3, 5}, .transform = {{4, 5, 6}, {}}};
    NINHO_REQUIRE(
        hash_states(std::array{first, second})
        == hash_states(std::array{second, first}));
}

NINHO_TEST("canonical state hash changes with quantized state")
{
    BodyState original{.handle = {1, 1}, .transform = {{1, 2, 3}, {}}};
    BodyState changed = original;
    changed.transform.position.x += 0.002f;
    NINHO_REQUIRE(hash_states(std::array{original}) != hash_states(std::array{changed}));
}

NINHO_TEST("body state invariant rejects invalid handles and quaternion")
{
    BodyState state{.handle = {1, 1}, .mass = 1};
    NINHO_REQUIRE(!validate_body_state(state, 10).has_value());
    state.handle = {};
    NINHO_REQUIRE(validate_body_state(state, 10)->field == "handle");
    state.handle = {1, 1};
    state.transform.rotation.w = 2.0f;
    NINHO_REQUIRE(validate_body_state(state, 10)->field == "rotation_norm");
}

NINHO_TEST("body state invariant rejects non finite fields and mass")
{
    BodyState state{.handle = {1, 1}, .mass = 1};
    state.transform.position.x = std::numeric_limits<float>::quiet_NaN();
    NINHO_REQUIRE(validate_body_state(state, 10)->field == "position.x");
    state.transform.position = {};
    state.angular_velocity.z = std::numeric_limits<float>::infinity();
    NINHO_REQUIRE(validate_body_state(state, 10)->field == "angular_velocity.z");
    state.angular_velocity = {};
    state.mass = std::numeric_limits<float>::infinity();
    NINHO_REQUIRE(validate_body_state(state, 10)->field == "mass");
}

NINHO_TEST("body state invariant rejects speed and missing ejection")
{
    BodyState state{.handle = {1, 1}, .mass = 1};
    state.linear_velocity = {100.01f, 0, 0};
    NINHO_REQUIRE(validate_body_state(state, 10)->field == "linear_speed");
    state.linear_velocity = {};
    state.transform.position = {60, 0, 0};
    NINHO_REQUIRE(validate_body_state(state, 10)->field == "radius_without_ejection");
    state.ejected = true;
    NINHO_REQUIRE(!validate_body_state(state, 10).has_value());
}

NINHO_TEST("canonical state hash rejects non finite and out of range input")
{
    const auto rejects = [](BodyState state) {
        try {
            static_cast<void>(hash_states(std::array{state}));
        } catch (const std::invalid_argument&) {
            return true;
        }
        return false;
    };
    BodyState state{.handle = {1, 1}, .mass = 1};
    state.transform.position.x = std::numeric_limits<float>::quiet_NaN();
    NINHO_REQUIRE(rejects(state));
    state.transform.position = {};
    state.linear_velocity.y = std::numeric_limits<float>::infinity();
    NINHO_REQUIRE(rejects(state));
    state.linear_velocity = {};
    state.transform.position.x = std::numeric_limits<float>::max();
    NINHO_REQUIRE(rejects(state));
    state.transform.position.x = std::ldexp(1.0f, 63) / 1000.0f;
    NINHO_REQUIRE(rejects(state));
}

NINHO_TEST("same scenario has exact canonical hash")
{
    const auto first = ScenarioRunner{}.run(ScenarioKind::MassRatio, 42, 4);
    const auto second = ScenarioRunner{}.run(ScenarioKind::MassRatio, 42, 4);
    NINHO_REQUIRE(first.violations.empty());
    NINHO_REQUIRE(first.final_hash != 0);
    NINHO_REQUIRE(first.final_hash == second.final_hash);
}

NINHO_TEST("scenario JSON escapes controls while preserving UTF-8")
{
    ScenarioResult result;
    result.name = "aspas\" barra\\ linha\n controle\x01 ação 🐦";
    result.violations.push_back({
        .scenario = result.name,
        .code = "código",
        .message = "tab\tretorno\r",
    });
    const std::string json = result.to_json();
    NINHO_REQUIRE(json.find("aspas\\\" barra\\\\ linha\\n controle\\u0001 ação 🐦") != std::string::npos);
    NINHO_REQUIRE(json.find("tab\\tretorno\\r") != std::string::npos);
    NINHO_REQUIRE(json.find("código") != std::string::npos);
}

NINHO_TEST("scenario JSON rejects non finite telemetry")
{
    ScenarioResult result;
    result.name = "non_finite";
    result.metrics.push_back({
        "bad", std::numeric_limits<double>::infinity(), "m/s"});
    bool rejected = false;
    try {
        static_cast<void>(result.to_json());
    } catch (const std::invalid_argument&) {
        rejected = true;
    }
    NINHO_REQUIRE(rejected);
}

NINHO_TEST("scenario JSON preserves precise integer and textual violation context")
{
    ScenarioResult result;
    result.name = "precise";
    result.violations.push_back({
        .scenario = result.name,
        .code = "hash_mismatch",
        .message = "exact values",
        .details = {
            {"expected_hash", "18446744073709551615"},
            {"value", "NaN"},
        },
    });
    const std::string json = result.to_json();
    NINHO_REQUIRE(json.find("18446744073709551615") != std::string::npos);
    NINHO_REQUIRE(json.find("\"value\":\"NaN\"") != std::string::npos);
}

NINHO_TEST("scenario JSON rejects malformed UTF-8")
{
    const std::array malformed{
        std::string{"\xc0\xaf", 2},
        std::string{"\xed\xa0\x80", 3},
        std::string{"\xe2\x82", 2},
        std::string{"\xf4\x90\x80\x80", 4},
        std::string{"\xe2\x28\xa1", 3},
    };
    for (const std::string& name : malformed) {
        ScenarioResult result;
        result.name = name;
        bool rejected = false;
        try {
            static_cast<void>(result.to_json());
        } catch (const std::invalid_argument&) {
            rejected = true;
        }
        NINHO_REQUIRE(rejected);
    }
}

NINHO_TEST("scenario runner rejects an unknown enum value")
{
    bool rejected = false;
    try {
        static_cast<void>(ScenarioRunner{}.run(
            static_cast<ScenarioKind>(99), 1, 4));
    } catch (const std::invalid_argument&) {
        rejected = true;
    }
    NINHO_REQUIRE(rejected);
}

NINHO_TEST("scenario exit code distinguishes clean violation and hash mismatch")
{
    ScenarioResult clean;
    NINHO_REQUIRE(scenario_exit_code(std::array{clean}, false) == 0);
    NINHO_REQUIRE(scenario_exit_code(std::array{clean}, true) == 1);
    ScenarioResult violated;
    violated.violations.push_back({
        .scenario = "synthetic",
        .code = "fatal",
        .message = "fatal",
    });
    NINHO_REQUIRE(scenario_exit_code(std::array{violated}, false) == 1);
}

NINHO_TEST("scenario watchdog uses sixty seconds and disarms after success")
{
    const auto path = std::filesystem::temp_directory_path()
        / "ninho-watchdog-disarm.json";
    std::error_code error;
    std::filesystem::remove(path, error);
    ScenarioRunner::set_emergency_json_path(path.string());
    NINHO_REQUIRE(ScenarioRunner::watchdog_timeout_seconds() == 60);
    const auto result = ScenarioRunner{}.run(ScenarioKind::RadialFall, 1, 4);
    ScenarioRunner::set_emergency_json_path(std::nullopt);
    NINHO_REQUIRE(result.violations.empty());
    NINHO_REQUIRE(!std::filesystem::exists(path));
}
