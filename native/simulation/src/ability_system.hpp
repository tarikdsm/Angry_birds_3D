#pragma once

#include "ability_runtime.hpp"
#include "shot_state.hpp"
#include "ninho/simulation/session.hpp"

#include <optional>
#include <string_view>

namespace ninho::simulation::detail {

class AbilityLifecycleHooks {
public:
    virtual ~AbilityLifecycleHooks() = default;

    virtual SessionStatus apply_before_step(
        ShotState&, const GravityFieldAbilityDefinition&, GravityFieldAbilityRuntime&);
    virtual SessionStatus apply_before_step(
        ShotState&, const MassBoostAbilityDefinition&, MassBoostAbilityRuntime&);
    virtual SessionStatus apply_before_step(
        ShotState&, const SpeedBoostAbilityDefinition&, SpeedBoostAbilityRuntime&);
    virtual SessionStatus apply_before_step(
        ShotState&, const ExplosionAbilityDefinition&, ExplosionAbilityRuntime&);
    virtual SessionStatus apply_before_step(
        ShotState&, const SplitAbilityDefinition&, SplitAbilityRuntime&);

    virtual SessionStatus process_after_step(
        ShotState&, const GravityFieldAbilityDefinition&, GravityFieldAbilityRuntime&);
    virtual SessionStatus process_after_step(
        ShotState&, const MassBoostAbilityDefinition&, MassBoostAbilityRuntime&);
    virtual SessionStatus process_after_step(
        ShotState&, const SpeedBoostAbilityDefinition&, SpeedBoostAbilityRuntime&);
    virtual SessionStatus process_after_step(
        ShotState&, const ExplosionAbilityDefinition&, ExplosionAbilityRuntime&);
    virtual SessionStatus process_after_step(
        ShotState&, const SplitAbilityDefinition&, SplitAbilityRuntime&);

    virtual SessionStatus finish_after_step(
        ShotState&, const GravityFieldAbilityDefinition&, GravityFieldAbilityRuntime&);
    virtual SessionStatus finish_after_step(
        ShotState&, const MassBoostAbilityDefinition&, MassBoostAbilityRuntime&);
    virtual SessionStatus finish_after_step(
        ShotState&, const SpeedBoostAbilityDefinition&, SpeedBoostAbilityRuntime&);
    virtual SessionStatus finish_after_step(
        ShotState&, const ExplosionAbilityDefinition&, ExplosionAbilityRuntime&);
    virtual SessionStatus finish_after_step(
        ShotState&, const SplitAbilityDefinition&, SplitAbilityRuntime&);
};

class AbilitySystem {
public:
    explicit AbilitySystem(AbilityLifecycleHooks& hooks) noexcept
        : hooks_(hooks)
    {
    }

    [[nodiscard]] static std::optional<ContentError> validate_definition(
        const AbilityArchetype&, std::string_view pointer);
    [[nodiscard]] static std::optional<ContentError> validate_session_support(
        const AbilityArchetype&, std::string_view pointer);
    [[nodiscard]] static std::uint32_t activation_arm_ticks(
        const AbilityArchetype&) noexcept;
    [[nodiscard]] static SessionStatus activate(
        const AbilityArchetype&, ShotState&, TickIndex activation_tick);

    [[nodiscard]] SessionStatus apply_before_step(
        const AbilityArchetype&, ShotState&);
    [[nodiscard]] SessionStatus process_after_step(
        const AbilityArchetype&, ShotState&);
    [[nodiscard]] SessionStatus finish_after_step(
        const AbilityArchetype&, ShotState&, TickIndex current_tick);

private:
    AbilityLifecycleHooks& hooks_;
};

}
