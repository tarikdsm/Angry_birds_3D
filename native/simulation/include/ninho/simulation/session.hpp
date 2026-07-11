#pragma once

#include "ninho/simulation/content.hpp"

#include <cstdint>

namespace ninho::simulation {

enum class SessionPhase : std::uint8_t {
    Inspection,
    Aim,
    FlightAbility,
    Resolution,
    Evaluation,
    Result,
    Faulted,
};

enum class Outcome : std::uint8_t {
    None,
    Victory,
    Defeat,
};

struct SessionState {
    TickIndex tick{};
    SessionPhase phase{SessionPhase::Inspection};
    Outcome outcome{Outcome::None};
};

}
