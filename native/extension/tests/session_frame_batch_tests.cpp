#include "test_framework.hpp"

#include <ninho/extension/orbital_session_node.hpp>

#include "session_test_facade.hpp"

#include <array>
#include <cmath>
#include <limits>
#include <ranges>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>

namespace {

using namespace ninho::extension::detail;
using namespace ninho::simulation;

std::string read_source_file(std::string_view relative_path)
{
    std::ifstream stream{std::filesystem::path{NINHO_SOURCE_DIR} / relative_path,
        std::ios::binary};
    NINHO_REQUIRE(stream.is_open());
    std::ostringstream content;
    content << stream.rdbuf();
    return content.str();
}

std::unique_ptr<SimulationSession> create_session()
{
    const auto materials = parse_material_catalog(read_source_file(
        "game/data/materials/vertical_slice.materials.json"));
    const auto archetypes = parse_archetype_catalog(read_source_file(
        "game/data/archetypes/vertical_slice.archetypes.json"));
    const auto level = parse_level_manifest(read_source_file(
        "game/data/levels/first_orbit.level.json"));
    NINHO_REQUIRE(materials && archetypes && level);
    auto result = SimulationSession::create(materials.value, archetypes.value, level.value);
    NINHO_REQUIRE(result);
    return std::move(result.value);
}

NINHO_TEST("orbital accumulator runs at sixty hertz caps four ticks and measures discarded time")
{
    SessionFixedStepAccumulator accumulator;

    const SessionTickSchedule half = accumulator.schedule(1.0 / 120.0);
    NINHO_REQUIRE(half.ok);
    NINHO_REQUIRE(half.tick_count == 0);
    NINHO_REQUIRE_NEAR(half.discarded_seconds, 0.0, 1.0e-12);

    const SessionTickSchedule whole = accumulator.schedule(1.0 / 120.0);
    NINHO_REQUIRE(whole.ok);
    NINHO_REQUIRE(whole.tick_count == 1);

    const SessionTickSchedule overloaded = accumulator.schedule(10.0 / 60.0);
    NINHO_REQUIRE(overloaded.ok);
    NINHO_REQUIRE(overloaded.tick_count == 4);
    NINHO_REQUIRE_NEAR(overloaded.discarded_seconds, 6.0 / 60.0, 1.0e-12);

    const SessionTickSchedule enormous =
        accumulator.schedule(std::numeric_limits<double>::max());
    NINHO_REQUIRE(enormous.ok);
    NINHO_REQUIRE(enormous.tick_count == 4);
    NINHO_REQUIRE(std::isfinite(enormous.discarded_seconds));
    NINHO_REQUIRE(enormous.discarded_seconds > 0.0);
}

NINHO_TEST("orbital accumulator rejects invalid delta and resets pending time")
{
    SessionFixedStepAccumulator accumulator;
    NINHO_REQUIRE(accumulator.schedule(1.0 / 120.0).ok);
    NINHO_REQUIRE(!accumulator.schedule(-1.0).ok);
    NINHO_REQUIRE(accumulator.schedule(1.0 / 120.0).tick_count == 0);
    NINHO_REQUIRE(!accumulator.schedule(std::numeric_limits<double>::infinity()).ok);
}

NINHO_TEST("session frame batch preserves every event and only the latest state snapshot and preview")
{
    SessionFrameBatch batch;
    DomainEvent first{.id = EventId{1}, .tick = TickIndex{3},
        .kind = DomainEventKind::BirdLaunched};
    DomainEvent second{.id = EventId{2}, .tick = TickIndex{4},
        .kind = DomainEventKind::AbilityStarted};
    EntitySnapshot old_snapshot{.entity_id = EntityId{10}, .part_id = PartId{1}};
    EntitySnapshot latest_snapshot{.entity_id = EntityId{20}, .part_id = PartId{2}};
    TrajectoryPreview old_preview{.canonical_hash = 11};
    TrajectoryPreview latest_preview{.canonical_hash = 22};

    const std::array first_events{first};
    const std::array first_snapshots{old_snapshot};
    batch.capture_tick(first_events, first_snapshots,
        SessionState{.tick = TickIndex{3}, .phase = SessionPhase::Aim},
        3, false, {}, old_preview);

    const std::array second_events{second};
    const std::array second_snapshots{latest_snapshot};
    batch.capture_tick(second_events, second_snapshots,
        SessionState{.tick = TickIndex{4}, .phase = SessionPhase::Aim},
        2, true, {}, latest_preview);

    const SessionFrameData frame = batch.consume();
    NINHO_REQUIRE(frame.ticks_executed == 2);
    NINHO_REQUIRE(frame.events == std::vector<DomainEvent>({first, second}));
    NINHO_REQUIRE(frame.snapshots == std::vector<EntitySnapshot>({latest_snapshot}));
    NINHO_REQUIRE(frame.state.tick == TickIndex{4});
    NINHO_REQUIRE(frame.birds_remaining == 2);
    NINHO_REQUIRE(frame.objectives_complete);
    NINHO_REQUIRE(frame.preview.has_value());
    NINHO_REQUIRE(frame.preview->canonical_hash == 22);

    const SessionFrameData after_consume = batch.consume();
    NINHO_REQUIRE(after_consume.events.empty());
    NINHO_REQUIRE(after_consume.ticks_executed == 0);
    NINHO_REQUIRE(after_consume.snapshots == frame.snapshots);
    NINHO_REQUIRE(after_consume.state.tick == frame.state.tick);
    NINHO_REQUIRE(after_consume.state.phase == frame.state.phase);
    NINHO_REQUIRE(after_consume.state.outcome == frame.state.outcome);
    NINHO_REQUIRE(after_consume.preview.has_value());
    NINHO_REQUIRE(after_consume.preview->canonical_hash == frame.preview->canonical_hash);
}

NINHO_TEST("session frame batch clears trajectory preview after cancel launch and result")
{
    SessionFrameBatch batch;
    TrajectoryPreview preview{.canonical_hash = 37};

    batch.capture_latest({}, SessionState{.phase = SessionPhase::Aim}, 3, false, {});
    batch.set_preview(preview);
    NINHO_REQUIRE(batch.peek().preview.has_value());

    batch.capture_latest(
        {}, SessionState{.phase = SessionPhase::Inspection}, 3, false, {});
    NINHO_REQUIRE(!batch.peek().preview.has_value());

    batch.set_preview(preview);
    NINHO_REQUIRE(batch.peek().preview.has_value());
    batch.capture_latest(
        {}, SessionState{.phase = SessionPhase::Aim}, 3, false, {});
    NINHO_REQUIRE(batch.peek().preview.has_value());

    batch.capture_latest(
        {}, SessionState{.phase = SessionPhase::FlightAbility}, 2, false, {});
    NINHO_REQUIRE(!batch.peek().preview.has_value());

    batch.set_preview(preview);
    batch.capture_latest(
        {}, SessionState{.phase = SessionPhase::Result}, 0, true, {});
    NINHO_REQUIRE(!batch.peek().preview.has_value());
}

NINHO_TEST("session frame batch latches first fault until explicitly cleared")
{
    SessionFrameBatch batch;
    batch.latch_fault({"first", "first failure"});
    batch.latch_fault({"second", "must not replace the first"});
    NINHO_REQUIRE(batch.fault().has_value());
    NINHO_REQUIRE(batch.fault()->code() == "first");
    batch.clear_fault();
    NINHO_REQUIRE(!batch.fault().has_value());
}

NINHO_TEST("session frame capture preserves events from a tick that ends faulted")
{
    auto session = create_session();
    NINHO_REQUIRE(session->enqueue(BeginAimCommand{}).ok());
    NINHO_REQUIRE(session->tick().ok());
    NINHO_REQUIRE(session->enqueue(SetAimCommand{AimState{
        {-13.0F, 0.0F, 0.0F}, {0.0F, 1.0F, 0.0F}, 10.5}}).ok());
    NINHO_REQUIRE(session->enqueue(LaunchCommand{}).ok());
    detail::SessionTestFacade::fail_next_canonical_refresh(
        *session, "expected adapter capture failure");

    const SessionStatus status = session->tick();
    NINHO_REQUIRE(!status.ok());
    SessionFrameBatch batch;
    capture_session_tick(batch, *session);
    const SessionFrameData frame = batch.consume();
    NINHO_REQUIRE(std::ranges::any_of(frame.events, [](const DomainEvent& event) {
        return event.kind == DomainEventKind::BirdLaunched;
    }));
    NINHO_REQUIRE(frame.state.phase == SessionPhase::Faulted);
}

}
