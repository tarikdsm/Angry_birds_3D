#include "test_framework.hpp"
#include "product_v2_playthrough_fixture.hpp"

#include <filesystem>
#include <fstream>
#include <memory>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

namespace {

using ninho::simulation::Outcome;

struct OrbitalRouteExpectation {
    std::string_view route_id;
    Outcome outcome;
    std::uint64_t score;
    std::uint32_t stars;
    std::uint32_t birds_remaining;
    bool objectives_complete;
};

std::filesystem::path fixture_path(std::string_view route_id)
{
    return std::filesystem::path{NINHO_SOURCE_DIR}
        / "native/simulation/tests/fixtures/product_v2"
        / (std::string{route_id} + ".playthrough.json");
}

std::string read_fixture(std::string_view route_id)
{
    std::ifstream input{fixture_path(route_id), std::ios::binary};
    NINHO_SIM_REQUIRE(input.is_open());
    std::ostringstream document;
    document << input.rdbuf();
    return document.str();
}

std::string read_source_file(std::string_view relative_path)
{
    std::ifstream input{std::filesystem::path{NINHO_SOURCE_DIR} / relative_path,
        std::ios::binary};
    NINHO_SIM_REQUIRE(input.is_open());
    std::ostringstream document;
    document << input.rdbuf();
    return document.str();
}

std::unique_ptr<ninho::simulation::SimulationSession> create_orbital_session()
{
    using namespace ninho::simulation;
    const auto materials = parse_material_catalog(read_source_file(
        "game/data/materials/product_v2.materials.json"));
    const auto archetypes = parse_archetype_catalog(read_source_file(
        "game/data/archetypes/product_v2.archetypes.json"));
    const auto level = parse_level_manifest(read_source_file(
        "game/data/levels/orbital/first_orbit_v2.level.json"));
    NINHO_SIM_REQUIRE(materials.ok());
    NINHO_SIM_REQUIRE(archetypes.ok());
    NINHO_SIM_REQUIRE(level.ok());
    auto created = SimulationSession::create(
        materials.value, archetypes.value, level.value);
    NINHO_SIM_REQUIRE(created.ok());
    return std::move(created.value);
}

std::unique_ptr<ninho::simulation::SimulationSession> create_legacy_session()
{
    using namespace ninho::simulation;
    const auto materials = parse_material_catalog(read_source_file(
        "game/data/materials/vertical_slice.materials.json"));
    const auto archetypes = parse_archetype_catalog(read_source_file(
        "game/data/archetypes/vertical_slice.archetypes.json"));
    const auto level = parse_level_manifest(read_source_file(
        "game/data/levels/first_orbit.level.json"));
    NINHO_SIM_REQUIRE(materials.ok());
    NINHO_SIM_REQUIRE(archetypes.ok());
    NINHO_SIM_REQUIRE(level.ok());
    auto created = SimulationSession::create(
        materials.value, archetypes.value, level.value);
    NINHO_SIM_REQUIRE(created.ok());
    return std::move(created.value);
}

std::vector<OrbitalRouteExpectation> orbital_expectations()
{
    return {
        {
            .route_id = "orbital_virela",
            .outcome = Outcome::Victory,
            .score = 15000U,
            .stars = 2U,
            .birds_remaining = 1U,
            .objectives_complete = true,
        },
        {
            .route_id = "orbital_structural",
            .outcome = Outcome::Victory,
            .score = 15660U,
            .stars = 2U,
            .birds_remaining = 1U,
            .objectives_complete = true,
        },
        {
            .route_id = "orbital_defeat",
            .outcome = Outcome::Defeat,
            .score = 0U,
            .stars = 0U,
            .birds_remaining = 0U,
            .objectives_complete = false,
        },
    };
}

NINHO_SIM_TEST("product v2 determinism orbital fixtures freeze exact routes and replay bytes")
{
    using namespace ninho::simulation;
    for (const OrbitalRouteExpectation& expected : orbital_expectations()) {
        const auto fixture = test::parse_frozen_playthrough_fixture_envelope(
            read_fixture(expected.route_id));
        NINHO_SIM_REQUIRE(fixture.ok());
        NINHO_SIM_REQUIRE(fixture.value.schema_version == 1U);
        NINHO_SIM_REQUIRE(fixture.value.route_id == expected.route_id);
        NINHO_SIM_REQUIRE(fixture.value.route.world_id == "orbital");
        NINHO_SIM_REQUIRE(fixture.value.route.level_id == "first_orbit_v2");
        const auto legacy_seeds = test::canonical_orbital_legacy_aim_seeds(
            expected.route_id);
        NINHO_SIM_REQUIRE(fixture.value.route.shots.size()
            == legacy_seeds.size());
        NINHO_SIM_REQUIRE(fixture.value.legacy_quantized_launches.size()
            == legacy_seeds.size());
        NINHO_SIM_REQUIRE(fixture.value.expected.outcome == expected.outcome);
        NINHO_SIM_REQUIRE(fixture.value.expected.score == expected.score);
        NINHO_SIM_REQUIRE(fixture.value.expected.stars == expected.stars);
        NINHO_SIM_REQUIRE(fixture.value.expected.birds_remaining
            == expected.birds_remaining);
        NINHO_SIM_REQUIRE(fixture.value.expected.objectives_complete
            == expected.objectives_complete);
        NINHO_SIM_REQUIRE(!fixture.value.expected.ordered_events_v3.empty());
        if (expected.route_id == "orbital_virela") {
            constexpr std::size_t ordered_event_header_bytes = 29U;
            constexpr std::size_t ordered_event_record_bytes = 274U;
            constexpr std::size_t virela_event_count = 98U;
            NINHO_SIM_REQUIRE(fixture.value.expected.ordered_events_v3.size()
                == ordered_event_header_bytes
                    + virela_event_count * ordered_event_record_bytes);
        }
        NINHO_SIM_REQUIRE(!fixture.value.expected.canonical_state_v3.empty());
        NINHO_SIM_REQUIRE(
            !fixture.value.expected.canonical_playthrough_v5.empty());

        const auto replayed = test::replay_frozen_playthrough(fixture.value);
        NINHO_SIM_REQUIRE(replayed.ok());
        NINHO_SIM_REQUIRE(replayed.value.actual == fixture.value.expected);
        NINHO_SIM_REQUIRE(replayed.value.shots.size()
            == fixture.value.route.shots.size());
        for (std::size_t index = 0U; index < replayed.value.shots.size(); ++index) {
            NINHO_SIM_REQUIRE(replayed.value.shots[index].quantized_launch
                == fixture.value.legacy_quantized_launches[index]);
        }
    }
}

NINHO_SIM_TEST("product v2 determinism exposes named legacy aim seeds instead of frozen tuples")
{
    using namespace ninho::simulation;

    const auto virela = test::canonical_orbital_legacy_aim_seeds(
        "orbital_virela");
    NINHO_SIM_REQUIRE(virela.size() == 2U);
    NINHO_SIM_REQUIRE((virela[0] == test::LegacyAimSeed{0.0, -0.1, 8.0}));
    NINHO_SIM_REQUIRE((virela[1] == test::LegacyAimSeed{0.0, 8.0, 8.18}));

    const AimState first = test::legacy_ring_aim(virela[0]);
    const ninho::physics::Vec3 expected_origin{-13.0F, 0.0F, 0.0F};
    NINHO_SIM_REQUIRE(first.origin_m == expected_origin);
    NINHO_SIM_REQUIRE(first.speed_m_s == 8.0);
}

NINHO_SIM_TEST("product v2 determinism gesture aim equals the canonical legacy API aim")
{
    using namespace ninho::simulation;

    for (const OrbitalRouteExpectation& expected : orbital_expectations()) {
        const auto fixture = test::parse_frozen_playthrough_fixture_envelope(
            read_fixture(expected.route_id));
        NINHO_SIM_REQUIRE(fixture.ok());
        const auto seeds = test::canonical_orbital_legacy_aim_seeds(
            expected.route_id);
        NINHO_SIM_REQUIRE(seeds.size() == fixture.value.route.shots.size());

        for (std::size_t index = 0U; index < seeds.size(); ++index) {
            auto legacy_session = create_legacy_session();
            auto product_v2_session = create_orbital_session();
            const auto canonical_legacy = legacy_session->quantize_aim(
                test::legacy_ring_aim(seeds[index]));
            NINHO_SIM_REQUIRE(canonical_legacy.ok());

            const GestureShotInput& gesture = fixture.value.route.shots[index];
            NINHO_SIM_REQUIRE(product_v2_session->enqueue(
                BeginGrabCommand{gesture.camera_right}).ok());
            NINHO_SIM_REQUIRE(product_v2_session->enqueue(SetPullCommand{
                gesture.pull_horizontal_m, gesture.pull_vertical_m}).ok());
            NINHO_SIM_REQUIRE(product_v2_session->tick().ok());
            NINHO_SIM_REQUIRE(product_v2_session->state().phase
                == SessionPhase::Grabbed);
            NINHO_SIM_REQUIRE(product_v2_session->state().launcher.has_value());
            const LauncherState& launcher =
                *product_v2_session->state().launcher;
            const auto canonical_gesture = legacy_session->quantize_aim(AimState{
                launcher.rest_position_m,
                launcher.launch_direction,
                launcher.predicted_speed_m_s});
            NINHO_SIM_REQUIRE(canonical_gesture.ok());
            NINHO_SIM_REQUIRE(canonical_gesture.value
                == canonical_legacy.value);

            const auto launch_units = test::canonical_legacy_launch_units(
                canonical_legacy.value.origin_m,
                canonical_legacy.value.tangent_direction,
                canonical_legacy.value.speed_m_s);
            NINHO_SIM_REQUIRE(launch_units.ok());
            NINHO_SIM_REQUIRE(fixture.value.legacy_quantized_launches[index]
                == launch_units.value);
        }

        // Route semantics are exercised by the adjacent frozen replay test,
        // only through the schema-v2 gesture FSM. SetAim/Launch remain
        // characterized separately on schema v1.
    }
}

}
