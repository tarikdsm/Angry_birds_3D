#pragma once

#include "ninho/simulation/content.hpp"
#include <ninho/physics/physics_types.hpp>

namespace ninho::simulation {

enum class DomainEventKind : std::uint8_t {
    BirdLaunched = 0,
    AbilityActivationRequested = 1,
    CommandRejected = 2,
    AbilityStarted = 3,
    AbilityAffectedBody = 4,
    AbilityPulse = 5,
    AbilityEnded = 6,
    DamageApplied = 7,
    EntityNeutralized = 8,
    JointOverloaded = 9,
    PieceFractureTriggered = 10,
    JointBroken = 11,
    PieceFractured = 12,
};

enum class CommandRejectionReason : std::uint8_t {
    None = 0,
    InvalidPhase = 1,
    InvalidAim = 2,
    NotArmed = 3,
    NoBirdAvailable = 4,
};

enum class NeutralizationCause : std::uint8_t {
    None = 0,
    IntegrityDepleted = 1,
    Ejection = 2,
};

enum class DamageClassification : std::uint8_t {
    None = 0,
    Protected = 1,
    Vulnerable = 2,
};

struct DomainEvent {
    EventId id{};
    TickIndex tick{};
    DomainEventKind kind{};
    EntityId entity_id{};
    BirdArchetypeId bird_archetype_id{};
    CommandRejectionReason rejection_reason{};
    AbilityId ability_id{};
    EntityId affected_entity_id{};
    PartId affected_part_id{};
    double weight{};
    ninho::physics::Vec3 force_n{};
    ninho::physics::Vec3 impulse_n_s{};
    PartId part_id{};
    ninho::physics::Vec3 position_m{};
    ninho::physics::Vec3 normal{};
    double energy_j{};
    double damage{};
    DamageClassification damage_classification{DamageClassification::None};
    NeutralizationCause neutralization_cause{NeutralizationCause::None};
    EventId cause_event_id{};
    JointId joint_id{};
    MaterialId material_id{};
    double joint_load_ratio{};
    double fracture_ratio{};

    bool operator==(const DomainEvent&) const = default;
};

}
