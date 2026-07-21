#include "damage_system.hpp"
#include "session_internal.hpp"

#include <algorithm>
#include <cmath>
#include <numbers>
#include <ranges>
#include <stdexcept>
#include <tuple>
#include <utility>

namespace ninho::simulation {
namespace detail {
namespace {

const MaterialDefinition* find_material(const MaterialCatalog& catalog, MaterialId id)
{
    const auto found = std::ranges::find(catalog.materials, id, &MaterialDefinition::id);
    return found == catalog.materials.end() ? nullptr : &*found;
}

const EnemyArchetype* find_enemy(const ArchetypeCatalog& catalog, EnemyArchetypeId id)
{
    const auto found = std::ranges::find(catalog.enemies, id, &EnemyArchetype::id);
    return found == catalog.enemies.end() ? nullptr : &*found;
}

const WeakpointProfile* find_weakpoint(const ArchetypeCatalog& catalog, WeakpointId id)
{
    const auto found = std::ranges::find(catalog.weakpoints, id, &WeakpointProfile::id);
    return found == catalog.weakpoints.end() ? nullptr : &*found;
}

ninho::physics::Vec3 rotate(
    ninho::physics::Quat rotation, ninho::physics::Vec3 value) noexcept
{
    const ninho::physics::Vec3 q{rotation.x, rotation.y, rotation.z};
    const ninho::physics::Vec3 twice_cross = 2.0f * ninho::physics::cross(q, value);
    return value + rotation.w * twice_cross + ninho::physics::cross(q, twice_cross);
}

const DamageBody* find_body(
    std::span<const DamageBody> bodies, EntityId entity, PartId part)
{
    const auto found = std::ranges::find_if(bodies, [&](const DamageBody& body) {
        return body.entity_id == entity && body.part_id == part;
    });
    return found == bodies.end() ? nullptr : &*found;
}

DamageState* find_enemy_state(std::vector<DamageState>& states, EntityId entity)
{
    const auto found = std::ranges::find_if(states, [&](const DamageState& current) {
        return current.entity_id == entity && current.enemy_archetype_id.has_value();
    });
    return found == states.end() ? nullptr : &*found;
}

void ensure_enemy_states(std::vector<DamageState>& states,
    const ArchetypeCatalog& archetypes, std::span<const DamageBody> bodies)
{
    for (const DamageBody& body : bodies) {
        if (!body.enemy_archetype_id || find_enemy_state(states, body.entity_id) != nullptr) {
            continue;
        }
        const EnemyArchetype* enemy = find_enemy(archetypes, *body.enemy_archetype_id);
        if (enemy != nullptr) {
            states.push_back({body.entity_id, body.part_id, std::nullopt,
                body.enemy_archetype_id, 0.0, enemy->integrity});
        }
    }
}

void apply_material_damage(std::vector<DamageState>& states,
    std::vector<DamageOutcome>& outcomes, const MaterialCatalog& materials,
    const DamageBody& target, EntityId cause_entity, PartId cause_part,
    ninho::physics::Vec3 position, ninho::physics::Vec3 cause_to_target, double energy,
    EventId cause_event_id = {})
{
    if (!target.material_id || energy <= 0.0) {
        return;
    }
    const MaterialDefinition* definition = find_material(materials, *target.material_id);
    if (definition == nullptr) {
        return;
    }
    auto found = std::ranges::find_if(states, [&](const DamageState& current) {
        return current.entity_id == target.entity_id && current.part_id == target.part_id;
    });
    if (found == states.end()) {
        states.push_back({target.entity_id, target.part_id, target.material_id});
        found = std::prev(states.end());
    }
    const double before = found->material_damage_energy_j;
    found->material_damage_energy_j = definition->response == MaterialResponse::Brittle
        ? std::max(before, energy) : before + energy;
    const double applied = found->material_damage_energy_j - before;
    if (applied > 0.0) {
        outcomes.push_back({DamageOutcomeKind::DamageApplied,
            cause_entity, cause_part, target.entity_id, target.part_id,
            position, cause_to_target, energy, applied,
            NeutralizationCause::None, DamageClassification::None, cause_event_id});
    }
}

struct DirectionalDamageResponse {
    double multiplier{};
    DamageClassification classification{DamageClassification::None};
};

DirectionalDamageResponse directional_response(const WeakpointProfile& weakpoint,
    const DamageBody& target, ninho::physics::Vec3 cause_to_target)
{
    const ninho::physics::Vec3 local_front{
        static_cast<float>(weakpoint.protected_direction[0]),
        static_cast<float>(weakpoint.protected_direction[1]),
        static_cast<float>(weakpoint.protected_direction[2])};
    const auto world_front = ninho::physics::normalized_or_zero(
        rotate(target.transform.rotation, local_front));
    const auto target_to_source = ninho::physics::normalized_or_zero(-cause_to_target);
    const double cone_cosine = std::cos(
        weakpoint.protected_cone_deg * std::numbers::pi / 180.0);
    const bool protected_hit = static_cast<double>(
        ninho::physics::dot(world_front, target_to_source)) >= cone_cosine;
    return protected_hit
        ? DirectionalDamageResponse{weakpoint.protected_multiplier,
            DamageClassification::Protected}
        : DirectionalDamageResponse{weakpoint.exposed_multiplier,
            DamageClassification::Vulnerable};
}

void apply_enemy_damage(std::vector<DamageState>& states,
    std::vector<DamageOutcome>& outcomes, const ArchetypeCatalog& archetypes,
    const DamageBody& target, EntityId cause_entity, PartId cause_part,
    ninho::physics::Vec3 position, ninho::physics::Vec3 cause_to_target, double energy,
    EventId cause_event_id = {})
{
    if (!target.enemy_archetype_id || energy <= 0.0) {
        return;
    }
    const EnemyArchetype* enemy = find_enemy(archetypes, *target.enemy_archetype_id);
    const WeakpointProfile* weakpoint = enemy == nullptr
        ? nullptr : find_weakpoint(archetypes, enemy->weakpoint_id);
    DamageState* state = find_enemy_state(states, target.entity_id);
    if (enemy == nullptr || weakpoint == nullptr || state == nullptr || state->neutralized) {
        return;
    }
    const double denominator = enemy->mass_kg * enemy->damage_energy_j_per_kg;
    const double uncapped = enemy->integrity * energy / denominator;
    const DirectionalDamageResponse response =
        directional_response(*weakpoint, target, cause_to_target);
    const double damage = std::min(enemy->max_damage,
        uncapped * response.multiplier);
    const double applied = std::min(state->remaining_integrity, damage);
    if (applied <= 0.0) {
        return;
    }
    state->remaining_integrity -= applied;
    outcomes.push_back({DamageOutcomeKind::DamageApplied,
        cause_entity, cause_part, target.entity_id, target.part_id,
        position, cause_to_target, energy, applied,
        NeutralizationCause::None, response.classification, cause_event_id});
    if (state->remaining_integrity <= 0.0) {
        state->remaining_integrity = 0.0;
        state->neutralized = true;
        outcomes.push_back({DamageOutcomeKind::EntityNeutralized,
            cause_entity, cause_part, target.entity_id, target.part_id,
            position, cause_to_target, energy, applied,
            NeutralizationCause::IntegrityDepleted, response.classification,
            cause_event_id});
    }
}

void apply_ejection_transitions(std::vector<DamageState>& states,
    std::vector<DamageOutcome>& outcomes, std::span<const DamageBody> bodies)
{
    for (const DamageBody& body : bodies) {
        DamageState* state = body.enemy_archetype_id
            ? find_enemy_state(states, body.entity_id) : nullptr;
        if (state == nullptr) {
            continue;
        }
        if (body.ejected && !state->was_ejected && !state->neutralized) {
            const auto outward = ninho::physics::normalized_or_zero(
                body.transform.position);
            const float radial_speed = std::max(0.0f,
                ninho::physics::dot(body.linear_velocity_m_s, outward));
            const double radial_energy = 0.5 * body.mass_kg
                * static_cast<double>(radial_speed) * radial_speed;
            state->remaining_integrity = 0.0;
            state->neutralized = true;
            outcomes.push_back({DamageOutcomeKind::EntityNeutralized,
                {}, {}, body.entity_id, body.part_id, body.transform.position,
                outward, radial_energy, 0.0, NeutralizationCause::Ejection});
        }
        state->was_ejected = body.ejected;
    }
}

}

}

void SimulationSession::Impl::process_damage_after_step()
{
    std::vector<detail::DamageBody> bodies;
    bodies.reserve(body_records.size());
    for (const BodyRecord& record : body_records) {
        const auto state = physics.state(record.physics_handle);
        if (!state) {
            continue;
        }
        bodies.push_back({record.entity_id, record.part_id, record.material_id,
            record.enemy_archetype_id, state->transform, state->mass,
            state->linear_velocity, state->ejected});
    }

    std::vector<detail::DamageContact> contacts;
    contacts.reserve(physics.contact_hits().size());
    for (const ninho::physics::ContactHit& hit : physics.contact_hits()) {
        const auto a = std::ranges::find(body_records, hit.a, &BodyRecord::physics_handle);
        const auto b = std::ranges::find(body_records, hit.b, &BodyRecord::physics_handle);
        if (a == body_records.end() || b == body_records.end()) {
            continue;
        }
        contacts.push_back({a->entity_id, a->part_id, b->entity_id, b->part_id,
            hit.point, hit.normal, hit.derived_energy});
    }

    const auto outcomes = damage_system.process(
        bundle.materials, bundle.archetypes, bodies, contacts,
        pending_external_damage);
    publish_damage_outcomes(outcomes);
    pending_external_damage.clear();
}

void SimulationSession::Impl::publish_damage_outcomes(
    std::span<const detail::DamageOutcome> outcomes)
{
    for (const detail::DamageOutcome& outcome : outcomes) {
        const DomainEventKind kind = outcome.kind == detail::DamageOutcomeKind::DamageApplied
            ? DomainEventKind::DamageApplied : DomainEventKind::EntityNeutralized;
        DomainEvent event{
            .id = EventId{next_event_sequence++},
            .tick = session_state.tick,
            .kind = kind,
            .entity_id = outcome.cause_entity_id,
            .affected_entity_id = outcome.target_entity_id,
            .affected_part_id = outcome.target_part_id,
            .part_id = outcome.cause_part_id,
            .position_m = outcome.position_m,
            .normal = outcome.normal_cause_to_target,
            .energy_j = outcome.energy_j,
            .damage = outcome.damage,
            .damage_classification = outcome.damage_classification,
            .neutralization_cause = outcome.neutralization_cause,
            .cause_event_id = outcome.cause_event_id,
        };
        domain_events.push_back(event);
        if (kind == DomainEventKind::EntityNeutralized) {
            for (BodyRecord& record : body_records) {
                if (record.entity_id == outcome.target_entity_id) {
                    record.neutralized = true;
                }
            }
        }
    }
}

namespace detail {

std::optional<DamageState> DamageSystem::state(EntityId entity, PartId part) const
{
    const auto found = std::ranges::find_if(states_, [&](const DamageState& current) {
        return current.entity_id == entity && current.part_id == part;
    });
    return found == states_.end() ? std::nullopt : std::optional{*found};
}

std::vector<DamageOutcome> DamageSystem::process(
    const MaterialCatalog& materials,
    const ArchetypeCatalog& archetypes,
    std::span<const DamageBody> bodies,
    std::span<const DamageContact> contacts,
    std::span<const ExternalDamage> external_damage)
{
    std::vector<DamageOutcome> outcomes;
    std::vector<DamageState> next_states = states_;
    ensure_enemy_states(next_states, archetypes, bodies);

    std::vector<ExternalDamage> ordered_external{
        external_damage.begin(), external_damage.end()};
    std::ranges::sort(ordered_external, [](const auto& lhs, const auto& rhs) {
        return std::tuple{lhs.cause_event_id, lhs.target_entity_id,
                   lhs.target_part_id}
            < std::tuple{rhs.cause_event_id, rhs.target_entity_id,
                   rhs.target_part_id};
    });
    for (std::size_t index = 1; index < ordered_external.size(); ++index) {
        const ExternalDamage& previous = ordered_external[index - 1U];
        const ExternalDamage& current = ordered_external[index];
        const bool same_key = previous.cause_event_id == current.cause_event_id
            && previous.target_entity_id == current.target_entity_id
            && previous.target_part_id == current.target_part_id;
        if (same_key && previous != current) {
            throw std::invalid_argument(
                "conflicting external damage for one burst and target");
        }
    }
    ordered_external.erase(std::unique(ordered_external.begin(), ordered_external.end()),
        ordered_external.end());
    for (const ExternalDamage& external : ordered_external) {
        if (!std::isfinite(external.energy_j) || external.energy_j <= 0.0
            || !ninho::physics::is_finite(external.position_m)
            || !ninho::physics::is_finite(external.normal_cause_to_target)) {
            throw std::invalid_argument("external damage must be positive and finite");
        }
        const DamageBody* target = find_body(
            bodies, external.target_entity_id, external.target_part_id);
        if (target == nullptr) {
            continue;
        }
        apply_material_damage(next_states, outcomes, materials, *target,
            external.cause_entity_id, external.cause_part_id,
            external.position_m, external.normal_cause_to_target,
            external.energy_j, external.cause_event_id);
        apply_enemy_damage(next_states, outcomes, archetypes, *target,
            external.cause_entity_id, external.cause_part_id,
            external.position_m, external.normal_cause_to_target,
            external.energy_j, external.cause_event_id);
    }
    for (const DamageContact& contact : contacts) {
        const DamageBody* a = find_body(bodies, contact.a_entity_id, contact.a_part_id);
        const DamageBody* b = find_body(bodies, contact.b_entity_id, contact.b_part_id);
        if (a == nullptr || b == nullptr) {
            continue;
        }
        apply_material_damage(next_states, outcomes, materials,
            *a, b->entity_id, b->part_id, contact.position_m,
            -contact.normal_a_to_b, contact.energy_j);
        apply_enemy_damage(next_states, outcomes, archetypes,
            *a, b->entity_id, b->part_id, contact.position_m,
            -contact.normal_a_to_b, contact.energy_j);
        apply_material_damage(next_states, outcomes, materials,
            *b, a->entity_id, a->part_id, contact.position_m,
            contact.normal_a_to_b, contact.energy_j);
        apply_enemy_damage(next_states, outcomes, archetypes,
            *b, a->entity_id, a->part_id, contact.position_m,
            contact.normal_a_to_b, contact.energy_j);
    }
    apply_ejection_transitions(next_states, outcomes, bodies);
    std::ranges::sort(next_states, [](const DamageState& lhs, const DamageState& rhs) {
        return std::pair{lhs.entity_id, lhs.part_id} < std::pair{rhs.entity_id, rhs.part_id};
    });
    states_.swap(next_states);
    return outcomes;
}

}

}
