#include "ability_system.hpp"
#include "session_internal.hpp"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <string>
#include <type_traits>
#include <utility>

namespace ninho::simulation::detail {
namespace {

[[nodiscard]] SessionStatus dispatch_failure(std::string message)
{
    return {{ContentErrorCode::InternalError, "/ability", std::move(message)}};
}

[[nodiscard]] SessionStatus unsupported_failure()
{
    return {{ContentErrorCode::InvalidInvariant, "/ability/kind",
        "ability kind has no concrete simulation system"}};
}

[[nodiscard]] bool has_concrete_system(AbilityKind kind) noexcept
{
    switch (kind) {
    case AbilityKind::LegacyGravityField:
    case AbilityKind::GravityField:
    case AbilityKind::MassBoost:
        return true;
    case AbilityKind::SpeedBoost:
    case AbilityKind::Explosion:
    case AbilityKind::Split:
        return false;
    }
    return false;
}

[[nodiscard]] const char* kind_name(AbilityKind kind) noexcept
{
    switch (kind) {
    case AbilityKind::LegacyGravityField:
    case AbilityKind::GravityField:
        return "gravity_field";
    case AbilityKind::MassBoost:
        return "mass_boost";
    case AbilityKind::SpeedBoost:
        return "speed_boost";
    case AbilityKind::Explosion:
        return "explosion";
    case AbilityKind::Split:
        return "split";
    }
    return nullptr;
}

[[nodiscard]] bool payload_matches(
    AbilityKind kind, const AbilityArchetype::Payload& payload) noexcept
{
    switch (kind) {
    case AbilityKind::LegacyGravityField:
    case AbilityKind::GravityField:
        return std::holds_alternative<GravityFieldAbilityDefinition>(payload);
    case AbilityKind::MassBoost:
        return std::holds_alternative<MassBoostAbilityDefinition>(payload);
    case AbilityKind::SpeedBoost:
        return std::holds_alternative<SpeedBoostAbilityDefinition>(payload);
    case AbilityKind::Explosion:
        return std::holds_alternative<ExplosionAbilityDefinition>(payload);
    case AbilityKind::Split:
        return std::holds_alternative<SplitAbilityDefinition>(payload);
    }
    return false;
}

[[nodiscard]] bool runtime_matches(
    AbilityKind kind, const AbilityRuntime& runtime) noexcept
{
    switch (kind) {
    case AbilityKind::LegacyGravityField:
    case AbilityKind::GravityField:
        return std::holds_alternative<GravityFieldAbilityRuntime>(runtime);
    case AbilityKind::MassBoost:
        return std::holds_alternative<MassBoostAbilityRuntime>(runtime);
    case AbilityKind::SpeedBoost:
        return std::holds_alternative<SpeedBoostAbilityRuntime>(runtime);
    case AbilityKind::Explosion:
        return std::holds_alternative<ExplosionAbilityRuntime>(runtime);
    case AbilityKind::Split:
        return std::holds_alternative<SplitAbilityRuntime>(runtime);
    }
    return false;
}

[[nodiscard]] GravityFieldAbilityDefinition gravity_definition(
    const AbilityArchetype& ability)
{
    if (ability.kind_v2 == AbilityKind::LegacyGravityField) {
        return {ability.arm_ticks, ability.duration_ticks, ability.radius_m,
            ability.max_body_mass_kg, ability.max_bodies,
            ability.max_acceleration_m_s2, ability.pulse_speed_m_s};
    }
    return std::get<GravityFieldAbilityDefinition>(ability.payload);
}

template <typename Operation>
SessionStatus dispatch(const AbilityArchetype& ability, ShotState& shot,
    AbilityLifecycleHooks& hooks, Operation operation)
{
    if (!runtime_matches(ability.kind_v2, shot.runtime)) {
        return dispatch_failure("ability runtime does not match its kind");
    }
    switch (ability.kind_v2) {
    case AbilityKind::LegacyGravityField:
    case AbilityKind::GravityField: {
        const GravityFieldAbilityDefinition definition = gravity_definition(ability);
        return operation(hooks, shot, definition,
            std::get<GravityFieldAbilityRuntime>(shot.runtime));
    }
    case AbilityKind::MassBoost:
        return operation(hooks, shot,
            std::get<MassBoostAbilityDefinition>(ability.payload),
            std::get<MassBoostAbilityRuntime>(shot.runtime));
    case AbilityKind::SpeedBoost:
        return operation(hooks, shot,
            std::get<SpeedBoostAbilityDefinition>(ability.payload),
            std::get<SpeedBoostAbilityRuntime>(shot.runtime));
    case AbilityKind::Explosion:
        return operation(hooks, shot,
            std::get<ExplosionAbilityDefinition>(ability.payload),
            std::get<ExplosionAbilityRuntime>(shot.runtime));
    case AbilityKind::Split:
        return operation(hooks, shot,
            std::get<SplitAbilityDefinition>(ability.payload),
            std::get<SplitAbilityRuntime>(shot.runtime));
    }
    return dispatch_failure("ability kind is unknown");
}

[[nodiscard]] std::uint32_t duration_ticks(const AbilityArchetype& ability)
{
    switch (ability.kind_v2) {
    case AbilityKind::LegacyGravityField:
        return ability.duration_ticks;
    case AbilityKind::GravityField:
        return std::get<GravityFieldAbilityDefinition>(ability.payload).duration_ticks;
    case AbilityKind::MassBoost:
        return std::get<MassBoostAbilityDefinition>(ability.payload).duration_ticks;
    case AbilityKind::SpeedBoost:
    case AbilityKind::Explosion:
    case AbilityKind::Split:
        return 1U;
    }
    return 0U;
}

}

#define NINHO_NOOP_HOOK(name, definition, runtime) \
    SessionStatus AbilityLifecycleHooks::name(ShotState&, \
        const definition&, runtime&) { return {}; }

NINHO_NOOP_HOOK(apply_before_step, GravityFieldAbilityDefinition,
    GravityFieldAbilityRuntime)
NINHO_NOOP_HOOK(apply_before_step, MassBoostAbilityDefinition,
    MassBoostAbilityRuntime)
NINHO_NOOP_HOOK(apply_before_step, SpeedBoostAbilityDefinition,
    SpeedBoostAbilityRuntime)
NINHO_NOOP_HOOK(apply_before_step, ExplosionAbilityDefinition,
    ExplosionAbilityRuntime)
NINHO_NOOP_HOOK(apply_before_step, SplitAbilityDefinition,
    SplitAbilityRuntime)
NINHO_NOOP_HOOK(process_after_step, GravityFieldAbilityDefinition,
    GravityFieldAbilityRuntime)
NINHO_NOOP_HOOK(process_after_step, MassBoostAbilityDefinition,
    MassBoostAbilityRuntime)
NINHO_NOOP_HOOK(process_after_step, SpeedBoostAbilityDefinition,
    SpeedBoostAbilityRuntime)
NINHO_NOOP_HOOK(process_after_step, ExplosionAbilityDefinition,
    ExplosionAbilityRuntime)
NINHO_NOOP_HOOK(process_after_step, SplitAbilityDefinition,
    SplitAbilityRuntime)
NINHO_NOOP_HOOK(finish_after_step, GravityFieldAbilityDefinition,
    GravityFieldAbilityRuntime)
NINHO_NOOP_HOOK(finish_after_step, MassBoostAbilityDefinition,
    MassBoostAbilityRuntime)
NINHO_NOOP_HOOK(finish_after_step, SpeedBoostAbilityDefinition,
    SpeedBoostAbilityRuntime)
NINHO_NOOP_HOOK(finish_after_step, ExplosionAbilityDefinition,
    ExplosionAbilityRuntime)
NINHO_NOOP_HOOK(finish_after_step, SplitAbilityDefinition,
    SplitAbilityRuntime)

#undef NINHO_NOOP_HOOK

std::optional<ContentError> AbilitySystem::validate_definition(
    const AbilityArchetype& ability, std::string_view pointer)
{
    const char* expected_name = kind_name(ability.kind_v2);
    if (expected_name == nullptr || ability.kind_v2 == AbilityKind::LegacyGravityField) {
        return ContentError{ContentErrorCode::InvalidEnum,
            std::string{pointer} + "/kind", "invalid product ability kind"};
    }
    if (ability.kind != expected_name) {
        return ContentError{ContentErrorCode::InvalidInvariant,
            std::string{pointer} + "/kind", "ability kind tag is inconsistent"};
    }
    if (!payload_matches(ability.kind_v2, ability.payload)) {
        return ContentError{ContentErrorCode::InvalidInvariant,
            std::string{pointer} + "/payload", "ability payload does not match its kind"};
    }
    if (ability.kind_v2 == AbilityKind::MassBoost) {
        const auto& definition =
            std::get<MassBoostAbilityDefinition>(ability.payload);
        const std::string payload_pointer = std::string{pointer} + "/payload";
        if (definition.duration_ticks < 1U
            || definition.duration_ticks > 3600U) {
            return ContentError{ContentErrorCode::OutOfRange,
                payload_pointer + "/duration_ticks", "integer out of range"};
        }
        if (!std::isfinite(definition.mass_multiplier)) {
            return ContentError{ContentErrorCode::InvalidNumber,
                payload_pointer + "/mass_multiplier", "number must be finite"};
        }
        if (definition.mass_multiplier < 1.0
            || definition.mass_multiplier > 20.0) {
            return ContentError{ContentErrorCode::OutOfRange,
                payload_pointer + "/mass_multiplier", "number out of range"};
        }
    }
    return std::nullopt;
}

std::optional<ContentError> AbilitySystem::validate_session_support(
    const AbilityArchetype& ability, std::string_view pointer)
{
    if (has_concrete_system(ability.kind_v2)) {
        return std::nullopt;
    }
    return ContentError{ContentErrorCode::InvalidInvariant,
        std::string{pointer} + "/kind",
        "ability kind has no concrete simulation system"};
}

std::uint32_t AbilitySystem::activation_arm_ticks(
    const AbilityArchetype& ability) noexcept
{
    return ability.kind_v2 == AbilityKind::MassBoost ? 9U : ability.arm_ticks;
}

SessionStatus AbilitySystem::activate(const AbilityArchetype& ability,
    ShotState& shot, TickIndex activation_tick)
{
    if (!has_concrete_system(ability.kind_v2)) {
        return unsupported_failure();
    }
    if (!payload_matches(ability.kind_v2, ability.payload)
        || !runtime_matches(ability.kind_v2, shot.runtime)) {
        return dispatch_failure("ability definition or runtime is incompatible");
    }
    const std::uint32_t duration = duration_ticks(ability);
    if (duration == 0U) {
        return dispatch_failure("ability duration is unavailable");
    }
    shot.activation_consumed = true;
    set_ability_runtime_active(shot.runtime, true);
    set_ability_runtime_window(shot.runtime, activation_tick,
        TickIndex{activation_tick.value() + static_cast<std::uint64_t>(duration) - 1U});
    return {};
}

SessionStatus AbilitySystem::apply_before_step(
    const AbilityArchetype& ability, ShotState& shot)
{
    if (!ability_runtime_active(shot.runtime)) {
        return {};
    }
    return dispatch(ability, shot, hooks_, [](auto& hooks, auto& selected_shot,
        const auto& definition, auto& selected_runtime) {
        return hooks.apply_before_step(
            selected_shot, definition, selected_runtime);
    });
}

SessionStatus AbilitySystem::process_after_step(
    const AbilityArchetype& ability, ShotState& shot)
{
    if (!ability_runtime_active(shot.runtime)) {
        return {};
    }
    return dispatch(ability, shot, hooks_, [](auto& hooks, auto& selected_shot,
        const auto& definition, auto& selected_runtime) {
        return hooks.process_after_step(
            selected_shot, definition, selected_runtime);
    });
}

SessionStatus AbilitySystem::finish_after_step(const AbilityArchetype& ability,
    ShotState& shot, TickIndex current_tick)
{
    if (!ability_runtime_active(shot.runtime)) {
        return {};
    }
    const SessionStatus status = dispatch(ability, shot, hooks_,
        [](auto& hooks, auto& selected_shot, const auto& definition,
            auto& selected_runtime) {
            return hooks.finish_after_step(
                selected_shot, definition, selected_runtime);
        });
    if (!status.ok()) {
        return status;
    }
    const auto end_tick = ability_runtime_end_tick(shot.runtime);
    if (end_tick && current_tick == *end_tick) {
        set_ability_runtime_active(shot.runtime, false);
    }
    return {};
}

}

namespace ninho::simulation {
namespace {

[[nodiscard]] SessionStatus missing_active_ability()
{
    return {{ContentErrorCode::InternalError, "/ability",
        "active ability archetype is unavailable"}};
}

}

SessionStatus SimulationSession::Impl::apply_ability_before_step()
{
    if (!shot || !ability_runtime_active(shot->runtime)) {
        return {};
    }
    const AbilityArchetype* ability = ability_archetype(shot->ability_id);
    if (ability == nullptr) {
        return missing_active_ability();
    }
    return ability_system.apply_before_step(*ability, *shot);
}

SessionStatus SimulationSession::Impl::process_ability_after_step()
{
    if (!shot || !ability_runtime_active(shot->runtime)) {
        return {};
    }
    const AbilityArchetype* ability = ability_archetype(shot->ability_id);
    if (ability == nullptr) {
        return missing_active_ability();
    }
    return ability_system.process_after_step(*ability, *shot);
}

SessionStatus SimulationSession::Impl::finish_ability_after_step()
{
    if (!shot || !ability_runtime_active(shot->runtime)) {
        return {};
    }
    const AbilityArchetype* ability = ability_archetype(shot->ability_id);
    if (ability == nullptr) {
        return missing_active_ability();
    }
    return ability_system.finish_after_step(
        *ability, *shot, session_state.tick);
}

}
