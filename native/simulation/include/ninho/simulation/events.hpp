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
};

enum class CommandRejectionReason : std::uint8_t {
    None,
    InvalidPhase,
    InvalidAim,
    NotArmed,
    NoBirdAvailable,
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

    bool operator==(const DomainEvent&) const = default;
};

}
