#include "product_v2_playthrough_fixture.hpp"

#include "product_v2_reader.hpp"

#include <nlohmann/json.hpp>

#include <algorithm>
#include <array>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <limits>
#include <memory>
#include <numbers>
#include <sstream>
#include <span>
#include <stdexcept>
#include <utility>

namespace ninho::simulation::test {
namespace {

using json = nlohmann::json;
namespace reader = ninho::simulation::detail::v2content;
constexpr std::size_t maximum_blob_hex_characters =
    frozen_playthrough_fixture_max_blob_bytes * 2U;
constexpr std::array virela_legacy_aim_seeds{
    LegacyAimSeed{0.0, -0.1, 8.0},
    LegacyAimSeed{0.0, 8.0, 8.18},
};
constexpr std::array structural_legacy_aim_seeds{
    LegacyAimSeed{0.0, 5.0, 8.0},
    LegacyAimSeed{0.0, 1.0, 8.0},
};
constexpr std::array defeat_legacy_aim_seeds{
    LegacyAimSeed{0.0, 90.0, 8.0},
    LegacyAimSeed{0.0, 90.0, 8.0},
    LegacyAimSeed{0.0, 90.0, 8.0},
};

std::string string_member(const json& object, std::string_view key,
    const std::string& pointer, std::size_t maximum)
{
    const std::string value_pointer = reader::child(pointer, key);
    const json& value = reader::member(object, key, pointer);
    if (!value.is_string()) {
        reader::fail(ContentErrorCode::InvalidType, value_pointer,
            "expected string");
    }
    auto result = value.get<std::string>();
    if (result.empty()) {
        reader::fail(ContentErrorCode::OutOfRange, value_pointer,
            "string cannot be empty");
    }
    if (result.size() > maximum) {
        reader::fail(ContentErrorCode::ResourceLimit, value_pointer,
            "string limit exceeded");
    }
    return result;
}

std::uint64_t unsigned_integer(const json& value, const std::string& pointer,
    std::uint64_t maximum = std::numeric_limits<std::uint64_t>::max())
{
    if (!value.is_number_integer() && !value.is_number_unsigned()) {
        reader::fail(ContentErrorCode::InvalidType, pointer,
            "expected unsigned integer");
    }
    std::uint64_t result{};
    try {
        if (value.is_number_integer()) {
            const auto signed_value = value.get<std::int64_t>();
            if (signed_value < 0) {
                reader::fail(ContentErrorCode::OutOfRange, pointer,
                    "integer out of range");
            }
            result = static_cast<std::uint64_t>(signed_value);
        } else {
            result = value.get<std::uint64_t>();
        }
    } catch (const reader::ParseFailure&) {
        throw;
    } catch (...) {
        reader::fail(ContentErrorCode::OutOfRange, pointer,
            "integer out of range");
    }
    if (result > maximum) {
        reader::fail(ContentErrorCode::OutOfRange, pointer,
            "integer out of range");
    }
    return result;
}

std::int64_t signed_integer(const json& value, const std::string& pointer,
    std::int64_t minimum, std::int64_t maximum)
{
    if (!value.is_number_integer() && !value.is_number_unsigned()) {
        reader::fail(ContentErrorCode::InvalidType, pointer,
            "expected signed integer");
    }
    std::int64_t result{};
    try {
        if (value.is_number_unsigned()) {
            const std::uint64_t source = value.get<std::uint64_t>();
            if (source > static_cast<std::uint64_t>(
                    std::numeric_limits<std::int64_t>::max())) {
                reader::fail(ContentErrorCode::OutOfRange, pointer,
                    "integer out of range");
            }
            result = static_cast<std::int64_t>(source);
        } else {
            result = value.get<std::int64_t>();
        }
    } catch (const reader::ParseFailure&) {
        throw;
    } catch (...) {
        reader::fail(ContentErrorCode::OutOfRange, pointer,
            "integer out of range");
    }
    if (result < minimum || result > maximum) {
        reader::fail(ContentErrorCode::OutOfRange, pointer,
            "integer out of range");
    }
    return result;
}

std::array<std::int64_t, 3> signed_integer_triple(const json& value,
    const std::string& pointer, std::int64_t minimum, std::int64_t maximum)
{
    reader::array(value, pointer, 3U, true);
    if (value.size() != 3U) {
        reader::fail(ContentErrorCode::OutOfRange, pointer,
            "integer vector has wrong length");
    }
    std::array<std::int64_t, 3> result{};
    for (std::size_t index = 0U; index < result.size(); ++index) {
        result[index] = signed_integer(value.at(index),
            reader::indexed(pointer, index), minimum, maximum);
    }
    return result;
}

int lowercase_hex_nibble(char value) noexcept
{
    if (value >= '0' && value <= '9') return value - '0';
    if (value >= 'a' && value <= 'f') return value - 'a' + 10;
    return -1;
}

std::vector<std::uint8_t> decode_hex(
    const std::string& text, const std::string& pointer)
{
    if ((text.size() & 1U) != 0U) {
        reader::fail(ContentErrorCode::InvalidInvariant, pointer,
            "hex must contain complete bytes");
    }
    std::vector<std::uint8_t> result;
    result.reserve(text.size() / 2U);
    for (std::size_t index = 0; index < text.size(); index += 2U) {
        const int high = lowercase_hex_nibble(text[index]);
        const int low = lowercase_hex_nibble(text[index + 1U]);
        if (high < 0 || low < 0) {
            reader::fail(ContentErrorCode::InvalidInvariant, pointer,
                "hex must use lowercase ASCII digits");
        }
        result.push_back(static_cast<std::uint8_t>((high << 4) | low));
    }
    return result;
}

std::uint64_t decode_hash(
    const std::string& text, const std::string& pointer)
{
    constexpr std::string_view prefix{"fnv1a64:"};
    if (text.size() != prefix.size() + 16U
        || !text.starts_with(prefix)) {
        reader::fail(ContentErrorCode::InvalidInvariant, pointer,
            "hash must be fnv1a64 followed by 16 lowercase hex digits");
    }
    std::uint64_t value{};
    for (std::size_t index = prefix.size(); index < text.size(); ++index) {
        const int nibble = lowercase_hex_nibble(text[index]);
        if (nibble < 0) {
            reader::fail(ContentErrorCode::InvalidInvariant, pointer,
                "hash must use lowercase ASCII digits");
        }
        value = (value << 4U) | static_cast<std::uint64_t>(nibble);
    }
    return value;
}

std::uint32_t read_little_u32(std::span<const std::uint8_t> bytes,
    std::size_t offset, const std::string& pointer)
{
    if (offset > bytes.size() || bytes.size() - offset < 4U) {
        reader::fail(ContentErrorCode::InvalidInvariant, pointer,
            "truncated binary header");
    }
    std::uint32_t value{};
    for (unsigned byte = 0U; byte < 4U; ++byte) {
        value |= static_cast<std::uint32_t>(bytes[offset + byte]) << (byte * 8U);
    }
    return value;
}

std::size_t validate_versioned_header(std::span<const std::uint8_t> bytes,
    std::string_view name, std::uint32_t version, const std::string& pointer)
{
    const std::uint32_t name_length = read_little_u32(bytes, 0U, pointer);
    if (name_length != name.size()
        || bytes.size() < 4U + static_cast<std::size_t>(name_length) + 4U) {
        reader::fail(ContentErrorCode::InvalidInvariant, pointer,
            "unexpected or truncated binary blob header");
    }
    for (std::size_t index = 0; index < name.size(); ++index) {
        if (bytes[4U + index] != static_cast<std::uint8_t>(name[index])) {
            reader::fail(ContentErrorCode::InvalidInvariant, pointer,
                "unexpected binary blob header");
        }
    }
    const std::size_t version_offset = 4U + name.size();
    if (read_little_u32(bytes, version_offset, pointer) != version) {
        reader::fail(ContentErrorCode::InvalidInvariant, pointer,
            "unexpected binary blob version");
    }
    return version_offset + 4U;
}

void validate_ordered_events(std::span<const std::uint8_t> bytes,
    const std::string& pointer)
{
    constexpr std::size_t event_record_bytes = 274U;
    const std::size_t count_offset = validate_versioned_header(
        bytes, "ordered_events_v3", 3U, pointer);
    const std::uint32_t count = read_little_u32(bytes, count_offset, pointer);
    const std::uint64_t expected_size = static_cast<std::uint64_t>(count_offset)
        + 4U + static_cast<std::uint64_t>(count) * event_record_bytes;
    if (expected_size != bytes.size()) {
        reader::fail(ContentErrorCode::InvalidInvariant, pointer,
            "ordered event count does not match fixed record framing");
    }
}

ninho::physics::Vec3 vector3(const json& value, const std::string& pointer,
    double minimum, double maximum)
{
    reader::array(value, pointer, 3U, true);
    if (value.size() != 3U) {
        reader::fail(ContentErrorCode::OutOfRange, pointer,
            "vector has wrong length");
    }
    return {
        static_cast<float>(reader::number_value(
            value.at(0), reader::indexed(pointer, 0U), minimum, maximum)),
        static_cast<float>(reader::number_value(
            value.at(1), reader::indexed(pointer, 1U), minimum, maximum)),
        static_cast<float>(reader::number_value(
            value.at(2), reader::indexed(pointer, 2U), minimum, maximum)),
    };
}

FrozenLegacyQuantizedLaunch legacy_launch(
    const json& value, const std::string& pointer)
{
    reader::keys(value, pointer,
        {"origin_mm", "normalized_direction_1e4",
            "speed_centimeters_per_second"});
    const std::string origin_pointer = reader::child(pointer, "origin_mm");
    const std::string direction_pointer =
        reader::child(pointer, "normalized_direction_1e4");
    FrozenLegacyQuantizedLaunch result{
        .origin_mm = signed_integer_triple(
            reader::member(value, "origin_mm", pointer), origin_pointer,
            -1000000, 1000000),
        .normalized_direction_1e4 = signed_integer_triple(
            reader::member(value, "normalized_direction_1e4", pointer),
            direction_pointer, -10000, 10000),
        .speed_centimeters_per_second = unsigned_integer(reader::member(
            value, "speed_centimeters_per_second", pointer),
            reader::child(pointer, "speed_centimeters_per_second"), 100000U),
    };
    if (std::ranges::all_of(result.normalized_direction_1e4,
            [](std::int64_t component) { return component == 0; })) {
        reader::fail(ContentErrorCode::InvalidInvariant, direction_pointer,
            "canonical normalized direction cannot be zero");
    }
    return result;
}

}

ContentResult<FrozenPlaythroughFixture>
parse_frozen_playthrough_fixture_envelope(std::string_view document) noexcept
{
    return reader::boundary<FrozenPlaythroughFixture>([&] {
        const json root = reader::parse_json(
            document, frozen_playthrough_fixture_max_bytes);
        reader::keys(root, "",
            {"schema_version", "route_id", "world_id", "level_id", "shots",
                "expected"});
        FrozenPlaythroughFixture fixture;
        fixture.schema_version = reader::uint(root, "schema_version", "", 1U, 1U);
        fixture.route_id = reader::text(root, "route_id", "");
        fixture.route.world_id = reader::text(root, "world_id", "");
        fixture.route.level_id = reader::text(root, "level_id", "");
        const bool orbital = fixture.route.world_id == "orbital";
        if (!orbital && fixture.route.world_id != "earth") {
            reader::fail(ContentErrorCode::InvalidEnum, "/world_id",
                "unknown playthrough world");
        }
        const json& shots = reader::member(root, "shots", "");
        reader::array(shots, "/shots", frozen_playthrough_fixture_max_shots, true);
        for (std::size_t index = 0; index < shots.size(); ++index) {
            const json& shot = shots[index];
            const std::string pointer = reader::indexed("/shots", index);
            if (orbital) {
                reader::keys(shot, pointer,
                    {"camera_right", "pull_horizontal_m", "pull_vertical_m",
                        "legacy_quantized_launch"},
                    {"ability_tick_after_launch"});
            } else {
                reader::keys(shot, pointer,
                    {"camera_right", "pull_horizontal_m", "pull_vertical_m"},
                    {"ability_tick_after_launch"});
            }
            std::optional<std::uint64_t> ability_tick;
            if (shot.contains("ability_tick_after_launch")) {
                ability_tick = unsigned_integer(
                    reader::member(shot, "ability_tick_after_launch", pointer),
                    reader::child(pointer, "ability_tick_after_launch"), 100000U);
            }
            fixture.route.shots.push_back({
                vector3(reader::member(shot, "camera_right", pointer),
                    reader::child(pointer, "camera_right"), -1.0, 1.0),
                reader::number(shot, "pull_horizontal_m", pointer, -4.25, 4.25),
                reader::number(shot, "pull_vertical_m", pointer, -4.25, 4.25),
                ability_tick,
            });
            std::optional<FrozenLegacyQuantizedLaunch> legacy;
            if (orbital) {
                const std::string legacy_pointer =
                    reader::child(pointer, "legacy_quantized_launch");
                legacy = legacy_launch(reader::member(
                    shot, "legacy_quantized_launch", pointer), legacy_pointer);
            }
            fixture.legacy_quantized_launches.push_back(std::move(legacy));
        }
        const json& expected = reader::member(root, "expected", "");
        reader::keys(expected, "/expected",
            {"outcome", "score", "stars", "birds_remaining",
                "objectives_complete", "ordered_events_v3_hex",
                "canonical_state_v3_hex", "canonical_state_v3_fnv1a64",
                "canonical_playthrough_v5_hex",
                "canonical_playthrough_v5_fnv1a64"});
        const std::string outcome = reader::text(expected, "outcome", "/expected");
        if (outcome == "victory") {
            fixture.expected.outcome = Outcome::Victory;
        } else if (outcome == "defeat") {
            fixture.expected.outcome = Outcome::Defeat;
        } else {
            reader::fail(ContentErrorCode::InvalidEnum, "/expected/outcome",
                "unknown terminal outcome");
        }
        fixture.expected.score = unsigned_integer(
            reader::member(expected, "score", "/expected"), "/expected/score");
        fixture.expected.stars = reader::uint(
            expected, "stars", "/expected", 0U, 3U);
        fixture.expected.birds_remaining = reader::uint(
            expected, "birds_remaining", "/expected", 0U, 32U);
        fixture.expected.objectives_complete = reader::boolean(
            expected, "objectives_complete", "/expected");
        fixture.expected.ordered_events_v3 = decode_hex(
            string_member(expected, "ordered_events_v3_hex", "/expected",
                maximum_blob_hex_characters),
            "/expected/ordered_events_v3_hex");
        fixture.expected.canonical_state_v3 = decode_hex(
            string_member(expected, "canonical_state_v3_hex", "/expected",
                maximum_blob_hex_characters),
            "/expected/canonical_state_v3_hex");
        fixture.expected.canonical_state_v3_fnv1a64 = decode_hash(
            reader::text(expected, "canonical_state_v3_fnv1a64", "/expected"),
            "/expected/canonical_state_v3_fnv1a64");
        fixture.expected.canonical_playthrough_v5 = decode_hex(
            string_member(expected, "canonical_playthrough_v5_hex", "/expected",
                maximum_blob_hex_characters),
            "/expected/canonical_playthrough_v5_hex");
        fixture.expected.canonical_playthrough_v5_fnv1a64 = decode_hash(
            reader::text(expected, "canonical_playthrough_v5_fnv1a64", "/expected"),
            "/expected/canonical_playthrough_v5_fnv1a64");

        validate_ordered_events(fixture.expected.ordered_events_v3,
            "/expected/ordered_events_v3_hex");
        validate_versioned_header(fixture.expected.canonical_state_v3,
            "canonical_state_v3", 3U, "/expected/canonical_state_v3_hex");
        if (fnv1a64(fixture.expected.canonical_state_v3)
            != fixture.expected.canonical_state_v3_fnv1a64) {
            reader::fail(ContentErrorCode::InvalidInvariant,
                "/expected/canonical_state_v3_fnv1a64",
                "canonical state hash mismatch");
        }
        validate_versioned_header(fixture.expected.canonical_playthrough_v5,
            "canonical_playthrough_v5", 5U,
            "/expected/canonical_playthrough_v5_hex");
        if (fnv1a64(fixture.expected.canonical_playthrough_v5)
            != fixture.expected.canonical_playthrough_v5_fnv1a64) {
            reader::fail(ContentErrorCode::InvalidInvariant,
                "/expected/canonical_playthrough_v5_fnv1a64",
                "canonical playthrough hash mismatch");
        }
        if (canonical_playthrough_v5(fixture.route,
                fixture.expected.ordered_events_v3,
                fixture.expected.canonical_state_v3)
            != fixture.expected.canonical_playthrough_v5) {
            reader::fail(ContentErrorCode::InvalidInvariant,
                "/expected/canonical_playthrough_v5_hex",
                "canonical playthrough does not match route and blobs");
        }
        return fixture;
    });
}

ContentResult<FrozenLegacyQuantizedLaunch>
canonical_legacy_launch_units(ninho::physics::Vec3 origin_m,
    ninho::physics::Vec3 direction, double speed_m_s) noexcept
{
    return reader::boundary<FrozenLegacyQuantizedLaunch>([&] {
        const auto require_finite_vector = [](ninho::physics::Vec3 value,
                                               const std::string& pointer) {
            if (!std::isfinite(value.x) || !std::isfinite(value.y)
                || !std::isfinite(value.z)) {
                reader::fail(ContentErrorCode::InvalidNumber, pointer,
                    "canonical launch vector must be finite");
            }
        };
        const auto signed_units = [](double value, double units_per_value,
                                      const std::string& pointer) {
            constexpr double first_unrepresentable_positive =
                9223372036854775808.0;
            constexpr double smallest_representable =
                -9223372036854775808.0;
            const double scaled = value * units_per_value;
            if (!std::isfinite(scaled)
                || scaled >= first_unrepresentable_positive
                || scaled < smallest_representable) {
                reader::fail(ContentErrorCode::OutOfRange, pointer,
                    "canonical launch integer units overflow");
            }
            return static_cast<std::int64_t>(std::llround(scaled));
        };

        constexpr std::string_view launch_pointer{"/legacy_quantized_launch"};
        const std::string origin_pointer =
            reader::child(std::string{launch_pointer}, "origin_m");
        const std::string direction_pointer =
            reader::child(std::string{launch_pointer}, "direction");
        const std::string speed_pointer =
            reader::child(std::string{launch_pointer}, "speed_m_s");
        require_finite_vector(origin_m, origin_pointer);
        require_finite_vector(direction, direction_pointer);
        if (!std::isfinite(speed_m_s)) {
            reader::fail(ContentErrorCode::InvalidNumber, speed_pointer,
                "canonical launch speed must be finite");
        }
        if (speed_m_s < 0.0) {
            reader::fail(ContentErrorCode::OutOfRange, speed_pointer,
                "canonical launch speed cannot be negative");
        }

        const double direction_x = static_cast<double>(direction.x);
        const double direction_y = static_cast<double>(direction.y);
        const double direction_z = static_cast<double>(direction.z);
        const double magnitude = std::sqrt(
            direction_x * direction_x + direction_y * direction_y
            + direction_z * direction_z);
        if (magnitude == 0.0) {
            reader::fail(ContentErrorCode::InvalidInvariant,
                direction_pointer,
                "canonical launch direction cannot be zero");
        }
        if (!std::isfinite(magnitude)) {
            reader::fail(ContentErrorCode::OutOfRange, direction_pointer,
                "canonical launch direction magnitude is out of range");
        }

        FrozenLegacyQuantizedLaunch result{
            .origin_mm = {
                signed_units(static_cast<double>(origin_m.x), 1000.0,
                    reader::indexed(origin_pointer, 0U)),
                signed_units(static_cast<double>(origin_m.y), 1000.0,
                    reader::indexed(origin_pointer, 1U)),
                signed_units(static_cast<double>(origin_m.z), 1000.0,
                    reader::indexed(origin_pointer, 2U)),
            },
            .normalized_direction_1e4 = {
                signed_units(direction_x / magnitude, 10000.0,
                    reader::indexed(direction_pointer, 0U)),
                signed_units(direction_y / magnitude, 10000.0,
                    reader::indexed(direction_pointer, 1U)),
                signed_units(direction_z / magnitude, 10000.0,
                    reader::indexed(direction_pointer, 2U)),
            },
        };
        const std::int64_t speed_units =
            signed_units(speed_m_s, 100.0, speed_pointer);
        result.speed_centimeters_per_second =
            static_cast<std::uint64_t>(speed_units);
        return result;
    });
}

namespace {

constexpr std::uint32_t replay_shot_watchdog_ticks = 2500U;

std::string replay_source_file(std::string_view relative)
{
    std::ifstream stream{std::filesystem::path{NINHO_SOURCE_DIR} / relative,
        std::ios::binary};
    if (!stream.is_open()) {
        throw std::runtime_error("could not open " + std::string{relative});
    }
    std::ostringstream text;
    text << stream.rdbuf();
    return text.str();
}

std::string shot_pointer(std::size_t shot_index, std::string_view suffix = {})
{
    std::string result = "/shots/" + std::to_string(shot_index);
    result.append(suffix);
    return result;
}

ContentError replay_error(std::string pointer, std::string message)
{
    return {ContentErrorCode::InvalidInvariant,
        std::move(pointer), std::move(message)};
}

ContentError contextual_error(
    std::size_t shot_index, const ContentError& source)
{
    std::string pointer = shot_pointer(shot_index);
    if (!source.pointer.empty()) {
        pointer += source.pointer;
    }
    return {source.code, std::move(pointer), source.message};
}

std::optional<ContentError> compare_replay(
    const FrozenPlaythroughExpected& actual,
    const FrozenPlaythroughExpected& expected)
{
    if (actual.outcome != expected.outcome) {
        return replay_error("/expected/outcome", "terminal outcome mismatch");
    }
    if (actual.score != expected.score) {
        return replay_error("/expected/score", "terminal score mismatch");
    }
    if (actual.stars != expected.stars) {
        return replay_error("/expected/stars", "terminal stars mismatch");
    }
    if (actual.birds_remaining != expected.birds_remaining) {
        return replay_error("/expected/birds_remaining",
            "terminal bird count mismatch");
    }
    if (actual.objectives_complete != expected.objectives_complete) {
        return replay_error("/expected/objectives_complete",
            "terminal objective state mismatch");
    }
    if (actual.ordered_events_v3 != expected.ordered_events_v3) {
        return replay_error("/expected/ordered_events_v3_hex",
            "ordered event bytes mismatch");
    }
    if (actual.canonical_state_v3 != expected.canonical_state_v3) {
        return replay_error("/expected/canonical_state_v3_hex",
            "canonical state bytes mismatch");
    }
    if (actual.canonical_state_v3_fnv1a64
        != expected.canonical_state_v3_fnv1a64) {
        return replay_error("/expected/canonical_state_v3_fnv1a64",
            "canonical state hash mismatch");
    }
    if (actual.canonical_playthrough_v5
        != expected.canonical_playthrough_v5) {
        return replay_error("/expected/canonical_playthrough_v5_hex",
            "canonical playthrough bytes mismatch");
    }
    if (actual.canonical_playthrough_v5_fnv1a64
        != expected.canonical_playthrough_v5_fnv1a64) {
        return replay_error("/expected/canonical_playthrough_v5_fnv1a64",
            "canonical playthrough hash mismatch");
    }
    return std::nullopt;
}

}

std::span<const LegacyAimSeed>
canonical_orbital_legacy_aim_seeds(std::string_view route_id) noexcept
{
    if (route_id == "orbital_virela") {
        return virela_legacy_aim_seeds;
    }
    if (route_id == "orbital_structural") {
        return structural_legacy_aim_seeds;
    }
    if (route_id == "orbital_defeat") {
        return defeat_legacy_aim_seeds;
    }
    return {};
}

AimState legacy_ring_aim(LegacyAimSeed seed) noexcept
{
    const double theta = seed.theta_deg * std::numbers::pi / 180.0;
    const double phase = seed.phase_deg * std::numbers::pi / 180.0;
    const ninho::physics::Vec3 origin{
        static_cast<float>(-13.0 * std::cos(theta)), 0.0F,
        static_cast<float>(13.0 * std::sin(theta))};
    const ninho::physics::Vec3 azimuth{
        static_cast<float>(std::sin(theta)), 0.0F,
        static_cast<float>(std::cos(theta))};
    const ninho::physics::Vec3 tangent = ninho::physics::normalized_or_zero(
        ninho::physics::Vec3{0.0F, static_cast<float>(std::cos(phase)), 0.0F}
        + azimuth * static_cast<float>(std::sin(phase)));
    return {origin, tangent, seed.speed_m_s};
}

ContentResult<FrozenPlaythroughReplay>
replay_frozen_playthrough(const FrozenPlaythroughFixture& fixture) noexcept
{
    ContentResult<FrozenPlaythroughReplay> result;
    try {
        std::string_view level_path;
        if (fixture.route.world_id == "earth"
            && fixture.route.level_id == "farm_reaction") {
            level_path = "game/data/levels/earth/farm_reaction.level.json";
        } else if (fixture.route.world_id == "orbital"
            && fixture.route.level_id == "first_orbit_v2") {
            level_path = "game/data/levels/orbital/first_orbit_v2.level.json";
        } else {
            result.error = replay_error("/level_id",
                "frozen replayer only accepts product-v2 route levels");
            return result;
        }
        if (fixture.route.shots.empty()
            || fixture.route.shots.size() > frozen_playthrough_fixture_max_shots) {
            result.error = replay_error("/shots",
                "frozen route has an invalid shot count");
            return result;
        }
        if (fixture.legacy_quantized_launches.size()
            != fixture.route.shots.size()) {
            result.error = replay_error("/shots",
                "legacy launch metadata count does not match frozen shots");
            return result;
        }
        const bool orbital = fixture.route.world_id == "orbital";
        for (std::size_t shot_index = 0U;
             shot_index < fixture.legacy_quantized_launches.size(); ++shot_index) {
            if (fixture.legacy_quantized_launches[shot_index].has_value()
                != orbital) {
                result.error = replay_error(shot_pointer(
                    shot_index, "/legacy_quantized_launch"),
                    "legacy launch metadata does not match the route world");
                return result;
            }
        }

        const auto materials = parse_material_catalog(replay_source_file(
            "game/data/materials/product_v2.materials.json"));
        const auto archetypes = parse_archetype_catalog(replay_source_file(
            "game/data/archetypes/product_v2.archetypes.json"));
        const auto level = parse_level_manifest(replay_source_file(level_path));
        if (!materials.ok()) {
            result.error = materials.error;
            return result;
        }
        if (!archetypes.ok()) {
            result.error = archetypes.error;
            return result;
        }
        if (!level.ok()) {
            result.error = level.error;
            return result;
        }
        auto created = SimulationSession::create(
            materials.value, archetypes.value, level.value);
        if (!created.ok()) {
            result.error = created.error;
            return result;
        }
        std::unique_ptr<SimulationSession> session = std::move(created.value);
        std::vector<DomainEvent> events;

        const auto advance = [&](std::size_t shot_index) -> bool {
            const SessionStatus status = session->tick();
            if (!status.ok()) {
                result.error = contextual_error(shot_index, status.error);
                return false;
            }
            events.insert(events.end(),
                session->events().begin(), session->events().end());
            return true;
        };
        const auto enqueue = [&](std::size_t shot_index,
                                 PlayerCommand command) -> bool {
            const SessionStatus status = session->enqueue(std::move(command));
            if (!status.ok()) {
                result.error = contextual_error(shot_index, status.error);
                return false;
            }
            return true;
        };

        result.value.shots.reserve(fixture.route.shots.size());
        for (std::size_t shot_index = 0U;
             shot_index < fixture.route.shots.size(); ++shot_index) {
            const GestureShotInput& input = fixture.route.shots[shot_index];
            if (session->state().phase == SessionPhase::Result) {
                result.error = replay_error(shot_pointer(shot_index),
                    "route terminated before the remaining frozen shot");
                return result;
            }
            if (session->state().phase != SessionPhase::Inspection) {
                result.error = replay_error(shot_pointer(shot_index),
                    "frozen shot did not begin in Inspection");
                return result;
            }
            if (!enqueue(shot_index, BeginGrabCommand{input.camera_right})
                || !enqueue(shot_index, SetPullCommand{
                    input.pull_horizontal_m, input.pull_vertical_m})
                || !advance(shot_index)) {
                return result;
            }
            if (session->state().phase != SessionPhase::Grabbed
                || !session->state().launcher) {
                result.error = replay_error(
                    shot_pointer(shot_index, "/camera_right"),
                    "BeginGrab and SetPull did not solve the frozen gesture");
                return result;
            }
            std::optional<FrozenLegacyQuantizedLaunch> quantized_launch;
            if (orbital) {
                const LauncherState& solved = *session->state().launcher;
                const auto canonical_launch = canonical_legacy_launch_units(
                    solved.rest_position_m,
                    solved.launch_direction,
                    solved.predicted_speed_m_s);
                if (!canonical_launch.ok()) {
                    result.error = contextual_error(
                        shot_index, canonical_launch.error);
                    return result;
                }
                quantized_launch = canonical_launch.value;
                if (quantized_launch
                    != fixture.legacy_quantized_launches[shot_index]) {
                    result.error = replay_error(shot_pointer(
                        shot_index, "/legacy_quantized_launch"),
                        "public gesture did not reproduce the frozen legacy launch");
                    return result;
                }
            }
            if (!enqueue(shot_index, ReleaseBirdCommand{})
                || !advance(shot_index)) {
                return result;
            }
            const std::optional<ShotStateView> launched = session->shot_state();
            if (!launched || launched->projectile_ids.empty()
                || session->state().phase != SessionPhase::FlightAbility) {
                result.error = replay_error(shot_pointer(shot_index),
                    "ReleaseBird did not create an active projectile");
                return result;
            }
            const EntityId primary_projectile = launched->projectile_ids.front();
            const auto launch_event = std::ranges::find_if(
                session->events(), [&](const DomainEvent& event) {
                    return event.kind == DomainEventKind::BirdLaunched
                        && event.shot_id == launched->shot_id
                        && event.entity_id == primary_projectile
                        && event.bird_archetype_id == launched->bird_archetype_id;
                });
            if (launch_event == session->events().end()
                || std::ranges::count(session->events(),
                       DomainEventKind::BirdLaunched, &DomainEvent::kind) != 1) {
                result.error = replay_error(shot_pointer(shot_index),
                    "release tick did not publish exactly one matching launch");
                return result;
            }
            result.value.shots.push_back({
                .shot_id = launched->shot_id,
                .bird_archetype_id = launched->bird_archetype_id,
                .ability_id = launched->ability_id,
                .primary_projectile_id = primary_projectile,
                .launch_tick = launched->launch_tick,
                .locked_camera_right = launched->locked_plane.camera_right,
                .quantized_launch = quantized_launch,
            });
            FrozenReplayedShot& replayed_shot = result.value.shots.back();

            if (input.ability_tick_after_launch) {
                const std::uint64_t offset = *input.ability_tick_after_launch;
                if (offset > std::numeric_limits<std::uint64_t>::max()
                        - launched->launch_tick.value()) {
                    result.error = replay_error(shot_pointer(
                        shot_index, "/ability_tick_after_launch"),
                        "requested ability tick overflows the timeline");
                    return result;
                }
                const std::uint64_t requested_tick =
                    launched->launch_tick.value() + offset;
                while (session->state().phase == SessionPhase::FlightAbility
                    && session->state().tick.value() + 1U < requested_tick) {
                    if (!advance(shot_index)) {
                        return result;
                    }
                }
                const std::optional<ShotStateView> active = session->shot_state();
                const bool same_active_shot = active
                    && active->shot_id == launched->shot_id
                    && active->bird_archetype_id == launched->bird_archetype_id
                    && active->ability_id == launched->ability_id
                    && std::ranges::find(active->projectile_ids,
                           primary_projectile) != active->projectile_ids.end();
                if (session->state().phase != SessionPhase::FlightAbility
                    || session->state().tick.value()
                        == std::numeric_limits<std::uint64_t>::max()
                    || session->state().tick.value() + 1U != requested_tick
                    || !same_active_shot || active->activation_consumed) {
                    result.error = replay_error(shot_pointer(
                        shot_index, "/ability_tick_after_launch"),
                        "ability cannot execute at the requested tick");
                    return result;
                }
                if (!enqueue(shot_index, ActivateAbilityCommand{})
                    || !advance(shot_index)) {
                    return result;
                }
                const auto started = std::ranges::find_if(
                    session->events(), [&](const DomainEvent& event) {
                        return event.kind == DomainEventKind::AbilityStarted
                            && event.tick.value() == requested_tick
                            && event.entity_id == primary_projectile
                            && event.bird_archetype_id == launched->bird_archetype_id
                            && event.ability_id == launched->ability_id;
                    });
                const std::optional<ShotStateView> activated = session->shot_state();
                const bool activation_bound_to_active_shot = activated
                    && activated->shot_id == launched->shot_id
                    && activated->bird_archetype_id == launched->bird_archetype_id
                    && activated->ability_id == launched->ability_id
                    && !activated->projectile_ids.empty()
                    && activated->activation_consumed
                    && (activated->ability_readiness == AbilityReadiness::Active
                        || activated->ability_readiness
                            == AbilityReadiness::Spent);
                if (session->state().tick.value() != requested_tick
                    || started == session->events().end()
                    || std::ranges::count(session->events(),
                           DomainEventKind::AbilityStarted,
                           &DomainEvent::kind) != 1
                    || !activation_bound_to_active_shot) {
                    result.error = replay_error(shot_pointer(
                        shot_index, "/ability_tick_after_launch"),
                        "ability did not start on the requested active shot tick");
                    return result;
                }
                replayed_shot.ability_started_tick = started->tick;
                replayed_shot.activation_consumed =
                    activated->activation_consumed;
            }

            bool resolved = false;
            for (std::uint32_t watchdog = 0U;
                 watchdog < replay_shot_watchdog_ticks; ++watchdog) {
                if (session->state().phase == SessionPhase::Evaluation) {
                    if (!advance(shot_index)) {
                        return result;
                    }
                    resolved = true;
                    break;
                }
                if (session->state().phase == SessionPhase::Inspection
                    || session->state().phase == SessionPhase::Result) {
                    resolved = true;
                    break;
                }
                if (session->state().phase == SessionPhase::Faulted) {
                    result.error = replay_error(shot_pointer(shot_index),
                        "session faulted while resolving the frozen shot");
                    return result;
                }
                if (!advance(shot_index)) {
                    return result;
                }
            }
            if (!resolved) {
                result.error = replay_error(shot_pointer(shot_index),
                    "frozen shot exceeded the replay watchdog");
                return result;
            }
            const bool has_remaining_shots =
                shot_index + 1U < fixture.route.shots.size();
            if (session->state().phase == SessionPhase::Result) {
                if (has_remaining_shots) {
                    result.error = replay_error(shot_pointer(shot_index + 1U),
                        "route terminated before the remaining frozen shot");
                    return result;
                }
            } else if (session->state().phase == SessionPhase::Inspection) {
                if (!has_remaining_shots) {
                    result.error = replay_error("/expected/outcome",
                        "frozen route ended before a terminal result");
                    return result;
                }
            } else {
                result.error = replay_error(shot_pointer(shot_index),
                    "shot resolution did not publish a replayable phase");
                return result;
            }
        }

        if (session->state().phase != SessionPhase::Result
            || session->state().outcome == Outcome::None) {
            result.error = replay_error("/expected/outcome",
                "frozen route is not terminal");
            return result;
        }
        const ScorePresentationState score = session->score_state();
        result.value.actual.outcome = session->state().outcome;
        result.value.actual.score = score.score;
        result.value.actual.stars = score.stars;
        result.value.actual.birds_remaining = session->birds_remaining();
        result.value.actual.objectives_complete = session->objectives_complete();
        result.value.actual.ordered_events_v3 = ordered_events_v3(events);
        result.value.actual.canonical_state_v3 = session->canonical_state_v3();
        result.value.actual.canonical_state_v3_fnv1a64 =
            session->canonical_hash_v3();
        if (fnv1a64(result.value.actual.canonical_state_v3)
            != result.value.actual.canonical_state_v3_fnv1a64) {
            result.error = replay_error("/expected/canonical_state_v3_fnv1a64",
                "session canonical state hash is internally inconsistent");
            return result;
        }
        result.value.actual.canonical_playthrough_v5 =
            canonical_playthrough_v5(fixture.route,
                result.value.actual.ordered_events_v3,
                result.value.actual.canonical_state_v3);
        result.value.actual.canonical_playthrough_v5_fnv1a64 =
            fnv1a64(result.value.actual.canonical_playthrough_v5);
        if (const auto mismatch = compare_replay(
                result.value.actual, fixture.expected)) {
            result.error = *mismatch;
        }
        return result;
    } catch (const std::bad_alloc&) {
        result.error = {ContentErrorCode::ResourceLimit, "/replay",
            "frozen replay allocation failed"};
    } catch (const std::exception& error) {
        result.error = {ContentErrorCode::InternalError, "/replay", error.what()};
    } catch (...) {
        result.error = {ContentErrorCode::InternalError, "/replay",
            "unknown frozen replay failure"};
    }
    return result;
}

}
