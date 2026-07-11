#include "test_framework.hpp"

#include <ninho/extension/orbital_session_node.hpp>

#include <filesystem>
#include <fstream>
#include <limits>
#include <sstream>
#include <string>
#include <string_view>
#include <type_traits>

namespace {

using ninho::extension::detail::OrbitalSessionAdapter;
using ninho::extension::detail::SessionFrameData;
using ninho::physics::Vec3;

std::string read_source_file(std::string_view relative_path)
{
    std::ifstream stream{std::filesystem::path{NINHO_SOURCE_DIR} / relative_path,
        std::ios::binary};
    NINHO_REQUIRE(stream.is_open());
    std::ostringstream content;
    content << stream.rdbuf();
    return content.str();
}

struct JsonFixture {
    std::string materials = read_source_file(
        "game/data/materials/vertical_slice.materials.json");
    std::string archetypes = read_source_file(
        "game/data/archetypes/vertical_slice.archetypes.json");
    std::string level = read_source_file(
        "game/data/levels/first_orbit.level.json");
};

static_assert(noexcept(std::declval<OrbitalSessionAdapter&>().configure(
    std::declval<std::string_view>(), std::declval<std::string_view>(),
    std::declval<std::string_view>())));
static_assert(noexcept(std::declval<OrbitalSessionAdapter&>().advance(0.0)));
static_assert(noexcept(std::declval<OrbitalSessionAdapter&>().consume_frame()));

NINHO_TEST("orbital adapter configures queues all commands advances and restarts")
{
    const JsonFixture json;
    OrbitalSessionAdapter adapter;
    NINHO_REQUIRE(adapter.configure(json.materials, json.archetypes, json.level));
    NINHO_REQUIRE(adapter.configured());
    NINHO_REQUIRE(!adapter.fault().has_value());

    NINHO_REQUIRE(adapter.queue_begin_aim());
    NINHO_REQUIRE(adapter.queue_aim(
        Vec3{-13.0004F, 0.0004F, 0.0F}, Vec3{0.0F, 1.00004F, 0.00004F}, 10.5));
    NINHO_REQUIRE(adapter.advance(1.0 / 60.0));
    NINHO_REQUIRE(adapter.queue_launch());
    NINHO_REQUIRE(adapter.advance(1.0 / 60.0));
    NINHO_REQUIRE(adapter.queue_activate_ability());
    NINHO_REQUIRE(adapter.advance(1.0 / 60.0));

    const SessionFrameData launched = adapter.consume_frame();
    NINHO_REQUIRE(launched.ticks_executed == 3);
    NINHO_REQUIRE(!launched.events.empty());
    NINHO_REQUIRE(!launched.snapshots.empty());
    NINHO_REQUIRE(launched.preview.has_value());

    NINHO_REQUIRE(adapter.restart());
    const SessionFrameData restarted = adapter.consume_frame();
    NINHO_REQUIRE(restarted.state.tick.value() == 0);
    NINHO_REQUIRE(restarted.events.empty());
}

NINHO_TEST("orbital adapter rolls back invalid configuration latches fault and recovers")
{
    const JsonFixture json;
    OrbitalSessionAdapter adapter;
    NINHO_REQUIRE(adapter.configure(json.materials, json.archetypes, json.level));
    const auto initial = adapter.consume_frame();
    NINHO_REQUIRE(!initial.snapshots.empty());

    NINHO_REQUIRE(!adapter.configure("{", json.archetypes, json.level));
    NINHO_REQUIRE(adapter.configured());
    NINHO_REQUIRE(adapter.fault().has_value());
    const auto fault = *adapter.fault();
    NINHO_REQUIRE(fault.code() == "invalid_json");
    NINHO_REQUIRE(!adapter.advance(1.0 / 60.0));
    NINHO_REQUIRE(adapter.consume_frame().state.tick.value() == 0);

    NINHO_REQUIRE(adapter.restart());
    NINHO_REQUIRE(!adapter.fault().has_value());
    NINHO_REQUIRE(adapter.advance(1.0 / 60.0));

    NINHO_REQUIRE(!adapter.queue_aim(
        Vec3{std::numeric_limits<float>::infinity(), 0.0F, 0.0F},
        Vec3{0.0F, 1.0F, 0.0F}, 10.5));
    NINHO_REQUIRE(adapter.fault().has_value());
    NINHO_REQUIRE(adapter.configure(json.materials, json.archetypes, json.level));
    NINHO_REQUIRE(!adapter.fault().has_value());
}

NINHO_TEST("orbital frame consume clears pending events while retaining latest snapshots")
{
    const JsonFixture json;
    OrbitalSessionAdapter adapter;
    NINHO_REQUIRE(adapter.configure(json.materials, json.archetypes, json.level));
    NINHO_REQUIRE(adapter.queue_begin_aim());
    NINHO_REQUIRE(adapter.advance(1.0 / 60.0));
    NINHO_REQUIRE(adapter.queue_aim(
        Vec3{-13.0F, 0.0F, 0.0F}, Vec3{0.0F, 1.0F, 0.0F}, 10.5));
    NINHO_REQUIRE(adapter.queue_launch());
    NINHO_REQUIRE(adapter.advance(1.0 / 60.0));

    const auto first = adapter.consume_frame();
    NINHO_REQUIRE(!first.events.empty());
    const auto second = adapter.consume_frame();
    NINHO_REQUIRE(second.events.empty());
    NINHO_REQUIRE(second.snapshots == first.snapshots);
    NINHO_REQUIRE(second.state.tick == first.state.tick);
    NINHO_REQUIRE(second.state.phase == first.state.phase);
    NINHO_REQUIRE(second.state.outcome == first.state.outcome);
}

NINHO_TEST("orbital adapter acknowledges a frame only after the consumer commits")
{
    const JsonFixture json;
    OrbitalSessionAdapter adapter;
    NINHO_REQUIRE(adapter.configure(json.materials, json.archetypes, json.level));
    NINHO_REQUIRE(adapter.queue_begin_aim());
    NINHO_REQUIRE(adapter.advance(1.0 / 60.0));
    NINHO_REQUIRE(adapter.queue_aim(
        Vec3{-13.0F, 0.0F, 0.0F}, Vec3{0.0F, 1.0F, 0.0F}, 10.5));
    NINHO_REQUIRE(adapter.queue_launch());
    NINHO_REQUIRE(adapter.advance(1.0 / 60.0));

    const SessionFrameData& first_peek = adapter.peek_frame();
    NINHO_REQUIRE(!first_peek.events.empty());
    NINHO_REQUIRE(!adapter.peek_frame().events.empty());
    adapter.acknowledge_frame();
    NINHO_REQUIRE(adapter.peek_frame().events.empty());
    NINHO_REQUIRE(!adapter.peek_frame().snapshots.empty());
}

}
