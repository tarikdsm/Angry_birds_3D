#include "test_framework.hpp"
#include "session_test_facade.hpp"
#include "ninho/simulation/commands.hpp"
#include "ninho/simulation/session.hpp"

#include <algorithm>
#include <array>
#include <memory>
#include <ranges>
#include <vector>

namespace {
using namespace ninho::physics;
using namespace ninho::simulation;
using ninho::simulation::detail::SessionTestFacade;

MaterialCatalog materials()
{
    MaterialCatalog result;
    result.schema_version = result.source_schema_version = 2;
    result.surfaces.push_back({SurfaceId{1001}, "black", 1000, 0.4, 0.1});
    result.materials.push_back({MaterialId{1}, "pine",
        MaterialResponse::Fibrous, 500, 0.5, 0, 0.5});
    return result;
}

ArchetypeCatalog archetypes()
{
    ArchetypeCatalog result;
    result.schema_version = result.source_schema_version = 2;
    result.presentation_ids = {"black_bird", "black_icon", "black_animation"};
    result.score_ids = {"black_score"};
    AbilityArchetype ability;
    ability.id = AbilityId{1};
    ability.key = ability.kind = "explosion";
    ability.kind_v2 = AbilityKind::Explosion;
    ability.payload = ExplosionAbilityDefinition{4.0, 1000.0, 10000.0, 32};
    result.abilities.push_back(ability);
    BirdArchetype bird;
    bird.id = BirdArchetypeId{1};
    bird.key = "black";
    bird.ability_id = AbilityId{1};
    bird.surface_id = SurfaceId{1001};
    bird.mass_kg = 8.0;
    bird.radius_m = 0.3;
    bird.friction = 0.4;
    bird.restitution = 0.1;
    bird.bullet = true;
    bird.projectile_visual_id = "black_bird";
    bird.launch_speed_cap_m_s = 45.0;
    bird.score_id = "black_score";
    bird.icon_id = "black_icon";
    bird.animation_id = "black_animation";
    result.birds.push_back(bird);
    return result;
}

LevelManifest level(bool with_trigger = false, std::uint32_t trigger_fuse_ticks = 0U)
{
    LevelManifest result;
    result.schema_version = result.source_schema_version = 2;
    result.id = "explosion_ability";
    result.world_id = "earth";
    result.world = UniformWorldDefinition{.acceleration_m_s2 = {0, -9.81, 0},
        .bounds_min_m = {-1000, -1000, -1000},
        .bounds_max_m = {1000, 1000, 1000}};
    result.slingshot = {.asset_id = "launcher",
        .rest_position_m = {-4, 20, 0}, .rest_rotation_xyzw = {0, 0, 0, 1},
        .spring_constant_n_m = 520, .energy_efficiency = 0.9,
        .minimum_extension_m = 0.2, .maximum_extension_m = 4.25,
        .plane_policy = "gravity_vertical_camera_yaw",
        .projectile_clearance_m = 0, .speed_ceiling_m_s = 45};
    result.bird_queue = {BirdArchetypeId{1}};
    result.settle_policy = {0.001, 0.001, 3};
    result.watchdog_ticks = 20;
    if (with_trigger) {
        BodyDefinition target;
        target.body_id = 1;
        target.entity_id = EntityId{50};
        target.part_id = PartId{1};
        target.body_type = ninho::simulation::BodyType::Static;
        target.material_id = MaterialId{1};
        target.density_kg_m3 = 500;
        target.transform.position_m = {5, 5, 0};
        target.transform.rotation_xyzw = {0, 0, 0, 1};
        target.shape = {.type = ShapeType::Box,
            .half_extents_m = {0.5, 0.5, 0.5}};
        target.visual = {.asset_id = "black_bird", .bounds_m = {1, 1, 1}};
        result.bodies.push_back(target);
        result.free_body_ids.push_back(1);
        result.triggers.push_back({.id = 7, .target_entity_id = EntityId{50},
            .damage_threshold = 10, .fuse_ticks = trigger_fuse_ticks,
            .cooldown_ticks = 3,
            .pressure_burst = {4, 100, 100, false, 16}});
    }
    return result;
}

std::unique_ptr<SimulationSession> create_session()
{
    auto created = SimulationSession::create(materials(), archetypes(), level());
    NINHO_SIM_REQUIRE(created.ok());
    return std::move(created.value);
}

std::unique_ptr<SimulationSession> create_environment_session()
{
    auto created = SimulationSession::create(materials(), archetypes(), level(true));
    if (!created.ok()) {
        ninho::simulation::test::fail(__FILE__, __LINE__,
            created.error.pointer + ": " + created.error.message);
    }
    return std::move(created.value);
}

void launch(SimulationSession& session, bool activate_same_tick = false)
{
    NINHO_SIM_REQUIRE(session.enqueue(BeginGrabCommand{{1, 0, 0}}).ok());
    NINHO_SIM_REQUIRE(session.enqueue(SetPullCommand{-1.25, 0.5}).ok());
    NINHO_SIM_REQUIRE(session.tick().ok());
    NINHO_SIM_REQUIRE(session.enqueue(ReleaseBirdCommand{}).ok());
    if (activate_same_tick) {
        NINHO_SIM_REQUIRE(session.enqueue(ActivateAbilityCommand{}).ok());
    }
    NINHO_SIM_REQUIRE(session.tick().ok());
}

void advance_to(SimulationSession& session, TickIndex tick)
{
    while (session.state().tick < tick) {
        NINHO_SIM_REQUIRE(session.tick().ok());
    }
}

std::size_t count_kind(std::span<const DomainEvent> events, DomainEventKind kind)
{
    return static_cast<std::size_t>(std::ranges::count(events, kind, &DomainEvent::kind));
}

template <typename Mutation>
void require_canonical_mutation(SimulationSession& session, Mutation mutation)
{
    const auto bytes_before = session.canonical_state_v3();
    const auto hash_before = session.canonical_hash_v3();
    NINHO_SIM_REQUIRE(mutation(session));
    SessionTestFacade::refresh_canonical_state(session);
    NINHO_SIM_REQUIRE(session.canonical_state_v3() != bytes_before);
    NINHO_SIM_REQUIRE(session.canonical_hash_v3() != hash_before);
}

std::unique_ptr<SimulationSession> create_armed_trigger_session()
{
    auto session = create_environment_session();
    NINHO_SIM_REQUIRE(SessionTestFacade::queue_external_damage(
        *session, EntityId{50}, PartId{1}, 10.0, EventId{900}));
    NINHO_SIM_REQUIRE(session->tick().ok());
    return session;
}

std::unique_ptr<SimulationSession> create_detonated_explosion_session()
{
    auto session = create_session();
    launch(*session);
    const TickIndex launch_tick = SessionTestFacade::projectile_launch_tick(*session);
    advance_to(*session, TickIndex{launch_tick.value() + 9});
    NINHO_SIM_REQUIRE(session->enqueue(ActivateAbilityCommand{}).ok());
    NINHO_SIM_REQUIRE(session->tick().ok());
    return session;
}

NINHO_SIM_TEST("explosion ability is concrete arms at launch plus nine and emits append only burst")
{
    static_assert(static_cast<std::uint8_t>(DomainEventKind::ExplosionFuseArmed) == 17);
    static_assert(static_cast<std::uint8_t>(DomainEventKind::PressureBurst) == 18);
    static_assert(static_cast<std::uint8_t>(DomainEventKind::EnvironmentalTriggerArmed) == 19);
    static_assert(static_cast<std::uint8_t>(DomainEventKind::EnvironmentalTriggerDetonated) == 20);
    auto session = create_session();
    launch(*session, true);
    const TickIndex launch_tick = SessionTestFacade::projectile_launch_tick(*session);
    std::vector<DomainEvent> history;
    history.insert(history.end(), session->events().begin(), session->events().end());
    for (std::uint32_t offset = 1; offset <= 8; ++offset) {
        NINHO_SIM_REQUIRE(session->enqueue(ActivateAbilityCommand{}).ok());
        NINHO_SIM_REQUIRE(session->tick().ok());
        NINHO_SIM_REQUIRE(session->state().tick
            == TickIndex{launch_tick.value() + offset});
        history.insert(history.end(), session->events().begin(), session->events().end());
    }
    NINHO_SIM_REQUIRE(count_kind(history, DomainEventKind::CommandRejected) == 9U);
    NINHO_SIM_REQUIRE(session->enqueue(ActivateAbilityCommand{}).ok());
    NINHO_SIM_REQUIRE(session->tick().ok());
    NINHO_SIM_REQUIRE(count_kind(session->events(), DomainEventKind::AbilityStarted) == 1U);
    NINHO_SIM_REQUIRE(count_kind(session->events(), DomainEventKind::PressureBurst) == 1U);
    NINHO_SIM_REQUIRE(session->enqueue(ActivateAbilityCommand{}).ok());
    NINHO_SIM_REQUIRE(session->tick().ok());
    NINHO_SIM_REQUIRE(count_kind(session->events(), DomainEventKind::PressureBurst) == 0U);
}

NINHO_SIM_TEST("explosion ability passive contact uses exact thresholds and detonates at C plus 39")
{
    auto session = create_session();
    launch(*session);
    const TickIndex launch_tick = SessionTestFacade::projectile_launch_tick(*session);
    advance_to(*session, TickIndex{launch_tick.value() + 3});
    SessionTestFacade::inject_explosion_contact(*session, 2.99999f, 249.99999f);
    NINHO_SIM_REQUIRE(session->tick().ok());
    NINHO_SIM_REQUIRE(count_kind(session->events(), DomainEventKind::ExplosionFuseArmed) == 0U);

    SessionTestFacade::inject_explosion_contact(*session, 3.0f, 0.0f);
    NINHO_SIM_REQUIRE(session->tick().ok());
    const TickIndex contact_tick = session->state().tick;
    NINHO_SIM_REQUIRE(count_kind(session->events(), DomainEventKind::ExplosionFuseArmed) == 1U);
    for (std::uint32_t elapsed = 1; elapsed < 39; ++elapsed) {
        NINHO_SIM_REQUIRE(session->tick().ok());
        NINHO_SIM_REQUIRE(count_kind(session->events(), DomainEventKind::PressureBurst) == 0U);
        NINHO_SIM_REQUIRE(session->state().phase != SessionPhase::Evaluation);
        NINHO_SIM_REQUIRE(session->state().phase != SessionPhase::Result);
    }
    NINHO_SIM_REQUIRE(session->tick().ok());
    NINHO_SIM_REQUIRE(session->state().tick == TickIndex{contact_tick.value() + 39});
    NINHO_SIM_REQUIRE(count_kind(session->events(), DomainEventKind::PressureBurst) == 1U);
}

NINHO_SIM_TEST("environmental trigger session publishes direct causal chain and fuse zero next prestep")
{
    auto session = create_environment_session();
    NINHO_SIM_REQUIRE(SessionTestFacade::queue_external_damage(
        *session, EntityId{50}, PartId{1}, 10.0, EventId{900}));
    NINHO_SIM_REQUIRE(session->tick().ok());
    const auto damage = std::ranges::find(
        session->events(), DomainEventKind::DamageApplied, &DomainEvent::kind);
    const auto armed = std::ranges::find(session->events(),
        DomainEventKind::EnvironmentalTriggerArmed, &DomainEvent::kind);
    NINHO_SIM_REQUIRE(damage != session->events().end());
    NINHO_SIM_REQUIRE(armed != session->events().end());
    NINHO_SIM_REQUIRE(armed->cause_event_id == damage->id);
    NINHO_SIM_REQUIRE(armed->environmental_trigger_id == 7U);
    const EventId armed_id = armed->id;

    NINHO_SIM_REQUIRE(session->tick().ok());
    const auto detonated = std::ranges::find(session->events(),
        DomainEventKind::EnvironmentalTriggerDetonated, &DomainEvent::kind);
    const auto burst = std::ranges::find(
        session->events(), DomainEventKind::PressureBurst, &DomainEvent::kind);
    NINHO_SIM_REQUIRE(detonated != session->events().end());
    NINHO_SIM_REQUIRE(burst != session->events().end());
    NINHO_SIM_REQUIRE(detonated->cause_event_id == armed_id);
    NINHO_SIM_REQUIRE(burst->cause_event_id == detonated->id);
    NINHO_SIM_REQUIRE(burst->environmental_trigger_id == 7U);
}

NINHO_SIM_TEST("environmental trigger pending fuse blocks watchdog evaluation")
{
    auto created = SimulationSession::create(
        materials(), archetypes(), level(true, 30U));
    NINHO_SIM_REQUIRE(created.ok());
    auto session = std::move(created.value);
    launch(*session);
    NINHO_SIM_REQUIRE(SessionTestFacade::queue_external_damage(
        *session, EntityId{50}, PartId{1}, 10.0, EventId{901}));
    NINHO_SIM_REQUIRE(session->tick().ok());
    const TickIndex armed_tick = session->state().tick;

    advance_to(*session, TickIndex{armed_tick.value() + 25U});
    NINHO_SIM_REQUIRE(session->state().phase != SessionPhase::Evaluation);
    NINHO_SIM_REQUIRE(session->state().phase != SessionPhase::Result);
    NINHO_SIM_REQUIRE(count_kind(
        session->events(), DomainEventKind::PressureBurst) == 0U);
}

NINHO_SIM_TEST("explosion ability manual activation on fuse due tick detonates exactly once")
{
    auto session = create_session();
    launch(*session);
    const TickIndex launch_tick = SessionTestFacade::projectile_launch_tick(*session);
    advance_to(*session, TickIndex{launch_tick.value() + 9});
    SessionTestFacade::inject_explosion_contact(*session, 0.0f, 250.0f);
    NINHO_SIM_REQUIRE(session->tick().ok());
    const TickIndex contact_tick = session->state().tick;
    advance_to(*session, TickIndex{contact_tick.value() + 38});
    NINHO_SIM_REQUIRE(session->enqueue(ActivateAbilityCommand{}).ok());
    NINHO_SIM_REQUIRE(session->tick().ok());
    NINHO_SIM_REQUIRE(session->state().tick == TickIndex{contact_tick.value() + 39});
    NINHO_SIM_REQUIRE(count_kind(session->events(), DomainEventKind::PressureBurst) == 1U);
}

NINHO_SIM_TEST("explosion ability manual activation anticipates an armed fuse")
{
    auto session = create_session();
    launch(*session);
    const TickIndex launch_tick = SessionTestFacade::projectile_launch_tick(*session);
    advance_to(*session, TickIndex{launch_tick.value() + 9});
    SessionTestFacade::inject_explosion_contact(*session, 3.0F, 0.0F);
    NINHO_SIM_REQUIRE(session->tick().ok());
    const TickIndex contact_tick = session->state().tick;
    NINHO_SIM_REQUIRE(session->enqueue(ActivateAbilityCommand{}).ok());
    NINHO_SIM_REQUIRE(session->tick().ok());
    NINHO_SIM_REQUIRE(session->state().tick
        == TickIndex{contact_tick.value() + 1U});
    NINHO_SIM_REQUIRE(count_kind(
        session->events(), DomainEventKind::PressureBurst) == 1U);
}

NINHO_SIM_TEST("explosion ability fifty repetitions preserve events bytes and hash")
{
    struct Signature {
        std::vector<DomainEvent> events;
        std::vector<std::uint8_t> bytes;
        std::uint64_t hash{};
        bool operator==(const Signature&) const = default;
    };
    const auto run = [] {
        auto session = create_session();
        launch(*session);
        const TickIndex launch_tick =
            SessionTestFacade::projectile_launch_tick(*session);
        advance_to(*session, TickIndex{launch_tick.value() + 9});
        NINHO_SIM_REQUIRE(session->enqueue(ActivateAbilityCommand{}).ok());
        NINHO_SIM_REQUIRE(session->tick().ok());
        return Signature{
            {session->events().begin(), session->events().end()},
            session->canonical_state_v3(), session->canonical_hash_v3()};
    };
    const Signature baseline = run();
    for (std::size_t repetition = 1; repetition < 50U; ++repetition) {
        NINHO_SIM_REQUIRE(run() == baseline);
    }
}

NINHO_SIM_TEST("explosion ability late burst failure rolls back all staged domain state")
{
    auto session = create_session();
    launch(*session);
    const TickIndex launch_tick = SessionTestFacade::projectile_launch_tick(*session);
    advance_to(*session, TickIndex{launch_tick.value() + 9});
    const auto shot_before = session->shot_state();
    const std::vector<DomainEvent> events_before{
        session->events().begin(), session->events().end()};

    SessionTestFacade::fail_next_pressure_burst_after_plan(*session);
    NINHO_SIM_REQUIRE(session->enqueue(ActivateAbilityCommand{}).ok());
    const auto canonical_before = session->canonical_state_v3();
    const SessionStatus status = session->tick();

    NINHO_SIM_REQUIRE(!status.ok());
    const auto shot_after = session->shot_state();
    NINHO_SIM_REQUIRE(shot_after.has_value());
    NINHO_SIM_REQUIRE(shot_before.has_value());
    NINHO_SIM_REQUIRE(shot_after->shot_id == shot_before->shot_id);
    NINHO_SIM_REQUIRE(shot_after->projectile_ids == shot_before->projectile_ids);
    NINHO_SIM_REQUIRE(shot_after->activation_consumed
        == shot_before->activation_consumed);
    NINHO_SIM_REQUIRE(!SessionTestFacade::ability_active(*session));
    NINHO_SIM_REQUIRE(std::ranges::equal(session->events(), events_before));
    NINHO_SIM_REQUIRE(session->canonical_state_v3() == canonical_before);
    NINHO_SIM_REQUIRE(SessionTestFacade::pending_pressure_burst_count(*session) == 0U);
    NINHO_SIM_REQUIRE(SessionTestFacade::pending_external_damage_count(*session) == 0U);
}

NINHO_SIM_TEST("explosion ability typed create and reconfigure share burst body limit atomically")
{
    ArchetypeCatalog invalid_ability = archetypes();
    std::get<ExplosionAbilityDefinition>(
        invalid_ability.abilities.front().payload).max_bodies = 33U;
    const auto rejected_ability = SimulationSession::create(
        materials(), invalid_ability, level());
    NINHO_SIM_REQUIRE(!rejected_ability.ok());
    NINHO_SIM_REQUIRE(rejected_ability.error.code == ContentErrorCode::OutOfRange);
    NINHO_SIM_REQUIRE(rejected_ability.error.pointer
        == "/abilities/0/payload/max_bodies");

    LevelManifest invalid_trigger = level(true);
    invalid_trigger.triggers.front().pressure_burst.max_bodies = 33U;
    const auto rejected_trigger = SimulationSession::create(
        materials(), archetypes(), invalid_trigger);
    NINHO_SIM_REQUIRE(!rejected_trigger.ok());
    NINHO_SIM_REQUIRE(rejected_trigger.error.code == ContentErrorCode::OutOfRange);
    NINHO_SIM_REQUIRE(rejected_trigger.error.pointer
        == "/triggers/0/pressure_burst/max_bodies");

    auto session = create_session();
    launch(*session);
    const auto canonical_before = session->canonical_state_v3();
    const auto state_before = session->state();
    const auto shot_before = session->shot_state();
    const SessionStatus status = session->reconfigure(
        materials(), invalid_ability, level());
    NINHO_SIM_REQUIRE(!status.ok());
    NINHO_SIM_REQUIRE(session->canonical_state_v3() == canonical_before);
    NINHO_SIM_REQUIRE(session->state().tick == state_before.tick);
    NINHO_SIM_REQUIRE(session->state().phase == state_before.phase);
    NINHO_SIM_REQUIRE(session->state().outcome == state_before.outcome);
    NINHO_SIM_REQUIRE(session->shot_state() == shot_before);
}

NINHO_SIM_TEST("explosion ability canonical probes isolate fuse detonation tick and flag")
{
    auto fuse = create_session();
    launch(*fuse);
    SessionTestFacade::inject_explosion_contact(*fuse, 3.0F, 0.0F);
    NINHO_SIM_REQUIRE(fuse->tick().ok());
    require_canonical_mutation(*fuse,
        SessionTestFacade::shift_explosion_fuse_due_tick);

    auto tick = create_detonated_explosion_session();
    require_canonical_mutation(*tick,
        SessionTestFacade::shift_explosion_detonation_tick);

    auto flag = create_detonated_explosion_session();
    require_canonical_mutation(*flag,
        SessionTestFacade::toggle_explosion_detonated);
}

NINHO_SIM_TEST("environmental trigger canonical probes isolate cause armed cooldown damage and origin")
{
    auto cause = create_armed_trigger_session();
    require_canonical_mutation(*cause, [](SimulationSession& session) {
        return SessionTestFacade::bump_trigger_initiating_damage_event(session, 7U);
    });

    auto armed = create_armed_trigger_session();
    require_canonical_mutation(*armed, [](SimulationSession& session) {
        return SessionTestFacade::toggle_trigger_armed(session, 7U);
    });

    auto cooldown = create_armed_trigger_session();
    NINHO_SIM_REQUIRE(cooldown->tick().ok());
    require_canonical_mutation(*cooldown, [](SimulationSession& session) {
        return SessionTestFacade::shift_trigger_cooldown_until_tick(session, 7U);
    });

    auto damage = create_armed_trigger_session();
    require_canonical_mutation(*damage, [](SimulationSession& session) {
        return SessionTestFacade::bump_trigger_accumulated_damage(session, 7U);
    });

    auto origin = create_armed_trigger_session();
    require_canonical_mutation(*origin, [](SimulationSession& session) {
        return SessionTestFacade::shift_trigger_captured_origin(session, 7U);
    });
}

}
