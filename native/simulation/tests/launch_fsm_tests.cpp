#include "test_framework.hpp"

#include "session_test_facade.hpp"
#include "ninho/simulation/commands.hpp"
#include "ninho/simulation/session.hpp"
#include <nlohmann/json.hpp>

#include <algorithm>
#include <array>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <numbers>
#include <limits>
#include <sstream>
#include <string_view>
#include <type_traits>
#include <utility>
#include <vector>

namespace {

using namespace ninho::simulation;

std::string read_source_file(std::string_view relative_path)
{
    std::ifstream stream{std::filesystem::path{NINHO_SOURCE_DIR} / relative_path,
        std::ios::binary};
    NINHO_SIM_REQUIRE(stream.is_open());
    std::ostringstream content;
    content << stream.rdbuf();
    return content.str();
}

ContentBundle load_bundle()
{
    const auto materials = parse_material_catalog(
        read_source_file("game/data/materials/vertical_slice.materials.json"));
    const auto archetypes = parse_archetype_catalog(
        read_source_file("game/data/archetypes/vertical_slice.archetypes.json"));
    const auto level = parse_level_manifest(
        read_source_file("game/data/levels/first_orbit.level.json"));
    NINHO_SIM_REQUIRE(materials.ok() && archetypes.ok() && level.ok());
    const auto bundle = make_content_bundle(materials.value, archetypes.value, level.value);
    NINHO_SIM_REQUIRE(bundle.ok());
    return bundle.value;
}

std::unique_ptr<SimulationSession> create_session()
{
    const auto bundle = load_bundle();
    auto session = SimulationSession::create(bundle.materials, bundle.archetypes, bundle.level);
    NINHO_SIM_REQUIRE(session.ok());
    return std::move(session.value);
}

AimState default_aim(double speed = 10.5)
{
    return {{-13.0004f, 0.0004f, 0.0f}, {0.0f, 1.00004f, 0.00004f}, speed};
}

void enter_aim(SimulationSession& session)
{
    NINHO_SIM_REQUIRE(session.enqueue(BeginAimCommand{}).ok());
    NINHO_SIM_REQUIRE(session.state().phase == SessionPhase::Inspection);
    NINHO_SIM_REQUIRE(session.tick().ok());
    NINHO_SIM_REQUIRE(session.state().phase == SessionPhase::Aim);
}

void launch(SimulationSession& session, AimState aim = default_aim())
{
    enter_aim(session);
    NINHO_SIM_REQUIRE(session.enqueue(SetAimCommand{aim}).ok());
    NINHO_SIM_REQUIRE(session.enqueue(LaunchCommand{}).ok());
    NINHO_SIM_REQUIRE(session.tick().ok());
    NINHO_SIM_REQUIRE(session.state().phase == SessionPhase::FlightAbility);
}

void settle_to_evaluation(SimulationSession& session);

template <typename T>
concept ExposesPhysicsHandle = requires(T value) {
    value.physics_handle;
    value.handle;
};

static_assert(!ExposesPhysicsHandle<AimState>);
static_assert(!ExposesPhysicsHandle<TrajectoryPreview>);
static_assert(!ExposesPhysicsHandle<DomainEvent>);
static_assert(!noexcept(std::declval<const SimulationSession&>().preview(
    std::declval<const AimState&>())));
static_assert(!noexcept(std::declval<const SimulationSession&>().quantize_aim(
    std::declval<const AimState&>())));

static_assert(PlayerCommand{BeginAimCommand{}}.index() == 0U);
static_assert(PlayerCommand{SetAimCommand{}}.index() == 1U);
static_assert(PlayerCommand{LaunchCommand{}}.index() == 2U);
static_assert(PlayerCommand{ActivateAbilityCommand{}}.index() == 3U);
static_assert(PlayerCommand{CancelAimCommand{}}.index() == 4U);
static_assert(PlayerCommand{BeginGrabCommand{}}.index() == 5U);
static_assert(PlayerCommand{SetPullCommand{}}.index() == 6U);
static_assert(PlayerCommand{ReleaseBirdCommand{}}.index() == 7U);
static_assert(PlayerCommand{CancelGrabCommand{}}.index() == 8U);

NINHO_SIM_TEST("launch fsm queue is bounded sequenced deferred and last aim wins")
{
    auto session = create_session();
    const auto initial_hash = session->canonical_hash_v2();
    for (std::size_t index = 0; index < 128; ++index) {
        NINHO_SIM_REQUIRE(session->enqueue(BeginAimCommand{}).ok());
    }
    NINHO_SIM_REQUIRE(session->canonical_hash_v2() != initial_hash);
    const auto overflow = session->enqueue(BeginAimCommand{});
    NINHO_SIM_REQUIRE(!overflow.ok());
    NINHO_SIM_REQUIRE(overflow.error.code == ContentErrorCode::ResourceLimit);
    NINHO_SIM_REQUIRE(session->state().phase == SessionPhase::Inspection);
    NINHO_SIM_REQUIRE(session->tick().ok());
    NINHO_SIM_REQUIRE(session->state().phase == SessionPhase::Aim);
    NINHO_SIM_REQUIRE(
        detail::SessionTestFacade::last_processed_command_sequence(*session) == 128U);

    NINHO_SIM_REQUIRE(session->enqueue(SetAimCommand{default_aim(9.111)}).ok());
    NINHO_SIM_REQUIRE(session->enqueue(SetAimCommand{default_aim(15.999)}).ok());
    NINHO_SIM_REQUIRE(session->tick().ok());
    NINHO_SIM_REQUIRE(session->state().aim.has_value());
    NINHO_SIM_REQUIRE(std::abs(session->state().aim->speed_m_s - 16.0) < 1.0e-9);
}

NINHO_SIM_TEST("launch fsm rejects a nonfinite command envelope without faulting or consuming queue")
{
    auto session = create_session();
    AimState invalid = default_aim();
    invalid.speed_m_s = std::numeric_limits<double>::quiet_NaN();
    const auto status = session->enqueue(SetAimCommand{invalid});
    NINHO_SIM_REQUIRE(!status.ok());
    NINHO_SIM_REQUIRE(status.error.code == ContentErrorCode::InvalidNumber);
    NINHO_SIM_REQUIRE(session->state().phase == SessionPhase::Inspection);
    NINHO_SIM_REQUIRE(detail::SessionTestFacade::last_processed_command_sequence(*session) == 0U);
    NINHO_SIM_REQUIRE(session->tick().ok());
    NINHO_SIM_REQUIRE(session->events().empty());
}

NINHO_SIM_TEST("launch fsm coalescing respects begin aim and launch barriers")
{
    auto launch_barrier = create_session();
    NINHO_SIM_REQUIRE(launch_barrier->enqueue(BeginAimCommand{}).ok());
    NINHO_SIM_REQUIRE(launch_barrier->enqueue(SetAimCommand{default_aim(9.0)}).ok());
    NINHO_SIM_REQUIRE(launch_barrier->enqueue(LaunchCommand{}).ok());
    NINHO_SIM_REQUIRE(launch_barrier->enqueue(SetAimCommand{default_aim(12.0)}).ok());
    NINHO_SIM_REQUIRE(launch_barrier->tick().ok());
    const auto bird = std::ranges::find_if(launch_barrier->snapshots(), [](const auto& snapshot) {
        return snapshot.entity_id == EntityId{0x80000000U};
    });
    NINHO_SIM_REQUIRE(bird != launch_barrier->snapshots().end());
    NINHO_SIM_REQUIRE(std::abs(bird->linear_velocity_m_s.y - 9.0f) < 0.01f);
    NINHO_SIM_REQUIRE(launch_barrier->events().size() == 2U);
    NINHO_SIM_REQUIRE(launch_barrier->events()[0].kind == DomainEventKind::BirdLaunched);
    NINHO_SIM_REQUIRE(launch_barrier->events()[1].kind == DomainEventKind::CommandRejected);
    NINHO_SIM_REQUIRE(
        launch_barrier->events()[1].rejection_reason == CommandRejectionReason::InvalidPhase);

    auto begin_barrier = create_session();
    NINHO_SIM_REQUIRE(begin_barrier->enqueue(SetAimCommand{default_aim(9.0)}).ok());
    NINHO_SIM_REQUIRE(begin_barrier->enqueue(BeginAimCommand{}).ok());
    NINHO_SIM_REQUIRE(begin_barrier->enqueue(SetAimCommand{default_aim(12.0)}).ok());
    NINHO_SIM_REQUIRE(begin_barrier->tick().ok());
    NINHO_SIM_REQUIRE(begin_barrier->state().phase == SessionPhase::Aim);
    NINHO_SIM_REQUIRE(std::abs(begin_barrier->state().aim->speed_m_s - 12.0) < 1e-9);
    NINHO_SIM_REQUIRE(begin_barrier->events().size() == 1U);
    NINHO_SIM_REQUIRE(
        begin_barrier->events()[0].rejection_reason == CommandRejectionReason::InvalidPhase);
}

NINHO_SIM_TEST("launch fsm coalesces only consecutive aim updates inside one barrier")
{
    auto session = create_session();
    NINHO_SIM_REQUIRE(session->enqueue(BeginAimCommand{}).ok());
    NINHO_SIM_REQUIRE(session->enqueue(SetAimCommand{default_aim(9.0)}).ok());
    NINHO_SIM_REQUIRE(session->enqueue(SetAimCommand{default_aim(11.0)}).ok());
    NINHO_SIM_REQUIRE(session->enqueue(SetAimCommand{default_aim(13.0)}).ok());
    NINHO_SIM_REQUIRE(session->tick().ok());
    NINHO_SIM_REQUIRE(session->events().empty());
    NINHO_SIM_REQUIRE(std::abs(session->state().aim->speed_m_s - 13.0) < 1e-9);
    NINHO_SIM_REQUIRE(
        detail::SessionTestFacade::last_processed_command_sequence(*session) == 4U);
}

NINHO_SIM_TEST("launch fsm quantizes and validates orbital aim contract")
{
    auto session = create_session();
    enter_aim(*session);
    NINHO_SIM_REQUIRE(session->enqueue(SetAimCommand{default_aim(10.504)}).ok());
    NINHO_SIM_REQUIRE(session->tick().ok());
    const AimState& aim = *session->state().aim;
    NINHO_SIM_REQUIRE(std::abs(aim.origin_m.x + 13.0f) < 1.0e-6f);
    NINHO_SIM_REQUIRE(std::abs(aim.origin_m.y) < 1.0e-6f);
    NINHO_SIM_REQUIRE(std::abs(aim.speed_m_s - 10.50) < 1.0e-9);
    NINHO_SIM_REQUIRE(std::abs(ninho::physics::length(aim.tangent_direction) - 1.0f) < 1.0e-5f);
    const auto radial = ninho::physics::normalized_or_zero(aim.origin_m);
    NINHO_SIM_REQUIRE(std::abs(ninho::physics::dot(radial, aim.tangent_direction)) <= 0.01f);

    NINHO_SIM_REQUIRE(session->enqueue(SetAimCommand{default_aim(40.0)}).ok());
    NINHO_SIM_REQUIRE(session->tick().ok());
    NINHO_SIM_REQUIRE(std::abs(session->state().aim->speed_m_s - 16.0) < 1.0e-9);

    const double theta = 51.0 * std::numbers::pi / 180.0;
    AimState outside{{static_cast<float>(-13.0 * std::cos(theta)), 0.0f,
                         static_cast<float>(13.0 * std::sin(theta))},
        {0.0f, 1.0f, 0.0f}, 10.5};
    NINHO_SIM_REQUIRE(session->enqueue(SetAimCommand{outside}).ok());
    const auto previous = session->state().aim;
    const auto invalid = session->tick();
    NINHO_SIM_REQUIRE(invalid.ok());
    NINHO_SIM_REQUIRE(session->state().phase == SessionPhase::Aim);
    NINHO_SIM_REQUIRE(session->state().outcome == Outcome::None);
    NINHO_SIM_REQUIRE(session->state().aim == previous);
    NINHO_SIM_REQUIRE(session->events().size() == 1U);
    NINHO_SIM_REQUIRE(session->events().front().kind == DomainEventKind::CommandRejected);
    NINHO_SIM_REQUIRE(
        session->events().front().rejection_reason == CommandRejectionReason::InvalidAim);
    NINHO_SIM_REQUIRE(session->tick().ok());
    NINHO_SIM_REQUIRE(session->events().empty());
}

NINHO_SIM_TEST("launch fsm consumes begin aim and set aim in sequence in one tick")
{
    auto session = create_session();
    NINHO_SIM_REQUIRE(session->enqueue(BeginAimCommand{}).ok());
    NINHO_SIM_REQUIRE(session->enqueue(SetAimCommand{default_aim(12.345)}).ok());
    NINHO_SIM_REQUIRE(session->tick().ok());
    NINHO_SIM_REQUIRE(session->state().phase == SessionPhase::Aim);
    NINHO_SIM_REQUIRE(session->state().aim.has_value());
    NINHO_SIM_REQUIRE(std::abs(session->state().aim->speed_m_s - 12.35) < 1.0e-9);
}

NINHO_SIM_TEST("launch fsm cancels aim without consuming the next bird")
{
    auto session = create_session();
    enter_aim(*session);
    NINHO_SIM_REQUIRE(session->enqueue(SetAimCommand{default_aim(8.0)}).ok());
    NINHO_SIM_REQUIRE(session->enqueue(CancelAimCommand{}).ok());
    NINHO_SIM_REQUIRE(session->tick().ok());
    NINHO_SIM_REQUIRE(session->state().phase == SessionPhase::Inspection);
    NINHO_SIM_REQUIRE(session->state().outcome == Outcome::None);
    NINHO_SIM_REQUIRE(!session->state().aim.has_value());
    NINHO_SIM_REQUIRE(session->birds_remaining() == 3U);
    NINHO_SIM_REQUIRE(session->events().empty());
}

NINHO_SIM_TEST("launch fsm enforces shell arc tangency and global speed boundaries")
{
    auto session = create_session();
    const auto aim_at = [](double radius, double theta_deg, double radial_direction,
                            double speed) {
        const double theta = theta_deg * std::numbers::pi / 180.0;
        return AimState{{static_cast<float>(-radius * std::cos(theta)), 0.0f,
                            static_cast<float>(radius * std::sin(theta))},
            {static_cast<float>(radial_direction), 1.0f, 0.0f}, speed};
    };
    NINHO_SIM_REQUIRE(session->quantize_aim(aim_at(13.049, -50.0, 0.0, 8.0)).ok());
    NINHO_SIM_REQUIRE(session->quantize_aim(aim_at(13.049, 50.0, 0.0, 40.0)).ok());
    NINHO_SIM_REQUIRE(!session->quantize_aim(aim_at(13.051, 0.0, 0.0, 10.5)).ok());
    NINHO_SIM_REQUIRE(!session->quantize_aim(aim_at(13.0, -50.1, 0.0, 10.5)).ok());
    NINHO_SIM_REQUIRE(!session->quantize_aim(aim_at(13.0, 50.1, 0.0, 10.5)).ok());
    NINHO_SIM_REQUIRE(!session->quantize_aim(aim_at(13.0, 0.0, -0.0111, 10.5)).ok());
    NINHO_SIM_REQUIRE(!session->quantize_aim(aim_at(13.0, 0.0, 0.0, 7.99)).ok());
    NINHO_SIM_REQUIRE(!session->quantize_aim(aim_at(13.0, 0.0, 0.0, 40.01)).ok());
}

NINHO_SIM_TEST("launch fsm creates one stable bullet bird and publishes one tick event")
{
    auto session = create_session();
    const auto initial_bodies = session->physics_metrics().body_count;
    launch(*session);
    NINHO_SIM_REQUIRE(session->birds_remaining() == 2U);
    NINHO_SIM_REQUIRE(session->physics_metrics().body_count == initial_bodies + 1);
    const auto bird = std::ranges::find_if(session->snapshots(), [](const auto& snapshot) {
        return snapshot.entity_id == EntityId{0x80000000U};
    });
    NINHO_SIM_REQUIRE(bird != session->snapshots().end());
    NINHO_SIM_REQUIRE(bird->shape.type == ShapeType::Sphere);
    NINHO_SIM_REQUIRE(std::abs(bird->shape.radius_m - 0.45) < 1.0e-9);
    NINHO_SIM_REQUIRE(std::abs(bird->mass_kg - 140.0) < 0.25);
    NINHO_SIM_REQUIRE(std::abs(bird->linear_velocity_m_s.y - 10.5f) < 0.01f);
    NINHO_SIM_REQUIRE(detail::SessionTestFacade::projectile_is_bullet(*session));
    NINHO_SIM_REQUIRE(session->events().size() == 1U);
    NINHO_SIM_REQUIRE(session->events().front().kind == DomainEventKind::BirdLaunched);
    NINHO_SIM_REQUIRE(session->events().front().entity_id == EntityId{0x80000000U});
    NINHO_SIM_REQUIRE(session->events().front().tick == TickIndex{2});
    NINHO_SIM_REQUIRE(session->tick().ok());
    NINHO_SIM_REQUIRE(session->events().empty());
}

NINHO_SIM_TEST("launch fsm trajectory preview stops at the world removal envelope")
{
    const auto bundle = load_bundle();
    auto session = create_session();
    const auto preview = session->preview(default_aim(16.0));
    const float removal_radius =
        6.0f * static_cast<float>(bundle.level.planet.radius_m);

    NINHO_SIM_REQUIRE(preview.status.ok());
    NINHO_SIM_REQUIRE(!preview.first_hit.has_value());
    NINHO_SIM_REQUIRE(preview.samples.size() > 2U);
    NINHO_SIM_REQUIRE(preview.samples.size() < 601U);
    NINHO_SIM_REQUIRE(
        ninho::physics::length(preview.samples.back()) >= removal_radius);
    NINHO_SIM_REQUIRE(ninho::physics::length(
                          preview.samples[preview.samples.size() - 2U])
        < removal_radius);
}

NINHO_SIM_TEST("launch fsm trajectory preview shares quantized gravity and collision path")
{
    auto session = create_session();
    NINHO_SIM_REQUIRE(detail::SessionTestFacade::add_static_sphere(
        *session, EntityId{9000}, {-12.7f, 4.0f, 0.0f}, 0.5));
    NINHO_SIM_REQUIRE(session->tick().ok());
    const AimState collision_aim = default_aim(8.0);
    const auto preview = session->preview(collision_aim);
    NINHO_SIM_REQUIRE(preview.status.ok());
    NINHO_SIM_REQUIRE(preview.samples.size() > 12U);
    NINHO_SIM_REQUIRE(preview.first_hit.has_value());
    NINHO_SIM_REQUIRE(preview.quantized_aim == session->quantize_aim(collision_aim).value);
    const auto golden_hash = preview.canonical_hash;
    NINHO_SIM_REQUIRE(golden_hash == 17815016211264547523ULL);
    NINHO_SIM_REQUIRE(preview.samples.size() == 25U);
    NINHO_SIM_REQUIRE(std::abs(preview.first_hit->point_m.x + 12.6396f) < 0.0001f);
    NINHO_SIM_REQUIRE(std::abs(preview.first_hit->point_m.y - 3.50366f) < 0.0001f);

    NINHO_SIM_REQUIRE(session->enqueue(BeginAimCommand{}).ok());
    NINHO_SIM_REQUIRE(session->enqueue(SetAimCommand{collision_aim}).ok());
    NINHO_SIM_REQUIRE(session->enqueue(LaunchCommand{}).ok());
    NINHO_SIM_REQUIRE(session->tick().ok());
    for (std::size_t tick = 0; tick < 600 && !session->state().last_impact_m; ++tick) {
        NINHO_SIM_REQUIRE(session->tick().ok());
    }
    NINHO_SIM_REQUIRE(session->state().last_impact_m.has_value());
    const auto delta = *session->state().last_impact_m - preview.first_hit->point_m;
    NINHO_SIM_REQUIRE(ninho::physics::length(delta) <= 0.10f);
    NINHO_SIM_REQUIRE(session->restart().ok());
    NINHO_SIM_REQUIRE(detail::SessionTestFacade::add_static_sphere(
        *session, EntityId{9000}, {-12.7f, 4.0f, 0.0f}, 0.5));
    NINHO_SIM_REQUIRE(session->tick().ok());
    NINHO_SIM_REQUIRE(session->preview(collision_aim).canonical_hash == golden_hash);
}

NINHO_SIM_TEST("launch fsm preview reads the current world after aim ticks and altered structure")
{
    auto session = create_session();
    enter_aim(*session);
    NINHO_SIM_REQUIRE(session->enqueue(SetAimCommand{default_aim(8.0)}).ok());
    NINHO_SIM_REQUIRE(session->tick().ok());
    const auto before_ticks = session->preview(default_aim(8.0));
    NINHO_SIM_REQUIRE(before_ticks.status.ok() && before_ticks.first_hit);
    for (int tick = 0; tick < 8; ++tick) {
        NINHO_SIM_REQUIRE(session->tick().ok());
    }
    const auto after_ticks = session->preview(default_aim(8.0));
    NINHO_SIM_REQUIRE(after_ticks.status.ok() && after_ticks.first_hit);
    NINHO_SIM_REQUIRE(after_ticks.canonical_hash != before_ticks.canonical_hash);

    auto altered = create_session();
    launch(*altered, default_aim(8.0));
    settle_to_evaluation(*altered);
    NINHO_SIM_REQUIRE(altered->tick().ok());
    NINHO_SIM_REQUIRE(detail::SessionTestFacade::impulse_entity(
        *altered, EntityId{110}, {0.0f, 0.0f, 250.0f}));
    NINHO_SIM_REQUIRE(altered->tick().ok());
    enter_aim(*altered);
    const auto altered_preview = altered->preview(default_aim(8.0));
    NINHO_SIM_REQUIRE(altered_preview.status.ok() && altered_preview.first_hit);
    auto fresh = create_session();
    const auto fresh_preview = fresh->preview(default_aim(8.0));
    NINHO_SIM_REQUIRE(fresh_preview.status.ok() && fresh_preview.first_hit);
    NINHO_SIM_REQUIRE(altered_preview.canonical_hash != fresh_preview.canonical_hash);
}

NINHO_SIM_TEST("launch fsm preview rejects flight phase instead of returning a stale path")
{
    auto session = create_session();
    launch(*session);
    const auto rejected = session->preview(default_aim());
    NINHO_SIM_REQUIRE(!rejected.status.ok());
    NINHO_SIM_REQUIRE(rejected.samples.empty());
    NINHO_SIM_REQUIRE(!rejected.first_hit.has_value());
}

void settle_to_evaluation(SimulationSession& session)
{
    detail::SessionTestFacade::finish_projectile(session);
    for (int tick = 0; tick < 62 && session.state().phase != SessionPhase::Evaluation; ++tick) {
        NINHO_SIM_REQUIRE(session.tick().ok());
    }
    NINHO_SIM_REQUIRE(session.state().phase == SessionPhase::Evaluation);
}

NINHO_SIM_TEST("launch fsm resolves victory on first second or third launch and defeat after third")
{
    for (std::uint32_t victory_launch = 1; victory_launch <= 3; ++victory_launch) {
        auto session = create_session();
        for (std::uint32_t launched = 1; launched <= victory_launch; ++launched) {
            launch(*session);
            if (launched == victory_launch) {
                detail::SessionTestFacade::complete_objective(*session);
            }
            settle_to_evaluation(*session);
            NINHO_SIM_REQUIRE(session->tick().ok());
            if (launched == victory_launch) {
                NINHO_SIM_REQUIRE(session->state().phase == SessionPhase::Result);
                NINHO_SIM_REQUIRE(session->state().outcome == Outcome::Victory);
            } else {
                NINHO_SIM_REQUIRE(session->state().phase == SessionPhase::Inspection);
                NINHO_SIM_REQUIRE(session->state().outcome == Outcome::None);
            }
        }
    }

    auto defeat = create_session();
    for (int launched = 0; launched < 3; ++launched) {
        launch(*defeat);
        settle_to_evaluation(*defeat);
        NINHO_SIM_REQUIRE(defeat->tick().ok());
    }
    NINHO_SIM_REQUIRE(defeat->state().phase == SessionPhase::Result);
    NINHO_SIM_REQUIRE(defeat->state().outcome == Outcome::Defeat);
}

NINHO_SIM_TEST("launch fsm completes a neutralized objective without waiting for unrelated debris")
{
    auto session = create_session();
    launch(*session);
    NINHO_SIM_REQUIRE(detail::SessionTestFacade::add_dynamic_sphere(*session,
        EntityId{0x70000000U}, PartId{1}, {0.0f, 20.0f, 0.0f}, 25.0,
        {18.0f, 0.0f, 0.0f}, 0.25));
    detail::SessionTestFacade::complete_objective(*session);

    NINHO_SIM_REQUIRE(session->tick().ok());
    NINHO_SIM_REQUIRE(session->objectives_complete());
    NINHO_SIM_REQUIRE(session->state().phase == SessionPhase::Evaluation);
    NINHO_SIM_REQUIRE(std::ranges::any_of(session->snapshots(), [](const auto& snapshot) {
        return snapshot.entity_id == EntityId{0x70000000U}
            && ninho::physics::length(snapshot.linear_velocity_m_s) > 1.0f;
    }));

    NINHO_SIM_REQUIRE(session->tick().ok());
    NINHO_SIM_REQUIRE(session->state().phase == SessionPhase::Result);
    NINHO_SIM_REQUIRE(session->state().outcome == Outcome::Victory);
}

NINHO_SIM_TEST("launch fsm waits for active ability then lifetime and watchdog guarantee evaluation")
{
    auto session = create_session();
    launch(*session);
    detail::SessionTestFacade::set_ability_active(*session, true);
    detail::SessionTestFacade::finish_projectile(*session);
    for (int tick = 0; tick < 80; ++tick) {
        NINHO_SIM_REQUIRE(session->tick().ok());
    }
    NINHO_SIM_REQUIRE(session->state().phase == SessionPhase::FlightAbility);
    detail::SessionTestFacade::set_ability_active(*session, false);
    NINHO_SIM_REQUIRE(session->tick().ok());
    NINHO_SIM_REQUIRE(session->state().phase == SessionPhase::Resolution);

    auto lifetime = create_session();
    launch(*lifetime);
    detail::SessionTestFacade::age_projectile(*lifetime, 599);
    NINHO_SIM_REQUIRE(lifetime->tick().ok());
    NINHO_SIM_REQUIRE(lifetime->state().phase == SessionPhase::Resolution);

    auto watchdog = create_session();
    launch(*watchdog);
    detail::SessionTestFacade::set_ability_active(*watchdog, true);
    detail::SessionTestFacade::age_projectile(*watchdog, 1799);
    NINHO_SIM_REQUIRE(watchdog->tick().ok());
    NINHO_SIM_REQUIRE(watchdog->state().phase == SessionPhase::Evaluation);
}

NINHO_SIM_TEST("launch fsm preserves a decided result after a watchdog evaluation")
{
    auto victory = create_session();
    launch(*victory);
    detail::SessionTestFacade::complete_objective(*victory);
    detail::SessionTestFacade::set_ability_active(*victory, true);
    detail::SessionTestFacade::age_projectile(*victory, 1799);
    NINHO_SIM_REQUIRE(victory->tick().ok());
    NINHO_SIM_REQUIRE(victory->state().phase == SessionPhase::Evaluation);
    NINHO_SIM_REQUIRE(victory->tick().ok());
    NINHO_SIM_REQUIRE(victory->state().phase == SessionPhase::Result);
    NINHO_SIM_REQUIRE(victory->state().outcome == Outcome::Victory);
    for (int tick = 0; tick < 3; ++tick) {
        NINHO_SIM_REQUIRE(victory->tick().ok());
        NINHO_SIM_REQUIRE(victory->state().phase == SessionPhase::Result);
        NINHO_SIM_REQUIRE(victory->state().outcome == Outcome::Victory);
    }

    auto defeat = create_session();
    for (int shot = 0; shot < 3; ++shot) {
        launch(*defeat);
        if (shot < 2) {
            settle_to_evaluation(*defeat);
            NINHO_SIM_REQUIRE(defeat->tick().ok());
            continue;
        }
        detail::SessionTestFacade::set_ability_active(*defeat, true);
        detail::SessionTestFacade::age_projectile(*defeat, 1799);
        NINHO_SIM_REQUIRE(defeat->tick().ok());
        NINHO_SIM_REQUIRE(defeat->state().phase == SessionPhase::Evaluation);
        NINHO_SIM_REQUIRE(defeat->tick().ok());
    }
    NINHO_SIM_REQUIRE(defeat->state().phase == SessionPhase::Result);
    NINHO_SIM_REQUIRE(defeat->state().outcome == Outcome::Defeat);
    for (int tick = 0; tick < 3; ++tick) {
        NINHO_SIM_REQUIRE(defeat->tick().ok());
        NINHO_SIM_REQUIRE(defeat->state().phase == SessionPhase::Result);
        NINHO_SIM_REQUIRE(defeat->state().outcome == Outcome::Defeat);
    }
}

NINHO_SIM_TEST("launch fsm activation arms exactly at launch tick plus nine")
{
    auto session = create_session();
    launch(*session);
    const TickIndex launch_tick = detail::SessionTestFacade::projectile_launch_tick(*session);
    NINHO_SIM_REQUIRE(launch_tick == session->state().tick);
    for (int tick = 0; tick < 7; ++tick) {
        NINHO_SIM_REQUIRE(session->tick().ok());
    }
    NINHO_SIM_REQUIRE(session->enqueue(ActivateAbilityCommand{}).ok());
    NINHO_SIM_REQUIRE(session->tick().ok());
    NINHO_SIM_REQUIRE(session->state().tick == TickIndex{launch_tick.value() + 8U});
    NINHO_SIM_REQUIRE(!detail::SessionTestFacade::ability_requested(*session));
    NINHO_SIM_REQUIRE(session->events().size() == 1U);
    NINHO_SIM_REQUIRE(session->events().front().kind == DomainEventKind::CommandRejected);
    NINHO_SIM_REQUIRE(
        session->events().front().rejection_reason == CommandRejectionReason::NotArmed);
    NINHO_SIM_REQUIRE(session->enqueue(ActivateAbilityCommand{}).ok());
    NINHO_SIM_REQUIRE(session->tick().ok());
    NINHO_SIM_REQUIRE(session->state().tick == TickIndex{launch_tick.value() + 9U});
    NINHO_SIM_REQUIRE(detail::SessionTestFacade::ability_requested(*session));
    NINHO_SIM_REQUIRE(detail::SessionTestFacade::ability_active(*session));
    NINHO_SIM_REQUIRE(session->events().front().kind == DomainEventKind::AbilityStarted);
    NINHO_SIM_REQUIRE(detail::SessionTestFacade::ability_end_tick(*session)
        == TickIndex{launch_tick.value() + 83U});
}

NINHO_SIM_TEST("launch fsm late activation ends after the data driven duration")
{
    auto session = create_session();
    launch(*session);
    const TickIndex launched = detail::SessionTestFacade::projectile_launch_tick(*session);
    while (session->state().tick.value() < launched.value() + 19U) {
        NINHO_SIM_REQUIRE(session->tick().ok());
    }
    NINHO_SIM_REQUIRE(session->enqueue(ActivateAbilityCommand{}).ok());
    NINHO_SIM_REQUIRE(session->tick().ok());
    const TickIndex activated{launched.value() + 20U};
    NINHO_SIM_REQUIRE(session->state().tick == activated);
    NINHO_SIM_REQUIRE(detail::SessionTestFacade::ability_end_tick(*session)
        == TickIndex{activated.value() + 74U});
}

NINHO_SIM_TEST("launch fsm resolves arm and duration from the selected bird ability")
{
    ContentBundle bundle = load_bundle();
    AbilityArchetype second_ability = bundle.archetypes.abilities.front();
    second_ability.id = AbilityId{2};
    second_ability.key = "short_test_ability";
    second_ability.arm_ticks = 2U;
    second_ability.duration_ticks = 10U;
    bundle.archetypes.abilities.push_back(second_ability);
    BirdArchetype second_bird = bundle.archetypes.birds.front();
    second_bird.id = BirdArchetypeId{2};
    second_bird.key = "second_ability_bird";
    second_bird.ability_id = AbilityId{2};
    bundle.archetypes.birds.push_back(second_bird);
    bundle.level.bird_roster = {{BirdArchetypeId{2}, 1U}};
    auto created = SimulationSession::create(
        bundle.materials, bundle.archetypes, bundle.level);
    NINHO_SIM_REQUIRE(created.ok());
    auto session = std::move(created.value);
    launch(*session);
    const TickIndex launched = detail::SessionTestFacade::projectile_launch_tick(*session);
    NINHO_SIM_REQUIRE(session->enqueue(ActivateAbilityCommand{}).ok());
    NINHO_SIM_REQUIRE(session->tick().ok());
    NINHO_SIM_REQUIRE(!detail::SessionTestFacade::ability_requested(*session));
    NINHO_SIM_REQUIRE(session->enqueue(ActivateAbilityCommand{}).ok());
    NINHO_SIM_REQUIRE(session->tick().ok());
    NINHO_SIM_REQUIRE(session->state().tick == TickIndex{launched.value() + 2U});
    NINHO_SIM_REQUIRE(detail::SessionTestFacade::ability_id(*session) == AbilityId{2});
    NINHO_SIM_REQUIRE(detail::SessionTestFacade::ability_end_tick(*session)
        == TickIndex{launched.value() + 11U});
}

NINHO_SIM_TEST("launch fsm canonical roster ordering selects birds by typed id and count")
{
    ContentBundle first = load_bundle();
    BirdArchetype second_bird = first.archetypes.birds.front();
    second_bird.id = BirdArchetypeId{2};
    second_bird.key = "second_test_bird";
    first.archetypes.birds.push_back(second_bird);
    first.level.bird_roster = {{BirdArchetypeId{2}, 2U}, {BirdArchetypeId{1}, 1U}};

    ContentBundle reordered = first;
    std::ranges::reverse(reordered.level.bird_roster);
    ContentBundle different_counts = first;
    different_counts.level.bird_roster = {
        {BirdArchetypeId{2}, 1U}, {BirdArchetypeId{1}, 2U}};

    const auto make = [](const ContentBundle& bundle) {
        auto value = SimulationSession::create(
            bundle.materials, bundle.archetypes, bundle.level);
        NINHO_SIM_REQUIRE(value.ok());
        return std::move(value.value);
    };
    auto a = make(first);
    auto b = make(reordered);
    auto c = make(different_counts);
    NINHO_SIM_REQUIRE(a->canonical_hash_v2() == b->canonical_hash_v2());
    NINHO_SIM_REQUIRE(a->canonical_hash_v2() != c->canonical_hash_v2());
    NINHO_SIM_REQUIRE(detail::SessionTestFacade::next_bird_archetype_id(*a)
        == BirdArchetypeId{1});
    NINHO_SIM_REQUIRE(detail::SessionTestFacade::next_bird_archetype_id(*c)
        == BirdArchetypeId{1});
    NINHO_SIM_REQUIRE(a->enqueue(BeginAimCommand{}).ok());
    NINHO_SIM_REQUIRE(a->enqueue(SetAimCommand{default_aim()}).ok());
    NINHO_SIM_REQUIRE(a->enqueue(LaunchCommand{}).ok());
    NINHO_SIM_REQUIRE(a->tick().ok());
    NINHO_SIM_REQUIRE(c->enqueue(BeginAimCommand{}).ok());
    NINHO_SIM_REQUIRE(c->enqueue(SetAimCommand{default_aim()}).ok());
    NINHO_SIM_REQUIRE(c->enqueue(LaunchCommand{}).ok());
    NINHO_SIM_REQUIRE(c->tick().ok());
    NINHO_SIM_REQUIRE(a->events()[0].bird_archetype_id == BirdArchetypeId{1});
    NINHO_SIM_REQUIRE(c->events()[0].bird_archetype_id == BirdArchetypeId{1});
    NINHO_SIM_REQUIRE(detail::SessionTestFacade::next_bird_archetype_id(*a)
        == BirdArchetypeId{2});
    NINHO_SIM_REQUIRE(detail::SessionTestFacade::next_bird_archetype_id(*c)
        == BirdArchetypeId{1});
}

NINHO_SIM_TEST("launch fsm restart restores roster launch tick and stable projectile identity")
{
    auto session = create_session();
    launch(*session);
    NINHO_SIM_REQUIRE(session->birds_remaining() == 2U);
    NINHO_SIM_REQUIRE(session->restart().ok());
    NINHO_SIM_REQUIRE(session->birds_remaining() == 3U);
    NINHO_SIM_REQUIRE(detail::SessionTestFacade::next_bird_archetype_id(*session)
        == BirdArchetypeId{1});
    NINHO_SIM_REQUIRE(detail::SessionTestFacade::projectile_launch_tick(*session) == TickIndex{});
    launch(*session);
    NINHO_SIM_REQUIRE(session->events().size() == 1U);
    NINHO_SIM_REQUIRE(session->events()[0].entity_id == EntityId{0x80000000U});
    NINHO_SIM_REQUIRE(detail::SessionTestFacade::projectile_launch_tick(*session)
        == session->state().tick);
}

NINHO_SIM_TEST("launch fsm parser rejects runtime entity namespace in every manifest location")
{
    using nlohmann::json;
    const auto base = json::parse(to_canonical_json(load_bundle().level));
    const auto require_rejected = [&](auto mutate, std::string_view pointer) {
        auto value = base;
        mutate(value);
        const auto parsed = parse_level_manifest(value.dump());
        NINHO_SIM_REQUIRE(!parsed.ok());
        NINHO_SIM_REQUIRE(parsed.error.code == ContentErrorCode::OutOfRange);
        NINHO_SIM_REQUIRE(parsed.error.pointer == pointer);
    };
    require_rejected([](json& value) { value["planet"]["entity_id"] = 0x80000000U; },
        "/planet/entity_id");
    require_rejected([](json& value) { value["bodies"][0]["entity_id"] = 0x80000000U; },
        "/bodies/0/entity_id");
    require_rejected([](json& value) {
        value["objectives"][0]["target_entity_id"] = 0x80000000U;
    }, "/objectives/0/target_entity_id");
}

NINHO_SIM_TEST("launch fsm typed bundle rejects runtime entity namespace atomically")
{
    const auto require_rejected = [](ContentBundle value, std::string_view pointer) {
        const auto result = make_content_bundle(
            value.materials, value.archetypes, value.level);
        NINHO_SIM_REQUIRE(!result.ok());
        NINHO_SIM_REQUIRE(result.error.code == ContentErrorCode::OutOfRange);
        NINHO_SIM_REQUIRE(result.error.pointer == pointer);
    };
    auto planet = load_bundle();
    planet.level.planet.entity_id = EntityId{0x80000000U};
    require_rejected(std::move(planet), "/planet/entity_id");
    auto body = load_bundle();
    body.level.bodies[0].entity_id = EntityId{0x80000000U};
    require_rejected(std::move(body), "/bodies/0/entity_id");
    auto objective = load_bundle();
    objective.level.objectives[0].target_entity_id = EntityId{0x80000000U};
    require_rejected(std::move(objective), "/objectives/0/target_entity_id");
}

NINHO_SIM_TEST("launch fsm allocates three stable runtime ids and restart resets the ordinal")
{
    auto session = create_session();
    const std::array expected{
        EntityId{0x80000000U}, EntityId{0x80000001U}, EntityId{0x80000002U}};
    for (const EntityId id : expected) {
        launch(*session);
        NINHO_SIM_REQUIRE(session->events()[0].entity_id == id);
        NINHO_SIM_REQUIRE(std::ranges::any_of(session->snapshots(), [&](const auto& snapshot) {
            return snapshot.entity_id == id;
        }));
        settle_to_evaluation(*session);
        NINHO_SIM_REQUIRE(session->tick().ok());
    }
    NINHO_SIM_REQUIRE(session->restart().ok());
    launch(*session);
    NINHO_SIM_REQUIRE(session->events()[0].entity_id == EntityId{0x80000000U});
}

NINHO_SIM_TEST("launch fsm rejects runtime entity ordinal overflow before physics mutation")
{
    auto session = create_session();
    const auto bodies_before = session->physics_metrics().body_count;
    detail::SessionTestFacade::set_launch_ordinal(*session, 0x80000000U);
    enter_aim(*session);
    NINHO_SIM_REQUIRE(session->enqueue(LaunchCommand{}).ok());
    const auto status = session->tick();
    NINHO_SIM_REQUIRE(!status.ok());
    NINHO_SIM_REQUIRE(status.error.code == ContentErrorCode::ResourceLimit);
    NINHO_SIM_REQUIRE(session->physics_metrics().body_count == bodies_before);
}

NINHO_SIM_TEST("launch fsm removes obsolete projectile records across multiple launches")
{
    auto session = create_session();
    const auto baseline = detail::SessionTestFacade::body_record_count(*session);
    for (int shot = 0; shot < 3; ++shot) {
        launch(*session);
        NINHO_SIM_REQUIRE(detail::SessionTestFacade::body_record_count(*session) == baseline + 1U);
        settle_to_evaluation(*session);
        NINHO_SIM_REQUIRE(detail::SessionTestFacade::body_record_count(*session) == baseline);
        NINHO_SIM_REQUIRE(session->tick().ok());
    }
}

}
