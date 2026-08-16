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

void require_published_damage_transition_inherits(bool bounds_exit)
{
    const MaterialCatalog materials{.schema_version = 2, .source_schema_version = 2};
    const auto archetypes = pig_catalog();
    constexpr EventId burst_event{77U};
    const detail::ExternalDamage external{
        .cause_entity_id = EntityId{3059U},
        .cause_part_id = PartId{1U},
        .target_entity_id = EntityId{200U},
        .target_part_id = PartId{1U},
        .position_m = {17.0F, 0.5F, -5.0F},
        .normal_cause_to_target = {0.0F, 0.0F, -1.0F},
        .energy_j = 2'000.0,
        .cause_event_id = burst_event,
    };
    detail::DamageSystem system;
    const auto damaged = system.process(materials, archetypes,
        std::array{pig_body()}, std::array<detail::DamageContact, 0>{},
        std::array{external});
    NINHO_SIM_REQUIRE(damaged.size() == 1U);
    NINHO_SIM_REQUIRE(damaged.front().kind
        == detail::DamageOutcomeKind::DamageApplied);
    NINHO_SIM_REQUIRE(damaged.front().cause_event_id == burst_event);
    constexpr EventId published_damage_event{78U};
    NINHO_SIM_REQUIRE(system.record_causal_damage_event(
        external.target_entity_id, external.target_part_id,
        published_damage_event));

    auto transitioned = pig_body();
    transitioned.bounds_exit = bounds_exit;
    transitioned.ejected = !bounds_exit;
    transitioned.transform.position = {4.0F, 0.0F, 0.0F};
    transitioned.linear_velocity_m_s = {3.0F, 0.0F, 0.0F};
    const auto neutralized = system.process(materials, archetypes,
        std::array{transitioned}, std::array<detail::DamageContact, 0>{});
    NINHO_SIM_REQUIRE(neutralized.size() == 1U);
    NINHO_SIM_REQUIRE(neutralized.front().kind
        == detail::DamageOutcomeKind::EntityNeutralized);
    NINHO_SIM_REQUIRE(neutralized.front().neutralization_cause
        == (bounds_exit ? NeutralizationCause::BoundsExit
                        : NeutralizationCause::Ejection));
    NINHO_SIM_REQUIRE(neutralized.front().cause_event_id
        == published_damage_event);
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

NINHO_SIM_TEST("pig damage remembers external causal events across process calls")
{
    const MaterialCatalog materials{.schema_version = 2, .source_schema_version = 2};
    const auto archetypes = pig_catalog();
    const std::array bodies{pig_body()};
    const detail::ExternalDamage first{EntityId{77}, PartId{1},
        EntityId{200}, PartId{1}, {}, {0.0f, 1.0f, 0.0f},
        65.0 * 20.0, EventId{51}};
    auto distinct = first;
    distinct.cause_event_id = EventId{52};

    detail::DamageSystem system;
    NINHO_SIM_REQUIRE(system.process(
        materials, archetypes, bodies, {}, std::array{first}).size() == 1U);
    NINHO_SIM_REQUIRE(system.process(
        materials, archetypes, bodies, {}, std::array{first}).empty());
    NINHO_SIM_REQUIRE(system.process(
        materials, archetypes, bodies, {}, std::array{distinct}).size() == 1U);
    NINHO_SIM_REQUIRE(std::abs(system.state(
        EntityId{200}, PartId{1})->remaining_integrity - 96.4) < 1.0e-12);
}

NINHO_SIM_TEST("pig damage uses entity mass and deduplicates one cause across body parts")
{
    const MaterialCatalog materials{.schema_version = 2, .source_schema_version = 2};
    const auto archetypes = pig_catalog();
    auto first_part = pig_body();
    first_part.mass_kg = 32.5;
    auto second_part = first_part;
    second_part.part_id = PartId{2};
    const detail::DamageBody source{.entity_id = EntityId{99},
        .part_id = PartId{1}, .mass_kg = 20.0};
    const std::array bodies{source, first_part, second_part};

    constexpr double effective_mass = 20.0;
    constexpr double entity_specific_energy = 20.0;
    const double raw_energy = 65.0 * entity_specific_energy;
    const double normal_speed = std::sqrt(2.0 * raw_energy / effective_mass);
    const detail::DamageContact contact{EntityId{99}, PartId{1},
        EntityId{200}, PartId{2}, {}, {1.0f, 0.0f, 0.0f}, 0.0,
        normal_speed, effective_mass};
    detail::DamageSystem contact_system;
    const auto contact_outcome = contact_system.process(
        materials, archetypes, bodies, std::array{contact});
    NINHO_SIM_REQUIRE(contact_outcome.size() == 1U);
    NINHO_SIM_REQUIRE(std::abs(contact_outcome.front().energy_j - raw_energy) < 1.0e-8);
    NINHO_SIM_REQUIRE(std::abs(contact_outcome.front().damage - 1.8) < 1.0e-10);

    const detail::ExternalDamage first{EntityId{77}, PartId{1},
        EntityId{200}, PartId{1}, {}, {0.0f, 1.0f, 0.0f},
        raw_energy, EventId{61}};
    auto second = first;
    second.target_part_id = PartId{2};
    detail::DamageSystem external_system;
    const auto external_outcome = external_system.process(
        materials, archetypes, bodies, {}, std::array{first, second});
    NINHO_SIM_REQUIRE(external_outcome.size() == 1U);
    NINHO_SIM_REQUIRE(std::abs(external_outcome.front().damage - 1.8) < 1.0e-10);
    NINHO_SIM_REQUIRE(std::abs(external_system.state(
        EntityId{200}, PartId{1})->remaining_integrity - 98.2) < 1.0e-10);
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

NINHO_SIM_TEST("pig damage crush preserves the deterministic causal load entity")
{
    using namespace ninho::simulation::detail;
    CrushDamageSystem system;
    const CrushBody pig{EntityId{200}, PartId{1}, 65.0, 9.81,
        false, {}, {0.0f, 1.0f, 0.0f}};
    constexpr double dt = 1.0 / 60.0;
    const double qualifying = pig.mass_kg * pig.gravity_m_s2 * dt * 5.0;
    std::vector<CrushDamagePlan> plans;
    for (std::uint64_t tick = 1U; tick <= 21U; ++tick) {
        const std::array loads{
            CrushContactLoad{pig.entity_id, pig.part_id, qualifying,
                EntityId{77}},
            CrushContactLoad{pig.entity_id, pig.part_id, qualifying,
                EntityId{66}},
        };
        plans = system.update(TickIndex{tick}, dt, std::span{&pig, 1U}, loads);
    }
    NINHO_SIM_REQUIRE(plans.size() == 1U);
    NINHO_SIM_REQUIRE(plans.front().cause_entity_id == EntityId{66});
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
    NINHO_SIM_REQUIRE(first.front().cause_event_id == EventId{});
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
    NINHO_SIM_REQUIRE(radial_outcome.front().cause_event_id == EventId{});
}

NINHO_SIM_TEST("pig damage bounds exit inherits the last published damage event")
{
    require_published_damage_transition_inherits(true);
}

NINHO_SIM_TEST("pig damage ejection inherits the last published damage event")
{
    require_published_damage_transition_inherits(false);
}

NINHO_SIM_TEST("pig damage never reuses an external receipt after newer contact damage")
{
    const MaterialCatalog materials{.schema_version = 2, .source_schema_version = 2};
    const auto archetypes = pig_catalog();
    const detail::DamageBody source{
        .entity_id = EntityId{99U}, .part_id = PartId{1U}, .mass_kg = 20.0};
    const std::array stable_bodies{source, pig_body()};
    const detail::ExternalDamage external{
        .cause_entity_id = EntityId{77U},
        .cause_part_id = PartId{1U},
        .target_entity_id = EntityId{200U},
        .target_part_id = PartId{1U},
        .normal_cause_to_target = {0.0F, 1.0F, 0.0F},
        .energy_j = 65.0 * 20.0,
        .cause_event_id = EventId{51U},
    };
    const detail::DamageContact contact{
        .a_entity_id = source.entity_id,
        .a_part_id = source.part_id,
        .b_entity_id = EntityId{200U},
        .b_part_id = PartId{1U},
        .normal_a_to_b = {1.0F, 0.0F, 0.0F},
        .normal_speed_m_s = 20.0,
        .effective_mass_kg = 6.5,
    };

    for (const bool bounds_exit : {false, true}) {
        detail::DamageSystem system;
        NINHO_SIM_REQUIRE(system.process(materials, archetypes,
            stable_bodies, {}, std::span{&external, 1U}).size() == 1U);
        const auto contacted = system.process(materials, archetypes,
            stable_bodies, std::span{&contact, 1U});
        NINHO_SIM_REQUIRE(contacted.size() == 1U);
        NINHO_SIM_REQUIRE(contacted.front().kind
            == detail::DamageOutcomeKind::DamageApplied);
        constexpr EventId published_contact_event{80U};
        NINHO_SIM_REQUIRE(system.record_causal_damage_event(
            EntityId{200U}, PartId{1U}, published_contact_event));

        auto exited = pig_body();
        exited.bounds_exit = bounds_exit;
        exited.ejected = !bounds_exit;
        exited.transform.position = {4.0F, 0.0F, 0.0F};
        exited.linear_velocity_m_s = {3.0F, 0.0F, 0.0F};
        const auto neutralized = system.process(materials, archetypes,
            std::array{exited}, {});
        NINHO_SIM_REQUIRE(neutralized.size() == 1U);
        NINHO_SIM_REQUIRE(neutralized.front().cause_event_id
            == published_contact_event);
    }
}

NINHO_SIM_TEST("pig damage causal anchor accepts only newer published damage event ids")
{
    const MaterialCatalog materials{.schema_version = 2, .source_schema_version = 2};
    const auto archetypes = pig_catalog();
    detail::DamageSystem system;
    const std::array bodies{pig_body()};
    NINHO_SIM_REQUIRE(system.process(materials, archetypes, bodies, {}).empty());

    NINHO_SIM_REQUIRE(system.record_causal_damage_event(
        EntityId{200U}, PartId{1U}, EventId{70U}));
    NINHO_SIM_REQUIRE(!system.record_causal_damage_event(
        EntityId{200U}, PartId{1U}, EventId{69U}));
    NINHO_SIM_REQUIRE(!system.record_causal_damage_event(
        EntityId{200U}, PartId{1U}, EventId{}));
    const auto anchored = system.state(EntityId{200U}, PartId{1U});
    NINHO_SIM_REQUIRE(anchored.has_value());
    NINHO_SIM_REQUIRE(anchored->last_causal_damage_event_id == EventId{70U});

    auto exited = pig_body();
    exited.bounds_exit = true;
    const auto neutralized = system.process(
        materials, archetypes, std::array{exited}, {});
    NINHO_SIM_REQUIRE(neutralized.size() == 1U);
    NINHO_SIM_REQUIRE(neutralized.front().cause_event_id == EventId{70U});
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
