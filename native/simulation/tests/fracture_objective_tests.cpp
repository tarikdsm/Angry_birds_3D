#include "test_framework.hpp"

#include "ninho/simulation/session.hpp"
#include "session_internal.hpp"
#include "session_test_facade.hpp"

#include <algorithm>
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

    detail::SessionTestFacade::override_joint_ratio_without_new_cause_after_solver(
        *session, joint, 1.0);
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
