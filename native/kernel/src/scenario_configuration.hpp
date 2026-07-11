#pragma once

#include <ninho/physics/scenario.hpp>

#include <cstdint>
#include <optional>

namespace ninho::physics::detail {

enum class RuntimeConfigurationStatus {
    Diagnostic,
    ReleaseMt,
    ConfigurationMismatch,
};

struct RuntimeConfiguration {
    bool release_build{};
    bool mt_defined{};
    bool dll_defined{};
    bool debug_defined{};
};

[[nodiscard]] constexpr RuntimeConfigurationStatus evaluate_runtime_configuration(
    RuntimeConfiguration configuration) noexcept
{
    if (!configuration.release_build) {
        return RuntimeConfigurationStatus::Diagnostic;
    }
    if (configuration.mt_defined && !configuration.dll_defined
        && !configuration.debug_defined) {
        return RuntimeConfigurationStatus::ReleaseMt;
    }
    return RuntimeConfigurationStatus::ConfigurationMismatch;
}

[[nodiscard]] std::optional<ScenarioResult> configuration_mismatch_result(
    ScenarioKind kind,
    std::uint64_t seed,
    int substeps,
    RuntimeConfiguration configuration);

}
