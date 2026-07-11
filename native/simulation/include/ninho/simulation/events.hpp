#pragma once

#include "ninho/simulation/content.hpp"
#include <ninho/physics/physics_types.hpp>

namespace ninho::simulation {

enum class DomainEventKind : std::uint8_t {
    BirdLaunched,
    AbilityActivationRequested,
    CommandRejected,
    AbilityStarted,
    AbilityAffectedBody,
    AbilityPulse,
    AbilityEnded,
    DamageApplied,
    EntityNeutralized,
    JointOverloaded,
    JointBroken,
    PieceFractured,
};

enum class CommandRejectionReason : std::uint8_t {
    None,
    InvalidPhase,
    InvalidAim,
    NotArmed,
    NoBirdAvailable,
};

enum class NeutralizationCause : std::uint8_t {
    None,
    IntegrityDepleted,
    Ejection,
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
    NeutralizationCause neutralization_cause{NeutralizationCause::None};
    EventId cause_event_id{};
    JointId joint_id{};
    MaterialId material_id{};
    double joint_load_ratio{};

    bool operator==(const DomainEvent&) const = default;
};

}
