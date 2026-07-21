#include "test_framework.hpp"
#include "crush_damage_system.hpp"
#include "damage_system.hpp"

#include <ninho/simulation/content.hpp>

#include <cmath>
#include <array>
#include <span>

using namespace ninho::simulation;

namespace {

ArchetypeCatalog pig_catalog()
{
    return {.schema_version = 2,
        .source_schema_version = 2,
        .weakpoints = {{WeakpointId{2}, "uniform", {0.0, 1.0, 0.0}, 1.0, 1.0, 1.0}},
        .enemies = {{EnemyArchetypeId{2}, "terrestrial_pig", WeakpointId{2}, SurfaceId{1005},
            65.0, 100.0, 18.0, 70.0, EnemyDamageModel::TerrestrialPig}}};
}

detail::DamageBody pig_body()
{
    return {.entity_id = EntityId{200}, .part_id = PartId{1},
        .enemy_archetype_id = EnemyArchetypeId{2}, .mass_kg = 65.0};
}

}

NINHO_SIM_TEST("pig damage raw specific energy threshold and cap are exact")
{
    const MaterialCatalog materials{.schema_version = 2, .source_schema_version = 2};
    const auto archetypes = pig_catalog();
    const std::vector bodies{detail::DamageBody{EntityId{99}, PartId{1}}, pig_body()};
    const auto hit = [](double normal_speed) {
        return detail::DamageContact{EntityId{99}, PartId{1}, EntityId{200}, PartId{1},
            {}, {1.0f, 0.0f, 0.0f}, 0.5 * 65.0 * normal_speed * normal_speed,
            normal_speed, 65.0};
    };

    detail::DamageSystem below;
    NINHO_SIM_REQUIRE(below.process(materials, archetypes, bodies,
        std::vector{hit(std::sqrt(2.0 * 17.999))}).empty());
    detail::DamageSystem exact;
    NINHO_SIM_REQUIRE(exact.process(materials, archetypes, bodies,
        std::vector{hit(6.0)}).empty());
    detail::DamageSystem above;
    const auto tiny = above.process(materials, archetypes, bodies,
        std::vector{hit(std::sqrt(2.0 * 18.001))});
    NINHO_SIM_REQUIRE(tiny.size() == 1U);
    NINHO_SIM_REQUIRE(std::abs(tiny.front().damage - 0.0009) < 1.0e-8);
    detail::DamageSystem capped;
    const auto large = capped.process(materials, archetypes, bodies,
        std::vector{hit(20.0)});
    NINHO_SIM_REQUIRE(large.size() == 1U && large.front().damage == 70.0);
}

NINHO_SIM_TEST("pig damage uses raw normal contact energy without the legacy deadzone")
{
    const MaterialCatalog materials{.schema_version = 2, .source_schema_version = 2};
    const auto archetypes = pig_catalog();
    const std::vector bodies{detail::DamageBody{EntityId{99}, PartId{1}}, pig_body()};
    const double normal_speed = std::sqrt(2.0 * 18.001);
    const detail::DamageContact contact{EntityId{99}, PartId{1},
        EntityId{200}, PartId{1}, {}, {1.0f, 0.0f, 0.0f},
        0.0, normal_speed, 65.0};

    detail::DamageSystem system;
    const auto outcomes = system.process(
        materials, archetypes, bodies, std::array{contact});
    NINHO_SIM_REQUIRE(outcomes.size() == 1U);
    NINHO_SIM_REQUIRE(std::abs(outcomes.front().damage - 0.0009) < 1.0e-8);
    NINHO_SIM_REQUIRE(std::abs(outcomes.front().energy_j - 65.0 * 18.001) < 1.0e-8);
}

NINHO_SIM_TEST("pig damage is uniform by direction and explosions share the same integrity path")
{
    const MaterialCatalog materials{.schema_version = 2, .source_schema_version = 2};
    const auto archetypes = pig_catalog();
    const std::vector bodies{detail::DamageBody{EntityId{99}, PartId{1}}, pig_body()};
    constexpr double specific_energy = 20.0;
    const double raw_energy = 65.0 * specific_energy;
    const double speed = std::sqrt(2.0 * specific_energy);
    const std::array normals{
        ninho::physics::Vec3{1.0f, 0.0f, 0.0f},
        ninho::physics::Vec3{-1.0f, 0.0f, 0.0f},
        ninho::physics::Vec3{0.0f, 1.0f, 0.0f},
        ninho::physics::Vec3{0.0f, 0.0f, -1.0f}};
    for (const auto normal : normals) {
        detail::DamageSystem contact_system;
        const detail::DamageContact contact{EntityId{99}, PartId{1},
            EntityId{200}, PartId{1}, {}, normal, 0.0, speed, 65.0};
        const auto contact_outcome = contact_system.process(
            materials, archetypes, bodies, std::array{contact});
        NINHO_SIM_REQUIRE(contact_outcome.size() == 1U);
        NINHO_SIM_REQUIRE(std::abs(contact_outcome.front().damage - 1.8) < 1.0e-12);
    }

    detail::DamageSystem explosion_system;
    const detail::ExternalDamage explosion{EntityId{77}, PartId{1},
        EntityId{200}, PartId{1}, {}, {0.0f, 1.0f, 0.0f}, raw_energy, EventId{51}};
    const auto explosion_outcome = explosion_system.process(
        materials, archetypes, bodies, {}, std::array{explosion, explosion});
    NINHO_SIM_REQUIRE(explosion_outcome.size() == 1U);
    NINHO_SIM_REQUIRE(std::abs(explosion_outcome.front().damage - 1.8) < 1.0e-12);
    NINHO_SIM_REQUIRE(explosion_outcome.front().cause_event_id == EventId{51});
    NINHO_SIM_REQUIRE(std::abs(explosion_system.state(
        EntityId{200}, PartId{1})->remaining_integrity - 98.2) < 1.0e-12);
}

NINHO_SIM_TEST("pig damage crush requires strictly over four weights for twenty one continuous ticks")
{
    using namespace ninho::simulation::detail;
    CrushDamageSystem system;
    CrushBody pig{EntityId{200}, PartId{1}, 65.0, 9.81, false, {}, {0.0f, 1.0f, 0.0f}};
    const std::array pigs{pig};
    constexpr double dt = 1.0 / 60.0;
    const double weight_impulse = pig.mass_kg * pig.gravity_m_s2 * dt;

    for (std::uint64_t tick = 1; tick <= 21; ++tick) {
        const std::array loads{CrushContactLoad{
            EntityId{200}, PartId{1}, 4.0 * weight_impulse}};
        const auto plan = system.update(TickIndex{tick}, dt, pigs, loads);
        NINHO_SIM_REQUIRE(plan.empty());
    }
    for (std::uint64_t tick = 22; tick <= 41; ++tick) {
        const std::array loads{CrushContactLoad{
            EntityId{200}, PartId{1}, 5.0 * weight_impulse}};
        const auto plan = system.update(TickIndex{tick}, dt, pigs, loads);
        NINHO_SIM_REQUIRE(plan.empty());
    }
    const std::array loads{CrushContactLoad{
        EntityId{200}, PartId{1}, 5.0 * weight_impulse}};
    const auto plan = system.update(TickIndex{42}, dt, pigs, loads);
    NINHO_SIM_REQUIRE(plan.size() == 1U);
    const double expected_damage = 0.9 * 0.5 * std::pow(21.0 * 9.81 / 60.0, 2.0);
    NINHO_SIM_REQUIRE(std::abs(plan.front().predicted_damage - expected_damage) < 1.0e-6);
    NINHO_SIM_REQUIRE(system.state(EntityId{200}, PartId{1})->streak == 0U);
    NINHO_SIM_REQUIRE(system.record_cause(
        EntityId{200}, PartId{1}, EventId{73}));
    NINHO_SIM_REQUIRE(system.state(EntityId{200}, PartId{1})->cause_event_id
        == EventId{73});
}

NINHO_SIM_TEST("pig damage uniform world exit uses append only BoundsExit cause")
{
    static_assert(static_cast<std::uint8_t>(NeutralizationCause::BoundsExit) == 3U);
    static_assert(static_cast<std::uint8_t>(DomainEventKind::MaterialYielded) == 21U);
    static_assert(static_cast<std::uint8_t>(DomainEventKind::CrushDamageApplied) == 22U);

    const MaterialCatalog materials{.schema_version = 2, .source_schema_version = 2};
    const auto archetypes = pig_catalog();
    auto exited = pig_body();
    exited.bounds_exit = true;
    exited.linear_velocity_m_s = {3.0f, 0.0f, 0.0f};
    detail::DamageSystem uniform;
    const auto first = uniform.process(
        materials, archetypes, std::array{exited}, std::array<detail::DamageContact, 0>{});
    NINHO_SIM_REQUIRE(first.size() == 1U);
    NINHO_SIM_REQUIRE(first.front().kind == detail::DamageOutcomeKind::EntityNeutralized);
    NINHO_SIM_REQUIRE(first.front().neutralization_cause == NeutralizationCause::BoundsExit);
    NINHO_SIM_REQUIRE(uniform.process(
        materials, archetypes, std::array{exited}, std::array<detail::DamageContact, 0>{}).empty());

    auto ejected = pig_body();
    ejected.ejected = true;
    ejected.transform.position = {4.0f, 0.0f, 0.0f};
    ejected.linear_velocity_m_s = {3.0f, 0.0f, 0.0f};
    detail::DamageSystem radial;
    const auto radial_outcome = radial.process(
        materials, archetypes, std::array{ejected}, std::array<detail::DamageContact, 0>{});
    NINHO_SIM_REQUIRE(radial_outcome.size() == 1U);
    NINHO_SIM_REQUIRE(radial_outcome.front().neutralization_cause
        == NeutralizationCause::Ejection);
}

NINHO_SIM_TEST("pig damage crush quantizes the boundary and aggregates contact loads")
{
    using namespace ninho::simulation::detail;
    constexpr double dt = 1.0 / 60.0;
    const CrushBody pig{EntityId{200}, PartId{1}, 65.0, 9.81, false};
    const std::array pigs{pig};
    const double weight = pig.mass_kg * pig.gravity_m_s2 * dt;
    CrushDamageSystem system;

    const std::array below{CrushContactLoad{
        pig.entity_id, pig.part_id, 4.000004 * weight}};
    NINHO_SIM_REQUIRE(system.update(TickIndex{1}, dt, pigs, below).empty());
    NINHO_SIM_REQUIRE(system.state(pig.entity_id, pig.part_id)->streak == 0U);

    const std::array split_load{
        CrushContactLoad{pig.entity_id, pig.part_id, 2.000003 * weight},
        CrushContactLoad{pig.entity_id, pig.part_id, 2.000003 * weight}};
    NINHO_SIM_REQUIRE(system.update(TickIndex{2}, dt, pigs, split_load).empty());
    NINHO_SIM_REQUIRE(system.state(pig.entity_id, pig.part_id)->streak == 1U);
}

NINHO_SIM_TEST("pig damage crush resets on interruption and emits once per twenty one tick window")
{
    using namespace ninho::simulation::detail;
    constexpr double dt = 1.0 / 60.0;
    const CrushBody pig{EntityId{200}, PartId{1}, 65.0, 9.81, false};
    const std::array pigs{pig};
    const double weight = pig.mass_kg * pig.gravity_m_s2 * dt;
    const std::array qualifying{CrushContactLoad{
        pig.entity_id, pig.part_id, 5.0 * weight}};
    const std::array interrupted{CrushContactLoad{
        pig.entity_id, pig.part_id, 4.0 * weight}};
    CrushDamageSystem system;
    for (std::uint64_t tick = 1; tick <= 20; ++tick) {
        NINHO_SIM_REQUIRE(system.update(TickIndex{tick}, dt, pigs, qualifying).empty());
    }
    NINHO_SIM_REQUIRE(system.update(TickIndex{21}, dt, pigs, interrupted).empty());
    NINHO_SIM_REQUIRE(system.state(pig.entity_id, pig.part_id)->streak == 0U);

    std::size_t emitted = 0U;
    for (std::uint64_t tick = 22; tick <= 63; ++tick) {
        emitted += system.update(TickIndex{tick}, dt, pigs, qualifying).size();
    }
    NINHO_SIM_REQUIRE(emitted == 2U);
    NINHO_SIM_REQUIRE(system.state(pig.entity_id, pig.part_id)->streak == 0U);
}

NINHO_SIM_TEST("pig damage crush safely ignores zero gravity missing and neutralized bodies")
{
    using namespace ninho::simulation::detail;
    constexpr double dt = 1.0 / 60.0;
    CrushDamageSystem system;
    NINHO_SIM_REQUIRE(system.update(
        TickIndex{1}, dt, std::span<const CrushBody>{}, std::span<const CrushContactLoad>{}).empty());

    const CrushBody zero_g{EntityId{1}, PartId{1}, 65.0, 0.0, false};
    const std::array zero_g_bodies{zero_g};
    const std::array load{CrushContactLoad{zero_g.entity_id, zero_g.part_id, 1000.0}};
    NINHO_SIM_REQUIRE(system.update(TickIndex{2}, dt, zero_g_bodies, load).empty());
    NINHO_SIM_REQUIRE(system.state(zero_g.entity_id, zero_g.part_id)->streak == 0U);

    const CrushBody neutralized{EntityId{2}, PartId{1}, 65.0, 9.81, true};
    const std::array neutralized_bodies{neutralized};
    const std::array huge{CrushContactLoad{
        neutralized.entity_id, neutralized.part_id, 1000000.0}};
    NINHO_SIM_REQUIRE(system.update(TickIndex{3}, dt,
        neutralized_bodies, huge).empty());
    NINHO_SIM_REQUIRE(system.state(
        neutralized.entity_id, neutralized.part_id)->streak == 0U);
}
