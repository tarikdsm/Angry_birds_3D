#include "test_framework.hpp"

#include "ability_system.hpp"
#include "session_internal.hpp"
#include "session_test_facade.hpp"
#include "ninho/simulation/commands.hpp"
#include "ninho/simulation/session.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <limits>
#include <memory>
#include <numbers>
#include <ranges>
#include <span>
#include <string>
#include <vector>

namespace {

using namespace ninho::physics;
using namespace ninho::simulation;
using ninho::simulation::detail::SessionTestFacade;

constexpr double split_angle_deg = 11.0;
constexpr double split_multiplier = 1.012400431604922;
constexpr std::uint32_t child_namespace = 0xC0000000U;

AbilityArchetype split_ability(std::uint32_t child_count = 3U,
    double angle_deg = split_angle_deg,
    double multiplier = split_multiplier)
{
    AbilityArchetype result;
    result.id = AbilityId{1};
    result.key = result.kind = "split";
    result.kind_v2 = AbilityKind::Split;
    result.payload = SplitAbilityDefinition{child_count, angle_deg, multiplier};
    return result;
}

MaterialCatalog materials()
{
    MaterialCatalog result;
    result.schema_version = result.source_schema_version = 2U;
    result.surfaces.push_back({SurfaceId{1001}, "bird", 1000.0, 0.4, 0.1});
    return result;
}

ArchetypeCatalog archetypes(bool child_visual = true, double mass_kg = 6.0,
    double radius_m = 0.25, AbilityArchetype ability = split_ability())
{
    ArchetypeCatalog result;
    result.schema_version = result.source_schema_version = 2U;
    result.presentation_ids = {"blue_bird", "blue_icon", "blue_animation"};
    if (child_visual) result.presentation_ids.push_back("CHR_BlueChild");
    result.score_ids = {"blue_score"};
    result.abilities.push_back(std::move(ability));
    for (std::uint32_t id = 1; id <= 2; ++id) {
        BirdArchetype bird;
        bird.id = BirdArchetypeId{id};
        bird.key = "blue_" + std::to_string(id);
        bird.ability_id = AbilityId{1};
        bird.surface_id = SurfaceId{1001};
        bird.mass_kg = mass_kg;
        bird.radius_m = radius_m;
        bird.friction = 0.4;
        bird.restitution = 0.1;
        bird.bullet = true;
        bird.projectile_visual_id = "blue_bird";
        bird.launch_speed_cap_m_s = 45.0;
        bird.score_id = "blue_score";
        bird.icon_id = "blue_icon";
        bird.animation_id = "blue_animation";
        result.birds.push_back(std::move(bird));
    }
    return result;
}

LevelManifest level(std::array<double, 3> gravity = {})
{
    LevelManifest result;
    result.schema_version = result.source_schema_version = 2U;
    result.id = "split_ability";
    result.world_id = "earth";
    result.world = UniformWorldDefinition{
        .acceleration_m_s2 = gravity,
        .bounds_min_m = {-10000.0, -10000.0, -10000.0},
        .bounds_max_m = {10000.0, 10000.0, 10000.0},
    };
    result.slingshot = {
        .asset_id = "launcher",
        .rest_position_m = {-4.0, 20.0, 0.0},
        .rest_rotation_xyzw = {0.0, 0.0, 0.0, 1.0},
        .spring_constant_n_m = 520.0,
        .energy_efficiency = 0.9,
        .minimum_extension_m = 0.2,
        .maximum_extension_m = 4.25,
        .plane_policy = "gravity_vertical_camera_yaw",
        .projectile_clearance_m = 0.0,
        .speed_ceiling_m_s = 45.0,
    };
    result.bird_queue = {BirdArchetypeId{1}, BirdArchetypeId{2}};
    result.settle_policy = {0.001, 0.001, 3U};
    result.watchdog_ticks = 1500U;
    return result;
}

std::unique_ptr<SimulationSession> create_session(
    std::array<double, 3> gravity = {}, ArchetypeCatalog catalog = archetypes())
{
    auto created = SimulationSession::create(materials(), catalog, level(gravity));
    NINHO_SIM_REQUIRE(created.ok());
    return std::move(created.value);
}

void launch(SimulationSession& session, Vec3 camera_right = {0.6F, 0.0F, 0.8F})
{
    NINHO_SIM_REQUIRE(session.enqueue(BeginGrabCommand{camera_right}).ok());
    NINHO_SIM_REQUIRE(session.enqueue(SetPullCommand{-1.25, 0.5}).ok());
    NINHO_SIM_REQUIRE(session.tick().ok());
    NINHO_SIM_REQUIRE(session.enqueue(ReleaseBirdCommand{}).ok());
    NINHO_SIM_REQUIRE(session.tick().ok());
}

void advance_to_offset(SimulationSession& session, std::uint32_t offset)
{
    const TickIndex launch_tick = SessionTestFacade::projectile_launch_tick(session);
    while (session.state().tick < TickIndex{launch_tick.value() + offset}) {
        NINHO_SIM_REQUIRE(session.tick().ok());
    }
}

std::vector<EntitySnapshot> projectiles(const SimulationSession& session)
{
    std::vector<EntitySnapshot> result;
    std::ranges::copy_if(session.snapshots(), std::back_inserter(result),
        &EntitySnapshot::is_projectile);
    return result;
}

EntitySnapshot only_projectile(const SimulationSession& session)
{
    auto values = projectiles(session);
    NINHO_SIM_REQUIRE(values.size() == 1U);
    return values.front();
}

const DomainEvent& require_event(
    std::span<const DomainEvent> events, DomainEventKind kind)
{
    const auto found = std::ranges::find(events, kind, &DomainEvent::kind);
    NINHO_SIM_REQUIRE(found != events.end());
    return *found;
}

void activate(SimulationSession& session)
{
    advance_to_offset(session, 8U);
    NINHO_SIM_REQUIRE(session.enqueue(ActivateAbilityCommand{}).ok());
    NINHO_SIM_REQUIRE(session.tick().ok());
}

void require_vector_near(Vec3 actual, Vec3 expected, float tolerance)
{
    NINHO_SIM_REQUIRE(is_finite(actual));
    NINHO_SIM_REQUIRE(length(actual - expected) <= tolerance);
}

Vec3 rotate_about(Vec3 value, Vec3 normal, double angle_radians)
{
    return value * static_cast<float>(std::cos(angle_radians))
        + cross(normal, value) * static_cast<float>(std::sin(angle_radians));
}

NINHO_SIM_TEST("split ability content accepts only the supported preset and shared validation")
{
    const std::string json = to_canonical_json(archetypes());
    const auto parsed = parse_archetype_catalog_v2(json);
    NINHO_SIM_REQUIRE(parsed.ok());
    NINHO_SIM_REQUIRE(ninho::simulation::detail::AbilitySystem::activation_arm_ticks(
        parsed.value.abilities.front()) == 9U);

    const std::array future_presets{
        split_ability(4U, split_angle_deg, split_multiplier),
        split_ability(3U, 12.0, split_multiplier),
        split_ability(3U, split_angle_deg, 1.5),
    };
    for (const auto& unsupported : future_presets) {
        const auto parsed_future = parse_archetype_catalog_v2(
            to_canonical_json(archetypes(true, 6.0, 0.25, unsupported)));
        NINHO_SIM_REQUIRE(parsed_future.ok());
        const auto created = SimulationSession::create(
            materials(), archetypes(true, 6.0, 0.25, unsupported), level());
        NINHO_SIM_REQUIRE(!created.ok());
        NINHO_SIM_REQUIRE(created.error.code == ContentErrorCode::InvalidInvariant);
    }

    const auto orphan = SimulationSession::create(
        materials(), archetypes(false), level());
    NINHO_SIM_REQUIRE(!orphan.ok());
    NINHO_SIM_REQUIRE(orphan.error.code == ContentErrorCode::MissingReference);
    const auto orphan_json = parse_archetype_catalog_v2(
        to_canonical_json(archetypes(false)));
    NINHO_SIM_REQUIRE(!orphan_json.ok());
    NINHO_SIM_REQUIRE(orphan_json.error.code
        == ContentErrorCode::MissingReference);

    constexpr double parent_safe_but_child_unsafe_mass = 8.0e-39;
    const auto unsafe = SimulationSession::create(materials(),
        archetypes(true, parent_safe_but_child_unsafe_mass), level());
    NINHO_SIM_REQUIRE(!unsafe.ok());
    NINHO_SIM_REQUIRE(unsafe.error.pointer == "/birds/0/mass_kg");
    const auto unsafe_json = parse_archetype_catalog_v2(to_canonical_json(
        archetypes(true, parent_safe_but_child_unsafe_mass)));
    NINHO_SIM_REQUIRE(!unsafe_json.ok());
    NINHO_SIM_REQUIRE(unsafe_json.error.pointer == "/birds/0/mass_kg");

    auto live = create_session();
    launch(*live);
    const auto before = live->canonical_state_v3();
    const auto shot_before = live->shot_state();
    const auto rejected = live->reconfigure(materials(),
        archetypes(true, 6.0, 0.25, future_presets.front()), level());
    NINHO_SIM_REQUIRE(!rejected.ok());
    NINHO_SIM_REQUIRE(live->canonical_state_v3() == before);
    NINHO_SIM_REQUIRE(live->shot_state() == shot_before);
}

NINHO_SIM_TEST("split ability typed definition rejects invalid finite ranges and numbers")
{
    const struct InvalidSplit {
        AbilityArchetype definition;
        ContentErrorCode code;
        std::string pointer;
    } invalids[]{
        {split_ability(1U), ContentErrorCode::OutOfRange,
            "/abilities/0/payload/child_count"},
        {split_ability(3U, 0.0), ContentErrorCode::OutOfRange,
            "/abilities/0/payload/spread_angle_deg"},
        {split_ability(3U, std::numeric_limits<double>::quiet_NaN()),
            ContentErrorCode::InvalidNumber,
            "/abilities/0/payload/spread_angle_deg"},
        {split_ability(3U, split_angle_deg, 0.0),
            ContentErrorCode::OutOfRange,
            "/abilities/0/payload/child_speed_multiplier"},
        {split_ability(3U, split_angle_deg,
            std::numeric_limits<double>::infinity()),
            ContentErrorCode::InvalidNumber,
            "/abilities/0/payload/child_speed_multiplier"},
    };
    auto live = create_session();
    launch(*live);
    const auto canonical_before = live->canonical_state_v3();
    const auto shot_before = live->shot_state();
    for (const auto& invalid : invalids) {
        const auto created = SimulationSession::create(materials(),
            archetypes(true, 6.0, 0.25, invalid.definition), level());
        NINHO_SIM_REQUIRE(!created.ok());
        NINHO_SIM_REQUIRE(created.error.code == invalid.code);
        NINHO_SIM_REQUIRE(created.error.pointer == invalid.pointer);
        const auto status = live->reconfigure(materials(),
            archetypes(true, 6.0, 0.25, invalid.definition), level());
        NINHO_SIM_REQUIRE(!status.ok());
        NINHO_SIM_REQUIRE(status.error.code == invalid.code);
        NINHO_SIM_REQUIRE(status.error.pointer == invalid.pointer);
        NINHO_SIM_REQUIRE(live->canonical_state_v3() == canonical_before);
        NINHO_SIM_REQUIRE(live->shot_state() == shot_before);
    }
}

NINHO_SIM_TEST("split ability arms at launch plus nine and consumes exactly once")
{
    for (std::uint32_t offset = 0; offset <= 8; ++offset) {
        auto early = create_session();
        NINHO_SIM_REQUIRE(early->enqueue(
            BeginGrabCommand{{0.6F, 0.0F, 0.8F}}).ok());
        NINHO_SIM_REQUIRE(early->enqueue(SetPullCommand{-1.25, 0.5}).ok());
        NINHO_SIM_REQUIRE(early->tick().ok());
        NINHO_SIM_REQUIRE(early->enqueue(ReleaseBirdCommand{}).ok());
        if (offset == 0U) {
            NINHO_SIM_REQUIRE(early->enqueue(ActivateAbilityCommand{}).ok());
        }
        NINHO_SIM_REQUIRE(early->tick().ok());
        if (offset > 0U) {
            advance_to_offset(*early, offset - 1U);
            NINHO_SIM_REQUIRE(early->enqueue(ActivateAbilityCommand{}).ok());
            NINHO_SIM_REQUIRE(early->tick().ok());
        }
        const TickIndex launched = SessionTestFacade::projectile_launch_tick(*early);
        NINHO_SIM_REQUIRE(early->state().tick
            == TickIndex{launched.value() + offset});
        NINHO_SIM_REQUIRE(require_event(early->events(),
            DomainEventKind::CommandRejected).rejection_reason
            == CommandRejectionReason::NotArmed);
    }

    auto session = create_session();
    launch(*session);
    advance_to_offset(*session, 8U);
    NINHO_SIM_REQUIRE(session->enqueue(ActivateAbilityCommand{}).ok());
    NINHO_SIM_REQUIRE(session->tick().ok());
    NINHO_SIM_REQUIRE(SessionTestFacade::ability_requested(*session));
    NINHO_SIM_REQUIRE(projectiles(*session).size() == 3U);
    NINHO_SIM_REQUIRE(session->enqueue(ActivateAbilityCommand{}).ok());
    NINHO_SIM_REQUIRE(session->tick().ok());
    NINHO_SIM_REQUIRE(require_event(session->events(),
        DomainEventKind::CommandRejected).rejection_reason
        == CommandRejectionReason::NotArmed);
}

NINHO_SIM_TEST("split ability creates ordered children and preserves full vector momentum")
{
    auto session = create_session();
    launch(*session);
    advance_to_offset(*session, 8U);
    const EntitySnapshot parent = only_projectile(*session);
    const std::size_t birds_before = session->birds_remaining();
    NINHO_SIM_REQUIRE(SessionTestFacade::impulse_entity(*session, parent.entity_id,
        session->shot_state()->locked_plane.plane_normal * 3.0F
            * static_cast<float>(parent.mass_kg)));
    NINHO_SIM_REQUIRE(session->tick().ok());
    const EntitySnapshot drifted_parent = only_projectile(*session);
    NINHO_SIM_REQUIRE(session->enqueue(ActivateAbilityCommand{}).ok());
    NINHO_SIM_REQUIRE(session->tick().ok());

    const auto children = projectiles(*session);
    NINHO_SIM_REQUIRE(children.size() == 3U);
    for (std::size_t index = 0; index < children.size(); ++index) {
        NINHO_SIM_REQUIRE(children[index].entity_id
            == EntityId{child_namespace + static_cast<std::uint32_t>(index)});
        NINHO_SIM_REQUIRE(children[index].visual_id == "CHR_BlueChild");
        NINHO_SIM_REQUIRE(children[index].shape.type == ShapeType::Sphere);
        NINHO_SIM_REQUIRE(std::abs(children[index].shape.radius_m
            - std::cbrt(1.0 / 3.0) * parent.shape.radius_m) <= 1.0e-6);
        NINHO_SIM_REQUIRE(children[index].surface_id == parent.surface_id);
    }
    double total_mass{};
    Vec3 total_momentum{};
    const Vec3 plane_normal = session->shot_state()->locked_plane.plane_normal;
    const Vec3 normal_velocity = plane_normal
        * dot(drifted_parent.linear_velocity_m_s, plane_normal);
    const Vec3 planar_velocity = drifted_parent.linear_velocity_m_s
        - normal_velocity;
    const Vec3 scaled_planar = planar_velocity
        * static_cast<float>(split_multiplier);
    const double angle_radians = split_angle_deg * std::numbers::pi / 180.0;
    const std::array expected_velocities{
        rotate_about(scaled_planar, plane_normal, -angle_radians) + normal_velocity,
        scaled_planar + normal_velocity,
        rotate_about(scaled_planar, plane_normal, angle_radians) + normal_velocity,
    };
    for (std::size_t index = 0; index < children.size(); ++index) {
        const auto& child = children[index];
        NINHO_SIM_REQUIRE(std::abs(child.mass_kg
            - drifted_parent.mass_kg / 3.0) <= 1.0e-5);
        require_vector_near(child.linear_velocity_m_s,
            expected_velocities[index], 2.0e-4F);
        require_vector_near(child.angular_velocity_rad_s,
            drifted_parent.angular_velocity_rad_s, 1.0e-6F);
        total_mass += child.mass_kg;
        total_momentum = total_momentum + child.linear_velocity_m_s
            * static_cast<float>(child.mass_kg);
    }
    NINHO_SIM_REQUIRE(std::abs(total_mass - drifted_parent.mass_kg) <= 1.0e-5);
    require_vector_near(total_momentum,
        drifted_parent.linear_velocity_m_s
            * static_cast<float>(drifted_parent.mass_kg), 1.0e-4F);
    NINHO_SIM_REQUIRE(session->birds_remaining() == birds_before);
    NINHO_SIM_REQUIRE(SessionTestFacade::projectile_is_bullet(*session));
}

NINHO_SIM_TEST("split ability preflight rejects an invalid locked plane before consumption")
{
    auto session = create_session();
    launch(*session);
    advance_to_offset(*session, 8U);
    const auto before = session->shot_state();
    const EntitySnapshot source = only_projectile(*session);
    SessionTestFacade::invalidate_locked_plane_for_testing(*session);
    NINHO_SIM_REQUIRE(session->enqueue(ActivateAbilityCommand{}).ok());
    NINHO_SIM_REQUIRE(session->tick().ok());
    NINHO_SIM_REQUIRE(session->state().phase != SessionPhase::Faulted);
    NINHO_SIM_REQUIRE(!SessionTestFacade::ability_requested(*session));
    NINHO_SIM_REQUIRE(projectiles(*session).size() == 1U);
    NINHO_SIM_REQUIRE(only_projectile(*session).entity_id == source.entity_id);
    NINHO_SIM_REQUIRE(std::ranges::none_of(session->events(), [](const auto& event) {
        return event.kind == DomainEventKind::AbilityStarted
            || event.kind == DomainEventKind::ProjectileSplit
            || event.kind == DomainEventKind::ProjectileSpawned;
    }));
    NINHO_SIM_REQUIRE(require_event(session->events(),
        DomainEventKind::CommandRejected).rejection_reason
        == CommandRejectionReason::AbilityUnavailable);
    NINHO_SIM_REQUIRE(session->shot_state()->projectile_ids
        == before->projectile_ids);
}

NINHO_SIM_TEST("split ability applies gravity to every child in its first solver step")
{
    constexpr std::array<double, 3> gravity{0.0, -9.0, 0.0};
    auto session = create_session(gravity);
    launch(*session);
    advance_to_offset(*session, 8U);
    const EntitySnapshot parent = only_projectile(*session);
    NINHO_SIM_REQUIRE(session->enqueue(ActivateAbilityCommand{}).ok());
    NINHO_SIM_REQUIRE(session->tick().ok());
    const auto children = projectiles(*session);
    NINHO_SIM_REQUIRE(children.size() == 3U);

    Vec3 aggregate{};
    for (const auto& child : children) {
        aggregate = aggregate + child.linear_velocity_m_s;
        NINHO_SIM_REQUIRE(child.transform.position.y < parent.transform.position.y);
        NINHO_SIM_REQUIRE(SessionTestFacade::affected_by_world_gravity(
            *session, child.entity_id, child.part_id));
    }
    require_vector_near(aggregate / 3.0F,
        parent.linear_velocity_m_s + Vec3{0.0F, -9.0F / 60.0F, 0.0F},
        2.0e-4F);
}

NINHO_SIM_TEST("split ability grace lasts three complete solver steps and restores finished child")
{
    auto session = create_session();
    launch(*session);
    activate(*session);
    const auto ids = session->shot_state()->projectile_ids;
    NINHO_SIM_REQUIRE(ids.size() == 3U);
    for (const EntityId id : ids) {
        NINHO_SIM_REQUIRE(SessionTestFacade::collision_group(*session, id) < 0);
    }
    SessionTestFacade::finish_projectile(*session, ids.front());
    NINHO_SIM_REQUIRE(session->tick().ok());
    for (const EntityId id : ids) {
        NINHO_SIM_REQUIRE(SessionTestFacade::collision_group(*session, id) < 0);
    }
    NINHO_SIM_REQUIRE(session->tick().ok());
    for (const EntityId id : ids) {
        NINHO_SIM_REQUIRE(SessionTestFacade::collision_group(*session, id) == 0);
    }
}

NINHO_SIM_TEST("split ability publishes causal ordered events and removes original once")
{
    auto session = create_session();
    launch(*session);
    advance_to_offset(*session, 8U);
    const EntityId source = only_projectile(*session).entity_id;
    NINHO_SIM_REQUIRE(session->enqueue(ActivateAbilityCommand{}).ok());
    NINHO_SIM_REQUIRE(session->tick().ok());
    const auto events = session->events();
    NINHO_SIM_REQUIRE(events.size() == 5U);
    NINHO_SIM_REQUIRE(events[0].kind == DomainEventKind::AbilityStarted);
    NINHO_SIM_REQUIRE(events[1].kind == DomainEventKind::ProjectileSplit);
    NINHO_SIM_REQUIRE(events[1].entity_id == source);
    for (std::size_t index = 0; index < 3U; ++index) {
        const auto& spawned = events[index + 2U];
        NINHO_SIM_REQUIRE(spawned.kind == DomainEventKind::ProjectileSpawned);
        NINHO_SIM_REQUIRE(spawned.entity_id == source);
        NINHO_SIM_REQUIRE(spawned.affected_entity_id
            == EntityId{child_namespace + static_cast<std::uint32_t>(index)});
        NINHO_SIM_REQUIRE(spawned.affected_part_id == PartId{1});
        NINHO_SIM_REQUIRE(spawned.cause_event_id == events[1].id);
    }
    NINHO_SIM_REQUIRE(std::ranges::none_of(session->snapshots(),
        [source](const auto& value) { return value.entity_id == source; }));
    NINHO_SIM_REQUIRE(session->tick().ok());
    NINHO_SIM_REQUIRE(session->tick().ok());
    const DomainEvent& ended = require_event(
        session->events(), DomainEventKind::AbilityEnded);
    NINHO_SIM_REQUIRE(ended.entity_id == source);
}

NINHO_SIM_TEST("split ability FSM waits for all children and then for settlement")
{
    auto session = create_session();
    launch(*session);
    activate(*session);
    const auto ids = session->shot_state()->projectile_ids;
    SessionTestFacade::finish_projectile(*session, ids[0]);
    NINHO_SIM_REQUIRE(session->tick().ok());
    NINHO_SIM_REQUIRE(session->state().phase == SessionPhase::FlightAbility);
    SessionTestFacade::finish_projectile(*session, ids[1]);
    NINHO_SIM_REQUIRE(session->tick().ok());
    NINHO_SIM_REQUIRE(session->state().phase == SessionPhase::FlightAbility);
    SessionTestFacade::finish_projectile(*session, ids[2]);
    NINHO_SIM_REQUIRE(session->tick().ok());
    NINHO_SIM_REQUIRE(session->state().phase == SessionPhase::Resolution);
    NINHO_SIM_REQUIRE(session->tick().ok());
    NINHO_SIM_REQUIRE(session->state().phase == SessionPhase::Resolution);
    for (std::uint32_t tick = 0; tick < 58U; ++tick) {
        NINHO_SIM_REQUIRE(session->state().phase == SessionPhase::Resolution);
        NINHO_SIM_REQUIRE(session->tick().ok());
    }
    NINHO_SIM_REQUIRE(session->state().phase == SessionPhase::Evaluation);
}

NINHO_SIM_TEST("split ability rejects capacity identity and ordinal overflow atomically")
{
    auto capacity = create_session();
    launch(*capacity);
    advance_to_offset(*capacity, 8U);
    const EntitySnapshot source = only_projectile(*capacity);
    for (std::uint32_t index = 0; index < 497U; ++index) {
        NINHO_SIM_REQUIRE(SessionTestFacade::add_static_sphere(*capacity,
            EntityId{1000U + index}, {100.0F + index, 0, 0}, 0.1));
    }
    const auto shot_before = capacity->shot_state();
    NINHO_SIM_REQUIRE(capacity->enqueue(ActivateAbilityCommand{}).ok());
    NINHO_SIM_REQUIRE(capacity->tick().ok());
    NINHO_SIM_REQUIRE(capacity->state().phase != SessionPhase::Faulted);
    NINHO_SIM_REQUIRE(!SessionTestFacade::ability_requested(*capacity));
    NINHO_SIM_REQUIRE(capacity->shot_state()->projectile_ids
        == shot_before->projectile_ids);
    NINHO_SIM_REQUIRE(only_projectile(*capacity).entity_id == source.entity_id);
    NINHO_SIM_REQUIRE(require_event(capacity->events(),
        DomainEventKind::CommandRejected).rejection_reason
        == CommandRejectionReason::AbilityUnavailable);
    NINHO_SIM_REQUIRE(std::ranges::none_of(capacity->events(), [](const auto& event) {
        return event.kind == DomainEventKind::AbilityStarted
            || event.kind == DomainEventKind::ProjectileSplit
            || event.kind == DomainEventKind::ProjectileSpawned;
    }));

    auto collision = create_session();
    launch(*collision);
    advance_to_offset(*collision, 8U);
    NINHO_SIM_REQUIRE(SessionTestFacade::add_static_sphere(*collision,
        EntityId{child_namespace}, {100, 0, 0}, 0.1));
    NINHO_SIM_REQUIRE(collision->enqueue(ActivateAbilityCommand{}).ok());
    NINHO_SIM_REQUIRE(collision->tick().ok());
    NINHO_SIM_REQUIRE(!SessionTestFacade::ability_requested(*collision));
    NINHO_SIM_REQUIRE(projectiles(*collision).size() == 1U);

    auto overflow = create_session();
    SessionTestFacade::set_launch_ordinal(*overflow, 357913942U);
    launch(*overflow);
    activate(*overflow);
    NINHO_SIM_REQUIRE(!SessionTestFacade::ability_requested(*overflow));
    NINHO_SIM_REQUIRE(projectiles(*overflow).size() == 1U);
    NINHO_SIM_REQUIRE(overflow->state().phase != SessionPhase::Faulted);

    auto boundary_overflow = create_session();
    SessionTestFacade::set_launch_ordinal(*boundary_overflow, 357913941U);
    launch(*boundary_overflow);
    advance_to_offset(*boundary_overflow, 8U);
    NINHO_SIM_REQUIRE(boundary_overflow->enqueue(
        ActivateAbilityCommand{}).ok());
    NINHO_SIM_REQUIRE(boundary_overflow->tick().ok());
    NINHO_SIM_REQUIRE(!SessionTestFacade::ability_requested(*boundary_overflow));
    NINHO_SIM_REQUIRE(projectiles(*boundary_overflow).size() == 1U);
    NINHO_SIM_REQUIRE(boundary_overflow->state().phase != SessionPhase::Faulted);

    auto last_valid = create_session();
    SessionTestFacade::set_launch_ordinal(*last_valid, 357913940U);
    launch(*last_valid);
    activate(*last_valid);
    const std::vector<EntityId> expected_last_ids{
        EntityId{0xFFFFFFFCU}, EntityId{0xFFFFFFFDU}, EntityId{0xFFFFFFFEU}};
    NINHO_SIM_REQUIRE(last_valid->shot_state()->projectile_ids
        == expected_last_ids);

    auto ordinal = create_session();
    SessionTestFacade::set_launch_ordinal(*ordinal, 7U);
    launch(*ordinal);
    activate(*ordinal);
    const auto ordinal_ids = ordinal->shot_state()->projectile_ids;
    const std::vector<EntityId> expected_ordinal_ids{
        EntityId{child_namespace + 21U}, EntityId{child_namespace + 22U},
        EntityId{child_namespace + 23U}};
    NINHO_SIM_REQUIRE(ordinal_ids == expected_ordinal_ids);
}

NINHO_SIM_TEST("split ability canonical state is deterministic and sensitive to split runtime")
{
    std::vector<std::uint8_t> baseline;
    std::uint64_t baseline_hash{};
    for (int repetition = 0; repetition < 50; ++repetition) {
        auto session = create_session();
        launch(*session);
        activate(*session);
        if (repetition == 0) {
            baseline = session->canonical_state_v3();
            baseline_hash = session->canonical_hash_v3();
        } else {
            NINHO_SIM_REQUIRE(session->canonical_state_v3() == baseline);
            NINHO_SIM_REQUIRE(session->canonical_hash_v3() == baseline_hash);
        }
    }

    auto child_mutation = create_session();
    launch(*child_mutation);
    activate(*child_mutation);
    const auto child_before = child_mutation->canonical_state_v3();
    NINHO_SIM_REQUIRE(SessionTestFacade::set_split_child_id_for_testing(
        *child_mutation, 0U, EntityId{child_namespace + 99U}));
    SessionTestFacade::refresh_canonical_state(*child_mutation);
    NINHO_SIM_REQUIRE(child_mutation->canonical_state_v3() != child_before);

    auto grace_mutation = create_session();
    launch(*grace_mutation);
    activate(*grace_mutation);
    const auto grace_before = grace_mutation->canonical_state_v3();
    NINHO_SIM_REQUIRE(SessionTestFacade::set_split_grace_end_for_testing(
        *grace_mutation, TickIndex{999U}));
    SessionTestFacade::refresh_canonical_state(*grace_mutation);
    NINHO_SIM_REQUIRE(grace_mutation->canonical_state_v3() != grace_before);
}

NINHO_SIM_TEST("split ability preserves append only event and rejection tags")
{
    static_assert(static_cast<std::uint8_t>(DomainEventKind::MassChanged) == 13U);
    static_assert(static_cast<std::uint8_t>(DomainEventKind::SpeedChanged) == 14U);
    static_assert(static_cast<std::uint8_t>(DomainEventKind::ProjectileSplit) == 15U);
    static_assert(static_cast<std::uint8_t>(DomainEventKind::ProjectileSpawned) == 16U);
    static_assert(static_cast<std::uint8_t>(
        CommandRejectionReason::AbilityUnavailable) == 5U);
    NINHO_SIM_REQUIRE(ninho::simulation::detail::canonical_tag_of(
        DomainEventKind::ProjectileSplit) == 15U);
    NINHO_SIM_REQUIRE(ninho::simulation::detail::canonical_tag_of(
        DomainEventKind::ProjectileSpawned) == 16U);
}

}
