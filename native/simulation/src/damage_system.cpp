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
    EventId cause_event_id = {}, double raw_contact_energy = -1.0)
{
    if (!target.enemy_archetype_id) {
        return;
    }
    const EnemyArchetype* enemy = find_enemy(archetypes, *target.enemy_archetype_id);
    const WeakpointProfile* weakpoint = enemy == nullptr
        ? nullptr : find_weakpoint(archetypes, enemy->weakpoint_id);
    DamageState* state = find_enemy_state(states, target.entity_id);
    if (enemy == nullptr || weakpoint == nullptr || state == nullptr || state->neutralized) {
        return;
    }
    const DirectionalDamageResponse response =
        directional_response(*weakpoint, target, cause_to_target);
    double damage = 0.0;
    double applied_energy = energy;
    if (enemy->damage_model == EnemyDamageModel::TerrestrialPig) {
        if (!std::isfinite(enemy->mass_kg) || enemy->mass_kg <= 0.0) {
            return;
        }
        const double selected_energy = raw_contact_energy >= 0.0
            ? raw_contact_energy : energy;
        if (!std::isfinite(selected_energy) || selected_energy <= 0.0) {
            return;
        }
        applied_energy = selected_energy;
        const double specific_energy = selected_energy / enemy->mass_kg;
        damage = std::clamp((specific_energy - 18.0) * 0.9,
            0.0, enemy->max_damage) * response.multiplier;
    } else {
        if (!std::isfinite(energy) || energy <= 0.0) {
            return;
        }
        const double denominator = enemy->mass_kg * enemy->damage_energy_j_per_kg;
        const double uncapped = enemy->integrity * energy / denominator;
        damage = std::min(enemy->max_damage,
            uncapped * response.multiplier);
    }
    const double applied = std::min(state->remaining_integrity, damage);
    if (applied <= 0.0) {
        return;
    }
    state->remaining_integrity -= applied;
    outcomes.push_back({DamageOutcomeKind::DamageApplied,
        cause_entity, cause_part, target.entity_id, target.part_id,
        position, cause_to_target, applied_energy, applied,
        NeutralizationCause::None, response.classification, cause_event_id});
    if (state->remaining_integrity <= 0.0) {
        state->remaining_integrity = 0.0;
        state->neutralized = true;
        outcomes.push_back({DamageOutcomeKind::EntityNeutralized,
            cause_entity, cause_part, target.entity_id, target.part_id,
            position, cause_to_target, applied_energy, applied,
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
        if (body.bounds_exit && !state->was_bounds_exit && !state->neutralized) {
            const auto outward = ninho::physics::normalized_or_zero(
                body.linear_velocity_m_s);
            const double kinetic_energy = 0.5 * body.mass_kg
                * static_cast<double>(ninho::physics::dot(
                    body.linear_velocity_m_s, body.linear_velocity_m_s));
            state->remaining_integrity = 0.0;
            state->neutralized = true;
            outcomes.push_back({DamageOutcomeKind::EntityNeutralized,
                {}, {}, body.entity_id, body.part_id, body.transform.position,
                outward, kinetic_energy, 0.0, NeutralizationCause::BoundsExit});
        } else if (body.ejected && !state->was_ejected && !state->neutralized) {
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
        state->was_bounds_exit = body.bounds_exit;
    }
}

}

}

void SimulationSession::Impl::process_damage_after_step()
{
    const bool uniform_world = bundle.level.source_schema_version == 2U
        && std::holds_alternative<UniformWorldDefinition>(bundle.level.world);
    const auto enemy_definition = [&](std::optional<EnemyArchetypeId> id)
        -> const EnemyArchetype* {
        if (!id) return nullptr;
        const auto found = std::ranges::find(
            bundle.archetypes.enemies, *id, &EnemyArchetype::id);
        return found == bundle.archetypes.enemies.end() ? nullptr : &*found;
    };
    std::vector<detail::DamageBody> bodies;
    bodies.reserve(body_records.size());
    for (const BodyRecord& record : body_records) {
        const auto state = physics.state(record.physics_handle);
        if (!state) {
            continue;
        }
        bodies.push_back({record.entity_id, record.part_id, record.material_id,
            record.enemy_archetype_id, state->transform, state->mass,
            state->linear_velocity, state->ejected,
            uniform_world && state->exited_world});
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
            hit.point, hit.normal, hit.derived_energy,
            hit.approach_speed, hit.effective_mass});
    }
    for (const ninho::physics::PhysicalContact& physical : physics.physical_contacts()) {
        const auto a = std::ranges::find(
            body_records, physical.a, &BodyRecord::physics_handle);
        const auto b = std::ranges::find(
            body_records, physical.b, &BodyRecord::physics_handle);
        if (a == body_records.end() || b == body_records.end()) {
            continue;
        }
        const auto found = std::ranges::find_if(contacts, [&](const auto& contact) {
            return contact.a_entity_id == a->entity_id
                && contact.a_part_id == a->part_id
                && contact.b_entity_id == b->entity_id
                && contact.b_part_id == b->part_id;
        });
        if (found == contacts.end()) {
            contacts.push_back({a->entity_id, a->part_id,
                b->entity_id, b->part_id, physical.point, physical.normal, 0.0,
                physical.approach_speed_m_s, physical.effective_mass_kg});
        } else {
            found->normal_speed_m_s = physical.approach_speed_m_s;
            found->effective_mass_kg = physical.effective_mass_kg;
        }
    }

    struct PigEntityAggregate {
        EntityId entity_id{};
        PartId representative_part_id{};
        const EnemyArchetype* definition{};
        double physical_mass_kg{};
        ninho::physics::Vec3 mass_weighted_center{};
        bool neutralized{};
    };
    std::vector<PigEntityAggregate> pig_entities;
    for (const BodyRecord& record : body_records) {
        const EnemyArchetype* enemy = enemy_definition(record.enemy_archetype_id);
        const auto state = physics.state(record.physics_handle);
        if (enemy == nullptr || enemy->damage_model != EnemyDamageModel::TerrestrialPig
            || !state) {
            continue;
        }
        auto aggregate = std::ranges::find(
            pig_entities, record.entity_id, &PigEntityAggregate::entity_id);
        if (aggregate == pig_entities.end()) {
            pig_entities.push_back({record.entity_id, record.part_id, enemy});
            aggregate = std::prev(pig_entities.end());
        }
        aggregate->representative_part_id = std::min(
            aggregate->representative_part_id, record.part_id);
        aggregate->physical_mass_kg += state->mass;
        aggregate->mass_weighted_center = aggregate->mass_weighted_center
            + state->world_center_of_mass * state->mass;
        aggregate->neutralized = aggregate->neutralized || record.neutralized;
    }
    std::ranges::sort(pig_entities, {}, &PigEntityAggregate::entity_id);
    std::vector<detail::CrushBody> crush_bodies;
    crush_bodies.reserve(pig_entities.size());
    for (const auto& pig : pig_entities) {
        if (!std::isfinite(pig.physical_mass_kg) || pig.physical_mass_kg <= 0.0) {
            continue;
        }
        const auto center = pig.mass_weighted_center
            / static_cast<float>(pig.physical_mass_kg);
        const auto gravity = physics.gravity_at(center);
        crush_bodies.push_back({pig.entity_id, pig.representative_part_id,
            pig.definition->mass_kg, ninho::physics::length(gravity),
            pig.neutralized, center,
            ninho::physics::normalized_or_zero(-gravity)});
    }
    const auto pig_representative_part = [&](EntityId entity)
        -> std::optional<PartId> {
        const auto found = std::ranges::find(
            pig_entities, entity, &PigEntityAggregate::entity_id);
        return found == pig_entities.end()
            ? std::nullopt : std::optional{found->representative_part_id};
    };
    std::vector<detail::CrushContactLoad> crush_loads;
    for (const auto& physical : physics.physical_contacts()) {
        for (const auto handle : {physical.a, physical.b}) {
            const auto record = std::ranges::find(
                body_records, handle, &BodyRecord::physics_handle);
            if (record == body_records.end()) continue;
            const EnemyArchetype* enemy = enemy_definition(record->enemy_archetype_id);
            if (enemy != nullptr
                && enemy->damage_model == EnemyDamageModel::TerrestrialPig) {
                const auto representative = pig_representative_part(record->entity_id);
                if (representative) {
                    crush_loads.push_back({record->entity_id, *representative,
                        physical.total_normal_impulse_n_s});
                }
            }
        }
    }
#if defined(NINHO_ENABLE_TEST_FACADES)
    for (const auto& load : crush_load_overrides_for_testing) {
        const auto representative = pig_representative_part(load.entity_id);
        if (representative) {
            crush_loads.push_back({load.entity_id, *representative,
                load.total_normal_impulse_n_s});
        }
    }
    crush_load_overrides_for_testing.clear();
#endif
    const auto crush_plans = crush_damage_system.update(session_state.tick,
        physics.config().time_step, crush_bodies, crush_loads);
    for (const auto& plan : crush_plans) {
        const EventId cause{next_event_sequence++};
        domain_events.push_back({
            .id = cause,
            .tick = session_state.tick,
            .kind = DomainEventKind::CrushDamageApplied,
            .affected_entity_id = plan.target_entity_id,
            .affected_part_id = plan.target_part_id,
            .position_m = plan.position_m,
            .normal = plan.normal,
            .energy_j = plan.external_energy_j,
            .damage = plan.predicted_damage,
        });
        if (!crush_damage_system.record_cause(
                plan.target_entity_id, plan.target_part_id, cause)) {
            throw std::runtime_error("crush cause could not be committed");
        }
        pending_external_damage.push_back({{}, {}, plan.target_entity_id,
            plan.target_part_id, plan.position_m, plan.normal,
            plan.external_energy_j, cause});
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

bool DamageSystem::erase_state(EntityId entity, PartId part) noexcept
{
    const auto before = states_.size();
    std::erase_if(states_, [&](const DamageState& current) {
        return current.entity_id == entity && current.part_id == part;
    });
    return states_.size() != before;
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
        const auto receipt_for_target = [&]() -> DamageState* {
            if (target->enemy_archetype_id) {
                return find_enemy_state(next_states, target->entity_id);
            }
            const auto found = std::ranges::find_if(next_states,
                [&](const DamageState& current) {
                    return current.entity_id == target->entity_id
                        && current.part_id == target->part_id;
                });
            return found == next_states.end() ? nullptr : &*found;
        };
        if (external.cause_event_id != EventId{}) {
            const DamageState* receipt = receipt_for_target();
            // EventIds are session-global and monotonic. A per-target watermark is
            // therefore an exact, deterministic and body-capacity-bounded ledger.
            if (receipt != nullptr
                && external.cause_event_id <= receipt->last_external_damage_event_id) {
                continue;
            }
        }
        apply_material_damage(next_states, outcomes, materials, *target,
            external.cause_entity_id, external.cause_part_id,
            external.position_m, external.normal_cause_to_target,
            external.energy_j, external.cause_event_id);
        apply_enemy_damage(next_states, outcomes, archetypes, *target,
            external.cause_entity_id, external.cause_part_id,
            external.position_m, external.normal_cause_to_target,
            external.energy_j, external.cause_event_id);
        if (external.cause_event_id != EventId{}) {
            if (DamageState* receipt = receipt_for_target()) {
                receipt->last_external_damage_event_id = external.cause_event_id;
            }
        }
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
            -contact.normal_a_to_b, contact.energy_j, {},
            0.5 * contact.effective_mass_kg * contact.normal_speed_m_s
                * contact.normal_speed_m_s);
        apply_material_damage(next_states, outcomes, materials,
            *b, a->entity_id, a->part_id, contact.position_m,
            contact.normal_a_to_b, contact.energy_j);
        apply_enemy_damage(next_states, outcomes, archetypes,
            *b, a->entity_id, a->part_id, contact.position_m,
            contact.normal_a_to_b, contact.energy_j, {},
            0.5 * contact.effective_mass_kg * contact.normal_speed_m_s
                * contact.normal_speed_m_s);
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
