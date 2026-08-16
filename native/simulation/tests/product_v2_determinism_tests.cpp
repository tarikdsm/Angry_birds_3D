#include "test_framework.hpp"
#include "product_v2_playthrough_fixture.hpp"
#include "ninho/simulation/playthrough.hpp"

#include <nlohmann/json.hpp>

#include <array>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <limits>
#include <optional>
#include <sstream>
#include <span>
#include <string>
#include <vector>

namespace {

constexpr std::array<std::string_view, 7> frozen_route_names{
    "farm_tutorial",
    "farm_chain",
    "farm_blue_alternative",
    "farm_defeat",
    "orbital_virela",
    "orbital_structural",
    "orbital_defeat",
};

std::filesystem::path frozen_fixture_path(std::string_view route_name)
{
    return std::filesystem::path{NINHO_SOURCE_DIR}
        / "native/simulation/tests/fixtures/product_v2"
        / (std::string{route_name} + ".playthrough.json");
}

std::string hex_bytes(std::span<const std::uint8_t> bytes)
{
    constexpr char digits[] = "0123456789abcdef";
    std::string result;
    result.reserve(bytes.size() * 2U);
    for (const std::uint8_t byte : bytes) {
        result.push_back(digits[byte >> 4U]);
        result.push_back(digits[byte & 0x0fU]);
    }
    return result;
}

std::string hash_text(std::uint64_t value)
{
    constexpr char digits[] = "0123456789abcdef";
    std::string result{"fnv1a64:"};
    for (int shift = 60; shift >= 0; shift -= 4) {
        result.push_back(digits[(value >> shift) & 0x0fU]);
    }
    return result;
}

std::vector<std::uint8_t> versioned_blob_header(
    std::string_view name, std::uint32_t version)
{
    std::vector<std::uint8_t> result;
    const auto append_u32 = [&](std::uint32_t value) {
        for (unsigned shift = 0U; shift < 32U; shift += 8U) {
            result.push_back(static_cast<std::uint8_t>(value >> shift));
        }
    };
    append_u32(static_cast<std::uint32_t>(name.size()));
    result.insert(result.end(), name.begin(), name.end());
    append_u32(version);
    return result;
}

std::string valid_frozen_fixture_json(bool orbital = false)
{
    using namespace ninho::simulation;
    using nlohmann::json;
    const auto ordered = ordered_events_v3(std::span<const DomainEvent>{});
    const auto state = versioned_blob_header("canonical_state_v3", 3U);
    PlaythroughRouteDefinition route{
        orbital ? "orbital" : "earth",
        orbital ? "first_orbit_v2" : "farm_reaction",
        {{{1.0F, 0.0F, 0.0F}, -4.0, -0.5, std::uint64_t{18U}}},
    };
    const auto playthrough = canonical_playthrough_v5(route, ordered, state);
    json shot{
        {"camera_right", {1.0, 0.0, 0.0}},
        {"pull_horizontal_m", -4.0},
        {"pull_vertical_m", -0.5},
        {"ability_tick_after_launch", 18U},
    };
    if (orbital) {
        shot["legacy_quantized_launch"] = {
            {"origin_mm", {-13000, 0, 0}},
            {"normalized_direction_1e4", {0, 10000, 0}},
            {"speed_centimeters_per_second", 800},
        };
    }
    return json{
        {"schema_version", 1U},
        {"route_id", orbital ? "orbital_virela" : "farm_tutorial"},
        {"world_id", route.world_id},
        {"level_id", route.level_id},
        {"shots", json::array({std::move(shot)})},
        {"expected", {
            {"outcome", "victory"},
            {"score", 1234U},
            {"stars", 1U},
            {"birds_remaining", 2U},
            {"objectives_complete", true},
            {"ordered_events_v3_hex", hex_bytes(ordered)},
            {"canonical_state_v3_hex", hex_bytes(state)},
            {"canonical_state_v3_fnv1a64", hash_text(fnv1a64(state))},
            {"canonical_playthrough_v5_hex", hex_bytes(playthrough)},
            {"canonical_playthrough_v5_fnv1a64", hash_text(fnv1a64(playthrough))},
        }},
    }.dump();
}

nlohmann::json valid_frozen_fixture(bool orbital = false)
{
    return nlohmann::json::parse(valid_frozen_fixture_json(orbital));
}

ninho::simulation::test::FrozenPlaythroughFixture executable_fixture(
    std::string world_id, std::string level_id,
    std::vector<ninho::simulation::GestureShotInput> shots)
{
    ninho::simulation::test::FrozenPlaythroughFixture fixture;
    fixture.schema_version = 1U;
    fixture.route_id = "strict_replayer_contract";
    fixture.route.world_id = std::move(world_id);
    fixture.route.level_id = std::move(level_id);
    fixture.route.shots = std::move(shots);
    fixture.legacy_quantized_launches.resize(fixture.route.shots.size());
    if (fixture.route.world_id == "orbital") {
        for (auto& legacy : fixture.legacy_quantized_launches) {
            legacy = ninho::simulation::test::FrozenLegacyQuantizedLaunch{
                .origin_mm = {-13000, 0, 0},
                .normalized_direction_1e4 = {-10000, 0, 0},
                .speed_centimeters_per_second = 1504U,
            };
        }
    }
    return fixture;
}

void require_fixture_rejected(std::string_view document,
    ninho::simulation::ContentErrorCode code, std::string_view pointer)
{
    const auto parsed = ninho::simulation::test::parse_frozen_playthrough_fixture_envelope(
        document);
    NINHO_SIM_REQUIRE(!parsed.ok());
    NINHO_SIM_REQUIRE(parsed.error.code == code);
    NINHO_SIM_REQUIRE(parsed.error.pointer == pointer);
    NINHO_SIM_REQUIRE(parsed.value.route_id.empty());
    NINHO_SIM_REQUIRE(parsed.value.route.shots.empty());
    NINHO_SIM_REQUIRE(parsed.value.expected.ordered_events_v3.empty());
}

void set_expected_blob(nlohmann::json& fixture, std::string_view hex_key,
    std::string_view hash_key, std::span<const std::uint8_t> bytes)
{
    fixture["expected"][std::string{hex_key}] = hex_bytes(bytes);
    fixture["expected"][std::string{hash_key}] =
        hash_text(ninho::simulation::fnv1a64(bytes));
}

NINHO_SIM_TEST("product v2 determinism fixture parser rejects malformed JSON atomically")
{
    require_fixture_rejected("{\"schema_version\":1",
        ninho::simulation::ContentErrorCode::InvalidJson, "");
}

NINHO_SIM_TEST("product v2 determinism fixture envelope is strict after the JSON value")
{
    using namespace ninho::simulation;
    const std::string valid = valid_frozen_fixture_json();

    const auto whitespace = test::parse_frozen_playthrough_fixture_envelope(
        valid + " \r\n\t");
    NINHO_SIM_REQUIRE(whitespace.ok());

    const auto trailing = test::parse_frozen_playthrough_fixture_envelope(
        valid + " true");
    NINHO_SIM_REQUIRE(!trailing.ok());
    NINHO_SIM_REQUIRE(trailing.error.code == ContentErrorCode::InvalidJson);
    NINHO_SIM_REQUIRE(trailing.error.pointer.empty());
}

NINHO_SIM_TEST("product v2 determinism fixture envelope defers canonical state semantics to replay")
{
    using namespace ninho::simulation;
    // The fixture helper intentionally creates a canonical_state_v3 blob that
    // contains only its syntactically valid versioned header.
    const auto parsed = test::parse_frozen_playthrough_fixture_envelope(
        valid_frozen_fixture_json());
    NINHO_SIM_REQUIRE(parsed.ok());
    NINHO_SIM_REQUIRE(parsed.value.expected.canonical_state_v3
        == versioned_blob_header("canonical_state_v3", 3U));

    const auto replayed = test::replay_frozen_playthrough(parsed.value);
    NINHO_SIM_REQUIRE(!replayed.ok());
    NINHO_SIM_REQUIRE(replayed.error.code == ContentErrorCode::InvalidInvariant);
    NINHO_SIM_REQUIRE(replayed.error.pointer == "/expected/outcome");
}

NINHO_SIM_TEST("product v2 determinism fixture parser rejects duplicate keys")
{
    std::string document = valid_frozen_fixture_json();
    document.insert(1U, "\"schema_version\":1,");
    require_fixture_rejected(document,
        ninho::simulation::ContentErrorCode::DuplicateKey, "/schema_version");
}

NINHO_SIM_TEST("product v2 determinism fixture parser rejects unknown keys")
{
    auto document = valid_frozen_fixture();
    document["expected"]["surprise"] = true;
    require_fixture_rejected(document.dump(),
        ninho::simulation::ContentErrorCode::UnknownKey, "/expected/surprise");
}

NINHO_SIM_TEST("product v2 determinism fixture parser rejects missing fields")
{
    auto document = valid_frozen_fixture();
    document.erase("route_id");
    require_fixture_rejected(document.dump(),
        ninho::simulation::ContentErrorCode::MissingField, "/route_id");
}

NINHO_SIM_TEST("product v2 determinism fixture parser rejects empty required values")
{
    auto empty_id = valid_frozen_fixture();
    empty_id["route_id"] = "";
    require_fixture_rejected(empty_id.dump(),
        ninho::simulation::ContentErrorCode::OutOfRange, "/route_id");

    auto empty_shots = valid_frozen_fixture();
    empty_shots["shots"] = nlohmann::json::array();
    require_fixture_rejected(empty_shots.dump(),
        ninho::simulation::ContentErrorCode::OutOfRange, "/shots");

    auto empty_blob = valid_frozen_fixture();
    empty_blob["expected"]["ordered_events_v3_hex"] = "";
    require_fixture_rejected(empty_blob.dump(),
        ninho::simulation::ContentErrorCode::OutOfRange,
        "/expected/ordered_events_v3_hex");
}

NINHO_SIM_TEST("product v2 determinism fixture parser rejects nonrepresentable numbers")
{
    std::string document = valid_frozen_fixture_json();
    const std::string needle = "\"pull_horizontal_m\":-4.0";
    const std::size_t position = document.find(needle);
    NINHO_SIM_REQUIRE(position != std::string::npos);
    document.replace(position, needle.size(), "\"pull_horizontal_m\":1e999");
    require_fixture_rejected(document,
        ninho::simulation::ContentErrorCode::InvalidNumber, "");

    auto runtime_overflow = valid_frozen_fixture();
    runtime_overflow["shots"][0]["camera_right"][0] = 1.0e100;
    require_fixture_rejected(runtime_overflow.dump(),
        ninho::simulation::ContentErrorCode::OutOfRange,
        "/shots/0/camera_right/0");
}

NINHO_SIM_TEST("product v2 determinism fixture parser enforces resource limits")
{
    std::string oversized(
        ninho::simulation::test::frozen_playthrough_fixture_max_bytes + 1U, ' ');
    require_fixture_rejected(oversized,
        ninho::simulation::ContentErrorCode::ResourceLimit, "");

    auto too_many_shots = valid_frozen_fixture();
    const auto shot = too_many_shots["shots"].front();
    while (too_many_shots["shots"].size()
        <= ninho::simulation::test::frozen_playthrough_fixture_max_shots) {
        too_many_shots["shots"].push_back(shot);
    }
    require_fixture_rejected(too_many_shots.dump(),
        ninho::simulation::ContentErrorCode::ResourceLimit, "/shots");
}

NINHO_SIM_TEST("product v2 determinism fixture parser distinguishes omitted ability from null")
{
    using namespace ninho::simulation;
    auto omitted = valid_frozen_fixture();
    omitted["shots"][0].erase("ability_tick_after_launch");
    const auto ordered = ordered_events_v3(std::span<const DomainEvent>{});
    const auto state = versioned_blob_header("canonical_state_v3", 3U);
    const PlaythroughRouteDefinition route{
        "earth", "farm_reaction",
        {{{1.0F, 0.0F, 0.0F}, -4.0, -0.5, std::nullopt}},
    };
    const auto playthrough = canonical_playthrough_v5(route, ordered, state);
    set_expected_blob(omitted, "canonical_playthrough_v5_hex",
        "canonical_playthrough_v5_fnv1a64", playthrough);
    const auto parsed = test::parse_frozen_playthrough_fixture_envelope(
        omitted.dump());
    NINHO_SIM_REQUIRE(parsed.ok());
    NINHO_SIM_REQUIRE(!parsed.value.route.shots.front().ability_tick_after_launch);

    auto explicit_null = valid_frozen_fixture();
    explicit_null["shots"][0]["ability_tick_after_launch"] = nullptr;
    require_fixture_rejected(explicit_null.dump(),
        ninho::simulation::ContentErrorCode::InvalidType,
        "/shots/0/ability_tick_after_launch");
}

NINHO_SIM_TEST("product v2 determinism fixture parser requires legacy launch only for orbital")
{
    auto farm_with_legacy = valid_frozen_fixture();
    farm_with_legacy["shots"][0]["legacy_quantized_launch"] = {
        {"origin_mm", {-13000, 0, 0}},
        {"normalized_direction_1e4", {0, 10000, 0}},
        {"speed_centimeters_per_second", 800},
    };
    require_fixture_rejected(farm_with_legacy.dump(),
        ninho::simulation::ContentErrorCode::UnknownKey,
        "/shots/0/legacy_quantized_launch");

    auto orbital_without_legacy = valid_frozen_fixture(true);
    orbital_without_legacy["shots"][0].erase("legacy_quantized_launch");
    require_fixture_rejected(orbital_without_legacy.dump(),
        ninho::simulation::ContentErrorCode::MissingField,
        "/shots/0/legacy_quantized_launch");

    const auto orbital = ninho::simulation::test::parse_frozen_playthrough_fixture_envelope(
        valid_frozen_fixture_json(true));
    NINHO_SIM_REQUIRE(orbital.ok());
    NINHO_SIM_REQUIRE(orbital.value.legacy_quantized_launches.front().has_value());
    NINHO_SIM_REQUIRE((orbital.value.legacy_quantized_launches.front()->origin_mm
        == std::array<std::int64_t, 3>{-13000, 0, 0}));
    NINHO_SIM_REQUIRE((orbital.value.legacy_quantized_launches.front()
        ->normalized_direction_1e4
        == std::array<std::int64_t, 3>{0, 10000, 0}));
    NINHO_SIM_REQUIRE(orbital.value.legacy_quantized_launches.front()
        ->speed_centimeters_per_second == 800U);
}

NINHO_SIM_TEST("product v2 determinism fixture parser requires integer legacy launch units")
{
    auto old_float_schema = valid_frozen_fixture(true);
    old_float_schema["shots"][0]["legacy_quantized_launch"] = {
        {"origin_m", {-13.0, 0.0, 0.0}},
        {"tangent_direction", {0.0, 1.0, 0.0}},
        {"speed_m_s", 8.0},
    };
    require_fixture_rejected(old_float_schema.dump(),
        ninho::simulation::ContentErrorCode::UnknownKey,
        "/shots/0/legacy_quantized_launch/origin_m");

    auto fractional = valid_frozen_fixture(true);
    fractional["shots"][0]["legacy_quantized_launch"]["origin_mm"][0]
        = -13000.5;
    require_fixture_rejected(fractional.dump(),
        ninho::simulation::ContentErrorCode::InvalidType,
        "/shots/0/legacy_quantized_launch/origin_mm/0");

    auto zero_direction = valid_frozen_fixture(true);
    zero_direction["shots"][0]["legacy_quantized_launch"]
        ["normalized_direction_1e4"] = {0, 0, 0};
    require_fixture_rejected(zero_direction.dump(),
        ninho::simulation::ContentErrorCode::InvalidInvariant,
        "/shots/0/legacy_quantized_launch/normalized_direction_1e4");

    auto component_overflow = valid_frozen_fixture(true);
    component_overflow["shots"][0]["legacy_quantized_launch"]
        ["normalized_direction_1e4"][1] = 10001;
    require_fixture_rejected(component_overflow.dump(),
        ninho::simulation::ContentErrorCode::OutOfRange,
        "/shots/0/legacy_quantized_launch/normalized_direction_1e4/1");
}

NINHO_SIM_TEST("product v2 determinism canonical legacy units converge across v1 and v2 quantization")
{
    using namespace ninho::simulation;
    const auto legacy = test::canonical_legacy_launch_units(
        {-13.000001F, 0.0F, 0.0F},
        {0.0F, 0.990264952F, 0.13919507F},
        8.1799999999999997);
    const auto product_v2 = test::canonical_legacy_launch_units(
        {-13.0F, 0.0F, 0.0F},
        {0.0F, 0.990267873F, 0.139174521F},
        8.18);
    const auto outside_origin_quantum = test::canonical_legacy_launch_units(
        {-13.001F, 0.0F, 0.0F},
        {0.0F, 0.990267873F, 0.139174521F},
        8.18);
    const auto positive_half_speed_unit =
        test::canonical_legacy_launch_units(
            {}, {0.0F, 1.0F, 0.0F}, 0.005);

    NINHO_SIM_REQUIRE(legacy.ok() && product_v2.ok()
        && outside_origin_quantum.ok() && positive_half_speed_unit.ok());
    const test::FrozenLegacyQuantizedLaunch expected{
        .origin_mm = {-13000, 0, 0},
        .normalized_direction_1e4 = {0, 9903, 1392},
        .speed_centimeters_per_second = 818U,
    };
    NINHO_SIM_REQUIRE(legacy.value == expected);
    NINHO_SIM_REQUIRE(product_v2.value == expected);
    NINHO_SIM_REQUIRE(outside_origin_quantum.value != expected);
    NINHO_SIM_REQUIRE(outside_origin_quantum.value.origin_mm[0] == -13001);
    NINHO_SIM_REQUIRE(positive_half_speed_unit.value
        .speed_centimeters_per_second == 1U);
}

NINHO_SIM_TEST("product v2 determinism canonical legacy units fail closed on zero nonfinite and overflow")
{
    using namespace ninho::simulation;
    const auto zero_direction = test::canonical_legacy_launch_units(
        {-13.0F, 0.0F, 0.0F}, {}, 8.0);
    NINHO_SIM_REQUIRE(!zero_direction.ok());
    NINHO_SIM_REQUIRE(zero_direction.error.code
        == ContentErrorCode::InvalidInvariant);

    const auto nonfinite = test::canonical_legacy_launch_units(
        {std::numeric_limits<float>::quiet_NaN(), 0.0F, 0.0F},
        {0.0F, 1.0F, 0.0F}, 8.0);
    NINHO_SIM_REQUIRE(!nonfinite.ok());
    NINHO_SIM_REQUIRE(nonfinite.error.code == ContentErrorCode::InvalidNumber);

    const auto origin_overflow = test::canonical_legacy_launch_units(
        {std::numeric_limits<float>::max(), 0.0F, 0.0F},
        {0.0F, 1.0F, 0.0F}, 8.0);
    NINHO_SIM_REQUIRE(!origin_overflow.ok());
    NINHO_SIM_REQUIRE(origin_overflow.error.code == ContentErrorCode::OutOfRange);

    const auto speed_overflow = test::canonical_legacy_launch_units(
        {-13.0F, 0.0F, 0.0F}, {0.0F, 1.0F, 0.0F},
        std::numeric_limits<double>::max());
    NINHO_SIM_REQUIRE(!speed_overflow.ok());
    NINHO_SIM_REQUIRE(speed_overflow.error.code == ContentErrorCode::OutOfRange);

    const auto negative_speed = test::canonical_legacy_launch_units(
        {-13.0F, 0.0F, 0.0F}, {0.0F, 1.0F, 0.0F}, -0.01);
    NINHO_SIM_REQUIRE(!negative_speed.ok());
    NINHO_SIM_REQUIRE(negative_speed.error.code == ContentErrorCode::OutOfRange);
}

NINHO_SIM_TEST("product v2 determinism fixture parser rejects noncanonical hex")
{
    for (const std::string_view malformed : {
             std::string_view{"0"}, std::string_view{"gg"},
             std::string_view{"AA"}}) {
        auto document = valid_frozen_fixture();
        document["expected"]["ordered_events_v3_hex"] = malformed;
        require_fixture_rejected(document.dump(),
            ninho::simulation::ContentErrorCode::InvalidInvariant,
            "/expected/ordered_events_v3_hex");
    }
}

NINHO_SIM_TEST("product v2 determinism fixture parser rejects malformed hash text")
{
    for (const std::string_view malformed : {
             std::string_view{"0000000000000000"},
             std::string_view{"fnv1a64:000000000000000"},
             std::string_view{"fnv1a64:000000000000000G"}}) {
        auto document = valid_frozen_fixture();
        document["expected"]["canonical_state_v3_fnv1a64"] = malformed;
        require_fixture_rejected(document.dump(),
            ninho::simulation::ContentErrorCode::InvalidInvariant,
            "/expected/canonical_state_v3_fnv1a64");
    }
}

NINHO_SIM_TEST("product v2 determinism fixture parser verifies both frozen hashes")
{
    auto state_mismatch = valid_frozen_fixture();
    state_mismatch["expected"]["canonical_state_v3_fnv1a64"] =
        "fnv1a64:0000000000000000";
    require_fixture_rejected(state_mismatch.dump(),
        ninho::simulation::ContentErrorCode::InvalidInvariant,
        "/expected/canonical_state_v3_fnv1a64");

    auto playthrough_mismatch = valid_frozen_fixture();
    playthrough_mismatch["expected"]["canonical_playthrough_v5_fnv1a64"] =
        "fnv1a64:0000000000000000";
    require_fixture_rejected(playthrough_mismatch.dump(),
        ninho::simulation::ContentErrorCode::InvalidInvariant,
        "/expected/canonical_playthrough_v5_fnv1a64");
}

NINHO_SIM_TEST("product v2 determinism fixture parser validates versioned blob headers")
{
    auto invalid_events = valid_frozen_fixture();
    auto events = ninho::simulation::ordered_events_v3(
        std::span<const ninho::simulation::DomainEvent>{});
    events[4U] = 'x';
    invalid_events["expected"]["ordered_events_v3_hex"] = hex_bytes(events);
    require_fixture_rejected(invalid_events.dump(),
        ninho::simulation::ContentErrorCode::InvalidInvariant,
        "/expected/ordered_events_v3_hex");

    auto invalid_state = valid_frozen_fixture();
    auto state = versioned_blob_header("canonical_state_v3", 2U);
    set_expected_blob(invalid_state, "canonical_state_v3_hex",
        "canonical_state_v3_fnv1a64", state);
    require_fixture_rejected(invalid_state.dump(),
        ninho::simulation::ContentErrorCode::InvalidInvariant,
        "/expected/canonical_state_v3_hex");

    auto invalid_playthrough = valid_frozen_fixture();
    auto playthrough = versioned_blob_header("canonical_playthrough_v5", 4U);
    set_expected_blob(invalid_playthrough, "canonical_playthrough_v5_hex",
        "canonical_playthrough_v5_fnv1a64", playthrough);
    require_fixture_rejected(invalid_playthrough.dump(),
        ninho::simulation::ContentErrorCode::InvalidInvariant,
        "/expected/canonical_playthrough_v5_hex");
}

NINHO_SIM_TEST("product v2 determinism fixture parser validates ordered event record framing")
{
    auto document = valid_frozen_fixture();
    auto events = ninho::simulation::ordered_events_v3(
        std::span<const ninho::simulation::DomainEvent>{});
    NINHO_SIM_REQUIRE(events.size() == 29U);
    events[25U] = 1U;
    document["expected"]["ordered_events_v3_hex"] = hex_bytes(events);
    require_fixture_rejected(document.dump(),
        ninho::simulation::ContentErrorCode::InvalidInvariant,
        "/expected/ordered_events_v3_hex");
}

NINHO_SIM_TEST("product v2 determinism fixture parser binds playthrough bytes to route and blobs")
{
    auto document = valid_frozen_fixture();
    document["shots"][0]["pull_vertical_m"] = -0.75;
    require_fixture_rejected(document.dump(),
        ninho::simulation::ContentErrorCode::InvalidInvariant,
        "/expected/canonical_playthrough_v5_hex");
}

NINHO_SIM_TEST("product v2 determinism fixture parser limits decoded blob size")
{
    auto document = valid_frozen_fixture();
    document["expected"]["ordered_events_v3_hex"] = std::string(
        ninho::simulation::test::frozen_playthrough_fixture_max_blob_bytes * 2U + 2U,
        '0');
    require_fixture_rejected(document.dump(),
        ninho::simulation::ContentErrorCode::ResourceLimit,
        "/expected/ordered_events_v3_hex");
}

NINHO_SIM_TEST("product v2 determinism fixture parser returns the complete frozen route")
{
    using namespace ninho::simulation;
    const auto parsed = test::parse_frozen_playthrough_fixture_envelope(
        valid_frozen_fixture_json());
    NINHO_SIM_REQUIRE(parsed.ok());
    NINHO_SIM_REQUIRE(parsed.value.schema_version == 1U);
    NINHO_SIM_REQUIRE(parsed.value.route_id == "farm_tutorial");
    NINHO_SIM_REQUIRE(parsed.value.route.world_id == "earth");
    NINHO_SIM_REQUIRE(parsed.value.route.level_id == "farm_reaction");
    NINHO_SIM_REQUIRE(parsed.value.route.shots.size() == 1U);
    NINHO_SIM_REQUIRE((parsed.value.route.shots.front().camera_right
        == ninho::physics::Vec3{1.0F, 0.0F, 0.0F}));
    NINHO_SIM_REQUIRE(parsed.value.route.shots.front().pull_horizontal_m == -4.0);
    NINHO_SIM_REQUIRE(parsed.value.route.shots.front().pull_vertical_m == -0.5);
    NINHO_SIM_REQUIRE(parsed.value.route.shots.front().ability_tick_after_launch == 18U);
    NINHO_SIM_REQUIRE(parsed.value.legacy_quantized_launches.size() == 1U);
    NINHO_SIM_REQUIRE(!parsed.value.legacy_quantized_launches.front());
    NINHO_SIM_REQUIRE(parsed.value.expected.outcome == Outcome::Victory);
    NINHO_SIM_REQUIRE(parsed.value.expected.score == 1234U);
    NINHO_SIM_REQUIRE(parsed.value.expected.stars == 1U);
    NINHO_SIM_REQUIRE(parsed.value.expected.birds_remaining == 2U);
    NINHO_SIM_REQUIRE(parsed.value.expected.objectives_complete);
    NINHO_SIM_REQUIRE(!parsed.value.expected.ordered_events_v3.empty());
    NINHO_SIM_REQUIRE(!parsed.value.expected.canonical_state_v3.empty());
    NINHO_SIM_REQUIRE(!parsed.value.expected.canonical_playthrough_v5.empty());
    NINHO_SIM_REQUIRE(parsed.value.expected.canonical_state_v3_fnv1a64
        == fnv1a64(parsed.value.expected.canonical_state_v3));
    NINHO_SIM_REQUIRE(parsed.value.expected.canonical_playthrough_v5_fnv1a64
        == fnv1a64(parsed.value.expected.canonical_playthrough_v5));
}

NINHO_SIM_TEST("product v2 determinism requires ordered events v3 and canonical playthrough v5")
{
    using namespace ninho::simulation;
    DomainEvent event;
    event.id = EventId{41U};
    event.tick = TickIndex{17U};
    event.kind = DomainEventKind::ScoreAwarded;
    event.entity_id = EntityId{100U};
    event.bird_archetype_id = BirdArchetypeId{2U};
    event.rejection_reason = CommandRejectionReason::AbilityUnavailable;
    event.ability_id = AbilityId{4U};
    event.affected_entity_id = EntityId{200U};
    event.affected_part_id = PartId{3U};
    event.weight = 1.25;
    event.force_n = {2.0F, 3.0F, 4.0F};
    event.impulse_n_s = {5.0F, 6.0F, 7.0F};
    event.part_id = PartId{8U};
    event.position_m = {9.0F, 10.0F, 11.0F};
    event.normal = {0.0F, 1.0F, 0.0F};
    event.energy_j = 12.0;
    event.damage = 13.0;
    event.damage_classification = DamageClassification::Vulnerable;
    event.neutralization_cause = NeutralizationCause::IntegrityDepleted;
    event.cause_event_id = EventId{40U};
    event.joint_id = JointId{14U};
    event.material_id = MaterialId{9U};
    event.joint_load_ratio = 1.5;
    event.fracture_ratio = 2.5;
    event.delta_velocity_m_s = {14.0F, 15.0F, 16.0F};
    event.environmental_trigger_id = 2U;
    event.scoring_identity_kind = ScoringIdentityKind::MaterialPiece;
    event.queue_slot_id = 3U;
    event.base_points = 120U;
    event.multiplier_percent = 130U;
    event.chain_index = 4U;
    event.awarded_points = 156U;
    event.total_score = 50000U;
    event.root_cause_event_id = EventId{1U};
    event.shot_id = 99U;
    event.stars = 3U;

    const std::array events{event};
    const std::vector<std::uint8_t> ordered = ordered_events_v3(events);
    NINHO_SIM_REQUIRE(ordered.size() == 29U + 274U);
    NINHO_SIM_REQUIRE(ordered == ordered_events_v3(events));
    DomainEvent changed = event;
    changed.shot_id += 1U;
    const std::array changed_events{changed};
    NINHO_SIM_REQUIRE(ordered != ordered_events_v3(changed_events));

    PlaythroughRouteDefinition route{
        "earth", "farm_reaction",
        {{{1.0F, 0.0F, 0.0F}, -4.0, -0.5, std::uint64_t{18U}}}};
    const std::array<std::uint8_t, 4> state{1U, 2U, 3U, 4U};
    const auto playthrough = canonical_playthrough_v5(route, ordered, state);
    NINHO_SIM_REQUIRE(playthrough == canonical_playthrough_v5(route, ordered, state));
    route.shots.front().pull_horizontal_m = -3.99999;
    NINHO_SIM_REQUIRE(playthrough != canonical_playthrough_v5(route, ordered, state));
    NINHO_SIM_REQUIRE(fnv1a64(playthrough) == fnv1a64(playthrough));
}

NINHO_SIM_TEST("product v2 determinism serializers reject non finite route input")
{
    using namespace ninho::simulation;
    PlaythroughRouteDefinition route{
        "earth", "farm_reaction",
        {{{1.0F, 0.0F, 0.0F}, std::numeric_limits<double>::infinity(),
            -0.5, std::nullopt}}};
    bool rejected = false;
    try {
        const std::array<std::uint8_t, 1> bytes{0U};
        (void)canonical_playthrough_v5(route, bytes, bytes);
    } catch (const std::invalid_argument&) {
        rejected = true;
    }
    NINHO_SIM_REQUIRE(rejected);

    DomainEvent event;
    event.weight = std::numeric_limits<double>::quiet_NaN();
    rejected = false;
    try {
        const std::array events{event};
        (void)ordered_events_v3(events);
    } catch (const std::invalid_argument&) {
        rejected = true;
    }
    NINHO_SIM_REQUIRE(rejected);
}

NINHO_SIM_TEST("product v2 determinism serializers preserve order and every extended payload")
{
    using namespace ninho::simulation;
    DomainEvent first;
    first.id = EventId{1U};
    first.tick = TickIndex{2U};
    first.kind = DomainEventKind::AbilityAffectedBody;
    first.weight = 1.0;
    first.force_n = {2.0F, 3.0F, 4.0F};
    first.impulse_n_s = {5.0F, 6.0F, 7.0F};
    first.position_m = {8.0F, 9.0F, 10.0F};
    first.normal = {0.0F, 1.0F, 0.0F};
    first.energy_j = 11.0;
    first.damage = 12.0;
    first.damage_classification = DamageClassification::Vulnerable;
    first.neutralization_cause = NeutralizationCause::IntegrityDepleted;
    first.cause_event_id = EventId{1U};
    first.joint_id = JointId{13U};
    first.material_id = MaterialId{9U};
    first.joint_load_ratio = 1.25;
    first.fracture_ratio = 1.5;
    first.delta_velocity_m_s = {13.0F, 14.0F, 15.0F};
    first.environmental_trigger_id = 2U;
    first.scoring_identity_kind = ScoringIdentityKind::MaterialPiece;
    first.queue_slot_id = 3U;
    first.base_points = 120U;
    first.multiplier_percent = 140U;
    first.chain_index = 5U;
    first.awarded_points = 168U;
    first.total_score = 50000U;
    first.root_cause_event_id = EventId{1U};
    first.shot_id = 7U;
    first.stars = 3U;
    DomainEvent second = first;
    second.id = EventId{2U};
    second.tick = TickIndex{3U};
    const std::array ordered_input{first, second};
    const std::array swapped_input{second, first};
    const auto baseline = ordered_events_v3(ordered_input);
    NINHO_SIM_REQUIRE(baseline != ordered_events_v3(swapped_input));

    const auto changed = [&](auto mutation) {
        DomainEvent value = first;
        mutation(value);
        const std::array input{value, second};
        NINHO_SIM_REQUIRE(baseline != ordered_events_v3(input));
    };
    changed([](DomainEvent& value) { value.id = EventId{2U}; });
    changed([](DomainEvent& value) { value.tick = TickIndex{3U}; });
    changed([](DomainEvent& value) { value.kind = DomainEventKind::PressureBurst; });
    changed([](DomainEvent& value) { value.entity_id = EntityId{3U}; });
    changed([](DomainEvent& value) {
        value.bird_archetype_id = BirdArchetypeId{2U};
    });
    changed([](DomainEvent& value) {
        value.rejection_reason = CommandRejectionReason::AbilityUnavailable;
    });
    changed([](DomainEvent& value) { value.ability_id = AbilityId{2U}; });
    changed([](DomainEvent& value) { value.affected_entity_id = EntityId{4U}; });
    changed([](DomainEvent& value) { value.affected_part_id = PartId{2U}; });
    changed([](DomainEvent& value) { value.weight += 0.00001; });
    changed([](DomainEvent& value) { value.force_n.x += 0.00001F; });
    changed([](DomainEvent& value) { value.impulse_n_s.y += 0.00001F; });
    changed([](DomainEvent& value) { value.part_id = PartId{2U}; });
    changed([](DomainEvent& value) { value.position_m.z += 0.00001F; });
    changed([](DomainEvent& value) { value.normal.x += 0.00001F; });
    changed([](DomainEvent& value) { value.energy_j += 0.00001; });
    changed([](DomainEvent& value) { value.damage += 0.00001; });
    changed([](DomainEvent& value) {
        value.damage_classification = DamageClassification::Protected;
    });
    changed([](DomainEvent& value) {
        value.neutralization_cause = NeutralizationCause::BoundsExit;
    });
    changed([](DomainEvent& value) { value.cause_event_id = EventId{2U}; });
    changed([](DomainEvent& value) { value.joint_id = JointId{14U}; });
    changed([](DomainEvent& value) { value.material_id = MaterialId{10U}; });
    changed([](DomainEvent& value) { value.joint_load_ratio += 0.00001; });
    changed([](DomainEvent& value) { value.fracture_ratio += 0.00001; });
    changed([](DomainEvent& value) { value.delta_velocity_m_s.z += 0.00001F; });
    changed([](DomainEvent& value) { value.environmental_trigger_id += 1U; });
    changed([](DomainEvent& value) {
        value.scoring_identity_kind = ScoringIdentityKind::EnemyEntity;
    });
    changed([](DomainEvent& value) { value.queue_slot_id += 1U; });
    changed([](DomainEvent& value) { value.base_points += 1U; });
    changed([](DomainEvent& value) { value.multiplier_percent += 1U; });
    changed([](DomainEvent& value) { value.chain_index += 1U; });
    changed([](DomainEvent& value) { value.awarded_points += 1U; });
    changed([](DomainEvent& value) { value.total_score += 1U; });
    changed([](DomainEvent& value) { value.root_cause_event_id = EventId{2U}; });
    changed([](DomainEvent& value) { value.shot_id += 1U; });
    changed([](DomainEvent& value) { value.stars = 2U; });
}

NINHO_SIM_TEST("product v2 determinism canonical playthrough normalizes inputs and embeds full blobs")
{
    using namespace ninho::simulation;
    const std::array<std::uint8_t, 2> events{1U, 2U};
    const std::array<std::uint8_t, 2> state{3U, 4U};
    PlaythroughRouteDefinition route{
        "earth", "farm_reaction",
        {{{1.0F, 0.0F, 0.0F}, -4.0, -0.5, std::uint64_t{18U}}}};
    const auto baseline = canonical_playthrough_v5(route, events, state);
    auto changed_route = route;
    changed_route.world_id = "orbital";
    NINHO_SIM_REQUIRE(baseline
        != canonical_playthrough_v5(changed_route, events, state));
    changed_route = route;
    changed_route.level_id = "first_orbit_v2";
    NINHO_SIM_REQUIRE(baseline
        != canonical_playthrough_v5(changed_route, events, state));
    auto equivalent = route;
    equivalent.shots.front().pull_horizontal_m = -4.000004;
    NINHO_SIM_REQUIRE(baseline == canonical_playthrough_v5(equivalent, events, state));
    equivalent.shots.front().pull_horizontal_m = -4.000006;
    NINHO_SIM_REQUIRE(baseline != canonical_playthrough_v5(equivalent, events, state));
    equivalent = route;
    equivalent.shots.front().camera_right.x += 0.00001F;
    NINHO_SIM_REQUIRE(baseline != canonical_playthrough_v5(equivalent, events, state));
    equivalent = route;
    equivalent.shots.front().camera_right.y += 0.00001F;
    NINHO_SIM_REQUIRE(baseline != canonical_playthrough_v5(equivalent, events, state));
    equivalent = route;
    equivalent.shots.front().camera_right.z += 0.00001F;
    NINHO_SIM_REQUIRE(baseline != canonical_playthrough_v5(equivalent, events, state));
    equivalent = route;
    equivalent.shots.front().pull_vertical_m += 0.00001;
    NINHO_SIM_REQUIRE(baseline != canonical_playthrough_v5(equivalent, events, state));
    equivalent = route;
    equivalent.shots.front().ability_tick_after_launch = 19U;
    NINHO_SIM_REQUIRE(baseline != canonical_playthrough_v5(equivalent, events, state));
    equivalent.shots.front().ability_tick_after_launch.reset();
    NINHO_SIM_REQUIRE(baseline != canonical_playthrough_v5(equivalent, events, state));
    equivalent = route;
    equivalent.shots.push_back(equivalent.shots.front());
    NINHO_SIM_REQUIRE(baseline != canonical_playthrough_v5(equivalent, events, state));
    auto changed_events = events;
    changed_events.back() += 1U;
    NINHO_SIM_REQUIRE(baseline != canonical_playthrough_v5(route, changed_events, state));
    auto changed_state = state;
    changed_state.back() += 1U;
    NINHO_SIM_REQUIRE(baseline != canonical_playthrough_v5(route, events, changed_state));
}

NINHO_SIM_TEST("product v2 determinism frozen replayer uses the frozen camera vector verbatim")
{
    using namespace ninho::simulation;
    auto baseline_fixture = executable_fixture("earth", "farm_reaction", {
        {{1.0F, 0.0F, 0.0F}, -4.0, -0.4, std::uint64_t{0U}},
    });
    auto perturbed_fixture = baseline_fixture;
    perturbed_fixture.route.shots.front().camera_right =
        {0.9998F, 0.0F, 0.02F};

    const auto baseline = test::replay_frozen_playthrough(baseline_fixture);
    const auto perturbed = test::replay_frozen_playthrough(perturbed_fixture);

    NINHO_SIM_REQUIRE(baseline.value.shots.size() == 1U);
    NINHO_SIM_REQUIRE(perturbed.value.shots.size() == 1U);
    NINHO_SIM_REQUIRE(baseline.value.shots.front().locked_camera_right
        != perturbed.value.shots.front().locked_camera_right);
}

NINHO_SIM_TEST("product v2 determinism frozen replayer rejects an unexecutable requested ability")
{
    using namespace ninho::simulation;
    const auto fixture = executable_fixture("earth", "farm_reaction", {
        {{1.0F, 0.0F, 0.0F}, -4.0, -0.4, std::uint64_t{0U}},
    });

    const auto replayed = test::replay_frozen_playthrough(fixture);

    NINHO_SIM_REQUIRE(!replayed.ok());
    NINHO_SIM_REQUIRE(replayed.error.code == ContentErrorCode::InvalidInvariant);
    NINHO_SIM_REQUIRE(replayed.error.pointer
        == "/shots/0/ability_tick_after_launch");
}

NINHO_SIM_TEST("product v2 determinism frozen replayer rejects orbital legacy launch metadata not reproduced by the gesture")
{
    using namespace ninho::simulation;
    auto fixture = executable_fixture("orbital", "first_orbit_v2", {
        {{0.0F, 1.0F, 0.0F}, 0.0, -4.0, std::uint64_t{0U}},
    });
    fixture.legacy_quantized_launches.front() =
        test::FrozenLegacyQuantizedLaunch{
            .origin_mm = {-12999, 0, 0},
            .normalized_direction_1e4 = {-10000, 0, 0},
            .speed_centimeters_per_second = 1504U,
        };

    const auto replayed = test::replay_frozen_playthrough(fixture);

    NINHO_SIM_REQUIRE(!replayed.ok());
    NINHO_SIM_REQUIRE(replayed.error.code == ContentErrorCode::InvalidInvariant);
    NINHO_SIM_REQUIRE(replayed.error.pointer
        == "/shots/0/legacy_quantized_launch");
}

NINHO_SIM_TEST("product v2 determinism frozen replayer binds Blue activation after source replacement")
{
    using namespace ninho::simulation;
    const GestureShotInput miss{{1.0F, 0.0F, 0.0F}, 0.0, -4.0,
        std::nullopt};
    const auto fixture = executable_fixture("earth", "farm_reaction", {
        miss,
        miss,
        {{1.0F, 0.0F, 0.0F}, 0.0, -4.0, std::uint64_t{9U}},
    });

    const auto replayed = test::replay_frozen_playthrough(fixture);

    NINHO_SIM_REQUIRE(!replayed.ok());
    NINHO_SIM_REQUIRE(replayed.error.pointer == "/expected/outcome");
    NINHO_SIM_REQUIRE(replayed.value.shots.size() == 3U);
    const auto& blue = replayed.value.shots.back();
    NINHO_SIM_REQUIRE(blue.ability_started_tick.has_value());
    NINHO_SIM_REQUIRE(blue.ability_started_tick->value()
        == blue.launch_tick.value() + 9U);
    NINHO_SIM_REQUIRE(blue.activation_consumed);
}

NINHO_SIM_TEST("product v2 determinism frozen replayer rejects a terminal route with shots remaining")
{
    using namespace ninho::simulation;
    const GestureShotInput miss{{0.0F, 1.0F, 0.0F}, 0.0, -4.0, std::nullopt};
    const auto fixture = executable_fixture("orbital", "first_orbit_v2", {
        miss, miss, miss, miss,
    });

    const auto replayed = test::replay_frozen_playthrough(fixture);

    NINHO_SIM_REQUIRE(!replayed.ok());
    NINHO_SIM_REQUIRE(replayed.error.code == ContentErrorCode::InvalidInvariant);
    NINHO_SIM_REQUIRE(replayed.error.pointer == "/shots/3");
}

NINHO_SIM_TEST("product v2 determinism replays all seven routes fifty times against common bytes")
{
    using namespace ninho::simulation;
    for (const std::string_view route_name : frozen_route_names) {
        std::ifstream fixture_file{frozen_fixture_path(route_name),
            std::ios::binary};
        NINHO_SIM_REQUIRE(fixture_file.is_open());
        std::ostringstream document;
        document << fixture_file.rdbuf();
        const auto fixture = test::parse_frozen_playthrough_fixture_envelope(
            document.str());
        NINHO_SIM_REQUIRE(fixture.ok());
        NINHO_SIM_REQUIRE(fixture.value.route_id == route_name);

        std::optional<test::FrozenPlaythroughReplay> baseline;
        for (std::uint32_t repeat = 0U; repeat < 50U; ++repeat) {
            const auto replayed = test::replay_frozen_playthrough(fixture.value);
            NINHO_SIM_REQUIRE(replayed.ok());
            if (!baseline) {
                baseline = replayed.value;
            } else {
                NINHO_SIM_REQUIRE(replayed.value == *baseline);
            }
        }
    }
}

}
