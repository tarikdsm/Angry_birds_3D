#pragma once

#include <ninho/physics/physics_world.hpp>

#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace ninho::physics {

enum class ScenarioKind {
    RadialFall,
    ProjectilePile,
    RadialPile,
    MassRatio,
    Stress,
    CapabilityMatrix,
};

enum class CapabilityStatus { Pass, Fallback, Blocked };

struct ScenarioValue {
    std::string name;
    double value{};
    std::string unit;
};

struct ScenarioLimit {
    std::string name;
    std::string comparison;
    double value{};
    std::string unit;
};

struct ScenarioDetail {
    std::string name;
    std::string value;
};

struct ScenarioViolation {
    std::string scenario;
    std::string code;
    std::string message;
    int tick{-1};
    BodyHandle handle{};
    std::vector<ScenarioValue> values;
    std::vector<ScenarioDetail> details;
    bool fatal{true};
};

struct StateValidationFailure {
    std::string field;
    double value{};
};

struct CapabilityRow {
    std::string capability;
    CapabilityStatus status{CapabilityStatus::Blocked};
    std::string fallback;
    std::string detail;
    std::vector<ScenarioValue> values;
    std::vector<std::uint64_t> fixture_hashes;
};

struct ScenarioResult {
    ScenarioKind kind{ScenarioKind::RadialFall};
    std::string name;
    std::uint64_t seed{};
    int substeps{};
    int ticks{};
    std::uint64_t final_hash{};
    std::vector<std::uint64_t> repeat_hashes;

    bool contact_before_pile_exit{};
    int ccd_primary_pass_count{};
    int ccd_fallback_pass_count{};
    double projectile_speed{};
    double surface_separation{};
    double final_linear_speed{};
    double final_angular_speed{};
    double p95_linear_speed{};
    double p95_angular_speed{};
    double sleep_ratio{};
    double max_penetration{};
    double minimum_density{};
    double maximum_density{};
    double energy_at_tick_600{};
    double max_energy_growth_ratio{};

    int dynamic_body_count{};
    int shape_count{};
    int joint_count{};
    int peak_awake_count{};
    int peak_contact_count{};

    double step_min_ms{};
    double step_p50_ms{};
    double step_p95_ms{};
    double step_max_ms{};

    bool working_set_available{};
    std::size_t working_set_baseline_bytes{};
    std::size_t working_set_baseline_low_bytes{};
    std::size_t working_set_peak_bytes{};
    std::size_t working_set_final_bytes{};
    std::size_t working_set_final_low_bytes{};
    double working_set_growth_ratio{};
    double working_set_instant_growth_ratio{};
    std::vector<std::size_t> working_set_warmup_samples;
    std::vector<std::size_t> working_set_cycle_samples;
    int warmup_ticks{};
    int measurement_ticks{};
    int stress_cycles{};

    std::string fallback;
    std::vector<ScenarioLimit> limits;
    std::vector<ScenarioValue> metrics;
    std::vector<CapabilityRow> matrix;
    std::vector<ScenarioViolation> violations;

    [[nodiscard]] std::string to_json() const;
};

struct ScenarioReport {
    std::string build_type;
    std::string cpu;
    std::uint64_t seed{};
    int substeps{};
    int repeat{};
    std::vector<ScenarioResult> scenarios;

    [[nodiscard]] std::string to_json() const;
};

[[nodiscard]] std::uint64_t hash_states(std::span<const BodyState> states);
[[nodiscard]] double max_rolling_energy_growth(
    std::span<const double> energies, std::size_t window, double epsilon);
[[nodiscard]] std::uint64_t hash_capability_rows(std::span<const CapabilityRow> rows);
[[nodiscard]] bool has_two_consecutive_samples(
    std::span<const double> samples, double threshold);
[[nodiscard]] CapabilityStatus classify_lifecycle_status(
    int completed_cycles,
    int invalid_handles,
    std::optional<double> working_set_growth);
[[nodiscard]] int scenario_exit_code(
    std::span<const ScenarioResult> scenarios, bool hash_mismatch);
[[nodiscard]] std::optional<StateValidationFailure> validate_body_state(
    const BodyState& state, double planet_radius);
[[nodiscard]] std::string_view scenario_name(ScenarioKind kind);
[[nodiscard]] std::optional<ScenarioKind> parse_scenario_kind(std::string_view name);

class ScenarioRunner {
public:
    [[nodiscard]] static constexpr int watchdog_timeout_seconds() noexcept
    {
        return 60;
    }

    static void set_emergency_json_path(std::optional<std::string> path);

    [[nodiscard]] ScenarioResult run(
        ScenarioKind kind, std::uint64_t seed, int substeps) const;
};

}
