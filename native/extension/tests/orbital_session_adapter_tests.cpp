#include "test_framework.hpp"

#include <ninho/extension/orbital_session_node.hpp>

#include <algorithm>
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

void replace_once(std::string& value, std::string_view from, std::string_view to)
{
    const std::size_t position = value.find(from);
    NINHO_REQUIRE(position != std::string::npos);
    value.replace(position, from.size(), to);
}

static_assert(noexcept(std::declval<OrbitalSessionAdapter&>().configure(
    std::declval<std::string_view>(), std::declval<std::string_view>(),
    std::declval<std::string_view>())));
static_assert(noexcept(std::declval<OrbitalSessionAdapter&>().advance(0.0)));
static_assert(noexcept(std::declval<OrbitalSessionAdapter&>().consume_frame()));

NINHO_TEST("orbital adapter publishes the configured manifest aim envelope")
{
    JsonFixture json;
    replace_once(json.level, "\"shell_offset_m\": 3.0", "\"shell_offset_m\": 6.0");
    replace_once(json.level, "\"theta_min_deg\": -50.0", "\"theta_min_deg\": -35.0");
    replace_once(json.level, "\"theta_max_deg\": 50.0", "\"theta_max_deg\": 60.0");
    replace_once(json.level, "\"phase_speed_min_m_s\": 8.0", "\"phase_speed_min_m_s\": 9.0");
    replace_once(json.level, "\"phase_speed_max_m_s\": 16.0", "\"phase_speed_max_m_s\": 18.0");
    replace_once(json.level, "\"default_speed_m_s\": 10.5", "\"default_speed_m_s\": 12.5");

    OrbitalSessionAdapter adapter;
    NINHO_REQUIRE(adapter.configure(json.materials, json.archetypes, json.level));

    const SessionFrameData frame = adapter.consume_frame();
    NINHO_REQUIRE_NEAR(frame.aim_envelope.shell_radius_m, 16.0, 1.0e-9);
    NINHO_REQUIRE_NEAR(frame.aim_envelope.theta_min_deg, -35.0, 1.0e-9);
    NINHO_REQUIRE_NEAR(frame.aim_envelope.theta_max_deg, 60.0, 1.0e-9);
    NINHO_REQUIRE_NEAR(frame.aim_envelope.speed_min_m_s, 9.0, 1.0e-9);
    NINHO_REQUIRE_NEAR(frame.aim_envelope.speed_max_m_s, 18.0, 1.0e-9);
    NINHO_REQUIRE_NEAR(frame.aim_envelope.default_speed_m_s, 12.5, 1.0e-9);
}

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
    NINHO_REQUIRE(std::ranges::count(
                      launched.snapshots, true, &ninho::simulation::EntitySnapshot::is_projectile)
        == 1);
    NINHO_REQUIRE(!launched.preview.has_value());
    NINHO_REQUIRE(launched.objective_targets.size() == 1U);
    NINHO_REQUIRE(launched.objective_targets.front().entity_id.value() == 200U);
    NINHO_REQUIRE_NEAR(launched.objective_targets.front().current_integrity, 100.0, 1.0e-9);
    NINHO_REQUIRE_NEAR(launched.objective_targets.front().maximum_integrity, 100.0, 1.0e-9);
    NINHO_REQUIRE(
        launched.ability_readiness == ninho::simulation::AbilityReadiness::Arming);

    NINHO_REQUIRE(adapter.restart());
    const SessionFrameData restarted = adapter.consume_frame();
    NINHO_REQUIRE(restarted.state.tick.value() == 0);
    NINHO_REQUIRE(restarted.events.empty());
    NINHO_REQUIRE(restarted.objective_targets.size() == 1U);
    NINHO_REQUIRE_NEAR(
        restarted.objective_targets.front().current_integrity, 100.0, 1.0e-9);
    NINHO_REQUIRE(
        restarted.ability_readiness == ninho::simulation::AbilityReadiness::Unavailable);
}

NINHO_TEST("orbital adapter catch up retains per tick events and only final frame state")
{
    const JsonFixture json;
    OrbitalSessionAdapter adapter;
    NINHO_REQUIRE(adapter.configure(json.materials, json.archetypes, json.level));
    NINHO_REQUIRE(adapter.queue_begin_aim());
    NINHO_REQUIRE(adapter.advance(1.0 / 60.0));
    static_cast<void>(adapter.consume_frame());

    NINHO_REQUIRE(adapter.queue_aim(
        Vec3{-13.0F, 0.0F, 0.0F}, Vec3{0.0F, 1.0F, 0.0F}, 10.5));
    NINHO_REQUIRE(adapter.queue_launch());
    NINHO_REQUIRE(adapter.advance(10.0 / 60.0));

    const SessionFrameData frame = adapter.consume_frame();
    NINHO_REQUIRE(frame.ticks_executed == 4);
    NINHO_REQUIRE(frame.state.tick.value() == 5);
    NINHO_REQUIRE_NEAR(frame.discarded_time_seconds, 6.0 / 60.0, 1.0e-12);
    NINHO_REQUIRE(std::ranges::any_of(frame.events, [](const auto& event) {
        return event.kind == ninho::simulation::DomainEventKind::BirdLaunched;
    }));
    NINHO_REQUIRE(std::ranges::count(
                      frame.snapshots, true,
                      &ninho::simulation::EntitySnapshot::is_projectile)
        == 1);

    const SessionFrameData acknowledged = adapter.consume_frame();
    NINHO_REQUIRE(acknowledged.ticks_executed == 0);
    NINHO_REQUIRE(acknowledged.events.empty());
    NINHO_REQUIRE_NEAR(acknowledged.discarded_time_seconds, 0.0, 1.0e-12);
    NINHO_REQUIRE(acknowledged.snapshots == frame.snapshots);
    NINHO_REQUIRE(acknowledged.state.tick == frame.state.tick);
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
    NINHO_REQUIRE(!adapter.fault().has_value());
    NINHO_REQUIRE(adapter.queue_begin_aim());
    NINHO_REQUIRE(adapter.advance(1.0 / 60.0));
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
