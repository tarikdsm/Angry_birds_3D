#include "test_framework.hpp"
#include "environmental_trigger_system.hpp"

#include <ninho/physics/physics_world.hpp>

#include <array>
#include <ranges>

namespace {
using namespace ninho::simulation;
using namespace ninho::simulation::detail;

NINHO_SIM_TEST("environmental trigger accumulates exact target damage and schedules fuse zero next tick")
{
    ninho::physics::PhysicsWorld physics(
        ninho::physics::make_legacy_radial_world_config({.surface_gravity = 0}));
    const auto handle = physics.create_body(ninho::physics::BodyDesc::static_box(
        {0.5f, 0.5f, 0.5f}, {{3, 4, 5}, {}})).value;
    physics.step();
    const std::array definitions{EnvironmentalTriggerDefinition{.id = 9,
        .target_entity_id = EntityId{20}, .damage_threshold = 10.0,
        .fuse_ticks = 0, .cooldown_ticks = 7,
        .pressure_burst = {4.0, 100.0, 200.0, false, 16}}};
    const std::array bodies{PressureBurstBody{EntityId{20}, PartId{1}, handle,
        BodyType::Static, 100.0}};
    auto runtimes = EnvironmentalTriggerSystem::initialize(definitions);
    const std::array first_events{
        DomainEvent{.id = EventId{1}, .tick = TickIndex{5},
            .kind = DomainEventKind::DamageApplied,
            .affected_entity_id = EntityId{99}, .damage = 100.0},
        DomainEvent{.id = EventId{2}, .tick = TickIndex{5},
            .kind = DomainEventKind::DamageApplied,
            .affected_entity_id = EntityId{20}, .damage = 9.0},
    };
    auto first = EnvironmentalTriggerSystem::observe_damage(
        physics, definitions, bodies, first_events, TickIndex{5}, runtimes);
    NINHO_SIM_REQUIRE(first.ok());
    NINHO_SIM_REQUIRE(first.value.empty());
    NINHO_SIM_REQUIRE(runtimes.front().accumulated_damage == 9.0);

    const std::array threshold{DomainEvent{.id = EventId{3}, .tick = TickIndex{6},
        .kind = DomainEventKind::DamageApplied,
        .affected_entity_id = EntityId{20}, .damage = 1.0}};
    auto armed = EnvironmentalTriggerSystem::observe_damage(
        physics, definitions, bodies, threshold, TickIndex{6}, runtimes);
    NINHO_SIM_REQUIRE(armed.ok());
    NINHO_SIM_REQUIRE(armed.value.size() == 1U);
    NINHO_SIM_REQUIRE(runtimes.front().armed);
    NINHO_SIM_REQUIRE(runtimes.front().due_tick == TickIndex{7});
    const ninho::physics::Vec3 expected_origin{3, 4, 5};
    NINHO_SIM_REQUIRE(runtimes.front().captured_origin_m == expected_origin);
    NINHO_SIM_REQUIRE(EnvironmentalTriggerSystem::due_requests(
        definitions, runtimes, TickIndex{6}).empty());
    NINHO_SIM_REQUIRE(EnvironmentalTriggerSystem::due_requests(
        definitions, runtimes, TickIndex{7}).size() == 1U);
}

NINHO_SIM_TEST("environmental trigger rejects ambiguous target and detonates once with canonical cooldown")
{
    ninho::physics::PhysicsWorld physics(
        ninho::physics::make_legacy_radial_world_config({.surface_gravity = 0}));
    const auto first = physics.create_body(ninho::physics::BodyDesc::static_sphere(
        0.5f, {{1, 0, 0}, {}})).value;
    const auto second = physics.create_body(ninho::physics::BodyDesc::static_sphere(
        0.5f, {{2, 0, 0}, {}})).value;
    physics.step();
    const std::array definitions{EnvironmentalTriggerDefinition{.id = 3,
        .target_entity_id = EntityId{20}, .damage_threshold = 5.0,
        .fuse_ticks = 2, .cooldown_ticks = 11,
        .pressure_burst = {4.0, 100.0, 200.0, false, 16}}};
    const std::array ambiguous{
        PressureBurstBody{EntityId{20}, PartId{1}, first, BodyType::Static, 1.0},
        PressureBurstBody{EntityId{20}, PartId{2}, second, BodyType::Static, 1.0},
    };
    auto runtimes = EnvironmentalTriggerSystem::initialize(definitions);
    const std::array damage{DomainEvent{.id = EventId{4}, .tick = TickIndex{10},
        .kind = DomainEventKind::DamageApplied,
        .affected_entity_id = EntityId{20}, .damage = 5.0}};
    const auto rejected = EnvironmentalTriggerSystem::observe_damage(
        physics, definitions, ambiguous, damage, TickIndex{10}, runtimes);
    NINHO_SIM_REQUIRE(!rejected.ok());
    NINHO_SIM_REQUIRE(!runtimes.front().armed);

    const std::array unique{ambiguous.front()};
    NINHO_SIM_REQUIRE(EnvironmentalTriggerSystem::observe_damage(
        physics, definitions, unique, damage, TickIndex{10}, runtimes).ok());
    NINHO_SIM_REQUIRE(runtimes.front().due_tick == TickIndex{12});
    NINHO_SIM_REQUIRE(EnvironmentalTriggerSystem::mark_detonated(
        runtimes, 3, TickIndex{12}, EventId{8}));
    NINHO_SIM_REQUIRE(runtimes.front().detonated);
    NINHO_SIM_REQUIRE(runtimes.front().cooldown_until_tick == TickIndex{23});
    NINHO_SIM_REQUIRE(!EnvironmentalTriggerSystem::mark_detonated(
        runtimes, 3, TickIndex{12}, EventId{9}));
    NINHO_SIM_REQUIRE(EnvironmentalTriggerSystem::due_requests(
        definitions, runtimes, TickIndex{99}).empty());

    const double accumulated = runtimes.front().accumulated_damage;
    const std::array later_damage{DomainEvent{.id = EventId{5},
        .tick = TickIndex{13}, .kind = DomainEventKind::DamageApplied,
        .affected_entity_id = EntityId{20}, .damage = 50.0}};
    const auto ignored = EnvironmentalTriggerSystem::observe_damage(
        physics, definitions, unique, later_damage, TickIndex{13}, runtimes);
    NINHO_SIM_REQUIRE(ignored.ok());
    NINHO_SIM_REQUIRE(ignored.value.empty());
    NINHO_SIM_REQUIRE(runtimes.front().accumulated_damage == accumulated);
}

NINHO_SIM_TEST("environmental trigger orders reversed definitions by trigger id")
{
    ninho::physics::PhysicsWorld physics(
        ninho::physics::make_legacy_radial_world_config({.surface_gravity = 0}));
    const auto handle = physics.create_body(ninho::physics::BodyDesc::static_sphere(
        0.5F, {{1, 2, 3}, {}})).value;
    physics.step();
    const std::array definitions{
        EnvironmentalTriggerDefinition{.id = 9,
            .target_entity_id = EntityId{20}, .damage_threshold = 1.0,
            .fuse_ticks = 1, .pressure_burst = {4, 100, 200, false, 16}},
        EnvironmentalTriggerDefinition{.id = 2,
            .target_entity_id = EntityId{20}, .damage_threshold = 1.0,
            .fuse_ticks = 1, .pressure_burst = {4, 100, 200, false, 16}},
    };
    const std::array bodies{PressureBurstBody{EntityId{20}, PartId{1}, handle,
        BodyType::Static, 100.0}};
    auto runtimes = EnvironmentalTriggerSystem::initialize(definitions);
    const std::array damage{DomainEvent{.id = EventId{11},
        .tick = TickIndex{20}, .kind = DomainEventKind::DamageApplied,
        .affected_entity_id = EntityId{20}, .damage = 1.0}};
    const auto observed = EnvironmentalTriggerSystem::observe_damage(
        physics, definitions, bodies, damage, TickIndex{20}, runtimes);
    NINHO_SIM_REQUIRE(observed.ok());
    NINHO_SIM_REQUIRE(observed.value.size() == 2U);
    NINHO_SIM_REQUIRE(observed.value[0].trigger_id == 2U);
    NINHO_SIM_REQUIRE(observed.value[1].trigger_id == 9U);
    const auto due = EnvironmentalTriggerSystem::due_requests(
        definitions, runtimes, TickIndex{21});
    NINHO_SIM_REQUIRE(due.size() == 2U);
    NINHO_SIM_REQUIRE(due[0].environmental_trigger_id == 2U);
    NINHO_SIM_REQUIRE(due[1].environmental_trigger_id == 9U);
}

}
