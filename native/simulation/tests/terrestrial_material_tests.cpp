#include "test_framework.hpp"
#include "ductile_joint_system.hpp"
#include "damage_system.hpp"

#include <ninho/simulation/content.hpp>
#include <ninho/physics/physics_world.hpp>

#include <cmath>
#include <array>

using namespace ninho::simulation;

NINHO_SIM_TEST("terrestrial materials preserve authored physical constants and failure semantics")
{
    const MaterialDefinition pine{MaterialId{1}, "pine", MaterialResponse::Fibrous,
        520.0, 0.55, 0.15, 0.26};
    const MaterialDefinition glass{MaterialId{9}, "glass", MaterialResponse::Brittle,
        2450.0, 0.38, 0.08, 0.072};
    const MaterialDefinition brick{MaterialId{5}, "brick", MaterialResponse::Masonry,
        1800.0, 0.72, 0.05, 0.52};
    const MaterialDefinition straw{MaterialId{13}, "straw", MaterialResponse::Compressible,
        110.0, 0.82, 0.12, 0.088};
    const MaterialDefinition steel{MaterialId{17}, "sheet_steel", MaterialResponse::Ductile,
        7800.0, 0.48, 0.10, 1.0};

    NINHO_SIM_REQUIRE(pine.density_kg_m3 == 520.0 && pine.friction == 0.55);
    NINHO_SIM_REQUIRE(glass.density_kg_m3 == 2450.0 && glass.restitution == 0.08);
    NINHO_SIM_REQUIRE(brick.density_kg_m3 == 1800.0 && brick.friction == 0.72);
    NINHO_SIM_REQUIRE(straw.density_kg_m3 == 110.0 && straw.restitution == 0.12);
    NINHO_SIM_REQUIRE(steel.density_kg_m3 == 7800.0 && steel.response == MaterialResponse::Ductile);
    static_assert(static_cast<std::uint8_t>(JointKind::StrawBind) == 3U);
    static_assert(static_cast<std::uint8_t>(JointKind::SteelDuctile) == 4U);
}

NINHO_SIM_TEST("terrestrial materials accumulate or peak at the authored exact thresholds")
{
    using namespace ninho::simulation::detail;
    const MaterialCatalog materials{.schema_version = 2, .source_schema_version = 2,
        .materials = {
            {MaterialId{1}, "pine", MaterialResponse::Fibrous, 520, 0.55, 0.15, 0.26},
            {MaterialId{5}, "brick", MaterialResponse::Masonry, 1800, 0.72, 0.05, 0.52},
            {MaterialId{9}, "glass", MaterialResponse::Brittle, 2450, 0.38, 0.08, 0.072},
            {MaterialId{13}, "straw", MaterialResponse::Compressible, 110, 0.82, 0.12, 0.088},
            {MaterialId{17}, "steel", MaterialResponse::Ductile, 7800, 0.48, 0.10, 1.0},
        }};
    const ArchetypeCatalog archetypes{.schema_version = 2, .source_schema_version = 2};
    const std::array bodies{
        DamageBody{.entity_id = EntityId{99}, .part_id = PartId{1}},
        DamageBody{.entity_id = EntityId{1}, .part_id = PartId{1}, .material_id = MaterialId{1}},
        DamageBody{.entity_id = EntityId{5}, .part_id = PartId{1}, .material_id = MaterialId{5}},
        DamageBody{.entity_id = EntityId{9}, .part_id = PartId{1}, .material_id = MaterialId{9}},
        DamageBody{.entity_id = EntityId{13}, .part_id = PartId{1}, .material_id = MaterialId{13}},
        DamageBody{.entity_id = EntityId{17}, .part_id = PartId{1}, .material_id = MaterialId{17}},
    };
    const auto hit = [](EntityId target, double energy) {
        return DamageContact{EntityId{99}, PartId{1}, target, PartId{1},
            {}, {1.0f, 0.0f, 0.0f}, energy};
    };
    DamageSystem damage;
    const std::array first{
        hit(EntityId{1}, 64.999), hit(EntityId{5}, 129.999),
        hit(EntityId{9}, 17.0), hit(EntityId{9}, 17.0),
        hit(EntityId{13}, 21.999), hit(EntityId{17}, 1000000.0)};
    static_cast<void>(damage.process(materials, archetypes, bodies, first));
    NINHO_SIM_REQUIRE(damage.state(EntityId{1}, PartId{1})->material_damage_energy_j == 64.999);
    NINHO_SIM_REQUIRE(damage.state(EntityId{5}, PartId{1})->material_damage_energy_j == 129.999);
    NINHO_SIM_REQUIRE(damage.state(EntityId{9}, PartId{1})->material_damage_energy_j == 17.0);
    NINHO_SIM_REQUIRE(damage.state(EntityId{13}, PartId{1})->material_damage_energy_j == 21.999);
    NINHO_SIM_REQUIRE(damage.state(EntityId{17}, PartId{1})->material_damage_energy_j == 1000000.0);

    const std::array exact{
        hit(EntityId{1}, 0.001), hit(EntityId{5}, 0.001),
        hit(EntityId{9}, 18.0), hit(EntityId{13}, 0.001)};
    static_cast<void>(damage.process(materials, archetypes, bodies, exact));
    NINHO_SIM_REQUIRE(std::abs(
        damage.state(EntityId{1}, PartId{1})->material_damage_energy_j - 65.0) < 1.0e-12);
    NINHO_SIM_REQUIRE(std::abs(
        damage.state(EntityId{5}, PartId{1})->material_damage_energy_j - 130.0) < 1.0e-12);
    NINHO_SIM_REQUIRE(damage.state(EntityId{9}, PartId{1})->material_damage_energy_j == 18.0);
    NINHO_SIM_REQUIRE(std::abs(
        damage.state(EntityId{13}, PartId{1})->material_damage_energy_j - 22.0) < 1.0e-12);

    NINHO_SIM_REQUIRE(250.0 * materials.materials[0].toughness == 65.0);
    NINHO_SIM_REQUIRE(250.0 * materials.materials[1].toughness == 130.0);
    NINHO_SIM_REQUIRE(250.0 * materials.materials[2].toughness == 18.0);
    NINHO_SIM_REQUIRE(250.0 * materials.materials[3].toughness == 22.0);
}

NINHO_SIM_TEST("terrestrial materials ductile joint yields first and recreates only on the next tick")
{
    using namespace ninho::simulation::detail;
    DuctileJointSystem system;
    const DuctileJointSample huge{
        JointId{7}, 10000.0, 2000.0, {}, {}, EventId{41}};

    const auto yielded = system.observe(TickIndex{10}, std::vector{huge});
    NINHO_SIM_REQUIRE(yielded.size() == 1U);
    NINHO_SIM_REQUIRE(yielded.front().kind == DuctileJointTransitionKind::Yielded);
    const auto after_yield = system.state(JointId{7});
    NINHO_SIM_REQUIRE(after_yield.has_value());
    NINHO_SIM_REQUIRE(after_yield->state == DuctileJointState::Yielded);
    NINHO_SIM_REQUIRE(after_yield->pending_recreate);

    NINHO_SIM_REQUIRE(system.recreate_due(TickIndex{10}).empty());
    const auto recreations = system.recreate_due(TickIndex{11});
    NINHO_SIM_REQUIRE(recreations.size() == 1U);
    NINHO_SIM_REQUIRE(recreations.front().joint_id == JointId{7});
    NINHO_SIM_REQUIRE(system.state(JointId{7})->pending_recreate);
    NINHO_SIM_REQUIRE(system.mark_recreated(JointId{7}));
    NINHO_SIM_REQUIRE(!system.state(JointId{7})->pending_recreate);

    const auto broken = system.observe(TickIndex{11}, std::vector{huge});
    NINHO_SIM_REQUIRE(broken.size() == 1U);
    NINHO_SIM_REQUIRE(broken.front().kind == DuctileJointTransitionKind::Broken);
    NINHO_SIM_REQUIRE(system.state(JointId{7})->state == DuctileJointState::Broken);
}

NINHO_SIM_TEST("terrestrial materials ductile joint exact force and torque thresholds are inclusive")
{
    using namespace ninho::simulation::detail;
    DuctileJointSystem force;
    const std::array below_force{DuctileJointSample{JointId{1}, 3199.999, 0.0}};
    const std::array exact_force{DuctileJointSample{JointId{1}, 3200.0, 0.0}};
    NINHO_SIM_REQUIRE(force.observe(TickIndex{1}, below_force).empty());
    NINHO_SIM_REQUIRE(force.observe(TickIndex{2}, exact_force).size() == 1U);

    DuctileJointSystem torque;
    const std::array below_torque{DuctileJointSample{JointId{2}, 0.0, 449.999}};
    const std::array exact_torque{DuctileJointSample{JointId{2}, 0.0, 450.0}};
    NINHO_SIM_REQUIRE(torque.observe(TickIndex{1}, below_torque).empty());
    NINHO_SIM_REQUIRE(torque.observe(TickIndex{2}, exact_torque).size() == 1U);
    NINHO_SIM_REQUIRE(torque.recreate_due(TickIndex{3}).size() == 1U);
    NINHO_SIM_REQUIRE(torque.mark_recreated(JointId{2}));
    const std::array below_break{DuctileJointSample{JointId{2}, 0.0, 899.999}};
    const std::array exact_break{DuctileJointSample{JointId{2}, 0.0, 900.0}};
    NINHO_SIM_REQUIRE(torque.observe(TickIndex{3}, below_break).empty());
    NINHO_SIM_REQUIRE(torque.observe(TickIndex{4}, exact_break).size() == 1U);

    DuctileJointSystem ordered;
    const std::array reversed{
        DuctileJointSample{JointId{9}, 3200.0, 0.0},
        DuctileJointSample{JointId{3}, 3200.0, 0.0}};
    const auto transitions = ordered.observe(TickIndex{1}, reversed);
    NINHO_SIM_REQUIRE(transitions.size() == 2U);
    NINHO_SIM_REQUIRE(transitions[0].joint_id == JointId{3});
    NINHO_SIM_REQUIRE(transitions[1].joint_id == JointId{9});
}

NINHO_SIM_TEST("terrestrial materials replace a joint atomically under the same handle at capacity")
{
    using namespace ninho::physics;
    WorldConfig config;
    config.gravity = UniformGravityConfig{{0.0f, 0.0f, 0.0f}};
    PhysicsWorld world(config);
    auto a = world.create_body(BodyDesc::dynamic_box(
        {0.5f, 0.5f, 0.5f}, {{0.0f, 2.0f, 0.0f}, {}}, 100.0f));
    auto b = world.create_body(BodyDesc::dynamic_box(
        {0.5f, 0.5f, 0.5f}, {{0.0f, 4.0f, 0.0f}, {}}, 100.0f));
    NINHO_SIM_REQUIRE(a && b);
    WeldJointDesc initial{.a = a.value, .b = b.value,
        .frame_a = {{0.0f, 1.0f, 0.0f}, {}},
        .frame_b = {{0.0f, -1.0f, 0.0f}, {}}};
    std::vector<JointHandle> handles;
    for (std::size_t index = 0; index < 250U; ++index) {
        const auto created = world.create_joint(initial);
        NINHO_SIM_REQUIRE(created);
        handles.push_back(created.value);
    }
    NINHO_SIM_REQUIRE(world.commit_pending_initial_state().ok());
    const auto before_a = world.state(a.value);
    const auto before_b = world.state(b.value);
    NINHO_SIM_REQUIRE(before_a && before_b);

    auto yielded = initial;
    yielded.frame_a.position = {0.0f, 1.0f, 0.0f};
    yielded.frame_b.position = {0.0f, -1.0f, 0.0f};
    NINHO_SIM_REQUIRE(world.replace_joint(handles.front(), yielded).ok());
    world.step();
    const auto after_a = world.state(a.value);
    const auto after_b = world.state(b.value);
    NINHO_SIM_REQUIRE(after_a && after_b);
    NINHO_SIM_REQUIRE(after_a->transform == before_a->transform);
    NINHO_SIM_REQUIRE(after_b->transform == before_b->transform);
    NINHO_SIM_REQUIRE(world.joint_reaction(handles.front()).has_value());
}
