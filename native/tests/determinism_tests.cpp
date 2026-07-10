#include "test_framework.hpp"

#include <ninho/physics/scenario.hpp>

#include <array>
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
