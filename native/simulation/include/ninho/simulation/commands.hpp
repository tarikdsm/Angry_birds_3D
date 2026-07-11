#pragma once

#include <ninho/physics/physics_types.hpp>

#include <variant>

namespace ninho::simulation {

struct AimState {
    ninho::physics::Vec3 origin_m{};
    ninho::physics::Vec3 tangent_direction{};
    double speed_m_s{10.5};

    bool operator==(const AimState&) const = default;
};

struct BeginAimCommand {
    bool operator==(const BeginAimCommand&) const = default;
};

struct SetAimCommand {
    AimState aim;
    bool operator==(const SetAimCommand&) const = default;
};

struct LaunchCommand {
    bool operator==(const LaunchCommand&) const = default;
};

struct ActivateAbilityCommand {
    bool operator==(const ActivateAbilityCommand&) const = default;
};

struct CancelAimCommand {
    bool operator==(const CancelAimCommand&) const = default;
};

using PlayerCommand = std::variant<
    BeginAimCommand, SetAimCommand, LaunchCommand, ActivateAbilityCommand, CancelAimCommand>;

}
