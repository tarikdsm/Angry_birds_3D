#include "test_framework.hpp"
#include "product_v2_playthrough_fixture.hpp"

#include <algorithm>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

namespace {

std::string read_farm_fixture(std::string_view route_id)
{
    const std::filesystem::path path = std::filesystem::path{NINHO_SOURCE_DIR}
        / "native/simulation/tests/fixtures/product_v2"
        / (std::string{route_id} + ".playthrough.json");
    std::ifstream input{path, std::ios::binary};
    NINHO_SIM_REQUIRE(input.is_open());
    std::ostringstream document;
    document << input.rdbuf();
    return document.str();
}

NINHO_SIM_TEST("product v2 determinism farm defeat fixture freezes four misses")
{
    using namespace ninho::simulation;
        const auto fixture = test::parse_frozen_playthrough_fixture_envelope(
        read_farm_fixture("farm_defeat"));
    NINHO_SIM_REQUIRE(fixture.ok());
    NINHO_SIM_REQUIRE(fixture.value.schema_version == 1U);
    NINHO_SIM_REQUIRE(fixture.value.route_id == "farm_defeat");
    NINHO_SIM_REQUIRE(fixture.value.route.world_id == "earth");
    NINHO_SIM_REQUIRE(fixture.value.route.level_id == "farm_reaction");
    NINHO_SIM_REQUIRE(fixture.value.route.shots.size() == 4U);
    NINHO_SIM_REQUIRE(fixture.value.legacy_quantized_launches.size() == 4U);
    for (std::size_t index = 0U; index < 4U; ++index) {
        const GestureShotInput& shot = fixture.value.route.shots[index];
        NINHO_SIM_REQUIRE((shot.camera_right
            == ninho::physics::Vec3{1.0F, 0.0F, 0.0F}));
        NINHO_SIM_REQUIRE(shot.pull_horizontal_m == 0.0);
        NINHO_SIM_REQUIRE(shot.pull_vertical_m == -4.0);
        NINHO_SIM_REQUIRE(!shot.ability_tick_after_launch);
        NINHO_SIM_REQUIRE(!fixture.value.legacy_quantized_launches[index]);
    }
    NINHO_SIM_REQUIRE(fixture.value.expected.outcome == Outcome::Defeat);
    NINHO_SIM_REQUIRE(fixture.value.expected.score == 0U);
    NINHO_SIM_REQUIRE(fixture.value.expected.stars == 0U);
    NINHO_SIM_REQUIRE(fixture.value.expected.birds_remaining == 0U);
    NINHO_SIM_REQUIRE(!fixture.value.expected.objectives_complete);
    NINHO_SIM_REQUIRE(!fixture.value.expected.ordered_events_v3.empty());
    constexpr std::size_t ordered_event_header_bytes = 29U;
    constexpr std::size_t ordered_event_record_bytes = 274U;
    constexpr std::size_t defeat_event_count = 8U;
    NINHO_SIM_REQUIRE(fixture.value.expected.ordered_events_v3.size()
        == ordered_event_header_bytes
            + defeat_event_count * ordered_event_record_bytes);
    NINHO_SIM_REQUIRE(!fixture.value.expected.canonical_state_v3.empty());
    NINHO_SIM_REQUIRE(
        !fixture.value.expected.canonical_playthrough_v5.empty());

    const auto replayed = test::replay_frozen_playthrough(fixture.value);
    NINHO_SIM_REQUIRE(replayed.ok());
    NINHO_SIM_REQUIRE(replayed.value.actual == fixture.value.expected);
    NINHO_SIM_REQUIRE(replayed.value.shots.size() == 4U);
    for (const test::FrozenReplayedShot& shot : replayed.value.shots) {
        NINHO_SIM_REQUIRE(!shot.quantized_launch);
        NINHO_SIM_REQUIRE(!shot.ability_started_tick);
        NINHO_SIM_REQUIRE(!shot.activation_consumed);
        NINHO_SIM_REQUIRE((shot.locked_camera_right
            == ninho::physics::Vec3{1.0F, 0.0F, 0.0F}));
    }
}

NINHO_SIM_TEST("product v2 determinism farm chain fixture freezes three star route")
{
    using namespace ninho::simulation;
    const std::vector<GestureShotInput> expected_shots{
        {{0.984807753F, 0.0F, -0.173648178F},
            -4.0, -0.4, std::uint64_t{10U}},
        {{0.998629535F, 0.0F, -0.052335956F},
            -3.7099248270557466, -0.82247053296479089,
            std::uint64_t{34U}},
    };
    const auto fixture = test::parse_frozen_playthrough_fixture_envelope(
        read_farm_fixture("farm_chain"));
    NINHO_SIM_REQUIRE(fixture.ok());
    NINHO_SIM_REQUIRE(fixture.value.schema_version == 1U);
    NINHO_SIM_REQUIRE(fixture.value.route_id == "farm_chain");
    NINHO_SIM_REQUIRE(fixture.value.route.world_id == "earth");
    NINHO_SIM_REQUIRE(fixture.value.route.level_id == "farm_reaction");
    NINHO_SIM_REQUIRE(fixture.value.route.shots == expected_shots);
    NINHO_SIM_REQUIRE(fixture.value.legacy_quantized_launches.size() == 2U);
    NINHO_SIM_REQUIRE(!fixture.value.legacy_quantized_launches[0]);
    NINHO_SIM_REQUIRE(!fixture.value.legacy_quantized_launches[1]);
    NINHO_SIM_REQUIRE(fixture.value.expected.outcome == Outcome::Victory);
    NINHO_SIM_REQUIRE(fixture.value.expected.score == 50054U);
    NINHO_SIM_REQUIRE(fixture.value.expected.stars == 3U);
    NINHO_SIM_REQUIRE(fixture.value.expected.birds_remaining == 2U);
    NINHO_SIM_REQUIRE(fixture.value.expected.objectives_complete);
    NINHO_SIM_REQUIRE(!fixture.value.expected.ordered_events_v3.empty());
    constexpr std::size_t ordered_event_header_bytes = 29U;
    constexpr std::size_t ordered_event_record_bytes = 274U;
    constexpr std::size_t chain_event_count = 610U;
    NINHO_SIM_REQUIRE(fixture.value.expected.ordered_events_v3.size()
        == ordered_event_header_bytes
            + chain_event_count * ordered_event_record_bytes);
    NINHO_SIM_REQUIRE(!fixture.value.expected.canonical_state_v3.empty());
    NINHO_SIM_REQUIRE(fixture.value.expected.canonical_state_v3_fnv1a64 != 0U);
    NINHO_SIM_REQUIRE(
        !fixture.value.expected.canonical_playthrough_v5.empty());
    NINHO_SIM_REQUIRE(
        fixture.value.expected.canonical_playthrough_v5_fnv1a64 != 0U);

    const auto replayed = test::replay_frozen_playthrough(fixture.value);
    NINHO_SIM_REQUIRE(replayed.ok());
    NINHO_SIM_REQUIRE(replayed.value.actual == fixture.value.expected);
    NINHO_SIM_REQUIRE(replayed.value.shots.size() == expected_shots.size());
}

NINHO_SIM_TEST("product v2 determinism farm tutorial fixture activates all four abilities")
{
    using namespace ninho::simulation;
    const std::vector<GestureShotInput> expected_shots{
        {{0.984807753F, 0.0F, -0.173648178F},
            -4.0, -0.4, std::uint64_t{10U}},
        {{1.0F, 0.0F, 0.0F}, 0.0, -4.0, std::uint64_t{10U}},
        {{0.99756405F, 0.0F, 0.069756474F},
            -4.0, -0.3, std::uint64_t{36U}},
        {{0.992546152F, 0.0F, -0.121869343F},
            -3.7, -1.5, std::uint64_t{105U}},
    };
    const auto fixture = test::parse_frozen_playthrough_fixture_envelope(
        read_farm_fixture("farm_tutorial"));
    NINHO_SIM_REQUIRE(fixture.ok());
    NINHO_SIM_REQUIRE(fixture.value.schema_version == 1U);
    NINHO_SIM_REQUIRE(fixture.value.route_id == "farm_tutorial");
    NINHO_SIM_REQUIRE(fixture.value.route.world_id == "earth");
    NINHO_SIM_REQUIRE(fixture.value.route.level_id == "farm_reaction");
    NINHO_SIM_REQUIRE(fixture.value.route.shots == expected_shots);
    NINHO_SIM_REQUIRE(fixture.value.legacy_quantized_launches.size() == 4U);
    NINHO_SIM_REQUIRE(std::ranges::none_of(
        fixture.value.legacy_quantized_launches,
        [](const auto& launch) { return launch.has_value(); }));
    NINHO_SIM_REQUIRE(fixture.value.expected.outcome == Outcome::Victory);
    NINHO_SIM_REQUIRE(fixture.value.expected.score == 28972U);
    NINHO_SIM_REQUIRE(fixture.value.expected.stars == 1U);
    NINHO_SIM_REQUIRE(fixture.value.expected.birds_remaining == 0U);
    NINHO_SIM_REQUIRE(fixture.value.expected.objectives_complete);
    NINHO_SIM_REQUIRE(!fixture.value.expected.ordered_events_v3.empty());
    constexpr std::size_t ordered_event_header_bytes = 29U;
    constexpr std::size_t ordered_event_record_bytes = 274U;
    constexpr std::size_t tutorial_event_count = 685U;
    NINHO_SIM_REQUIRE(fixture.value.expected.ordered_events_v3.size()
        == ordered_event_header_bytes
            + tutorial_event_count * ordered_event_record_bytes);
    NINHO_SIM_REQUIRE(!fixture.value.expected.canonical_state_v3.empty());
    NINHO_SIM_REQUIRE(
        !fixture.value.expected.canonical_playthrough_v5.empty());

    const auto replayed = test::replay_frozen_playthrough(fixture.value);
    NINHO_SIM_REQUIRE(replayed.ok());
    NINHO_SIM_REQUIRE(replayed.value.actual == fixture.value.expected);
    NINHO_SIM_REQUIRE(replayed.value.shots.size() == 4U);
    for (std::size_t index = 0U; index < replayed.value.shots.size(); ++index) {
        const test::FrozenReplayedShot& shot = replayed.value.shots[index];
        NINHO_SIM_REQUIRE(shot.bird_archetype_id.value() == index + 2U);
        NINHO_SIM_REQUIRE(shot.ability_id.value() == index + 2U);
        NINHO_SIM_REQUIRE(shot.ability_started_tick.has_value());
        NINHO_SIM_REQUIRE(shot.ability_started_tick->value()
            == shot.launch_tick.value()
                + *expected_shots[index].ability_tick_after_launch);
        NINHO_SIM_REQUIRE(shot.activation_consumed);
    }
}

NINHO_SIM_TEST("product v2 determinism farm blue alternative fixture requires split")
{
    using namespace ninho::simulation;
    const std::vector<GestureShotInput> expected_shots{
        {{0.984807753F, 0.0F, -0.173648178F},
            -4.0, -0.4, std::uint64_t{10U}},
        {{0.998629535F, 0.0F, -0.052335956F},
            -3.7587396274117437, -0.83329251366169610,
            std::uint64_t{34U}},
        {{0.999450207F, 0.0F, 0.033155177F},
            -3.9815847934687154, -0.38338301008089593,
            std::uint64_t{32U}},
    };
    const auto fixture = test::parse_frozen_playthrough_fixture_envelope(
        read_farm_fixture("farm_blue_alternative"));
    NINHO_SIM_REQUIRE(fixture.ok());
    NINHO_SIM_REQUIRE(fixture.value.schema_version == 1U);
    NINHO_SIM_REQUIRE(fixture.value.route_id == "farm_blue_alternative");
    NINHO_SIM_REQUIRE(fixture.value.route.world_id == "earth");
    NINHO_SIM_REQUIRE(fixture.value.route.level_id == "farm_reaction");
    NINHO_SIM_REQUIRE(fixture.value.route.shots == expected_shots);
    NINHO_SIM_REQUIRE(fixture.value.legacy_quantized_launches.size() == 3U);
    NINHO_SIM_REQUIRE(std::ranges::none_of(
        fixture.value.legacy_quantized_launches,
        [](const auto& launch) { return launch.has_value(); }));
    NINHO_SIM_REQUIRE(fixture.value.expected.outcome == Outcome::Victory);
    NINHO_SIM_REQUIRE(fixture.value.expected.score == 40022U);
    NINHO_SIM_REQUIRE(fixture.value.expected.stars == 2U);
    NINHO_SIM_REQUIRE(fixture.value.expected.birds_remaining == 1U);
    NINHO_SIM_REQUIRE(fixture.value.expected.objectives_complete);
    NINHO_SIM_REQUIRE(!fixture.value.expected.ordered_events_v3.empty());
    constexpr std::size_t ordered_event_header_bytes = 29U;
    constexpr std::size_t ordered_event_record_bytes = 274U;
    constexpr std::size_t alternative_event_count = 792U;
    NINHO_SIM_REQUIRE(fixture.value.expected.ordered_events_v3.size()
        == ordered_event_header_bytes
            + alternative_event_count * ordered_event_record_bytes);
    NINHO_SIM_REQUIRE(!fixture.value.expected.canonical_state_v3.empty());
    NINHO_SIM_REQUIRE(
        !fixture.value.expected.canonical_playthrough_v5.empty());

    const auto replayed = test::replay_frozen_playthrough(fixture.value);
    NINHO_SIM_REQUIRE(replayed.ok());
    NINHO_SIM_REQUIRE(replayed.value.actual == fixture.value.expected);
    NINHO_SIM_REQUIRE(replayed.value.shots.size() == 3U);
    const test::FrozenReplayedShot& blue = replayed.value.shots[2];
    NINHO_SIM_REQUIRE(blue.bird_archetype_id.value() == 4U);
    NINHO_SIM_REQUIRE(blue.ability_id.value() == 4U);
    NINHO_SIM_REQUIRE(blue.ability_started_tick.has_value());
    NINHO_SIM_REQUIRE(blue.ability_started_tick->value()
        == blue.launch_tick.value() + 32U);
    NINHO_SIM_REQUIRE(blue.activation_consumed);
    NINHO_SIM_REQUIRE(blue.primary_projectile_id.value() != 0U);
}

}
