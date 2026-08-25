#include "test_framework.hpp"

#include "ninho/simulation/session.hpp"
#include "session_internal.hpp"
#include "session_test_facade.hpp"

#include <algorithm>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <optional>
#include <ranges>
#include <sstream>
#include <string_view>

namespace {

using namespace ninho::simulation;

std::string read_source_file(std::string_view relative)
{
    std::ifstream stream{std::filesystem::path{NINHO_SOURCE_DIR} / relative,
        std::ios::binary};
    NINHO_SIM_REQUIRE(stream.is_open());
    std::ostringstream text;
    text << stream.rdbuf();
    return text.str();
}

std::unique_ptr<SimulationSession> create_session()
{
    const auto materials = parse_material_catalog(
        read_source_file("game/data/materials/vertical_slice.materials.json"));
    const auto archetypes = parse_archetype_catalog(
        read_source_file("game/data/archetypes/vertical_slice.archetypes.json"));
    auto level = parse_level_manifest(
        read_source_file("game/data/levels/first_orbit.level.json"));
    NINHO_SIM_REQUIRE(materials.ok() && archetypes.ok() && level.ok());
    auto created = SimulationSession::create(materials.value, archetypes.value, level.value);
    NINHO_SIM_REQUIRE(created.ok());
    return std::move(created.value);
}

struct JointlessFractureOptions {
    double impact_speed_m_s{16.0};
    MaterialResponse response{MaterialResponse::Brittle};
    bool authored_pattern{true};
    bool exhaust_body_capacity{false};
    bool enable_scoring{false};
    bool remove_incident_joints{true};
};

std::unique_ptr<SimulationSession> create_jointless_fracture_session(
    JointlessFractureOptions options = {})
{
    auto materials = parse_material_catalog(
        read_source_file("game/data/materials/vertical_slice.materials.json"));
    const auto archetypes = parse_archetype_catalog(
        read_source_file("game/data/archetypes/vertical_slice.archetypes.json"));
    auto level = parse_level_manifest(
        read_source_file("game/data/levels/first_orbit.level.json"));
    NINHO_SIM_REQUIRE(materials.ok() && archetypes.ok() && level.ok());

    if (options.enable_scoring) {
        level.value.scoring = {
            .pig_points = 5000U,
            .unused_bird_points = 10000U,
            .star_thresholds = {1U, 38000U, 50000U},
            .chain_window_ticks = 45U,
            .chain_multiplier_step = 0.10,
            .max_chain_multiplier = 2.0,
        };
    }

    constexpr EntityId target_entity{110};
    constexpr PartId target_part{1};
    const auto material = std::ranges::find(
        materials.value.materials, MaterialId{9}, &MaterialDefinition::id);
    NINHO_SIM_REQUIRE(material != materials.value.materials.end());
    material->response = options.response;

    const auto target = std::ranges::find_if(
        level.value.bodies, [](const BodyDefinition& body) {
            return body.entity_id == target_entity && body.part_id == target_part;
    });
    NINHO_SIM_REQUIRE(target != level.value.bodies.end());
    const std::uint32_t target_body_id = target->body_id;
    target->fracture_pattern.reset();
    if (options.authored_pattern) {
        ShapeDefinition half;
        half.type = ShapeType::Box;
        half.half_extents_m = {0.02, 0.35, 0.225};
        PhysicalFragmentDefinition first{
            1U, half, {{0.0, 0.0, -0.225}, {0.0, 0.0, 0.0, 1.0}},
            2450.0, "TEST_GlassHalf_A"};
        PhysicalFragmentDefinition second{
            2U, half, {{0.0, 0.0, 0.225}, {0.0, 0.0, 0.0, 1.0}},
            2450.0, "TEST_GlassHalf_B"};
        target->fracture_pattern = FracturePatternDefinition{
            {std::move(first), std::move(second)}, {}};
    }
    if (options.remove_incident_joints) {
        target->assembly_id.reset();
        std::vector<JointId> removed_joints;
        std::erase_if(level.value.joints, [&](const JointDefinition& joint) {
            const bool incident = joint.body_a_id == target_body_id
                || joint.body_b_id == target_body_id;
            if (incident) removed_joints.push_back(joint.id);
            return incident;
        });
        for (AssemblyDefinition& assembly : level.value.assemblies) {
            std::erase(assembly.body_ids, target_body_id);
            for (const JointId joint : removed_joints) {
                std::erase(assembly.joint_ids, joint);
            }
        }
        level.value.free_body_ids.push_back(target_body_id);
    }

    auto created = SimulationSession::create(
        materials.value, archetypes.value, level.value);
    if (!created.ok()) {
        ninho::simulation::test::fail(__FILE__, __LINE__,
            created.error.pointer + ": " + created.error.message);
    }
    auto session = std::move(created.value);
    const auto snapshot = std::ranges::find_if(
        session->snapshots(), [](const EntitySnapshot& body) {
            return body.entity_id == target_entity && body.part_id == target_part;
        });
    NINHO_SIM_REQUIRE(snapshot != session->snapshots().end());
    const ninho::physics::Vec3 impact_position = snapshot->transform.position
        + ninho::physics::Vec3{0.0f, 0.0f, -0.97f};
    NINHO_SIM_REQUIRE(detail::SessionTestFacade::add_dynamic_sphere(*session,
        EntityId{0x80002000U}, PartId{1}, impact_position, 140.0,
        {0.0f, 0.0f, static_cast<float>(options.impact_speed_m_s)}, 0.5));

    if (options.exhaust_body_capacity) {
        std::uint32_t identity = 0x81000000U;
        while (detail::SessionTestFacade::remaining_body_capacity(*session) > 1U) {
            NINHO_SIM_REQUIRE(detail::SessionTestFacade::add_static_sphere(
                *session, EntityId{identity++}, {0.0f, 0.0f, 0.0f}, 0.001));
        }
    }
    return session;
}

std::unique_ptr<SimulationSession> create_farm_session()
{
    const auto materials = parse_material_catalog(
        read_source_file("game/data/materials/product_v2.materials.json"));
    const auto archetypes = parse_archetype_catalog(
        read_source_file("game/data/archetypes/product_v2.archetypes.json"));
    const auto level = parse_level_manifest(
        read_source_file("game/data/levels/earth/farm_reaction.level.json"));
    NINHO_SIM_REQUIRE(materials.ok() && archetypes.ok() && level.ok());
    auto created = SimulationSession::create(
        materials.value, archetypes.value, level.value);
    NINHO_SIM_REQUIRE(created.ok());
    return std::move(created.value);
}

const DomainEvent* find_event(
    const SimulationSession& session, DomainEventKind kind, JointId joint = {})
{
    const auto found = std::ranges::find_if(session.events(), [&](const DomainEvent& event) {
        return event.kind == kind && (joint == JointId{} || event.joint_id == joint);
    });
    return found == session.events().end() ? nullptr : &*found;
}

const DomainEvent* find_target_event(
    const SimulationSession& session, DomainEventKind kind)
{
    constexpr EntityId target_entity{110};
    constexpr PartId target_part{1};
    const auto found = std::ranges::find_if(
        session.events(), [&](const DomainEvent& event) {
            return event.kind == kind
                && event.affected_entity_id == target_entity
                && event.affected_part_id == target_part;
        });
    return found == session.events().end() ? nullptr : &*found;
}

std::optional<DomainEvent> advance_to_jointless_trigger(
    SimulationSession& session, int maximum_ticks = 40)
{
    for (int tick = 0; tick < maximum_ticks; ++tick) {
        for (const StructuralJointSnapshot& joint : session.structural_joints()) {
            if (joint.active) {
                detail::SessionTestFacade::override_joint_ratio_after_solver(
                    session, joint.id, 0.0);
            }
        }
        NINHO_SIM_REQUIRE(session.tick().ok());
        const DomainEvent* trigger = find_target_event(
            session, DomainEventKind::PieceFractureTriggered);
        if (trigger != nullptr) return *trigger;
    }
    return std::nullopt;
}

NINHO_SIM_TEST("fracture objective physically fractures a free brittle patterned body without a joint")
{
    auto session = create_jointless_fracture_session();
    constexpr EntityId target_entity{110};
    constexpr PartId target_part{1};
    const auto parent = std::ranges::find_if(
        session->snapshots(), [](const EntitySnapshot& body) {
            return body.entity_id == target_entity && body.part_id == target_part;
        });
    NINHO_SIM_REQUIRE(parent != session->snapshots().end());
    const double parent_mass = parent->mass_kg;
    const std::size_t body_records_before =
        detail::SessionTestFacade::body_record_count(*session);
    const std::size_t capacity_before =
        detail::SessionTestFacade::remaining_body_capacity(*session);
    NINHO_SIM_REQUIRE(std::ranges::none_of(
        session->structural_joints(), [](const StructuralJointSnapshot& joint) {
            return joint.a == JointEndpoint{target_entity, target_part}
                || joint.b == JointEndpoint{target_entity, target_part};
        }));

    const auto trigger = advance_to_jointless_trigger(*session);
    NINHO_SIM_REQUIRE(trigger.has_value());
    NINHO_SIM_REQUIRE(trigger->joint_id == JointId{});
    NINHO_SIM_REQUIRE(trigger->cause_event_id != EventId{});
    NINHO_SIM_REQUIRE(trigger->fracture_ratio >= 1.0);
    const auto damage = std::ranges::find(
        session->events(), trigger->cause_event_id, &DomainEvent::id);
    NINHO_SIM_REQUIRE(damage != session->events().end());
    NINHO_SIM_REQUIRE(damage->kind == DomainEventKind::DamageApplied);
    NINHO_SIM_REQUIRE(std::ranges::none_of(session->events(), [](const DomainEvent& event) {
        return event.kind == DomainEventKind::JointBroken;
    }));

    for (const StructuralJointSnapshot& joint : session->structural_joints()) {
        if (joint.active) {
            detail::SessionTestFacade::override_joint_ratio_after_solver(
                *session, joint.id, 0.0);
        }
    }
    NINHO_SIM_REQUIRE(session->tick().ok());
    const DomainEvent* fractured = find_target_event(
        *session, DomainEventKind::PieceFractured);
    NINHO_SIM_REQUIRE(fractured != nullptr);
    NINHO_SIM_REQUIRE(fractured->joint_id == JointId{});
    NINHO_SIM_REQUIRE(fractured->cause_event_id == trigger->id);
    NINHO_SIM_REQUIRE(std::ranges::none_of(session->events(), [](const DomainEvent& event) {
        return event.kind == DomainEventKind::JointBroken;
    }));

    std::vector<EntitySnapshot> fragments;
    for (const EntitySnapshot& body : session->snapshots()) {
        if (body.entity_id == target_entity) fragments.push_back(body);
    }
    NINHO_SIM_REQUIRE(fragments.size() == 2U);
    NINHO_SIM_REQUIRE(std::ranges::none_of(fragments, [](const EntitySnapshot& body) {
        return body.part_id == target_part;
    }));
    double fragment_mass = 0.0;
    std::vector<std::string> fragment_visuals;
    for (const EntitySnapshot& fragment : fragments) {
        NINHO_SIM_REQUIRE(fragment.body_type == BodyType::Dynamic);
        NINHO_SIM_REQUIRE(detail::SessionTestFacade::affected_by_world_gravity(
            *session, fragment.entity_id, fragment.part_id));
        fragment_mass += fragment.mass_kg;
        fragment_visuals.push_back(fragment.visual_id);
    }
    std::ranges::sort(fragment_visuals);
    const std::vector<std::string> expected_visuals{
        "TEST_GlassHalf_A", "TEST_GlassHalf_B"};
    NINHO_SIM_REQUIRE(fragment_visuals == expected_visuals);
    NINHO_SIM_REQUIRE(std::abs(fragment_mass - parent_mass) <= parent_mass * 1.0e-6);
    NINHO_SIM_REQUIRE(detail::SessionTestFacade::body_record_count(*session)
        == body_records_before + 1U);
    NINHO_SIM_REQUIRE(detail::SessionTestFacade::remaining_body_capacity(*session)
        == capacity_before - 1U);
}

NINHO_SIM_TEST("fracture objective jointless material path rejects ineligible bodies")
{
    for (const JointlessFractureOptions options : {
        JointlessFractureOptions{.impact_speed_m_s = 2.0},
        JointlessFractureOptions{.response = MaterialResponse::Ductile},
        JointlessFractureOptions{.authored_pattern = false},
    }) {
        auto session = create_jointless_fracture_session(options);
        NINHO_SIM_REQUIRE(!advance_to_jointless_trigger(*session).has_value());
        const auto parent = std::ranges::find_if(
            session->snapshots(), [](const EntitySnapshot& body) {
                return body.entity_id == EntityId{110} && body.part_id == PartId{1};
            });
        NINHO_SIM_REQUIRE(parent != session->snapshots().end());
    }
}

NINHO_SIM_TEST("fracture objective jointless capacity failure retains the parent atomically")
{
    auto session = create_jointless_fracture_session(
        {.exhaust_body_capacity = true});
    const std::size_t body_records_before =
        detail::SessionTestFacade::body_record_count(*session);
    NINHO_SIM_REQUIRE(detail::SessionTestFacade::remaining_body_capacity(*session) == 1U);
    const auto trigger = advance_to_jointless_trigger(*session);
    NINHO_SIM_REQUIRE(trigger.has_value());
    NINHO_SIM_REQUIRE(trigger->joint_id == JointId{});

    for (int tick = 0; tick < 3; ++tick) {
        NINHO_SIM_REQUIRE(session->tick().ok());
        NINHO_SIM_REQUIRE(find_target_event(
            *session, DomainEventKind::PieceFractured) == nullptr);
        NINHO_SIM_REQUIRE(find_target_event(
            *session, DomainEventKind::PieceFractureTriggered) == nullptr);
        const auto parent = std::ranges::find_if(
            session->snapshots(), [](const EntitySnapshot& body) {
                return body.entity_id == EntityId{110} && body.part_id == PartId{1};
            });
        NINHO_SIM_REQUIRE(parent != session->snapshots().end());
        NINHO_SIM_REQUIRE(std::ranges::count(
            session->snapshots(), EntityId{110}, &EntitySnapshot::entity_id) == 1);
        NINHO_SIM_REQUIRE(detail::SessionTestFacade::body_record_count(*session)
            == body_records_before);
        NINHO_SIM_REQUIRE(
            detail::SessionTestFacade::remaining_body_capacity(*session) == 1U);
    }
}

NINHO_SIM_TEST("fracture objective cancels a retained physical fracture when its parent disappears")
{
    auto session = create_jointless_fracture_session({
        .exhaust_body_capacity = true,
        .enable_scoring = true,
    });
    constexpr EntityId target_entity{110};
    constexpr PartId target_part{1};
    const auto trigger = advance_to_jointless_trigger(*session);
    NINHO_SIM_REQUIRE(trigger.has_value());
    NINHO_SIM_REQUIRE(trigger->joint_id == JointId{});

    // First retry proves the capacity gate retained the physical replacement.
    NINHO_SIM_REQUIRE(session->tick().ok());
    NINHO_SIM_REQUIRE(find_target_event(
        *session, DomainEventKind::PieceFractured) == nullptr);
    NINHO_SIM_REQUIRE(std::ranges::count(
        session->snapshots(), target_entity,
        &EntitySnapshot::entity_id) == 1);
    const std::uint64_t score_before = session->score_state().score;

    NINHO_SIM_REQUIRE(detail::SessionTestFacade::remove_body_for_testing(
        *session, target_entity, target_part));
    NINHO_SIM_REQUIRE(session->tick().ok());

    NINHO_SIM_REQUIRE(find_target_event(
        *session, DomainEventKind::PieceFractured) == nullptr);
    NINHO_SIM_REQUIRE(std::ranges::none_of(
        session->events(), [](const DomainEvent& event) {
            return event.kind == DomainEventKind::ScoreAwarded;
        }));
    NINHO_SIM_REQUIRE(session->score_state().score == score_before);
    NINHO_SIM_REQUIRE(std::ranges::none_of(
        session->snapshots(), [](const EntitySnapshot& body) {
            return body.entity_id == EntityId{110};
    }));
}

NINHO_SIM_TEST("fracture objective treats a patterned jointed pending as a required physical replacement")
{
    auto session = create_jointless_fracture_session({
        .exhaust_body_capacity = true,
        .enable_scoring = true,
        .remove_incident_joints = false,
    });
    constexpr EntityId target_entity{110};
    constexpr PartId target_part{1};
    const auto trigger = advance_to_jointless_trigger(*session);
    NINHO_SIM_REQUIRE(trigger.has_value());
    NINHO_SIM_REQUIRE(trigger->joint_id != JointId{});

    NINHO_SIM_REQUIRE(session->tick().ok());
    NINHO_SIM_REQUIRE(find_target_event(
        *session, DomainEventKind::PieceFractured) == nullptr);
    const std::uint64_t score_before = session->score_state().score;
    NINHO_SIM_REQUIRE(detail::SessionTestFacade::remove_body_for_testing(
        *session, target_entity, target_part));
    NINHO_SIM_REQUIRE(session->tick().ok());

    NINHO_SIM_REQUIRE(find_target_event(
        *session, DomainEventKind::PieceFractured) == nullptr);
    NINHO_SIM_REQUIRE(std::ranges::none_of(
        session->events(), [](const DomainEvent& event) {
            return event.kind == DomainEventKind::ScoreAwarded;
        }));
    NINHO_SIM_REQUIRE(session->score_state().score == score_before);
    NINHO_SIM_REQUIRE(std::ranges::none_of(
        session->snapshots(), [](const EntitySnapshot& body) {
            return body.entity_id == EntityId{110};
        }));
}

NINHO_SIM_TEST("fracture objective static authored support cannot emit a fracture ratio")
{
    auto session = create_farm_session();
    std::vector<DomainEvent> history;
    const auto tick = [&] {
        NINHO_SIM_REQUIRE(session->tick().ok());
        history.insert(history.end(), session->events().begin(), session->events().end());
    };
    NINHO_SIM_REQUIRE(session->enqueue(
        BeginGrabCommand{{1.0F, 0.0F, 0.0F}}).ok());
    tick();
    NINHO_SIM_REQUIRE(session->enqueue(SetPullCommand{-4.0, -1.0}).ok());
    NINHO_SIM_REQUIRE(session->enqueue(ReleaseBirdCommand{}).ok());
    tick();
    const auto shot = session->shot_state();
    NINHO_SIM_REQUIRE(shot.has_value());
    while (session->state().tick.value() < shot->launch_tick.value() + 9U) tick();
    NINHO_SIM_REQUIRE(session->enqueue(ActivateAbilityCommand{}).ok());
    tick();
    for (int step = 0; step < 60; ++step) tick();

    NINHO_SIM_REQUIRE(session->state().phase != SessionPhase::Faulted);
    NINHO_SIM_REQUIRE(std::ranges::none_of(history, [](const DomainEvent& event) {
        return event.kind == DomainEventKind::PieceFractureTriggered
            && event.affected_entity_id == EntityId{3013};
    }));
}

NINHO_SIM_TEST("fracture objective overload schedules after solver and applies next tick")
{
    auto session = create_session();
    constexpr JointId joint{1};
    detail::SessionTestFacade::override_joint_ratio_after_solver(*session, joint, 1.5);

    NINHO_SIM_REQUIRE(session->tick().ok());
    NINHO_SIM_REQUIRE(find_event(*session, DomainEventKind::DamageApplied) == nullptr);
    const DomainEvent* overload = find_event(*session, DomainEventKind::JointOverloaded, joint);
    NINHO_SIM_REQUIRE(overload != nullptr);
    NINHO_SIM_REQUIRE(overload->joint_load_ratio == 1.5);
    const EventId overload_id = overload->id;
    NINHO_SIM_REQUIRE(std::ranges::find(session->structural_joints(), joint,
        &StructuralJointSnapshot::id)->active);
    NINHO_SIM_REQUIRE(find_event(*session, DomainEventKind::JointBroken, joint) == nullptr);

    NINHO_SIM_REQUIRE(session->tick().ok());
    const auto broken = std::ranges::find(session->structural_joints(), joint,
        &StructuralJointSnapshot::id);
    NINHO_SIM_REQUIRE(broken != session->structural_joints().end() && !broken->active);
    const DomainEvent* event = find_event(*session, DomainEventKind::JointBroken, joint);
    NINHO_SIM_REQUIRE(event != nullptr);
    NINHO_SIM_REQUIRE(event->cause_event_id == overload_id);

    NINHO_SIM_REQUIRE(session->tick().ok());
    NINHO_SIM_REQUIRE(find_event(*session, DomainEventKind::JointBroken, joint) == nullptr);
}

NINHO_SIM_TEST("fracture objective consecutive ratio threshold resets below one")
{
    auto session = create_session();
    constexpr JointId joint{2};
    for (const double ratio : {1.0, 0.99, 1.0}) {
        detail::SessionTestFacade::override_joint_ratio_after_solver(*session, joint, ratio);
        NINHO_SIM_REQUIRE(session->tick().ok());
    }
    NINHO_SIM_REQUIRE(std::ranges::find(session->structural_joints(), joint,
        &StructuralJointSnapshot::id)->active);
    detail::SessionTestFacade::override_joint_ratio_after_solver(*session, joint, 1.0);
    NINHO_SIM_REQUIRE(session->tick().ok());
    NINHO_SIM_REQUIRE(std::ranges::find(session->structural_joints(), joint,
        &StructuralJointSnapshot::id)->active);
    NINHO_SIM_REQUIRE(session->tick().ok());
    NINHO_SIM_REQUIRE(!std::ranges::find(session->structural_joints(), joint,
        &StructuralJointSnapshot::id)->active);
}

NINHO_SIM_TEST("fracture objective consecutive overload publishes cause on second threshold tick")
{
    auto session = create_session();
    constexpr JointId joint{3};
    detail::SessionTestFacade::override_joint_ratio_after_solver(*session, joint, 1.0);
    NINHO_SIM_REQUIRE(session->tick().ok());
    NINHO_SIM_REQUIRE(find_event(*session, DomainEventKind::DamageApplied) == nullptr);
    NINHO_SIM_REQUIRE(find_event(*session, DomainEventKind::JointOverloaded, joint) == nullptr);

    // Repeating the same override is the point: a second tick over the
    // threshold without any new cause is what publishes the overload. The
    // former "without_new_cause" entry point was byte-identical to this one and
    // only made the test look like it exercised a second path.
    detail::SessionTestFacade::override_joint_ratio_after_solver(*session, joint, 1.0);
    NINHO_SIM_REQUIRE(session->tick().ok());
    NINHO_SIM_REQUIRE(find_event(*session, DomainEventKind::DamageApplied) == nullptr);
    const DomainEvent* overload = find_event(*session, DomainEventKind::JointOverloaded, joint);
    NINHO_SIM_REQUIRE(overload != nullptr);
    const EventId overload_id = overload->id;
    NINHO_SIM_REQUIRE(session->tick().ok());
    const DomainEvent* broken = find_event(*session, DomainEventKind::JointBroken, joint);
    NINHO_SIM_REQUIRE(broken != nullptr);
    NINHO_SIM_REQUIRE(broken->cause_event_id == overload_id);
}

NINHO_SIM_TEST("fracture objective material threshold publishes typed trigger")
{
    auto session = create_session();
    constexpr EntityId target_entity{110};
    constexpr PartId target_part{1};
    const auto target = std::ranges::find_if(session->snapshots(), [](const auto& snapshot) {
        return snapshot.entity_id == target_entity && snapshot.part_id == target_part;
    });
    NINHO_SIM_REQUIRE(target != session->snapshots().end());
    const ninho::physics::Vec3 impact_position = target->transform.position
        + ninho::physics::Vec3{0.0f, 0.0f, -0.97f};
    NINHO_SIM_REQUIRE(detail::SessionTestFacade::add_dynamic_sphere(*session,
        EntityId{0x80002000U}, PartId{1}, impact_position, 140.0,
        {0.0f, 0.0f, 16.0f}, 0.5));

    std::optional<DomainEvent> damage;
    std::optional<DomainEvent> trigger;
    for (int tick = 0; tick < 30 && !trigger; ++tick) {
        for (const StructuralJointSnapshot& joint : session->structural_joints()) {
            if (joint.active) {
                detail::SessionTestFacade::override_joint_ratio_after_solver(
                    *session, joint.id, 0.0);
            }
        }
        NINHO_SIM_REQUIRE(session->tick().ok());
        const auto material_trigger = std::ranges::find_if(
            session->events(), [](const auto& event) {
                return event.kind == DomainEventKind::PieceFractureTriggered
                    && event.affected_entity_id == target_entity
                    && event.affected_part_id == target_part;
        });
        if (material_trigger != session->events().end()) {
            trigger = *material_trigger;
            const auto causal_damage = std::ranges::find_if(
                session->events(), [&](const auto& event) {
                    return event.id == trigger->cause_event_id
                        && event.kind == DomainEventKind::DamageApplied;
                });
            if (causal_damage != session->events().end()) {
                damage = *causal_damage;
            }
        }
        NINHO_SIM_REQUIRE(
            find_event(*session, DomainEventKind::JointOverloaded) == nullptr);
    }

    NINHO_SIM_REQUIRE(damage.has_value() && trigger.has_value());
    NINHO_SIM_REQUIRE(trigger->cause_event_id == damage->id);
    NINHO_SIM_REQUIRE(trigger->fracture_ratio >= 1.0);
    NINHO_SIM_REQUIRE(trigger->joint_load_ratio == 0.0);

    for (const StructuralJointSnapshot& joint : session->structural_joints()) {
        if (joint.active) {
            detail::SessionTestFacade::override_joint_ratio_after_solver(
                *session, joint.id, 0.0);
        }
    }
    NINHO_SIM_REQUIRE(session->tick().ok());
    const DomainEvent* broken = find_event(
        *session, DomainEventKind::JointBroken, trigger->joint_id);
    const DomainEvent* fractured = find_event(*session, DomainEventKind::PieceFractured);
    NINHO_SIM_REQUIRE(broken == nullptr && fractured != nullptr);
    NINHO_SIM_REQUIRE(fractured->cause_event_id == trigger->id);
}

NINHO_SIM_TEST("fracture objective piece chooses nearest incident joint then JointId")
{
    auto session = create_session();
    detail::SessionTestFacade::fracture_piece_at_incident_tie_after_solver(
        *session, EntityId{104}, PartId{1});
    for (const StructuralJointSnapshot& joint : session->structural_joints()) {
        detail::SessionTestFacade::override_joint_ratio_after_solver(
            *session, joint.id, 0.0);
    }
    NINHO_SIM_REQUIRE(session->tick().ok());
    NINHO_SIM_REQUIRE(find_event(*session, DomainEventKind::DamageApplied) == nullptr);
    NINHO_SIM_REQUIRE(find_event(*session, DomainEventKind::JointOverloaded) == nullptr);
    const DomainEvent* trigger = find_event(
        *session, DomainEventKind::PieceFractureTriggered, JointId{1});
    NINHO_SIM_REQUIRE(trigger != nullptr);
    NINHO_SIM_REQUIRE(trigger->fracture_ratio == 1.0);
    NINHO_SIM_REQUIRE(trigger->joint_load_ratio == 0.0);
    const EventId trigger_id = trigger->id;
    NINHO_SIM_REQUIRE(find_event(*session, DomainEventKind::PieceFractured) == nullptr);
    NINHO_SIM_REQUIRE(session->tick().ok());
    const DomainEvent* fractured = find_event(*session, DomainEventKind::PieceFractured);
    NINHO_SIM_REQUIRE(fractured != nullptr);
    NINHO_SIM_REQUIRE(fractured->affected_entity_id == EntityId{104});
    NINHO_SIM_REQUIRE(fractured->cause_event_id == trigger_id);
    NINHO_SIM_REQUIRE(fractured->joint_id == JointId{1});
    const DomainEvent* broken = find_event(*session, DomainEventKind::JointBroken, JointId{1});
    NINHO_SIM_REQUIRE(broken == nullptr);
    NINHO_SIM_REQUIRE(session->tick().ok());
    NINHO_SIM_REQUIRE(find_event(*session, DomainEventKind::PieceFractured) == nullptr);
}

NINHO_SIM_TEST("fracture objective keeps active joint endpoints represented beyond six radii")
{
    auto session = create_session();
    constexpr JointId joint_id{11};
    const auto joint = std::ranges::find(
        session->structural_joints(), joint_id, &StructuralJointSnapshot::id);
    NINHO_SIM_REQUIRE(joint != session->structural_joints().end() && joint->active);
    const auto endpoint = std::ranges::find_if(session->snapshots(), [&](const auto& value) {
        return value.entity_id == joint->a.entity_id && value.part_id == joint->a.part_id;
    });
    NINHO_SIM_REQUIRE(endpoint != session->snapshots().end());
    const JointEndpoint moved_identity{endpoint->entity_id, endpoint->part_id};
    const auto outward = ninho::physics::normalized_or_zero(endpoint->transform.position);
    NINHO_SIM_REQUIRE(detail::SessionTestFacade::impulse_entity(
        *session, endpoint->entity_id, outward * 200000.0f));

    bool crossed_six_r{};
    for (int tick = 0; tick < 120 && !crossed_six_r; ++tick) {
        for (const StructuralJointSnapshot& current : session->structural_joints()) {
            if (current.active) {
                detail::SessionTestFacade::override_joint_ratio_after_solver(
                    *session, current.id, 0.0);
            }
        }
        NINHO_SIM_REQUIRE(session->tick().ok());
        const auto moved = std::ranges::find_if(session->snapshots(), [&](const auto& value) {
            return value.entity_id == moved_identity.entity_id
                && value.part_id == moved_identity.part_id;
        });
        crossed_six_r = moved != session->snapshots().end()
            && ninho::physics::length(moved->transform.position) > 60.0f;
    }
    NINHO_SIM_REQUIRE(crossed_six_r);

    NINHO_SIM_REQUIRE(session->tick().ok());
    const auto current_joint = std::ranges::find(
        session->structural_joints(), joint_id, &StructuralJointSnapshot::id);
    NINHO_SIM_REQUIRE(current_joint != session->structural_joints().end());
    NINHO_SIM_REQUIRE(current_joint->active);
    for (const JointEndpoint identity : {current_joint->a, current_joint->b}) {
        NINHO_SIM_REQUIRE(std::ranges::any_of(
            session->snapshots(), [&](const EntitySnapshot& snapshot) {
                return snapshot.entity_id == identity.entity_id
                    && snapshot.part_id == identity.part_id;
            }));
    }
}

NINHO_SIM_TEST("fracture objective completion is monotonic and result only follows Evaluation")
{
    auto session = create_session();
    NINHO_SIM_REQUIRE(detail::SessionTestFacade::set_body_neutralized(
        *session, EntityId{200}, PartId{1}, true));
    NINHO_SIM_REQUIRE(session->state().phase == SessionPhase::Inspection);
    NINHO_SIM_REQUIRE(session->tick().ok());
    NINHO_SIM_REQUIRE(session->objectives_complete());
    NINHO_SIM_REQUIRE(session->state().phase == SessionPhase::Inspection);
    NINHO_SIM_REQUIRE(session->state().outcome == Outcome::None);
    NINHO_SIM_REQUIRE(detail::SessionTestFacade::set_body_neutralized(
        *session, EntityId{200}, PartId{1}, false));
    NINHO_SIM_REQUIRE(session->tick().ok());
    NINHO_SIM_REQUIRE(session->objectives_complete());
    NINHO_SIM_REQUIRE(session->state().outcome == Outcome::None);
}

}
