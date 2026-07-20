#include "test_framework.hpp"

#include "launcher_system.hpp"
#include "session_test_facade.hpp"
#include "ninho/simulation/commands.hpp"
#include "ninho/simulation/session.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <limits>
#include <ranges>
#include <utility>

namespace {

using namespace ninho::simulation;

constexpr double epsilon = 1.0e-6;

SlingshotDefinition slingshot(std::array<double, 3> rest = {-4.0, 2.0, 0.0})
{
    return {
        .asset_id = "launcher",
        .rest_position_m = rest,
        .rest_rotation_xyzw = {0.0, 0.0, 0.0, 1.0},
        .spring_constant_n_m = 5200.0,
        .energy_efficiency = 0.90,
        .minimum_extension_m = 0.20,
        .maximum_extension_m = 4.25,
        .plane_policy = "gravity_vertical_camera_yaw",
        .projectile_clearance_m = 0.0,
        .speed_ceiling_m_s = 100.0,
    };
}

WorldDefinition uniform_world()
{
    return UniformWorldDefinition{
        .acceleration_m_s2 = {0.0, -9.81, 0.0},
        .bounds_min_m = {-24.0, -12.0, -12.0},
        .bounds_max_m = {48.0, 32.0, 12.0},
    };
}

WorldDefinition radial_world()
{
    return RadialWorldDefinition{
        .center_m = {10.0, 0.0, 0.0},
        .reference_radius_m = 2.0,
        .reference_acceleration_m_s2 = 9.0,
        .bounds_radius_m = 20.0,
    };
}

detail::LauncherProjectile projectile(double mass_kg = 5.0, double cap_m_s = 100.0)
{
    return {.mass_kg = mass_kg, .speed_cap_m_s = cap_m_s};
}

bool close(double lhs, double rhs, double tolerance = epsilon)
{
    return std::abs(lhs - rhs) <= tolerance;
}

bool close(ninho::physics::Vec3 lhs, ninho::physics::Vec3 rhs,
    float tolerance = 1.0e-5F)
{
    return ninho::physics::length(lhs - rhs) <= tolerance;
}

MaterialCatalog v2_materials()
{
    MaterialCatalog result;
    result.schema_version = result.source_schema_version = 2U;
    result.surfaces.push_back({SurfaceId{1001}, "ground", 1000.0, 0.7, 0.0});
    result.surfaces.push_back({SurfaceId{1002}, "bird", 1000.0, 0.4, 0.1});
    return result;
}

ArchetypeCatalog v2_archetypes(double bird_mass_kg = 5.0, double cap_m_s = 100.0)
{
    ArchetypeCatalog result;
    result.schema_version = result.source_schema_version = 2U;
    result.presentation_ids = {"bird", "icon", "animation"};
    result.score_ids = {"bird_score"};
    AbilityArchetype ability;
    ability.id = AbilityId{1};
    ability.key = "gravity_field";
    ability.kind = "gravity_field";
    ability.kind_v2 = AbilityKind::GravityField;
    ability.arm_ticks = 9U;
    ability.duration_ticks = 75U;
    ability.payload = GravityFieldAbilityDefinition{9U, 75U, 6.0, 1000.0,
        20U, 25.0, 10.0};
    result.abilities.push_back(ability);
    BirdArchetype bird;
    bird.id = BirdArchetypeId{1};
    bird.key = "bird";
    bird.ability_id = AbilityId{1};
    bird.surface_id = SurfaceId{1002};
    bird.mass_kg = bird_mass_kg;
    bird.radius_m = 0.25;
    bird.friction = 0.4;
    bird.restitution = 0.1;
    bird.bullet = true;
    bird.projectile_visual_id = "bird";
    bird.launch_speed_cap_m_s = cap_m_s;
    bird.score_id = "bird_score";
    bird.icon_id = "icon";
    bird.animation_id = "animation";
    result.birds.push_back(bird);
    return result;
}

LevelManifest v2_level(WorldDefinition world = uniform_world())
{
    LevelManifest result;
    result.schema_version = result.source_schema_version = 2U;
    result.id = "launcher_lab";
    result.world_id = std::holds_alternative<UniformWorldDefinition>(world)
        ? "earth" : "orbital";
    result.world = std::move(world);
    result.slingshot = std::holds_alternative<UniformWorldDefinition>(result.world)
        ? slingshot() : slingshot({13.0, 0.0, 0.0});
    result.bird_queue = {BirdArchetypeId{1}, BirdArchetypeId{1}};
    return result;
}

std::unique_ptr<SimulationSession> create_session(
    WorldDefinition world = uniform_world(), double bird_mass_kg = 5.0,
    double cap_m_s = 100.0)
{
    auto level = v2_level(std::move(world));
    auto created = SimulationSession::create(
        v2_materials(), v2_archetypes(bird_mass_kg, cap_m_s), level);
    NINHO_SIM_REQUIRE(created.ok());
    return std::move(created.value);
}

void begin_grab(SimulationSession& session, ninho::physics::Vec3 camera_right)
{
    NINHO_SIM_REQUIRE(session.enqueue(BeginGrabCommand{camera_right}).ok());
    NINHO_SIM_REQUIRE(session.tick().ok());
    NINHO_SIM_REQUIRE(session.state().phase == SessionPhase::Grabbed);
    NINHO_SIM_REQUIRE(session.state().launcher.has_value());
}

const EntitySnapshot& projectile_snapshot(const SimulationSession& session)
{
    const auto found = std::ranges::find_if(session.snapshots(), [](const auto& value) {
        return value.is_projectile;
    });
    NINHO_SIM_REQUIRE(found != session.snapshots().end());
    return *found;
}

}

NINHO_SIM_TEST("launcher system terrestrial plane locks world up and projected camera yaw")
{
    detail::LauncherSystem first{uniform_world(), slingshot({-4.0004, 2.0004, 0.0004})};
    NINHO_SIM_REQUIRE(first.begin_grab({0.60001F, 0.20001F, 0.80001F}, projectile()).ok());
    const LauncherState locked = *first.state();
    NINHO_SIM_REQUIRE(close(locked.rest_position_m, {-4.0F, 2.0F, 0.0F}));
    NINHO_SIM_REQUIRE(close(locked.up, {0.0F, 1.0F, 0.0F}));
    NINHO_SIM_REQUIRE(close(locked.horizontal, {0.6F, 0.0F, 0.8F}, 1.0e-4F));
    NINHO_SIM_REQUIRE(close(locked.plane_normal, {-0.8F, 0.0F, 0.6F}, 1.0e-4F));

    detail::LauncherSystem same_quantized_base{uniform_world(), slingshot()};
    NINHO_SIM_REQUIRE(same_quantized_base.begin_grab(
        {0.60004F, 0.20004F, 0.80004F}, projectile()).ok());
    NINHO_SIM_REQUIRE(*same_quantized_base.state() == locked);

    detail::LauncherSystem pitched{uniform_world(), slingshot()};
    NINHO_SIM_REQUIRE(pitched.begin_grab(
        {0.60001F, -0.40001F, 0.80001F}, projectile()).ok());
    NINHO_SIM_REQUIRE(pitched.state()->up == locked.up);
    NINHO_SIM_REQUIRE(pitched.state()->horizontal == locked.horizontal);
    NINHO_SIM_REQUIRE(pitched.state()->plane_normal == locked.plane_normal);

    detail::LauncherSystem yawed{uniform_world(), slingshot()};
    NINHO_SIM_REQUIRE(yawed.begin_grab({0.0F, 0.4F, 1.0F}, projectile()).ok());
    NINHO_SIM_REQUIRE(close(yawed.state()->up, locked.up));
    NINHO_SIM_REQUIRE(close(yawed.state()->horizontal, {0.0F, 0.0F, 1.0F}));
}

NINHO_SIM_TEST("launcher system orbital plane contains radial up and camera selected tangent")
{
    detail::LauncherSystem launcher{radial_world(), slingshot({13.0, 0.0, 0.0})};
    NINHO_SIM_REQUIRE(launcher.begin_grab({0.2F, 1.0F, 0.0F}, projectile()).ok());
    const auto& state = *launcher.state();
    NINHO_SIM_REQUIRE(close(state.up, {1.0F, 0.0F, 0.0F}));
    NINHO_SIM_REQUIRE(close(state.horizontal, {0.0F, 1.0F, 0.0F}));
    NINHO_SIM_REQUIRE(close(state.plane_normal, {0.0F, 0.0F, -1.0F}));
    NINHO_SIM_REQUIRE(close(ninho::physics::dot(state.horizontal, state.up), 0.0));
    NINHO_SIM_REQUIRE(close(ninho::physics::dot(state.plane_normal, state.up), 0.0));
}

NINHO_SIM_TEST("launcher system rejects invalid camera bases and never relocks an active grab")
{
    detail::LauncherSystem launcher{uniform_world(), slingshot()};
    NINHO_SIM_REQUIRE(!launcher.begin_grab({0.0F, 1.0F, 0.0F}, projectile()).ok());
    NINHO_SIM_REQUIRE(!launcher.state().has_value());
    NINHO_SIM_REQUIRE(!launcher.begin_grab(
        {std::numeric_limits<float>::quiet_NaN(), 0.0F, 1.0F}, projectile()).ok());
    NINHO_SIM_REQUIRE(!launcher.state().has_value());

    NINHO_SIM_REQUIRE(launcher.begin_grab({1.0F, 0.0F, 0.0F}, projectile()).ok());
    const LauncherState locked = *launcher.state();
    NINHO_SIM_REQUIRE(launcher.set_pull(1.2344, -0.8764).ok());
    NINHO_SIM_REQUIRE(launcher.state()->camera_right == locked.camera_right);
    NINHO_SIM_REQUIRE(launcher.state()->up == locked.up);
    NINHO_SIM_REQUIRE(launcher.state()->horizontal == locked.horizontal);
    NINHO_SIM_REQUIRE(launcher.state()->plane_normal == locked.plane_normal);
    NINHO_SIM_REQUIRE(!launcher.begin_grab({0.0F, 0.0F, 1.0F}, projectile()).ok());
    NINHO_SIM_REQUIRE(launcher.state()->camera_right == locked.camera_right);
    NINHO_SIM_REQUIRE(launcher.state()->up == locked.up);
    NINHO_SIM_REQUIRE(launcher.state()->horizontal == locked.horizontal);
    NINHO_SIM_REQUIRE(launcher.state()->plane_normal == locked.plane_normal);
}

NINHO_SIM_TEST("launcher system quantizes pull to millimeters and circularly clamps at 4.25 meters")
{
    detail::LauncherSystem launcher{uniform_world(), slingshot()};
    NINHO_SIM_REQUIRE(launcher.begin_grab({1.0F, 0.0F, 0.0F}, projectile()).ok());
    NINHO_SIM_REQUIRE(launcher.set_pull(3.0004, 4.0004).ok());
    NINHO_SIM_REQUIRE(close(launcher.state()->pull_horizontal_m, 2.55));
    NINHO_SIM_REQUIRE(close(launcher.state()->pull_vertical_m, 3.40));
    NINHO_SIM_REQUIRE(close(launcher.state()->extension_m, 4.25));
    NINHO_SIM_REQUIRE(close(launcher.state()->deadzone_m, 0.20));
    NINHO_SIM_REQUIRE(close(launcher.state()->maximum_extension_m, 4.25));
    NINHO_SIM_REQUIRE(launcher.solution().ok());
    NINHO_SIM_REQUIRE(launcher.solution().value.launchable);

    NINHO_SIM_REQUIRE(launcher.set_pull(0.1994, 0.0).ok());
    NINHO_SIM_REQUIRE(close(launcher.state()->pull_horizontal_m, 0.199));
    NINHO_SIM_REQUIRE(!launcher.solution().value.launchable);
    NINHO_SIM_REQUIRE(!launcher.set_pull(
        std::numeric_limits<double>::infinity(), 0.0).ok());
}

NINHO_SIM_TEST("launcher system solves Hooke energy mass direction and dissipative speed cap")
{
    detail::LauncherSystem light{uniform_world(), slingshot()};
    NINHO_SIM_REQUIRE(light.begin_grab({1.0F, 0.0F, 0.0F}, projectile(5.0)).ok());
    NINHO_SIM_REQUIRE(light.set_pull(1.2, -1.6).ok());
    const auto light_state = *light.state();
    NINHO_SIM_REQUIRE(close(light_state.extension_m, 2.0));
    NINHO_SIM_REQUIRE(close(light_state.spring_energy_j, 10400.0));
    NINHO_SIM_REQUIRE(close(light_state.launch_energy_j, 9360.0));
    NINHO_SIM_REQUIRE(close(light_state.predicted_speed_m_s, 61.19, 0.0051));
    NINHO_SIM_REQUIRE(close(light_state.launch_direction, {-0.6F, 0.8F, 0.0F}));
    const auto solved = light.solution();
    NINHO_SIM_REQUIRE(solved.ok() && solved.value.launchable);
    NINHO_SIM_REQUIRE(solved.value.origin_m == light_state.rest_position_m);
    NINHO_SIM_REQUIRE(solved.value.direction == light_state.launch_direction);
    NINHO_SIM_REQUIRE(solved.value.speed_m_s == light_state.predicted_speed_m_s);

    detail::LauncherSystem heavy{uniform_world(), slingshot()};
    NINHO_SIM_REQUIRE(heavy.begin_grab({1.0F, 0.0F, 0.0F}, projectile(20.0)).ok());
    NINHO_SIM_REQUIRE(heavy.set_pull(1.2, -1.6).ok());
    NINHO_SIM_REQUIRE(close(heavy.state()->predicted_speed_m_s, 30.59, 0.0051));
    NINHO_SIM_REQUIRE(heavy.state()->predicted_speed_m_s < light_state.predicted_speed_m_s);

    detail::LauncherSystem capped{uniform_world(), slingshot()};
    NINHO_SIM_REQUIRE(capped.begin_grab({1.0F, 0.0F, 0.0F}, projectile(5.0, 15.0)).ok());
    NINHO_SIM_REQUIRE(capped.set_pull(1.2, -1.6).ok());
    NINHO_SIM_REQUIRE(close(capped.state()->spring_energy_j, 10400.0));
    NINHO_SIM_REQUIRE(close(capped.state()->launch_energy_j, 9360.0));
    NINHO_SIM_REQUIRE(close(capped.state()->predicted_speed_m_s, 15.0));
}

NINHO_SIM_TEST("launcher system floors nonquantized authored speed caps")
{
    const auto verify = [](SlingshotDefinition definition,
                            detail::LauncherProjectile selected_projectile) {
        detail::LauncherSystem launcher{
            uniform_world(), std::move(definition)};
        NINHO_SIM_REQUIRE(launcher.begin_grab(
            {1.0F, 0.0F, 0.0F}, selected_projectile).ok());
        NINHO_SIM_REQUIRE(launcher.set_pull(1.2, -1.6).ok());
        const auto& state = *launcher.state();
        NINHO_SIM_REQUIRE(close(state.spring_energy_j, 10400.0));
        NINHO_SIM_REQUIRE(close(state.launch_energy_j, 9360.0));
        NINHO_SIM_REQUIRE(close(state.predicted_speed_m_s, 15.0));
        NINHO_SIM_REQUIRE(state.predicted_speed_m_s <= 15.007);
        NINHO_SIM_REQUIRE(close(
            state.predicted_speed_m_s * 100.0,
            std::round(state.predicted_speed_m_s * 100.0)));
    };

    verify(slingshot(), projectile(5.0, 15.007));
    auto capped_slingshot = slingshot();
    capped_slingshot.speed_ceiling_m_s = 15.007;
    verify(capped_slingshot, projectile());
}

NINHO_SIM_TEST("launcher system grabbed session publishes only a ghost and keeps its locked frame")
{
    auto session = create_session();
    const auto canonical_before_grab = session->canonical_hash_v3();
    NINHO_SIM_REQUIRE(session->canonical_state_v2().empty());
    NINHO_SIM_REQUIRE(canonical_before_grab != 0U);
    const int bodies_before = session->physics_metrics().body_count;
    const std::uint32_t birds_before = session->birds_remaining();
    begin_grab(*session, {1.0F, 0.25F, 0.0F});
    const auto canonical_locked = session->canonical_hash_v3();
    NINHO_SIM_REQUIRE(canonical_locked != canonical_before_grab);
    const LauncherState locked = *session->state().launcher;
    NINHO_SIM_REQUIRE(session->physics_metrics().body_count == bodies_before);
    NINHO_SIM_REQUIRE(session->birds_remaining() == birds_before);
    NINHO_SIM_REQUIRE(std::ranges::none_of(session->snapshots(), &EntitySnapshot::is_projectile));

    NINHO_SIM_REQUIRE(session->enqueue(SetPullCommand{1.0, -0.5}).ok());
    NINHO_SIM_REQUIRE(session->enqueue(BeginGrabCommand{{0.0F, 0.0F, 1.0F}}).ok());
    NINHO_SIM_REQUIRE(session->tick().ok());
    NINHO_SIM_REQUIRE(session->canonical_hash_v3() != canonical_locked);
    NINHO_SIM_REQUIRE(session->state().phase == SessionPhase::Grabbed);
    NINHO_SIM_REQUIRE(session->state().launcher->camera_right == locked.camera_right);
    NINHO_SIM_REQUIRE(session->state().launcher->up == locked.up);
    NINHO_SIM_REQUIRE(session->state().launcher->horizontal == locked.horizontal);
    NINHO_SIM_REQUIRE(session->state().launcher->plane_normal == locked.plane_normal);
    NINHO_SIM_REQUIRE(session->physics_metrics().body_count == bodies_before);
}

NINHO_SIM_TEST("launcher system deadzone and cancel preserve queue while valid release consumes exactly one")
{
    auto session = create_session();
    const int bodies_before = session->physics_metrics().body_count;
    begin_grab(*session, {1.0F, 0.0F, 0.0F});
    NINHO_SIM_REQUIRE(session->enqueue(SetPullCommand{0.1994, 0.0}).ok());
    NINHO_SIM_REQUIRE(session->enqueue(ReleaseBirdCommand{}).ok());
    NINHO_SIM_REQUIRE(session->tick().ok());
    NINHO_SIM_REQUIRE(session->state().phase == SessionPhase::Inspection);
    NINHO_SIM_REQUIRE(session->birds_remaining() == 2U);
    NINHO_SIM_REQUIRE(session->physics_metrics().body_count == bodies_before);

    begin_grab(*session, {1.0F, 0.0F, 0.0F});
    NINHO_SIM_REQUIRE(session->enqueue(SetPullCommand{1.0, 0.0}).ok());
    NINHO_SIM_REQUIRE(session->enqueue(CancelGrabCommand{}).ok());
    NINHO_SIM_REQUIRE(session->tick().ok());
    NINHO_SIM_REQUIRE(session->state().phase == SessionPhase::Inspection);
    NINHO_SIM_REQUIRE(session->birds_remaining() == 2U);
    NINHO_SIM_REQUIRE(session->physics_metrics().body_count == bodies_before);

    begin_grab(*session, {1.0F, 0.0F, 0.0F});
    NINHO_SIM_REQUIRE(session->enqueue(SetPullCommand{-1.0, 0.0}).ok());
    NINHO_SIM_REQUIRE(session->tick().ok());
    const LauncherState released = *session->state().launcher;
    NINHO_SIM_REQUIRE(session->enqueue(ReleaseBirdCommand{}).ok());
    NINHO_SIM_REQUIRE(session->tick().ok());
    NINHO_SIM_REQUIRE(session->state().phase == SessionPhase::FlightAbility);
    NINHO_SIM_REQUIRE(session->birds_remaining() == 1U);
    NINHO_SIM_REQUIRE(session->physics_metrics().body_count == bodies_before + 1);
    NINHO_SIM_REQUIRE(projectile_snapshot(*session).visual_id == "bird");
    NINHO_SIM_REQUIRE(session->state().launcher == released);
}

NINHO_SIM_TEST("launcher system preview and release share solved uniform state and first impact")
{
    auto session = create_session();
    NINHO_SIM_REQUIRE(detail::SessionTestFacade::add_static_sphere(
        *session, EntityId{9000}, {1.0F, 1.5F, 0.0F}, 0.5));
    NINHO_SIM_REQUIRE(session->tick().ok());
    begin_grab(*session, {1.0F, 0.0F, 0.0F});
    NINHO_SIM_REQUIRE(session->enqueue(SetPullCommand{-1.0, 0.0}).ok());
    NINHO_SIM_REQUIRE(session->tick().ok());
    const LauncherState solved = *session->state().launcher;
    const auto preview = session->preview();
    NINHO_SIM_REQUIRE(preview.status.ok());
    NINHO_SIM_REQUIRE(preview.quantized_aim.origin_m == solved.rest_position_m);
    NINHO_SIM_REQUIRE(preview.quantized_aim.tangent_direction == solved.launch_direction);
    NINHO_SIM_REQUIRE(preview.quantized_aim.speed_m_s == solved.predicted_speed_m_s);
    NINHO_SIM_REQUIRE(preview.first_hit.has_value());
    NINHO_SIM_REQUIRE(preview.samples.size() <= 181U);

    NINHO_SIM_REQUIRE(session->enqueue(ReleaseBirdCommand{}).ok());
    NINHO_SIM_REQUIRE(session->tick().ok());
    for (std::size_t tick = 0; tick < 180U && !session->state().last_impact_m; ++tick) {
        NINHO_SIM_REQUIRE(session->tick().ok());
    }
    NINHO_SIM_REQUIRE(session->state().last_impact_m.has_value());
    NINHO_SIM_REQUIRE(ninho::physics::length(
        *session->state().last_impact_m - preview.first_hit->point_m) <= 0.10F);
}

NINHO_SIM_TEST("launcher system preview samples uniform and radial gravity_at for at most three seconds")
{
    const auto verify = [](WorldDefinition world, ninho::physics::Vec3 camera_right,
                            double horizontal, double vertical) {
        auto session = create_session(std::move(world));
        begin_grab(*session, camera_right);
        NINHO_SIM_REQUIRE(session->enqueue(SetPullCommand{horizontal, vertical}).ok());
        NINHO_SIM_REQUIRE(session->tick().ok());
        const auto state = *session->state().launcher;
        const auto preview = session->preview();
        NINHO_SIM_REQUIRE(preview.status.ok());
        NINHO_SIM_REQUIRE(preview.samples.size() >= 2U);
        NINHO_SIM_REQUIRE(preview.samples.size() <= 181U);
        constexpr float dt = 1.0F / 60.0F;
        const auto expected_velocity = state.launch_direction
            * static_cast<float>(state.predicted_speed_m_s)
            + detail::SessionTestFacade::gravity_at(*session, state.rest_position_m) * dt;
        const auto expected = state.rest_position_m + expected_velocity * dt;
        NINHO_SIM_REQUIRE(close(preview.samples[1], expected, 2.0e-4F));
    };
    verify(uniform_world(), {1.0F, 0.0F, 0.0F}, -1.0, 0.0);
    verify(radial_world(), {0.0F, 1.0F, 0.0F}, 0.0, -1.0);
}

NINHO_SIM_TEST("launcher system preview stops at bounds before an out of bounds hit")
{
    auto bounded = std::get<UniformWorldDefinition>(uniform_world());
    bounded.acceleration_m_s2 = {0.0, 0.0, 0.0};
    auto session = create_session(WorldDefinition{bounded});
    NINHO_SIM_REQUIRE(detail::SessionTestFacade::add_static_sphere(
        *session, EntityId{9001}, {48.95F, 2.0F, 0.0F}, 0.5));
    NINHO_SIM_REQUIRE(session->tick().ok());
    begin_grab(*session, {1.0F, 0.0F, 0.0F});
    NINHO_SIM_REQUIRE(session->enqueue(SetPullCommand{-1.0, 0.0}).ok());
    NINHO_SIM_REQUIRE(session->tick().ok());
    const auto preview = session->preview();
    NINHO_SIM_REQUIRE(preview.status.ok());
    NINHO_SIM_REQUIRE(!preview.first_hit.has_value());
    NINHO_SIM_REQUIRE(close(preview.samples.back().x, 48.0, 1.0e-5));
}

NINHO_SIM_TEST("launcher system preview orders same segment AABB boundary and hit")
{
    const auto preview_with_maximum_x = [](double maximum_x) {
        auto bounded = std::get<UniformWorldDefinition>(uniform_world());
        bounded.acceleration_m_s2 = {0.0, 0.0, 0.0};
        bounded.bounds_max_m[0] = maximum_x;
        auto session = create_session(WorldDefinition{bounded});
        NINHO_SIM_REQUIRE(detail::SessionTestFacade::add_static_sphere(
            *session, EntityId{9010}, {-2.95F, 2.0F, 0.0F}, 0.5));
        NINHO_SIM_REQUIRE(session->tick().ok());
        begin_grab(*session, {1.0F, 0.0F, 0.0F});
        NINHO_SIM_REQUIRE(session->enqueue(SetPullCommand{-1.0, 0.0}).ok());
        NINHO_SIM_REQUIRE(session->tick().ok());
        return session->preview();
    };

    const auto bounds_first = preview_with_maximum_x(-3.75);
    NINHO_SIM_REQUIRE(bounds_first.status.ok());
    NINHO_SIM_REQUIRE(!bounds_first.first_hit.has_value());
    NINHO_SIM_REQUIRE(close(bounds_first.samples.back().x, -3.75, 1.0e-5));

    const auto hit_first = preview_with_maximum_x(-3.60);
    NINHO_SIM_REQUIRE(hit_first.status.ok());
    NINHO_SIM_REQUIRE(hit_first.first_hit.has_value());
    NINHO_SIM_REQUIRE(hit_first.first_hit->entity_id == EntityId{9010});
    NINHO_SIM_REQUIRE(hit_first.samples.back().x < -3.60F);

    const auto tied = preview_with_maximum_x(-3.70);
    NINHO_SIM_REQUIRE(tied.status.ok());
    NINHO_SIM_REQUIRE(!tied.first_hit.has_value());
    NINHO_SIM_REQUIRE(close(tied.samples.back().x, -3.70, 1.0e-5));
}

NINHO_SIM_TEST("launcher system preview orders same segment radial boundary and hit")
{
    const auto preview_with_radius = [](double bounds_radius_m) {
        auto bounded = std::get<RadialWorldDefinition>(radial_world());
        bounded.bounds_radius_m = bounds_radius_m;
        auto session = create_session(WorldDefinition{bounded});
        NINHO_SIM_REQUIRE(detail::SessionTestFacade::add_static_sphere(
            *session, EntityId{9011}, {14.05F, 0.0F, 0.0F}, 0.5));
        NINHO_SIM_REQUIRE(session->tick().ok());
        begin_grab(*session, {0.0F, 1.0F, 0.0F});
        NINHO_SIM_REQUIRE(session->enqueue(SetPullCommand{0.0, -1.0}).ok());
        NINHO_SIM_REQUIRE(session->tick().ok());
        return session->preview();
    };

    const auto bounds_first = preview_with_radius(3.25);
    NINHO_SIM_REQUIRE(bounds_first.status.ok());
    NINHO_SIM_REQUIRE(!bounds_first.first_hit.has_value());
    NINHO_SIM_REQUIRE(close(bounds_first.samples.back().x, 13.25, 1.0e-5));

    const auto hit_first = preview_with_radius(3.40);
    NINHO_SIM_REQUIRE(hit_first.status.ok());
    NINHO_SIM_REQUIRE(hit_first.first_hit.has_value());
    NINHO_SIM_REQUIRE(hit_first.first_hit->entity_id == EntityId{9011});
    NINHO_SIM_REQUIRE(hit_first.samples.back().x < 13.40F);

    const auto tied = preview_with_radius(3.30);
    NINHO_SIM_REQUIRE(tied.status.ok());
    NINHO_SIM_REQUIRE(!tied.first_hit.has_value());
    NINHO_SIM_REQUIRE(close(tied.samples.back().x, 13.30, 1.0e-5));
}
