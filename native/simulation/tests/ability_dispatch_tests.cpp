#include "test_framework.hpp"

#include "ability_system.hpp"
#include "session_test_facade.hpp"
#include "ninho/simulation/commands.hpp"
#include "ninho/simulation/session.hpp"

#include <algorithm>
#include <array>
#include <cstdint>
#include <memory>
#include <ranges>
#include <string>
#include <utility>
#include <variant>
#include <vector>

namespace {

using namespace ninho::simulation;
using ninho::simulation::detail::AbilityLifecycleHooks;
using ninho::simulation::detail::AbilitySystem;

struct RecordingHooks final : AbilityLifecycleHooks {
    std::array<std::uint32_t, 5> before{};
    std::array<std::uint32_t, 5> after{};
    std::array<std::uint32_t, 5> finish{};

    SessionStatus apply_before_step(ShotState&, const GravityFieldAbilityDefinition&,
        GravityFieldAbilityRuntime&) override
    {
        ++before[0];
        return {};
    }
    SessionStatus apply_before_step(ShotState&, const MassBoostAbilityDefinition&,
        MassBoostAbilityRuntime&) override
    {
        ++before[1];
        return {};
    }
    SessionStatus apply_before_step(ShotState&, const SpeedBoostAbilityDefinition&,
        SpeedBoostAbilityRuntime&) override
    {
        ++before[2];
        return {};
    }
    SessionStatus apply_before_step(ShotState&, const ExplosionAbilityDefinition&,
        ExplosionAbilityRuntime&) override
    {
        ++before[3];
        return {};
    }
    SessionStatus apply_before_step(ShotState&, const SplitAbilityDefinition&,
        SplitAbilityRuntime&) override
    {
        ++before[4];
        return {};
    }

    SessionStatus process_after_step(ShotState&, const GravityFieldAbilityDefinition&,
        GravityFieldAbilityRuntime&) override
    {
        ++after[0];
        return {};
    }
    SessionStatus process_after_step(ShotState&, const MassBoostAbilityDefinition&,
        MassBoostAbilityRuntime&) override
    {
        ++after[1];
        return {};
    }
    SessionStatus process_after_step(ShotState&, const SpeedBoostAbilityDefinition&,
        SpeedBoostAbilityRuntime&) override
    {
        ++after[2];
        return {};
    }
    SessionStatus process_after_step(ShotState&, const ExplosionAbilityDefinition&,
        ExplosionAbilityRuntime&) override
    {
        ++after[3];
        return {};
    }
    SessionStatus process_after_step(ShotState&, const SplitAbilityDefinition&,
        SplitAbilityRuntime&) override
    {
        ++after[4];
        return {};
    }

    SessionStatus finish_after_step(ShotState&, const GravityFieldAbilityDefinition&,
        GravityFieldAbilityRuntime&) override
    {
        ++finish[0];
        return {};
    }
    SessionStatus finish_after_step(ShotState&, const MassBoostAbilityDefinition&,
        MassBoostAbilityRuntime&) override
    {
        ++finish[1];
        return {};
    }
    SessionStatus finish_after_step(ShotState&, const SpeedBoostAbilityDefinition&,
        SpeedBoostAbilityRuntime&) override
    {
        ++finish[2];
        return {};
    }
    SessionStatus finish_after_step(ShotState&, const ExplosionAbilityDefinition&,
        ExplosionAbilityRuntime&) override
    {
        ++finish[3];
        return {};
    }
    SessionStatus finish_after_step(ShotState&, const SplitAbilityDefinition&,
        SplitAbilityRuntime&) override
    {
        ++finish[4];
        return {};
    }
};

AbilityArchetype ability(AbilityKind kind)
{
    AbilityArchetype result;
    result.id = AbilityId{static_cast<std::uint32_t>(kind) + 1U};
    result.arm_ticks = 9U;
    result.duration_ticks = 75U;
    switch (kind) {
    case AbilityKind::LegacyGravityField:
        result.key = result.kind = "gravity_field";
        result.radius_m = 4.0;
        result.max_body_mass_kg = 150.0;
        result.max_bodies = 20U;
        result.max_acceleration_m_s2 = 12.0;
        result.pulse_speed_m_s = 4.0;
        break;
    case AbilityKind::GravityField:
        result.key = result.kind = "gravity_field";
        result.payload = GravityFieldAbilityDefinition{
            9U, 75U, 4.0, 150.0, 20U, 12.0, 4.0};
        break;
    case AbilityKind::MassBoost:
        result.key = result.kind = "mass_boost";
        result.payload = MassBoostAbilityDefinition{75U, 2.25};
        break;
    case AbilityKind::SpeedBoost:
        result.key = result.kind = "speed_boost";
        result.payload = SpeedBoostAbilityDefinition{12.0};
        break;
    case AbilityKind::Explosion:
        result.key = result.kind = "explosion";
        result.payload = ExplosionAbilityDefinition{4.0, 12.0, 90.0, 32U};
        break;
    case AbilityKind::Split:
        result.key = result.kind = "split";
        result.payload = SplitAbilityDefinition{3U, 11.0, 1.0};
        break;
    }
    result.kind_v2 = kind;
    return result;
}

AbilityRuntime runtime(AbilityKind kind)
{
    AbilityRuntime result = make_ability_runtime(kind);
    set_ability_runtime_active(result, true);
    set_ability_runtime_window(result, TickIndex{10}, TickIndex{20});
    return result;
}

std::unique_ptr<ShotState> shot(AbilityKind kind)
{
    auto result = std::make_unique<ShotState>(
        ProjectileState{EntityId{0x80000000U}, {}, true});
    result->ability_id = AbilityId{static_cast<std::uint32_t>(kind) + 1U};
    result->runtime = runtime(kind);
    return result;
}

MaterialCatalog materials()
{
    MaterialCatalog result;
    result.schema_version = result.source_schema_version = 2U;
    result.surfaces.push_back({SurfaceId{1001}, "bird", 1000.0, 0.4, 0.1});
    return result;
}

ArchetypeCatalog archetypes(AbilityArchetype selected = ability(AbilityKind::GravityField))
{
    ArchetypeCatalog result;
    result.schema_version = result.source_schema_version = 2U;
    result.presentation_ids = {"bird", "icon", "animation"};
    result.score_ids = {"bird_score"};
    selected.id = AbilityId{1};
    result.abilities.push_back(std::move(selected));
    for (std::uint32_t id = 1U; id <= 2U; ++id) {
        BirdArchetype bird;
        bird.id = BirdArchetypeId{id};
        bird.key = "bird_" + std::to_string(id);
        bird.ability_id = AbilityId{1};
        bird.surface_id = SurfaceId{1001};
        bird.mass_kg = 6.0;
        bird.radius_m = 0.25;
        bird.friction = 0.4;
        bird.restitution = 0.1;
        bird.bullet = true;
        bird.projectile_visual_id = "bird";
        bird.launch_speed_cap_m_s = 40.0;
        bird.score_id = "bird_score";
        bird.icon_id = "icon";
        bird.animation_id = "animation";
        result.birds.push_back(std::move(bird));
    }
    return result;
}

LevelManifest level()
{
    LevelManifest result;
    result.schema_version = result.source_schema_version = 2U;
    result.id = "ability_dispatch";
    result.world_id = "earth";
    result.world = UniformWorldDefinition{
        .acceleration_m_s2 = {0.0, -9.81, 0.0},
        .bounds_min_m = {-24.0, -12.0, -12.0},
        .bounds_max_m = {48.0, 32.0, 12.0},
    };
    result.slingshot = {
        .asset_id = "launcher",
        .rest_position_m = {-4.0, 2.0, 0.0},
        .rest_rotation_xyzw = {0.0, 0.0, 0.0, 1.0},
        .spring_constant_n_m = 5200.0,
        .energy_efficiency = 0.9,
        .minimum_extension_m = 0.2,
        .maximum_extension_m = 4.25,
        .plane_policy = "gravity_vertical_camera_yaw",
        .projectile_clearance_m = 0.0,
        .speed_ceiling_m_s = 45.0,
    };
    result.bird_queue = {BirdArchetypeId{1}, BirdArchetypeId{2}};
    result.settle_policy = {0.15, 0.20, 60U};
    result.watchdog_ticks = 1500U;
    return result;
}

std::unique_ptr<SimulationSession> create_session(
    ArchetypeCatalog source = archetypes())
{
    auto created = SimulationSession::create(materials(), source, level());
    NINHO_SIM_REQUIRE(created.ok());
    return std::move(created.value);
}

void launch(SimulationSession& session)
{
    NINHO_SIM_REQUIRE(session.enqueue(BeginGrabCommand{{1.0F, 0.0F, 0.0F}}).ok());
    NINHO_SIM_REQUIRE(session.enqueue(SetPullCommand{-1.25, 0.5}).ok());
    NINHO_SIM_REQUIRE(session.tick().ok());
    NINHO_SIM_REQUIRE(session.enqueue(ReleaseBirdCommand{}).ok());
    NINHO_SIM_REQUIRE(session.tick().ok());
}

void advance_to_armed(SimulationSession& session)
{
    const TickIndex launch_tick =
        detail::SessionTestFacade::projectile_launch_tick(session);
    while (session.state().tick < TickIndex{launch_tick.value() + 8U}) {
        NINHO_SIM_REQUIRE(session.tick().ok());
    }
}

}

NINHO_SIM_TEST("ability dispatch invokes only the matching payload and runtime hooks")
{
    const std::array kinds{AbilityKind::LegacyGravityField, AbilityKind::GravityField,
        AbilityKind::MassBoost, AbilityKind::SpeedBoost, AbilityKind::Explosion,
        AbilityKind::Split};
    for (const AbilityKind kind : kinds) {
        RecordingHooks hooks;
        AbilitySystem system{hooks};
        AbilityArchetype selected = ability(kind);
        auto selected_shot = shot(kind);
        NINHO_SIM_REQUIRE(system.apply_before_step(selected, *selected_shot).ok());
        NINHO_SIM_REQUIRE(system.process_after_step(selected, *selected_shot).ok());
        NINHO_SIM_REQUIRE(system.finish_after_step(
            selected, *selected_shot, TickIndex{19}).ok());
        const std::size_t expected = kind == AbilityKind::LegacyGravityField
            ? 0U : static_cast<std::size_t>(kind) - 1U;
        NINHO_SIM_REQUIRE(hooks.before[expected] == 1U);
        NINHO_SIM_REQUIRE(hooks.after[expected] == 1U);
        NINHO_SIM_REQUIRE(hooks.finish[expected] == 1U);
        NINHO_SIM_REQUIRE(std::ranges::count(hooks.before, 0U) == 4U);
        NINHO_SIM_REQUIRE(std::ranges::count(hooks.after, 0U) == 4U);
        NINHO_SIM_REQUIRE(std::ranges::count(hooks.finish, 0U) == 4U);
    }
}

NINHO_SIM_TEST("ability dispatch rejects unknown and incompatible definitions during load")
{
    auto incompatible = ability(AbilityKind::GravityField);
    incompatible.payload = MassBoostAbilityDefinition{75U, 2.25};
    const auto wrong_payload = SimulationSession::create(
        materials(), archetypes(incompatible), level());
    NINHO_SIM_REQUIRE(!wrong_payload.ok());
    NINHO_SIM_REQUIRE(wrong_payload.error.code == ContentErrorCode::InvalidInvariant);
    NINHO_SIM_REQUIRE(wrong_payload.error.pointer == "/abilities/0/payload");

    auto unknown = ability(AbilityKind::GravityField);
    unknown.kind_v2 = static_cast<AbilityKind>(255U);
    const auto wrong_kind = SimulationSession::create(
        materials(), archetypes(unknown), level());
    NINHO_SIM_REQUIRE(!wrong_kind.ok());
    NINHO_SIM_REQUIRE(wrong_kind.error.code == ContentErrorCode::InvalidEnum);
    NINHO_SIM_REQUIRE(wrong_kind.error.pointer == "/abilities/0/kind");
}

NINHO_SIM_TEST("ability dispatch rejects activation for abilities without a concrete system")
{
    const std::array unsupported{AbilityKind::SpeedBoost,
        AbilityKind::Explosion, AbilityKind::Split};
    for (const AbilityKind kind : unsupported) {
        RecordingHooks hooks;
        AbilitySystem system{hooks};
        const AbilityArchetype selected = ability(kind);
        auto selected_shot = shot(kind);
        set_ability_runtime_active(selected_shot->runtime, false);
        const AbilityRuntime runtime_before = selected_shot->runtime;

        const SessionStatus status = AbilitySystem::activate(
            selected, *selected_shot, TickIndex{42});

        NINHO_SIM_REQUIRE(!status.ok());
        NINHO_SIM_REQUIRE(!selected_shot->activation_consumed);
        NINHO_SIM_REQUIRE(selected_shot->runtime == runtime_before);
        NINHO_SIM_REQUIRE(std::ranges::count(hooks.before, 0U) == 5U);
        NINHO_SIM_REQUIRE(std::ranges::count(hooks.after, 0U) == 5U);
        NINHO_SIM_REQUIRE(std::ranges::count(hooks.finish, 0U) == 5U);
    }
}

NINHO_SIM_TEST("ability dispatch fixes mass boost arming at nine ticks")
{
    AbilityArchetype selected = ability(AbilityKind::MassBoost);
    selected.arm_ticks = 0U;
    NINHO_SIM_REQUIRE(AbilitySystem::activation_arm_ticks(selected) == 9U);
    selected.arm_ticks = 999U;
    NINHO_SIM_REQUIRE(AbilitySystem::activation_arm_ticks(selected) == 9U);
}

NINHO_SIM_TEST("ability dispatch fails closed for unimplemented session abilities")
{
    const std::array unsupported{AbilityKind::SpeedBoost,
        AbilityKind::Explosion, AbilityKind::Split};
    auto session = create_session();
    launch(*session);
    advance_to_armed(*session);
    NINHO_SIM_REQUIRE(session->enqueue(ActivateAbilityCommand{}).ok());
    NINHO_SIM_REQUIRE(session->tick().ok());
    const auto canonical_before = session->canonical_state_v3();
    const auto shot_before = session->shot_state();
    const std::vector<DomainEvent> events_before{
        session->events().begin(), session->events().end()};

    for (const AbilityKind kind : unsupported) {
        const auto created = SimulationSession::create(
            materials(), archetypes(ability(kind)), level());
        NINHO_SIM_REQUIRE(!created.ok());
        NINHO_SIM_REQUIRE(created.error.code == ContentErrorCode::InvalidInvariant);
        NINHO_SIM_REQUIRE(created.error.pointer == "/abilities/0/kind");

        const SessionStatus status = session->reconfigure(
            materials(), archetypes(ability(kind)), level());
        NINHO_SIM_REQUIRE(!status.ok());
        NINHO_SIM_REQUIRE(status.error.code == ContentErrorCode::InvalidInvariant);
        NINHO_SIM_REQUIRE(status.error.pointer == "/abilities/0/kind");
        NINHO_SIM_REQUIRE(session->canonical_state_v3() == canonical_before);
        NINHO_SIM_REQUIRE(session->shot_state() == shot_before);
        NINHO_SIM_REQUIRE(std::ranges::equal(session->events(), events_before));
    }
}

NINHO_SIM_TEST("ability dispatch consumes activation once and restart reconfigure stay atomic")
{
    auto session = create_session();
    launch(*session);
    advance_to_armed(*session);
    NINHO_SIM_REQUIRE(session->enqueue(ActivateAbilityCommand{}).ok());
    NINHO_SIM_REQUIRE(session->enqueue(ActivateAbilityCommand{}).ok());
    NINHO_SIM_REQUIRE(session->tick().ok());
    NINHO_SIM_REQUIRE(session->shot_state()->activation_consumed);
    NINHO_SIM_REQUIRE(std::ranges::count(
        session->events(), DomainEventKind::AbilityStarted, &DomainEvent::kind) == 1);
    NINHO_SIM_REQUIRE(std::ranges::count(
        session->events(), DomainEventKind::CommandRejected, &DomainEvent::kind) == 1);

    const auto active_state = session->canonical_state_v3();
    auto incompatible = archetypes();
    incompatible.abilities.front().payload = MassBoostAbilityDefinition{75U, 2.25};
    NINHO_SIM_REQUIRE(!session->reconfigure(materials(), incompatible, level()).ok());
    NINHO_SIM_REQUIRE(session->canonical_state_v3() == active_state);
    NINHO_SIM_REQUIRE(session->shot_state()->activation_consumed);

    NINHO_SIM_REQUIRE(session->restart().ok());
    NINHO_SIM_REQUIRE(!session->shot_state().has_value());
    NINHO_SIM_REQUIRE(session->birds_remaining() == 2U);
}

NINHO_SIM_TEST("ability dispatch keeps the shot relevant until every child finishes")
{
    using detail::SessionTestFacade;
    auto session = create_session();
    launch(*session);
    const EntityId primary = session->shot_state()->projectile_ids.front();
    const EntityId child{0x80000010U};
    NINHO_SIM_REQUIRE(SessionTestFacade::append_projectile_body_for_testing(
        *session, child, {8.0F, 8.0F, 0.0F}, {0.0F, 1.0F, 0.0F}));
    SessionTestFacade::finish_projectile(*session, primary);
    NINHO_SIM_REQUIRE(session->tick().ok());
    NINHO_SIM_REQUIRE(session->state().phase == SessionPhase::FlightAbility);
    NINHO_SIM_REQUIRE((session->shot_state()->projectile_ids
        == std::vector{child}));
    NINHO_SIM_REQUIRE(session->birds_remaining() == 1U);

    SessionTestFacade::finish_projectile(*session, child);
    NINHO_SIM_REQUIRE(session->tick().ok());
    NINHO_SIM_REQUIRE(session->state().phase == SessionPhase::Resolution);
    NINHO_SIM_REQUIRE(session->birds_remaining() == 1U);
}

NINHO_SIM_TEST("ability dispatch removes one child without skipping the remaining iteration")
{
    using detail::SessionTestFacade;
    auto session = create_session();
    launch(*session);
    const EntityId primary = session->shot_state()->projectile_ids.front();
    const EntityId first{0x80000010U};
    const EntityId removed{0x80000020U};
    const EntityId last{0x80000030U};
    NINHO_SIM_REQUIRE(SessionTestFacade::append_projectile_body_for_testing(
        *session, first, {6.0F, 8.0F, -4.0F}, {0.0F, 1.0F, 0.0F}));
    NINHO_SIM_REQUIRE(SessionTestFacade::append_projectile_body_for_testing(
        *session, removed, {8.0F, 8.0F, 0.0F}, {0.0F, 1.0F, 0.0F}));
    NINHO_SIM_REQUIRE(SessionTestFacade::append_projectile_body_for_testing(
        *session, last, {10.0F, 8.0F, 4.0F}, {0.0F, 1.0F, 0.0F}));
    SessionTestFacade::finish_projectile(*session, removed);
    const auto first_age = SessionTestFacade::projectile_age(*session, first);
    const auto last_age = SessionTestFacade::projectile_age(*session, last);
    NINHO_SIM_REQUIRE(session->tick().ok());
    NINHO_SIM_REQUIRE(SessionTestFacade::projectile_age(*session, first)
        == first_age + 1U);
    NINHO_SIM_REQUIRE(SessionTestFacade::projectile_age(*session, last)
        == last_age + 1U);
    NINHO_SIM_REQUIRE(!SessionTestFacade::has_body_record(*session, removed));
    NINHO_SIM_REQUIRE((session->shot_state()->projectile_ids == std::vector{
        primary, first, last}));
}

NINHO_SIM_TEST("ability dispatch watchdog expires only the old projectile in either order")
{
    using detail::SessionTestFacade;
    for (const bool expired_before_survivor : {false, true}) {
        auto session = create_session();
        launch(*session);
        const EntityId parent = session->shot_state()->projectile_ids.front();
        const EntityId clone{0x80000010U};
        NINHO_SIM_REQUIRE(SessionTestFacade::append_projectile_body_for_testing(
            *session, clone, {8.0F, 8.0F, 0.0F}, {0.0F, 1.0F, 0.0F}));
        const EntityId expired = expired_before_survivor ? parent : clone;
        const EntityId survivor = expired_before_survivor ? clone : parent;
        SessionTestFacade::age_projectile(*session, expired, 1499U);
        const std::uint32_t survivor_age =
            SessionTestFacade::projectile_age(*session, survivor);

        NINHO_SIM_REQUIRE(session->tick().ok());
        NINHO_SIM_REQUIRE(session->state().phase == SessionPhase::FlightAbility);
        NINHO_SIM_REQUIRE((session->shot_state()->projectile_ids
            == std::vector{survivor}));
        NINHO_SIM_REQUIRE(SessionTestFacade::has_body_record(*session, survivor));
        NINHO_SIM_REQUIRE(SessionTestFacade::projectile_age(*session, survivor)
            == survivor_age + 1U);

        for (std::uint32_t tick = 0U; tick < 3U; ++tick) {
            const std::uint32_t age_before =
                SessionTestFacade::projectile_age(*session, survivor);
            NINHO_SIM_REQUIRE(session->tick().ok());
            NINHO_SIM_REQUIRE(session->state().phase == SessionPhase::FlightAbility);
            NINHO_SIM_REQUIRE((session->shot_state()->projectile_ids
                == std::vector{survivor}));
            NINHO_SIM_REQUIRE(!SessionTestFacade::has_body_record(*session, expired));
            NINHO_SIM_REQUIRE(SessionTestFacade::projectile_age(*session, survivor)
                == age_before + 1U);
        }

        SessionTestFacade::age_projectile(*session, survivor, 1499U);
        NINHO_SIM_REQUIRE(session->tick().ok());
        NINHO_SIM_REQUIRE(session->state().phase == SessionPhase::Evaluation);
        NINHO_SIM_REQUIRE(session->birds_remaining() == 1U);
        NINHO_SIM_REQUIRE(session->tick().ok());
        NINHO_SIM_REQUIRE(session->state().phase == SessionPhase::Inspection);
        NINHO_SIM_REQUIRE(session->birds_remaining() == 1U);
    }
}

NINHO_SIM_TEST("ability dispatch watchdog waits for an active bounded ability")
{
    using detail::SessionTestFacade;
    auto session = create_session();
    launch(*session);
    advance_to_armed(*session);
    NINHO_SIM_REQUIRE(session->enqueue(ActivateAbilityCommand{}).ok());
    NINHO_SIM_REQUIRE(session->tick().ok());
    SessionTestFacade::age_projectile(*session, 1499U);
    NINHO_SIM_REQUIRE(session->tick().ok());
    NINHO_SIM_REQUIRE(session->state().phase == SessionPhase::FlightAbility);
    NINHO_SIM_REQUIRE(SessionTestFacade::ability_active(*session));

    const TickIndex end_tick = SessionTestFacade::ability_end_tick(*session);
    while (session->state().tick < end_tick) {
        NINHO_SIM_REQUIRE(session->tick().ok());
    }
    NINHO_SIM_REQUIRE(!SessionTestFacade::ability_active(*session));
    NINHO_SIM_REQUIRE(session->state().phase == SessionPhase::Evaluation);
    NINHO_SIM_REQUIRE(std::ranges::any_of(session->events(), [](const DomainEvent& event) {
        return event.kind == DomainEventKind::AbilityEnded;
    }));
}
