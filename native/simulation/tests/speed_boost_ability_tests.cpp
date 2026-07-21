#include "test_framework.hpp"

#include "ability_system.hpp"
#include "session_internal.hpp"
#include "session_test_facade.hpp"
#include "ninho/simulation/commands.hpp"
#include "ninho/simulation/session.hpp"

#include <nlohmann/json.hpp>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <limits>
#include <memory>
#include <ranges>
#include <sstream>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace {

using namespace ninho::physics;
using namespace ninho::simulation;
using ninho::simulation::detail::AbilityLifecycleHooks;
using ninho::simulation::detail::AbilitySystem;
using ninho::simulation::detail::SessionTestFacade;

constexpr double fallback_speed_m_s = 12.0;
constexpr double speed_multiplier = 1.55;
constexpr double absolute_speed_cap_m_s = 45.0;

void require_near(double actual, double expected, double tolerance)
{
    NINHO_SIM_REQUIRE(std::isfinite(actual));
    NINHO_SIM_REQUIRE(std::isfinite(expected));
    if (std::abs(actual - expected) > tolerance) {
        std::ostringstream message;
        message << "expected " << expected << " +/- " << tolerance
                << ", got " << actual
                << " (delta " << std::abs(actual - expected) << ')';
        ninho::simulation::test::fail(__FILE__, __LINE__, message.str());
    }
}

void require_vector_near(Vec3 actual, Vec3 expected, double tolerance)
{
    NINHO_SIM_REQUIRE(is_finite(actual));
    NINHO_SIM_REQUIRE(is_finite(expected));
    const double difference = length(actual - expected);
    if (difference > tolerance) {
        std::ostringstream message;
        message << "expected (" << expected.x << ", " << expected.y << ", "
                << expected.z << ") +/- " << tolerance << ", got ("
                << actual.x << ", " << actual.y << ", " << actual.z
                << ") (vector delta " << difference << ')';
        ninho::simulation::test::fail(__FILE__, __LINE__, message.str());
    }
}

void require_quantized(Vec3 value)
{
    for (const float component : {value.x, value.y, value.z}) {
        const auto fixed = SessionTestFacade::quantize_canonical(component);
        const float reconstructed = static_cast<float>(fixed) / 100000.0F;
        NINHO_SIM_REQUIRE(component == reconstructed);
    }
}

AbilityArchetype speed_boost_ability(double fallback = fallback_speed_m_s)
{
    AbilityArchetype result;
    result.id = AbilityId{1};
    result.key = result.kind = "speed_boost";
    result.arm_ticks = 0U;
    result.kind_v2 = AbilityKind::SpeedBoost;
    result.payload = SpeedBoostAbilityDefinition{fallback};
    return result;
}

MaterialCatalog materials()
{
    MaterialCatalog result;
    result.schema_version = result.source_schema_version = 2U;
    result.surfaces.push_back({SurfaceId{1001}, "bird", 1000.0, 0.4, 0.1});
    return result;
}

ArchetypeCatalog archetypes(double fallback = fallback_speed_m_s,
    double bird_mass_kg = 6.0, double bird_radius_m = 0.25)
{
    ArchetypeCatalog result;
    result.schema_version = result.source_schema_version = 2U;
    result.presentation_ids = {"yellow_bird", "yellow_icon", "yellow_animation"};
    result.score_ids = {"yellow_score"};
    result.abilities.push_back(speed_boost_ability(fallback));
    for (std::uint32_t id = 1U; id <= 2U; ++id) {
        BirdArchetype bird;
        bird.id = BirdArchetypeId{id};
        bird.key = "yellow_" + std::to_string(id);
        bird.ability_id = AbilityId{1};
        bird.surface_id = SurfaceId{1001};
        bird.mass_kg = bird_mass_kg;
        bird.radius_m = bird_radius_m;
        bird.friction = 0.4;
        bird.restitution = 0.1;
        bird.bullet = true;
        bird.projectile_visual_id = "yellow_bird";
        bird.launch_speed_cap_m_s = 45.0;
        bird.score_id = "yellow_score";
        bird.icon_id = "yellow_icon";
        bird.animation_id = "yellow_animation";
        result.birds.push_back(std::move(bird));
    }
    return result;
}

LevelManifest level(double spring_constant_n_m = 520.0,
    std::array<double, 3> acceleration_m_s2 = {})
{
    LevelManifest result;
    result.schema_version = result.source_schema_version = 2U;
    result.id = "speed_boost_ability";
    result.world_id = "earth";
    result.world = UniformWorldDefinition{
        .acceleration_m_s2 = acceleration_m_s2,
        .bounds_min_m = {-10000.0, -10000.0, -10000.0},
        .bounds_max_m = {10000.0, 10000.0, 10000.0},
    };
    result.slingshot = {
        .asset_id = "launcher",
        .rest_position_m = {-4.0, 20.0, 0.0},
        .rest_rotation_xyzw = {0.0, 0.0, 0.0, 1.0},
        .spring_constant_n_m = spring_constant_n_m,
        .energy_efficiency = 0.9,
        .minimum_extension_m = 0.2,
        .maximum_extension_m = 4.25,
        .plane_policy = "gravity_vertical_camera_yaw",
        .projectile_clearance_m = 0.0,
        .speed_ceiling_m_s = 45.0,
    };
    result.bird_queue = {BirdArchetypeId{1}, BirdArchetypeId{2}};
    result.settle_policy = {0.001, 0.001, 60U};
    result.watchdog_ticks = 1500U;
    return result;
}

std::unique_ptr<SimulationSession> create_session(
    double spring_constant_n_m = 520.0, double fallback = fallback_speed_m_s,
    double bird_mass_kg = 6.0,
    std::array<double, 3> acceleration_m_s2 = {})
{
    auto created = SimulationSession::create(
        materials(), archetypes(fallback, bird_mass_kg),
        level(spring_constant_n_m, acceleration_m_s2));
    NINHO_SIM_REQUIRE(created.ok());
    return std::move(created.value);
}

void begin_pull(SimulationSession& session)
{
    NINHO_SIM_REQUIRE(session.enqueue(BeginGrabCommand{{1.0F, 0.0F, 0.0F}}).ok());
    NINHO_SIM_REQUIRE(session.enqueue(SetPullCommand{-1.25, 0.5}).ok());
    NINHO_SIM_REQUIRE(session.tick().ok());
}

void release(SimulationSession& session, bool activate_same_tick = false)
{
    NINHO_SIM_REQUIRE(session.enqueue(ReleaseBirdCommand{}).ok());
    if (activate_same_tick) {
        NINHO_SIM_REQUIRE(session.enqueue(ActivateAbilityCommand{}).ok());
    }
    NINHO_SIM_REQUIRE(session.tick().ok());
}

void launch(SimulationSession& session)
{
    begin_pull(session);
    release(session);
}

void advance_to_offset(SimulationSession& session, std::uint32_t offset)
{
    const TickIndex launched = SessionTestFacade::projectile_launch_tick(session);
    while (session.state().tick < TickIndex{launched.value() + offset}) {
        NINHO_SIM_REQUIRE(session.tick().ok());
    }
}

EntitySnapshot projectile_snapshot(const SimulationSession& session)
{
    const auto found = std::ranges::find_if(
        session.snapshots(), [](const auto& value) { return value.is_projectile; });
    NINHO_SIM_REQUIRE(found != session.snapshots().end());
    return *found;
}

void queue_projectile_velocity(SimulationSession& session, Vec3 target)
{
    const EntitySnapshot before = projectile_snapshot(session);
    NINHO_SIM_REQUIRE(SessionTestFacade::impulse_entity(session,
        before.entity_id, (target - before.linear_velocity_m_s)
            * static_cast<float>(before.mass_kg)));
}

const DomainEvent& require_event(
    std::span<const DomainEvent> events, DomainEventKind kind)
{
    const auto found = std::ranges::find(events, kind, &DomainEvent::kind);
    NINHO_SIM_REQUIRE(found != events.end());
    return *found;
}

void append_events(
    const SimulationSession& session, std::vector<DomainEvent>& destination)
{
    destination.insert(
        destination.end(), session.events().begin(), session.events().end());
}

NINHO_SIM_TEST("speed boost ability parser derives nine arm ticks and shares payload validation")
{
    const std::string canonical = to_canonical_json(archetypes());
    NINHO_SIM_REQUIRE(canonical.find("\"speed_boost\"") != std::string::npos);
    NINHO_SIM_REQUIRE(canonical.find("\"arm_ticks\"") == std::string::npos);
    const auto parsed = parse_archetype_catalog_v2(canonical);
    NINHO_SIM_REQUIRE(parsed.ok());
    NINHO_SIM_REQUIRE(parsed.value.abilities.front().arm_ticks == 9U);
    NINHO_SIM_REQUIRE(std::get<SpeedBoostAbilityDefinition>(
        parsed.value.abilities.front().payload).impulse_m_s == fallback_speed_m_s);

    for (const double invalid : {0.0, -1.0, 1001.0}) {
        const auto rejected = parse_archetype_catalog_v2(
            to_canonical_json(archetypes(invalid)));
        NINHO_SIM_REQUIRE(!rejected.ok());
        NINHO_SIM_REQUIRE(rejected.error.code == ContentErrorCode::OutOfRange);
        NINHO_SIM_REQUIRE(rejected.error.pointer
            == "/abilities/0/payload/impulse_m_s");
    }
}

NINHO_SIM_TEST("speed boost ability typed create and reconfigure validate payload atomically")
{
    const struct InvalidDefinition {
        double value;
        ContentErrorCode code;
    } invalids[]{
        {0.0, ContentErrorCode::OutOfRange},
        {-1.0, ContentErrorCode::OutOfRange},
        {1001.0, ContentErrorCode::OutOfRange},
        {std::numeric_limits<double>::quiet_NaN(), ContentErrorCode::InvalidNumber},
        {std::numeric_limits<double>::infinity(), ContentErrorCode::InvalidNumber},
    };

    auto session = create_session();
    launch(*session);
    const auto canonical_before = session->canonical_state_v3();
    const auto shot_before = session->shot_state();
    const SessionState state_before = session->state();
    const std::vector<EntitySnapshot> snapshots_before{
        session->snapshots().begin(), session->snapshots().end()};
    const std::size_t birds_before = session->birds_remaining();

    for (const InvalidDefinition invalid : invalids) {
        const auto created = SimulationSession::create(
            materials(), archetypes(invalid.value), level());
        NINHO_SIM_REQUIRE(!created.ok());
        NINHO_SIM_REQUIRE(created.error.code == invalid.code);
        NINHO_SIM_REQUIRE(created.error.pointer
            == "/abilities/0/payload/impulse_m_s");

        const SessionStatus status = session->reconfigure(
            materials(), archetypes(invalid.value), level());
        NINHO_SIM_REQUIRE(!status.ok());
        NINHO_SIM_REQUIRE(status.error.code == invalid.code);
        NINHO_SIM_REQUIRE(status.error.pointer
            == "/abilities/0/payload/impulse_m_s");
        NINHO_SIM_REQUIRE(session->canonical_state_v3() == canonical_before);
        NINHO_SIM_REQUIRE(session->shot_state() == shot_before);
        NINHO_SIM_REQUIRE(session->state().tick == state_before.tick);
        NINHO_SIM_REQUIRE(session->state().phase == state_before.phase);
        NINHO_SIM_REQUIRE(session->state().outcome == state_before.outcome);
        NINHO_SIM_REQUIRE(std::ranges::equal(
            session->snapshots(), snapshots_before));
        NINHO_SIM_REQUIRE(session->birds_remaining() == birds_before);
    }
}

NINHO_SIM_TEST("speed boost bird rejects Box3D unsafe mass in JSON and typed boundaries")
{
    constexpr double unsafe_mass_kg = 1.0e-39;
    const auto parsed = parse_archetype_catalog_v2(
        to_canonical_json(archetypes(fallback_speed_m_s, unsafe_mass_kg)));
    NINHO_SIM_REQUIRE(!parsed.ok());
    NINHO_SIM_REQUIRE(parsed.error.code == ContentErrorCode::OutOfRange);
    NINHO_SIM_REQUIRE(parsed.error.pointer == "/birds/0/mass_kg");

    const auto created = SimulationSession::create(materials(),
        archetypes(fallback_speed_m_s, unsafe_mass_kg), level());
    NINHO_SIM_REQUIRE(!created.ok());
    NINHO_SIM_REQUIRE(created.error.code == ContentErrorCode::OutOfRange);
    NINHO_SIM_REQUIRE(created.error.pointer == "/birds/0/mass_kg");
}

NINHO_SIM_TEST("speed boost bird JSON and typed mass validation reject equivalent invalid values")
{
    const struct InvalidMass {
        double value;
        ContentErrorCode typed_code;
    } invalids[]{
        {0.0, ContentErrorCode::OutOfRange},
        {-1.0, ContentErrorCode::OutOfRange},
        {std::numeric_limits<double>::quiet_NaN(), ContentErrorCode::InvalidNumber},
        {std::numeric_limits<double>::infinity(), ContentErrorCode::InvalidNumber},
    };

    for (const InvalidMass invalid : invalids) {
        const auto created = SimulationSession::create(materials(),
            archetypes(fallback_speed_m_s, invalid.value), level());
        NINHO_SIM_REQUIRE(!created.ok());
        NINHO_SIM_REQUIRE(created.error.code == invalid.typed_code);
        NINHO_SIM_REQUIRE(created.error.pointer == "/birds/0/mass_kg");

        auto document = nlohmann::json::parse(to_canonical_json(archetypes()));
        document["birds"][0]["mass_kg"] = invalid.value;
        const auto parsed = parse_archetype_catalog_v2(document.dump());
        NINHO_SIM_REQUIRE(!parsed.ok());
        NINHO_SIM_REQUIRE(parsed.error.pointer == "/birds/0/mass_kg");
    }
}

NINHO_SIM_TEST("speed boost bird rejects unsafe derived runtime sphere density")
{
    constexpr double individually_representable_radius_m =
        static_cast<double>(std::numeric_limits<float>::denorm_min());
    const std::array invalids{
        archetypes(fallback_speed_m_s, 1.0,
            individually_representable_radius_m),
        archetypes(fallback_speed_m_s,
            static_cast<double>(std::numeric_limits<float>::denorm_min()), 100.0),
    };

    for (const auto& invalid : invalids) {
        const auto parsed = parse_archetype_catalog_v2(to_canonical_json(invalid));
        NINHO_SIM_REQUIRE(!parsed.ok());
        NINHO_SIM_REQUIRE(parsed.error.code == ContentErrorCode::OutOfRange);
        NINHO_SIM_REQUIRE(parsed.error.pointer == "/birds/0/mass_kg");

        const auto created = SimulationSession::create(materials(), invalid, level());
        NINHO_SIM_REQUIRE(!created.ok());
        NINHO_SIM_REQUIRE(created.error.code == ContentErrorCode::OutOfRange);
        NINHO_SIM_REQUIRE(created.error.pointer == "/birds/0/mass_kg");
    }
}

NINHO_SIM_TEST("speed boost bird invalid reconfigure preserves live session and queued activation")
{
    auto session = create_session();
    launch(*session);
    advance_to_offset(*session, 8U);
    NINHO_SIM_REQUIRE(session->enqueue(ActivateAbilityCommand{}).ok());
    const auto canonical_before = session->canonical_state_v3();
    const SessionState state_before = session->state();
    const auto shot_before = session->shot_state();
    const std::vector<EntitySnapshot> snapshots_before{
        session->snapshots().begin(), session->snapshots().end()};
    const std::vector<DomainEvent> events_before{
        session->events().begin(), session->events().end()};
    const auto metrics_before = session->physics_metrics();
    const std::size_t birds_before = session->birds_remaining();

    const SessionStatus status = session->reconfigure(materials(),
        archetypes(fallback_speed_m_s, 1.0e-39), level());
    NINHO_SIM_REQUIRE(!status.ok());
    NINHO_SIM_REQUIRE(status.error.code == ContentErrorCode::OutOfRange);
    NINHO_SIM_REQUIRE(status.error.pointer == "/birds/0/mass_kg");
    NINHO_SIM_REQUIRE(session->canonical_state_v3() == canonical_before);
    NINHO_SIM_REQUIRE(session->state().tick == state_before.tick);
    NINHO_SIM_REQUIRE(session->state().phase == state_before.phase);
    NINHO_SIM_REQUIRE(session->state().outcome == state_before.outcome);
    NINHO_SIM_REQUIRE(session->state().aim == state_before.aim);
    NINHO_SIM_REQUIRE(session->state().launcher == state_before.launcher);
    NINHO_SIM_REQUIRE(session->state().last_impact_m == state_before.last_impact_m);
    NINHO_SIM_REQUIRE(session->shot_state() == shot_before);
    NINHO_SIM_REQUIRE(std::ranges::equal(session->snapshots(), snapshots_before));
    NINHO_SIM_REQUIRE(std::ranges::equal(session->events(), events_before));
    NINHO_SIM_REQUIRE(session->birds_remaining() == birds_before);
    const auto metrics_after = session->physics_metrics();
    NINHO_SIM_REQUIRE(metrics_after.body_count == metrics_before.body_count);
    NINHO_SIM_REQUIRE(metrics_after.shape_count == metrics_before.shape_count);
    NINHO_SIM_REQUIRE(metrics_after.joint_count == metrics_before.joint_count);
    NINHO_SIM_REQUIRE(metrics_after.contact_count == metrics_before.contact_count);
    NINHO_SIM_REQUIRE(metrics_after.awake_count == metrics_before.awake_count);

    NINHO_SIM_REQUIRE(session->tick().ok());
    NINHO_SIM_REQUIRE(require_event(
        session->events(), DomainEventKind::SpeedChanged).kind
        == DomainEventKind::SpeedChanged);
}

NINHO_SIM_TEST("speed boost bird smallest proven safe mass launches and accelerates")
{
    constexpr double safe_mass_kg = 1.0e-38;
    const auto parsed = parse_archetype_catalog_v2(
        to_canonical_json(archetypes(fallback_speed_m_s, safe_mass_kg)));
    NINHO_SIM_REQUIRE(parsed.ok());

    constexpr double scaled_spring_constant_n_m =
        520.0 * safe_mass_kg / 6.0;
    auto session = create_session(
        scaled_spring_constant_n_m, fallback_speed_m_s, safe_mass_kg);
    launch(*session);
    advance_to_offset(*session, 8U);
    const EntitySnapshot before = projectile_snapshot(*session);
    NINHO_SIM_REQUIRE(std::isfinite(before.mass_kg));
    NINHO_SIM_REQUIRE(before.mass_kg > 0.0);
    NINHO_SIM_REQUIRE(session->enqueue(ActivateAbilityCommand{}).ok());
    NINHO_SIM_REQUIRE(session->tick().ok());
    const EntitySnapshot after = projectile_snapshot(*session);
    NINHO_SIM_REQUIRE(length(after.linear_velocity_m_s)
        > length(before.linear_velocity_m_s));
    require_event(session->events(), DomainEventKind::SpeedChanged);
}

NINHO_SIM_TEST("speed boost ability rejects launch offsets zero through eight and consumes once at nine")
{
    auto active = create_session();
    begin_pull(*active);
    release(*active, true);
    const TickIndex launched = SessionTestFacade::projectile_launch_tick(*active);
    std::vector<DomainEvent> history;
    append_events(*active, history);
    NINHO_SIM_REQUIRE(require_event(
        active->events(), DomainEventKind::CommandRejected).rejection_reason
        == CommandRejectionReason::NotArmed);

    for (std::uint32_t offset = 1U; offset <= 8U; ++offset) {
        NINHO_SIM_REQUIRE(active->enqueue(ActivateAbilityCommand{}).ok());
        NINHO_SIM_REQUIRE(active->tick().ok());
        NINHO_SIM_REQUIRE(active->state().tick
            == TickIndex{launched.value() + offset});
        append_events(*active, history);
        NINHO_SIM_REQUIRE(active->events().size() == 1U);
        NINHO_SIM_REQUIRE(active->events().front().kind
            == DomainEventKind::CommandRejected);
        NINHO_SIM_REQUIRE(active->events().front().rejection_reason
            == CommandRejectionReason::NotArmed);
    }
    NINHO_SIM_REQUIRE(std::ranges::count(
        history, DomainEventKind::CommandRejected, &DomainEvent::kind) == 9);

    NINHO_SIM_REQUIRE(active->enqueue(ActivateAbilityCommand{}).ok());
    NINHO_SIM_REQUIRE(active->tick().ok());
    append_events(*active, history);
    NINHO_SIM_REQUIRE(active->state().tick == TickIndex{launched.value() + 9U});
    NINHO_SIM_REQUIRE(SessionTestFacade::ability_requested(*active));
    NINHO_SIM_REQUIRE(std::ranges::count(
        history, DomainEventKind::AbilityStarted, &DomainEvent::kind) == 1);
    NINHO_SIM_REQUIRE(std::ranges::count(
        history, DomainEventKind::SpeedChanged, &DomainEvent::kind) == 1);

    NINHO_SIM_REQUIRE(active->enqueue(ActivateAbilityCommand{}).ok());
    NINHO_SIM_REQUIRE(active->tick().ok());
    append_events(*active, history);
    NINHO_SIM_REQUIRE(active->events().size() == 1U);
    NINHO_SIM_REQUIRE(active->events().front().kind
        == DomainEventKind::CommandRejected);
    NINHO_SIM_REQUIRE(active->events().front().rejection_reason
        == CommandRejectionReason::NotArmed);
    NINHO_SIM_REQUIRE(std::ranges::count(
        history, DomainEventKind::AbilityStarted, &DomainEvent::kind) == 1);
    NINHO_SIM_REQUIRE(std::ranges::count(
        history, DomainEventKind::SpeedChanged, &DomainEvent::kind) == 1);
}

NINHO_SIM_TEST("speed boost ability multiplies velocity centrally without changing physical identity")
{
    auto active = create_session();
    auto control = create_session();
    launch(*active);
    launch(*control);
    advance_to_offset(*active, 8U);
    advance_to_offset(*control, 8U);
    const EntitySnapshot before = projectile_snapshot(*active);
    const EntitySnapshot control_before = projectile_snapshot(*control);
    require_vector_near(before.linear_velocity_m_s,
        control_before.linear_velocity_m_s, 1.0e-6);
    const Vec3 direction_before = normalized_or_zero(before.linear_velocity_m_s);
    const double speed_before = length(before.linear_velocity_m_s);
    NINHO_SIM_REQUIRE(speed_before > 0.1);
    NINHO_SIM_REQUIRE(speed_before * speed_multiplier < absolute_speed_cap_m_s);

    NINHO_SIM_REQUIRE(active->enqueue(ActivateAbilityCommand{}).ok());
    NINHO_SIM_REQUIRE(active->tick().ok());
    NINHO_SIM_REQUIRE(control->tick().ok());

    const EntitySnapshot boosted = projectile_snapshot(*active);
    const EntitySnapshot unboosted = projectile_snapshot(*control);
    require_near(length(boosted.linear_velocity_m_s),
        speed_before * speed_multiplier, 2.0e-4);
    require_vector_near(normalized_or_zero(boosted.linear_velocity_m_s),
        direction_before, 1.0e-4);
    require_vector_near(unboosted.linear_velocity_m_s,
        before.linear_velocity_m_s, 1.0e-6);
    require_near(boosted.mass_kg, before.mass_kg, 1.0e-6);
    require_vector_near(boosted.angular_velocity_rad_s,
        before.angular_velocity_rad_s, 1.0e-6);
    NINHO_SIM_REQUIRE(boosted.entity_id == before.entity_id);
    NINHO_SIM_REQUIRE(boosted.part_id == before.part_id);
    NINHO_SIM_REQUIRE(boosted.shape == before.shape);
    NINHO_SIM_REQUIRE(boosted.visual_id == before.visual_id);

    const DomainEvent& changed = require_event(
        active->events(), DomainEventKind::SpeedChanged);
    const Vec3 measured_delta = boosted.linear_velocity_m_s
        - before.linear_velocity_m_s;
    require_vector_near(changed.delta_velocity_m_s, measured_delta, 2.0e-5);
    require_vector_near(changed.impulse_n_s
            / static_cast<float>(before.mass_kg),
        measured_delta, 2.0e-5);
    require_quantized(changed.delta_velocity_m_s);
    require_quantized(changed.impulse_n_s);
    NINHO_SIM_REQUIRE(changed.entity_id == boosted.entity_id);
}

NINHO_SIM_TEST("speed boost ability clamps post ability speed to forty five meters per second")
{
    auto session = create_session(5200.0);
    launch(*session);
    advance_to_offset(*session, 8U);
    const EntitySnapshot before = projectile_snapshot(*session);
    NINHO_SIM_REQUIRE(length(before.linear_velocity_m_s) * speed_multiplier
        > absolute_speed_cap_m_s);
    const Vec3 direction = normalized_or_zero(before.linear_velocity_m_s);

    NINHO_SIM_REQUIRE(session->enqueue(ActivateAbilityCommand{}).ok());
    NINHO_SIM_REQUIRE(session->tick().ok());
    const EntitySnapshot boosted = projectile_snapshot(*session);

    require_near(length(boosted.linear_velocity_m_s),
        absolute_speed_cap_m_s, 2.0e-4);
    require_vector_near(
        normalized_or_zero(boosted.linear_velocity_m_s), direction, 1.0e-4);
}

NINHO_SIM_TEST("speed boost ability changes real velocity for a tiny accepted mass under gravity")
{
    constexpr double tiny_mass_kg = 1.0e-7;
    constexpr Vec3 initial_velocity{6.0F, 8.0F, 0.0F};
    constexpr std::array<double, 3> gravity{0.0, -9.81, 0.0};
    auto active = create_session(
        520.0, fallback_speed_m_s, tiny_mass_kg, gravity);
    auto control = create_session(
        520.0, fallback_speed_m_s, tiny_mass_kg, gravity);
    launch(*active);
    launch(*control);
    advance_to_offset(*active, 7U);
    advance_to_offset(*control, 7U);
    constexpr Vec3 gravity_per_tick{0.0F, -9.81F / 60.0F, 0.0F};
    queue_projectile_velocity(*active, initial_velocity - gravity_per_tick);
    queue_projectile_velocity(*control, initial_velocity - gravity_per_tick);
    NINHO_SIM_REQUIRE(active->tick().ok());
    NINHO_SIM_REQUIRE(control->tick().ok());
    const EntitySnapshot before = projectile_snapshot(*active);
    require_vector_near(before.linear_velocity_m_s, initial_velocity, 3.0e-5);
    NINHO_SIM_REQUIRE(before.mass_kg > 0.0);
    NINHO_SIM_REQUIRE(before.mass_kg < 2.0e-7);

    NINHO_SIM_REQUIRE(active->enqueue(ActivateAbilityCommand{}).ok());
    NINHO_SIM_REQUIRE(active->tick().ok());
    NINHO_SIM_REQUIRE(control->tick().ok());

    const DomainEvent& started = require_event(
        active->events(), DomainEventKind::AbilityStarted);
    const DomainEvent& changed = require_event(
        active->events(), DomainEventKind::SpeedChanged);
    NINHO_SIM_REQUIRE(started.entity_id == before.entity_id);
    NINHO_SIM_REQUIRE(changed.entity_id == before.entity_id);
    const EntitySnapshot boosted = projectile_snapshot(*active);
    const EntitySnapshot unboosted = projectile_snapshot(*control);
    constexpr Vec3 expected_delta_velocity{3.3F, 4.4F, 0.0F};
    require_vector_near(boosted.linear_velocity_m_s
            - unboosted.linear_velocity_m_s,
        expected_delta_velocity, 3.0e-4);
    require_vector_near(changed.delta_velocity_m_s,
        expected_delta_velocity, 1.0e-5);
    NINHO_SIM_REQUIRE(changed.impulse_n_s == Vec3{});
    require_vector_near(boosted.angular_velocity_rad_s,
        before.angular_velocity_rad_s, 1.0e-6);
    require_vector_near(unboosted.angular_velocity_rad_s,
        before.angular_velocity_rad_s, 1.0e-6);
}

NINHO_SIM_TEST("speed boost ability preserves fractional mass precision in normal and near rest paths")
{
    constexpr double fractional_mass_kg = 0.1234567;
    constexpr Vec3 initial_velocity{6.0F, 8.0F, 0.0F};
    auto normal = create_session(
        520.0, fallback_speed_m_s, fractional_mass_kg);
    launch(*normal);
    advance_to_offset(*normal, 8U);
    queue_projectile_velocity(*normal, initial_velocity);
    NINHO_SIM_REQUIRE(normal->tick().ok());
    require_vector_near(projectile_snapshot(*normal).linear_velocity_m_s,
        initial_velocity, 2.0e-5);
    NINHO_SIM_REQUIRE(normal->enqueue(ActivateAbilityCommand{}).ok());
    NINHO_SIM_REQUIRE(normal->tick().ok());
    require_vector_near(projectile_snapshot(*normal).linear_velocity_m_s,
        initial_velocity * static_cast<float>(speed_multiplier), 3.0e-4);

    auto fallback = create_session(
        520.0, fallback_speed_m_s, fractional_mass_kg);
    launch(*fallback);
    const Vec3 last_direction = normalized_or_zero(
        projectile_snapshot(*fallback).linear_velocity_m_s);
    advance_to_offset(*fallback, 8U);
    constexpr Vec3 almost_stopped{6.0e-5F, 8.0e-5F, 0.0F};
    queue_projectile_velocity(*fallback, almost_stopped);
    NINHO_SIM_REQUIRE(fallback->tick().ok());
    require_vector_near(projectile_snapshot(*fallback).linear_velocity_m_s,
        almost_stopped, 2.0e-5);
    NINHO_SIM_REQUIRE(fallback->enqueue(ActivateAbilityCommand{}).ok());
    NINHO_SIM_REQUIRE(fallback->tick().ok());
    const EntitySnapshot restarted = projectile_snapshot(*fallback);
    require_vector_near(restarted.linear_velocity_m_s,
        last_direction * static_cast<float>(fallback_speed_m_s), 3.0e-4);
    require_vector_near(restarted.angular_velocity_rad_s, {}, 1.0e-6);
    require_quantized(require_event(
        fallback->events(), DomainEventKind::SpeedChanged).impulse_n_s);
}

void cancel_projectile_velocity(SimulationSession& session)
{
    const EntitySnapshot before = projectile_snapshot(session);
    NINHO_SIM_REQUIRE(SessionTestFacade::impulse_entity(session,
        before.entity_id,
        before.linear_velocity_m_s * static_cast<float>(-before.mass_kg)));
    NINHO_SIM_REQUIRE(session.tick().ok());
    require_near(length(projectile_snapshot(session).linear_velocity_m_s),
        0.0, 2.0e-5);
}

NINHO_SIM_TEST("speed boost ability uses last valid direction near rest and fails closed without it")
{
    auto fallback = create_session();
    launch(*fallback);
    const Vec3 launch_direction = normalized_or_zero(
        projectile_snapshot(*fallback).linear_velocity_m_s);
    cancel_projectile_velocity(*fallback);
    advance_to_offset(*fallback, 8U);
    const EntitySnapshot stopped = projectile_snapshot(*fallback);
    NINHO_SIM_REQUIRE(length(stopped.linear_velocity_m_s) < 1.0e-4F);
    NINHO_SIM_REQUIRE(fallback->enqueue(ActivateAbilityCommand{}).ok());
    NINHO_SIM_REQUIRE(fallback->tick().ok());
    const EntitySnapshot boosted = projectile_snapshot(*fallback);
    require_near(length(boosted.linear_velocity_m_s), fallback_speed_m_s, 2.0e-4);
    require_vector_near(normalized_or_zero(boosted.linear_velocity_m_s),
        launch_direction, 1.0e-4);

    auto invalid = create_session();
    launch(*invalid);
    cancel_projectile_velocity(*invalid);
    advance_to_offset(*invalid, 8U);
    SessionTestFacade::clear_speed_boost_direction_for_testing(*invalid);
    const EntitySnapshot invalid_before = projectile_snapshot(*invalid);
    const auto shot_before = invalid->shot_state();
    NINHO_SIM_REQUIRE(invalid->enqueue(ActivateAbilityCommand{}).ok());
    const SessionStatus status = invalid->tick();
    NINHO_SIM_REQUIRE(!status.ok());
    NINHO_SIM_REQUIRE(status.error.code == ContentErrorCode::InvalidInvariant);
    NINHO_SIM_REQUIRE(status.error.pointer == "/ability/direction");
    const auto shot_after = invalid->shot_state();
    NINHO_SIM_REQUIRE(shot_after.has_value());
    NINHO_SIM_REQUIRE(shot_after->shot_id == shot_before->shot_id);
    NINHO_SIM_REQUIRE(shot_after->bird_archetype_id
        == shot_before->bird_archetype_id);
    NINHO_SIM_REQUIRE(shot_after->ability_id == shot_before->ability_id);
    NINHO_SIM_REQUIRE(shot_after->launch_tick == shot_before->launch_tick);
    NINHO_SIM_REQUIRE(shot_after->locked_plane == shot_before->locked_plane);
    NINHO_SIM_REQUIRE(shot_after->pull_horizontal_m
        == shot_before->pull_horizontal_m);
    NINHO_SIM_REQUIRE(shot_after->pull_vertical_m
        == shot_before->pull_vertical_m);
    NINHO_SIM_REQUIRE(shot_after->activation_consumed
        == shot_before->activation_consumed);
    NINHO_SIM_REQUIRE(shot_after->projectile_ids == shot_before->projectile_ids);
    NINHO_SIM_REQUIRE(shot_after->ability_readiness == AbilityReadiness::Spent);
    NINHO_SIM_REQUIRE(!SessionTestFacade::ability_requested(*invalid));
    const EntitySnapshot invalid_after = projectile_snapshot(*invalid);
    require_vector_near(invalid_after.transform.position,
        invalid_before.transform.position, 0.0);
    require_vector_near(invalid_after.linear_velocity_m_s,
        invalid_before.linear_velocity_m_s, 0.0);
    require_vector_near(invalid_after.angular_velocity_rad_s,
        invalid_before.angular_velocity_rad_s, 0.0);
    require_near(invalid_after.mass_kg, invalid_before.mass_kg, 0.0);
    NINHO_SIM_REQUIRE(invalid_after.entity_id == invalid_before.entity_id);
    NINHO_SIM_REQUIRE(invalid_after.part_id == invalid_before.part_id);
    NINHO_SIM_REQUIRE(invalid_after.shape == invalid_before.shape);
    NINHO_SIM_REQUIRE(invalid_after.visual_id == invalid_before.visual_id);
    NINHO_SIM_REQUIRE(std::ranges::none_of(
        invalid->events(), [](const DomainEvent& event) {
            return event.kind == DomainEventKind::AbilityStarted
                || event.kind == DomainEventKind::SpeedChanged;
        }));
    const TickIndex fault_tick = invalid->state().tick;
    const SessionStatus repeated = invalid->tick();
    NINHO_SIM_REQUIRE(repeated.error.code == status.error.code);
    NINHO_SIM_REQUIRE(repeated.error.pointer == status.error.pointer);
    NINHO_SIM_REQUIRE(invalid->state().tick == fault_tick);
    NINHO_SIM_REQUIRE(invalid->state().phase == SessionPhase::Faulted);
    NINHO_SIM_REQUIRE(projectile_snapshot(*invalid) == invalid_after);
    NINHO_SIM_REQUIRE(std::ranges::none_of(
        invalid->events(), [](const DomainEvent& event) {
            return event.kind == DomainEventKind::AbilityStarted
                || event.kind == DomainEventKind::SpeedChanged;
        }));
}

struct SpeedReplay {
    std::uint64_t hash{};
    std::vector<std::uint8_t> canonical;
    Vec3 delta_velocity{};
    Vec3 impulse{};

    bool operator==(const SpeedReplay&) const = default;
};

SpeedReplay speed_replay()
{
    auto session = create_session();
    launch(*session);
    advance_to_offset(*session, 8U);
    NINHO_SIM_REQUIRE(session->enqueue(ActivateAbilityCommand{}).ok());
    NINHO_SIM_REQUIRE(session->tick().ok());
    const DomainEvent& event = require_event(
        session->events(), DomainEventKind::SpeedChanged);
    return {session->canonical_hash_v3(), session->canonical_state_v3(),
        event.delta_velocity_m_s, event.impulse_n_s};
}

NINHO_SIM_TEST("speed boost ability fifty repetitions preserve event and canonical hash")
{
    const SpeedReplay expected = speed_replay();
    for (int repetition = 1; repetition < 50; ++repetition) {
        NINHO_SIM_REQUIRE(speed_replay() == expected);
    }
}

NINHO_SIM_TEST("speed boost ability canonical bytes and hash distinguish only last valid direction")
{
    auto session = create_session();
    launch(*session);
    SessionTestFacade::set_speed_boost_direction_for_testing(
        *session, {1.0F, 0.0F, 0.0F});
    SessionTestFacade::refresh_canonical_state(*session);
    const std::vector<std::uint8_t> first_bytes = session->canonical_state_v3();
    const std::uint64_t first_hash = session->canonical_hash_v3();

    SessionTestFacade::set_speed_boost_direction_for_testing(
        *session, {0.0F, 1.0F, 0.0F});
    SessionTestFacade::refresh_canonical_state(*session);
    NINHO_SIM_REQUIRE(session->canonical_state_v3() != first_bytes);
    NINHO_SIM_REQUIRE(session->canonical_hash_v3() != first_hash);
}

NINHO_SIM_TEST("speed boost ability canonical bytes and hash distinguish only event delta velocity")
{
    auto session = create_session();
    launch(*session);
    advance_to_offset(*session, 8U);
    NINHO_SIM_REQUIRE(session->enqueue(ActivateAbilityCommand{}).ok());
    NINHO_SIM_REQUIRE(session->tick().ok());
    require_event(session->events(), DomainEventKind::SpeedChanged);

    NINHO_SIM_REQUIRE(SessionTestFacade::set_last_speed_changed_delta_for_testing(
        *session, {1.0F, 2.0F, 3.0F}));
    SessionTestFacade::refresh_canonical_state(*session);
    const std::vector<std::uint8_t> first_bytes = session->canonical_state_v3();
    const std::uint64_t first_hash = session->canonical_hash_v3();

    NINHO_SIM_REQUIRE(SessionTestFacade::set_last_speed_changed_delta_for_testing(
        *session, {3.0F, 2.0F, 1.0F}));
    SessionTestFacade::refresh_canonical_state(*session);
    NINHO_SIM_REQUIRE(session->canonical_state_v3() != first_bytes);
    NINHO_SIM_REQUIRE(session->canonical_hash_v3() != first_hash);
}

NINHO_SIM_TEST("speed boost ability preserves event tag fourteen after split promotion")
{
    static_assert(static_cast<std::uint8_t>(DomainEventKind::MassChanged) == 13U);
    static_assert(static_cast<std::uint8_t>(DomainEventKind::SpeedChanged) == 14U);
    NINHO_SIM_REQUIRE(ninho::simulation::detail::canonical_tag_of(
        DomainEventKind::MassChanged) == 13U);
    NINHO_SIM_REQUIRE(ninho::simulation::detail::canonical_tag_of(
        DomainEventKind::SpeedChanged) == 14U);

    NINHO_SIM_REQUIRE(SimulationSession::create(
        materials(), archetypes(), level()).ok());
    ArchetypeCatalog explosion = archetypes();
    AbilityArchetype& ability = explosion.abilities.front();
    ability.key = ability.kind = "explosion";
    ability.kind_v2 = AbilityKind::Explosion;
    ability.payload = ExplosionAbilityDefinition{4.0, 12.0, 90.0, 32U};
    const auto created = SimulationSession::create(
        materials(), explosion, level());
    NINHO_SIM_REQUIRE(created.ok());
}

}
