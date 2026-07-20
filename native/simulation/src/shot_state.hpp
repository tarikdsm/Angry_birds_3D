#pragma once

#include "ability_runtime.hpp"
#include "ninho/simulation/content.hpp"

#include <ninho/physics/physics_types.hpp>

#include <algorithm>
#include <cstdint>
#include <span>
#include <utility>
#include <vector>

namespace ninho::simulation {

struct LockedLaunchPlane {
    ninho::physics::Vec3 camera_right{};
    ninho::physics::Vec3 up{};
    ninho::physics::Vec3 horizontal{};
    ninho::physics::Vec3 plane_normal{};

    bool operator==(const LockedLaunchPlane&) const = default;
};

class ProjectileState {
public:
    ProjectileState(EntityId entity_id,
        ninho::physics::BodyHandle physics_handle, bool bullet) noexcept
        : entity_id_(entity_id)
        , physics_handle(physics_handle)
        , bullet(bullet)
    {
    }

    ProjectileState(const ProjectileState&) = default;
    ProjectileState(ProjectileState&&) noexcept = default;

    ProjectileState& operator=(const ProjectileState& other) noexcept
    {
        if (this != &other) {
            assign_runtime(other);
        }
        return *this;
    }

    ProjectileState& operator=(ProjectileState&& other) noexcept
    {
        if (this != &other) {
            assign_runtime(other);
        }
        return *this;
    }

    [[nodiscard]] EntityId entity_id() const noexcept { return entity_id_; }

    [[nodiscard]] ProjectileState copy_with_entity_id(EntityId entity_id) const
    {
        ProjectileState copy = *this;
        copy.entity_id_ = entity_id;
        return copy;
    }

    ninho::physics::BodyHandle physics_handle;
    bool bullet{};
    std::uint32_t age_ticks{};
    std::uint32_t rest_ticks{};
    bool finished{};
    bool pending_destroy{};

    bool operator==(const ProjectileState&) const = default;

private:
    void assign_runtime(const ProjectileState& other) noexcept
    {
        physics_handle = other.physics_handle;
        bullet = other.bullet;
        age_ticks = other.age_ticks;
        rest_ticks = other.rest_ticks;
        finished = other.finished;
        pending_destroy = other.pending_destroy;
    }

    EntityId entity_id_;
};

struct ShotState {
    explicit ShotState(ProjectileState primary)
        : projectiles_{std::move(primary)}
    {
    }

    ShotState(const ShotState&) = default;
    ShotState(ShotState&&) = delete;
    ShotState& operator=(const ShotState&) = delete;
    ShotState& operator=(ShotState&&) = delete;

    std::uint64_t shot_id{};
    BirdArchetypeId bird_archetype_id;
    AbilityId ability_id;
    TickIndex launch_tick;
    LockedLaunchPlane locked_plane;
    double pull_horizontal_m{};
    double pull_vertical_m{};
    bool activation_consumed{};
    AbilityRuntime runtime{GravityFieldAbilityRuntime{}};

    [[nodiscard]] bool insert_projectile(ProjectileState projectile)
    {
        const auto position = std::ranges::lower_bound(projectiles_,
            projectile.entity_id(), {}, &ProjectileState::entity_id);
        if (position != projectiles_.end()
            && position->entity_id() == projectile.entity_id()) {
            return false;
        }
        std::vector<ProjectileState> normalized;
        normalized.reserve(projectiles_.size() + 1U);
        for (auto current = projectiles_.cbegin(); current != position; ++current) {
            normalized.push_back(*current);
        }
        normalized.push_back(std::move(projectile));
        for (auto current = position; current != projectiles_.cend(); ++current) {
            normalized.push_back(*current);
        }
        projectiles_.swap(normalized);
        return true;
    }

    [[nodiscard]] bool replace_projectile(ProjectileState projectile)
    {
        const auto position = std::ranges::lower_bound(projectiles_,
            projectile.entity_id(), {}, &ProjectileState::entity_id);
        if (position == projectiles_.end()
            || position->entity_id() != projectile.entity_id()) {
            return false;
        }
        *position = std::move(projectile);
        return true;
    }

    [[nodiscard]] bool replace_all_projectiles(
        std::vector<ProjectileState> replacements) noexcept
    {
        if (replacements.empty()) {
            return false;
        }
        for (std::size_t index = 1U; index < replacements.size(); ++index) {
            if (replacements[index - 1U].entity_id()
                >= replacements[index].entity_id()) {
                return false;
            }
        }
        projectiles_.swap(replacements);
        return true;
    }

    [[nodiscard]] bool erase_projectile(EntityId entity_id)
    {
        const auto position = std::ranges::lower_bound(
            projectiles_, entity_id, {}, &ProjectileState::entity_id);
        if (position == projectiles_.end() || position->entity_id() != entity_id
            || projectiles_.size() == 1U) {
            return false;
        }
        std::vector<ProjectileState> normalized;
        normalized.reserve(projectiles_.size() - 1U);
        for (const ProjectileState& projectile : projectiles_) {
            if (projectile.entity_id() != entity_id) {
                normalized.push_back(projectile);
            }
        }
        projectiles_.swap(normalized);
        return true;
    }

    [[nodiscard]] std::span<const ProjectileState> projectiles() const noexcept
    {
        return projectiles_;
    }

    [[nodiscard]] bool has_valid_projectiles() const noexcept
    {
        if (projectiles_.empty()) {
            return false;
        }
        for (std::size_t index = 1U; index < projectiles_.size(); ++index) {
            if (projectiles_[index - 1U].entity_id()
                >= projectiles_[index].entity_id()) {
                return false;
            }
        }
        return true;
    }

    [[nodiscard]] ProjectileState* primary_projectile() noexcept
    {
        return projectiles_.empty() ? nullptr : &projectiles_.front();
    }

    [[nodiscard]] const ProjectileState* primary_projectile() const noexcept
    {
        return projectiles_.empty() ? nullptr : &projectiles_.front();
    }

    bool operator==(const ShotState&) const = default;

private:
    std::vector<ProjectileState> projectiles_;
};

}
