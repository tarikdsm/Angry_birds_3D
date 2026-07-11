#pragma once

#include "ninho/simulation/content.hpp"

namespace ninho::simulation {

enum class DomainEventKind : std::uint8_t {
    BirdLaunched,
    AbilityActivationRequested,
    CommandRejected,
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
};

}
