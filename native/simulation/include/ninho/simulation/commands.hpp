#pragma once

#include <cstdint>

namespace ninho::simulation {

enum class PlayerCommandKind : std::uint8_t {
    BeginAim,
    UpdateAim,
    Launch,
    ActivateAbility,
    Restart,
};

}
