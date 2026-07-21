#pragma once

#include "ninho/simulation/content.hpp"
#include "ninho/simulation/events.hpp"

#include <ninho/physics/physics_types.hpp>

#include <optional>
#include <span>
#include <vector>

namespace ninho::simulation::detail {

struct DamageBody {
    EntityId entity_id{};
    PartId part_id{};
    std::optional<MaterialId> material_id;
    std::optional<EnemyArchetypeId> enemy_archetype_id;
    ninho::physics::Transform transform;
    double mass_kg{};
    ninho::physics::Vec3 linear_velocity_m_s{};
    bool ejected{};
};

struct DamageContact {
    EntityId a_entity_id{};
    PartId a_part_id{};
    EntityId b_entity_id{};
    PartId b_part_id{};
    ninho::physics::Vec3 position_m{};
    ninho::physics::Vec3 normal_a_to_b{};
    double energy_j{};
};

struct ExternalDamage {
    EntityId cause_entity_id{};
    PartId cause_part_id{};
    EntityId target_entity_id{};
    PartId target_part_id{};
    ninho::physics::Vec3 position_m{};
    ninho::physics::Vec3 normal_cause_to_target{};
    double energy_j{};
    EventId cause_event_id{};
    bool operator==(const ExternalDamage&) const = default;
};

enum class DamageOutcomeKind : std::uint8_t {
    DamageApplied = 0, EntityNeutralized = 1};

struct DamageOutcome {
    DamageOutcomeKind kind{};
    EntityId cause_entity_id{};
    PartId cause_part_id{};
    EntityId target_entity_id{};
    PartId target_part_id{};
    ninho::physics::Vec3 position_m{};
    ninho::physics::Vec3 normal_cause_to_target{};
    double energy_j{};
    double damage{};
    NeutralizationCause neutralization_cause{NeutralizationCause::None};
    DamageClassification damage_classification{DamageClassification::None};
    EventId cause_event_id{};
};

struct DamageState {
    EntityId entity_id{};
    PartId part_id{};
    std::optional<MaterialId> material_id;
    std::optional<EnemyArchetypeId> enemy_archetype_id;
    double material_damage_energy_j{};
    double remaining_integrity{};
    bool was_ejected{};
    bool neutralized{};
};

class DamageSystem {
public:
    std::vector<DamageOutcome> process(const MaterialCatalog&, const ArchetypeCatalog&,
        std::span<const DamageBody>, std::span<const DamageContact>,
        std::span<const ExternalDamage> external_damage = {});
    [[nodiscard]] std::optional<DamageState> state(EntityId, PartId) const;
    [[nodiscard]] std::span<const DamageState> states() const noexcept { return states_; }

private:
    std::vector<DamageState> states_;
};

}
