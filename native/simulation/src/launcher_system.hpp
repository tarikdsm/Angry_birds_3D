#pragma once

#include "ninho/simulation/session.hpp"

#include <optional>

namespace ninho::simulation::detail {

struct LauncherProjectile {
    double mass_kg{};
    double speed_cap_m_s{};
};

struct LauncherSolution {
    bool launchable{};
    ninho::physics::Vec3 origin_m{};
    ninho::physics::Vec3 direction{};
    double speed_m_s{};
};

class LauncherSystem {
public:
    LauncherSystem(WorldDefinition, SlingshotDefinition);

    [[nodiscard]] ContentResult<LauncherState> begin_grab(
        ninho::physics::Vec3 camera_right, LauncherProjectile);
    [[nodiscard]] ContentResult<LauncherState> set_pull(
        double horizontal_m, double vertical_m);
    [[nodiscard]] ContentResult<LauncherSolution> solution() const;

    void cancel_grab() noexcept;
    void complete_release() noexcept;
    [[nodiscard]] const std::optional<LauncherState>& state() const noexcept;

private:
    void update_solved_state() noexcept;

    WorldDefinition world_;
    SlingshotDefinition definition_;
    LauncherProjectile projectile_;
    std::optional<LauncherState> state_;
    bool grabbed_{};
};

}
