#pragma once

#include "ninho/simulation/content.hpp"

#include <ninho/physics/physics_types.hpp>

#include <optional>
#include <span>
#include <vector>

namespace ninho::simulation::detail {

enum class DuctileJointState : std::uint8_t { Elastic = 0, Yielded = 1, Broken = 2 };
enum class DuctileJointTransitionKind : std::uint8_t { Yielded = 0, Broken = 1 };

struct DuctileJointSample {
    JointId joint_id{};
    double force_n{};
    double torque_nm{};
    ninho::physics::Transform frame_a{};
    ninho::physics::Transform frame_b{};
    EventId cause_event_id{};
};

struct DuctileJointRuntime {
    JointId joint_id{};
    DuctileJointState state{DuctileJointState::Elastic};
    bool pending_recreate{};
    ninho::physics::Transform frame_a{};
    ninho::physics::Transform frame_b{};
    EventId cause_event_id{};
    TickIndex yielded_tick{};
    bool solver_ran_after_recreate{};

    bool operator==(const DuctileJointRuntime&) const = default;
};

struct DuctileJointTransition {
    JointId joint_id{};
    DuctileJointTransitionKind kind{};
    EventId cause_event_id{};
    double force_n{};
    double torque_nm{};
};

class DuctileJointSystem {
public:
    [[nodiscard]] std::vector<DuctileJointTransition> observe(
        TickIndex, std::span<const DuctileJointSample>);
    [[nodiscard]] std::vector<DuctileJointRuntime> recreate_due(TickIndex);
    [[nodiscard]] bool mark_recreated(JointId) noexcept;
    [[nodiscard]] std::optional<DuctileJointRuntime> state(JointId) const;
    [[nodiscard]] std::span<const DuctileJointRuntime> states() const noexcept {
        return states_;
    }

private:
    std::vector<DuctileJointRuntime> states_;
};

}
