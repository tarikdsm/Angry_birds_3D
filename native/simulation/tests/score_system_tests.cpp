#include "test_framework.hpp"

#include "score_system.hpp"
#include "session_test_facade.hpp"
#include "ninho/simulation/session.hpp"

#include <algorithm>
#include <array>
#include <cstdint>
#include <memory>
#include <numbers>
#include <tuple>
#include <utility>
#include <vector>

namespace {

using namespace ninho::simulation;

ScoringDefinition scoring()
{
    return {
        .pig_points = 5000U,
        .unused_bird_points = 10000U,
        .star_thresholds = {1U, 38000U, 50000U},
        .chain_window_ticks = 45U,
        .chain_multiplier_step = 0.10,
        .max_chain_multiplier = 2.0,
    };
}

std::unique_ptr<SimulationSession> integration_session(bool with_scoring)
{
    MaterialCatalog materials{.schema_version = 2U, .source_schema_version = 2U};
    materials.surfaces = {
        {SurfaceId{1}, "bird", 1000.0, 0.4, 0.1},
        {SurfaceId{2}, "pig", 500.0, 0.65, 0.05},
    };
    ArchetypeCatalog archetypes{.schema_version = 2U, .source_schema_version = 2U};
    archetypes.presentation_ids = {"bird", "icon", "animation", "pig"};
    archetypes.score_ids = {"bird_score"};
    archetypes.abilities.push_back({.id = AbilityId{1}, .key = "gravity_field",
        .kind = "gravity_field", .kind_v2 = AbilityKind::GravityField,
        .payload = GravityFieldAbilityDefinition{9U, 75U, 6.0, 1000.0,
            20U, 25.0, 10.0}});
    archetypes.birds.push_back({.id = BirdArchetypeId{1}, .key = "bird",
        .ability_id = AbilityId{1}, .surface_id = SurfaceId{1},
        .mass_kg = 8.0, .radius_m = 0.3, .friction = 0.4,
        .restitution = 0.1, .bullet = true, .projectile_visual_id = "bird",
        .launch_speed_cap_m_s = 45.0, .score_id = "bird_score",
        .icon_id = "icon", .animation_id = "animation"});
    archetypes.weakpoints.push_back({WeakpointId{1}, "uniform",
        {0.0, 1.0, 0.0}, 1.0, 1.0, 1.0});
    archetypes.enemies.push_back({EnemyArchetypeId{1}, "pig", WeakpointId{1},
        SurfaceId{2}, 65.0, 100.0, 18.0, 70.0,
        EnemyDamageModel::TerrestrialPig});

    LevelManifest level;
    level.schema_version = level.source_schema_version = 2U;
    level.id = "score_integration";
    level.world_id = "earth";
    level.world = UniformWorldDefinition{.acceleration_m_s2 = {0.0, -9.81, 0.0},
        .bounds_min_m = {-100.0, -100.0, -100.0},
        .bounds_max_m = {100.0, 100.0, 100.0}};
    level.slingshot = {.asset_id = "launcher", .rest_position_m = {-4.0, 2.0, 0.0},
        .rest_rotation_xyzw = {0.0, 0.0, 0.0, 1.0},
        .spring_constant_n_m = 520.0, .energy_efficiency = 0.9,
        .minimum_extension_m = 0.2, .maximum_extension_m = 4.25,
        .plane_policy = "gravity_vertical_camera_yaw",
        .projectile_clearance_m = 0.0, .speed_ceiling_m_s = 45.0};
    level.bird_queue = {BirdArchetypeId{1}, BirdArchetypeId{1},
        BirdArchetypeId{1}, BirdArchetypeId{1}};
    if (with_scoring) {
        level.scoring = scoring();
    }
    level.settle_policy = {0.001, 0.001, 3U};
    level.watchdog_ticks = 20U;
    BodyDefinition pig;
    pig.body_id = 1U;
    pig.entity_id = EntityId{200};
    pig.part_id = PartId{1};
    pig.body_type = BodyType::Dynamic;
    pig.surface_id = SurfaceId{2};
    pig.enemy_archetype_id = EnemyArchetypeId{1};
    pig.shape = {.type = ShapeType::Sphere, .radius_m = 0.5};
    pig.density_kg_m3 = 65.0
        / (4.0 / 3.0 * std::numbers::pi * 0.5 * 0.5 * 0.5);
    pig.transform.position_m = {0.0, 1.0, 0.0};
    pig.transform.rotation_xyzw = {0.0, 0.0, 0.0, 1.0};
    pig.visual = {.asset_id = "pig", .bounds_m = {1.0, 1.0, 1.0}};
    level.bodies.push_back(pig);
    level.free_body_ids.push_back(1U);
    level.objectives.push_back({1U, ObjectiveKind::NeutralizeEntity, EntityId{200}});

    auto created = SimulationSession::create(materials, archetypes, level);
    if (!created.ok()) {
        ninho::simulation::test::fail(__FILE__, __LINE__,
            created.error.pointer + ": " + created.error.message);
    }
    return std::move(created.value);
}

std::vector<detail::ScoreTransition> consume(detail::ScoreSystem& system,
    TickIndex tick, std::span<const DomainEvent> events,
    std::span<const detail::ScoringCandidate> candidates = {},
    detail::ScoreTerminalState terminal = detail::ScoreTerminalState::None,
    std::uint32_t remaining_birds = 0U)
{
    return system.consume({tick, events, candidates, terminal, remaining_birds});
}

NINHO_SIM_TEST("score system starts a causal chain with the exact integer award")
{
    detail::ScoreSystem system{scoring(), 4U};
    const DomainEvent launch{
        .id = EventId{1}, .tick = TickIndex{1},
        .kind = DomainEventKind::BirdLaunched,
        .entity_id = EntityId{0x80000000U},
    };
    const DomainEvent neutralized{
        .id = EventId{2}, .tick = TickIndex{2},
        .kind = DomainEventKind::EntityNeutralized,
        .entity_id = EntityId{0x80000000U},
        .affected_entity_id = EntityId{200},
    };
    const std::array events{launch, neutralized};
    const std::array candidates{detail::ScoringCandidate{
        .identity = detail::ScoringIdentity::enemy(EntityId{200}),
        .source_event_id = EventId{2},
        .base_points = 5000U,
    }};

    const auto transitions = system.consume({
        .tick = TickIndex{2},
        .events = events,
        .candidates = candidates,
        .terminal = detail::ScoreTerminalState::None,
        .remaining_birds = 3U,
    });

    NINHO_SIM_REQUIRE(system.state().current_score == 5000U);
    NINHO_SIM_REQUIRE(system.state().chain_index == 1U);
    NINHO_SIM_REQUIRE(system.state().multiplier_percent == 100U);
    NINHO_SIM_REQUIRE(system.state().current_chain_root_cause_event_id == EventId{1});
    NINHO_SIM_REQUIRE(transitions.size() == 2U);
    NINHO_SIM_REQUIRE(transitions.back().kind == detail::ScoreTransitionKind::Awarded);
    NINHO_SIM_REQUIRE(transitions.back().awarded_points == 5000U);
}

NINHO_SIM_TEST("score system maps every terrestrial material to its exact base value")
{
    NINHO_SIM_REQUIRE(detail::ScoreSystem::material_points(
        MaterialResponse::Compressible) == 80U);
    NINHO_SIM_REQUIRE(detail::ScoreSystem::material_points(
        MaterialResponse::Fibrous) == 120U);
    NINHO_SIM_REQUIRE(detail::ScoreSystem::material_points(
        MaterialResponse::Brittle) == 160U);
    NINHO_SIM_REQUIRE(detail::ScoreSystem::material_points(
        MaterialResponse::Masonry) == 180U);
    NINHO_SIM_REQUIRE(detail::ScoreSystem::material_points(
        MaterialResponse::Ductile) == 220U);
}

NINHO_SIM_TEST("score system deduplicates sheet transitions and multi body pigs by typed identity")
{
    detail::ScoreSystem system{scoring(), 4U};
    const std::array events{
        DomainEvent{.id = EventId{1}, .tick = TickIndex{1},
            .kind = DomainEventKind::BirdLaunched, .entity_id = EntityId{1000}},
        DomainEvent{.id = EventId{2}, .tick = TickIndex{2},
            .kind = DomainEventKind::MaterialYielded, .entity_id = EntityId{1000}},
        DomainEvent{.id = EventId{3}, .tick = TickIndex{3},
            .kind = DomainEventKind::PieceFractured, .entity_id = EntityId{1000}},
        DomainEvent{.id = EventId{4}, .tick = TickIndex{4},
            .kind = DomainEventKind::EntityNeutralized, .entity_id = EntityId{1000}},
        DomainEvent{.id = EventId{5}, .tick = TickIndex{5},
            .kind = DomainEventKind::EntityNeutralized, .entity_id = EntityId{1000}},
    };
    const auto sheet = detail::ScoringIdentity::material(EntityId{20}, PartId{1});
    const auto pig = detail::ScoringIdentity::enemy(EntityId{200});
    const std::array candidates{
        detail::ScoringCandidate{sheet, EventId{2}, 220U},
        detail::ScoringCandidate{sheet, EventId{3}, 220U},
        detail::ScoringCandidate{pig, EventId{4}, 5000U},
        detail::ScoringCandidate{pig, EventId{5}, 5000U},
    };

    const auto transitions = consume(system, TickIndex{5}, events, candidates);
    NINHO_SIM_REQUIRE(std::ranges::count_if(transitions, [](const auto& value) {
        return value.kind == detail::ScoreTransitionKind::Awarded;
    }) == 2);
    NINHO_SIM_REQUIRE(system.state().scored_identities.size() == 2U);
    NINHO_SIM_REQUIRE(system.state().current_score == 220U + 5500U);
}

NINHO_SIM_TEST("score system continues at 45 ticks and restarts at 46 or a different root")
{
    detail::ScoreSystem system{scoring(), 4U};
    const DomainEvent launch{.id = EventId{1}, .tick = TickIndex{1},
        .kind = DomainEventKind::BirdLaunched, .entity_id = EntityId{1000}};
    consume(system, TickIndex{1}, std::span{&launch, 1U});
    const auto award_at = [&](std::uint64_t event_id, std::uint64_t tick,
                              std::uint32_t entity, EventId cause = {}) {
        const DomainEvent source{.id = EventId{event_id}, .tick = TickIndex{tick},
            .kind = DomainEventKind::PieceFractured,
            .entity_id = cause == EventId{} ? EntityId{1000} : EntityId{},
            .affected_entity_id = EntityId{entity}, .cause_event_id = cause};
        const detail::ScoringCandidate candidate{
            detail::ScoringIdentity::material(EntityId{entity}, PartId{1}),
            source.id, 100U};
        consume(system, TickIndex{tick}, std::span{&source, 1U},
            std::span{&candidate, 1U});
    };
    award_at(2, 2, 10);
    award_at(3, 47, 11);
    NINHO_SIM_REQUIRE(system.state().chain_index == 2U);
    NINHO_SIM_REQUIRE(system.state().multiplier_percent == 110U);
    award_at(4, 93, 12);
    NINHO_SIM_REQUIRE(system.state().chain_index == 1U);
    NINHO_SIM_REQUIRE(system.state().multiplier_percent == 100U);

    const DomainEvent other_launch{.id = EventId{5}, .tick = TickIndex{94},
        .kind = DomainEventKind::BirdLaunched, .entity_id = EntityId{1001}};
    consume(system, TickIndex{94}, std::span{&other_launch, 1U});
    NINHO_SIM_REQUIRE(system.state().chain_index == 0U);
    const DomainEvent other_score{.id = EventId{6}, .tick = TickIndex{94},
        .kind = DomainEventKind::EntityNeutralized, .entity_id = EntityId{1001},
        .affected_entity_id = EntityId{201}};
    const detail::ScoringCandidate other_candidate{
        detail::ScoringIdentity::enemy(EntityId{201}), EventId{6}, 5000U};
    consume(system, TickIndex{94}, std::span{&other_score, 1U},
        std::span{&other_candidate, 1U});
    NINHO_SIM_REQUIRE(system.state().chain_index == 1U);
    NINHO_SIM_REQUIRE(system.state().current_chain_root_cause_event_id == EventId{5});
}

NINHO_SIM_TEST("score system resolves deep causal paths and isolates zero or foreign causes")
{
    detail::ScoreSystem system{scoring(), 4U};
    const std::array causal{
        DomainEvent{.id = EventId{1}, .tick = TickIndex{1},
            .kind = DomainEventKind::BirdLaunched, .entity_id = EntityId{1000}},
        DomainEvent{.id = EventId{2}, .tick = TickIndex{2},
            .kind = DomainEventKind::PressureBurst, .cause_event_id = EventId{1}},
        DomainEvent{.id = EventId{3}, .tick = TickIndex{3},
            .kind = DomainEventKind::EnvironmentalTriggerDetonated,
            .cause_event_id = EventId{2}},
        DomainEvent{.id = EventId{4}, .tick = TickIndex{4},
            .kind = DomainEventKind::CrushDamageApplied,
            .cause_event_id = EventId{3}},
        DomainEvent{.id = EventId{5}, .tick = TickIndex{5},
            .kind = DomainEventKind::EntityNeutralized,
            .affected_entity_id = EntityId{200}, .cause_event_id = EventId{4}},
    };
    const detail::ScoringCandidate pig{
        detail::ScoringIdentity::enemy(EntityId{200}), EventId{5}, 5000U};
    consume(system, TickIndex{5}, causal, std::span{&pig, 1U});
    NINHO_SIM_REQUIRE(system.state().current_chain_root_cause_event_id == EventId{1});

    const DomainEvent foreign{.id = EventId{6}, .tick = TickIndex{6},
        .kind = DomainEventKind::PieceFractured,
        .entity_id = EntityId{1000}, .cause_event_id = EventId{999}};
    const detail::ScoringCandidate foreign_candidate{
        detail::ScoringIdentity::material(EntityId{20}, PartId{1}), EventId{6}, 120U};
    consume(system, TickIndex{6}, std::span{&foreign, 1U},
        std::span{&foreign_candidate, 1U});
    NINHO_SIM_REQUIRE(system.state().chain_index == 1U);
    NINHO_SIM_REQUIRE(system.state().current_chain_root_cause_event_id == EventId{});

    const DomainEvent zero{.id = EventId{7}, .tick = TickIndex{7},
        .kind = DomainEventKind::PieceFractured,
        .affected_entity_id = EntityId{21}};
    const detail::ScoringCandidate zero_candidate{
        detail::ScoringIdentity::material(EntityId{21}, PartId{1}), EventId{7}, 120U};
    consume(system, TickIndex{7}, std::span{&zero, 1U},
        std::span{&zero_candidate, 1U});
    NINHO_SIM_REQUIRE(system.state().chain_index == 1U);
}

NINHO_SIM_TEST("score system attributes Blue child contact to the original launch root")
{
    detail::ScoreSystem system{scoring(), 4U};
    const std::array events{
        DomainEvent{.id = EventId{1}, .tick = TickIndex{1},
            .kind = DomainEventKind::BirdLaunched,
            .entity_id = EntityId{1000}, .shot_id = 9U},
        DomainEvent{.id = EventId{2}, .tick = TickIndex{2},
            .kind = DomainEventKind::ProjectileSplit,
            .entity_id = EntityId{1000}},
        DomainEvent{.id = EventId{3}, .tick = TickIndex{2},
            .kind = DomainEventKind::ProjectileSpawned,
            .entity_id = EntityId{1000}, .affected_entity_id = EntityId{1001},
            .cause_event_id = EventId{2}},
        DomainEvent{.id = EventId{4}, .tick = TickIndex{3},
            .kind = DomainEventKind::EntityNeutralized,
            .entity_id = EntityId{1001}, .affected_entity_id = EntityId{200}},
    };
    const detail::ScoringCandidate candidate{
        detail::ScoringIdentity::enemy(EntityId{200}), EventId{4}, 5000U};
    const auto transitions = consume(system, TickIndex{3}, events,
        std::span{&candidate, 1U});
    const auto award = std::ranges::find(transitions,
        detail::ScoreTransitionKind::Awarded, &detail::ScoreTransition::kind);
    NINHO_SIM_REQUIRE(award != transitions.end());
    NINHO_SIM_REQUIRE(award->root_cause_event_id == EventId{1});
    NINHO_SIM_REQUIRE(award->shot_id == 9U);
}

NINHO_SIM_TEST("score system carries provenance from damaged material into later crush")
{
    detail::ScoreSystem system{scoring(), 4U};
    const std::array events{
        DomainEvent{.id = EventId{1}, .tick = TickIndex{1},
            .kind = DomainEventKind::BirdLaunched,
            .entity_id = EntityId{1000}, .shot_id = 3U},
        DomainEvent{.id = EventId{2}, .tick = TickIndex{2},
            .kind = DomainEventKind::DamageApplied,
            .entity_id = EntityId{1000}, .affected_entity_id = EntityId{20},
            .affected_part_id = PartId{1}},
        DomainEvent{.id = EventId{3}, .tick = TickIndex{20},
            .kind = DomainEventKind::CrushDamageApplied,
            .entity_id = EntityId{20}, .affected_entity_id = EntityId{200}},
        DomainEvent{.id = EventId{4}, .tick = TickIndex{20},
            .kind = DomainEventKind::EntityNeutralized,
            .affected_entity_id = EntityId{200}, .cause_event_id = EventId{3}},
    };
    const detail::ScoringCandidate candidate{
        detail::ScoringIdentity::enemy(EntityId{200}), EventId{4}, 5000U};
    const auto transitions = consume(system, TickIndex{20}, events,
        std::span{&candidate, 1U});
    const auto award = std::ranges::find(transitions,
        detail::ScoreTransitionKind::Awarded, &detail::ScoreTransition::kind);
    NINHO_SIM_REQUIRE(award != transitions.end());
    NINHO_SIM_REQUIRE(award->root_cause_event_id == EventId{1});
    NINHO_SIM_REQUIRE(award->shot_id == 3U);
}

NINHO_SIM_TEST("score system propagates rooted physical contact through both dynamic endpoints")
{
    constexpr EntityId projectile{1000U};
    constexpr EntityId first{20U};
    constexpr EntityId relay{21U};
    constexpr EntityId target{22U};
    constexpr EntityId other{23U};
    constexpr EntityId static_support{30U};
    constexpr std::array dynamic_entities{
        first, relay, target, other, projectile};

    detail::ScoreSystem system{scoring(), 4U};
    const std::array seed{
        DomainEvent{.id = EventId{1}, .tick = TickIndex{1},
            .kind = DomainEventKind::BirdLaunched,
            .entity_id = projectile, .shot_id = 6U},
        DomainEvent{.id = EventId{2}, .tick = TickIndex{2},
            .kind = DomainEventKind::DamageApplied,
            .entity_id = projectile, .affected_entity_id = first},
    };
    static_cast<void>(system.consume({.tick = TickIndex{2}, .events = seed,
        .dynamic_entities = dynamic_entities}));

    const DomainEvent reverse_contact{.id = EventId{3}, .tick = TickIndex{3},
        .kind = DomainEventKind::DamageApplied,
        .entity_id = relay, .affected_entity_id = first};
    static_cast<void>(system.consume({.tick = TickIndex{3},
        .events = std::span{&reverse_contact, 1U},
        .dynamic_entities = dynamic_entities}));
    NINHO_SIM_REQUIRE(system.has_root_for(relay));

    const std::array scored_events{
        DomainEvent{.id = EventId{4}, .tick = TickIndex{4},
            .kind = DomainEventKind::DamageApplied,
            .entity_id = relay, .affected_entity_id = target},
        DomainEvent{.id = EventId{5}, .tick = TickIndex{4},
            .kind = DomainEventKind::PieceFractured,
            .entity_id = target, .affected_entity_id = target},
        DomainEvent{.id = EventId{6}, .tick = TickIndex{5},
            .kind = DomainEventKind::DamageApplied,
            .entity_id = relay, .affected_entity_id = other},
        DomainEvent{.id = EventId{7}, .tick = TickIndex{5},
            .kind = DomainEventKind::PieceFractured,
            .entity_id = other, .affected_entity_id = other},
    };
    const std::array candidates{
        detail::ScoringCandidate{detail::ScoringIdentity::material(
            target, PartId{1U}), EventId{5}, 100U},
        detail::ScoringCandidate{detail::ScoringIdentity::material(
            other, PartId{1U}), EventId{7}, 100U},
    };
    const auto transitions = system.consume({.tick = TickIndex{5},
        .events = scored_events, .candidates = candidates,
        .dynamic_entities = dynamic_entities});
    std::vector<detail::ScoreTransition> awards;
    std::ranges::copy_if(transitions, std::back_inserter(awards),
        [](const auto& value) {
            return value.kind == detail::ScoreTransitionKind::Awarded;
        });
    NINHO_SIM_REQUIRE(awards.size() == 2U);
    NINHO_SIM_REQUIRE(awards[0].root_cause_event_id == EventId{1});
    NINHO_SIM_REQUIRE(awards[1].root_cause_event_id == EventId{1});
    NINHO_SIM_REQUIRE(awards[0].multiplier_percent == 100U);
    NINHO_SIM_REQUIRE(awards[1].multiplier_percent == 110U);
    NINHO_SIM_REQUIRE(system.state().current_score == 210U);

    const DomainEvent static_reverse{.id = EventId{8}, .tick = TickIndex{6},
        .kind = DomainEventKind::DamageApplied,
        .entity_id = static_support, .affected_entity_id = first};
    static_cast<void>(system.consume({.tick = TickIndex{6},
        .events = std::span{&static_reverse, 1U},
        .dynamic_entities = dynamic_entities}));
    NINHO_SIM_REQUIRE(!system.has_root_for(static_support));

    detail::ScoreSystem rootless{scoring(), 4U};
    static_cast<void>(rootless.consume({.tick = TickIndex{3},
        .events = std::span{&reverse_contact, 1U},
        .dynamic_entities = dynamic_entities}));
    NINHO_SIM_REQUIRE(!rootless.has_root_for(relay));
}

NINHO_SIM_TEST("score system propagates one launch root across a contact component without damage")
{
    constexpr EntityId source{1000U};
    constexpr EntityId relay{20U};
    constexpr EntityId target{21U};
    constexpr std::array dynamic_entities{relay, target, source};
    constexpr std::array contact_entities{
        std::pair{source, relay}, std::pair{relay, target}};
    const std::array events{
        DomainEvent{.id = EventId{1}, .tick = TickIndex{1},
            .kind = DomainEventKind::BirdLaunched,
            .entity_id = source, .shot_id = 7U},
        DomainEvent{.id = EventId{2}, .tick = TickIndex{1},
            .kind = DomainEventKind::PieceFractured,
            .affected_entity_id = target, .affected_part_id = PartId{1U}},
    };
    const std::array candidates{detail::ScoringCandidate{
        detail::ScoringIdentity::material(target, PartId{1U}), EventId{2}, 160U}};

    detail::ScoreSystem system{scoring(), 4U};
    const auto transitions = system.consume({
        .tick = TickIndex{1},
        .events = events,
        .candidates = candidates,
        .dynamic_entities = dynamic_entities,
        .dynamic_physical_edges = contact_entities,
    });
    const auto award = std::ranges::find(
        transitions, detail::ScoreTransitionKind::Awarded,
        &detail::ScoreTransition::kind);
    NINHO_SIM_REQUIRE(award != transitions.end());
    NINHO_SIM_REQUIRE(award->root_cause_event_id == EventId{1});
    NINHO_SIM_REQUIRE(award->shot_id == 7U);
    NINHO_SIM_REQUIRE(system.has_root_for(relay));
    NINHO_SIM_REQUIRE(system.has_root_for(target));
}

NINHO_SIM_TEST("score system contact component propagation is pair order independent")
{
    constexpr EntityId source{1000U};
    constexpr EntityId first{20U};
    constexpr EntityId second{21U};
    constexpr EntityId target{22U};
    constexpr std::array dynamic_entities{first, second, target, source};
    const auto run = [&](std::span<const std::pair<EntityId, EntityId>> contacts) {
        detail::ScoreSystem system{scoring(), 4U};
        const std::array events{
            DomainEvent{.id = EventId{1}, .tick = TickIndex{1},
                .kind = DomainEventKind::BirdLaunched,
                .entity_id = source, .shot_id = 9U},
            DomainEvent{.id = EventId{2}, .tick = TickIndex{1},
                .kind = DomainEventKind::PieceFractured,
                .affected_entity_id = target, .affected_part_id = PartId{1U}},
        };
        const std::array candidates{detail::ScoringCandidate{
            detail::ScoringIdentity::material(target, PartId{1U}),
            EventId{2}, 160U}};
        const auto transitions = system.consume({
            .tick = TickIndex{1},
            .events = events,
            .candidates = candidates,
            .dynamic_entities = dynamic_entities,
            .dynamic_physical_edges = contacts,
        });
        const auto award = std::ranges::find(
            transitions, detail::ScoreTransitionKind::Awarded,
            &detail::ScoreTransition::kind);
        NINHO_SIM_REQUIRE(award != transitions.end());
        return std::tuple{award->root_cause_event_id, award->shot_id,
            award->awarded_points,
            std::vector<detail::ScoreSystem::EntityRoot>{
                system.entity_roots().begin(), system.entity_roots().end()}};
    };
    const std::array canonical{
        std::pair{source, first}, std::pair{first, second},
        std::pair{second, target}};
    const std::array permuted{
        std::pair{target, second}, std::pair{second, first},
        std::pair{first, source}, std::pair{second, target}};
    NINHO_SIM_REQUIRE(run(canonical) == run(permuted));
}

NINHO_SIM_TEST("score system leaves a contact component rootless on launch conflict")
{
    constexpr EntityId first_source{1000U};
    constexpr EntityId relay{20U};
    constexpr EntityId second_source{1001U};
    constexpr std::array dynamic_entities{relay, first_source, second_source};
    constexpr std::array contact_entities{
        std::pair{first_source, relay}, std::pair{relay, second_source}};
    const std::array events{
        DomainEvent{.id = EventId{1}, .tick = TickIndex{1},
            .kind = DomainEventKind::BirdLaunched,
            .entity_id = first_source, .shot_id = 1U},
        DomainEvent{.id = EventId{2}, .tick = TickIndex{1},
            .kind = DomainEventKind::BirdLaunched,
            .entity_id = second_source, .shot_id = 2U},
        DomainEvent{.id = EventId{3}, .tick = TickIndex{1},
            .kind = DomainEventKind::PieceFractured,
            .affected_entity_id = relay, .affected_part_id = PartId{1U}},
    };
    const std::array candidates{detail::ScoringCandidate{
        detail::ScoringIdentity::material(relay, PartId{1U}), EventId{3}, 160U}};

    detail::ScoreSystem system{scoring(), 4U};
    const auto transitions = system.consume({
        .tick = TickIndex{1},
        .events = events,
        .candidates = candidates,
        .dynamic_entities = dynamic_entities,
        .dynamic_physical_edges = contact_entities,
    });
    const auto award = std::ranges::find(
        transitions, detail::ScoreTransitionKind::Awarded,
        &detail::ScoreTransition::kind);
    NINHO_SIM_REQUIRE(award != transitions.end());
    NINHO_SIM_REQUIRE(award->root_cause_event_id == EventId{});
    NINHO_SIM_REQUIRE(award->shot_id == 0U);
    NINHO_SIM_REQUIRE(!system.has_root_for(relay));
}

NINHO_SIM_TEST("score system preserves reciprocal launch roots and leaves the conflicted relay rootless")
{
    constexpr EntityId relay{20U};
    constexpr EntityId first_projectile{1000U};
    constexpr EntityId second_projectile{1001U};
    constexpr std::array dynamic_entities{
        relay, first_projectile, second_projectile};

    struct Outcome {
        std::pair<EventId, std::uint64_t> first_root;
        std::pair<EventId, std::uint64_t> second_root;
        std::pair<EventId, std::uint64_t> relay_root;
        std::pair<EventId, std::uint64_t> relay_award;

        bool operator==(const Outcome&) const = default;
    };

    const auto run = [&](bool reverse_damage_ids) {
        detail::ScoreSystem system{scoring(), 4U};
        const DomainEvent first_damage{
            .id = EventId{reverse_damage_ids ? 4U : 3U},
            .tick = TickIndex{1},
            .kind = DomainEventKind::DamageApplied,
            .entity_id = first_projectile,
            .affected_entity_id = second_projectile,
        };
        const DomainEvent second_damage{
            .id = EventId{reverse_damage_ids ? 3U : 4U},
            .tick = TickIndex{1},
            .kind = DomainEventKind::DamageApplied,
            .entity_id = second_projectile,
            .affected_entity_id = first_projectile,
        };
        const std::array events{
            DomainEvent{.id = EventId{1}, .tick = TickIndex{1},
                .kind = DomainEventKind::BirdLaunched,
                .entity_id = first_projectile, .shot_id = 11U},
            DomainEvent{.id = EventId{2}, .tick = TickIndex{1},
                .kind = DomainEventKind::BirdLaunched,
                .entity_id = second_projectile, .shot_id = 22U},
            first_damage,
            second_damage,
            DomainEvent{.id = EventId{5}, .tick = TickIndex{1},
                .kind = DomainEventKind::PieceFractured,
                .affected_entity_id = relay, .affected_part_id = PartId{1U}},
        };
        const std::array physical_edges{
            std::pair{first_projectile, relay},
            std::pair{relay, second_projectile},
        };
        const detail::ScoringCandidate candidate{
            detail::ScoringIdentity::material(relay, PartId{1U}),
            EventId{5}, 160U};
        const auto transitions = system.consume({
            .tick = TickIndex{1},
            .events = events,
            .candidates = std::span{&candidate, 1U},
            .dynamic_entities = dynamic_entities,
            .dynamic_physical_edges = physical_edges,
        });

        const auto root_of = [&](EntityId entity) {
            const auto found = std::ranges::find(system.entity_roots(), entity,
                &detail::ScoreSystem::EntityRoot::entity_id);
            return found == system.entity_roots().end()
                ? std::pair{EventId{}, std::uint64_t{0}}
                : std::pair{found->root_event_id, found->shot_id};
        };
        const auto award = std::ranges::find(transitions,
            detail::ScoreTransitionKind::Awarded,
            &detail::ScoreTransition::kind);
        NINHO_SIM_REQUIRE(award != transitions.end());
        return Outcome{
            .first_root = root_of(first_projectile),
            .second_root = root_of(second_projectile),
            .relay_root = root_of(relay),
            .relay_award = {award->root_cause_event_id, award->shot_id},
        };
    };

    const Outcome expected{
        .first_root = {EventId{1}, 11U},
        .second_root = {EventId{2}, 22U},
        .relay_root = {EventId{}, 0U},
        .relay_award = {EventId{}, 0U},
    };
    const Outcome forward = run(false);
    const Outcome reverse = run(true);
    NINHO_SIM_REQUIRE(forward == expected);
    NINHO_SIM_REQUIRE(reverse == expected);
    NINHO_SIM_REQUIRE(forward == reverse);
}

NINHO_SIM_TEST("score system one way damage supersedes its target without contaminating an isolated relay")
{
    constexpr EntityId relay{20U};
    constexpr EntityId first_projectile{1000U};
    constexpr EntityId second_projectile{1001U};
    constexpr std::array dynamic_entities{
        relay, first_projectile, second_projectile};
    const std::array events{
        DomainEvent{.id = EventId{1}, .tick = TickIndex{1},
            .kind = DomainEventKind::BirdLaunched,
            .entity_id = first_projectile, .shot_id = 11U},
        DomainEvent{.id = EventId{2}, .tick = TickIndex{1},
            .kind = DomainEventKind::BirdLaunched,
            .entity_id = second_projectile, .shot_id = 22U},
        DomainEvent{.id = EventId{3}, .tick = TickIndex{1},
            .kind = DomainEventKind::DamageApplied,
            .entity_id = first_projectile,
            .affected_entity_id = second_projectile},
    };
    const std::array physical_edges{
        std::pair{first_projectile, second_projectile},
    };

    detail::ScoreSystem system{scoring(), 4U};
    static_cast<void>(system.consume({
        .tick = TickIndex{1},
        .events = events,
        .dynamic_entities = dynamic_entities,
        .dynamic_physical_edges = physical_edges,
    }));

    const auto root_of = [&](EntityId entity) {
        const auto found = std::ranges::find(system.entity_roots(), entity,
            &detail::ScoreSystem::EntityRoot::entity_id);
        return found == system.entity_roots().end()
            ? std::pair{EventId{}, std::uint64_t{0}}
            : std::pair{found->root_event_id, found->shot_id};
    };
    const std::pair first_expected{EventId{1}, std::uint64_t{11}};
    const std::pair second_expected{EventId{1}, std::uint64_t{11}};
    const std::pair relay_expected{EventId{}, std::uint64_t{0}};
    NINHO_SIM_REQUIRE(root_of(first_projectile) == first_expected);
    NINHO_SIM_REQUIRE(root_of(second_projectile) == second_expected);
    NINHO_SIM_REQUIRE(root_of(relay) == relay_expected);
}

NINHO_SIM_TEST("score system contact components exclude static endpoints and preserve empty input")
{
    constexpr EntityId source{1000U};
    constexpr EntityId static_target{30U};
    constexpr EntityId isolated_target{31U};
    constexpr std::array dynamic_entities{isolated_target, source};
    constexpr std::array static_contact{
        std::pair{source, static_target}};
    const std::pair rootless{EventId{}, std::uint64_t{0}};
    const auto run = [&](EntityId target,
        std::span<const std::pair<EntityId, EntityId>> contacts) {
        detail::ScoreSystem system{scoring(), 4U};
        const std::array events{
            DomainEvent{.id = EventId{1}, .tick = TickIndex{1},
                .kind = DomainEventKind::BirdLaunched,
                .entity_id = source, .shot_id = 4U},
            DomainEvent{.id = EventId{2}, .tick = TickIndex{1},
                .kind = DomainEventKind::PieceFractured,
                .affected_entity_id = target, .affected_part_id = PartId{1U}},
        };
        const std::array candidates{detail::ScoringCandidate{
            detail::ScoringIdentity::material(target, PartId{1U}),
            EventId{2}, 160U}};
        const auto transitions = system.consume({
            .tick = TickIndex{1},
            .events = events,
            .candidates = candidates,
            .dynamic_entities = dynamic_entities,
            .dynamic_physical_edges = contacts,
        });
        const auto award = std::ranges::find(
            transitions, detail::ScoreTransitionKind::Awarded,
            &detail::ScoreTransition::kind);
        NINHO_SIM_REQUIRE(award != transitions.end());
        return std::pair{award->root_cause_event_id, award->shot_id};
    };
    NINHO_SIM_REQUIRE(run(static_target, static_contact) == rootless);
    NINHO_SIM_REQUIRE(run(isolated_target, {}) == rootless);
}

NINHO_SIM_TEST("score system propagates a launch root through an active joint and a contact")
{
    constexpr EntityId projectile{1000U};
    constexpr EntityId glass{20U};
    constexpr EntityId beam{21U};
    constexpr EntityId latch{22U};
    constexpr std::array dynamic_entities{glass, beam, latch, projectile};

    detail::ScoreSystem system{scoring(), 4U};
    const std::array seed{
        DomainEvent{.id = EventId{1}, .tick = TickIndex{1},
            .kind = DomainEventKind::BirdLaunched,
            .entity_id = projectile, .shot_id = 8U},
        DomainEvent{.id = EventId{2}, .tick = TickIndex{1},
            .kind = DomainEventKind::DamageApplied,
            .entity_id = projectile, .affected_entity_id = glass},
    };
    static_cast<void>(system.consume({
        .tick = TickIndex{1},
        .events = seed,
        .dynamic_entities = dynamic_entities,
    }));

    // The first edge represents the active glass-beam joint. The second is
    // the beam-latch contact observed by the physics step.
    constexpr std::array physical_edges{
        std::pair{glass, beam}, std::pair{beam, latch}};
    const std::array events{DomainEvent{
        .id = EventId{3}, .tick = TickIndex{2},
        .kind = DomainEventKind::PieceFractured,
        .affected_entity_id = latch, .affected_part_id = PartId{1U}}};
    const std::array candidates{detail::ScoringCandidate{
        detail::ScoringIdentity::material(latch, PartId{1U}), EventId{3}, 160U}};
    const auto transitions = system.consume({
        .tick = TickIndex{2},
        .events = events,
        .candidates = candidates,
        .dynamic_entities = dynamic_entities,
        .dynamic_physical_edges = physical_edges,
    });

    const auto award = std::ranges::find(
        transitions, detail::ScoreTransitionKind::Awarded,
        &detail::ScoreTransition::kind);
    NINHO_SIM_REQUIRE(award != transitions.end());
    NINHO_SIM_REQUIRE(award->root_cause_event_id == EventId{1});
    NINHO_SIM_REQUIRE(award->shot_id == 8U);
    NINHO_SIM_REQUIRE(system.has_root_for(beam));
    NINHO_SIM_REQUIRE(system.has_root_for(latch));
}

NINHO_SIM_TEST("score system excludes inactive joints and static joint endpoints")
{
    constexpr EntityId projectile{1000U};
    constexpr EntityId glass{20U};
    constexpr EntityId beam{21U};
    constexpr EntityId latch{22U};
    constexpr EntityId static_support{30U};
    constexpr std::array dynamic_entities{glass, beam, latch, projectile};
    const std::pair rootless{EventId{}, std::uint64_t{0}};

    const auto run = [&](std::span<const std::pair<EntityId, EntityId>> edges) {
        detail::ScoreSystem system{scoring(), 4U};
        const std::array seed{
            DomainEvent{.id = EventId{1}, .tick = TickIndex{1},
                .kind = DomainEventKind::BirdLaunched,
                .entity_id = projectile, .shot_id = 8U},
            DomainEvent{.id = EventId{2}, .tick = TickIndex{1},
                .kind = DomainEventKind::DamageApplied,
                .entity_id = projectile, .affected_entity_id = glass},
        };
        static_cast<void>(system.consume({
            .tick = TickIndex{1},
            .events = seed,
            .dynamic_entities = dynamic_entities,
        }));
        const std::array events{DomainEvent{
            .id = EventId{3}, .tick = TickIndex{2},
            .kind = DomainEventKind::PieceFractured,
            .affected_entity_id = latch, .affected_part_id = PartId{1U}}};
        const std::array candidates{detail::ScoringCandidate{
            detail::ScoringIdentity::material(latch, PartId{1U}),
            EventId{3}, 160U}};
        const auto transitions = system.consume({
            .tick = TickIndex{2},
            .events = events,
            .candidates = candidates,
            .dynamic_entities = dynamic_entities,
            .dynamic_physical_edges = edges,
        });
        const auto award = std::ranges::find(
            transitions, detail::ScoreTransitionKind::Awarded,
            &detail::ScoreTransition::kind);
        NINHO_SIM_REQUIRE(award != transitions.end());
        return std::pair{award->root_cause_event_id, award->shot_id};
    };

    // An inactive joint contributes no edge, leaving only the rootless
    // beam-latch contact component.
    constexpr std::array inactive_joint_edges{std::pair{beam, latch}};
    NINHO_SIM_REQUIRE(run(inactive_joint_edges) == rootless);

    // Even if a malformed collector supplies a static endpoint, the dynamic
    // entity filter must prevent it from bridging into the contact component.
    constexpr std::array static_joint_edges{
        std::pair{glass, static_support}, std::pair{static_support, beam},
        std::pair{beam, latch}};
    NINHO_SIM_REQUIRE(run(static_joint_edges) == rootless);
}

NINHO_SIM_TEST("score system propagates a rooted overload through historical joint endpoints")
{
    constexpr EntityId projectile{1000U};
    constexpr EntityId glass{20U};
    constexpr EntityId beam{21U};
    constexpr EntityId latch{22U};
    constexpr std::array dynamic_entities{glass, beam, latch, projectile};
    constexpr std::array joint_endpoints{
        detail::JointEntityEndpoints{JointId{27U}, glass, beam}};
    constexpr std::array contact_edges{std::pair{beam, latch}};
    const std::array events{
        DomainEvent{.id = EventId{1}, .tick = TickIndex{1},
            .kind = DomainEventKind::BirdLaunched,
            .entity_id = projectile, .shot_id = 8U},
        DomainEvent{.id = EventId{2}, .tick = TickIndex{1},
            .kind = DomainEventKind::JointOverloaded,
            .cause_event_id = EventId{1}, .joint_id = JointId{27U}},
        DomainEvent{.id = EventId{3}, .tick = TickIndex{1},
            .kind = DomainEventKind::PieceFractured,
            .affected_entity_id = latch, .affected_part_id = PartId{1U}},
    };
    const std::array candidates{detail::ScoringCandidate{
        detail::ScoringIdentity::material(latch, PartId{1U}), EventId{3}, 160U}};

    detail::ScoreSystem system{scoring(), 4U};
    const auto transitions = system.consume({
        .tick = TickIndex{1},
        .events = events,
        .candidates = candidates,
        .dynamic_entities = dynamic_entities,
        .dynamic_physical_edges = contact_edges,
        .joint_entity_endpoints = joint_endpoints,
    });
    const auto award = std::ranges::find(
        transitions, detail::ScoreTransitionKind::Awarded,
        &detail::ScoreTransition::kind);
    NINHO_SIM_REQUIRE(award != transitions.end());
    NINHO_SIM_REQUIRE(award->root_cause_event_id == EventId{1});
    NINHO_SIM_REQUIRE(award->shot_id == 8U);
    NINHO_SIM_REQUIRE(system.has_root_for(glass));
    NINHO_SIM_REQUIRE(system.has_root_for(beam));
    NINHO_SIM_REQUIRE(system.has_root_for(latch));
}

NINHO_SIM_TEST("score system uses broken joint history without overwriting a conflicting root")
{
    constexpr EntityId first_projectile{1000U};
    constexpr EntityId second_projectile{1001U};
    constexpr EntityId first_endpoint{20U};
    constexpr EntityId conflicting_endpoint{21U};
    constexpr std::array dynamic_entities{
        first_endpoint, conflicting_endpoint, first_projectile, second_projectile};
    constexpr std::array joint_endpoints{detail::JointEntityEndpoints{
        JointId{35U}, first_endpoint, conflicting_endpoint}};
    const std::array seed{
        DomainEvent{.id = EventId{1}, .tick = TickIndex{1},
            .kind = DomainEventKind::BirdLaunched,
            .entity_id = first_projectile, .shot_id = 1U},
        DomainEvent{.id = EventId{2}, .tick = TickIndex{1},
            .kind = DomainEventKind::BirdLaunched,
            .entity_id = second_projectile, .shot_id = 2U},
        DomainEvent{.id = EventId{3}, .tick = TickIndex{1},
            .kind = DomainEventKind::DamageApplied,
            .entity_id = second_projectile,
            .affected_entity_id = conflicting_endpoint},
    };
    detail::ScoreSystem system{scoring(), 4U};
    static_cast<void>(system.consume({
        .tick = TickIndex{1},
        .events = seed,
        .dynamic_entities = dynamic_entities,
    }));

    const DomainEvent broken{
        .id = EventId{4}, .tick = TickIndex{2},
        .kind = DomainEventKind::JointBroken,
        .cause_event_id = EventId{1}, .joint_id = JointId{35U}};
    static_cast<void>(system.consume({
        .tick = TickIndex{2},
        .events = std::span{&broken, 1U},
        .dynamic_entities = dynamic_entities,
        .joint_entity_endpoints = joint_endpoints,
    }));

    const auto root_of = [&](EntityId entity) {
        const auto found = std::ranges::find(
            system.entity_roots(), entity,
            &detail::ScoreSystem::EntityRoot::entity_id);
        NINHO_SIM_REQUIRE(found != system.entity_roots().end());
        return std::pair{found->root_event_id, found->shot_id};
    };
    const std::pair first_root{EventId{1}, std::uint64_t{1}};
    const std::pair second_root{EventId{2}, std::uint64_t{2}};
    NINHO_SIM_REQUIRE(root_of(first_endpoint) == first_root);
    NINHO_SIM_REQUIRE(root_of(conflicting_endpoint) == second_root);
}

NINHO_SIM_TEST("score system ignores rootless unknown and static joint endpoints")
{
    constexpr EntityId projectile{1000U};
    constexpr EntityId dynamic_endpoint{20U};
    constexpr EntityId rootless_first{21U};
    constexpr EntityId rootless_second{22U};
    constexpr EntityId unknown_target{23U};
    constexpr EntityId static_endpoint{30U};
    constexpr std::array dynamic_entities{dynamic_endpoint, rootless_first,
        rootless_second, unknown_target, projectile};
    constexpr std::array joint_endpoints{
        detail::JointEntityEndpoints{
            JointId{27U}, dynamic_endpoint, static_endpoint},
        detail::JointEntityEndpoints{
            JointId{28U}, rootless_first, rootless_second},
    };
    const std::array events{
        DomainEvent{.id = EventId{1}, .tick = TickIndex{1},
            .kind = DomainEventKind::BirdLaunched,
            .entity_id = projectile, .shot_id = 3U},
        DomainEvent{.id = EventId{2}, .tick = TickIndex{1},
            .kind = DomainEventKind::JointOverloaded,
            .cause_event_id = EventId{1}, .joint_id = JointId{27U}},
        DomainEvent{.id = EventId{3}, .tick = TickIndex{1},
            .kind = DomainEventKind::JointBroken,
            .joint_id = JointId{28U}},
        DomainEvent{.id = EventId{4}, .tick = TickIndex{1},
            .kind = DomainEventKind::JointBroken,
            .affected_entity_id = unknown_target,
            .cause_event_id = EventId{1}, .joint_id = JointId{999U}},
    };

    detail::ScoreSystem system{scoring(), 4U};
    static_cast<void>(system.consume({
        .tick = TickIndex{1},
        .events = events,
        .dynamic_entities = dynamic_entities,
        .joint_entity_endpoints = joint_endpoints,
    }));

    NINHO_SIM_REQUIRE(system.has_root_for(dynamic_endpoint));
    NINHO_SIM_REQUIRE(!system.has_root_for(static_endpoint));
    NINHO_SIM_REQUIRE(!system.has_root_for(rootless_first));
    NINHO_SIM_REQUIRE(!system.has_root_for(rootless_second));
    NINHO_SIM_REQUIRE(!system.has_root_for(unknown_target));
}

NINHO_SIM_TEST("score system does not reverse propagate explicitly caused dynamic damage")
{
    constexpr EntityId first_projectile{1000U};
    constexpr EntityId external_projectile{1001U};
    constexpr EntityId target{20U};
    constexpr EntityId relay{21U};
    constexpr std::array dynamic_entities{
        target, relay, first_projectile, external_projectile};
    detail::ScoreSystem system{scoring(), 4U};
    const std::array seed{
        DomainEvent{.id = EventId{1}, .tick = TickIndex{1},
            .kind = DomainEventKind::BirdLaunched,
            .entity_id = first_projectile, .shot_id = 1U},
        DomainEvent{.id = EventId{2}, .tick = TickIndex{2},
            .kind = DomainEventKind::DamageApplied,
            .entity_id = first_projectile, .affected_entity_id = target},
        DomainEvent{.id = EventId{3}, .tick = TickIndex{3},
            .kind = DomainEventKind::BirdLaunched,
            .entity_id = external_projectile, .shot_id = 2U},
    };
    static_cast<void>(system.consume({.tick = TickIndex{3}, .events = seed,
        .dynamic_entities = dynamic_entities}));

    const DomainEvent externally_caused{
        .id = EventId{4}, .tick = TickIndex{4},
        .kind = DomainEventKind::DamageApplied,
        .entity_id = relay, .affected_entity_id = target,
        .cause_event_id = EventId{3},
    };
    static_cast<void>(system.consume({.tick = TickIndex{4},
        .events = std::span{&externally_caused, 1U},
        .dynamic_entities = dynamic_entities}));

    NINHO_SIM_REQUIRE(!system.has_root_for(relay));
}

NINHO_SIM_TEST("score system resolves same tick provenance before awards for every permutation")
{
    struct Outcome {
        detail::ScoreState state;
        std::uint64_t observed_launches{};
        std::vector<std::array<std::uint64_t, 3>> causal_records;
        std::vector<std::array<std::uint64_t, 3>> entity_roots;
    };
    const auto execute = [](const std::array<std::size_t, 3>& order) {
        detail::ScoreSystem system{scoring(), 4U};
        const std::array seed{
            DomainEvent{.id = EventId{1}, .tick = TickIndex{1},
                .kind = DomainEventKind::BirdLaunched,
                .entity_id = EntityId{1000}, .shot_id = 1U},
            DomainEvent{.id = EventId{2}, .tick = TickIndex{2},
                .kind = DomainEventKind::DamageApplied,
                .entity_id = EntityId{1000},
                .affected_entity_id = EntityId{200}},
        };
        consume(system, TickIndex{2}, seed);

        const std::array authored{
            DomainEvent{.id = EventId{3}, .tick = TickIndex{3},
                .kind = DomainEventKind::BirdLaunched,
                .entity_id = EntityId{1001}, .shot_id = 2U},
            DomainEvent{.id = EventId{4}, .tick = TickIndex{3},
                .kind = DomainEventKind::EntityNeutralized,
                .affected_entity_id = EntityId{200}},
            DomainEvent{.id = EventId{5}, .tick = TickIndex{3},
                .kind = DomainEventKind::DamageApplied,
                .entity_id = EntityId{1001},
                .affected_entity_id = EntityId{200}},
        };
        std::array<DomainEvent, 3> events{
            authored[order[0]], authored[order[1]], authored[order[2]]};
        const detail::ScoringCandidate candidate{
            detail::ScoringIdentity::enemy(EntityId{200}), EventId{4}, 5000U};
        const auto transitions = consume(system, TickIndex{3}, events,
            std::span{&candidate, 1U});
        const auto award = std::ranges::find(transitions,
            detail::ScoreTransitionKind::Awarded, &detail::ScoreTransition::kind);
        NINHO_SIM_REQUIRE(award != transitions.end());
        NINHO_SIM_REQUIRE(award->root_cause_event_id == EventId{3});
        NINHO_SIM_REQUIRE(award->shot_id == 2U);

        Outcome result{.state = system.state(),
            .observed_launches = system.observed_launches()};
        for (const auto& record : system.causal_records()) {
            result.causal_records.push_back({record.event_id.value(),
                record.root_event_id.value(), record.shot_id});
        }
        for (const auto& root : system.entity_roots()) {
            result.entity_roots.push_back({root.entity_id.value(),
                root.root_event_id.value(), root.shot_id});
        }
        return result;
    };

    std::array<std::size_t, 3> order{0U, 1U, 2U};
    const Outcome baseline = execute(order);
    while (std::ranges::next_permutation(order).found) {
        const Outcome permuted = execute(order);
        NINHO_SIM_REQUIRE(permuted.state == baseline.state);
        NINHO_SIM_REQUIRE(permuted.observed_launches == baseline.observed_launches);
        NINHO_SIM_REQUIRE(permuted.causal_records == baseline.causal_records);
        NINHO_SIM_REQUIRE(permuted.entity_roots == baseline.entity_roots);
    }
}

NINHO_SIM_TEST("score system preserves target provenance for bounds exit and ejection")
{
    for (const NeutralizationCause neutralization : {
            NeutralizationCause::BoundsExit, NeutralizationCause::Ejection}) {
        detail::ScoreSystem system{scoring(), 4U};
        const std::array seed{
            DomainEvent{.id = EventId{1}, .tick = TickIndex{1},
                .kind = DomainEventKind::BirdLaunched,
                .entity_id = EntityId{1000}, .shot_id = 7U},
            DomainEvent{.id = EventId{2}, .tick = TickIndex{2},
                .kind = DomainEventKind::DamageApplied,
                .entity_id = EntityId{1000},
                .affected_entity_id = EntityId{200}},
        };
        consume(system, TickIndex{2}, seed);
        const DomainEvent exited{.id = EventId{3}, .tick = TickIndex{20},
            .kind = DomainEventKind::EntityNeutralized,
            .affected_entity_id = EntityId{200},
            .neutralization_cause = neutralization};
        const detail::ScoringCandidate candidate{
            detail::ScoringIdentity::enemy(EntityId{200}), exited.id, 5000U};
        const auto transitions = consume(system, TickIndex{20},
            std::span{&exited, 1U}, std::span{&candidate, 1U});
        const auto award = std::ranges::find(transitions,
            detail::ScoreTransitionKind::Awarded, &detail::ScoreTransition::kind);
        NINHO_SIM_REQUIRE(award != transitions.end());
        NINHO_SIM_REQUIRE(award->root_cause_event_id == EventId{1});
        NINHO_SIM_REQUIRE(award->shot_id == 7U);
    }
}

NINHO_SIM_TEST("score system foreign cause cannot mask valid target provenance")
{
    detail::ScoreSystem system{scoring(), 4U};
    const std::array seed{
        DomainEvent{.id = EventId{1}, .tick = TickIndex{1},
            .kind = DomainEventKind::BirdLaunched,
            .entity_id = EntityId{1000}, .shot_id = 5U},
        DomainEvent{.id = EventId{2}, .tick = TickIndex{2},
            .kind = DomainEventKind::DamageApplied,
            .entity_id = EntityId{1000},
            .affected_entity_id = EntityId{200}},
    };
    consume(system, TickIndex{2}, seed);
    const std::array events{
        DomainEvent{.id = EventId{3}, .tick = TickIndex{3},
            .kind = DomainEventKind::PieceFractured,
            .entity_id = EntityId{1000},
            .affected_entity_id = EntityId{200},
            .affected_part_id = PartId{2}, .cause_event_id = EventId{999}},
        DomainEvent{.id = EventId{4}, .tick = TickIndex{3},
            .kind = DomainEventKind::EntityNeutralized,
            .affected_entity_id = EntityId{200},
            .neutralization_cause = NeutralizationCause::BoundsExit},
    };
    const std::array candidates{
        detail::ScoringCandidate{detail::ScoringIdentity::material(
            EntityId{200}, PartId{2}), EventId{3}, 120U},
        detail::ScoringCandidate{detail::ScoringIdentity::enemy(
            EntityId{200}), EventId{4}, 5000U},
    };
    const auto transitions = consume(system, TickIndex{3}, events, candidates);
    std::vector<detail::ScoreTransition> awards;
    std::ranges::copy_if(transitions, std::back_inserter(awards), [](const auto& value) {
        return value.kind == detail::ScoreTransitionKind::Awarded;
    });
    NINHO_SIM_REQUIRE(awards.size() == 2U);
    NINHO_SIM_REQUIRE(awards[0].root_cause_event_id == EventId{});
    NINHO_SIM_REQUIRE(awards[1].root_cause_event_id == EventId{1});
    NINHO_SIM_REQUIRE(awards[1].shot_id == 5U);
}

NINHO_SIM_TEST("score system uses integer multiplier awards from 100 through capped 200 percent")
{
    detail::ScoreSystem system{scoring(), 4U};
    std::vector<DomainEvent> events;
    std::vector<detail::ScoringCandidate> candidates;
    events.push_back({.id = EventId{1}, .tick = TickIndex{1},
        .kind = DomainEventKind::BirdLaunched, .entity_id = EntityId{1000}});
    for (std::uint32_t index = 0; index < 13U; ++index) {
        events.push_back({.id = EventId{2U + index}, .tick = TickIndex{2U + index},
            .kind = DomainEventKind::PieceFractured, .entity_id = EntityId{1000}});
        candidates.push_back({detail::ScoringIdentity::material(
            EntityId{100U + index}, PartId{1}), EventId{2U + index}, 81U});
    }
    const auto transitions = consume(system, TickIndex{14}, events, candidates);
    std::vector<detail::ScoreTransition> awards;
    std::ranges::copy_if(transitions, std::back_inserter(awards), [](const auto& value) {
        return value.kind == detail::ScoreTransitionKind::Awarded;
    });
    NINHO_SIM_REQUIRE(awards.size() == 13U);
    NINHO_SIM_REQUIRE(awards[0].multiplier_percent == 100U);
    NINHO_SIM_REQUIRE(awards[1].multiplier_percent == 110U);
    NINHO_SIM_REQUIRE(awards[1].awarded_points == 89U);
    NINHO_SIM_REQUIRE(awards[10].multiplier_percent == 200U);
    NINHO_SIM_REQUIRE(awards[12].multiplier_percent == 200U);
}

NINHO_SIM_TEST("score system formula is fixed at ten percent and capped at two times")
{
    ScoringDefinition authored = scoring();
    authored.chain_multiplier_step = 0.25;
    authored.max_chain_multiplier = 3.0;
    detail::ScoreSystem system{authored, 4U};
    std::vector<DomainEvent> events;
    std::vector<detail::ScoringCandidate> candidates;
    events.push_back({.id = EventId{1}, .tick = TickIndex{1},
        .kind = DomainEventKind::BirdLaunched, .entity_id = EntityId{1000}});
    for (std::uint32_t index = 0; index < 12U; ++index) {
        events.push_back({.id = EventId{2U + index},
            .tick = TickIndex{2U + index},
            .kind = DomainEventKind::PieceFractured,
            .entity_id = EntityId{1000}});
        candidates.push_back({detail::ScoringIdentity::material(
            EntityId{100U + index}, PartId{1}), EventId{2U + index}, 100U});
    }
    const auto transitions = consume(system, TickIndex{13}, events, candidates);
    std::vector<detail::ScoreTransition> awards;
    std::ranges::copy_if(transitions, std::back_inserter(awards), [](const auto& value) {
        return value.kind == detail::ScoreTransitionKind::Awarded;
    });
    NINHO_SIM_REQUIRE(awards.size() == 12U);
    NINHO_SIM_REQUIRE(awards[1].multiplier_percent == 110U);
    NINHO_SIM_REQUIRE(awards[10].multiplier_percent == 200U);
    NINHO_SIM_REQUIRE(awards[11].multiplier_percent == 200U);
}

NINHO_SIM_TEST("score system awards unused queue slots and stars exactly once at victory")
{
    detail::ScoreSystem system{scoring(), 4U};
    const auto first = consume(system, TickIndex{1}, {}, {},
        detail::ScoreTerminalState::Victory, 2U);
    NINHO_SIM_REQUIRE(system.state().current_score == 20000U);
    NINHO_SIM_REQUIRE(system.state().stars == 1U);
    NINHO_SIM_REQUIRE(system.state().terminal_bonus_awarded);
    const std::vector expected_identities{
        detail::ScoringIdentity::unused_bird(2U),
        detail::ScoringIdentity::unused_bird(3U)};
    NINHO_SIM_REQUIRE(system.state().scored_identities == expected_identities);
    NINHO_SIM_REQUIRE(std::ranges::count_if(first, [](const auto& value) {
        return value.kind == detail::ScoreTransitionKind::Awarded;
    }) == 2);
    for (std::uint64_t tick = 2; tick <= 101; ++tick) {
        NINHO_SIM_REQUIRE(consume(system, TickIndex{tick}, {}, {},
            detail::ScoreTerminalState::Victory, 2U).empty());
    }
    NINHO_SIM_REQUIRE(system.state().current_score == 20000U);
}

NINHO_SIM_TEST("score system terminal thresholds and defeat are exact")
{
    const auto terminal_after = [](std::uint32_t points,
                                   detail::ScoreTerminalState terminal) {
        detail::ScoreSystem system{scoring(), 0U};
        const std::array events{
            DomainEvent{.id = EventId{1}, .tick = TickIndex{1},
                .kind = DomainEventKind::BirdLaunched, .entity_id = EntityId{1000}},
            DomainEvent{.id = EventId{2}, .tick = TickIndex{2},
                .kind = DomainEventKind::EntityNeutralized, .entity_id = EntityId{1000}},
        };
        const detail::ScoringCandidate candidate{
            detail::ScoringIdentity::enemy(EntityId{200}), EventId{2}, points};
        consume(system, TickIndex{2}, events, std::span{&candidate, 1U});
        consume(system, TickIndex{3}, {}, {}, terminal, 0U);
        return system.state();
    };
    NINHO_SIM_REQUIRE(terminal_after(1U, detail::ScoreTerminalState::Victory).stars == 1U);
    NINHO_SIM_REQUIRE(terminal_after(38000U, detail::ScoreTerminalState::Victory).stars == 2U);
    NINHO_SIM_REQUIRE(terminal_after(49999U, detail::ScoreTerminalState::Victory).stars == 2U);
    NINHO_SIM_REQUIRE(terminal_after(50000U, detail::ScoreTerminalState::Victory).stars == 3U);
    const auto defeat = terminal_after(50000U, detail::ScoreTerminalState::Defeat);
    NINHO_SIM_REQUIRE(defeat.stars == 0U);
    NINHO_SIM_REQUIRE(defeat.current_score == 50000U);
}

NINHO_SIM_TEST("score system session integration publishes score state events and restart")
{
    MaterialCatalog materials{.schema_version = 2U, .source_schema_version = 2U};
    materials.surfaces = {
        {SurfaceId{1}, "bird", 1000.0, 0.4, 0.1},
        {SurfaceId{2}, "pig", 500.0, 0.65, 0.05},
    };
    ArchetypeCatalog archetypes{.schema_version = 2U, .source_schema_version = 2U};
    archetypes.presentation_ids = {"bird", "icon", "animation", "pig"};
    archetypes.score_ids = {"bird_score"};
    archetypes.abilities.push_back({.id = AbilityId{1}, .key = "gravity_field",
        .kind = "gravity_field", .kind_v2 = AbilityKind::GravityField,
        .payload = GravityFieldAbilityDefinition{9U, 75U, 6.0, 1000.0,
            20U, 25.0, 10.0}});
    archetypes.birds.push_back({.id = BirdArchetypeId{1}, .key = "bird",
        .ability_id = AbilityId{1}, .surface_id = SurfaceId{1},
        .mass_kg = 8.0, .radius_m = 0.3, .friction = 0.4,
        .restitution = 0.1, .bullet = true, .projectile_visual_id = "bird",
        .launch_speed_cap_m_s = 45.0, .score_id = "bird_score",
        .icon_id = "icon", .animation_id = "animation"});
    archetypes.weakpoints.push_back({WeakpointId{1}, "uniform",
        {0.0, 1.0, 0.0}, 1.0, 1.0, 1.0});
    archetypes.enemies.push_back({EnemyArchetypeId{1}, "pig", WeakpointId{1},
        SurfaceId{2}, 65.0, 100.0, 18.0, 70.0,
        EnemyDamageModel::TerrestrialPig});

    LevelManifest level;
    level.schema_version = level.source_schema_version = 2U;
    level.id = "score_integration";
    level.world_id = "earth";
    level.world = UniformWorldDefinition{.acceleration_m_s2 = {0.0, -9.81, 0.0},
        .bounds_min_m = {-100.0, -100.0, -100.0},
        .bounds_max_m = {100.0, 100.0, 100.0}};
    level.slingshot = {.asset_id = "launcher", .rest_position_m = {-4.0, 2.0, 0.0},
        .rest_rotation_xyzw = {0.0, 0.0, 0.0, 1.0},
        .spring_constant_n_m = 520.0, .energy_efficiency = 0.9,
        .minimum_extension_m = 0.2, .maximum_extension_m = 4.25,
        .plane_policy = "gravity_vertical_camera_yaw",
        .projectile_clearance_m = 0.0, .speed_ceiling_m_s = 45.0};
    level.bird_queue = {BirdArchetypeId{1}, BirdArchetypeId{1},
        BirdArchetypeId{1}, BirdArchetypeId{1}};
    level.scoring = scoring();
    level.settle_policy = {0.001, 0.001, 3U};
    level.watchdog_ticks = 20U;
    BodyDefinition pig;
    pig.body_id = 1U;
    pig.entity_id = EntityId{200};
    pig.part_id = PartId{1};
    pig.body_type = BodyType::Dynamic;
    pig.surface_id = SurfaceId{2};
    pig.enemy_archetype_id = EnemyArchetypeId{1};
    pig.shape = {.type = ShapeType::Sphere, .radius_m = 0.5};
    pig.density_kg_m3 = 65.0
        / (4.0 / 3.0 * std::numbers::pi * 0.5 * 0.5 * 0.5);
    pig.transform.position_m = {0.0, 1.0, 0.0};
    pig.transform.rotation_xyzw = {0.0, 0.0, 0.0, 1.0};
    pig.visual = {.asset_id = "pig", .bounds_m = {1.0, 1.0, 1.0}};
    level.bodies.push_back(pig);
    level.free_body_ids.push_back(1U);
    level.objectives.push_back({1U, ObjectiveKind::NeutralizeEntity, EntityId{200}});

    auto created = SimulationSession::create(materials, archetypes, level);
    if (!created.ok()) {
        ninho::simulation::test::fail(__FILE__, __LINE__,
            created.error.pointer + ": " + created.error.message);
    }
    auto session = std::move(created.value);
    NINHO_SIM_REQUIRE(session->score_state().score == 0U);
    NINHO_SIM_REQUIRE(detail::SessionTestFacade::queue_external_damage(
        *session, EntityId{200}, PartId{1}, 65.0 * 200.0, EventId{77}));
    NINHO_SIM_REQUIRE(detail::SessionTestFacade::queue_external_damage(
        *session, EntityId{200}, PartId{1}, 65.0 * 200.0, EventId{78}));
    const SessionStatus tick_status = session->tick();
    if (!tick_status.ok()) {
        ninho::simulation::test::fail(__FILE__, __LINE__,
            tick_status.error.pointer + ": " + tick_status.error.message);
    }
    NINHO_SIM_REQUIRE(session->score_state().score == 5000U);
    NINHO_SIM_REQUIRE(std::ranges::any_of(session->events(), [](const auto& event) {
        return event.kind == DomainEventKind::ScoreAwarded
            && event.scoring_identity_kind == ScoringIdentityKind::EnemyEntity
            && event.affected_entity_id == EntityId{200}
            && event.base_points == 5000U && event.awarded_points == 5000U;
    }));

    auto canonical = session->canonical_state_v3();
    const auto mutation_changes_canonical = [&](auto mutate) {
        NINHO_SIM_REQUIRE(mutate(*session));
        detail::SessionTestFacade::refresh_canonical_state(*session);
        NINHO_SIM_REQUIRE(session->canonical_state_v3() != canonical);
        canonical = session->canonical_state_v3();
    };
    mutation_changes_canonical(detail::SessionTestFacade::bump_score_total);
    mutation_changes_canonical(detail::SessionTestFacade::bump_score_root);
    mutation_changes_canonical(detail::SessionTestFacade::shift_score_tick);
    mutation_changes_canonical(detail::SessionTestFacade::bump_score_chain);
    mutation_changes_canonical(detail::SessionTestFacade::append_score_identity);
    const auto canonical_ordered_identities = session->canonical_state_v3();
    NINHO_SIM_REQUIRE(detail::SessionTestFacade::reverse_score_identities(*session));
    detail::SessionTestFacade::refresh_canonical_state(*session);
    NINHO_SIM_REQUIRE(session->canonical_state_v3() == canonical_ordered_identities);
    canonical = session->canonical_state_v3();
    mutation_changes_canonical(detail::SessionTestFacade::toggle_score_terminal_gate);
    mutation_changes_canonical(detail::SessionTestFacade::bump_score_stars);

    NINHO_SIM_REQUIRE(session->restart().ok());
    NINHO_SIM_REQUIRE(session->score_state().score == 0U);
    NINHO_SIM_REQUIRE(session->score_state().stars == 0U);
    NINHO_SIM_REQUIRE(session->score_state().chain_index == 0U);
    NINHO_SIM_REQUIRE(session->score_state().multiplier_percent == 100U);
}

NINHO_SIM_TEST("score system omits scoring-only crush provenance when score is disabled")
{
    auto session = integration_session(false);
    const SessionStatus tick_status = session->tick();
    if (!tick_status.ok()) {
        ninho::simulation::test::fail(__FILE__, __LINE__,
            tick_status.error.pointer + ": " + tick_status.error.message);
    }
    const auto canonical_without_provenance = session->canonical_state_v3();
    NINHO_SIM_REQUIRE(detail::SessionTestFacade::set_crush_load_cause_for_testing(
        *session, EntityId{200}, PartId{1}, EntityId{77}));
    detail::SessionTestFacade::refresh_canonical_state(*session);
    NINHO_SIM_REQUIRE(session->canonical_state_v3() == canonical_without_provenance);
}

}
