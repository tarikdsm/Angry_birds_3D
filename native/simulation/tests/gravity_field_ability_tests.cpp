#include "test_framework.hpp"

#include "session_test_facade.hpp"
#include "ninho/simulation/commands.hpp"
#include "ninho/simulation/session.hpp"

#include <algorithm>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string_view>
#include <vector>

namespace {

using namespace ninho::simulation;
using ninho::physics::Vec3;

std::string read_source_file(std::string_view relative_path)
{
    std::ifstream stream{std::filesystem::path{NINHO_SOURCE_DIR} / relative_path,
        std::ios::binary};
    NINHO_SIM_REQUIRE(stream.is_open());
    std::ostringstream content;
    content << stream.rdbuf();
    return content.str();
}

ContentBundle load_bundle()
{
    const auto materials = parse_material_catalog(
        read_source_file("game/data/materials/vertical_slice.materials.json"));
    const auto archetypes = parse_archetype_catalog(
        read_source_file("game/data/archetypes/vertical_slice.archetypes.json"));
    const auto level = parse_level_manifest(
        read_source_file("game/data/levels/first_orbit.level.json"));
    NINHO_SIM_REQUIRE(materials.ok() && archetypes.ok() && level.ok());
    const auto bundle = make_content_bundle(materials.value, archetypes.value, level.value);
    NINHO_SIM_REQUIRE(bundle.ok());
    return bundle.value;
}

std::unique_ptr<SimulationSession> create_session(const ContentBundle& bundle)
{
    auto created = SimulationSession::create(
        bundle.materials, bundle.archetypes, bundle.level);
    NINHO_SIM_REQUIRE(created.ok());
    return std::move(created.value);
}

std::unique_ptr<SimulationSession> create_session()
{
    return create_session(load_bundle());
}

void launch(SimulationSession& session)
{
    NINHO_SIM_REQUIRE(session.enqueue(BeginAimCommand{}).ok());
    NINHO_SIM_REQUIRE(session.tick().ok());
    NINHO_SIM_REQUIRE(session.enqueue(SetAimCommand{{
        {-13.0f, 0.0f, 0.0f}, {0.0f, 1.0f, 0.0f}, 10.5}}).ok());
    NINHO_SIM_REQUIRE(session.enqueue(LaunchCommand{}).ok());
    NINHO_SIM_REQUIRE(session.tick().ok());
}

const EntitySnapshot& snapshot(
    const SimulationSession& session, EntityId entity, PartId part = PartId{1})
{
    const auto found = std::ranges::find_if(session.snapshots(), [&](const auto& value) {
        return value.entity_id == entity && value.part_id == part;
    });
    NINHO_SIM_REQUIRE(found != session.snapshots().end());
    return *found;
}

EntitySnapshot projectile_snapshot(const SimulationSession& session)
{
    const auto found = std::ranges::find_if(session.snapshots(), [](const auto& value) {
        return (value.entity_id.value() & 0x80000000U) != 0U;
    });
    NINHO_SIM_REQUIRE(found != session.snapshots().end());
    return *found;
}

const DomainEvent& affected_event(const SimulationSession& session, EntityId entity)
{
    const auto found = std::ranges::find_if(session.events(), [&](const auto& event) {
        return event.kind == DomainEventKind::AbilityAffectedBody
            && event.affected_entity_id == entity;
    });
    NINHO_SIM_REQUIRE(found != session.events().end());
    return *found;
}

void require_vector_near(Vec3 actual, Vec3 expected, double tolerance)
{
    const double error = ninho::physics::length(actual - expected);
    if (error > tolerance) {
        std::ostringstream message;
        message << "vector error " << error << " exceeds " << tolerance
                << "; actual=(" << actual.x << ',' << actual.y << ',' << actual.z
                << ") expected=(" << expected.x << ',' << expected.y << ','
                << expected.z << ')';
        ninho::simulation::test::fail(__FILE__, __LINE__, message.str());
    }
}

void advance_to_tick(SimulationSession& session, TickIndex target)
{
    while (session.state().tick < target) {
        NINHO_SIM_REQUIRE(session.tick().ok());
    }
}

NINHO_SIM_TEST("gravity field ability production proxies are eligible by mass")
{
    auto session = create_session();
    for (const auto& value : session->snapshots()) {
        if (value.material_id == MaterialId{5} || value.material_id == MaterialId{9}) {
            NINHO_SIM_REQUIRE(value.mass_kg <= 150.0);
        }
    }
}

NINHO_SIM_TEST("gravity field ability arms at L plus 9 and runs exactly 75 ticks")
{
    auto session = create_session();
    launch(*session);
    const TickIndex launched = detail::SessionTestFacade::projectile_launch_tick(*session);
    advance_to_tick(*session, TickIndex{launched.value() + 8U});

    NINHO_SIM_REQUIRE(session->enqueue(ActivateAbilityCommand{}).ok());
    NINHO_SIM_REQUIRE(session->tick().ok());
    NINHO_SIM_REQUIRE(session->state().tick == TickIndex{launched.value() + 9U});
    NINHO_SIM_REQUIRE(detail::SessionTestFacade::ability_active(*session));
    NINHO_SIM_REQUIRE(session->events().front().kind == DomainEventKind::AbilityStarted);
    NINHO_SIM_REQUIRE(session->events().front().ability_id == AbilityId{1});

    for (std::uint32_t ability_tick = 2; ability_tick < 75; ++ability_tick) {
        NINHO_SIM_REQUIRE(session->tick().ok());
        NINHO_SIM_REQUIRE(detail::SessionTestFacade::ability_active(*session));
    }
    NINHO_SIM_REQUIRE(session->tick().ok());
    NINHO_SIM_REQUIRE(session->state().tick == TickIndex{launched.value() + 83U});
    NINHO_SIM_REQUIRE(!detail::SessionTestFacade::ability_active(*session));
    NINHO_SIM_REQUIRE(session->events().size() >= 2U);
    NINHO_SIM_REQUIRE(session->events()[session->events().size() - 2U].kind
        == DomainEventKind::AbilityPulse);
    NINHO_SIM_REQUIRE(session->events().back().kind == DomainEventKind::AbilityEnded);

    NINHO_SIM_REQUIRE(session->enqueue(ActivateAbilityCommand{}).ok());
    NINHO_SIM_REQUIRE(session->tick().ok());
    NINHO_SIM_REQUIRE(std::ranges::any_of(session->events(), [](const auto& event) {
        return event.kind == DomainEventKind::CommandRejected;
    }));
}

NINHO_SIM_TEST("gravity field ability follows projectile applies smoothstep force and final pulse")
{
    const ContentBundle active_bundle = load_bundle();
    ContentBundle control_bundle = active_bundle;
    control_bundle.archetypes.abilities.front().duration_ticks = 76U;
    auto active = create_session(active_bundle);
    auto control = create_session(control_bundle);
    launch(*active);
    launch(*control);
    const TickIndex launched = detail::SessionTestFacade::projectile_launch_tick(*active);
    advance_to_tick(*active, TickIndex{launched.value() + 7U});
    advance_to_tick(*control, TickIndex{launched.value() + 7U});
    const auto projectile = projectile_snapshot(*active);
    const auto center = projectile.transform.position;
    const EntityId candidate{900};
    const Vec3 candidate_position = center + Vec3{2.0f, 0.0f, 0.0f};
    NINHO_SIM_REQUIRE(detail::SessionTestFacade::add_dynamic_sphere(
        *active, candidate, PartId{3}, candidate_position, 100.0,
        projectile.linear_velocity_m_s));
    NINHO_SIM_REQUIRE(detail::SessionTestFacade::add_dynamic_sphere(
        *control, candidate, PartId{3}, candidate_position, 100.0,
        projectile.linear_velocity_m_s));
    NINHO_SIM_REQUIRE(active->tick().ok());
    NINHO_SIM_REQUIRE(control->tick().ok());

    NINHO_SIM_REQUIRE(active->enqueue(ActivateAbilityCommand{}).ok());
    NINHO_SIM_REQUIRE(control->enqueue(ActivateAbilityCommand{}).ok());
    const auto source_before_first = projectile_snapshot(*active);
    const auto candidate_before_first = snapshot(*active, candidate, PartId{3});
    NINHO_SIM_REQUIRE(active->tick().ok());
    NINHO_SIM_REQUIRE(control->tick().ok());
    const double weight = 0.5 * 0.5 * (3.0 - 2.0 * 0.5);
    const auto first_event = affected_event(*active, candidate);
    NINHO_SIM_REQUIRE(first_event.affected_part_id == PartId{3});
    NINHO_SIM_REQUIRE(std::abs(first_event.weight - weight) < 0.03);
    NINHO_SIM_REQUIRE(std::abs(ninho::physics::length(first_event.force_n) - 600.0) < 25.0);
    const auto expected_first_direction = ninho::physics::normalized_or_zero(
        source_before_first.transform.position - candidate_before_first.transform.position);
    NINHO_SIM_REQUIRE(ninho::physics::dot(
        ninho::physics::normalized_or_zero(first_event.force_n), expected_first_direction)
        > 0.9999f);

    const auto source_before_second = projectile_snapshot(*active);
    const auto candidate_before_second = snapshot(*active, candidate, PartId{3});
    NINHO_SIM_REQUIRE(active->tick().ok());
    NINHO_SIM_REQUIRE(control->tick().ok());
    const auto second_event = affected_event(*active, candidate);
    const auto expected_second_direction = ninho::physics::normalized_or_zero(
        source_before_second.transform.position - candidate_before_second.transform.position);
    NINHO_SIM_REQUIRE(ninho::physics::length(source_before_second.transform.position
        - source_before_first.transform.position) > 0.01f);
    NINHO_SIM_REQUIRE(ninho::physics::dot(
        ninho::physics::normalized_or_zero(second_event.force_n), expected_second_direction)
        > 0.9999f);

    for (std::uint32_t tick = 3; tick < 75; ++tick) {
        NINHO_SIM_REQUIRE(active->tick().ok());
        NINHO_SIM_REQUIRE(control->tick().ok());
    }
    const auto source_before_pulse = projectile_snapshot(*active);
    const auto candidate_before_pulse = snapshot(*active, candidate, PartId{3});
    NINHO_SIM_REQUIRE(active->tick().ok());
    NINHO_SIM_REQUIRE(control->tick().ok());
    const auto final_affected = affected_event(*active, candidate);
    const auto outward = ninho::physics::normalized_or_zero(
        candidate_before_pulse.transform.position - source_before_pulse.transform.position);
    const auto expected_velocity_pulse = outward
        * static_cast<float>(4.0 * final_affected.weight);
    require_vector_near(final_affected.impulse_n_s
            / static_cast<float>(candidate_before_pulse.mass_kg),
        expected_velocity_pulse, 1.0e-4);
    NINHO_SIM_REQUIRE(active->events()[active->events().size() - 2U].kind
        == DomainEventKind::AbilityPulse);
    NINHO_SIM_REQUIRE(active->events().back().kind == DomainEventKind::AbilityEnded);
    const auto pulse_delta = snapshot(*active, candidate, PartId{3}).linear_velocity_m_s
        - snapshot(*control, candidate, PartId{3}).linear_velocity_m_s;
    require_vector_near(pulse_delta, expected_velocity_pulse,
        ninho::physics::length(expected_velocity_pulse) * 0.02);
    NINHO_SIM_REQUIRE(ninho::physics::dot(
        ninho::physics::normalized_or_zero(pulse_delta), outward) > 0.9999f);
    NINHO_SIM_REQUIRE(!detail::SessionTestFacade::ability_active(*active));
    NINHO_SIM_REQUIRE(detail::SessionTestFacade::ability_active(*control));
}

NINHO_SIM_TEST("gravity field ability selects at most 20 eligible bodies in canonical identity order")
{
    auto session = create_session();
    launch(*session);
    const TickIndex launched = detail::SessionTestFacade::projectile_launch_tick(*session);
    advance_to_tick(*session, TickIndex{launched.value() + 7U});
    const auto projectile = projectile_snapshot(*session);
    const auto center = projectile.transform.position;
    for (std::uint32_t index = 0; index < 25U; ++index) {
        const EntityId entity{1000U + (24U - index)};
        const float angle = static_cast<float>(index) * 2.0f
            * 3.14159265358979323846f / 25.0f;
        NINHO_SIM_REQUIRE(detail::SessionTestFacade::add_dynamic_sphere(*session,
            entity, PartId{2}, center + Vec3{0.0f, 4.5f * std::sin(angle),
                4.5f * std::cos(angle)}, 10.0, projectile.linear_velocity_m_s, 0.55));
    }
    for (std::uint32_t index = 0; index < 23U; ++index) {
        const EntityId entity{2000U + (22U - index)};
        const float angle = static_cast<float>(index) * 2.0f
            * 3.14159265358979323846f / 23.0f;
        NINHO_SIM_REQUIRE(detail::SessionTestFacade::add_dynamic_sphere(*session,
            entity, PartId{4}, center + Vec3{0.0f, 2.0f * std::sin(angle),
                2.0f * std::cos(angle)}, 10.0, projectile.linear_velocity_m_s));
    }
    NINHO_SIM_REQUIRE(detail::SessionTestFacade::add_dynamic_sphere(*session,
        EntityId{1600}, PartId{1}, center + Vec3{1.0f, 0.0f, 0.0f}, 151.0,
        projectile.linear_velocity_m_s));
    NINHO_SIM_REQUIRE(detail::SessionTestFacade::add_dynamic_sphere(*session,
        EntityId{1500}, PartId{5}, center + Vec3{1.0f, 0.0f, 0.5f}, 10.0,
        projectile.linear_velocity_m_s));
    NINHO_SIM_REQUIRE(detail::SessionTestFacade::set_body_neutralized(
        *session, EntityId{1500}, PartId{5}, true));
    NINHO_SIM_REQUIRE(detail::SessionTestFacade::add_static_sphere(*session,
        EntityId{1700}, center + Vec3{1.0f, 0.0f, -0.5f}, 0.1));
    NINHO_SIM_REQUIRE(session->tick().ok());
    NINHO_SIM_REQUIRE(session->enqueue(ActivateAbilityCommand{}).ok());
    NINHO_SIM_REQUIRE(session->tick().ok());

    std::vector<std::pair<EntityId, PartId>> affected;
    for (const auto& event : session->events()) {
        if (event.kind == DomainEventKind::AbilityAffectedBody) {
            affected.emplace_back(event.affected_entity_id, event.affected_part_id);
        }
    }
    NINHO_SIM_REQUIRE(affected.size() == 20U);
    NINHO_SIM_REQUIRE(std::ranges::is_sorted(affected));
    const std::pair<EntityId, PartId> first_expected{EntityId{2000}, PartId{4}};
    const std::pair<EntityId, PartId> last_expected{EntityId{2019}, PartId{4}};
    NINHO_SIM_REQUIRE(affected.front() == first_expected);
    NINHO_SIM_REQUIRE(affected.back() == last_expected);
    const auto source = projectile_snapshot(*session).entity_id;
    NINHO_SIM_REQUIRE(std::ranges::none_of(affected, [&](const auto& identity) {
        return identity.first == source || identity.first == EntityId{1}
            || identity.first == EntityId{1500} || identity.first == EntityId{1600}
            || identity.first == EntityId{1700};
    }));
}

NINHO_SIM_TEST("gravity field ability remains active after projectile impact")
{
    auto session = create_session();
    launch(*session);
    const TickIndex launched = detail::SessionTestFacade::projectile_launch_tick(*session);
    advance_to_tick(*session, TickIndex{launched.value() + 8U});
    NINHO_SIM_REQUIRE(session->enqueue(ActivateAbilityCommand{}).ok());
    NINHO_SIM_REQUIRE(session->tick().ok());
    const auto before_impact = projectile_snapshot(*session);
    const auto flight_direction = ninho::physics::normalized_or_zero(
        before_impact.linear_velocity_m_s);
    NINHO_SIM_REQUIRE(detail::SessionTestFacade::add_static_sphere(*session,
        EntityId{950}, before_impact.transform.position + flight_direction * 0.8f, 0.5));
    NINHO_SIM_REQUIRE(session->tick().ok());
    NINHO_SIM_REQUIRE(session->state().last_impact_m.has_value());
    NINHO_SIM_REQUIRE(detail::SessionTestFacade::ability_active(*session));
    NINHO_SIM_REQUIRE(session->state().phase == SessionPhase::FlightAbility);
    NINHO_SIM_REQUIRE(std::ranges::none_of(session->events(), [](const auto& event) {
        return event.kind == DomainEventKind::AbilityAffectedBody
            && event.affected_entity_id == EntityId{950};
    }));
    for (std::uint32_t tick = 3; tick < 75; ++tick) {
        NINHO_SIM_REQUIRE(session->tick().ok());
        NINHO_SIM_REQUIRE(detail::SessionTestFacade::ability_active(*session));
        NINHO_SIM_REQUIRE(session->state().phase == SessionPhase::FlightAbility);
    }
    NINHO_SIM_REQUIRE(session->tick().ok());
    NINHO_SIM_REQUIRE(!detail::SessionTestFacade::ability_active(*session));
    NINHO_SIM_REQUIRE(session->state().phase == SessionPhase::Resolution);
}

NINHO_SIM_TEST("gravity field ability owns an ejected projectile through AbilityEnded")
{
    auto session = create_session();
    launch(*session);
    const TickIndex launched = detail::SessionTestFacade::projectile_launch_tick(*session);
    advance_to_tick(*session, TickIndex{launched.value() + 8U});
    NINHO_SIM_REQUIRE(session->enqueue(ActivateAbilityCommand{}).ok());
    NINHO_SIM_REQUIRE(session->tick().ok());
    const TickIndex ability_end = detail::SessionTestFacade::ability_end_tick(*session);
    const auto projectile = projectile_snapshot(*session);
    const auto outward = ninho::physics::normalized_or_zero(projectile.transform.position);
    NINHO_SIM_REQUIRE(detail::SessionTestFacade::impulse_entity(
        *session, projectile.entity_id, outward * 10000.0f));

    double maximum_radius{};
    bool ability_ended{};
    while (session->state().tick < ability_end) {
        NINHO_SIM_REQUIRE(session->tick().ok());
        const auto current = std::ranges::find_if(session->snapshots(), [&](const auto& value) {
            return value.entity_id == projectile.entity_id;
        });
        if (current != session->snapshots().end()) {
            maximum_radius = std::max(maximum_radius,
                static_cast<double>(ninho::physics::length(current->transform.position)));
        }
        ability_ended = ability_ended || std::ranges::any_of(
            session->events(), [](const auto& event) {
                return event.kind == DomainEventKind::AbilityEnded;
            });
    }

    NINHO_SIM_REQUIRE(maximum_radius > 60.0);
    NINHO_SIM_REQUIRE(ability_ended);
    NINHO_SIM_REQUIRE(!detail::SessionTestFacade::ability_active(*session));
    NINHO_SIM_REQUIRE(session->state().phase == SessionPhase::Resolution);
}

NINHO_SIM_TEST("gravity field ability rejects early second and expired activation")
{
    auto early = create_session();
    launch(*early);
    const TickIndex launched = detail::SessionTestFacade::projectile_launch_tick(*early);
    advance_to_tick(*early, TickIndex{launched.value() + 7U});
    NINHO_SIM_REQUIRE(early->enqueue(ActivateAbilityCommand{}).ok());
    NINHO_SIM_REQUIRE(early->tick().ok());
    NINHO_SIM_REQUIRE(early->events().size() == 1U);
    NINHO_SIM_REQUIRE(early->events().front().kind == DomainEventKind::CommandRejected);
    NINHO_SIM_REQUIRE(early->events().front().rejection_reason
        == CommandRejectionReason::NotArmed);
    NINHO_SIM_REQUIRE(early->enqueue(ActivateAbilityCommand{}).ok());
    NINHO_SIM_REQUIRE(early->tick().ok());
    NINHO_SIM_REQUIRE(detail::SessionTestFacade::ability_active(*early));
    NINHO_SIM_REQUIRE(early->enqueue(ActivateAbilityCommand{}).ok());
    NINHO_SIM_REQUIRE(early->tick().ok());
    NINHO_SIM_REQUIRE(std::ranges::any_of(early->events(), [](const auto& event) {
        return event.kind == DomainEventKind::CommandRejected;
    }));

    auto impacted = create_session();
    launch(*impacted);
    const auto before_impact = projectile_snapshot(*impacted);
    const auto flight_direction = ninho::physics::normalized_or_zero(
        before_impact.linear_velocity_m_s);
    NINHO_SIM_REQUIRE(detail::SessionTestFacade::add_static_sphere(*impacted,
        EntityId{951}, before_impact.transform.position + flight_direction * 0.8f, 0.5));
    NINHO_SIM_REQUIRE(impacted->tick().ok());
    NINHO_SIM_REQUIRE(impacted->state().last_impact_m.has_value());
    NINHO_SIM_REQUIRE(impacted->state().phase == SessionPhase::Resolution);
    NINHO_SIM_REQUIRE(impacted->enqueue(ActivateAbilityCommand{}).ok());
    NINHO_SIM_REQUIRE(impacted->tick().ok());
    NINHO_SIM_REQUIRE(!detail::SessionTestFacade::ability_active(*impacted));
    NINHO_SIM_REQUIRE(std::ranges::any_of(impacted->events(), [](const auto& event) {
        return event.kind == DomainEventKind::CommandRejected;
    }));

    auto timed_out = create_session();
    launch(*timed_out);
    detail::SessionTestFacade::age_projectile(*timed_out, 599U);
    NINHO_SIM_REQUIRE(timed_out->tick().ok());
    NINHO_SIM_REQUIRE(timed_out->state().phase == SessionPhase::Resolution);
    NINHO_SIM_REQUIRE(timed_out->enqueue(ActivateAbilityCommand{}).ok());
    NINHO_SIM_REQUIRE(timed_out->tick().ok());
    NINHO_SIM_REQUIRE(!detail::SessionTestFacade::ability_active(*timed_out));
    NINHO_SIM_REQUIRE(std::ranges::any_of(timed_out->events(), [](const auto& event) {
        return event.kind == DomainEventKind::CommandRejected;
    }));

    auto ejected = create_session();
    launch(*ejected);
    const auto projectile = projectile_snapshot(*ejected);
    const auto outward = ninho::physics::normalized_or_zero(projectile.transform.position);
    NINHO_SIM_REQUIRE(detail::SessionTestFacade::impulse_entity(
        *ejected, projectile.entity_id, outward * 2800.0f));
    std::uint32_t ticks_above_four_r{};
    double maximum_radius{};
    for (std::uint32_t tick = 0; tick < 150U
        && ejected->state().phase == SessionPhase::FlightAbility; ++tick) {
        NINHO_SIM_REQUIRE(ejected->tick().ok());
        const auto current = std::ranges::find_if(ejected->snapshots(), [&](const auto& value) {
            return value.entity_id == projectile.entity_id;
        });
        if (current != ejected->snapshots().end()) {
            const double radius = ninho::physics::length(current->transform.position);
            maximum_radius = std::max(maximum_radius, radius);
            if (radius >= 40.0) {
                ++ticks_above_four_r;
            }
        }
    }
    NINHO_SIM_REQUIRE(ejected->state().phase == SessionPhase::Resolution);
    NINHO_SIM_REQUIRE(!ejected->state().last_impact_m.has_value());
    NINHO_SIM_REQUIRE(maximum_radius >= 40.0 && maximum_radius < 60.0);
    NINHO_SIM_REQUIRE(ticks_above_four_r >= 29U);
    NINHO_SIM_REQUIRE(ejected->state().tick.value()
        - detail::SessionTestFacade::projectile_launch_tick(*ejected).value() < 150U);
    // Ejection is consumed by the FSM and destruction is scheduled before the
    // final public snapshot rebuild, so absence after Resolution is intentional.
    NINHO_SIM_REQUIRE(std::ranges::none_of(ejected->snapshots(), [&](const auto& value) {
        return value.entity_id == projectile.entity_id;
    }));
    NINHO_SIM_REQUIRE(!detail::SessionTestFacade::ability_active(*ejected));
}

struct DeterministicRun {
    std::vector<std::uint64_t> hashes;
    std::vector<DomainEvent> events;
};

DeterministicRun run_deterministic_scenario()
{
    auto session = create_session();
    launch(*session);
    const TickIndex launched = detail::SessionTestFacade::projectile_launch_tick(*session);
    advance_to_tick(*session, TickIndex{launched.value() + 8U});
    NINHO_SIM_REQUIRE(session->enqueue(ActivateAbilityCommand{}).ok());
    DeterministicRun result;
    for (std::uint32_t tick = 0; tick < 75; ++tick) {
        NINHO_SIM_REQUIRE(session->tick().ok());
        result.hashes.push_back(session->canonical_hash_v2());
        result.events.insert(
            result.events.end(), session->events().begin(), session->events().end());
    }
    return result;
}

NINHO_SIM_TEST("gravity field ability produces identical canonical hashes and events ten times")
{
    const auto reference = run_deterministic_scenario();
    for (int run = 1; run < 10; ++run) {
        const auto repeated = run_deterministic_scenario();
        NINHO_SIM_REQUIRE(repeated.hashes == reference.hashes);
        NINHO_SIM_REQUIRE(repeated.events == reference.events);
    }
}

}
