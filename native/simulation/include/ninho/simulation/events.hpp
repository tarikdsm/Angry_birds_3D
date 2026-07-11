#pragma once

#include "ninho/simulation/content.hpp"

namespace ninho::simulation {

struct DomainEvent {
    EventId id{};
    TickIndex tick{};
};

}
