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

struct ScenarioWarning {
    std::string code;
    std::string message;
    std::vector<ScenarioDetail> details;
};

struct StateValidationFailure {
    std::string field;
    double value{};
};

struct CapabilityRow {
    std::string capability;
    CapabilityStatus status{CapabilityStatus::Blocked};
    std::string fallback;
    CapabilityStatus functional_status{CapabilityStatus::Blocked};
    std::string functional_fallback;
    std::string detail;
    int peak_body_count{};
    int peak_shape_count{};
    int peak_joint_count{};
    int peak_awake_count{};
    int peak_contact_count{};
    std::vector<ScenarioValue> values;
    std::vector<std::uint64_t> fixture_hashes;
};

enum class PrivateCommitStatus { Pass, Unavailable, Unstable, Growth };
struct PrivateCommitAssessment {
    PrivateCommitStatus status{PrivateCommitStatus::Unavailable};
    bool available{};
    bool stable{};
    bool terminal_growth{};
    std::size_t baseline_full_min_bytes{};
    std::size_t baseline_central_min_bytes{};
    std::size_t baseline_median_bytes{};
    std::size_t baseline_central_max_bytes{};
    std::size_t baseline_full_max_bytes{};
    std::size_t final_full_min_bytes{};
    std::size_t final_central_min_bytes{};
    std::size_t final_median_bytes{};
    std::size_t final_central_max_bytes{};
    std::size_t final_full_max_bytes{};
    double growth_ratio{};
    double warmup_trimmed_span_ratio{};
    double warmup_full_span_ratio{};
    double measured_trimmed_span_ratio{};
    double measured_full_span_ratio{};
};

struct ProcessMemorySample {
    std::size_t private_usage_bytes{};
    std::size_t working_set_bytes{};
    std::size_t peak_working_set_bytes{};
};

struct Box3dAllocatorObservation {
    std::int64_t baseline_bytes{};
    std::int64_t final_bytes{};
    std::int64_t max_abs_delta{};
    bool exact_return{true};
    std::vector<std::int64_t> warmup_post_teardown_bytes;
    std::vector<std::int64_t> measured_post_teardown_bytes;
};

struct CrtMemoryObservation {
    bool applicable{};
    bool balanced{true};
    std::int64_t normal_block_count_delta{};
    std::int64_t normal_block_bytes_delta{};
    std::int64_t client_block_count_delta{};
    std::int64_t client_block_bytes_delta{};
};

struct MemoryObservation {
    std::string gate_scope{"release_mt"};
    std::string gate_status{"diagnostic"};
    std::string assessment_status{"unavailable"};
    bool gate_applied{};
    bool budget_qualified{};
    std::string budget_scope{"future_packaged_reference_hardware"};
    bool private_commit_available{};
    bool private_commit_stable{};
    bool private_commit_terminal_growth{};
    std::size_t private_commit_baseline_bytes{};
    std::size_t private_commit_baseline_full_min_bytes{};
    std::size_t private_commit_baseline_central_min_bytes{};
    std::size_t private_commit_baseline_median_bytes{};
    std::size_t private_commit_baseline_central_max_bytes{};
    std::size_t private_commit_baseline_full_max_bytes{};
    std::size_t private_commit_final_bytes{};
    std::size_t private_commit_final_full_min_bytes{};
    std::size_t private_commit_final_central_min_bytes{};
    std::size_t private_commit_final_median_bytes{};
    std::size_t private_commit_final_central_max_bytes{};
    std::size_t private_commit_final_full_max_bytes{};
    std::size_t private_commit_peak_bytes{};
    double private_commit_growth_ratio{};
    double private_commit_warmup_trimmed_span_ratio{};
    double private_commit_warmup_full_span_ratio{};
    double private_commit_measured_trimmed_span_ratio{};
    double private_commit_measured_full_span_ratio{};
    std::vector<std::size_t> private_commit_warmup_samples;
    std::vector<std::size_t> private_commit_cycle_samples;

    std::string working_set_gate_status{"diagnostic"};
    std::string working_set_assessment_status{"unavailable"};
    bool working_set_gate_applied{};
    bool working_set_budget_qualified{};
    std::string working_set_budget_scope{"future_packaged_reference_hardware"};
    bool working_set_available{};
    bool working_set_stable{};
    bool working_set_terminal_growth{};
    std::size_t working_set_baseline_bytes{};
    std::size_t working_set_baseline_full_min_bytes{};
    std::size_t working_set_baseline_central_min_bytes{};
    std::size_t working_set_baseline_median_bytes{};
    std::size_t working_set_baseline_central_max_bytes{};
    std::size_t working_set_baseline_full_max_bytes{};
    std::size_t working_set_final_bytes{};
    std::size_t working_set_final_full_min_bytes{};
    std::size_t working_set_final_central_min_bytes{};
    std::size_t working_set_final_median_bytes{};
    std::size_t working_set_final_central_max_bytes{};
    std::size_t working_set_final_full_max_bytes{};
    std::size_t working_set_peak_bytes{};
    double working_set_growth_ratio{};
    double working_set_instant_growth_ratio{};
    double working_set_warmup_trimmed_span_ratio{};
    double working_set_warmup_full_span_ratio{};
    double working_set_measured_trimmed_span_ratio{};
    double working_set_measured_full_span_ratio{};
    std::vector<std::size_t> working_set_warmup_samples;
    std::vector<std::size_t> working_set_cycle_samples;
};

struct RepeatObservation {
    int repeat_index{};
    std::uint64_t hash{};
    int peak_body_count{};
    int peak_shape_count{};
    int peak_joint_count{};
    int peak_awake_count{};
    int peak_contact_count{};
    MemoryObservation memory;
    Box3dAllocatorObservation box3d_allocator;
    CrtMemoryObservation crt;
};

struct ScenarioResult {
    ScenarioKind kind{ScenarioKind::RadialFall};
    std::string name;
    std::uint64_t seed{};
    int substeps{};
    int ticks{};
    std::uint64_t final_hash{};
    std::vector<std::uint64_t> repeat_hashes;
    std::vector<RepeatObservation> repeat_observations;
    Box3dAllocatorObservation box3d_allocator;
    CrtMemoryObservation crt;

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
    int peak_body_count{};
    int peak_shape_count{};
    int peak_joint_count{};
    int peak_awake_count{};
    int peak_contact_count{};

    double step_min_ms{};
    double step_p50_ms{};
    double step_p95_ms{};
    double step_max_ms{};

    std::string private_commit_gate_scope{"release_mt"};
    std::string private_commit_gate_status{"diagnostic"};
    std::string private_commit_assessment_status{"unavailable"};
    bool private_commit_gate_applied{};
    bool private_commit_budget_qualified{};
    std::string private_commit_budget_scope{"future_packaged_reference_hardware"};
    bool private_commit_available{};
    bool private_commit_stable{};
    bool private_commit_terminal_growth{};
    std::size_t private_commit_baseline_bytes{};
    std::size_t private_commit_baseline_full_min_bytes{};
    std::size_t private_commit_baseline_central_min_bytes{};
    std::size_t private_commit_baseline_median_bytes{};
    std::size_t private_commit_baseline_central_max_bytes{};
    std::size_t private_commit_baseline_full_max_bytes{};
    std::size_t private_commit_final_bytes{};
    std::size_t private_commit_final_full_min_bytes{};
    std::size_t private_commit_final_central_min_bytes{};
    std::size_t private_commit_final_median_bytes{};
    std::size_t private_commit_final_central_max_bytes{};
    std::size_t private_commit_final_full_max_bytes{};
    std::size_t private_commit_peak_bytes{};
    double private_commit_growth_ratio{};
    double private_commit_warmup_trimmed_span_ratio{};
    double private_commit_warmup_full_span_ratio{};
    double private_commit_measured_trimmed_span_ratio{};
    double private_commit_measured_full_span_ratio{};
    std::vector<std::size_t> private_commit_warmup_samples;
    std::vector<std::size_t> private_commit_cycle_samples;

    std::string working_set_gate_status{"diagnostic"};
    std::string working_set_assessment_status{"unavailable"};
    bool working_set_gate_applied{};
    bool working_set_budget_qualified{};
    std::string working_set_budget_scope{"future_packaged_reference_hardware"};
    bool working_set_available{};
    bool working_set_stable{};
    bool working_set_terminal_growth{};
    std::size_t working_set_baseline_bytes{};
    std::size_t working_set_baseline_low_bytes{};
    std::size_t working_set_baseline_central_low_bytes{};
    std::size_t working_set_baseline_median_bytes{};
    std::size_t working_set_baseline_central_high_bytes{};
    std::size_t working_set_baseline_max_bytes{};
    std::size_t working_set_peak_bytes{};
    std::size_t working_set_final_bytes{};
    std::size_t working_set_final_low_bytes{};
    std::size_t working_set_final_central_low_bytes{};
    std::size_t working_set_final_median_bytes{};
    std::size_t working_set_final_central_high_bytes{};
    std::size_t working_set_final_max_bytes{};
    double working_set_growth_ratio{};
    double working_set_instant_growth_ratio{};
    double working_set_warmup_trimmed_span_ratio{};
    double working_set_warmup_full_span_ratio{};
    double working_set_measured_trimmed_span_ratio{};
    double working_set_measured_full_span_ratio{};
    std::vector<std::size_t> working_set_warmup_samples;
    std::vector<std::size_t> working_set_cycle_samples;
    int allocator_warmup_cycles{};
    int warmup_ticks{};
    int measurement_ticks{};
    int stress_cycles{};

    std::string fallback;
    std::vector<ScenarioLimit> limits;
    std::vector<ScenarioValue> metrics;
    std::vector<CapabilityRow> matrix;
    std::vector<ScenarioWarning> warnings;
    std::vector<ScenarioViolation> violations;

    [[nodiscard]] std::string to_json() const;
};

struct ScenarioReport {
    std::string build_type;
    std::string cpu;
    std::uint64_t seed{};
    int substeps{};
    int repeat{};
    Box3dAllocatorObservation process_box3d_allocator;
    std::vector<ScenarioResult> scenarios;

    [[nodiscard]] std::string to_json() const;
};

[[nodiscard]] std::uint64_t hash_states(std::span<const BodyState> states);
[[nodiscard]] double max_rolling_energy_growth(
    std::span<const double> energies, std::size_t window, double epsilon);
[[nodiscard]] PrivateCommitAssessment assess_private_commit(
    std::span<const std::size_t> warmup_samples,
    std::span<const std::size_t> measured_samples);
[[nodiscard]] CrtMemoryObservation debug_crt_allocation_probe(
    bool intentional_allocation);
[[nodiscard]] RepeatObservation make_repeat_observation(
    int repeat_index, const ScenarioResult& result);
[[nodiscard]] bool repeat_topology_matches(
    const RepeatObservation& expected, const RepeatObservation& actual) noexcept;
[[nodiscard]] bool record_repeat_topology_mismatch(
    ScenarioResult& result,
    const RepeatObservation& expected,
    const RepeatObservation& actual);
[[nodiscard]] std::uint64_t hash_capability_rows(std::span<const CapabilityRow> rows);
[[nodiscard]] bool has_two_consecutive_samples(
    std::span<const double> samples, double threshold);
[[nodiscard]] CapabilityStatus classify_lifecycle_status(
    int completed_cycles,
    int invalid_handles,
    std::optional<double> private_commit_growth,
    std::optional<PrivateCommitStatus> working_set_status = std::nullopt);
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
