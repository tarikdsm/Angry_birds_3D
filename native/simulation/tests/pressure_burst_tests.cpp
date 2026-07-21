#include "test_framework.hpp"
#include "pressure_burst_system.hpp"
#include "damage_system.hpp"

#include <ninho/physics/physics_world.hpp>

#include <array>
#include <cmath>
#include <limits>
#include <vector>

namespace {

using namespace ninho::physics;
using namespace ninho::simulation;
using namespace ninho::simulation::detail;

PhysicsWorld flat_world()
{
    WorldConfig config = make_legacy_radial_world_config({.surface_gravity = 0});
    config.max_bodies = 64;
    return PhysicsWorld{config};
}

void require_near(double actual, double expected, double tolerance)
{
    NINHO_SIM_REQUIRE(std::abs(actual - expected) <= tolerance);
}

NINHO_SIM_TEST("pressure burst uses exact smoothstep and pre falloff mass caps")
{
    auto world = flat_world();
    const BodyHandle light = world.create_body(
        BodyDesc::dynamic_sphere(0.25F, {{0, 5, 0}, {}}, 100.0F)).value;
    const BodyHandle heavy_midpoint = world.create_body(
        BodyDesc::dynamic_sphere(0.25F, {{2, 5, 0}, {}}, 1000.0F)).value;
    const BodyHandle edge = world.create_body(
        BodyDesc::dynamic_sphere(0.25F, {{4, 5, 0}, {}}, 100.0F)).value;
    world.step();
    const std::array bodies{
        PressureBurstBody{EntityId{2}, PartId{1}, light,
            ninho::simulation::BodyType::Dynamic},
        PressureBurstBody{EntityId{3}, PartId{1}, heavy_midpoint,
            ninho::simulation::BodyType::Dynamic},
        PressureBurstBody{EntityId{4}, PartId{1}, edge,
            ninho::simulation::BodyType::Dynamic},
    };
    const PressureBurstRequest request{.source_entity_id = EntityId{99},
        .origin_m = {0, 5, 0},
        .definition = {4.0, 100.0, 1000.0, false, 32}};
    const auto planned = PressureBurstSystem::plan(
        world, bodies, std::span<const PressureBurstRequest>{&request, 1});
    NINHO_SIM_REQUIRE(planned.ok());
    const auto& effects = planned.value.requests.front().effects;
    NINHO_SIM_REQUIRE(effects.size() == 2U);
    const auto light_effect = std::ranges::find(
        effects, EntityId{2}, &PressureBurstEffect::target_entity_id);
    const auto heavy_effect = std::ranges::find(
        effects, EntityId{3}, &PressureBurstEffect::target_entity_id);
    NINHO_SIM_REQUIRE(light_effect != effects.end());
    NINHO_SIM_REQUIRE(heavy_effect != effects.end());
    const double light_mass = world.state(light)->mass;
    const double heavy_mass = world.state(heavy_midpoint)->mass;
    require_near(length(light_effect->impulse_n_s),
        std::min(100.0, light_mass * 12.0), 1.0e-4);
    require_near(light_effect->energy_j,
        std::min(1000.0, light_mass * 90.0), 1.0e-4);
    require_near(length(heavy_effect->impulse_n_s),
        std::min(100.0, heavy_mass * 12.0) * 0.5, 1.0e-4);
    require_near(heavy_effect->energy_j,
        std::min(1000.0, heavy_mass * 90.0) * 0.5, 1.0e-4);
    NINHO_SIM_REQUIRE(light_mass * 12.0 < 100.0);
    NINHO_SIM_REQUIRE(heavy_mass * 12.0 > 100.0);
}

NINHO_SIM_TEST("pressure burst caps candidates by canonical domain identity")
{
    auto world = flat_world();
    std::array<BodyHandle, 4> handles{};
    for (std::size_t index = 0; index < handles.size(); ++index) {
        handles[index] = world.create_body(BodyDesc::dynamic_sphere(
            0.1F, {{0.5F + static_cast<float>(index) * 0.5F, 5, 0}, {}},
            100.0F)).value;
    }
    world.step();
    const std::array bodies{
        PressureBurstBody{EntityId{40}, PartId{1}, handles[0],
            ninho::simulation::BodyType::Dynamic},
        PressureBurstBody{EntityId{10}, PartId{1}, handles[1],
            ninho::simulation::BodyType::Dynamic},
        PressureBurstBody{EntityId{30}, PartId{1}, handles[2],
            ninho::simulation::BodyType::Dynamic},
        PressureBurstBody{EntityId{20}, PartId{1}, handles[3],
            ninho::simulation::BodyType::Dynamic},
    };
    const PressureBurstRequest request{.source_entity_id = EntityId{99},
        .origin_m = {0, 5, 0},
        .definition = {4.0, 100.0, 1000.0, false, 2}};
    const auto planned = PressureBurstSystem::plan(
        world, bodies, std::span<const PressureBurstRequest>{&request, 1});
    NINHO_SIM_REQUIRE(planned.ok());
    const auto& effects = planned.value.requests.front().effects;
    NINHO_SIM_REQUIRE(effects.size() == 2U);
    NINHO_SIM_REQUIRE(effects[0].target_entity_id == EntityId{10});
    NINHO_SIM_REQUIRE(effects[1].target_entity_id == EntityId{20});
}

NINHO_SIM_TEST("pressure burst plans smoothstep caps source exclusion and static energy")
{
    auto world = flat_world();
    const BodyHandle source = world.create_body(
        BodyDesc::dynamic_sphere(0.25f, {{0, 5, 0}, {}}, 100)).value;
    const BodyHandle center = world.create_body(
        BodyDesc::dynamic_sphere(0.25f, {{0.5f, 5, 0}, {}}, 100)).value;
    const BodyHandle middle = world.create_body(
        BodyDesc::dynamic_sphere(0.25f, {{2, 5, 0}, {}}, 100)).value;
    const BodyHandle edge = world.create_body(
        BodyDesc::dynamic_sphere(0.25f, {{4, 5, 0}, {}}, 100)).value;
    const BodyHandle static_body = world.create_body(
        BodyDesc::static_sphere(0.25f, {{1, 6, 0}, {}})).value;
    world.step();
    const std::array bodies{
        PressureBurstBody{EntityId{1}, PartId{1}, source, ninho::simulation::BodyType::Dynamic},
        PressureBurstBody{EntityId{2}, PartId{1}, center, ninho::simulation::BodyType::Dynamic},
        PressureBurstBody{EntityId{3}, PartId{1}, middle, ninho::simulation::BodyType::Dynamic},
        PressureBurstBody{EntityId{4}, PartId{1}, edge, ninho::simulation::BodyType::Dynamic},
        PressureBurstBody{EntityId{5}, PartId{1}, static_body, ninho::simulation::BodyType::Static, 100.0},
    };
    const PressureBurstRequest request{
        .source_entity_id = EntityId{1},
        .source_part_id = PartId{1},
        .origin_m = {0, 5, 0},
        .definition = {4.0, 1.0e9, 1.0e12, false, 32},
        .cause_event_id = EventId{9},
    };
    const auto planned = PressureBurstSystem::plan(world, bodies,
        std::span<const PressureBurstRequest>{&request, 1});
    NINHO_SIM_REQUIRE(planned.ok());
    NINHO_SIM_REQUIRE(planned.value.requests.size() == 1U);
    const auto& effects = planned.value.requests.front().effects;
    NINHO_SIM_REQUIRE(effects.size() == 3U);
    NINHO_SIM_REQUIRE(std::ranges::none_of(effects, [](const auto& effect) {
        return effect.target_entity_id == EntityId{1}
            || effect.target_entity_id == EntityId{4};
    }));
    const auto center_effect = std::ranges::find(
        effects, EntityId{2}, &PressureBurstEffect::target_entity_id);
    const auto middle_effect = std::ranges::find(
        effects, EntityId{3}, &PressureBurstEffect::target_entity_id);
    const auto static_effect = std::ranges::find(
        effects, EntityId{5}, &PressureBurstEffect::target_entity_id);
    NINHO_SIM_REQUIRE(center_effect != effects.end()
        && middle_effect != effects.end() && static_effect != effects.end());
    const float center_mass = world.state(center)->mass;
    NINHO_SIM_REQUIRE(length(center_effect->impulse_n_s) <= center_mass * 12.0f + 1e-4f);
    NINHO_SIM_REQUIRE(center_effect->energy_j <= center_mass * 90.0 + 1e-4);
    NINHO_SIM_REQUIRE(length(middle_effect->impulse_n_s)
        < length(center_effect->impulse_n_s));
    NINHO_SIM_REQUIRE(static_effect->impulse_n_s == Vec3{});
    NINHO_SIM_REQUIRE(static_effect->energy_j > 0.0);
}

NINHO_SIM_TEST("pressure burst gives structural energy to static hull and hull compound")
{
    auto world = flat_world();
    const HullShape tetrahedron{
        .vertices = {{0, 0, 0}, {1, 0, 0}, {0, 1, 0}, {0, 0, 1}}};
    BodyDesc hull_desc{.type = ninho::physics::BodyType::Static,
        .transform = {{1, 5, 0}, {}}};
    hull_desc.shapes = {{.geometry = tetrahedron, .density = 600.0f}};
    const BodyHandle hull = world.create_body(hull_desc).value;

    HullShape translated = tetrahedron;
    translated.local.position = {1.5f, 0, 0};
    BodyDesc compound_desc{.type = ninho::physics::BodyType::Static,
        .transform = {{2, 5, 0}, {}}};
    compound_desc.shapes = {{
        .geometry = CompoundShape{.children = {tetrahedron, translated}},
        .density = 600.0f,
    }};
    const BodyHandle compound = world.create_body(compound_desc).value;
    world.step();

    const auto hull_mass = world.structural_mass(hull);
    const auto compound_mass = world.structural_mass(compound);
    NINHO_SIM_REQUIRE(hull_mass.has_value() && compound_mass.has_value());
    const std::array bodies{
        PressureBurstBody{EntityId{5}, PartId{1}, hull,
            ninho::simulation::BodyType::Static, *hull_mass},
        PressureBurstBody{EntityId{6}, PartId{1}, compound,
            ninho::simulation::BodyType::Static, *compound_mass},
    };
    const PressureBurstRequest request{.source_entity_id = EntityId{99},
        .origin_m = {0, 5, 0},
        .definition = {5.0, 1000.0, 1000.0, false, 32}};
    const auto planned = PressureBurstSystem::plan(
        world, bodies, std::span<const PressureBurstRequest>{&request, 1});
    NINHO_SIM_REQUIRE(planned.ok());
    NINHO_SIM_REQUIRE(planned.value.requests.front().effects.size() == 2U);
    for (const auto& effect : planned.value.requests.front().effects) {
        NINHO_SIM_REQUIRE(effect.impulse_n_s == Vec3{});
        NINHO_SIM_REQUIRE(effect.energy_j > 0.0);
    }
}

NINHO_SIM_TEST("pressure burst line of sight max domain count and late failure are atomic")
{
    auto world = flat_world();
    const BodyHandle source = world.create_body(
        BodyDesc::dynamic_sphere(0.2f, {{0, 5, 0}, {}}, 100)).value;
    const BodyHandle blocker = world.create_body(
        BodyDesc::static_box({0.1f, 1, 1}, {{1, 5, 0}, {}})).value;
    const BodyHandle target = world.create_body(
        BodyDesc::dynamic_sphere(0.2f, {{2, 5, 0}, {}}, 100)).value;
    world.step();
    const std::array bodies{
        PressureBurstBody{EntityId{1}, PartId{1}, source, ninho::simulation::BodyType::Dynamic},
        PressureBurstBody{EntityId{2}, PartId{1}, blocker, ninho::simulation::BodyType::Static},
        PressureBurstBody{EntityId{3}, PartId{1}, target, ninho::simulation::BodyType::Dynamic},
    };
    PressureBurstRequest request{.source_entity_id = EntityId{1},
        .origin_m = {0, 5, 0},
        .definition = {4.0, 1000.0, 1000.0, true, 32}};
    const auto blocked = PressureBurstSystem::plan(world, bodies,
        std::span<const PressureBurstRequest>{&request, 1});
    NINHO_SIM_REQUIRE(blocked.ok());
    NINHO_SIM_REQUIRE(std::ranges::none_of(blocked.value.requests.front().effects,
        [](const auto& effect) { return effect.target_entity_id == EntityId{3}; }));

    request.origin_m.x = std::numeric_limits<float>::quiet_NaN();
    const auto before = *world.state(target);
    const auto invalid = PressureBurstSystem::plan(world, bodies,
        std::span<const PressureBurstRequest>{&request, 1});
    NINHO_SIM_REQUIRE(!invalid.ok());
    world.step();
    NINHO_SIM_REQUIRE(world.state(target)->linear_velocity == before.linear_velocity);
}

NINHO_SIM_TEST("damage anchor external damage is causal unique and transactional")
{
    MaterialCatalog materials{.schema_version = 2, .source_schema_version = 2,
        .materials = {{MaterialId{1}, "pine", MaterialResponse::Fibrous,
            500, 0.5, 0, 0.5}}};
    ArchetypeCatalog archetypes{.schema_version = 2, .source_schema_version = 2};
    const std::array bodies{
        DamageBody{.entity_id = EntityId{2}, .part_id = PartId{1},
            .material_id = MaterialId{1}},
    };
    const std::array duplicate{
        ExternalDamage{EntityId{1}, PartId{1}, EntityId{2}, PartId{1},
            {}, {1, 0, 0}, 30.0, EventId{55}},
        ExternalDamage{EntityId{1}, PartId{1}, EntityId{2}, PartId{1},
            {}, {1, 0, 0}, 30.0, EventId{55}},
    };
    DamageSystem damage;
    const auto outcomes = damage.process(materials, archetypes, bodies, {}, duplicate);
    NINHO_SIM_REQUIRE(outcomes.size() == 1U);
    NINHO_SIM_REQUIRE(outcomes.front().cause_event_id == EventId{55});
    NINHO_SIM_REQUIRE(outcomes.front().energy_j == 30.0);
    NINHO_SIM_REQUIRE(damage.state(EntityId{2}, PartId{1})->material_damage_energy_j == 30.0);
}

NINHO_SIM_TEST("damage anchor preserves burst cause when same tick contact neutralizes target")
{
    const MaterialCatalog materials{.schema_version = 2, .source_schema_version = 2};
    const ArchetypeCatalog archetypes{.schema_version = 2, .source_schema_version = 2,
        .weakpoints = {{WeakpointId{1}, "profile", {0.0, 0.0, -1.0},
            45.0, 1.0, 1.0}},
        .enemies = {{EnemyArchetypeId{1}, "pig", WeakpointId{1}, SurfaceId{1},
            1.0, 10.0, 1.0, 10.0}}};
    const std::array bodies{
        DamageBody{.entity_id = EntityId{1}, .part_id = PartId{1}},
        DamageBody{.entity_id = EntityId{2}, .part_id = PartId{1},
            .enemy_archetype_id = EnemyArchetypeId{1}},
    };
    const std::array contacts{DamageContact{EntityId{1}, PartId{1},
        EntityId{2}, PartId{1}, {}, {1, 0, 0}, 10.0}};
    const std::array external{ExternalDamage{EntityId{9}, PartId{1},
        EntityId{2}, PartId{1}, {}, {1, 0, 0}, 0.1, EventId{55}}};

    DamageSystem damage;
    const auto outcomes = damage.process(
        materials, archetypes, bodies, contacts, external);
    NINHO_SIM_REQUIRE(std::ranges::count(
        outcomes, EventId{55}, &DamageOutcome::cause_event_id) == 1);
    NINHO_SIM_REQUIRE(damage.state(EntityId{2}, PartId{1})->neutralized);
}

NINHO_SIM_TEST("pressure burst simultaneous requests aggregate central impulse per body")
{
    auto world = flat_world();
    const BodyHandle target = world.create_body(
        BodyDesc::dynamic_sphere(0.5f, {{0, 5, 0}, {}}, 100)).value;
    world.step();
    const auto before = *world.state(target);
    PressureBurstPlan plan;
    plan.requests = {
        PlannedPressureBurst{.effects = {{EntityId{3}, PartId{1}, target,
            before.world_center_of_mass, {1, 0, 0}, {10, 0, 0}, 20.0}}},
        PlannedPressureBurst{.effects = {{EntityId{3}, PartId{1}, target,
            before.world_center_of_mass, {0, 1, 0}, {0, 15, 0}, 30.0}}},
    };
    NINHO_SIM_REQUIRE(PressureBurstSystem::commit_impulses(world, plan).ok());
    world.step();
    const auto after = *world.state(target);
    NINHO_SIM_REQUIRE(std::abs(after.linear_velocity.x
        - (before.linear_velocity.x + 10.0f / before.mass)) < 1.0e-5f);
    NINHO_SIM_REQUIRE(std::abs(after.linear_velocity.y
        - (before.linear_velocity.y + 15.0f / before.mass)) < 1.0e-5f);
}

NINHO_SIM_TEST("damage anchor rejects conflicting external duplicate without state mutation")
{
    MaterialCatalog materials{.schema_version = 2, .source_schema_version = 2,
        .materials = {{MaterialId{1}, "pine", MaterialResponse::Fibrous,
            500, 0.5, 0, 0.5}}};
    ArchetypeCatalog archetypes{.schema_version = 2, .source_schema_version = 2};
    const std::array bodies{DamageBody{.entity_id = EntityId{2},
        .part_id = PartId{1}, .material_id = MaterialId{1}}};
    DamageSystem damage;
    const std::array initial{ExternalDamage{EntityId{1}, PartId{1},
        EntityId{2}, PartId{1}, {}, {1, 0, 0}, 10.0, EventId{55}}};
    NINHO_SIM_REQUIRE(damage.process(materials, archetypes, bodies, {}, initial).size() == 1U);
    const auto before = *damage.state(EntityId{2}, PartId{1});
    const std::array conflicting{
        ExternalDamage{EntityId{1}, PartId{1}, EntityId{2}, PartId{1},
            {}, {1, 0, 0}, 30.0, EventId{77}},
        ExternalDamage{EntityId{1}, PartId{1}, EntityId{2}, PartId{1},
            {}, {0, 1, 0}, 40.0, EventId{77}},
    };
    bool rejected = false;
    try {
        static_cast<void>(damage.process(
            materials, archetypes, bodies, {}, conflicting));
    } catch (const std::invalid_argument&) {
        rejected = true;
    }
    NINHO_SIM_REQUIRE(rejected);
    const auto after = damage.state(EntityId{2}, PartId{1});
    NINHO_SIM_REQUIRE(after.has_value());
    NINHO_SIM_REQUIRE(after->material_damage_energy_j == before.material_damage_energy_j);
    NINHO_SIM_REQUIRE(after->remaining_integrity == before.remaining_integrity);
    NINHO_SIM_REQUIRE(after->neutralized == before.neutralized);
}

}
