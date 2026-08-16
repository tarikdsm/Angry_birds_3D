#include "product_v2_playthrough_fixture.hpp"
#include "product_v2_layout_fixture.hpp"
#include "product_v2_fixture_writer.hpp"

#include <nlohmann/json.hpp>

#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <optional>
#include <span>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace {

using ninho::simulation::GestureShotInput;
using ninho::simulation::Outcome;
using ninho::simulation::PlaythroughRouteDefinition;
using ninho::simulation::test::FixtureDocument;
using ninho::simulation::test::FrozenPlaythroughFixture;

struct RouteSeed {
    std::string route_id;
    PlaythroughRouteDefinition route;
    Outcome expected_outcome;
    std::uint64_t expected_score;
    std::uint32_t expected_stars;
    std::uint32_t expected_birds_remaining;
    bool expected_objectives_complete;
    std::optional<std::size_t> expected_event_count;
};

[[noreturn]] void fail(std::string message)
{
    throw std::runtime_error(std::move(message));
}

void require(bool condition, std::string_view message)
{
    if (!condition) fail(std::string{message});
}

std::vector<RouteSeed> route_seeds()
{
    return {
        {
            .route_id = "orbital_virela",
            .route = {
                .world_id = "orbital",
                .level_id = "first_orbit_v2",
                .shots = {
                    {{0.0F, 0.99999845F, -0.00174532842F},
                        -2.127, 0.0, std::uint64_t{43U}},
                    {{0.0F, 0.990268052F, 0.1391731F},
                        -2.175, 0.0, std::nullopt},
                },
            },
            .expected_outcome = Outcome::Victory,
            .expected_score = 15000U,
            .expected_stars = 2U,
            .expected_birds_remaining = 1U,
            .expected_objectives_complete = true,
            .expected_event_count = 98U,
        },
        {
            .route_id = "orbital_structural",
            .route = {
                .world_id = "orbital",
                .level_id = "first_orbit_v2",
                .shots = {
                    {{0.0F, 0.9961947F, 0.0871557444F},
                        -2.127, 0.0, std::nullopt},
                    {{0.0F, 0.9998477F, 0.0174524058F},
                        -2.127, 0.0, std::nullopt},
                },
            },
            .expected_outcome = Outcome::Victory,
            .expected_score = 15660U,
            .expected_stars = 2U,
            .expected_birds_remaining = 1U,
            .expected_objectives_complete = true,
            .expected_event_count = 134U,
        },
        {
            .route_id = "orbital_defeat",
            .route = {
                .world_id = "orbital",
                .level_id = "first_orbit_v2",
                .shots = {
                    {{0.0F, 0.0F, 1.0F}, -2.127, 0.0, std::nullopt},
                    {{0.0F, 0.0F, 1.0F}, -2.127, 0.0, std::nullopt},
                    {{0.0F, 0.0F, 1.0F}, -2.127, 0.0, std::nullopt},
                },
            },
            .expected_outcome = Outcome::Defeat,
            .expected_score = 0U,
            .expected_stars = 0U,
            .expected_birds_remaining = 0U,
            .expected_objectives_complete = false,
            .expected_event_count = 4U,
        },
        {
            .route_id = "farm_tutorial",
            .route = {
                .world_id = "earth",
                .level_id = "farm_reaction",
                .shots = {
                    {{0.984807753F, 0.0F, -0.173648178F},
                        -4.0, -0.4, std::uint64_t{10U}},
                    {{1.0F, 0.0F, 0.0F},
                        0.0, -4.0, std::uint64_t{10U}},
                    {{0.99756405F, 0.0F, 0.069756474F},
                        -4.0, -0.3, std::uint64_t{36U}},
                    {{0.992546152F, 0.0F, -0.121869343F},
                        -3.7, -1.5, std::uint64_t{105U}},
                },
            },
            .expected_outcome = Outcome::Victory,
            .expected_score = 28972U,
            .expected_stars = 1U,
            .expected_birds_remaining = 0U,
            .expected_objectives_complete = true,
            .expected_event_count = 685U,
        },
        {
            .route_id = "farm_chain",
            .route = {
                .world_id = "earth",
                .level_id = "farm_reaction",
                .shots = {
                    {{0.984807753F, 0.0F, -0.173648178F},
                        -4.0, -0.4, std::uint64_t{10U}},
                    {{0.998629535F, 0.0F, -0.052335956F},
                        -3.7099248270557466, -0.82247053296479089,
                        std::uint64_t{34U}},
                },
            },
            .expected_outcome = Outcome::Victory,
            .expected_score = 50054U,
            .expected_stars = 3U,
            .expected_birds_remaining = 2U,
            .expected_objectives_complete = true,
            .expected_event_count = 610U,
        },
        {
            .route_id = "farm_blue_alternative",
            .route = {
                .world_id = "earth",
                .level_id = "farm_reaction",
                .shots = {
                    {{0.984807753F, 0.0F, -0.173648178F},
                        -4.0, -0.4, std::uint64_t{10U}},
                    {{0.998629535F, 0.0F, -0.052335956F},
                        -3.7587396274117437, -0.83329251366169610,
                        std::uint64_t{34U}},
                    {{0.999450207F, 0.0F, 0.033155177F},
                        -3.9815847934687154, -0.38338301008089593,
                        std::uint64_t{32U}},
                },
            },
            .expected_outcome = Outcome::Victory,
            .expected_score = 40022U,
            .expected_stars = 2U,
            .expected_birds_remaining = 1U,
            .expected_objectives_complete = true,
            .expected_event_count = 792U,
        },
        {
            .route_id = "farm_defeat",
            .route = {
                .world_id = "earth",
                .level_id = "farm_reaction",
                .shots = {
                    {{1.0F, 0.0F, 0.0F}, 0.0, -4.0, std::nullopt},
                    {{1.0F, 0.0F, 0.0F}, 0.0, -4.0, std::nullopt},
                    {{1.0F, 0.0F, 0.0F}, 0.0, -4.0, std::nullopt},
                    {{1.0F, 0.0F, 0.0F}, 0.0, -4.0, std::nullopt},
                },
            },
            .expected_outcome = Outcome::Defeat,
            .expected_score = 0U,
            .expected_stars = 0U,
            .expected_birds_remaining = 0U,
            .expected_objectives_complete = false,
            .expected_event_count = 8U,
        },
    };
}

std::string hex_bytes(std::span<const std::uint8_t> bytes)
{
    constexpr char digits[] = "0123456789abcdef";
    std::string text;
    text.reserve(bytes.size() * 2U);
    for (const std::uint8_t byte : bytes) {
        text.push_back(digits[byte >> 4U]);
        text.push_back(digits[byte & 0x0fU]);
    }
    return text;
}

std::string hash_text(std::uint64_t value)
{
    constexpr char digits[] = "0123456789abcdef";
    std::string text{"fnv1a64:"};
    for (int shift = 60; shift >= 0; shift -= 4) {
        text.push_back(digits[(value >> shift) & 0x0fU]);
    }
    return text;
}

std::string outcome_text(Outcome outcome)
{
    switch (outcome) {
    case Outcome::Victory: return "victory";
    case Outcome::Defeat: return "defeat";
    case Outcome::None: break;
    }
    fail("fixture emitter received a nonterminal outcome");
}

std::vector<std::optional<
    ninho::simulation::test::FrozenLegacyQuantizedLaunch>>
canonical_launches(const RouteSeed& seed)
{
    std::vector<std::optional<
        ninho::simulation::test::FrozenLegacyQuantizedLaunch>> launches(
            seed.route.shots.size());
    if (seed.route.world_id != "orbital") {
        return launches;
    }

    const auto seeds =
        ninho::simulation::test::canonical_orbital_legacy_aim_seeds(
            seed.route_id);
    require(seeds.size() == seed.route.shots.size(),
        "orbital route and canonical legacy seed counts differ");
    for (std::size_t index = 0U; index < seeds.size(); ++index) {
        const ninho::simulation::AimState aim =
            ninho::simulation::test::legacy_ring_aim(seeds[index]);
        const auto canonical =
            ninho::simulation::test::canonical_legacy_launch_units(
                aim.origin_m, aim.tangent_direction, aim.speed_m_s);
        require(canonical.ok(),
            "canonical legacy aim seed could not be quantized");
        launches[index] = canonical.value;
    }
    return launches;
}

FrozenPlaythroughFixture capture_fixture(const RouteSeed& seed)
{
    FrozenPlaythroughFixture fixture{
        .schema_version = 1U,
        .route_id = seed.route_id,
        .route = seed.route,
        .legacy_quantized_launches = canonical_launches(seed),
    };

    // A zeroed expected value intentionally makes the strict replayer stop at
    // its final comparison after it has captured every real terminal byte.
    const auto captured =
        ninho::simulation::test::replay_frozen_playthrough(fixture);
    require(!captured.ok(), "capture pass unexpectedly matched an empty golden");
    require(captured.error.pointer == "/expected/outcome",
        "capture pass failed before reaching its terminal comparison");
    fixture.expected = captured.value.actual;

    require(fixture.expected.outcome == seed.expected_outcome,
        "route terminal outcome diverged from its accepted result");
    require(fixture.expected.score == seed.expected_score,
        "route score diverged from its accepted result");
    require(fixture.expected.stars == seed.expected_stars,
        "route stars diverged from its accepted result");
    require(fixture.expected.birds_remaining == seed.expected_birds_remaining,
        "route remaining-bird count diverged from its accepted result");
    require(fixture.expected.objectives_complete
            == seed.expected_objectives_complete,
        "route objective state diverged from its accepted result");
    require(!fixture.expected.ordered_events_v3.empty(),
        "route emitted an empty ordered event stream");
    if (seed.expected_event_count) {
        constexpr std::size_t ordered_event_header_bytes = 29U;
        constexpr std::size_t ordered_event_record_bytes = 274U;
        require(fixture.expected.ordered_events_v3.size()
                == ordered_event_header_bytes
                    + *seed.expected_event_count * ordered_event_record_bytes,
            "route event count diverged from its accepted result");
    }
    require(!fixture.expected.canonical_state_v3.empty(),
        "route emitted an empty canonical state");
    require(!fixture.expected.canonical_playthrough_v5.empty(),
        "route emitted an empty canonical playthrough");

    const auto verified =
        ninho::simulation::test::replay_frozen_playthrough(fixture);
    require(verified.ok(), "captured fixture did not replay against its bytes");
    require(verified.value.actual == fixture.expected,
        "captured fixture replay changed its terminal evidence");
    return fixture;
}

nlohmann::json fixture_json(const FrozenPlaythroughFixture& fixture)
{
    using nlohmann::json;
    json shots = json::array();
    for (std::size_t index = 0U; index < fixture.route.shots.size(); ++index) {
        const GestureShotInput& shot = fixture.route.shots[index];
        const auto& legacy = fixture.legacy_quantized_launches[index];
        json encoded{
            {"camera_right", {shot.camera_right.x,
                shot.camera_right.y, shot.camera_right.z}},
            {"pull_horizontal_m", shot.pull_horizontal_m},
            {"pull_vertical_m", shot.pull_vertical_m},
        };
        if (legacy) {
            encoded["legacy_quantized_launch"] = {
                {"origin_mm", legacy->origin_mm},
                {"normalized_direction_1e4",
                    legacy->normalized_direction_1e4},
                {"speed_centimeters_per_second",
                    legacy->speed_centimeters_per_second},
            };
        }
        if (shot.ability_tick_after_launch) {
            encoded["ability_tick_after_launch"] =
                *shot.ability_tick_after_launch;
        }
        shots.push_back(std::move(encoded));
    }

    const auto& expected = fixture.expected;
    return {
        {"schema_version", fixture.schema_version},
        {"route_id", fixture.route_id},
        {"world_id", fixture.route.world_id},
        {"level_id", fixture.route.level_id},
        {"shots", std::move(shots)},
        {"expected", {
            {"outcome", outcome_text(expected.outcome)},
            {"score", expected.score},
            {"stars", expected.stars},
            {"birds_remaining", expected.birds_remaining},
            {"objectives_complete", expected.objectives_complete},
            {"ordered_events_v3_hex", hex_bytes(expected.ordered_events_v3)},
            {"canonical_state_v3_hex", hex_bytes(expected.canonical_state_v3)},
            {"canonical_state_v3_fnv1a64",
                hash_text(expected.canonical_state_v3_fnv1a64)},
            {"canonical_playthrough_v5_hex",
                hex_bytes(expected.canonical_playthrough_v5)},
            {"canonical_playthrough_v5_fnv1a64",
                hash_text(expected.canonical_playthrough_v5_fnv1a64)},
        }},
    };
}

FixtureDocument fixture_document(const FrozenPlaythroughFixture& fixture)
{
    const std::string document = fixture_json(fixture).dump(2) + '\n';
    const auto parsed =
        ninho::simulation::test::parse_frozen_playthrough_fixture_envelope(
            document);
    require(parsed.ok(), "emitted document failed strict fixture parsing");
    require(parsed.value == fixture,
        "emitted document did not preserve the captured fixture");
    const auto replayed =
        ninho::simulation::test::replay_frozen_playthrough(parsed.value);
    require(replayed.ok(), "emitted document failed strict fixture replay");
    return {
        .filename = fixture.route_id + ".playthrough.json",
        .bytes = document,
    };
}

FixtureDocument layout_fixture_document()
{
    const std::filesystem::path source = std::filesystem::path{NINHO_SOURCE_DIR}
        / "game/data/levels/earth/farm_reaction.level.json";
    std::ifstream input{source, std::ios::binary};
    require(input.is_open(), "could not open farm level for layout capture");
    std::ostringstream source_document;
    source_document << input.rdbuf();
    const auto level = ninho::simulation::parse_level_manifest_v2(
        source_document.str());
    require(level.ok(), "farm level failed typed parsing for layout capture");

    const std::string document =
        ninho::simulation::test::product_v2_layout_fixture_document(level.value);
    return {
        .filename = "farm_reaction_layout_v1.json",
        .bytes = document,
    };
}

}

int main(int argc, char** argv)
{
    try {
        bool layout_only = false;
        bool force = false;
        std::optional<std::filesystem::path> output_directory;
        for (int index = 1; index < argc; ++index) {
            const std::string_view argument{argv[index]};
            if (argument == "--layout-only") {
                require(!layout_only, "--layout-only was specified twice");
                layout_only = true;
            } else if (argument == "--force") {
                require(!force, "--force was specified twice");
                force = true;
            } else {
                require(!output_directory.has_value(),
                    "fixture emitter accepts exactly one output directory");
                output_directory = std::filesystem::path{argv[index]};
            }
        }
        if (!output_directory) {
            std::cerr << "usage: ninho_product_v2_fixture_emitter "
                         "[--force] [--layout-only] "
                         "<absolute-output-directory>\n";
            return 2;
        }
        require(output_directory->is_absolute(),
            "fixture output directory must be an explicit absolute path");
        require(std::filesystem::is_directory(*output_directory),
            "fixture output directory does not exist");

        std::vector<FixtureDocument> documents;
        if (layout_only) {
            documents.push_back(layout_fixture_document());
        } else {
            const std::vector<RouteSeed> seeds = route_seeds();
            std::vector<FrozenPlaythroughFixture> fixtures;
            fixtures.reserve(seeds.size());
            // Capture the complete batch before serializing or touching the
            // destination. A late route failure therefore cannot partially
            // update an accepted golden set.
            for (const RouteSeed& seed : seeds) {
                fixtures.push_back(capture_fixture(seed));
            }
            documents.reserve(fixtures.size());
            for (const FrozenPlaythroughFixture& fixture : fixtures) {
                documents.push_back(fixture_document(fixture));
            }
        }
        ninho::simulation::test::publish_fixture_documents(
            *output_directory, documents, {.force = force});
        for (const FixtureDocument& document : documents) {
            std::cout << (*output_directory / document.filename).string()
                      << '\n';
        }
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
