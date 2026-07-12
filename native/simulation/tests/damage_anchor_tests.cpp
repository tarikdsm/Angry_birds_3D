#include "test_framework.hpp"
#include "damage_system.hpp"
#include "session_test_facade.hpp"

#include <ninho/physics/physics_world.hpp>
#include <ninho/simulation/session.hpp>

#include <algorithm>
#include <cmath>
#include <fstream>
#include <numbers>
#include <ranges>
#include <sstream>
#include <vector>

namespace {

using ninho::physics::BodyDesc;
using ninho::physics::ContactNormalConvention;
using ninho::physics::PhysicsWorld;
using ninho::physics::contact_normal_convention;
using ninho::physics::derived_contact_energy;
using ninho::physics::stronger_contact_for_pair;

std::string read_text(const char* relative)
{
    std::ifstream input(std::string{NINHO_SOURCE_DIR} + relative, std::ios::binary);
    std::ostringstream contents;
    contents << input.rdbuf();
    return contents.str();
}

ninho::simulation::ContentBundle load_real_bundle()
{
    using namespace ninho::simulation;
    const auto materials = parse_material_catalog(
        read_text("/game/data/materials/vertical_slice.materials.json"));
    const auto archetypes = parse_archetype_catalog(
        read_text("/game/data/archetypes/vertical_slice.archetypes.json"));
    const auto level = parse_level_manifest(
        read_text("/game/data/levels/first_orbit.level.json"));
    NINHO_SIM_REQUIRE(materials.ok() && archetypes.ok() && level.ok());
    const auto bundle = make_content_bundle(materials.value, archetypes.value, level.value);
    NINHO_SIM_REQUIRE(bundle.ok());
    return bundle.value;
}

std::unique_ptr<ninho::simulation::SimulationSession> create_real_session()
{
    using namespace ninho::simulation;
    const auto bundle = load_real_bundle();
    auto session = SimulationSession::create(
        bundle.materials, bundle.archetypes, bundle.level);
    NINHO_SIM_REQUIRE(session.ok());
    return std::move(session.value);
}

NINHO_SIM_TEST("damage anchor contact normal is fixed from a to b")
{
    static_assert(contact_normal_convention == ContactNormalConvention::AToB);

    PhysicsWorld world({.surface_gravity = 0.0f});
    auto a = BodyDesc::dynamic_sphere(0.5f, {{-2.0f, 5.0f, 0.0f}, {}}, 100.0f);
    auto b = BodyDesc::dynamic_sphere(0.5f, {{2.0f, 5.0f, 0.0f}, {}}, 400.0f);
    a.linear_velocity = {10.0f, 0.0f, 0.0f};
    b.linear_velocity = {-10.0f, 0.0f, 0.0f};
    a.bullet = true;
    b.bullet = true;
    const auto handle_a = world.create_body(a);
    const auto handle_b = world.create_body(b);
    NINHO_SIM_REQUIRE(handle_a && handle_b);
    NINHO_SIM_REQUIRE(world.commit_pending_initial_state().ok());

    for (int tick = 0; tick < 30 && world.contact_hits().empty(); ++tick) {
        world.step();
    }

    NINHO_SIM_REQUIRE(world.contact_hits().size() == 1U);
    const auto& contact = world.contact_hits().front();
    NINHO_SIM_REQUIRE(contact.a == handle_a.value);
    NINHO_SIM_REQUIRE(contact.b == handle_b.value);
    NINHO_SIM_REQUIRE(contact.normal.x > 0.99f);
    NINHO_SIM_REQUIRE(std::abs(contact.normal.y) < 1.0e-5f);
    NINHO_SIM_REQUIRE(std::abs(contact.normal.z) < 1.0e-5f);
}

NINHO_SIM_TEST("damage anchor deduplicates strongest contact and preserves energy formula")
{
    const ninho::physics::BodyHandle a{1U, 1U};
    const ninho::physics::BodyHandle b{2U, 1U};
    const ninho::physics::ContactHit weaker{
        .a = a, .b = b, .approach_speed = 7.0f, .effective_mass = 80.0f};
    const ninho::physics::ContactHit stronger{
        .a = a, .b = b, .approach_speed = 11.0f, .effective_mass = 80.0f};

    NINHO_SIM_REQUIRE(stronger_contact_for_pair(stronger, weaker));
    NINHO_SIM_REQUIRE(!stronger_contact_for_pair(weaker, stronger));
    NINHO_SIM_REQUIRE(std::abs(derived_contact_energy(80.0f, 11.0f) - 4000.0f)
        < 1.0e-4f);
    NINHO_SIM_REQUIRE(derived_contact_energy(80.0f, 0.5f) == 0.0f);
}

NINHO_SIM_TEST("damage anchor material responses accumulate or retain an individual peak without regeneration")
{
    using namespace ninho::simulation;
    MaterialCatalog materials{.schema_version = 1,
        .materials = {
            {MaterialId{1}, "pine", MaterialResponse::Fibrous, 1.0, 0.5, 0.0, 0.55},
            {MaterialId{5}, "brick", MaterialResponse::Masonry, 1.0, 0.5, 0.0, 0.52},
            {MaterialId{9}, "glass", MaterialResponse::Brittle, 1.0, 0.5, 0.0, 0.16},
        }};
    const ArchetypeCatalog archetypes{.schema_version = 1};
    detail::DamageSystem damage;
    const std::vector bodies{
        detail::DamageBody{EntityId{10}, PartId{1}, MaterialId{1}},
        detail::DamageBody{EntityId{20}, PartId{1}, MaterialId{5}},
        detail::DamageBody{EntityId{30}, PartId{1}, MaterialId{9}},
        detail::DamageBody{EntityId{99}, PartId{1}},
    };
    const auto impact_b = [](EntityId target, double energy) {
        return detail::DamageContact{EntityId{99}, PartId{1}, target, PartId{1},
            {}, {1.0f, 0.0f, 0.0f}, energy};
    };
    const auto impact_a = [](EntityId target, double energy) {
        return detail::DamageContact{target, PartId{1}, EntityId{99}, PartId{1},
            {}, {1.0f, 0.0f, 0.0f}, energy};
    };

    damage.process(materials, archetypes, bodies,
        std::vector{impact_b(EntityId{10}, 10.0), impact_a(EntityId{20}, 10.0),
            impact_b(EntityId{30}, 10.0)});
    damage.process(materials, archetypes, bodies,
        std::vector{impact_b(EntityId{10}, 20.0), impact_a(EntityId{20}, 20.0),
            impact_b(EntityId{30}, 20.0)});
    damage.process(materials, archetypes, bodies,
        std::vector{impact_b(EntityId{10}, 5.0), impact_a(EntityId{20}, 5.0),
            impact_b(EntityId{30}, 5.0)});

    const auto pine_state = damage.state(EntityId{10}, PartId{1});
    const auto brick_state = damage.state(EntityId{20}, PartId{1});
    const auto glass_state = damage.state(EntityId{30}, PartId{1});
    NINHO_SIM_REQUIRE(pine_state && brick_state && glass_state);
    NINHO_SIM_REQUIRE(pine_state->material_damage_energy_j == 35.0);
    NINHO_SIM_REQUIRE(brick_state->material_damage_energy_j == 35.0);
    NINHO_SIM_REQUIRE(glass_state->material_damage_energy_j == 20.0);
}

NINHO_SIM_TEST("damage anchor uses data driven directional protection mass denominator and ceiling")
{
    using namespace ninho::simulation;
    const MaterialCatalog materials{.schema_version = 1};
    const ArchetypeCatalog archetypes{.schema_version = 1,
        .weakpoints = {{WeakpointId{1}, "anchor_directional_armor",
            {0.0, 0.0, -1.0}, 45.0, 0.25, 1.0}},
        .enemies = {{EnemyArchetypeId{1}, "anchor", WeakpointId{1}, SurfaceId{1004},
            480.0, 100.0, 80.0, 55.0}}};
    const std::vector bodies{
        detail::DamageBody{EntityId{99}, PartId{1}},
        detail::DamageBody{.entity_id = EntityId{300}, .part_id = PartId{1},
            .enemy_archetype_id = EnemyArchetypeId{1}, .transform = {}},
    };
    constexpr double half_denominator_energy = 480.0 * 80.0 * 0.5;
    const auto hit = [](ninho::physics::Vec3 normal, double energy) {
        return detail::DamageContact{EntityId{99}, PartId{1}, EntityId{300}, PartId{1},
            {3.0f, 4.0f, 5.0f}, normal, energy};
    };

    detail::DamageSystem front;
    const auto front_events = front.process(materials, archetypes, bodies,
        std::vector{hit({0.0f, 0.0f, 1.0f}, half_denominator_energy)});
    const ninho::physics::Vec3 expected_position{3.0f, 4.0f, 5.0f};
    const ninho::physics::Vec3 expected_normal{0.0f, 0.0f, 1.0f};
    const auto front_state = front.state(EntityId{300}, PartId{1});
    NINHO_SIM_REQUIRE(front_events.size() == 1U);
    NINHO_SIM_REQUIRE(front_state.has_value());
    NINHO_SIM_REQUIRE(std::abs(front_events.front().damage - 12.5) < 1.0e-9);
    NINHO_SIM_REQUIRE(std::abs(front_state->remaining_integrity - 87.5) < 1.0e-9);
    NINHO_SIM_REQUIRE(front_events.front().cause_entity_id == EntityId{99});
    NINHO_SIM_REQUIRE(front_events.front().target_entity_id == EntityId{300});
    NINHO_SIM_REQUIRE(front_events.front().position_m == expected_position);
    NINHO_SIM_REQUIRE(front_events.front().normal_cause_to_target == expected_normal);
    NINHO_SIM_REQUIRE(front_events.front().energy_j == half_denominator_energy);

    detail::DamageSystem saturated_front;
    const auto saturated_front_events = saturated_front.process(
        materials, archetypes, bodies,
        std::vector{hit({0.0f, 0.0f, 1.0f}, half_denominator_energy * 10.0)});
    NINHO_SIM_REQUIRE(saturated_front_events.size() == 1U);
    NINHO_SIM_REQUIRE(std::abs(saturated_front_events.front().damage - 55.0) < 1.0e-9);

    detail::DamageSystem lateral;
    const auto lateral_events = lateral.process(materials, archetypes, bodies,
        std::vector{hit({1.0f, 0.0f, 0.0f}, half_denominator_energy)});
    NINHO_SIM_REQUIRE(lateral_events.size() == 1U);
    NINHO_SIM_REQUIRE(std::abs(lateral_events.front().damage - 50.0) < 1.0e-9);

    const auto cause_to_target_at = [](double degrees) {
        const double radians = degrees * std::numbers::pi / 180.0;
        return ninho::physics::Vec3{-static_cast<float>(std::sin(radians)), 0.0f,
            static_cast<float>(std::cos(radians))};
    };
    detail::DamageSystem cone_boundary;
    const auto boundary_events = cone_boundary.process(materials, archetypes, bodies,
        std::vector{hit(cause_to_target_at(45.0), half_denominator_energy)});
    NINHO_SIM_REQUIRE(boundary_events.size() == 1U);
    NINHO_SIM_REQUIRE(std::abs(boundary_events.front().damage - 12.5) < 1.0e-9);
    detail::DamageSystem outside_cone;
    const auto outside_events = outside_cone.process(materials, archetypes, bodies,
        std::vector{hit(cause_to_target_at(46.0), half_denominator_energy)});
    NINHO_SIM_REQUIRE(outside_events.size() == 1U);
    NINHO_SIM_REQUIRE(std::abs(outside_events.front().damage - 50.0) < 1.0e-9);

    detail::DamageSystem rear;
    const auto rear_events = rear.process(materials, archetypes, bodies,
        std::vector{hit({0.0f, 0.0f, -1.0f}, half_denominator_energy * 4.0)});
    NINHO_SIM_REQUIRE(rear_events.size() == 1U);
    NINHO_SIM_REQUIRE(std::abs(rear_events.front().damage - 55.0) < 1.0e-9);
}

NINHO_SIM_TEST("damage anchor neutralizes once on integrity or first ejection transition including same tick")
{
    using namespace ninho::simulation;
    const MaterialCatalog materials{.schema_version = 1};
    const ArchetypeCatalog archetypes{.schema_version = 1,
        .weakpoints = {{WeakpointId{1}, "profile", {0.0, 0.0, -1.0}, 45.0, 0.25, 1.0}},
        .enemies = {{EnemyArchetypeId{1}, "enemy", WeakpointId{1}, SurfaceId{1004},
            480.0, 100.0, 80.0, 55.0}}};
    const detail::DamageBody cause{EntityId{99}, PartId{1}};
    const detail::DamageBody anchor{.entity_id = EntityId{300}, .part_id = PartId{1},
        .enemy_archetype_id = EnemyArchetypeId{1},
        .transform = {{0.0f, 0.0f, 50.0f}, {}}, .mass_kg = 480.0,
        .linear_velocity_m_s = {3.0f, 4.0f, 12.0f}, .ejected = false};
    const detail::DamageContact hit{EntityId{99}, PartId{1}, EntityId{300}, PartId{1},
        {1.0f, 2.0f, 3.0f}, {1.0f, 0.0f, 0.0f}, 480.0 * 80.0 * 0.5};

    detail::DamageSystem integrity;
    const std::vector stable_bodies{cause, anchor};
    const auto first_integrity = integrity.process(
        materials, archetypes, stable_bodies, std::vector{hit});
    NINHO_SIM_REQUIRE(first_integrity.size() == 1U);
    const auto lethal = integrity.process(materials, archetypes, stable_bodies,
        std::vector{hit});
    NINHO_SIM_REQUIRE(lethal.size() == 2U);
    NINHO_SIM_REQUIRE(lethal.back().kind == detail::DamageOutcomeKind::EntityNeutralized);
    NINHO_SIM_REQUIRE(lethal.back().neutralization_cause
        == NeutralizationCause::IntegrityDepleted);
    NINHO_SIM_REQUIRE(lethal.back().position_m == hit.position_m);
    NINHO_SIM_REQUIRE(lethal.back().normal_cause_to_target == hit.normal_a_to_b);
    NINHO_SIM_REQUIRE(lethal.back().energy_j == hit.energy_j);
    const auto after_neutralized = integrity.process(
        materials, archetypes, stable_bodies, std::vector{hit});
    NINHO_SIM_REQUIRE(after_neutralized.empty());

    detail::DamageSystem ejection;
    const auto before_ejection = ejection.process(materials, archetypes, stable_bodies, {});
    NINHO_SIM_REQUIRE(before_ejection.empty());
    auto ejected_anchor = anchor;
    ejected_anchor.ejected = true;
    const std::vector ejected_bodies{cause, ejected_anchor};
    const auto first_ejection = ejection.process(materials, archetypes, ejected_bodies, {});
    const ninho::physics::Vec3 expected_ejection_normal{0.0f, 0.0f, 1.0f};
    NINHO_SIM_REQUIRE(first_ejection.size() == 1U);
    NINHO_SIM_REQUIRE(first_ejection.front().kind
        == detail::DamageOutcomeKind::EntityNeutralized);
    NINHO_SIM_REQUIRE(first_ejection.front().neutralization_cause
        == NeutralizationCause::Ejection);
    NINHO_SIM_REQUIRE(first_ejection.front().position_m
        == ejected_anchor.transform.position);
    NINHO_SIM_REQUIRE(first_ejection.front().normal_cause_to_target
        == expected_ejection_normal);
    NINHO_SIM_REQUIRE(std::abs(first_ejection.front().energy_j - 34560.0) < 1.0e-9);
    const auto repeated_ejection = ejection.process(materials, archetypes, ejected_bodies, {});
    NINHO_SIM_REQUIRE(repeated_ejection.empty());

    detail::DamageSystem same_tick;
    const auto simultaneous = same_tick.process(materials, archetypes, ejected_bodies,
        std::vector{hit, hit});
    const auto neutralized_count = std::ranges::count(simultaneous,
        detail::DamageOutcomeKind::EntityNeutralized, &detail::DamageOutcome::kind);
    NINHO_SIM_REQUIRE(neutralized_count == 1);
}

NINHO_SIM_TEST("damage anchor session preserves typed radial ejection causality")
{
    using namespace ninho::simulation;
    auto bundle = load_real_bundle();
    const auto enemy_body = std::ranges::find_if(bundle.level.bodies, [](const auto& body) {
        return body.enemy_archetype_id == EnemyArchetypeId{1};
    });
    NINHO_SIM_REQUIRE(enemy_body != bundle.level.bodies.end());
    enemy_body->transform.position_m = {0.0, 41.0, 0.0};
    auto created = SimulationSession::create(
        bundle.materials, bundle.archetypes, bundle.level);
    NINHO_SIM_REQUIRE(created.ok());
    auto session = std::move(created.value);
    const auto initial = std::ranges::find_if(session->snapshots(), [](const auto& snapshot) {
        return snapshot.enemy_archetype_id == EnemyArchetypeId{1};
    });
    NINHO_SIM_REQUIRE(initial != session->snapshots().end());
    const EntityId target_entity = initial->entity_id;
    const PartId target_part = initial->part_id;
    const auto outward = ninho::physics::normalized_or_zero(initial->transform.position);
    const auto impulse = outward * static_cast<float>(initial->mass_kg * 35.0);
    NINHO_SIM_REQUIRE(detail::SessionTestFacade::impulse_entity(
        *session, target_entity, impulse));

    std::optional<DomainEvent> neutralized;
    std::optional<EntitySnapshot> ejected_snapshot;
    for (int tick = 0; tick < 180 && !neutralized; ++tick) {
        NINHO_SIM_REQUIRE(session->tick().ok());
        const auto event = std::ranges::find_if(session->events(), [&](const DomainEvent& value) {
            return value.kind == DomainEventKind::EntityNeutralized
                && value.affected_entity_id == target_entity;
        });
        if (event == session->events().end()) {
            continue;
        }
        neutralized = *event;
        const auto snapshot = std::ranges::find_if(session->snapshots(), [&](const auto& value) {
            return value.entity_id == target_entity && value.part_id == target_part;
        });
        NINHO_SIM_REQUIRE(snapshot != session->snapshots().end());
        ejected_snapshot = *snapshot;
    }

    NINHO_SIM_REQUIRE(neutralized.has_value() && ejected_snapshot.has_value());
    const auto expected_normal = ninho::physics::normalized_or_zero(
        ejected_snapshot->transform.position);
    const float radial_speed = std::max(0.0f,
        ninho::physics::dot(ejected_snapshot->linear_velocity_m_s, expected_normal));
    const double expected_energy = 0.5 * ejected_snapshot->mass_kg
        * static_cast<double>(radial_speed) * radial_speed;
    NINHO_SIM_REQUIRE(neutralized->neutralization_cause == NeutralizationCause::Ejection);
    NINHO_SIM_REQUIRE(neutralized->entity_id == EntityId{});
    NINHO_SIM_REQUIRE(neutralized->part_id == PartId{});
    NINHO_SIM_REQUIRE(ninho::physics::length(
        neutralized->position_m - ejected_snapshot->transform.position) < 1.0e-4f);
    NINHO_SIM_REQUIRE(ninho::physics::length(
        neutralized->normal - expected_normal) < 1.0e-5f);
    NINHO_SIM_REQUIRE(std::abs(neutralized->energy_j - expected_energy)
        < expected_energy * 1.0e-5);
}

NINHO_SIM_TEST("damage anchor session processes post step contacts into canonical causal events")
{
    using namespace ninho::simulation;
    auto session = create_real_session();
    const auto target = std::ranges::find_if(session->snapshots(), [](const auto& snapshot) {
        return snapshot.enemy_archetype_id == EnemyArchetypeId{1};
    });
    NINHO_SIM_REQUIRE(target != session->snapshots().end());
    NINHO_SIM_REQUIRE(target->shape.type == ShapeType::Box);
    const EntityId target_entity = target->entity_id;
    const PartId target_part = target->part_id;
    const auto target_transform = target->transform;
    const auto target_shape = target->shape;

    constexpr EntityId cause_entity{0x80001000U};
    const float separation = static_cast<float>(target_shape.half_extents_m[0] + 0.52);
    const auto cause_position = target_transform.position
        + ninho::physics::Vec3{-separation, 0.0f, 0.0f};
    NINHO_SIM_REQUIRE(detail::SessionTestFacade::add_dynamic_sphere(*session,
        cause_entity, PartId{1}, cause_position, 140.0, {5.0f, 0.0f, 0.0f}, 0.5));

    std::optional<DomainEvent> applied;
    for (int tick = 0; tick < 30 && !applied; ++tick) {
        NINHO_SIM_REQUIRE(session->tick().ok());
        const auto event = std::ranges::find_if(session->events(), [&](const DomainEvent& value) {
            return value.kind == DomainEventKind::DamageApplied
                && value.affected_entity_id == target_entity;
        });
        if (event != session->events().end()) {
            applied = *event;
        }
    }

    NINHO_SIM_REQUIRE(applied.has_value());
    NINHO_SIM_REQUIRE(applied->entity_id == cause_entity);
    NINHO_SIM_REQUIRE(applied->part_id == PartId{1});
    NINHO_SIM_REQUIRE(applied->affected_entity_id == target_entity);
    NINHO_SIM_REQUIRE(applied->affected_part_id == target_part);
    NINHO_SIM_REQUIRE(applied->energy_j > 0.0);
    NINHO_SIM_REQUIRE(applied->damage > 0.0);
    NINHO_SIM_REQUIRE(ninho::physics::is_finite(applied->position_m));
    NINHO_SIM_REQUIRE(ninho::physics::is_finite(applied->normal));
    NINHO_SIM_REQUIRE(session->canonical_hash_v1() != 0U);
}

}
