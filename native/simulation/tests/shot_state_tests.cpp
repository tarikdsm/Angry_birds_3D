#include "test_framework.hpp"

#include "ability_runtime.hpp"
#include "session_internal.hpp"
#include "session_test_facade.hpp"
#include "shot_state.hpp"
#include "ninho/simulation/session.hpp"

#include <algorithm>
#include <array>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <memory>
#include <sstream>
#include <span>
#include <string>
#include <type_traits>
#include <variant>

namespace {

using namespace ninho::simulation;

std::uint64_t fnv1a64(std::span<const std::uint8_t> bytes) noexcept
{
    std::uint64_t hash = 14695981039346656037ULL;
    for (const std::uint8_t byte : bytes) {
        hash ^= byte;
        hash *= 1099511628211ULL;
    }
    return hash;
}

std::uint32_t read_u32(
    std::span<const std::uint8_t> bytes, std::size_t& offset)
{
    NINHO_SIM_REQUIRE(offset + sizeof(std::uint32_t) <= bytes.size());
    std::uint32_t value{};
    for (std::size_t byte = 0; byte < sizeof(value); ++byte) {
        value |= static_cast<std::uint32_t>(bytes[offset++]) << (byte * 8U);
    }
    return value;
}

MaterialCatalog materials()
{
    MaterialCatalog result;
    result.schema_version = result.source_schema_version = 2U;
    result.surfaces.push_back({SurfaceId{1001}, "bird", 1000.0, 0.4, 0.1});
    return result;
}

ArchetypeCatalog archetypes()
{
    ArchetypeCatalog result;
    result.schema_version = result.source_schema_version = 2U;
    result.presentation_ids = {"bird_a", "bird_b", "icon", "animation"};
    result.score_ids = {"bird_score"};
    AbilityArchetype ability;
    ability.id = AbilityId{1};
    ability.key = ability.kind = "gravity_field";
    ability.kind_v2 = AbilityKind::GravityField;
    ability.arm_ticks = 9U;
    ability.duration_ticks = 75U;
    ability.radius_m = 4.0;
    ability.max_body_mass_kg = 150.0;
    ability.max_bodies = 20U;
    ability.max_acceleration_m_s2 = 12.0;
    ability.pulse_speed_m_s = 4.0;
    ability.payload = GravityFieldAbilityDefinition{
        9U, 75U, 4.0, 150.0, 20U, 12.0, 4.0};
    result.abilities.push_back(ability);
    for (std::uint32_t id = 1U; id <= 2U; ++id) {
        BirdArchetype bird;
        bird.id = BirdArchetypeId{id};
        bird.key = id == 1U ? "bird_a" : "bird_b";
        bird.ability_id = AbilityId{1};
        bird.surface_id = SurfaceId{1001};
        bird.mass_kg = 5.0 + id;
        bird.radius_m = 0.25;
        bird.friction = 0.4;
        bird.restitution = 0.1;
        bird.bullet = true;
        bird.projectile_visual_id = bird.key;
        bird.launch_speed_cap_m_s = 40.0;
        bird.score_id = "bird_score";
        bird.icon_id = "icon";
        bird.animation_id = "animation";
        result.birds.push_back(std::move(bird));
    }
    return result;
}

LevelManifest level(std::array<double, 3> acceleration = {0.0, -9.81, 0.0},
    std::vector<BirdArchetypeId> queue = {BirdArchetypeId{1}, BirdArchetypeId{2}})
{
    LevelManifest result;
    result.schema_version = result.source_schema_version = 2U;
    result.id = "canonical_v3_minimal";
    result.world_id = "earth";
    result.world = UniformWorldDefinition{
        .acceleration_m_s2 = acceleration,
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
    result.bird_queue = std::move(queue);
    return result;
}

std::unique_ptr<SimulationSession> create_session(LevelManifest source = level())
{
    auto created = SimulationSession::create(materials(), archetypes(), source);
    NINHO_SIM_REQUIRE(created.ok());
    return std::move(created.value);
}

void launch_v2(SimulationSession& session)
{
    NINHO_SIM_REQUIRE(session.enqueue(
        BeginGrabCommand{{1.0F, 0.0F, 0.0F}}).ok());
    NINHO_SIM_REQUIRE(session.enqueue(SetPullCommand{-1.25, 0.5}).ok());
    NINHO_SIM_REQUIRE(session.tick().ok());
    NINHO_SIM_REQUIRE(session.enqueue(ReleaseBirdCommand{}).ok());
    NINHO_SIM_REQUIRE(session.tick().ok());
}

}

static_assert(std::is_same_v<
    decltype(std::declval<const SimulationSession&>().shot_state()),
    std::optional<ShotStateView>>);

NINHO_SIM_TEST("shot state public view is a value snapshot with ordered membership")
{
    using detail::SessionTestFacade;
    auto session = create_session();
    NINHO_SIM_REQUIRE(!session->shot_state().has_value());
    launch_v2(*session);

    const auto canonical_before_view = session->canonical_state_v3();
    const auto view = session->shot_state();
    NINHO_SIM_REQUIRE(view.has_value());
    NINHO_SIM_REQUIRE(view->shot_id == 1U);
    NINHO_SIM_REQUIRE(view->bird_archetype_id == BirdArchetypeId{1});
    NINHO_SIM_REQUIRE(view->ability_id == AbilityId{1});
    NINHO_SIM_REQUIRE(view->launch_tick == TickIndex{2});
    NINHO_SIM_REQUIRE((view->locked_plane.camera_right
        == ninho::physics::Vec3{1.0F, 0.0F, 0.0F}));
    NINHO_SIM_REQUIRE(view->pull_horizontal_m == -1.25);
    NINHO_SIM_REQUIRE(view->pull_vertical_m == 0.5);
    NINHO_SIM_REQUIRE(!view->activation_consumed);
    NINHO_SIM_REQUIRE(view->ability_readiness == AbilityReadiness::Arming);
    NINHO_SIM_REQUIRE((view->projectile_ids
        == std::vector{EntityId{0x80000000U}}));
    NINHO_SIM_REQUIRE(session->canonical_state_v3() == canonical_before_view);

    NINHO_SIM_REQUIRE(SessionTestFacade::append_projectile_for_testing(
        *session, EntityId{0x80000003U}));
    NINHO_SIM_REQUIRE(SessionTestFacade::append_projectile_for_testing(
        *session, EntityId{0x80000002U}));
    const auto expanded = session->shot_state();
    NINHO_SIM_REQUIRE(expanded.has_value());
    NINHO_SIM_REQUIRE((expanded->projectile_ids == std::vector{
        EntityId{0x80000000U}, EntityId{0x80000002U}, EntityId{0x80000003U}}));
    NINHO_SIM_REQUIRE(std::ranges::count(
        session->snapshots(), true, &EntitySnapshot::is_projectile) == 1);
    NINHO_SIM_REQUIRE((view->projectile_ids
        == std::vector{EntityId{0x80000000U}}));
}

NINHO_SIM_TEST("shot state public view follows authoritative resolution lifecycle")
{
    using detail::SessionTestFacade;
    auto session = create_session();
    launch_v2(*session);
    const ShotStateView launched = *session->shot_state();

    SessionTestFacade::finish_projectile(*session);
    NINHO_SIM_REQUIRE(session->tick().ok());
    NINHO_SIM_REQUIRE(session->state().phase == SessionPhase::Resolution);
    const auto resolution = session->shot_state();
    NINHO_SIM_REQUIRE(resolution.has_value());
    NINHO_SIM_REQUIRE(resolution->shot_id == launched.shot_id);
    NINHO_SIM_REQUIRE(resolution->bird_archetype_id == launched.bird_archetype_id);
    NINHO_SIM_REQUIRE(resolution->locked_plane == launched.locked_plane);
    NINHO_SIM_REQUIRE(resolution->projectile_ids == launched.projectile_ids);
    NINHO_SIM_REQUIRE(resolution->ability_readiness == AbilityReadiness::Spent);
    NINHO_SIM_REQUIRE(std::ranges::none_of(
        session->snapshots(), &EntitySnapshot::is_projectile));

    for (int tick = 0;
         tick < 64 && session->state().phase != SessionPhase::Evaluation;
         ++tick) {
        NINHO_SIM_REQUIRE(session->tick().ok());
        NINHO_SIM_REQUIRE(session->shot_state().has_value());
    }
    NINHO_SIM_REQUIRE(session->state().phase == SessionPhase::Evaluation);
    const auto evaluation = session->shot_state();
    NINHO_SIM_REQUIRE(evaluation.has_value());
    NINHO_SIM_REQUIRE(evaluation->shot_id == launched.shot_id);
    NINHO_SIM_REQUIRE(evaluation->bird_archetype_id == launched.bird_archetype_id);
    NINHO_SIM_REQUIRE(evaluation->locked_plane == launched.locked_plane);
    NINHO_SIM_REQUIRE(evaluation->projectile_ids == launched.projectile_ids);
    NINHO_SIM_REQUIRE(evaluation->ability_readiness == AbilityReadiness::Spent);

    NINHO_SIM_REQUIRE(session->tick().ok());
    NINHO_SIM_REQUIRE(session->state().phase == SessionPhase::Inspection);
    NINHO_SIM_REQUIRE(!session->shot_state().has_value());
}

NINHO_SIM_TEST("shot state public view preserves atomic reconfigure and clears restart state")
{
    auto session = create_session();
    launch_v2(*session);
    const auto launched = session->shot_state();
    const auto canonical_launched = session->canonical_state_v3();

    LevelManifest invalid = level();
    invalid.bird_queue.clear();
    NINHO_SIM_REQUIRE(!session->reconfigure(materials(), archetypes(), invalid).ok());
    NINHO_SIM_REQUIRE(session->shot_state() == launched);
    NINHO_SIM_REQUIRE(session->canonical_state_v3() == canonical_launched);

    NINHO_SIM_REQUIRE(session->restart().ok());
    NINHO_SIM_REQUIRE(!session->shot_state().has_value());
    launch_v2(*session);
    NINHO_SIM_REQUIRE(session->shot_state().has_value());
    NINHO_SIM_REQUIRE(session->reconfigure(
        materials(), archetypes(), level({0.0, -9.5, 0.0})).ok());
    NINHO_SIM_REQUIRE(!session->shot_state().has_value());
}

NINHO_SIM_TEST("shot state owns the locked plane runtime and ordered projectiles")
{
    ShotState shot{
        ProjectileState{EntityId{0x80000002U}, {}, false}};
    NINHO_SIM_REQUIRE(shot.has_valid_projectiles());
    NINHO_SIM_REQUIRE(shot.projectiles().size() == 1U);
    shot.shot_id = 7U;
    shot.bird_archetype_id = BirdArchetypeId{9};
    shot.ability_id = AbilityId{11};
    shot.launch_tick = TickIndex{13};
    shot.locked_plane.camera_right = {1.0F, 0.0F, 0.0F};
    shot.locked_plane.up = {0.0F, 1.0F, 0.0F};
    shot.locked_plane.horizontal = {1.0F, 0.0F, 0.0F};
    shot.locked_plane.plane_normal = {0.0F, 0.0F, 1.0F};
    shot.pull_horizontal_m = -1.25;
    shot.pull_vertical_m = 0.5;
    shot.activation_consumed = true;
    shot.runtime = GravityFieldAbilityRuntime{
        .start_tick = TickIndex{17},
        .end_tick = TickIndex{23},
        .active = true,
    };
    NINHO_SIM_REQUIRE(shot.insert_projectile(
        ProjectileState{EntityId{0x80000001U}, {}, true}));

    ShotState alternate{
        ProjectileState{EntityId{0x80000001U}, {}, true}};
    NINHO_SIM_REQUIRE(alternate.insert_projectile(
        ProjectileState{EntityId{0x80000002U}, {}, false}));

    NINHO_SIM_REQUIRE(shot.shot_id == 7U);
    NINHO_SIM_REQUIRE(shot.bird_archetype_id == BirdArchetypeId{9});
    NINHO_SIM_REQUIRE(shot.activation_consumed);
    NINHO_SIM_REQUIRE(std::holds_alternative<GravityFieldAbilityRuntime>(shot.runtime));
    NINHO_SIM_REQUIRE(shot.has_valid_projectiles());
    NINHO_SIM_REQUIRE(shot.projectiles().size() == 2U);
    NINHO_SIM_REQUIRE(std::ranges::equal(
        shot.projectiles(), alternate.projectiles()));
    NINHO_SIM_REQUIRE(shot.primary_projectile() != nullptr);
    NINHO_SIM_REQUIRE(shot.primary_projectile()->entity_id()
        == EntityId{0x80000001U});

    const std::vector<ProjectileState> before_duplicate{
        shot.projectiles().begin(), shot.projectiles().end()};
    NINHO_SIM_REQUIRE(!shot.insert_projectile(
        ProjectileState{EntityId{0x80000001U}, {9U, 9U}, false}));
    NINHO_SIM_REQUIRE(std::ranges::equal(
        shot.projectiles(), before_duplicate));

    *shot.primary_projectile() =
        shot.primary_projectile()->copy_with_entity_id(EntityId{0x80000002U});
    NINHO_SIM_REQUIRE(shot.primary_projectile()->entity_id()
        == EntityId{0x80000001U});
    NINHO_SIM_REQUIRE(shot.has_valid_projectiles());

    NINHO_SIM_REQUIRE(shot.replace_projectile(
        ProjectileState{EntityId{0x80000001U}, {9U, 9U}, false}));
    NINHO_SIM_REQUIRE(!shot.primary_projectile()->bullet);
    NINHO_SIM_REQUIRE(shot.erase_projectile(EntityId{0x80000002U}));
    NINHO_SIM_REQUIRE(shot.projectiles().size() == 1U);
    NINHO_SIM_REQUIRE(!shot.erase_projectile(EntityId{0x80000001U}));
    NINHO_SIM_REQUIRE(shot.has_valid_projectiles());

    const ShotState copied{shot};
    NINHO_SIM_REQUIRE(copied == shot);
    NINHO_SIM_REQUIRE(copied.has_valid_projectiles());
}

NINHO_SIM_TEST("shot state canonical v3 api is separate from legacy v2")
{
    static_assert(!std::is_default_constructible_v<ShotState>);
    static_assert(std::is_copy_constructible_v<ShotState>);
    static_assert(!std::is_move_constructible_v<ShotState>);
    static_assert(!std::is_copy_assignable_v<ShotState>);
    static_assert(!std::is_move_assignable_v<ShotState>);
    static_assert(requires(const SimulationSession& session) {
        session.canonical_state_v3();
        session.canonical_hash_v3();
    });
}

NINHO_SIM_TEST("shot state canonical tags are explicit and append only")
{
    using ninho::simulation::detail::canonical_tag_of;
    NINHO_SIM_REQUIRE(canonical_tag_of(PlayerCommand{BeginAimCommand{}}) == 0U);
    NINHO_SIM_REQUIRE(canonical_tag_of(PlayerCommand{SetAimCommand{}}) == 1U);
    NINHO_SIM_REQUIRE(canonical_tag_of(PlayerCommand{LaunchCommand{}}) == 2U);
    NINHO_SIM_REQUIRE(canonical_tag_of(PlayerCommand{ActivateAbilityCommand{}}) == 3U);
    NINHO_SIM_REQUIRE(canonical_tag_of(PlayerCommand{CancelAimCommand{}}) == 4U);
    NINHO_SIM_REQUIRE(canonical_tag_of(PlayerCommand{BeginGrabCommand{}}) == 5U);
    NINHO_SIM_REQUIRE(canonical_tag_of(PlayerCommand{SetPullCommand{}}) == 6U);
    NINHO_SIM_REQUIRE(canonical_tag_of(PlayerCommand{ReleaseBirdCommand{}}) == 7U);
    NINHO_SIM_REQUIRE(canonical_tag_of(PlayerCommand{CancelGrabCommand{}}) == 8U);

    NINHO_SIM_REQUIRE(canonical_tag_of(WorldDefinition{UniformWorldDefinition{}}) == 0U);
    NINHO_SIM_REQUIRE(canonical_tag_of(WorldDefinition{RadialWorldDefinition{}}) == 1U);
    NINHO_SIM_REQUIRE(canonical_tag_of(
        AbilityRuntime{GravityFieldAbilityRuntime{}}) == 0U);
    NINHO_SIM_REQUIRE(canonical_tag_of(AbilityRuntime{MassBoostAbilityRuntime{}}) == 1U);
    NINHO_SIM_REQUIRE(canonical_tag_of(AbilityRuntime{SpeedBoostAbilityRuntime{}}) == 2U);
    NINHO_SIM_REQUIRE(canonical_tag_of(AbilityRuntime{ExplosionAbilityRuntime{}}) == 3U);
    NINHO_SIM_REQUIRE(canonical_tag_of(AbilityRuntime{SplitAbilityRuntime{}}) == 4U);

    NINHO_SIM_REQUIRE(canonical_tag_of(SessionPhase::Inspection) == 0U);
    NINHO_SIM_REQUIRE(canonical_tag_of(SessionPhase::Faulted) == 6U);
    NINHO_SIM_REQUIRE(canonical_tag_of(SessionPhase::Grabbed) == 7U);
    NINHO_SIM_REQUIRE(canonical_tag_of(DomainEventKind::BirdLaunched) == 0U);
    NINHO_SIM_REQUIRE(canonical_tag_of(DomainEventKind::PieceFractured) == 12U);
}

NINHO_SIM_TEST("shot state canonical v3 publishes schema two without reviving v2")
{
    auto first = create_session();
    auto second = create_session();
    NINHO_SIM_REQUIRE(first->canonical_state_v2().empty());
    NINHO_SIM_REQUIRE(first->canonical_hash_v2() == 0U);
    NINHO_SIM_REQUIRE(!first->canonical_state_v3().empty());
    NINHO_SIM_REQUIRE(first->canonical_hash_v3() != 0U);
    NINHO_SIM_REQUIRE(fnv1a64(first->canonical_state_v3()) == first->canonical_hash_v3());
    NINHO_SIM_REQUIRE(first->canonical_state_v3() == second->canonical_state_v3());
    std::size_t offset{};
    const std::span<const std::uint8_t> bytes = first->canonical_state_v3();
    const std::uint32_t name_size = read_u32(bytes, offset);
    NINHO_SIM_REQUIRE(offset + name_size <= bytes.size());
    const std::string name{reinterpret_cast<const char*>(bytes.data() + offset), name_size};
    offset += name_size;
    NINHO_SIM_REQUIRE(name == "canonical_state_v3");
    NINHO_SIM_REQUIRE(read_u32(bytes, offset) == 3U);
    constexpr std::uint64_t canonical_state_v3_minimal_golden =
        12470858622598513254ULL;
    if (first->canonical_hash_v3() != canonical_state_v3_minimal_golden) {
        test::fail(__FILE__, __LINE__, "canonical_state_v3 minimal golden: actual="
            + std::to_string(first->canonical_hash_v3()));
    }

    const auto frozen = first->canonical_state_v3();
    for (int repetition = 0; repetition < 50; ++repetition) {
        detail::SessionTestFacade::refresh_canonical_state(*first);
        NINHO_SIM_REQUIRE(first->canonical_state_v3() == frozen);
    }
}

NINHO_SIM_TEST("shot state serializers exclude variant positions handles and addresses")
{
    const auto source_path = std::filesystem::path{NINHO_SOURCE_DIR}
        / "native/simulation/src/canonical_state.cpp";
    std::ifstream stream{source_path, std::ios::binary};
    NINHO_SIM_REQUIRE(stream.is_open());
    std::ostringstream buffer;
    buffer << stream.rdbuf();
    const std::string source = buffer.str();
    NINHO_SIM_REQUIRE(source.find(".index()") == std::string::npos);
    NINHO_SIM_REQUIRE(source.find("physics_handle") == std::string::npos);
    NINHO_SIM_REQUIRE(source.find("BodyHandle") == std::string::npos);
}

NINHO_SIM_TEST("shot state canonical v3 includes ordered queue locked camera pull and shot runtime")
{
    auto first = create_session();
    auto reordered = create_session(level(
        {0.0, -9.81, 0.0}, {BirdArchetypeId{2}, BirdArchetypeId{1}}));
    NINHO_SIM_REQUIRE(first->canonical_hash_v3() != reordered->canonical_hash_v3());

    NINHO_SIM_REQUIRE(first->enqueue(BeginGrabCommand{{1.0F, 0.0F, 0.0F}}).ok());
    NINHO_SIM_REQUIRE(reordered->enqueue(BeginGrabCommand{{0.0F, 0.0F, 1.0F}}).ok());
    NINHO_SIM_REQUIRE(first->tick().ok());
    NINHO_SIM_REQUIRE(reordered->tick().ok());
    NINHO_SIM_REQUIRE(first->canonical_hash_v3() != reordered->canonical_hash_v3());

    const auto before_pull = first->canonical_hash_v3();
    NINHO_SIM_REQUIRE(first->enqueue(SetPullCommand{-1.0, 0.5}).ok());
    NINHO_SIM_REQUIRE(first->tick().ok());
    NINHO_SIM_REQUIRE(first->canonical_hash_v3() != before_pull);
    const auto before_release = first->canonical_hash_v3();
    NINHO_SIM_REQUIRE(first->enqueue(ReleaseBirdCommand{}).ok());
    NINHO_SIM_REQUIRE(first->tick().ok());
    NINHO_SIM_REQUIRE(first->canonical_hash_v3() != before_release);
}

NINHO_SIM_TEST("shot state canonical v3 normalizes negative zero at one e minus five")
{
    auto positive_zero = create_session(level({0.0, -9.81, 0.0}));
    auto negative_zero = create_session(level({-0.0, -9.81, 0.0}));
    auto below_quantum = create_session(level({0.000004, -9.81, 0.0}));
    auto above_quantum = create_session(level({0.000006, -9.81, 0.0}));
    NINHO_SIM_REQUIRE(positive_zero->canonical_state_v3()
        == negative_zero->canonical_state_v3());
    NINHO_SIM_REQUIRE(positive_zero->canonical_state_v3()
        == below_quantum->canonical_state_v3());
    NINHO_SIM_REQUIRE(positive_zero->canonical_hash_v3()
        != above_quantum->canonical_hash_v3());
}

NINHO_SIM_TEST("shot state canonical v3 serializes exited world runtime and projectile order")
{
    using detail::SessionTestFacade;
    auto session = create_session();
    NINHO_SIM_REQUIRE(SessionTestFacade::add_static_sphere(
        *session, EntityId{9000}, {2.0F, 2.0F, 0.0F}, 0.5));
    NINHO_SIM_REQUIRE(session->tick().ok());
    const auto inside = session->canonical_state_v3();
    NINHO_SIM_REQUIRE(SessionTestFacade::set_snapshot_exited_world(
        *session, EntityId{9000}, PartId{1}, true));
    SessionTestFacade::refresh_canonical_state(*session);
    NINHO_SIM_REQUIRE(session->canonical_state_v3() != inside);
    NINHO_SIM_REQUIRE(SessionTestFacade::set_snapshot_exited_world(
        *session, EntityId{9000}, PartId{1}, false));
    SessionTestFacade::refresh_canonical_state(*session);
    NINHO_SIM_REQUIRE(session->canonical_state_v3() == inside);

    const auto launch = [](SimulationSession& target) {
        NINHO_SIM_REQUIRE(target.enqueue(
            BeginGrabCommand{{1.0F, 0.0F, 0.0F}}).ok());
        NINHO_SIM_REQUIRE(target.enqueue(SetPullCommand{-1.0, 0.0}).ok());
        NINHO_SIM_REQUIRE(target.tick().ok());
        NINHO_SIM_REQUIRE(target.enqueue(ReleaseBirdCommand{}).ok());
        NINHO_SIM_REQUIRE(target.tick().ok());
    };
    launch(*session);
    NINHO_SIM_REQUIRE(SessionTestFacade::append_projectile_for_testing(
        *session, EntityId{0x80000003U}));
    NINHO_SIM_REQUIRE(SessionTestFacade::append_projectile_for_testing(
        *session, EntityId{0x80000002U}));
    SessionTestFacade::refresh_canonical_state(*session);
    const auto ordered = session->canonical_state_v3();
    NINHO_SIM_REQUIRE(!SessionTestFacade::append_projectile_for_testing(
        *session, EntityId{0x80000002U}));
    SessionTestFacade::refresh_canonical_state(*session);
    NINHO_SIM_REQUIRE(session->canonical_state_v3() == ordered);

    auto alternate_order = create_session();
    NINHO_SIM_REQUIRE(SessionTestFacade::add_static_sphere(
        *alternate_order, EntityId{9000}, {2.0F, 2.0F, 0.0F}, 0.5));
    NINHO_SIM_REQUIRE(alternate_order->tick().ok());
    launch(*alternate_order);
    NINHO_SIM_REQUIRE(SessionTestFacade::append_projectile_for_testing(
        *alternate_order, EntityId{0x80000002U}));
    NINHO_SIM_REQUIRE(SessionTestFacade::append_projectile_for_testing(
        *alternate_order, EntityId{0x80000003U}));
    SessionTestFacade::refresh_canonical_state(*alternate_order);
    NINHO_SIM_REQUIRE(alternate_order->canonical_state_v3() == ordered);

    const auto runtime_base = session->canonical_state_v3();
    SessionTestFacade::set_shot_runtime_for_testing(
        *session, true, false, std::nullopt, std::nullopt);
    SessionTestFacade::refresh_canonical_state(*session);
    NINHO_SIM_REQUIRE(session->canonical_state_v3() != runtime_base);
    const auto consumed = session->canonical_state_v3();
    SessionTestFacade::set_shot_runtime_for_testing(
        *session, true, true, TickIndex{20}, TickIndex{30});
    SessionTestFacade::refresh_canonical_state(*session);
    NINHO_SIM_REQUIRE(session->canonical_state_v3() != consumed);
}

NINHO_SIM_TEST("shot state canonical v3 cache invalidates atomically and rejects nonfinite state")
{
    using detail::SessionTestFacade;
    auto session = create_session();
    const auto initial = session->canonical_state_v3();
    NINHO_SIM_REQUIRE(session->enqueue(BeginGrabCommand{{1.0F, 0.0F, 0.0F}}).ok());
    NINHO_SIM_REQUIRE(session->canonical_state_v3() != initial);
    NINHO_SIM_REQUIRE(session->tick().ok());
    NINHO_SIM_REQUIRE(session->restart().ok());
    NINHO_SIM_REQUIRE(session->canonical_state_v3() == initial);

    LevelManifest changed = level({0.0, -9.5, 0.0});
    NINHO_SIM_REQUIRE(session->reconfigure(materials(), archetypes(), changed).ok());
    NINHO_SIM_REQUIRE(session->canonical_state_v3() != initial);
    const auto reconfigured = session->canonical_state_v3();
    LevelManifest invalid = changed;
    invalid.bird_queue.clear();
    NINHO_SIM_REQUIRE(!session->reconfigure(materials(), archetypes(), invalid).ok());
    NINHO_SIM_REQUIRE(session->canonical_state_v3() == reconfigured);

    NINHO_SIM_REQUIRE(SessionTestFacade::add_dynamic_sphere(
        *session, EntityId{9100}, PartId{1}, {0.0F, 2.0F, 0.0F}, 5.0));
    SessionTestFacade::override_next_snapshot_mass(*session, 1.0e14);
    const SessionStatus status = session->tick();
    NINHO_SIM_REQUIRE(!status.ok());
    NINHO_SIM_REQUIRE(status.error.code == ContentErrorCode::InternalError);
    NINHO_SIM_REQUIRE(session->canonical_state_v3() == reconfigured);
}
