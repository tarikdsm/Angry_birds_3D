#include "test_framework.hpp"

#include "ninho/simulation/session.hpp"
#include "session_internal.hpp"
#include "session_test_facade.hpp"

#include <algorithm>
#include <filesystem>
#include <fstream>
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

const DomainEvent* find_event(
    const SimulationSession& session, DomainEventKind kind, JointId joint = {})
{
    const auto found = std::ranges::find_if(session.events(), [&](const DomainEvent& event) {
        return event.kind == kind && (joint == JointId{} || event.joint_id == joint);
    });
    return found == session.events().end() ? nullptr : &*found;
}

NINHO_SIM_TEST("fracture objective overload schedules after solver and applies next tick")
{
    auto session = create_session();
    constexpr JointId joint{1};
    detail::SessionTestFacade::override_joint_ratio_after_solver(*session, joint, 1.5);

    NINHO_SIM_REQUIRE(session->tick().ok());
    const DomainEvent* cause = find_event(*session, DomainEventKind::DamageApplied);
    NINHO_SIM_REQUIRE(cause != nullptr);
    const EventId cause_id = cause->id;
    NINHO_SIM_REQUIRE(std::ranges::find(session->structural_joints(), joint,
        &StructuralJointSnapshot::id)->active);
    NINHO_SIM_REQUIRE(find_event(*session, DomainEventKind::JointBroken, joint) == nullptr);

    NINHO_SIM_REQUIRE(session->tick().ok());
    const auto broken = std::ranges::find(session->structural_joints(), joint,
        &StructuralJointSnapshot::id);
    NINHO_SIM_REQUIRE(broken != session->structural_joints().end() && !broken->active);
    const DomainEvent* event = find_event(*session, DomainEventKind::JointBroken, joint);
    NINHO_SIM_REQUIRE(event != nullptr);
    NINHO_SIM_REQUIRE(event->cause_event_id == cause_id);

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

NINHO_SIM_TEST("fracture objective consecutive overload preserves first tick cause")
{
    auto session = create_session();
    constexpr JointId joint{3};
    detail::SessionTestFacade::override_joint_ratio_after_solver(*session, joint, 1.0);
    NINHO_SIM_REQUIRE(session->tick().ok());
    const DomainEvent* cause = find_event(*session, DomainEventKind::DamageApplied);
    NINHO_SIM_REQUIRE(cause != nullptr);
    const EventId cause_id = cause->id;

    detail::SessionTestFacade::override_joint_ratio_without_new_cause_after_solver(
        *session, joint, 1.0);
    NINHO_SIM_REQUIRE(session->tick().ok());
    NINHO_SIM_REQUIRE(find_event(*session, DomainEventKind::DamageApplied) == nullptr);
    NINHO_SIM_REQUIRE(session->tick().ok());
    const DomainEvent* broken = find_event(*session, DomainEventKind::JointBroken, joint);
    NINHO_SIM_REQUIRE(broken != nullptr);
    NINHO_SIM_REQUIRE(broken->cause_event_id == cause_id);
}

NINHO_SIM_TEST("fracture objective piece chooses nearest incident joint then JointId")
{
    auto session = create_session();
    detail::SessionTestFacade::fracture_piece_at_incident_tie_after_solver(
        *session, EntityId{104}, PartId{1});
    NINHO_SIM_REQUIRE(session->tick().ok());
    const DomainEvent* cause = find_event(*session, DomainEventKind::DamageApplied);
    NINHO_SIM_REQUIRE(cause != nullptr);
    const EventId cause_id = cause->id;
    NINHO_SIM_REQUIRE(find_event(*session, DomainEventKind::PieceFractured) == nullptr);
    NINHO_SIM_REQUIRE(session->tick().ok());
    const DomainEvent* fractured = find_event(*session, DomainEventKind::PieceFractured);
    NINHO_SIM_REQUIRE(fractured != nullptr);
    NINHO_SIM_REQUIRE(fractured->affected_entity_id == EntityId{104});
    NINHO_SIM_REQUIRE(fractured->cause_event_id == cause_id);
    NINHO_SIM_REQUIRE(fractured->joint_id == JointId{1});
    const DomainEvent* broken = find_event(*session, DomainEventKind::JointBroken, JointId{1});
    NINHO_SIM_REQUIRE(broken != nullptr);
    NINHO_SIM_REQUIRE(broken->cause_event_id == fractured->cause_event_id);
    NINHO_SIM_REQUIRE(session->tick().ok());
    NINHO_SIM_REQUIRE(find_event(*session, DomainEventKind::PieceFractured) == nullptr);
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
