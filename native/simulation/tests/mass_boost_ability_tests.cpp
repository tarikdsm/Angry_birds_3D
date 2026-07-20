#include "test_framework.hpp"

#include "physics_world_test_facade.hpp"
#include "session_test_facade.hpp"
#include "ninho/simulation/commands.hpp"
#include "ninho/simulation/session.hpp"
#include <ninho/physics/physics_world.hpp>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <limits>
#include <memory>
#include <ranges>
#include <string>
#include <vector>

namespace {

using namespace ninho::physics;
using namespace ninho::simulation;

constexpr double mass_multiplier = 2.25;
constexpr std::uint32_t ability_duration_ticks = 75U;

void require_near(double actual, double expected, double tolerance)
{
    NINHO_SIM_REQUIRE(std::isfinite(actual));
    NINHO_SIM_REQUIRE(std::isfinite(expected));
    NINHO_SIM_REQUIRE(std::abs(actual - expected) <= tolerance);
}

void require_vector_near(Vec3 actual, Vec3 expected, double tolerance)
{
    NINHO_SIM_REQUIRE(length(actual - expected) <= tolerance);
}

AbilityArchetype mass_boost_ability()
{
    AbilityArchetype result;
    result.id = AbilityId{1};
    result.key = result.kind = "mass_boost";
    // Product v2 omits arm_ticks from this payload. The session boundary owns
    // the fixed nine-tick MassBoost arming rule even for an in-memory catalog.
    result.arm_ticks = 0U;
    result.kind_v2 = AbilityKind::MassBoost;
    result.payload = MassBoostAbilityDefinition{
        ability_duration_ticks, mass_multiplier};
    return result;
}

struct InvalidMassBoostDefinition {
    std::uint32_t duration_ticks;
    double mass_multiplier;
    ContentErrorCode expected_code;
    std::string_view expected_field;
};

ArchetypeCatalog archetypes();

std::vector<InvalidMassBoostDefinition> invalid_mass_boost_definitions()
{
    return {
        {ability_duration_ticks, 0.0, ContentErrorCode::OutOfRange,
            "mass_multiplier"},
        {ability_duration_ticks, 0.5, ContentErrorCode::OutOfRange,
            "mass_multiplier"},
        {ability_duration_ticks, 1.0, ContentErrorCode::OutOfRange,
            "mass_multiplier"},
        {ability_duration_ticks, 21.0, ContentErrorCode::OutOfRange,
            "mass_multiplier"},
        {ability_duration_ticks, std::numeric_limits<double>::quiet_NaN(),
            ContentErrorCode::InvalidNumber, "mass_multiplier"},
        {ability_duration_ticks, std::numeric_limits<double>::infinity(),
            ContentErrorCode::InvalidNumber, "mass_multiplier"},
        {0U, mass_multiplier, ContentErrorCode::OutOfRange,
            "duration_ticks"},
        {3601U, mass_multiplier, ContentErrorCode::OutOfRange,
            "duration_ticks"},
    };
}

ArchetypeCatalog archetypes_with_mass_boost(
    const InvalidMassBoostDefinition& definition)
{
    ArchetypeCatalog result = archetypes();
    result.abilities.front().payload = MassBoostAbilityDefinition{
        definition.duration_ticks, definition.mass_multiplier};
    return result;
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
    result.presentation_ids = {"red_bird", "red_icon", "red_animation"};
    result.score_ids = {"red_score"};
    result.abilities.push_back(mass_boost_ability());
    for (std::uint32_t id = 1U; id <= 2U; ++id) {
        BirdArchetype bird;
        bird.id = BirdArchetypeId{id};
        bird.key = "red_" + std::to_string(id);
        bird.ability_id = AbilityId{1};
        bird.surface_id = SurfaceId{1001};
        bird.mass_kg = 6.0;
        bird.radius_m = 0.25;
        bird.friction = 0.4;
        bird.restitution = 0.1;
        bird.bullet = true;
        bird.projectile_visual_id = "red_bird";
        bird.launch_speed_cap_m_s = 40.0;
        bird.score_id = "red_score";
        bird.icon_id = "red_icon";
        bird.animation_id = "red_animation";
        result.birds.push_back(std::move(bird));
    }
    return result;
}

LevelManifest level()
{
    LevelManifest result;
    result.schema_version = result.source_schema_version = 2U;
    result.id = "mass_boost_ability";
    result.world_id = "earth";
    result.world = UniformWorldDefinition{
        .acceleration_m_s2 = {0.0, -9.81, 0.0},
        .bounds_min_m = {-10000.0, -10000.0, -10000.0},
        .bounds_max_m = {10000.0, 10000.0, 10000.0},
    };
    result.slingshot = {
        .asset_id = "launcher",
        .rest_position_m = {-4.0, 20.0, 0.0},
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

std::unique_ptr<SimulationSession> create_session()
{
    auto created = SimulationSession::create(materials(), archetypes(), level());
    NINHO_SIM_REQUIRE(created.ok());
    return std::move(created.value);
}

void begin_pull(SimulationSession& session)
{
    NINHO_SIM_REQUIRE(session.enqueue(BeginGrabCommand{{1.0F, 0.0F, 0.0F}}).ok());
    NINHO_SIM_REQUIRE(session.enqueue(SetPullCommand{-1.25, 0.5}).ok());
    NINHO_SIM_REQUIRE(session.tick().ok());
}

void release(SimulationSession& session, bool request_ability_same_tick = false)
{
    NINHO_SIM_REQUIRE(session.enqueue(ReleaseBirdCommand{}).ok());
    if (request_ability_same_tick) {
        NINHO_SIM_REQUIRE(session.enqueue(ActivateAbilityCommand{}).ok());
    }
    NINHO_SIM_REQUIRE(session.tick().ok());
}

void launch(SimulationSession& session)
{
    begin_pull(session);
    release(session);
}

void advance_to_activation(SimulationSession& session)
{
    const TickIndex launched =
        ninho::simulation::detail::SessionTestFacade::projectile_launch_tick(session);
    while (session.state().tick < TickIndex{launched.value() + 8U}) {
        NINHO_SIM_REQUIRE(session.tick().ok());
    }
    NINHO_SIM_REQUIRE(session.enqueue(ActivateAbilityCommand{}).ok());
    NINHO_SIM_REQUIRE(session.tick().ok());
}

EntitySnapshot projectile_snapshot(const SimulationSession& session)
{
    const auto found = std::ranges::find_if(session.snapshots(), [](const auto& value) {
        return value.is_projectile;
    });
    NINHO_SIM_REQUIRE(found != session.snapshots().end());
    return *found;
}

void append_events(
    const SimulationSession& session, std::vector<DomainEvent>& destination)
{
    destination.insert(destination.end(), session.events().begin(), session.events().end());
}

const DomainEvent& require_event(
    std::span<const DomainEvent> events, DomainEventKind kind)
{
    const auto found = std::ranges::find(events, kind, &DomainEvent::kind);
    NINHO_SIM_REQUIRE(found != events.end());
    return *found;
}

NINHO_SIM_TEST("mass boost ability parser derives nine arm ticks without changing payload")
{
    const std::string canonical = to_canonical_json(archetypes());
    NINHO_SIM_REQUIRE(canonical.find("\"mass_boost\"") != std::string::npos);
    NINHO_SIM_REQUIRE(canonical.find("\"arm_ticks\"") == std::string::npos);

    const auto parsed = parse_archetype_catalog_v2(canonical);
    NINHO_SIM_REQUIRE(parsed.ok());
    NINHO_SIM_REQUIRE(parsed.value.abilities.size() == 1U);
    const AbilityArchetype& ability = parsed.value.abilities.front();
    NINHO_SIM_REQUIRE(ability.arm_ticks == 9U);
    NINHO_SIM_REQUIRE(ability.duration_ticks == ability_duration_ticks);
    const MassBoostAbilityDefinition expected{
        ability_duration_ticks, mass_multiplier};
    NINHO_SIM_REQUIRE(std::get<MassBoostAbilityDefinition>(ability.payload)
        == expected);
}

NINHO_SIM_TEST("mass boost ability parser rejects a no op multiplier")
{
    ArchetypeCatalog no_op = archetypes();
    std::get<MassBoostAbilityDefinition>(
        no_op.abilities.front().payload).mass_multiplier = 1.0;

    const auto parsed = parse_archetype_catalog_v2(to_canonical_json(no_op));

    NINHO_SIM_REQUIRE(!parsed.ok());
    NINHO_SIM_REQUIRE(parsed.error.code == ContentErrorCode::OutOfRange);
    NINHO_SIM_REQUIRE(parsed.error.pointer
        == "/abilities/0/payload/mass_multiplier");
}

NINHO_SIM_TEST("mass boost ability typed create rejects invalid payload semantics")
{
    NINHO_SIM_REQUIRE(
        SimulationSession::create(materials(), archetypes(), level()).ok());

    for (const InvalidMassBoostDefinition& definition
        : invalid_mass_boost_definitions()) {
        const auto created = SimulationSession::create(
            materials(), archetypes_with_mass_boost(definition), level());
        NINHO_SIM_REQUIRE(!created.ok());
        NINHO_SIM_REQUIRE(created.error.code == definition.expected_code);
        NINHO_SIM_REQUIRE(created.error.pointer
            == "/abilities/0/payload/" + std::string{definition.expected_field});
    }
}

NINHO_SIM_TEST("mass boost ability typed reconfigure rejects invalid payload atomically")
{
    auto session = create_session();
    launch(*session);
    const auto canonical_before = session->canonical_state_v3();
    const SessionState state_before = session->state();
    const auto shot_before = session->shot_state();
    const std::vector<EntitySnapshot> snapshots_before{
        session->snapshots().begin(), session->snapshots().end()};
    const std::size_t birds_before = session->birds_remaining();

    for (const InvalidMassBoostDefinition& definition
        : invalid_mass_boost_definitions()) {
        const SessionStatus status = session->reconfigure(
            materials(), archetypes_with_mass_boost(definition), level());
        NINHO_SIM_REQUIRE(!status.ok());
        NINHO_SIM_REQUIRE(status.error.code == definition.expected_code);
        NINHO_SIM_REQUIRE(status.error.pointer
            == "/abilities/0/payload/" + std::string{definition.expected_field});
        NINHO_SIM_REQUIRE(session->canonical_state_v3() == canonical_before);
        NINHO_SIM_REQUIRE(session->state().tick == state_before.tick);
        NINHO_SIM_REQUIRE(session->state().phase == state_before.phase);
        NINHO_SIM_REQUIRE(session->state().outcome == state_before.outcome);
        NINHO_SIM_REQUIRE(session->state().aim == state_before.aim);
        NINHO_SIM_REQUIRE(session->state().launcher == state_before.launcher);
        NINHO_SIM_REQUIRE(session->state().last_impact_m == state_before.last_impact_m);
        NINHO_SIM_REQUIRE(session->shot_state() == shot_before);
        NINHO_SIM_REQUIRE(std::ranges::equal(
            session->snapshots(), snapshots_before));
        NINHO_SIM_REQUIRE(session->birds_remaining() == birds_before);
    }
}

NINHO_SIM_TEST("mass boost ability physics scales mass and inertia without replacing shapes")
{
    WorldConfig config;
    config.gravity = UniformGravityConfig{{}};
    config.bounds = NoWorldBounds{};
    PhysicsWorld world{config};

    BodyDesc body = BodyDesc::dynamic_sphere(
        0.45f, {{1.0f, 2.0f, 3.0f}, {}}, 17.0f);
    body.linear_velocity = {4.0f, -5.0f, 6.0f};
    body.angular_velocity = {-0.7f, 0.8f, -0.9f};
    body.shapes.push_back({
        .geometry = CompoundShape{{
            BoxShape{{0.2f, 0.3f, 0.4f}, {{0.5f, 0.0f, 0.0f}, {}}},
            CapsuleShape{0.15f, 0.25f, {{-0.4f, 0.1f, 0.0f}, {}}},
        }},
        .density = 31.0f,
    });
    const auto created = world.create_body(body);
    NINHO_SIM_REQUIRE(static_cast<bool>(created));
    NINHO_SIM_REQUIRE(world.commit_pending_initial_state().ok());

    const BodyHandle handle = created.value;
    const auto before = world.state(handle);
    NINHO_SIM_REQUIRE(before.has_value());
    const auto inertia_before =
        ninho::physics::detail::PhysicsWorldTestFacade::local_inertia(world, handle);
    const auto center_before =
        ninho::physics::detail::PhysicsWorldTestFacade::local_center(world, handle);
    const auto shapes_before =
        ninho::physics::detail::PhysicsWorldTestFacade::shape_keys(world, handle);
    const auto densities_before =
        ninho::physics::detail::PhysicsWorldTestFacade::shape_densities(world, handle);
    const auto base_densities_before =
        ninho::physics::detail::PhysicsWorldTestFacade::base_shape_densities(
            world, handle);
    const auto bounds_before = world.body_bounds(handle);
    const WorldMetrics metrics_before = world.metrics();
    NINHO_SIM_REQUIRE(base_densities_before == densities_before);
    require_near(ninho::physics::detail::PhysicsWorldTestFacade::base_mass(
        world, handle), before->mass, 1.0e-6);
    require_near(ninho::physics::detail::PhysicsWorldTestFacade::mass_scale(
        world, handle), 1.0, 1.0e-6);

    NINHO_SIM_REQUIRE(world.set_body_mass_scale(handle, 2.25f).ok());

    const auto after = world.state(handle);
    NINHO_SIM_REQUIRE(after.has_value());
    require_near(after->mass, before->mass * 2.25, 1.0e-4);
    const auto inertia_after =
        ninho::physics::detail::PhysicsWorldTestFacade::local_inertia(world, handle);
    for (std::size_t index = 0; index < inertia_before.size(); ++index) {
        require_near(inertia_after[index], inertia_before[index] * 2.25, 1.0e-4);
    }
    require_vector_near(after->linear_velocity, before->linear_velocity, 1.0e-5);
    require_vector_near(after->angular_velocity, before->angular_velocity, 1.0e-5);
    NINHO_SIM_REQUIRE(after->awake == before->awake);
    require_vector_near(
        ninho::physics::detail::PhysicsWorldTestFacade::local_center(world, handle),
        center_before, 1.0e-6);
    NINHO_SIM_REQUIRE(
        ninho::physics::detail::PhysicsWorldTestFacade::shape_keys(world, handle)
        == shapes_before);
    const auto densities_after =
        ninho::physics::detail::PhysicsWorldTestFacade::shape_densities(world, handle);
    NINHO_SIM_REQUIRE(densities_after.size() == densities_before.size());
    for (std::size_t index = 0; index < densities_before.size(); ++index) {
        require_near(densities_after[index], densities_before[index] * 2.25, 1.0e-5);
    }
    NINHO_SIM_REQUIRE(
        ninho::physics::detail::PhysicsWorldTestFacade::base_shape_densities(
            world, handle) == base_densities_before);
    require_near(ninho::physics::detail::PhysicsWorldTestFacade::base_mass(
        world, handle), before->mass, 1.0e-6);
    require_near(ninho::physics::detail::PhysicsWorldTestFacade::mass_scale(
        world, handle), 2.25, 1.0e-6);
    const auto bounds_after = world.body_bounds(handle);
    NINHO_SIM_REQUIRE(bounds_before.has_value());
    NINHO_SIM_REQUIRE(bounds_after.has_value());
    NINHO_SIM_REQUIRE(bounds_after->lower == bounds_before->lower);
    NINHO_SIM_REQUIRE(bounds_after->upper == bounds_before->upper);
    NINHO_SIM_REQUIRE(world.metrics().body_count == metrics_before.body_count);
    NINHO_SIM_REQUIRE(world.metrics().shape_count == metrics_before.shape_count);

    // Scale is absolute relative to immutable base densities, not cumulative.
    NINHO_SIM_REQUIRE(world.set_body_mass_scale(handle, 2.25f).ok());
    require_near(world.state(handle)->mass, before->mass * 2.25, 1.0e-4);
    NINHO_SIM_REQUIRE(world.set_body_mass_scale(handle, 3.0f).ok());
    require_near(world.state(handle)->mass, before->mass * 3.0, 1.0e-4);
    require_near(ninho::physics::detail::PhysicsWorldTestFacade::mass_scale(
        world, handle), 3.0, 1.0e-6);
}

NINHO_SIM_TEST("mass boost ability physics rejects invalid or non live bodies atomically")
{
    WorldConfig config;
    config.gravity = UniformGravityConfig{{}};
    config.bounds = NoWorldBounds{};
    PhysicsWorld world{config};
    const auto dynamic = world.create_body(
        BodyDesc::dynamic_sphere(0.25f, {}, 1000.0f));
    const auto fixed = world.create_body(BodyDesc::static_sphere(0.25f, {}));
    NINHO_SIM_REQUIRE(dynamic && fixed);

    NINHO_SIM_REQUIRE(!world.set_body_mass_scale(dynamic.value, 2.25f).ok());
    NINHO_SIM_REQUIRE(!world.set_body_mass_scale({}, 2.25f).ok());
    NINHO_SIM_REQUIRE(!world.set_body_mass_scale(dynamic.value, 0.0f).ok());
    NINHO_SIM_REQUIRE(!world.set_body_mass_scale(
        dynamic.value, std::numeric_limits<float>::infinity()).ok());
    NINHO_SIM_REQUIRE(world.commit_pending_initial_state().ok());
    const auto before = world.state(dynamic.value);
    NINHO_SIM_REQUIRE(before.has_value());

    const auto fixed_densities_before =
        ninho::physics::detail::PhysicsWorldTestFacade::shape_densities(
            world, fixed.value);
    const Status fixed_status = world.set_body_mass_scale(fixed.value, 2.25f);
    NINHO_SIM_REQUIRE(fixed_status.code == StatusCode::InvalidArgument);
    NINHO_SIM_REQUIRE(
        ninho::physics::detail::PhysicsWorldTestFacade::shape_densities(
            world, fixed.value) == fixed_densities_before);
    NINHO_SIM_REQUIRE(!world.set_body_mass_scale(dynamic.value, -1.0f).ok());
    NINHO_SIM_REQUIRE(!world.set_body_mass_scale(
        dynamic.value, std::numeric_limits<float>::quiet_NaN()).ok());
    require_near(world.state(dynamic.value)->mass, before->mass, 1.0e-6);

    NINHO_SIM_REQUIRE(world.destroy_body(dynamic.value).ok());
    NINHO_SIM_REQUIRE(!world.set_body_mass_scale(dynamic.value, 2.25f).ok());
}

NINHO_SIM_TEST("mass boost ability physics prevalidates every compound child before overflow")
{
    WorldConfig config;
    config.gravity = UniformGravityConfig{{}};
    config.bounds = NoWorldBounds{};
    PhysicsWorld world{config};

    BodyDesc body = BodyDesc::dynamic_sphere(0.25f, {}, 11.0f);
    body.shapes.push_back({
        .geometry = CompoundShape{{
            BoxShape{{0.1f, 0.2f, 0.3f}, {{0.4f, 0.0f, 0.0f}, {}}},
            SphereShape{0.15f, {{-0.4f, 0.0f, 0.0f}, {}}},
        }},
        .density = 19.0f,
    });
    const auto created = world.create_body(body);
    NINHO_SIM_REQUIRE(created);
    NINHO_SIM_REQUIRE(world.commit_pending_initial_state().ok());

    const BodyHandle handle = created.value;
    const auto before = world.state(handle);
    const auto densities_before =
        ninho::physics::detail::PhysicsWorldTestFacade::shape_densities(
            world, handle);
    const auto inertia_before =
        ninho::physics::detail::PhysicsWorldTestFacade::local_inertia(
            world, handle);
    const auto shapes_before =
        ninho::physics::detail::PhysicsWorldTestFacade::shape_keys(world, handle);
    NINHO_SIM_REQUIRE(before.has_value());
    NINHO_SIM_REQUIRE(densities_before.size() == 3U);

    // Corrupt the final cached child to prove no earlier shape is changed before
    // the whole operation is known to fit in Box3D's float density units.
    ninho::physics::detail::PhysicsWorldTestFacade::set_base_density(
        world, handle, 2U, std::numeric_limits<float>::max());
    const Status status = world.set_body_mass_scale(handle, 2.0f);

    NINHO_SIM_REQUIRE(status.code == StatusCode::InvalidArgument);
    NINHO_SIM_REQUIRE(
        ninho::physics::detail::PhysicsWorldTestFacade::shape_densities(
            world, handle) == densities_before);
    NINHO_SIM_REQUIRE(
        ninho::physics::detail::PhysicsWorldTestFacade::local_inertia(
            world, handle) == inertia_before);
    NINHO_SIM_REQUIRE(
        ninho::physics::detail::PhysicsWorldTestFacade::shape_keys(world, handle)
        == shapes_before);
    require_near(world.state(handle)->mass, before->mass, 1.0e-6);
    require_near(
        ninho::physics::detail::PhysicsWorldTestFacade::mass_scale(world, handle),
        1.0, 0.0);
}

NINHO_SIM_TEST("mass boost ability physics restores native state after post write failure")
{
    WorldConfig config;
    config.gravity = UniformGravityConfig{{}};
    config.bounds = NoWorldBounds{};
    PhysicsWorld world{config};
    BodyDesc body = BodyDesc::dynamic_sphere(
        0.2F, {{1.0F, 2.0F, 3.0F}, {}}, 13.0F);
    body.linear_velocity = {3.0F, -4.0F, 5.0F};
    body.angular_velocity = {-0.5F, 0.75F, -1.0F};
    body.shapes.push_back({
        .geometry = CompoundShape{{
            BoxShape{{0.15F, 0.25F, 0.35F}, {{0.7F, 0.1F, 0.0F}, {}}},
            CapsuleShape{0.12F, 0.3F, {{-0.2F, 0.45F, 0.1F}, {}}},
        }},
        .density = 29.0F,
    });
    const auto created = world.create_body(body);
    NINHO_SIM_REQUIRE(created);
    NINHO_SIM_REQUIRE(world.commit_pending_initial_state().ok());
    const BodyHandle handle = created.value;
    NINHO_SIM_REQUIRE(world.set_body_mass_scale(handle, 1.5F).ok());
    const auto before = world.state(handle);
    const auto densities_before =
        ninho::physics::detail::PhysicsWorldTestFacade::shape_densities(
            world, handle);
    const auto inertia_before =
        ninho::physics::detail::PhysicsWorldTestFacade::local_inertia(
            world, handle);
    const Vec3 center_before =
        ninho::physics::detail::PhysicsWorldTestFacade::local_center(world, handle);
    NINHO_SIM_REQUIRE(before.has_value());
    NINHO_SIM_REQUIRE(length(center_before) > 0.01F);
    ninho::physics::detail::PhysicsWorldTestFacade::
        fail_next_mass_scale_postcondition(world);

    const Status status = world.set_body_mass_scale(handle, 2.25F);

    NINHO_SIM_REQUIRE(status.code == StatusCode::Box3DFault);
    const auto after = world.state(handle);
    NINHO_SIM_REQUIRE(after.has_value());
    require_near(after->mass, before->mass, 1.0e-6);
    require_vector_near(after->linear_velocity, before->linear_velocity, 1.0e-6);
    require_vector_near(after->angular_velocity, before->angular_velocity, 1.0e-6);
    NINHO_SIM_REQUIRE(after->awake == before->awake);
    NINHO_SIM_REQUIRE(
        ninho::physics::detail::PhysicsWorldTestFacade::shape_densities(
            world, handle) == densities_before);
    NINHO_SIM_REQUIRE(
        ninho::physics::detail::PhysicsWorldTestFacade::local_inertia(
            world, handle) == inertia_before);
    require_vector_near(
        ninho::physics::detail::PhysicsWorldTestFacade::local_center(world, handle),
        center_before, 1.0e-6);
    require_near(
        ninho::physics::detail::PhysicsWorldTestFacade::mass_scale(world, handle),
        1.5, 0.0);
}

NINHO_SIM_TEST("mass boost ability physics preserves sleep and resets reused body slots")
{
    WorldConfig config;
    config.max_bodies = 1U;
    config.gravity = UniformGravityConfig{{}};
    config.bounds = NoWorldBounds{};
    PhysicsWorld world{config};
    BodyHandle previous{};

    for (std::uint32_t iteration = 0U; iteration < 20U; ++iteration) {
        const float density = 10.0f + static_cast<float>(iteration);
        BodyDesc body = BodyDesc::dynamic_box({0.2f, 0.3f, 0.4f}, {}, density);
        body.shapes.push_back({
            .geometry = SphereShape{0.1f, {{0.5f, 0.0f, 0.0f}, {}}},
            .density = density + 1.0f,
        });
        const auto created = world.create_body(body);
        NINHO_SIM_REQUIRE(created);
        world.step();
        const BodyHandle handle = created.value;
        NINHO_SIM_REQUIRE(handle.index == 1U);
        if (previous.valid()) {
            NINHO_SIM_REQUIRE(handle.generation != previous.generation);
        }

        const auto base_densities =
            ninho::physics::detail::PhysicsWorldTestFacade::base_shape_densities(
                world, handle);
        NINHO_SIM_REQUIRE(base_densities.size() == 2U);
        require_near(base_densities[0], density, 0.0);
        require_near(base_densities[1], density + 1.0f, 0.0);
        require_near(
            ninho::physics::detail::PhysicsWorldTestFacade::base_mass(world, handle),
            world.state(handle)->mass, 1.0e-6);
        require_near(
            ninho::physics::detail::PhysicsWorldTestFacade::mass_scale(world, handle),
            1.0, 0.0);

        for (int settle = 0; settle < 90; ++settle) {
            world.step();
        }
        NINHO_SIM_REQUIRE(!world.state(handle)->awake);
        NINHO_SIM_REQUIRE(world.set_body_mass_scale(handle, 2.25f).ok());
        NINHO_SIM_REQUIRE(!world.state(handle)->awake);

        NINHO_SIM_REQUIRE(world.destroy_body(handle).ok());
        world.step();
        NINHO_SIM_REQUIRE(!world.state(handle).has_value());
        NINHO_SIM_REQUIRE(
            ninho::physics::detail::PhysicsWorldTestFacade::shape_keys(world, handle)
            .empty());
        NINHO_SIM_REQUIRE(
            ninho::physics::detail::PhysicsWorldTestFacade::base_shape_densities(
                world, handle).empty());
        NINHO_SIM_REQUIRE(world.metrics().body_count == 0U);
        NINHO_SIM_REQUIRE(world.metrics().shape_count == 0U);
        previous = handle;
    }
}

NINHO_SIM_TEST("mass boost ability rejects ticks zero through eight and activates at tick nine")
{
    auto active = create_session();
    auto control = create_session();
    begin_pull(*active);
    begin_pull(*control);
    release(*active, true);
    release(*control);

    const TickIndex launched =
        ninho::simulation::detail::SessionTestFacade::projectile_launch_tick(*active);
    NINHO_SIM_REQUIRE(active->state().tick == launched);
    std::vector<DomainEvent> history;
    append_events(*active, history);
    const DomainEvent& tick_zero_rejection =
        require_event(active->events(), DomainEventKind::CommandRejected);
    NINHO_SIM_REQUIRE(tick_zero_rejection.rejection_reason
        == CommandRejectionReason::NotArmed);

    for (std::uint32_t offset = 1U; offset <= 8U; ++offset) {
        NINHO_SIM_REQUIRE(active->enqueue(ActivateAbilityCommand{}).ok());
        NINHO_SIM_REQUIRE(active->tick().ok());
        NINHO_SIM_REQUIRE(control->tick().ok());
        NINHO_SIM_REQUIRE(active->state().tick
            == TickIndex{launched.value() + offset});
        NINHO_SIM_REQUIRE(active->events().size() == 1U);
        NINHO_SIM_REQUIRE(active->events().front().kind
            == DomainEventKind::CommandRejected);
        NINHO_SIM_REQUIRE(active->events().front().rejection_reason
            == CommandRejectionReason::NotArmed);
        append_events(*active, history);
    }
    NINHO_SIM_REQUIRE(std::ranges::count(
        history, DomainEventKind::CommandRejected, &DomainEvent::kind) == 9);
    NINHO_SIM_REQUIRE(
        !ninho::simulation::detail::SessionTestFacade::ability_active(*active));

    const EntitySnapshot before = projectile_snapshot(*active);
    const auto gravity_before = ninho::simulation::detail::SessionTestFacade::gravity_at(
        *active, before.transform.position);
    NINHO_SIM_REQUIRE(active->enqueue(ActivateAbilityCommand{}).ok());
    NINHO_SIM_REQUIRE(active->tick().ok());
    NINHO_SIM_REQUIRE(control->tick().ok());
    append_events(*active, history);

    NINHO_SIM_REQUIRE(active->state().tick
        == TickIndex{launched.value() + 9U});
    NINHO_SIM_REQUIRE(
        ninho::simulation::detail::SessionTestFacade::ability_active(*active));
    const EntitySnapshot boosted = projectile_snapshot(*active);
    const EntitySnapshot unboosted = projectile_snapshot(*control);
    require_near(boosted.mass_kg, before.mass_kg * mass_multiplier, 1.0e-4);
    require_near(unboosted.mass_kg, before.mass_kg, 1.0e-4);
    require_vector_near(
        boosted.linear_velocity_m_s, unboosted.linear_velocity_m_s, 1.0e-5);
    require_vector_near(
        boosted.angular_velocity_rad_s, unboosted.angular_velocity_rad_s, 1.0e-5);
    require_vector_near(boosted.transform.position, unboosted.transform.position, 1.0e-5);
    require_vector_near(
        ninho::simulation::detail::SessionTestFacade::gravity_at(
            *active, boosted.transform.position),
        gravity_before, 1.0e-6);
    NINHO_SIM_REQUIRE(boosted.entity_id == before.entity_id);
    NINHO_SIM_REQUIRE(boosted.part_id == before.part_id);
    NINHO_SIM_REQUIRE(boosted.shape == before.shape);
    NINHO_SIM_REQUIRE(boosted.visual_id == before.visual_id);
    const DomainEvent& changed = require_event(
        active->events(), DomainEventKind::MassChanged);
    require_near(changed.weight, mass_multiplier, 1.0e-9);
    NINHO_SIM_REQUIRE(changed.entity_id == boosted.entity_id);
    NINHO_SIM_REQUIRE(std::ranges::count(
        history, DomainEventKind::MassChanged, &DomainEvent::kind) == 1);
    NINHO_SIM_REQUIRE(std::ranges::count(
        history, DomainEventKind::AbilityStarted, &DomainEvent::kind) == 1);

    for (int gravity_tick = 0; gravity_tick < 6; ++gravity_tick) {
        NINHO_SIM_REQUIRE(active->tick().ok());
        NINHO_SIM_REQUIRE(control->tick().ok());
        append_events(*active, history);
        const EntitySnapshot active_projectile = projectile_snapshot(*active);
        const EntitySnapshot control_projectile = projectile_snapshot(*control);
        require_vector_near(active_projectile.linear_velocity_m_s,
            control_projectile.linear_velocity_m_s, 1.0e-5);
        require_vector_near(active_projectile.transform.position,
            control_projectile.transform.position, 1.0e-5);
        require_near(active_projectile.mass_kg,
            before.mass_kg * mass_multiplier, 1.0e-4);
    }

    NINHO_SIM_REQUIRE(active->enqueue(ActivateAbilityCommand{}).ok());
    NINHO_SIM_REQUIRE(active->tick().ok());
    append_events(*active, history);
    NINHO_SIM_REQUIRE(active->events().size() == 1U);
    NINHO_SIM_REQUIRE(active->events().front().kind
        == DomainEventKind::CommandRejected);
    NINHO_SIM_REQUIRE(active->events().front().rejection_reason
        == CommandRejectionReason::NotArmed);
    NINHO_SIM_REQUIRE(
        ninho::simulation::detail::SessionTestFacade::ability_active(*active));
}

NINHO_SIM_TEST("mass boost ability remains until removal and emits lifecycle events once")
{
    auto session = create_session();
    launch(*session);
    advance_to_activation(*session);
    const EntitySnapshot boosted = projectile_snapshot(*session);
    const TickIndex end =
        ninho::simulation::detail::SessionTestFacade::ability_end_tick(*session);
    std::vector<DomainEvent> history;
    append_events(*session, history);

    while (session->state().tick < end) {
        NINHO_SIM_REQUIRE(session->tick().ok());
        append_events(*session, history);
        const EntitySnapshot current = projectile_snapshot(*session);
        require_near(current.mass_kg, boosted.mass_kg, 1.0e-4);
    }

    NINHO_SIM_REQUIRE(
        !ninho::simulation::detail::SessionTestFacade::ability_active(*session));
    NINHO_SIM_REQUIRE(std::ranges::count(
        history, DomainEventKind::MassChanged, &DomainEvent::kind) == 1);
    NINHO_SIM_REQUIRE(std::ranges::count(
        history, DomainEventKind::AbilityEnded, &DomainEvent::kind) == 1);
    require_near(projectile_snapshot(*session).mass_kg, boosted.mass_kg, 1.0e-4);

    ninho::simulation::detail::SessionTestFacade::finish_projectile(*session);
    NINHO_SIM_REQUIRE(session->tick().ok());
    append_events(*session, history);
    NINHO_SIM_REQUIRE(std::ranges::none_of(
        session->snapshots(), [](const EntitySnapshot& value) {
            return value.is_projectile;
        }));
    NINHO_SIM_REQUIRE(std::ranges::count(
        history, DomainEventKind::MassChanged, &DomainEvent::kind) == 1);
    NINHO_SIM_REQUIRE(std::ranges::count(
        history, DomainEventKind::AbilityEnded, &DomainEvent::kind) == 1);
}

NINHO_SIM_TEST("mass boost ability twenty restarts reset shape and base mass caches")
{
    auto session = create_session();
    const WorldMetrics empty_metrics = session->physics_metrics();
    NINHO_SIM_REQUIRE(empty_metrics.body_count == 0);
    NINHO_SIM_REQUIRE(empty_metrics.shape_count == 0);

    for (int restart = 0; restart < 20; ++restart) {
        launch(*session);
        const EntitySnapshot base = projectile_snapshot(*session);
        require_near(base.mass_kg, 6.0, 1.0e-4);
        advance_to_activation(*session);
        require_near(
            projectile_snapshot(*session).mass_kg, 6.0 * mass_multiplier, 1.0e-4);
        NINHO_SIM_REQUIRE(session->physics_metrics().body_count == 1);
        NINHO_SIM_REQUIRE(session->physics_metrics().shape_count == 1);

        NINHO_SIM_REQUIRE(session->restart().ok());
        NINHO_SIM_REQUIRE(!session->shot_state().has_value());
        NINHO_SIM_REQUIRE(session->snapshots().empty());
        NINHO_SIM_REQUIRE(session->physics_metrics().body_count
            == empty_metrics.body_count);
        NINHO_SIM_REQUIRE(session->physics_metrics().shape_count
            == empty_metrics.shape_count);
    }
}

NINHO_SIM_TEST("mass boost ability recreate and reconfigure start from immutable base mass")
{
    for (int recreation = 0; recreation < 2; ++recreation) {
        auto recreated = create_session();
        launch(*recreated);
        advance_to_activation(*recreated);
        require_near(projectile_snapshot(*recreated).mass_kg,
            6.0 * mass_multiplier, 1.0e-4);
    }

    auto session = create_session();
    launch(*session);
    advance_to_activation(*session);
    require_near(projectile_snapshot(*session).mass_kg,
        6.0 * mass_multiplier, 1.0e-4);

    NINHO_SIM_REQUIRE(session->reconfigure(materials(), archetypes(), level()).ok());
    NINHO_SIM_REQUIRE(!session->shot_state().has_value());
    NINHO_SIM_REQUIRE(session->snapshots().empty());
    NINHO_SIM_REQUIRE(session->physics_metrics().body_count == 0U);
    NINHO_SIM_REQUIRE(session->physics_metrics().shape_count == 0U);

    launch(*session);
    require_near(projectile_snapshot(*session).mass_kg, 6.0, 1.0e-4);
    advance_to_activation(*session);
    require_near(projectile_snapshot(*session).mass_kg,
        6.0 * mass_multiplier, 1.0e-4);
}

}
