#pragma once

#include "ability_runtime.hpp"
#include "ninho/simulation/content.hpp"

#include <ninho/physics/physics_types.hpp>

#include <algorithm>
#include <cstdint>
#include <vector>

namespace ninho::simulation {

struct LockedLaunchPlane {
    ninho::physics::Vec3 camera_right{};
    ninho::physics::Vec3 up{};
    ninho::physics::Vec3 horizontal{};
    ninho::physics::Vec3 plane_normal{};

    bool operator==(const LockedLaunchPlane&) const = default;
};

struct ProjectileState {
    EntityId entity_id;
    ninho::physics::BodyHandle physics_handle;
    bool bullet{};
    std::uint32_t age_ticks{};
    std::uint32_t rest_ticks{};
    bool finished{};
    bool pending_destroy{};

    bool operator==(const ProjectileState&) const = default;
};

struct ShotState {
    std::uint64_t shot_id{};
    BirdArchetypeId bird_archetype_id;
    AbilityId ability_id;
    TickIndex launch_tick;
    LockedLaunchPlane locked_plane;
    double pull_horizontal_m{};
    double pull_vertical_m{};
    bool activation_consumed{};
    AbilityRuntime runtime{GravityFieldAbilityRuntime{}};
    std::vector<ProjectileState> projectiles;

    bool operator==(const ShotState&) const = default;
};

inline void sort_projectiles(ShotState& shot)
{
    std::ranges::sort(shot.projectiles, {}, &ProjectileState::entity_id);
}

[[nodiscard]] inline ProjectileState* primary_projectile(ShotState& shot) noexcept
{
    return shot.projectiles.empty() ? nullptr : &shot.projectiles.front();
}

[[nodiscard]] inline const ProjectileState* primary_projectile(
    const ShotState& shot) noexcept
{
    return shot.projectiles.empty() ? nullptr : &shot.projectiles.front();
}

}
