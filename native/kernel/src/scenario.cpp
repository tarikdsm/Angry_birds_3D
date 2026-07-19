#include <ninho/physics/scenario.hpp>

#include "box3d_replay_conformance.hpp"
#include "box3d_allocator_probe.hpp"
#include "scenario_configuration.hpp"
#if defined(NINHO_ENABLE_TEST_FACADES)
#include "scenario_test_facade.hpp"
#endif

#include <algorithm>
#include <array>
#include <atomic>
#include <bit>
#include <chrono>
#include <charconv>
#include <cmath>
#include <condition_variable>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <limits>
#include <mutex>
#include <optional>
#include <ranges>
#include <set>
#include <stdexcept>
#include <thread>
#include <system_error>
#include <tuple>
#include <string_view>
#include <vector>

#if defined(_MSC_VER) && defined(_DEBUG)
#include <crtdbg.h>
#endif

#ifdef _WIN32
#define NOMINMAX
#include <windows.h>
#include <psapi.h>
#endif

namespace ninho::physics {
namespace {

#if defined(NINHO_ENABLE_TEST_FACADES)
std::size_t projectile_simulation_count_value{};
#endif

[[nodiscard]] std::string_view name_of(ScenarioKind kind)
{
    switch (kind) {
    case ScenarioKind::RadialFall:
        return "radial_fall";
    case ScenarioKind::ProjectilePile:
        return "projectile_pile";
    case ScenarioKind::RadialPile:
        return "radial_pile";
    case ScenarioKind::MassRatio:
        return "mass_ratio";
    case ScenarioKind::Stress:
        return "stress";
    case ScenarioKind::CapabilityMatrix:
        return "capability_matrix";
    }
    return "unknown";
}

std::mutex emergency_json_mutex;
std::optional<std::string> emergency_json_path;

class ScenarioWatchdog {
public:
    ScenarioWatchdog(ScenarioKind kind, std::uint64_t seed)
        : scenario_(name_of(kind))
        , seed_(seed)
        , timeout_seconds_(ScenarioRunner::watchdog_timeout_seconds(kind))
        , deadline_(
              std::chrono::steady_clock::now()
              + std::chrono::seconds(timeout_seconds_))
    {
        {
            const std::scoped_lock lock(emergency_json_mutex);
            path_ = emergency_json_path;
        }
        worker_ = std::thread([this] {
            std::unique_lock lock(mutex_);
            if (!condition_.wait_until(lock, deadline_, [this] { return done_; })) {
                lock.unlock();
                terminate_process();
            }
        });
    }

    ScenarioWatchdog(const ScenarioWatchdog&) = delete;
    ScenarioWatchdog& operator=(const ScenarioWatchdog&) = delete;

    ~ScenarioWatchdog()
    {
        {
            const std::scoped_lock lock(mutex_);
            done_ = true;
        }
        condition_.notify_one();
        if (worker_.joinable()) {
            worker_.join();
        }
    }

    void checkpoint(int tick)
    {
        tick_.store(tick, std::memory_order_relaxed);
        if (std::chrono::steady_clock::now() >= deadline_) {
            terminate_process();
        }
    }

private:
    [[noreturn]] void terminate_process()
    {
        if (terminating_.test_and_set(std::memory_order_acq_rel)) {
            for (;;) {
                std::this_thread::yield();
            }
        }
        const int tick = tick_.load(std::memory_order_relaxed);
        std::fprintf(
            stderr,
            "error: scenario watchdog expired: scenario=%.*s seed=%llu tick=%d "
            "timeout_seconds=%d\n",
            static_cast<int>(scenario_.size()),
            scenario_.data(),
            static_cast<unsigned long long>(seed_),
            tick,
            timeout_seconds_);
        std::fflush(stderr);
        write_emergency_json(tick);
#ifdef _WIN32
        TerminateProcess(GetCurrentProcess(), 1);
#endif
        std::_Exit(1);
    }

    void write_emergency_json(int tick) const noexcept
    {
        if (!path_) {
            return;
        }
        try {
            const std::filesystem::path path{*path_};
            if (const auto parent = path.parent_path(); !parent.empty()) {
                std::error_code error;
                std::filesystem::create_directories(parent, error);
                if (error) {
                    return;
                }
            }
            std::ofstream stream(path, std::ios::binary | std::ios::trunc);
            if (!stream) {
                return;
            }
            stream << "{\"watchdog\":{\"scenario\":\"" << scenario_
                   << "\",\"seed\":" << seed_ << ",\"tick\":" << tick
                   << ",\"timeout_seconds\":" << timeout_seconds_
                   << "},\"violations\":[{\"scenario\":\"" << scenario_
                   << "\",\"code\":\"scenario_timeout\",\"message\":\"scenario "
                      "exceeded the configured watchdog timeout\",\"tick\":"
                   << tick << ",\"fatal\":true}]}";
            stream.flush();
        } catch (...) {
        }
    }

    std::string_view scenario_;
    std::uint64_t seed_{};
    int timeout_seconds_{};
    std::chrono::steady_clock::time_point deadline_;
    std::optional<std::string> path_;
    std::atomic<int> tick_{0};
    std::atomic_flag terminating_ = ATOMIC_FLAG_INIT;
    std::mutex mutex_;
    std::condition_variable condition_;
    bool done_{};
    std::thread worker_;
};

thread_local ScenarioWatchdog* active_watchdog{};

class WatchdogActivation {
public:
    explicit WatchdogActivation(ScenarioWatchdog& watchdog)
        : previous_(active_watchdog)
    {
        active_watchdog = &watchdog;
    }

    ~WatchdogActivation()
    {
        active_watchdog = previous_;
    }

private:
    ScenarioWatchdog* previous_{};
};

void watchdog_checkpoint(int tick)
{
    if (active_watchdog) {
        active_watchdog->checkpoint(tick);
    }
}

void watched_step(PhysicsWorld& world, int tick)
{
    watchdog_checkpoint(tick);
    world.step();
    watchdog_checkpoint(tick);
}

[[nodiscard]] ScenarioResult make_result(
    ScenarioKind kind, std::uint64_t seed, int substeps)
{
    return {
        .kind = kind,
        .name = std::string{name_of(kind)},
        .seed = seed,
        .substeps = substeps,
    };
}

void add_violation(
    ScenarioResult& result,
    std::string code,
    std::string message,
    int tick,
    BodyHandle handle,
    std::vector<ScenarioValue> values = {},
    std::vector<ScenarioDetail> details = {})
{
    result.violations.push_back({
        .scenario = result.name,
        .code = std::move(code),
        .message = std::move(message),
        .tick = tick,
        .handle = handle,
        .values = std::move(values),
        .details = std::move(details),
    });
}

[[nodiscard]] std::string exact_value_text(double value)
{
    if (std::isnan(value)) {
        return "NaN";
    }
    if (std::isinf(value)) {
        return value < 0.0 ? "-Inf" : "Inf";
    }
    std::array<char, 64> buffer{};
    const auto converted = std::to_chars(
        buffer.data(),
        buffer.data() + buffer.size(),
        value,
        std::chars_format::general,
        std::numeric_limits<double>::max_digits10);
    if (converted.ec != std::errc{}) {
        throw std::invalid_argument("state value cannot be formatted");
    }
    return std::string{buffer.data(), converted.ptr};
}

struct StateScanFailure {
    BodyHandle handle{};
    StateValidationFailure invariant;
};

[[nodiscard]] std::optional<StateScanFailure> scan_body_states(
    std::span<const BodyState> states, double planet_radius)
{
    for (const BodyState& state : states) {
        if (const auto failure = validate_body_state(state, planet_radius)) {
            return StateScanFailure{state.handle, *failure};
        }
    }
    return std::nullopt;
}

[[nodiscard]] bool record_state_scan(
    ScenarioResult& result,
    std::span<const BodyState> states,
    int tick,
    double planet_radius)
{
    const auto failure = scan_body_states(states, planet_radius);
    if (!failure) {
        return true;
    }
    const std::string value = exact_value_text(failure->invariant.value);
    add_violation(
        result,
        "fatal_state_invariant",
        "body state invariant failed: " + failure->invariant.field + '=' + value,
        tick,
        failure->handle,
        {},
        {{"field", failure->invariant.field}, {"value", value}});
    return false;
}

void sample_world_metrics(
    ScenarioResult& result, const WorldMetrics& metrics, std::vector<double>& timings)
{
    timings.push_back(metrics.step_ms);
    result.peak_body_count = std::max(result.peak_body_count, metrics.body_count);
    result.peak_shape_count = std::max(result.peak_shape_count, metrics.shape_count);
    result.peak_joint_count = std::max(result.peak_joint_count, metrics.joint_count);
    result.peak_awake_count = std::max(result.peak_awake_count, metrics.awake_count);
    result.peak_contact_count = std::max(result.peak_contact_count, metrics.contact_count);
}

[[nodiscard]] double percentile(const std::vector<double>& sorted, double fraction)
{
    if (sorted.empty()) {
        return 0.0;
    }
    const double rank = std::ceil(fraction * static_cast<double>(sorted.size()));
    const std::size_t index = static_cast<std::size_t>(std::max(1.0, rank)) - 1;
    return sorted[std::min(index, sorted.size() - 1)];
}

void finish_timings(ScenarioResult& result, std::vector<double> timings)
{
    if (timings.empty()) {
        return;
    }
    std::ranges::sort(timings);
    result.step_min_ms = timings.front();
    result.step_p50_ms = percentile(timings, 0.50);
    result.step_p95_ms = percentile(timings, 0.95);
    result.step_max_ms = timings.back();
}

[[nodiscard]] double sphere_kinetic_energy(const BodyState& state, double radius)
{
    const double linear_speed = length(state.linear_velocity);
    const double angular_speed = length(state.angular_velocity);
    const double inertia = 0.4 * state.mass * radius * radius;
    return 0.5 * state.mass * linear_speed * linear_speed
        + 0.5 * inertia * angular_speed * angular_speed;
}

[[nodiscard]] ScenarioResult run_radial_fall(
    std::uint64_t seed, int substeps)
{
    ScenarioResult result = make_result(ScenarioKind::RadialFall, seed, substeps);
    result.ticks = 600;
    result.dynamic_body_count = 1;
    result.shape_count = 2;
    result.limits = {
        {"surface_separation_min", ">=", -0.02, "m"},
        {"surface_separation_max", "<=", 0.03, "m"},
        {"final_linear_speed", "<", 0.05, "m/s"},
        {"final_angular_speed", "<", 0.10, "rad/s"},
        {"max_energy_growth_ratio", "<=", 0.02, "ratio"},
    };

    constexpr float planet_radius = 10.0f;
    constexpr float ball_radius = 0.4f;
    PhysicsWorld world(WorldConfig{
        .substeps = substeps,
        .planet_radius = planet_radius,
        .surface_gravity = 9.0f,
        .max_bodies = 2,
    });
    const auto planet =
        world.create_body(BodyDesc::static_sphere(planet_radius, {}));
    const auto ball = world.create_body(
        BodyDesc::dynamic_sphere(ball_radius, {{0, 15, 0}, {}}, 520.0f));
    if (!planet || !ball) {
        add_violation(
            result,
            "create_body",
            "failed to create radial-fall fixture",
            0,
            ball.value,
            {{"planet_status", static_cast<double>(planet.status.code), "status"},
             {"ball_status", static_cast<double>(ball.status.code), "status"}});
        return result;
    }

    std::vector<double> timings;
    timings.reserve(result.ticks);
    constexpr std::size_t energy_window = 120;
    constexpr double energy_epsilon = 1.0e-9;
    std::vector<double> post_impact_energy;
    post_impact_energy.reserve(result.ticks);
    bool contacted = false;
    for (int tick = 1; tick <= result.ticks; ++tick) {
        watched_step(world, tick);
        sample_world_metrics(result, world.metrics(), timings);
        if (!record_state_scan(result, world.states(), tick, planet_radius)) {
            break;
        }
        const auto state = world.state(ball.value);
        if (!state || !is_finite(state->transform.position)
            || !is_finite(state->linear_velocity) || !is_finite(state->angular_velocity)) {
            add_violation(
                result, "invalid_state", "radial-fall state is missing or non-finite", tick, ball.value);
            break;
        }
        if (!contacted) {
            contacted = std::ranges::any_of(world.contact_hits(), [&](const ContactHit& hit) {
                return hit.a == std::min(planet.value, ball.value)
                    && hit.b == std::max(planet.value, ball.value);
            });
        }
        if (contacted) {
            post_impact_energy.push_back(sphere_kinetic_energy(*state, ball_radius));
        }
    }

    finish_timings(result, std::move(timings));
    if (!result.violations.empty()) {
        return result;
    }
    const auto final_state = world.state(ball.value);
    if (!final_state) {
        if (result.violations.empty()) {
            add_violation(result, "missing_body", "radial-fall body disappeared", 600, ball.value);
        }
        return result;
    }
    result.surface_separation =
        length(final_state->transform.position) - planet_radius - ball_radius;
    result.final_linear_speed = length(final_state->linear_velocity);
    result.final_angular_speed = length(final_state->angular_velocity);
    if (post_impact_energy.size() < energy_window) {
        add_violation(
            result,
            "radial_fall_energy_samples",
            "fewer than 120 post-impact energy samples were recorded",
            result.ticks,
            ball.value,
            {{"samples", static_cast<double>(post_impact_energy.size()), "ticks"}});
        return result;
    }
    result.max_energy_growth_ratio = max_rolling_energy_growth(
        post_impact_energy, energy_window, energy_epsilon);
    result.final_hash = hash_states(world.states());
    result.metrics = {
        {"surface_separation", result.surface_separation, "m"},
        {"final_linear_speed", result.final_linear_speed, "m/s"},
        {"final_angular_speed", result.final_angular_speed, "rad/s"},
        {"max_energy_growth_ratio", result.max_energy_growth_ratio, "ratio"},
        {"energy_window", static_cast<double>(energy_window), "ticks"},
        {"energy_epsilon", energy_epsilon, "J"},
    };

    if (result.surface_separation < -0.02 || result.surface_separation > 0.03) {
        add_violation(
            result,
            "radial_fall_separation",
            "surface separation is outside the fixed interval",
            600,
            ball.value,
            {{"surface_separation", result.surface_separation, "m"}});
    }
    if (result.final_linear_speed >= 0.05) {
        add_violation(
            result,
            "radial_fall_linear_speed",
            "final linear speed exceeds the fixed limit",
            600,
            ball.value,
            {{"linear_speed", result.final_linear_speed, "m/s"}});
    }
    if (result.final_angular_speed >= 0.10) {
        add_violation(
            result,
            "radial_fall_angular_speed",
            "final angular speed exceeds the fixed limit",
            600,
            ball.value,
            {{"angular_speed", result.final_angular_speed, "rad/s"}});
    }
    if (result.max_energy_growth_ratio > 0.02) {
        add_violation(
            result,
            "radial_fall_energy_growth",
            "kinetic energy grew by more than two percent in any 120-tick post-contact window",
            600,
            ball.value,
            {{"growth_ratio", result.max_energy_growth_ratio, "ratio"}});
    }
    return result;
}

class XorShift64 {
public:
    explicit XorShift64(std::uint64_t seed)
        : state_(seed == 0 ? 0x9e3779b97f4a7c15ull : seed)
    {
    }

    [[nodiscard]] std::uint64_t next()
    {
        state_ ^= state_ << 13;
        state_ ^= state_ >> 7;
        state_ ^= state_ << 17;
        return state_;
    }

    [[nodiscard]] float centered(float magnitude)
    {
        const double unit = static_cast<double>(next() >> 11)
            * (1.0 / 9007199254740992.0);
        return static_cast<float>((unit * 2.0 - 1.0) * magnitude);
    }

private:
    std::uint64_t state_;
};

struct ProjectileOutcome {
    bool contact_before_exit{};
    bool invalid_state{};
    int ticks{};
    BodyHandle projectile{};
    double step_min_ms{};
    double step_p50_ms{};
    double step_p95_ms{};
    double step_max_ms{};
    int peak_awake_count{};
    int peak_contact_count{};
    int peak_body_count{};
    int peak_shape_count{};
    int peak_joint_count{};
    std::uint64_t final_hash{};
    std::optional<StateScanFailure> state_failure;
};

void sample_capability_metrics(CapabilityRow& row, const WorldMetrics& metrics)
{
    row.peak_body_count = std::max(row.peak_body_count, metrics.body_count);
    row.peak_shape_count = std::max(row.peak_shape_count, metrics.shape_count);
    row.peak_joint_count = std::max(row.peak_joint_count, metrics.joint_count);
    row.peak_awake_count = std::max(row.peak_awake_count, metrics.awake_count);
    row.peak_contact_count = std::max(row.peak_contact_count, metrics.contact_count);
}

void merge_capability_metrics(CapabilityRow& row, const ProjectileOutcome& outcome)
{
    row.peak_body_count = std::max(row.peak_body_count, outcome.peak_body_count);
    row.peak_shape_count = std::max(row.peak_shape_count, outcome.peak_shape_count);
    row.peak_joint_count = std::max(row.peak_joint_count, outcome.peak_joint_count);
    row.peak_awake_count = std::max(row.peak_awake_count, outcome.peak_awake_count);
    row.peak_contact_count = std::max(row.peak_contact_count, outcome.peak_contact_count);
}

[[nodiscard]] ProjectileOutcome simulate_projectile(
    std::uint64_t seed, float speed, int substeps)
{
#if defined(NINHO_ENABLE_TEST_FACADES)
    ++projectile_simulation_count_value;
#endif
    constexpr float planet_radius = 10.0f;
    constexpr float platform_half_height = 0.25f;
    constexpr int block_count = 120;
    PhysicsWorld world(WorldConfig{
        .substeps = substeps,
        .planet_radius = planet_radius,
        .surface_gravity = 9.0f,
        .max_bodies = block_count + 3,
    });
    const auto planet = world.create_body(BodyDesc::static_sphere(planet_radius, {}));
    const auto platform = world.create_body(BodyDesc::static_box(
        {4.0f, platform_half_height, 2.0f},
        {{0, planet_radius + platform_half_height, 0}, {}}));
    if (!planet || !platform) {
        return {.invalid_state = true};
    }

    XorShift64 random(seed);
    std::vector<BodyHandle> blocks;
    blocks.reserve(block_count);
    constexpr int columns = 6;
    constexpr int rows = block_count / columns;
    for (int row = 0; row < rows; ++row) {
        for (int column = 0; column < columns; ++column) {
            const float x = (static_cast<float>(column) - 2.5f) * 0.61f
                + random.centered(0.0005f);
            const float y = planet_radius + 0.5f + 0.26f
                + static_cast<float>(row) * 0.51f;
            BodyDesc block = BodyDesc::dynamic_box(
                {0.30f, 0.25f, 0.50f},
                {{x, y, random.centered(0.0005f)}, {}},
                500.0f + random.centered(25.0f));
            block.shapes.front().material_id = 1000u
                + static_cast<std::uint64_t>(row * columns + column);
            const auto created = world.create_body(block);
            if (!created) {
                return {.invalid_state = true};
            }
            blocks.push_back(created.value);
        }
    }

    BodyDesc projectile = BodyDesc::dynamic_sphere(
        0.45f, {{-8.0f, planet_radius + 3.25f, 0}, {}}, 520.0f);
    projectile.linear_velocity = {speed, 0, 0};
    projectile.bullet = true;
    projectile.enable_sleep = false;
    projectile.shapes.front().material_id = 9001;
    const auto projectile_result = world.create_body(projectile);
    if (!projectile_result) {
        return {.invalid_state = true};
    }

    ProjectileOutcome outcome{.projectile = projectile_result.value};
    std::vector<double> timings;
    timings.reserve(90);
    constexpr float pile_exit_x = 3.0f;
    for (int tick = 1; tick <= 90; ++tick) {
        watched_step(world, tick);
        outcome.ticks = tick;
        const WorldMetrics metrics = world.metrics();
        timings.push_back(metrics.step_ms);
        outcome.peak_body_count = std::max(outcome.peak_body_count, metrics.body_count);
        outcome.peak_shape_count = std::max(outcome.peak_shape_count, metrics.shape_count);
        outcome.peak_joint_count = std::max(outcome.peak_joint_count, metrics.joint_count);
        outcome.peak_awake_count = std::max(outcome.peak_awake_count, metrics.awake_count);
        outcome.peak_contact_count = std::max(outcome.peak_contact_count, metrics.contact_count);

        if (const auto failure = scan_body_states(world.states(), planet_radius)) {
            outcome.invalid_state = true;
            outcome.state_failure = failure;
            break;
        }

        const auto projectile_state = world.state(projectile_result.value);
        if (!projectile_state || !is_finite(projectile_state->transform.position)
            || !is_finite(projectile_state->linear_velocity)
            || !is_finite(projectile_state->angular_velocity)
            || length(projectile_state->linear_velocity) > 100.0f) {
            outcome.invalid_state = true;
            break;
        }
        const bool exited = projectile_state->transform.position.x > pile_exit_x;
        for (const ContactHit& hit : world.contact_hits()) {
            const BodyHandle other = hit.a == projectile_result.value ? hit.b
                : hit.b == projectile_result.value                    ? hit.a
                                                                     : BodyHandle{};
            if (other.valid() && std::ranges::find(blocks, other) != blocks.end()) {
                outcome.contact_before_exit = !exited;
                break;
            }
        }
        if (outcome.contact_before_exit || exited) {
            break;
        }
    }
    std::ranges::sort(timings);
    if (!timings.empty()) {
        outcome.step_min_ms = timings.front();
        outcome.step_p50_ms = percentile(timings, 0.50);
        outcome.step_p95_ms = percentile(timings, 0.95);
        outcome.step_max_ms = timings.back();
    }
    if (!outcome.invalid_state) {
        outcome.final_hash = hash_states(world.states());
    }
    return outcome;
}

struct ProjectileGate {
    std::array<ProjectileOutcome, 20> primary;
    std::array<ProjectileOutcome, 20> fallback;
    int primary_pass_count{};
    int fallback_pass_count{};
    int primary_invalid_count{};
    int fallback_invalid_count{};
    bool uses_fallback{};
};

[[nodiscard]] ProjectileGate run_projectile_gate()
{
    ProjectileGate gate{};
    for (std::size_t index = 0; index < gate.primary.size(); ++index) {
        gate.primary[index] = simulate_projectile(index + 1, 35.0f, 4);
        gate.primary_invalid_count += gate.primary[index].invalid_state;
        gate.primary_pass_count += gate.primary[index].contact_before_exit
            && !gate.primary[index].invalid_state;
    }
    if (gate.primary_pass_count != 20) {
        gate.uses_fallback = true;
        for (std::size_t index = 0; index < gate.fallback.size(); ++index) {
            gate.fallback[index] = simulate_projectile(index + 1, 30.0f, 6);
            gate.fallback_invalid_count += gate.fallback[index].invalid_state;
            gate.fallback_pass_count += gate.fallback[index].contact_before_exit
                && !gate.fallback[index].invalid_state;
        }
    }
    return gate;
}

[[nodiscard]] ScenarioResult run_projectile_pile(std::uint64_t seed)
{
    static const ProjectileGate gate = run_projectile_gate();
    const bool fallback_passed = gate.uses_fallback && gate.fallback_pass_count == 20;
    const bool use_fallback_outcome = gate.uses_fallback;
    const float speed = use_fallback_outcome ? 30.0f : 35.0f;
    const int substeps = use_fallback_outcome ? 6 : 4;

    ScenarioResult result = make_result(ScenarioKind::ProjectilePile, seed, substeps);
    result.dynamic_body_count = 121;
    result.shape_count = 123;
    result.ccd_primary_pass_count = gate.primary_pass_count;
    result.ccd_fallback_pass_count = gate.fallback_pass_count;
    result.projectile_speed = speed;
    result.limits = gate.uses_fallback
        ? std::vector<ScenarioLimit>{
              {"primary_seed_passes", "<", 20, "seeds"},
              {"fallback_seed_passes", "==", 20, "seeds"},
              {"invalid_states", "==", 0, "states"},
          }
        : std::vector<ScenarioLimit>{
              {"primary_seed_passes", "==", 20, "seeds"},
              {"fallback_seed_passes", "==", 0, "seeds"},
              {"invalid_states", "==", 0, "states"},
          };
    if (fallback_passed) {
        result.fallback = "speed30_substeps6";
    }

    const ProjectileOutcome outcome = simulate_projectile(seed, speed, substeps);
    result.contact_before_pile_exit = outcome.contact_before_exit;
    result.ticks = outcome.ticks;
    result.final_hash = outcome.final_hash;
    result.step_min_ms = outcome.step_min_ms;
    result.step_p50_ms = outcome.step_p50_ms;
    result.step_p95_ms = outcome.step_p95_ms;
    result.step_max_ms = outcome.step_max_ms;
    result.peak_body_count = outcome.peak_body_count;
    result.peak_shape_count = outcome.peak_shape_count;
    result.peak_joint_count = outcome.peak_joint_count;
    result.peak_awake_count = outcome.peak_awake_count;
    result.peak_contact_count = outcome.peak_contact_count;
    result.metrics = {
        {"primary_seed_passes", static_cast<double>(gate.primary_pass_count), "seeds"},
        {"fallback_seed_passes", static_cast<double>(gate.fallback_pass_count), "seeds"},
        {"projectile_speed", speed, "m/s"},
        {"primary_invalid_states", static_cast<double>(gate.primary_invalid_count), "count"},
        {"fallback_invalid_states", static_cast<double>(gate.fallback_invalid_count), "count"},
        {"fallback_seed_evaluated", gate.uses_fallback ? 1.0 : 0.0, "bool"},
    };

    if (gate.primary_invalid_count != 0 || gate.fallback_invalid_count != 0) {
        add_violation(
            result,
            "fatal_state_invariant",
            "CCD gate encountered an invalid body state in a fixed seed set",
            outcome.ticks,
            outcome.projectile,
            {{"primary_invalid_states", static_cast<double>(gate.primary_invalid_count), "count"},
             {"fallback_invalid_states", static_cast<double>(gate.fallback_invalid_count), "count"}});
    } else if (gate.uses_fallback && !fallback_passed) {
        add_violation(
            result,
            "ccd_dynamic_dynamic",
            "dynamic-dynamic CCD failed both approved 20-seed parameter sets",
            outcome.ticks,
            outcome.projectile,
            {{"primary_passes", static_cast<double>(gate.primary_pass_count), "seeds"},
             {"fallback_passes", static_cast<double>(gate.fallback_pass_count), "seeds"}});
    } else if (outcome.state_failure) {
        const std::string value = exact_value_text(outcome.state_failure->invariant.value);
        add_violation(
            result,
            "fatal_state_invariant",
            "projectile fixture state invariant failed: "
                + outcome.state_failure->invariant.field + '=' + value,
            outcome.ticks,
            outcome.state_failure->handle,
            {},
            {{"field", outcome.state_failure->invariant.field}, {"value", value}});
    } else if (!outcome.contact_before_exit || outcome.invalid_state) {
        add_violation(
            result,
            "ccd_seed_miss",
            "projectile did not contact a pile body before crossing the pile",
            outcome.ticks,
            outcome.projectile,
            {{"speed", speed, "m/s"},
             {"substeps", static_cast<double>(substeps), "count"}});
    }
    return result;
}

[[nodiscard]] double cube_kinetic_energy(const BodyState& state, double side)
{
    const double linear_speed = length(state.linear_velocity);
    const double angular_speed = length(state.angular_velocity);
    const double inertia = state.mass * side * side / 6.0;
    return 0.5 * state.mass * linear_speed * linear_speed
        + 0.5 * inertia * angular_speed * angular_speed;
}

struct OrientedBox {
    Vec3 center{};
    std::array<Vec3, 3> axes{};
    Vec3 half_extents{};
};

[[nodiscard]] OrientedBox oriented_box(
    const BodyState& state, Vec3 half_extents)
{
    const Quat q = state.transform.rotation;
    const float xx = q.x * q.x;
    const float yy = q.y * q.y;
    const float zz = q.z * q.z;
    const float xy = q.x * q.y;
    const float xz = q.x * q.z;
    const float yz = q.y * q.z;
    const float wx = q.w * q.x;
    const float wy = q.w * q.y;
    const float wz = q.w * q.z;
    return {
        .center = state.transform.position,
        .axes = {
            Vec3{1 - 2 * (yy + zz), 2 * (xy + wz), 2 * (xz - wy)},
            Vec3{2 * (xy - wz), 1 - 2 * (xx + zz), 2 * (yz + wx)},
            Vec3{2 * (xz + wy), 2 * (yz - wx), 1 - 2 * (xx + yy)},
        },
        .half_extents = half_extents,
    };
}

[[nodiscard]] double projection_radius(const OrientedBox& box, Vec3 axis)
{
    return std::abs(dot(box.axes[0], axis)) * box.half_extents.x
        + std::abs(dot(box.axes[1], axis)) * box.half_extents.y
        + std::abs(dot(box.axes[2], axis)) * box.half_extents.z;
}

[[nodiscard]] double obb_penetration(const OrientedBox& first, const OrientedBox& second)
{
    std::array<Vec3, 15> candidates{
        first.axes[0],
        first.axes[1],
        first.axes[2],
        second.axes[0],
        second.axes[1],
        second.axes[2],
    };
    std::size_t candidate_count = 6;
    for (const Vec3 first_axis : first.axes) {
        for (const Vec3 second_axis : second.axes) {
            const Vec3 product = cross(first_axis, second_axis);
            if (dot(product, product) > 1.0e-10f) {
                candidates[candidate_count++] = normalized_or_zero(product);
            }
        }
    }

    double minimum_overlap = std::numeric_limits<double>::max();
    const Vec3 center_delta = second.center - first.center;
    for (std::size_t index = 0; index < candidate_count; ++index) {
        const Vec3 axis = candidates[index];
        const double overlap = projection_radius(first, axis)
            + projection_radius(second, axis) - std::abs(dot(center_delta, axis));
        if (overlap <= 0.0) {
            return 0.0;
        }
        minimum_overlap = std::min(minimum_overlap, overlap);
    }
    return minimum_overlap;
}

[[nodiscard]] ScenarioResult run_stability_pile(
    ScenarioKind kind, std::uint64_t seed, int substeps)
{
    const bool mass_ratio = kind == ScenarioKind::MassRatio;
    const double sleep_limit = mass_ratio ? 0.80 : 0.90;
    const double linear_limit = mass_ratio ? 0.10 : 0.05;
    const double angular_limit = mass_ratio ? 0.20 : 0.10;
    const double penetration_limit = mass_ratio ? 0.025 : 0.02;
    const std::string code_prefix = mass_ratio ? "mass_ratio" : "radial_pile";
    ScenarioResult result = make_result(kind, seed, substeps);
    result.ticks = 1800;
    result.dynamic_body_count = 80;
    result.shape_count = 81;
    result.minimum_density = mass_ratio ? 85.0 : 480.0;
    result.maximum_density = mass_ratio ? 3400.0 : 520.0;
    result.limits = {
        {"sleep_ratio", ">=", sleep_limit, "ratio"},
        {"p95_linear_speed", "<", linear_limit, "m/s"},
        {"p95_angular_speed", "<", angular_limit, "rad/s"},
        {"max_penetration", "<", penetration_limit, "m"},
        {"spontaneous_speed", "<=", 100.0, "m/s"},
        {"world_radius", "<=", 60.0, "m"},
    };
    if (!mass_ratio) {
        result.limits.push_back(
            {"energy_growth_from_tick_600", "<=", 0.02, "ratio"});
    }

    constexpr float planet_radius = 10.0f;
    constexpr float platform_top = 10.5f;
    constexpr float cube_side = 0.5f;
    PhysicsWorld world(WorldConfig{
        .substeps = substeps,
        .planet_radius = planet_radius,
        .surface_gravity = 9.0f,
        .max_bodies = 81,
    });
    const auto platform = world.create_body(
        BodyDesc::static_box({4, 0.25f, 4}, {{0, 10.25f, 0}, {}}));
    if (!platform) {
        add_violation(result, "create_body", "failed to create stability-pile platform", 0, {});
        return result;
    }

    XorShift64 random(seed);
    std::vector<BodyHandle> blocks;
    blocks.reserve(80);
    int block_index = 0;
    for (int layer = 0; layer < 4; ++layer) {
        for (int z_index = 0; z_index < 4; ++z_index) {
            for (int x_index = 0; x_index < 5; ++x_index) {
                const Vec3 position{
                    (static_cast<float>(x_index) - 2.0f) * 0.51f
                        + random.centered(0.0002f),
                    platform_top + 0.26f + static_cast<float>(layer) * 0.51f,
                    (static_cast<float>(z_index) - 1.5f) * 0.51f
                        + random.centered(0.0002f),
                };
                const float density = mass_ratio
                    ? 85.0f + (3400.0f - 85.0f)
                        * static_cast<float>(block_index) / 79.0f
                    : 500.0f + random.centered(20.0f);
                BodyDesc block = BodyDesc::dynamic_box(
                    {0.25f, 0.25f, 0.25f},
                    {position, {}},
                    density);
                const auto created = world.create_body(block);
                if (!created) {
                    add_violation(
                        result,
                        "create_body",
                        "failed to create all 80 stability-pile blocks",
                        0,
                        created.value,
                        {{"created_blocks", static_cast<double>(blocks.size()), "count"}});
                    return result;
                }
                blocks.push_back(created.value);
                ++block_index;
            }
        }
    }

    std::vector<double> timings;
    timings.reserve(result.ticks);
    std::vector<double> measured_linear_speeds;
    std::vector<double> measured_angular_speeds;
    measured_linear_speeds.reserve(80 * 300);
    measured_angular_speeds.reserve(80 * 300);
    std::size_t sleeping_samples = 0;
    std::size_t total_samples = 0;
    double maximum_energy_after_600 = 0.0;
    double maximum_platform_penetration = 0.0;
    double maximum_pair_penetration = 0.0;
    int penetration_tick = -1;
    BodyHandle penetration_body{};
    BodyHandle penetration_other{};

    for (int tick = 1; tick <= result.ticks; ++tick) {
        watched_step(world, tick);
        sample_world_metrics(result, world.metrics(), timings);
        if (!record_state_scan(result, world.states(), tick, planet_radius)) {
            break;
        }
        double total_energy = 0.0;
        bool invalid_tick = false;
        std::vector<BodyState> measured_states;
        if (tick > 1500) {
            measured_states.reserve(blocks.size());
        }
        for (const BodyHandle block : blocks) {
            const auto state = world.state(block);
            if (!state || !is_finite(state->transform.position)
                || !is_finite(state->linear_velocity) || !is_finite(state->angular_velocity)) {
                add_violation(
                    result, "invalid_state", "stability-pile state is missing or non-finite", tick, block);
                invalid_tick = true;
                break;
            }
            const double linear_speed = length(state->linear_velocity);
            if (linear_speed > 100.0) {
                add_violation(
                    result,
                    "spontaneous_speed",
                    "stability-pile body exceeded 100 m/s",
                    tick,
                    block,
                    {{"linear_speed", linear_speed, "m/s"}});
                invalid_tick = true;
                break;
            }
            const double radius = length(state->transform.position);
            if (radius > 6.0 * planet_radius && !state->ejected) {
                add_violation(
                    result,
                    "outside_world_without_ejection",
                    "stability-pile body crossed 6R without valid ejection",
                    tick,
                    block,
                    {{"radius", radius, "m"}});
                invalid_tick = true;
                break;
            }
            total_energy += cube_kinetic_energy(*state, cube_side);
            if (tick > 1500) {
                measured_linear_speeds.push_back(linear_speed);
                measured_angular_speeds.push_back(length(state->angular_velocity));
                sleeping_samples += !state->awake;
                ++total_samples;
                measured_states.push_back(*state);
            }
        }
        if (invalid_tick) {
            break;
        }
        if (tick == 600) {
            result.energy_at_tick_600 = total_energy;
        } else if (tick > 600) {
            maximum_energy_after_600 = std::max(maximum_energy_after_600, total_energy);
        }
        if (tick > 1500) {
            const auto platform_state = world.state(platform.value);
            if (!platform_state) {
                add_violation(
                    result,
                    "invalid_state",
                    "stability-pile platform state is unavailable",
                    tick,
                    platform.value);
                break;
            }
            const OrientedBox platform_box =
                oriented_box(*platform_state, {4, 0.25f, 4});
            for (std::size_t first = 0; first < measured_states.size(); ++first) {
                const OrientedBox first_box =
                    oriented_box(measured_states[first], {0.25f, 0.25f, 0.25f});
                const double platform_penetration =
                    obb_penetration(first_box, platform_box);
                if (platform_penetration > result.max_penetration) {
                    result.max_penetration = platform_penetration;
                    penetration_tick = tick;
                    penetration_body = blocks[first];
                    penetration_other = platform.value;
                }
                maximum_platform_penetration =
                    std::max(maximum_platform_penetration, platform_penetration);
                for (std::size_t second = first + 1; second < measured_states.size(); ++second) {
                    const OrientedBox second_box =
                        oriented_box(measured_states[second], {0.25f, 0.25f, 0.25f});
                    const double pair_penetration =
                        obb_penetration(first_box, second_box);
                    if (pair_penetration > result.max_penetration) {
                        result.max_penetration = pair_penetration;
                        penetration_tick = tick;
                        penetration_body = blocks[first];
                        penetration_other = blocks[second];
                    }
                    maximum_pair_penetration =
                        std::max(maximum_pair_penetration, pair_penetration);
                }
            }
        }
    }

    finish_timings(result, std::move(timings));
    if (!result.violations.empty()) {
        return result;
    }
    std::ranges::sort(measured_linear_speeds);
    std::ranges::sort(measured_angular_speeds);
    result.p95_linear_speed = percentile(measured_linear_speeds, 0.95);
    result.p95_angular_speed = percentile(measured_angular_speeds, 0.95);
    result.sleep_ratio = total_samples == 0
        ? 0.0
        : static_cast<double>(sleeping_samples) / static_cast<double>(total_samples);
    result.max_energy_growth_ratio = std::max(
        0.0,
        (maximum_energy_after_600 - result.energy_at_tick_600)
            / std::max(1.0e-9, result.energy_at_tick_600));
    result.final_hash = hash_states(world.states());
    result.metrics = {
        {"sleep_ratio", result.sleep_ratio, "ratio"},
        {"p95_linear_speed", result.p95_linear_speed, "m/s"},
        {"p95_angular_speed", result.p95_angular_speed, "rad/s"},
        {"max_penetration", result.max_penetration, "m"},
        {"max_platform_penetration", maximum_platform_penetration, "m"},
        {"max_pair_penetration", maximum_pair_penetration, "m"},
        {"minimum_density", result.minimum_density, "kg/m3"},
        {"maximum_density", result.maximum_density, "kg/m3"},
        {"energy_at_tick_600", result.energy_at_tick_600, "J"},
        {"max_energy_growth_ratio", result.max_energy_growth_ratio, "ratio"},
    };

    const auto fixed_gate = [&](bool failed,
                                const char* code,
                                const char* message,
                                double value,
                                const char* unit) {
        if (failed) {
            add_violation(
                result, code, message, result.ticks, {}, {{code, value, unit}});
        }
    };
    fixed_gate(
        result.sleep_ratio < sleep_limit,
        (code_prefix + "_sleep_ratio").c_str(),
        "too few measured body samples slept",
        result.sleep_ratio,
        "ratio");
    fixed_gate(
        result.p95_linear_speed >= linear_limit,
        (code_prefix + "_linear_p95").c_str(),
        "p95 linear speed exceeded the fixed limit",
        result.p95_linear_speed,
        "m/s");
    fixed_gate(
        result.p95_angular_speed >= angular_limit,
        (code_prefix + "_angular_p95").c_str(),
        "p95 angular speed exceeded the fixed limit",
        result.p95_angular_speed,
        "rad/s");
    fixed_gate(
        result.max_penetration >= penetration_limit,
        (code_prefix + "_penetration").c_str(),
        "estimated maximum penetration exceeded the fixed limit",
        result.max_penetration,
        "m");
    if (result.max_penetration >= penetration_limit && !result.violations.empty()) {
        ScenarioViolation& violation = result.violations.back();
        violation.tick = penetration_tick;
        violation.handle = penetration_body;
        violation.values.push_back(
            {"other_handle_index", static_cast<double>(penetration_other.index), "index"});
        violation.values.push_back(
            {"other_handle_generation", static_cast<double>(penetration_other.generation), "generation"});
        violation.values.push_back(
            {"platform_penetration", maximum_platform_penetration, "m"});
        violation.values.push_back({"pair_penetration", maximum_pair_penetration, "m"});
    }
    if (!mass_ratio) {
        fixed_gate(
            result.max_energy_growth_ratio > 0.02,
            "radial_pile_energy_growth",
            "kinetic energy grew more than two percent after tick 600",
            result.max_energy_growth_ratio,
            "ratio");
    }
    return result;
}

[[nodiscard]] std::optional<ProcessMemorySample> process_memory_sample()
{
#ifdef _WIN32
    PROCESS_MEMORY_COUNTERS_EX counters{};
    counters.cb = sizeof(counters);
    if (GetProcessMemoryInfo(
            GetCurrentProcess(),
            reinterpret_cast<PROCESS_MEMORY_COUNTERS*>(&counters),
            sizeof(counters)) == FALSE) {
        return std::nullopt;
    }
    return ProcessMemorySample{
        .private_usage_bytes = static_cast<std::size_t>(counters.PrivateUsage),
        .working_set_bytes = static_cast<std::size_t>(counters.WorkingSetSize),
        .peak_working_set_bytes = static_cast<std::size_t>(counters.PeakWorkingSetSize),
    };
#else
    return std::nullopt;
#endif
}

[[nodiscard]] constexpr detail::RuntimeConfiguration current_runtime_configuration()
{
    constexpr bool release_build = std::string_view{NINHO_BUILD_TYPE} == "Release";
#ifdef _MT
    constexpr bool mt_defined = true;
#else
    constexpr bool mt_defined = false;
#endif
#ifdef _DLL
    constexpr bool dll_defined = true;
#else
    constexpr bool dll_defined = false;
#endif
#ifdef _DEBUG
    constexpr bool debug_defined = true;
#else
    constexpr bool debug_defined = false;
#endif
    return {
        .release_build = release_build,
        .mt_defined = mt_defined,
        .dll_defined = dll_defined,
        .debug_defined = debug_defined,
    };
}

[[nodiscard]] std::string_view private_commit_status_name(PrivateCommitStatus status)
{
    switch (status) {
    case PrivateCommitStatus::Pass:
        return "pass";
    case PrivateCommitStatus::Unavailable:
        return "unavailable";
    case PrivateCommitStatus::Unstable:
        return "unstable";
    case PrivateCommitStatus::Growth:
        return "growth";
    }
    return "unavailable";
}

struct StressCycleResult {
    bool passed{};
    std::string code;
    std::string detail;
    int tick{-1};
    BodyHandle handle{};
    std::uint64_t final_hash{};
    std::vector<double> timings;
    int peak_awake_count{};
    int peak_contact_count{};
    int peak_body_count{};
    int peak_shape_count{};
    int peak_joint_count{};
    int executed_substeps{};
};

[[nodiscard]] StressCycleResult execute_stress_cycle(
    int warmup_ticks, int measurement_ticks, bool collect_timings, int substeps)
{
    constexpr int body_count = 500;
    constexpr int shape_count = 800;
    constexpr int joint_count = 250;
    StressCycleResult cycle;
    PhysicsWorld world(WorldConfig{
        .substeps = substeps,
        .planet_radius = 10,
        .surface_gravity = 0,
        .max_bodies = body_count,
    });
    cycle.executed_substeps = world.config().substeps;

    std::vector<BodyHandle> bodies;
    bodies.reserve(body_count);
    for (int index = 0; index < body_count; ++index) {
        const int pair = index / 2;
        const float base_x = static_cast<float>(pair % 10) * 3.0f;
        const float base_y = static_cast<float>((pair / 10) % 5) * 3.0f;
        const float base_z = static_cast<float>(pair / 50) * 3.0f;
        BodyDesc body{
            .type = BodyType::Dynamic,
            .transform = {
                {base_x + static_cast<float>(index % 2), base_y, base_z}, {}},
            .radial_gravity = false,
            .remove_beyond_six_r = false,
        };
        const int shapes_on_body = index < 300 ? 2 : 1;
        for (int shape = 0; shape < shapes_on_body; ++shape) {
            const float local_x = shapes_on_body == 1
                ? 0.0f
                : (shape == 0 ? -0.12f : 0.12f);
            body.shapes.push_back(ShapeDesc{
                .geometry = BoxShape{{0.10f, 0.10f, 0.10f}, {{local_x, 0, 0}, {}}},
                .density = 500.0f,
            });
        }
        const auto created = world.create_body(body);
        if (!created) {
            cycle.code = "stress_create_body";
            cycle.detail = created.status.message;
            cycle.handle = created.value;
            return cycle;
        }
        bodies.push_back(created.value);
    }

    std::vector<JointHandle> joints;
    joints.reserve(joint_count);
    for (int index = 0; index < joint_count; ++index) {
        const auto created = world.create_joint(DistanceJointDesc{
            .a = bodies[static_cast<std::size_t>(index) * 2],
            .b = bodies[static_cast<std::size_t>(index) * 2 + 1],
            .length = 1.0f,
        });
        if (!created) {
            cycle.code = "stress_create_joint";
            cycle.detail = created.status.message;
            return cycle;
        }
        joints.push_back(created.value);
    }

    const int total_ticks = warmup_ticks + measurement_ticks;
    if (collect_timings) {
        cycle.timings.reserve(measurement_ticks);
    }
    for (int tick = 1; tick <= total_ticks; ++tick) {
        watched_step(world, tick);
        const WorldMetrics metrics = world.metrics();
        cycle.peak_body_count = std::max(cycle.peak_body_count, metrics.body_count);
        cycle.peak_shape_count = std::max(cycle.peak_shape_count, metrics.shape_count);
        cycle.peak_joint_count = std::max(cycle.peak_joint_count, metrics.joint_count);
        cycle.peak_awake_count = std::max(cycle.peak_awake_count, metrics.awake_count);
        cycle.peak_contact_count = std::max(cycle.peak_contact_count, metrics.contact_count);
        if (tick == 1
            && (metrics.body_count != body_count || metrics.shape_count != shape_count
                || metrics.joint_count != joint_count)) {
            cycle.code = "stress_topology";
            cycle.detail = "world metrics do not match 500 bodies, 800 shapes, and 250 joints";
            cycle.tick = tick;
            return cycle;
        }
        if (collect_timings && tick > warmup_ticks) {
            cycle.timings.push_back(metrics.step_ms);
        }
        if (const auto failure = scan_body_states(world.states(), 10.0)) {
            cycle.code = "fatal_state_invariant";
            cycle.detail = failure->invariant.field + '='
                + exact_value_text(failure->invariant.value);
            cycle.tick = tick;
            cycle.handle = failure->handle;
            return cycle;
        }
    }
    cycle.final_hash = hash_states(world.states());

    for (const JointHandle joint : joints) {
        if (!world.destroy_joint(joint).ok()) {
            cycle.code = "stress_destroy_joint";
            cycle.detail = "failed to queue stress joint destruction";
            return cycle;
        }
    }
    for (const BodyHandle body : bodies) {
        if (!world.destroy_body(body).ok()) {
            cycle.code = "stress_destroy_body";
            cycle.detail = "failed to queue stress body destruction";
            cycle.handle = body;
            return cycle;
        }
    }
    watched_step(world, total_ticks + 1);
    const WorldMetrics empty_metrics = world.metrics();
    if (empty_metrics.body_count != 0 || empty_metrics.shape_count != 0
        || empty_metrics.joint_count != 0 || !world.states().empty()) {
        cycle.code = "stress_incomplete_destroy";
        cycle.detail = "stress create/simulate/destroy cycle retained live resources";
        return cycle;
    }
    cycle.passed = true;
    return cycle;
}

struct CrtStressProbeResult {
    CrtMemoryObservation memory;
    bool workload_passed{true};
};

[[nodiscard]] CrtStressProbeResult probe_debug_crt_stress_cycle(
    int warmup_ticks, int measurement_ticks, int substeps)
{
    CrtStressProbeResult result;
#if defined(_MSC_VER) && defined(_DEBUG)
    static_cast<void>(debug_crt_allocation_probe(false));
    result.memory.applicable = true;
    _CrtMemState before{};
    _CrtMemState after{};
    _CrtMemState difference{};
    _CrtMemCheckpoint(&before);
    {
        const StressCycleResult cycle = execute_stress_cycle(
            warmup_ticks, measurement_ticks, false, substeps);
        result.workload_passed = cycle.passed;
    }
    _CrtMemCheckpoint(&after);
    _CrtMemDifference(&difference, &before, &after);
    result.memory.normal_block_count_delta =
        static_cast<std::int64_t>(difference.lCounts[_NORMAL_BLOCK]);
    result.memory.normal_block_bytes_delta =
        static_cast<std::int64_t>(difference.lSizes[_NORMAL_BLOCK]);
    result.memory.client_block_count_delta =
        static_cast<std::int64_t>(difference.lCounts[_CLIENT_BLOCK]);
    result.memory.client_block_bytes_delta =
        static_cast<std::int64_t>(difference.lSizes[_CLIENT_BLOCK]);
    result.memory.balanced = result.memory.normal_block_count_delta == 0
        && result.memory.normal_block_bytes_delta == 0
        && result.memory.client_block_count_delta == 0
        && result.memory.client_block_bytes_delta == 0;
#else
    static_cast<void>(warmup_ticks);
    static_cast<void>(measurement_ticks);
    static_cast<void>(substeps);
#endif
    return result;
}

[[nodiscard]] ScenarioResult run_stress(std::uint64_t seed, int substeps)
{
    ScenarioResult result = make_result(ScenarioKind::Stress, seed, substeps);
    result.box3d_allocator.baseline_bytes = detail::box3d_allocator_byte_count();
    result.box3d_allocator.final_bytes = result.box3d_allocator.baseline_bytes;
    result.box3d_allocator.exact_return =
        result.box3d_allocator.baseline_bytes == 0;
    result.dynamic_body_count = 500;
    result.shape_count = 800;
    result.joint_count = 250;
    result.allocator_warmup_cycles = 10;
    result.warmup_ticks = 300;
    result.measurement_ticks = 1200;
    result.limits = {
        {"allocator_warmup_cycles", "==", 10.0, "cycles"},
        {"scenario_timeout", "<=", 60.0, "s"},
        {"spontaneous_speed", "<=", 100.0, "m/s"},
    };

    std::vector<double> timings;
    // Commit the timing buffer before the memory baseline so its fixed storage
    // cannot appear later as persistent growth during the measured cycles.
    timings.resize(static_cast<std::size_t>(result.measurement_ticks) * 10);
    std::ranges::fill(timings, 0.0);
    timings.clear();

    const auto protocol_start = std::chrono::steady_clock::now();
    int executed_substeps = 0;
    result.private_commit_warmup_samples.reserve(
        static_cast<std::size_t>(result.allocator_warmup_cycles));
    result.working_set_warmup_samples.reserve(
        static_cast<std::size_t>(result.allocator_warmup_cycles));
    const auto record_memory = [&](bool warmup) {
        const auto sample = process_memory_sample();
        if (!sample) {
            return;
        }
        auto& private_samples = warmup ? result.private_commit_warmup_samples
                                       : result.private_commit_cycle_samples;
        auto& working_samples = warmup ? result.working_set_warmup_samples
                                       : result.working_set_cycle_samples;
        private_samples.push_back(sample->private_usage_bytes);
        working_samples.push_back(sample->working_set_bytes);
        result.private_commit_peak_bytes = std::max(
            result.private_commit_peak_bytes, sample->private_usage_bytes);
        result.working_set_peak_bytes = std::max(
            result.working_set_peak_bytes, sample->peak_working_set_bytes);
    };
    const auto record_allocator = [&](bool warmup) {
        const std::int64_t bytes = detail::box3d_allocator_byte_count();
        auto& samples = warmup
            ? result.box3d_allocator.warmup_post_teardown_bytes
            : result.box3d_allocator.measured_post_teardown_bytes;
        samples.push_back(bytes);
        result.box3d_allocator.final_bytes = bytes;
        const std::int64_t delta = bytes - result.box3d_allocator.baseline_bytes;
        result.box3d_allocator.max_abs_delta = std::max(
            result.box3d_allocator.max_abs_delta, std::abs(delta));
        result.box3d_allocator.exact_return =
            result.box3d_allocator.exact_return && delta == 0;
        return delta == 0;
    };
    if (!result.box3d_allocator.exact_return) {
        add_violation(
            result,
            "box3d_allocator_imbalance",
            "Box3 allocator baseline was not zero before Stress",
            0,
            {},
            {{"baseline_bytes",
              static_cast<double>(result.box3d_allocator.baseline_bytes),
              "bytes"}});
        return result;
    }
    for (int warmup_cycle = 0;
         warmup_cycle < result.allocator_warmup_cycles;
         ++warmup_cycle) {
        watchdog_checkpoint(
            warmup_cycle * (result.warmup_ticks + result.measurement_ticks + 1));
        const StressCycleResult allocator_warmup = execute_stress_cycle(
            result.warmup_ticks, result.measurement_ticks, false, substeps);
        watchdog_checkpoint(
            (warmup_cycle + 1) * (result.warmup_ticks + result.measurement_ticks + 1));
        if (!allocator_warmup.passed) {
            add_violation(
                result,
                allocator_warmup.code,
                "allocator warmup failed: " + allocator_warmup.detail,
                allocator_warmup.tick,
                allocator_warmup.handle,
                {{"warmup_cycle", static_cast<double>(warmup_cycle + 1), "count"}});
            return result;
        }
        executed_substeps = allocator_warmup.executed_substeps;
        if (executed_substeps != substeps) {
            add_violation(
                result,
                "stress_substeps_mismatch",
                "stress world did not execute the requested substep count",
                0,
                {},
                {{"requested", static_cast<double>(substeps), "count"},
                 {"executed", static_cast<double>(executed_substeps), "count"}});
            return result;
        }
        const bool allocator_balanced = record_allocator(true);
        record_memory(true);
        if (!allocator_balanced) {
            add_violation(
                result,
                "box3d_allocator_imbalance",
                "Box3 allocator did not return to baseline after a Stress warmup cycle",
                0,
                {},
                {{"warmup_cycle", static_cast<double>(warmup_cycle + 1), "count"},
                 {"allocator_bytes",
                  static_cast<double>(result.box3d_allocator.final_bytes),
                  "bytes"}});
            return result;
        }
    }

    const CrtStressProbeResult crt_probe = probe_debug_crt_stress_cycle(
        result.warmup_ticks, result.measurement_ticks, substeps);
    result.crt = crt_probe.memory;
    const std::int64_t after_crt_allocator = detail::box3d_allocator_byte_count();
    if (result.box3d_allocator.baseline_bytes != after_crt_allocator) {
        result.box3d_allocator.final_bytes = after_crt_allocator;
        result.box3d_allocator.exact_return = false;
        result.box3d_allocator.max_abs_delta = std::max(
            result.box3d_allocator.max_abs_delta,
            std::abs(after_crt_allocator - result.box3d_allocator.baseline_bytes));
        add_violation(
            result,
            "box3d_allocator_imbalance",
            "Box3 allocator did not return after the Debug CRT probe cycle",
            0,
            {});
        return result;
    }
    if (!crt_probe.workload_passed) {
        add_violation(
            result,
            "crt_probe_workload_failed",
            "Debug CRT probe workload failed before memory comparison",
            0,
            {});
        return result;
    }
    if (result.crt.applicable && !result.crt.balanced) {
        add_violation(
            result,
            "crt_live_block_imbalance",
            "Debug CRT live normal/client blocks did not return exactly to zero",
            0,
            {},
            {{"normal_count_delta",
              static_cast<double>(result.crt.normal_block_count_delta),
              "count"},
             {"normal_bytes_delta",
              static_cast<double>(result.crt.normal_block_bytes_delta),
              "bytes"},
             {"client_count_delta",
              static_cast<double>(result.crt.client_block_count_delta),
              "count"},
             {"client_bytes_delta",
              static_cast<double>(result.crt.client_block_bytes_delta),
              "bytes"}});
        return result;
    }

    result.private_commit_cycle_samples.reserve(10);
    result.working_set_cycle_samples.reserve(10);
    for (int cycle_index = 0; cycle_index < 10; ++cycle_index) {
        watchdog_checkpoint(result.ticks);
        StressCycleResult cycle = execute_stress_cycle(
            result.warmup_ticks, result.measurement_ticks, true, substeps);
        if (!cycle.passed) {
            add_violation(
                result,
                cycle.code,
                "stress cycle failed: " + cycle.detail,
                cycle.tick,
                cycle.handle,
                {{"cycle", static_cast<double>(cycle_index + 1), "count"}});
            break;
        }
        executed_substeps = cycle.executed_substeps;
        if (executed_substeps != substeps) {
            add_violation(
                result,
                "stress_substeps_mismatch",
                "stress cycle did not execute the requested substep count",
                result.ticks,
                {},
                {{"requested", static_cast<double>(substeps), "count"},
                 {"executed", static_cast<double>(executed_substeps), "count"}});
            break;
        }
        ++result.stress_cycles;
        result.ticks += result.warmup_ticks + result.measurement_ticks;
        watchdog_checkpoint(result.ticks);
        result.final_hash = cycle.final_hash;
        result.peak_body_count =
            std::max(result.peak_body_count, cycle.peak_body_count);
        result.peak_shape_count =
            std::max(result.peak_shape_count, cycle.peak_shape_count);
        result.peak_joint_count =
            std::max(result.peak_joint_count, cycle.peak_joint_count);
        result.peak_awake_count =
            std::max(result.peak_awake_count, cycle.peak_awake_count);
        result.peak_contact_count =
            std::max(result.peak_contact_count, cycle.peak_contact_count);
        timings.insert(
            timings.end(),
            std::make_move_iterator(cycle.timings.begin()),
            std::make_move_iterator(cycle.timings.end()));
        const bool allocator_balanced = record_allocator(false);
        record_memory(false);
        if (!allocator_balanced) {
            add_violation(
                result,
                "box3d_allocator_imbalance",
                "Box3 allocator did not return to baseline after a measured Stress cycle",
                result.ticks,
                {},
                {{"cycle", static_cast<double>(cycle_index + 1), "count"},
                 {"allocator_bytes",
                  static_cast<double>(result.box3d_allocator.final_bytes),
                  "bytes"}});
            break;
        }

        const double elapsed_seconds = std::chrono::duration<double>(
                                           std::chrono::steady_clock::now() - protocol_start)
                                           .count();
        if (ScenarioRunner::watchdog_timeout_seconds(ScenarioKind::Stress)
                == ScenarioRunner::watchdog_timeout_seconds()
            && elapsed_seconds
                > static_cast<double>(ScenarioRunner::watchdog_timeout_seconds())) {
            add_violation(
                result,
                "stress_timeout",
                "stress protocol exceeded the fixed 60 second timeout",
                result.ticks,
                {},
                {{"elapsed", elapsed_seconds, "s"}});
            break;
        }
    }
    finish_timings(result, std::move(timings));
    const PrivateCommitAssessment private_assessment = assess_private_commit(
        result.private_commit_warmup_samples,
        result.private_commit_cycle_samples);
    result.private_commit_available = private_assessment.available;
    result.private_commit_stable = private_assessment.stable;
    result.private_commit_terminal_growth = private_assessment.terminal_growth;
    result.private_commit_baseline_full_min_bytes = private_assessment.baseline_full_min_bytes;
    result.private_commit_baseline_central_min_bytes = private_assessment.baseline_central_min_bytes;
    result.private_commit_baseline_median_bytes = private_assessment.baseline_median_bytes;
    result.private_commit_baseline_central_max_bytes = private_assessment.baseline_central_max_bytes;
    result.private_commit_baseline_full_max_bytes = private_assessment.baseline_full_max_bytes;
    result.private_commit_final_full_min_bytes = private_assessment.final_full_min_bytes;
    result.private_commit_final_central_min_bytes = private_assessment.final_central_min_bytes;
    result.private_commit_final_median_bytes = private_assessment.final_median_bytes;
    result.private_commit_final_central_max_bytes = private_assessment.final_central_max_bytes;
    result.private_commit_final_full_max_bytes = private_assessment.final_full_max_bytes;
    result.private_commit_growth_ratio = private_assessment.growth_ratio;
    result.private_commit_instant_growth_ratio =
        private_assessment.instant_growth_ratio;
    result.private_commit_warmup_trimmed_span_ratio = private_assessment.warmup_trimmed_span_ratio;
    result.private_commit_warmup_full_span_ratio = private_assessment.warmup_full_span_ratio;
    result.private_commit_measured_trimmed_span_ratio = private_assessment.measured_trimmed_span_ratio;
    result.private_commit_measured_full_span_ratio = private_assessment.measured_full_span_ratio;
    if (!private_assessment.available) {
        result.private_commit_warmup_samples.clear();
        result.private_commit_cycle_samples.clear();
        result.private_commit_peak_bytes = 0;
    } else if (!result.private_commit_warmup_samples.empty()) {
        result.private_commit_baseline_bytes =
            result.private_commit_warmup_samples.back();
    }
    if (private_assessment.available && !result.private_commit_cycle_samples.empty()) {
        result.private_commit_final_bytes = result.private_commit_cycle_samples.back();
    }
    result.private_commit_gate_scope = "release_mt";
    result.private_commit_assessment_status =
        std::string{private_commit_status_name(private_assessment.status)};
    result.private_commit_gate_applied = false;
    result.private_commit_gate_status = "diagnostic";
    result.private_commit_budget_qualified = false;
    result.private_commit_budget_scope = "future_packaged_reference_hardware";
    result.warnings.push_back({
        .code = "private_commit_budget_unqualified",
        .message = "PrivateUsage is diagnostic in the foundation; qualify the 5% budget in a packaged Release build on reference hardware",
        .details = {
            {"assessment", result.private_commit_assessment_status},
            {"budget_scope", result.private_commit_budget_scope},
        },
    });

    const PrivateCommitAssessment working_set_summary = assess_private_commit(
        result.working_set_warmup_samples,
        result.working_set_cycle_samples);
    result.working_set_gate_status = "diagnostic";
    result.working_set_assessment_status =
        std::string{private_commit_status_name(working_set_summary.status)};
    result.working_set_gate_applied = false;
    result.working_set_budget_qualified = false;
    result.working_set_budget_scope = "future_packaged_reference_hardware";
    result.working_set_available = working_set_summary.available;
    result.working_set_stable = working_set_summary.stable;
    result.working_set_terminal_growth = working_set_summary.terminal_growth;
    result.working_set_baseline_low_bytes = working_set_summary.baseline_full_min_bytes;
    result.working_set_baseline_central_low_bytes =
        working_set_summary.baseline_central_min_bytes;
    result.working_set_baseline_median_bytes = working_set_summary.baseline_median_bytes;
    result.working_set_baseline_central_high_bytes =
        working_set_summary.baseline_central_max_bytes;
    result.working_set_baseline_max_bytes = working_set_summary.baseline_full_max_bytes;
    result.working_set_final_low_bytes = working_set_summary.final_full_min_bytes;
    result.working_set_final_central_low_bytes =
        working_set_summary.final_central_min_bytes;
    result.working_set_final_median_bytes = working_set_summary.final_median_bytes;
    result.working_set_final_central_high_bytes =
        working_set_summary.final_central_max_bytes;
    result.working_set_final_max_bytes = working_set_summary.final_full_max_bytes;
    result.working_set_warmup_trimmed_span_ratio =
        working_set_summary.warmup_trimmed_span_ratio;
    result.working_set_warmup_full_span_ratio =
        working_set_summary.warmup_full_span_ratio;
    result.working_set_measured_trimmed_span_ratio =
        working_set_summary.measured_trimmed_span_ratio;
    result.working_set_measured_full_span_ratio =
        working_set_summary.measured_full_span_ratio;
    if (!working_set_summary.available) {
        result.working_set_warmup_samples.clear();
        result.working_set_cycle_samples.clear();
        result.working_set_peak_bytes = 0;
    } else if (!result.working_set_warmup_samples.empty()) {
        result.working_set_baseline_bytes = result.working_set_warmup_samples.back();
    }
    if (working_set_summary.available && !result.working_set_cycle_samples.empty()) {
        result.working_set_final_bytes = result.working_set_cycle_samples.back();
    }
    if (working_set_summary.available) {
        result.working_set_growth_ratio = working_set_summary.growth_ratio;
        result.working_set_instant_growth_ratio =
            working_set_summary.instant_growth_ratio;
    }

    result.metrics = {
        {"private_commit_diagnostic_growth_target", 0.05, "ratio"},
        {"private_commit_diagnostic_trimmed_span_target", 0.05, "ratio"},
        {"working_set_diagnostic_growth_target", 0.05, "ratio"},
        {"working_set_diagnostic_trimmed_span_target", 0.05, "ratio"},
        {"private_commit_baseline_median_bytes",
         static_cast<double>(result.private_commit_baseline_median_bytes),
         "bytes"},
        {"private_commit_final_median_bytes",
         static_cast<double>(result.private_commit_final_median_bytes),
         "bytes"},
        {"private_commit_growth_ratio", result.private_commit_growth_ratio, "ratio"},
        {"private_commit_instant_growth_ratio",
         result.private_commit_instant_growth_ratio,
         "ratio"},
        {"private_commit_warmup_trimmed_span_ratio",
         result.private_commit_warmup_trimmed_span_ratio,
         "ratio"},
        {"private_commit_measured_trimmed_span_ratio",
         result.private_commit_measured_trimmed_span_ratio,
         "ratio"},
        {"working_set_baseline_bytes",
         static_cast<double>(result.working_set_baseline_bytes),
         "bytes"},
        {"working_set_baseline_low_bytes",
         static_cast<double>(result.working_set_baseline_low_bytes),
         "bytes"},
        {"working_set_peak_bytes",
         static_cast<double>(result.working_set_peak_bytes),
         "bytes"},
        {"working_set_final_bytes",
         static_cast<double>(result.working_set_final_bytes),
         "bytes"},
        {"working_set_final_low_bytes",
         static_cast<double>(result.working_set_final_low_bytes),
         "bytes"},
        {"working_set_growth_ratio", result.working_set_growth_ratio, "ratio"},
        {"working_set_instant_growth_ratio",
         result.working_set_instant_growth_ratio,
         "ratio"},
        {"executed_substeps", static_cast<double>(executed_substeps), "count"},
        {"allocator_warmup_cycles",
         static_cast<double>(result.allocator_warmup_cycles),
         "cycles"},
        {"completed_cycles", static_cast<double>(result.stress_cycles), "count"},
    };
    return result;
}

[[nodiscard]] CapabilityRow prove_ccd_capability()
{
    const ProjectileGate gate = run_projectile_gate();
    CapabilityRow row{.capability = "ccd_dynamic_dynamic"};
    row.values = {
        {"primary_passes", static_cast<double>(gate.primary_pass_count), "seeds"},
        {"fallback_passes", static_cast<double>(gate.fallback_pass_count), "seeds"},
        {"primary_speed", 35.0, "m/s"},
        {"primary_substeps", 4.0, "count"},
        {"fallback_speed", 30.0, "m/s"},
        {"fallback_substeps", 6.0, "count"},
        {"fallback_evaluated", gate.uses_fallback ? 1.0 : 0.0, "bool"},
        {"primary_invalid_states", static_cast<double>(gate.primary_invalid_count), "count"},
        {"fallback_invalid_states", static_cast<double>(gate.fallback_invalid_count), "count"},
    };
    const auto& outcomes = gate.primary_pass_count == 20 ? gate.primary : gate.fallback;
    for (const ProjectileOutcome& outcome : outcomes) {
        row.fixture_hashes.push_back(outcome.final_hash);
        merge_capability_metrics(row, outcome);
    }
    if (gate.primary_invalid_count != 0 || gate.fallback_invalid_count != 0) {
        row.status = CapabilityStatus::Blocked;
        row.detail = "a fixed CCD seed produced an invalid body state";
    } else if (gate.primary_pass_count == 20) {
        row.status = CapabilityStatus::Pass;
        row.detail = "all 20 fixed seeds contacted a dynamic pile body at 35 m/s and four substeps";
    } else if (gate.fallback_pass_count == 20) {
        row.status = CapabilityStatus::Fallback;
        row.fallback = "speed30_substeps6";
        row.detail = "the complete fallback set passed 20 of 20 fixed seeds";
    } else {
        row.status = CapabilityStatus::Blocked;
        row.detail = "dynamic-dynamic CCD tunneled in both approved parameter sets";
    }
    return row;
}

[[nodiscard]] CapabilityRow prove_shape_query()
{
    CapabilityRow row{.capability = "shape_cast_overlap"};
    PhysicsWorld world(WorldConfig{.surface_gravity = 0, .max_bodies = 1});
    BodyDesc target{.type = BodyType::Static};
    target.shapes = {ShapeDesc{
        .geometry = HullShape{{{-0.5f, -0.5f, -0.5f},
                               {0.5f, -0.5f, -0.5f},
                               {0, 0.5f, -0.5f},
                               {0, 0, 0.5f}},
                              {{0.2f, 0, 0}, {}}},
        .material_id = 303,
    }};
    const auto created = world.create_body(target);
    if (!created) {
        row.status = CapabilityStatus::Blocked;
        row.detail = "the transformed hull query fixture could not be created";
        return row;
    }
    watched_step(world, 1);
    sample_capability_metrics(row, world.metrics());
    if (const auto failure = scan_body_states(world.states(), 10.0)) {
        row.status = CapabilityStatus::Blocked;
        row.detail = "shape-query fixture state failed: " + failure->invariant.field
            + '=' + exact_value_text(failure->invariant.value);
        row.values = {{"invalid_handle", static_cast<double>(failure->handle.index), "index"}};
        return row;
    }
    row.fixture_hashes.push_back(hash_states(world.states()));
    const SphereShape query{.radius = 0.1f};
    constexpr Vec3 origin{0, 2.5f, 0};
    constexpr Vec3 translation{0, -3, 0};
    const auto hit = world.cast_shape(query, {origin, {}}, translation);
    bool overlap_matches = false;
    if (hit) {
        const Vec3 inside =
            origin + translation * std::min(1.0f, hit->fraction + 0.05f);
        const auto overlaps = world.overlap_shape(query, {inside, {}});
        overlap_matches = std::ranges::any_of(overlaps, [&](const QueryHit& value) {
            return value.body == created.value;
        });
    }
    const bool finite_hit = hit && hit->body == created.value
        && is_finite(hit->point) && is_finite(hit->normal)
        && std::isfinite(hit->fraction) && hit->material_id == 303;
    row.values = {
        {"cast_distance", 3.0, "m"},
        {"first_handle_index", hit ? static_cast<double>(hit->body.index) : 0.0, "index"},
        {"first_handle_matches_overlap", overlap_matches ? 1.0 : 0.0, "bool"},
        {"fraction", hit ? hit->fraction : -1.0, "ratio"},
    };
    if (finite_hit && overlap_matches) {
        row.status = CapabilityStatus::Pass;
        row.detail = "the first three-meter sphere cast handle agrees with overlap at contact";
    } else {
        row.status = CapabilityStatus::Blocked;
        row.detail = "cast and overlap do not provide one safe deterministic runtime result";
    }
    return row;
}

[[nodiscard]] CapabilityRow prove_contact_events()
{
    CapabilityRow row{.capability = "contact_hit_events"};
    PhysicsWorld world(WorldConfig{.substeps = 6, .surface_gravity = 0, .max_bodies = 2});
    BodyDesc light_desc = BodyDesc::dynamic_sphere(0.5f, {{-2, 5, 0}, {}}, 100);
    light_desc.linear_velocity = {10, 0, 0};
    light_desc.bullet = true;
    light_desc.shapes.front().material_id = 111;
    const auto light = world.create_body(light_desc);
    BodyDesc heavy_desc = BodyDesc::dynamic_sphere(0.5f, {{2, 5, 0}, {}}, 400);
    heavy_desc.linear_velocity = {-5, 0, 0};
    heavy_desc.bullet = true;
    heavy_desc.shapes.front().material_id = 222;
    const auto heavy = world.create_body(heavy_desc);
    if (!light || !heavy) {
        row.status = CapabilityStatus::Blocked;
        row.detail = "the asymmetric contact fixture could not be created";
        return row;
    }
    watched_step(world, 1);
    sample_capability_metrics(row, world.metrics());
    if (const auto failure = scan_body_states(world.states(), 10.0)) {
        row.status = CapabilityStatus::Blocked;
        row.detail = "contact fixture state failed: " + failure->invariant.field
            + '=' + exact_value_text(failure->invariant.value);
        return row;
    }
    const auto light_state = world.state(light.value);
    const auto heavy_state = world.state(heavy.value);
    for (int tick = 0; tick < 30 && world.contact_hits().empty(); ++tick) {
        watched_step(world, tick + 2);
        sample_capability_metrics(row, world.metrics());
        if (const auto failure = scan_body_states(world.states(), 10.0)) {
            row.status = CapabilityStatus::Blocked;
            row.detail = "contact fixture state failed: " + failure->invariant.field
                + '=' + exact_value_text(failure->invariant.value);
            return row;
        }
    }
    row.fixture_hashes.push_back(hash_states(world.states()));

    std::set<std::pair<BodyHandle, BodyHandle>> unique_pairs;
    bool pairs_are_unique = true;
    for (const ContactHit& hit : world.contact_hits()) {
        pairs_are_unique = pairs_are_unique && hit.a < hit.b
            && unique_pairs.insert({hit.a, hit.b}).second;
    }
    const auto selected = std::find_if(
        world.contact_hits().begin(),
        world.contact_hits().end(),
        [&](const ContactHit& hit) {
            return hit.a == std::min(light.value, heavy.value)
                && hit.b == std::max(light.value, heavy.value);
        });
    if (selected == world.contact_hits().end() || !light_state || !heavy_state) {
        row.status = CapabilityStatus::Blocked;
        row.detail = "no real asymmetric contact hit was copied by the public API";
        row.values = {{"unique_pairs", static_cast<double>(unique_pairs.size()), "count"}};
        return row;
    }

    double approach_speed = selected->approach_speed;
    double effective_mass = selected->effective_mass;
    double derived_energy = selected->derived_energy;
    const bool common_data_valid = pairs_are_unique && is_finite(selected->normal)
        && length(selected->normal) > 0.9f && is_finite(selected->point)
        && selected->material_a == 111 && selected->material_b == 222;
    const bool raw_numeric_valid = std::isfinite(approach_speed) && approach_speed > 0.0
        && std::isfinite(effective_mass) && effective_mass > 0.0
        && std::isfinite(derived_energy) && derived_energy > 0.0;
    if (common_data_valid && raw_numeric_valid) {
        row.status = CapabilityStatus::Pass;
        row.detail = "real hit data is finite, energetic, material-tagged, and pair-unique at six substeps";
    } else if (common_data_valid) {
        const Vec3 relative_velocity =
            heavy_state->linear_velocity - light_state->linear_velocity;
        approach_speed = std::abs(dot(relative_velocity, selected->normal));
        effective_mass = light_state->mass * heavy_state->mass
            / (light_state->mass + heavy_state->mass);
        const double damaging_speed = std::max(0.0, approach_speed - 1.0);
        derived_energy = 0.5 * effective_mass * damaging_speed * damaging_speed;
        if (approach_speed > 0.0 && effective_mass > 0.0 && derived_energy > 0.0
            && std::isfinite(derived_energy)) {
            row.status = CapabilityStatus::Fallback;
            row.fallback = "derived_relative_energy";
            row.detail = "energy was derived from copied body velocities with the approved equation";
        } else {
            row.status = CapabilityStatus::Blocked;
            row.detail = "relative energy could not be derived from the real contact pair";
        }
    } else {
        row.status = CapabilityStatus::Blocked;
        row.detail = "contact normal, materials, or ordered-pair deduplication is unusable";
    }
    row.values = {
        {"approach_speed", approach_speed, "m/s"},
        {"effective_mass", effective_mass, "kg"},
        {"derived_energy", derived_energy, "J"},
        {"normal_length", length(selected->normal), "ratio"},
        {"material_a", static_cast<double>(selected->material_a), "id"},
        {"material_b", static_cast<double>(selected->material_b), "id"},
        {"unique_pairs", static_cast<double>(unique_pairs.size()), "count"},
        {"substeps", 6.0, "count"},
    };
    return row;
}

[[nodiscard]] CapabilityRow prove_joint_reaction()
{
    CapabilityRow row{.capability = "joint_force_torque"};
    PhysicsWorld world(WorldConfig{.surface_gravity = 0, .max_bodies = 2});
    const auto anchor = world.create_body(
        BodyDesc::static_box({0.5f, 0.5f, 0.5f}, {{0, 0, 0}, {}}));
    const auto loaded = world.create_body(
        BodyDesc::dynamic_box({0.5f, 0.5f, 0.5f}, {{0, 1, 0}, {}}, 500));
    if (!anchor || !loaded) {
        row.status = CapabilityStatus::Blocked;
        row.detail = "joint reaction bodies could not be created";
        return row;
    }
    watched_step(world, 1);
    sample_capability_metrics(row, world.metrics());
    if (const auto failure = scan_body_states(world.states(), 10.0)) {
        row.status = CapabilityStatus::Blocked;
        row.detail = "joint fixture state failed: " + failure->invariant.field
            + '=' + exact_value_text(failure->invariant.value);
        return row;
    }
    const auto joint = world.create_joint(
        DistanceJointDesc{.a = anchor.value, .b = loaded.value, .length = 1});
    if (!joint) {
        row.status = CapabilityStatus::Blocked;
        row.detail = "distance joint could not be created";
        return row;
    }
    float previous = 0.0f;
    float maximum_force = 0.0f;
    bool monotonic = true;
    bool finite = true;
    double maximum_deformation = 0.0;
    std::vector<double> deformation_samples;
    constexpr double deformation_threshold = 0.01;
    for (int load = 1000; load <= 12000; load += 1000) {
        world.apply_force(loaded.value, {static_cast<float>(load), 0, 0}, {0, 1, 0});
        watched_step(world, load / 1000 + 1);
        sample_capability_metrics(row, world.metrics());
        if (const auto failure = scan_body_states(world.states(), 10.0)) {
            row.status = CapabilityStatus::Blocked;
            row.detail = "joint fixture state failed: " + failure->invariant.field
                + '=' + exact_value_text(failure->invariant.value);
            return row;
        }
        const auto anchor_state = world.state(anchor.value);
        const auto loaded_state = world.state(loaded.value);
        if (!anchor_state || !loaded_state) {
            row.status = CapabilityStatus::Blocked;
            row.detail = "joint fixture body state disappeared";
            return row;
        }
        const double relative_deformation = std::abs(
            static_cast<double>(length(
                loaded_state->transform.position - anchor_state->transform.position))
            - 1.0);
        deformation_samples.push_back(relative_deformation);
        maximum_deformation = std::max(maximum_deformation, relative_deformation);
        const auto reaction = world.joint_reaction(joint.value);
        if (!reaction || !is_finite(reaction->force) || !is_finite(reaction->torque)
            || !std::isfinite(reaction->linear_separation)
            || !std::isfinite(reaction->angular_separation)) {
            finite = false;
            continue;
        }
        const float current = length(reaction->force);
        monotonic = monotonic && current + 50.0f >= previous;
        previous = current;
        maximum_force = std::max(maximum_force, current);
    }
    row.fixture_hashes.push_back(hash_states(world.states()));
    const bool consecutive_deformation =
        has_two_consecutive_samples(deformation_samples, deformation_threshold);
    if (finite && monotonic && maximum_force > 10000.0f) {
        row.status = CapabilityStatus::Pass;
        row.detail = "joint reaction grows within 50 N tolerance and crosses 10 kN";
    } else if (consecutive_deformation) {
        row.status = CapabilityStatus::Fallback;
        row.fallback = "relative_deformation_two_ticks";
        row.detail = "joint rupture can be evaluated from relative deformation over two ticks";
    } else {
        row.status = CapabilityStatus::Blocked;
        row.detail = "no public joint metric supports a predictable rupture threshold";
    }
    row.values = {
        {"monotonic_tolerance", 50.0, "N"},
        {"maximum_force", maximum_force, "N"},
        {"rupture_threshold", 10000.0, "N"},
        {"maximum_deformation", maximum_deformation, "m"},
        {"deformation_threshold", deformation_threshold, "m"},
        {"consecutive_deformation_ticks", consecutive_deformation ? 2.0 : 0.0, "ticks"},
    };
    return row;
}

[[nodiscard]] BodyDesc eight_hull_body(bool compound)
{
    std::vector<PrimitiveShape> children;
    children.reserve(8);
    for (int index = 0; index < 8; ++index) {
        children.push_back(HullShape{
            {{-0.2f, -0.2f, -0.2f},
             {0.2f, -0.2f, -0.2f},
             {0, 0.2f, -0.2f},
             {0, 0, 0.2f}},
            {{(static_cast<float>(index) - 3.5f) * 0.4f, 0, 0}, {}},
        });
    }
    BodyDesc body{
        .type = BodyType::Dynamic,
        .transform = {{0, 3, 0}, {}},
        .radial_gravity = false,
        .remove_beyond_six_r = false,
    };
    if (compound) {
        body.shapes.push_back(ShapeDesc{
            .geometry = CompoundShape{std::move(children)},
            .density = 500,
        });
    } else {
        for (PrimitiveShape& child : children) {
            std::visit(
                [&](auto&& geometry) {
                    body.shapes.push_back(ShapeDesc{
                        .geometry = std::move(geometry),
                        .density = 500,
                    });
                },
                std::move(child));
        }
    }
    return body;
}

[[nodiscard]] CapabilityRow prove_hulls_compounds()
{
    const std::int64_t allocator_baseline = detail::box3d_allocator_byte_count();
    CapabilityRow row{.capability = "hulls_compounds"};
    const auto exercise = [](bool compound) {
        struct Result {
            bool created{};
            bool valid_mass{};
            bool valid_bounds{};
            bool contacted{};
            bool state_valid{true};
            double mass{};
            Aabb bounds{};
            std::uint64_t fixture_hash{};
            int peak_body_count{};
            int peak_shape_count{};
            int peak_joint_count{};
            int peak_awake_count{};
            int peak_contact_count{};
        } result;
        const auto sample = [&](const WorldMetrics& metrics) {
            result.peak_body_count = std::max(result.peak_body_count, metrics.body_count);
            result.peak_shape_count = std::max(result.peak_shape_count, metrics.shape_count);
            result.peak_joint_count = std::max(result.peak_joint_count, metrics.joint_count);
            result.peak_awake_count = std::max(result.peak_awake_count, metrics.awake_count);
            result.peak_contact_count = std::max(result.peak_contact_count, metrics.contact_count);
        };
        constexpr double expected_mass = 42.666666666666664;
        constexpr double tolerance = 1.0e-4;
        constexpr Aabb expected_bounds{{-1.62f, 2.78f, -0.22f}, {1.62f, 3.22f, 0.22f}};
        PhysicsWorld world(WorldConfig{.surface_gravity = 0, .max_bodies = 2});
        const auto body = world.create_body(eight_hull_body(compound));
        const auto platform = world.create_body(
            BodyDesc::static_box({4, 0.25f, 4}, {{0, 0, 0}, {}}));
        if (!body || !platform) {
            return result;
        }
        result.created = true;
        watched_step(world, 1);
        sample(world.metrics());
        if (scan_body_states(world.states(), 10.0)) {
            result.state_valid = false;
            return result;
        }
        const auto state = world.state(body.value);
        const auto bounds = world.body_bounds(body.value);
        result.mass = state ? state->mass : 0.0;
        result.bounds = bounds.value_or(Aabb{});
        result.valid_mass = state
            && std::abs(static_cast<double>(state->mass) - expected_mass) <= tolerance;
        result.valid_bounds = bounds && is_finite(bounds->lower) && is_finite(bounds->upper)
            && std::abs(bounds->lower.x - expected_bounds.lower.x) <= tolerance
            && std::abs(bounds->lower.y - expected_bounds.lower.y) <= tolerance
            && std::abs(bounds->lower.z - expected_bounds.lower.z) <= tolerance
            && std::abs(bounds->upper.x - expected_bounds.upper.x) <= tolerance
            && std::abs(bounds->upper.y - expected_bounds.upper.y) <= tolerance
            && std::abs(bounds->upper.z - expected_bounds.upper.z) <= tolerance;
        for (int tick = 0; tick < 180 && world.contact_hits().empty(); ++tick) {
            world.apply_force(body.value, {0, -4000, 0}, {0, 3, 0});
            watched_step(world, tick + 2);
            sample(world.metrics());
            if (scan_body_states(world.states(), 10.0)) {
                result.state_valid = false;
                return result;
            }
        }
        result.contacted = std::ranges::any_of(world.contact_hits(), [&](const ContactHit& hit) {
            return hit.a == std::min(body.value, platform.value)
                && hit.b == std::max(body.value, platform.value);
        });
        result.fixture_hash = hash_states(world.states());
        return result;
    };

    auto proof = exercise(true);
    if (proof.created && proof.state_valid && proof.valid_mass && proof.valid_bounds
        && proof.contacted) {
        row.status = CapabilityStatus::Pass;
        row.detail = "one compound containing eight hull children has valid mass, bounds, and contact";
    } else if (!proof.created) {
        const auto fallback = exercise(false);
        proof = fallback;
        if (fallback.created && fallback.state_valid && fallback.valid_mass
            && fallback.valid_bounds && fallback.contacted) {
            row.status = CapabilityStatus::Fallback;
            row.fallback = "multiple_shapes_same_body";
            row.detail = "eight hulls pass as multiple shapes on the same body";
        } else {
            row.status = CapabilityStatus::Blocked;
            row.detail = "neither compound nor approved multi-shape fixture is safe";
        }
    } else {
        row.status = CapabilityStatus::Blocked;
        row.detail = "the primary compound was created but failed exact mass, bounds, state, or contact proof";
    }
    row.fixture_hashes.push_back(proof.fixture_hash);
    row.peak_body_count = proof.peak_body_count;
    row.peak_shape_count = proof.peak_shape_count;
    row.peak_joint_count = proof.peak_joint_count;
    row.peak_awake_count = proof.peak_awake_count;
    row.peak_contact_count = proof.peak_contact_count;
    row.values = {
        {"hull_count", 8.0, "count"},
        {"expected_mass", 42.666666666666664, "kg"},
        {"mass", proof.mass, "kg"},
        {"bounds_lower_x", proof.bounds.lower.x, "m"},
        {"bounds_lower_y", proof.bounds.lower.y, "m"},
        {"bounds_lower_z", proof.bounds.lower.z, "m"},
        {"bounds_upper_x", proof.bounds.upper.x, "m"},
        {"bounds_upper_y", proof.bounds.upper.y, "m"},
        {"bounds_upper_z", proof.bounds.upper.z, "m"},
        {"bounds_tolerance", 1.0e-4, "m"},
        {"contacted", proof.contacted ? 1.0 : 0.0, "bool"},
        {"state_valid", proof.state_valid ? 1.0 : 0.0, "bool"},
    };
    row.functional_status = row.status;
    row.functional_fallback = row.fallback;
    const std::int64_t allocator_final = detail::box3d_allocator_byte_count();
    row.values.push_back(
        {"box3d_allocator_baseline_bytes",
         static_cast<double>(allocator_baseline),
         "bytes"});
    row.values.push_back(
        {"box3d_allocator_final_bytes", static_cast<double>(allocator_final), "bytes"});
    if (allocator_baseline != allocator_final) {
        row.status = CapabilityStatus::Blocked;
        row.detail = "hull/compound proof retained Box3 allocator bytes";
    }
    return row;
}

[[nodiscard]] CapabilityRow prove_batch_lifecycle()
{
    const std::int64_t allocator_baseline = detail::box3d_allocator_byte_count();
    CapabilityRow row = [&] {
    CapabilityRow row{.capability = "batch_lifecycle"};
    PhysicsWorld world(WorldConfig{.surface_gravity = 0, .max_bodies = 1});
    BodyDesc body = BodyDesc::dynamic_sphere(0.25f, {}, 10);
    body.radial_gravity = false;
    body.remove_beyond_six_r = false;

    BodyHandle previous{};
    for (int warmup = 0; warmup < 64; ++warmup) {
        const auto created = world.create_body(body);
        if (!created) {
            row.status = CapabilityStatus::Blocked;
            row.detail = "allocator warmup creation failed";
            return row;
        }
        watched_step(world, warmup * 2 + 1);
        sample_capability_metrics(row, world.metrics());
        if (const auto failure = scan_body_states(world.states(), 10.0)) {
            row.status = CapabilityStatus::Blocked;
            row.detail = "lifecycle warmup state failed: " + failure->invariant.field
                + '=' + exact_value_text(failure->invariant.value);
            return row;
        }
        world.destroy_body(created.value);
        watched_step(world, warmup * 2 + 2);
        sample_capability_metrics(row, world.metrics());
        previous = created.value;
    }
    const auto baseline = process_memory_sample();
#if defined(_MSC_VER) && defined(_DEBUG)
    _CrtMemState crt_before{};
    _CrtMemState crt_after{};
    _CrtMemState crt_difference{};
    _CrtMemCheckpoint(&crt_before);
#endif
    int invalid_handles = 0;
    int completed_cycles = 0;
    for (int cycle = 0; cycle < 10000; ++cycle) {
        const auto created = world.create_body(body);
        if (!created) {
            ++invalid_handles;
            break;
        }
        if (previous.valid()
            && (created.value.index != previous.index
                || created.value.generation == previous.generation)) {
            ++invalid_handles;
        }
        watched_step(world, cycle * 2 + 1);
        sample_capability_metrics(row, world.metrics());
        if (const auto failure = scan_body_states(world.states(), 10.0)) {
            row.status = CapabilityStatus::Blocked;
            row.detail = "lifecycle state failed: " + failure->invariant.field
                + '=' + exact_value_text(failure->invariant.value);
            row.values = {
                {"generation_cycles", static_cast<double>(completed_cycles), "count"},
                {"invalid_handle_index", static_cast<double>(failure->handle.index), "index"},
            };
            return row;
        }
        if (!world.state(created.value)) {
            ++invalid_handles;
        }
        if (!world.destroy_body(created.value).ok()) {
            ++invalid_handles;
        }
        if (world.apply_impulse(created.value, {}, {}).code != StatusCode::InvalidHandle) {
            ++invalid_handles;
        }
        watched_step(world, cycle * 2 + 2);
        sample_capability_metrics(row, world.metrics());
        if (world.state(created.value)) {
            ++invalid_handles;
        }
        previous = created.value;
        ++completed_cycles;
    }
#if defined(_MSC_VER) && defined(_DEBUG)
    _CrtMemCheckpoint(&crt_after);
    _CrtMemDifference(&crt_difference, &crt_before, &crt_after);
    CrtMemoryObservation crt{
        .applicable = true,
        .balanced = crt_difference.lCounts[_NORMAL_BLOCK] == 0
            && crt_difference.lSizes[_NORMAL_BLOCK] == 0
            && crt_difference.lCounts[_CLIENT_BLOCK] == 0
            && crt_difference.lSizes[_CLIENT_BLOCK] == 0,
        .normal_block_count_delta = static_cast<std::int64_t>(
            crt_difference.lCounts[_NORMAL_BLOCK]),
        .normal_block_bytes_delta = static_cast<std::int64_t>(
            crt_difference.lSizes[_NORMAL_BLOCK]),
        .client_block_count_delta = static_cast<std::int64_t>(
            crt_difference.lCounts[_CLIENT_BLOCK]),
        .client_block_bytes_delta = static_cast<std::int64_t>(
            crt_difference.lSizes[_CLIENT_BLOCK]),
    };
#else
    CrtMemoryObservation crt;
#endif
    const auto final = process_memory_sample();
    const std::optional<double> private_growth = baseline && final
            && baseline->private_usage_bytes != 0 && final->private_usage_bytes != 0
        ? std::optional<double>{std::max(
            0.0,
            (static_cast<double>(final->private_usage_bytes)
             - static_cast<double>(baseline->private_usage_bytes))
                / static_cast<double>(baseline->private_usage_bytes))}
        : std::nullopt;
    row.values = {
        {"generation_cycles", static_cast<double>(completed_cycles), "count"},
        {"invalid_handles", static_cast<double>(invalid_handles), "count"},
        {"private_commit_available", private_growth ? 1.0 : 0.0, "bool"},
        {"private_commit_growth", private_growth.value_or(0.0), "ratio"},
        {"private_commit_baseline_bytes",
         static_cast<double>(baseline ? baseline->private_usage_bytes : 0),
         "bytes"},
        {"private_commit_final_bytes",
         static_cast<double>(final ? final->private_usage_bytes : 0),
         "bytes"},
        {"working_set_baseline_bytes",
         static_cast<double>(baseline ? baseline->working_set_bytes : 0),
         "bytes"},
        {"working_set_final_bytes",
         static_cast<double>(final ? final->working_set_bytes : 0),
         "bytes"},
        {"crt_normal_count_delta", static_cast<double>(crt.normal_block_count_delta), "count"},
        {"crt_normal_bytes_delta", static_cast<double>(crt.normal_block_bytes_delta), "bytes"},
        {"crt_client_count_delta", static_cast<double>(crt.client_block_count_delta), "count"},
        {"crt_client_bytes_delta", static_cast<double>(crt.client_block_bytes_delta), "bytes"},
    };
    row.fixture_hashes.push_back(
        (static_cast<std::uint64_t>(previous.generation) << 32) | previous.index);
    row.functional_status = classify_lifecycle_status(
        completed_cycles, invalid_handles, private_growth);
    row.status = row.functional_status;
    if (crt.applicable && !crt.balanced) {
        row.status = CapabilityStatus::Blocked;
    }
    if (row.status == CapabilityStatus::Pass) {
        row.detail = "10,000 create-destroy cycles preserved handle generations without persistent growth";
    } else {
        row.status = CapabilityStatus::Blocked;
        row.detail = "functional, Box3, CRT, or release private-commit lifecycle proof failed";
    }
    return row;
    }();
    const std::int64_t allocator_final = detail::box3d_allocator_byte_count();
    row.values.push_back(
        {"box3d_allocator_baseline_bytes",
         static_cast<double>(allocator_baseline),
         "bytes"});
    row.values.push_back(
        {"box3d_allocator_final_bytes", static_cast<double>(allocator_final), "bytes"});
    if (allocator_baseline != allocator_final) {
        row.status = CapabilityStatus::Blocked;
        row.detail = "lifecycle proof retained Box3 allocator bytes";
    }
    return row;
}

[[nodiscard]] std::optional<std::uint64_t> own_replay_hash(
    std::uint64_t seed, CapabilityRow* metrics = nullptr)
{
    XorShift64 random(seed);
    PhysicsWorld world(WorldConfig{.surface_gravity = 0, .max_bodies = 1});
    BodyDesc body = BodyDesc::dynamic_sphere(
        0.5f, {{random.centered(0.1f), 2, 0}, {}}, 100);
    body.radial_gravity = false;
    body.remove_beyond_six_r = false;
    body.linear_velocity = {1, 0, 0};
    world.create_body(body);
    for (int tick = 0; tick < 60; ++tick) {
        watched_step(world, tick + 1);
        if (metrics) {
            sample_capability_metrics(*metrics, world.metrics());
        }
        if (scan_body_states(world.states(), 10.0)) {
            return std::nullopt;
        }
    }
    return hash_states(world.states());
}

[[nodiscard]] CapabilityRow prove_replay(std::uint64_t seed)
{
    const std::int64_t allocator_baseline = detail::box3d_allocator_byte_count();
    CapabilityRow row{.capability = "upstream_replay"};
    const std::filesystem::path path = std::filesystem::temp_directory_path()
        / ("ninho-box3d-replay-" + std::to_string(seed) + ".b3rec");
    const detail::ReplayConformanceResult proof =
        detail::validate_box3d_replay(path);
    const auto own_first = own_replay_hash(seed, &row);
    const auto own_second = own_replay_hash(seed);
    if (own_first) {
        row.fixture_hashes.push_back(*own_first);
    }
    row.values = {
        {"saved", proof.saved ? 1.0 : 0.0, "bool"},
        {"loaded", proof.loaded ? 1.0 : 0.0, "bool"},
        {"validated", proof.validated ? 1.0 : 0.0, "bool"},
        {"recording_bytes", static_cast<double>(proof.bytes), "bytes"},
        {"temporary_file_removed", proof.temporary_file_removed ? 1.0 : 0.0, "bool"},
    };
    if (!own_first || !own_second) {
        row.status = CapabilityStatus::Blocked;
        row.detail = "public replay fallback fixture produced an invalid body state";
    } else if (proof.saved && proof.loaded && proof.validated && proof.temporary_file_removed) {
        row.status = CapabilityStatus::Pass;
        row.detail = "the pinned official recorder saved, loaded, and validated a minimal replay";
    } else {
        if (proof.temporary_file_removed && own_first == own_second) {
            row.status = CapabilityStatus::Fallback;
            row.fallback = "input_metric_replay";
            row.detail = "official replay failed; deterministic input and metric replay remains available: "
                + proof.error;
        } else {
            row.status = CapabilityStatus::Blocked;
            row.detail = "neither official replay nor the documented diagnostic fallback is reproducible: "
                + proof.error;
        }
    }
    row.functional_status = row.status;
    row.functional_fallback = row.fallback;
    const std::int64_t allocator_final = detail::box3d_allocator_byte_count();
    row.values.push_back(
        {"box3d_allocator_baseline_bytes",
         static_cast<double>(allocator_baseline),
         "bytes"});
    row.values.push_back(
        {"box3d_allocator_final_bytes", static_cast<double>(allocator_final), "bytes"});
    if (allocator_baseline != allocator_final) {
        row.status = CapabilityStatus::Blocked;
        row.detail = "replay proof retained Box3 allocator bytes";
    }
    return row;
}

[[nodiscard]] ScenarioResult run_capability_matrix(
    std::uint64_t seed, int substeps)
{
    ScenarioResult result = make_result(ScenarioKind::CapabilityMatrix, seed, substeps);
    const auto append_functional_row = [&](CapabilityRow row) {
        row.functional_status = row.status;
        row.functional_fallback = row.fallback;
        result.matrix.push_back(std::move(row));
    };
    append_functional_row(prove_ccd_capability());
    append_functional_row(prove_shape_query());
    append_functional_row(prove_contact_events());
    append_functional_row(prove_joint_reaction());
    result.matrix.push_back(prove_hulls_compounds());

    const ScenarioResult sleep =
        run_stability_pile(ScenarioKind::RadialPile, seed, substeps);
    CapabilityRow sleep_row{
        .capability = "radial_sleep",
        .status = sleep.violations.empty() ? CapabilityStatus::Pass
                                           : CapabilityStatus::Blocked,
        .functional_status = sleep.violations.empty() ? CapabilityStatus::Pass
                                                      : CapabilityStatus::Blocked,
        .detail = sleep.violations.empty()
            ? "the public radial-gravity pile satisfies every fixed sleep limit"
            : "the public radial-gravity pile violates at least one fixed sleep limit",
        .peak_body_count = sleep.peak_body_count,
        .peak_shape_count = sleep.peak_shape_count,
        .peak_joint_count = sleep.peak_joint_count,
        .peak_awake_count = sleep.peak_awake_count,
        .peak_contact_count = sleep.peak_contact_count,
        .values = {
            {"sleep_ratio", sleep.sleep_ratio, "ratio"},
            {"p95_linear_speed", sleep.p95_linear_speed, "m/s"},
            {"p95_angular_speed", sleep.p95_angular_speed, "rad/s"},
            {"max_penetration", sleep.max_penetration, "m"},
            {"energy_growth", sleep.max_energy_growth_ratio, "ratio"},
        },
        .fixture_hashes = {sleep.final_hash},
    };
    result.matrix.push_back(std::move(sleep_row));
    result.matrix.push_back(prove_batch_lifecycle());
    result.matrix.push_back(prove_replay(seed));

    result.ticks = sleep.ticks + 20000;
    result.final_hash = hash_capability_rows(result.matrix);
    result.step_min_ms = sleep.step_min_ms;
    result.step_p50_ms = sleep.step_p50_ms;
    result.step_p95_ms = sleep.step_p95_ms;
    result.step_max_ms = sleep.step_max_ms;
    for (const CapabilityRow& row : result.matrix) {
        result.peak_body_count = std::max(result.peak_body_count, row.peak_body_count);
        result.peak_shape_count = std::max(result.peak_shape_count, row.peak_shape_count);
        result.peak_joint_count = std::max(result.peak_joint_count, row.peak_joint_count);
        result.peak_awake_count = std::max(result.peak_awake_count, row.peak_awake_count);
        result.peak_contact_count = std::max(result.peak_contact_count, row.peak_contact_count);
        if (row.status == CapabilityStatus::Blocked) {
            add_violation(
                result,
                "capability_blocked",
                row.capability + ": " + row.detail,
                result.ticks,
                {},
                row.values,
                {{"capability", row.capability},
                 {"status", "blocked"},
                 {"fallback", row.fallback}});
        }
    }
    return result;
}

[[nodiscard]] bool valid_utf8(std::string_view value)
{
    const auto continuation = [](unsigned char byte) {
        return byte >= 0x80 && byte <= 0xbf;
    };
    std::size_t index = 0;
    while (index < value.size()) {
        const unsigned char first = static_cast<unsigned char>(value[index]);
        if (first <= 0x7f) {
            ++index;
            continue;
        }
        if (first >= 0xc2 && first <= 0xdf) {
            if (index + 1 >= value.size()
                || !continuation(static_cast<unsigned char>(value[index + 1]))) {
                return false;
            }
            index += 2;
            continue;
        }
        if (first >= 0xe0 && first <= 0xef) {
            if (index + 2 >= value.size()) {
                return false;
            }
            const unsigned char second = static_cast<unsigned char>(value[index + 1]);
            const unsigned char third = static_cast<unsigned char>(value[index + 2]);
            const bool valid_second = first == 0xe0 ? second >= 0xa0 && second <= 0xbf
                : first == 0xed                 ? second >= 0x80 && second <= 0x9f
                                                : continuation(second);
            if (!valid_second || !continuation(third)) {
                return false;
            }
            index += 3;
            continue;
        }
        if (first >= 0xf0 && first <= 0xf4) {
            if (index + 3 >= value.size()) {
                return false;
            }
            const unsigned char second = static_cast<unsigned char>(value[index + 1]);
            const bool valid_second = first == 0xf0 ? second >= 0x90 && second <= 0xbf
                : first == 0xf4                 ? second >= 0x80 && second <= 0x8f
                                                : continuation(second);
            if (!valid_second
                || !continuation(static_cast<unsigned char>(value[index + 2]))
                || !continuation(static_cast<unsigned char>(value[index + 3]))) {
                return false;
            }
            index += 4;
            continue;
        }
        return false;
    }
    return true;
}

void append_json_string(std::string& output, std::string_view value)
{
    if (!valid_utf8(value)) {
        throw std::invalid_argument("invalid UTF-8 in scenario JSON string");
    }
    constexpr char hex[] = "0123456789abcdef";
    output.push_back('"');
    for (const unsigned char byte : value) {
        switch (byte) {
        case '"':
            output += "\\\"";
            break;
        case '\\':
            output += "\\\\";
            break;
        case '\b':
            output += "\\b";
            break;
        case '\f':
            output += "\\f";
            break;
        case '\n':
            output += "\\n";
            break;
        case '\r':
            output += "\\r";
            break;
        case '\t':
            output += "\\t";
            break;
        default:
            if (byte < 0x20) {
                output += "\\u00";
                output.push_back(hex[byte >> 4]);
                output.push_back(hex[byte & 0x0f]);
            } else {
                output.push_back(static_cast<char>(byte));
            }
            break;
        }
    }
    output.push_back('"');
}

template<class Integer>
void append_json_integer(std::string& output, Integer value)
{
    std::array<char, 32> buffer{};
    const auto converted = std::to_chars(buffer.data(), buffer.data() + buffer.size(), value);
    if (converted.ec != std::errc{}) {
        throw std::invalid_argument("integer cannot be represented in scenario JSON");
    }
    output.append(buffer.data(), converted.ptr);
}

void append_json_number(std::string& output, double value)
{
    if (!std::isfinite(value)) {
        throw std::invalid_argument("non-finite telemetry cannot be represented in JSON");
    }
    std::array<char, 64> buffer{};
    const auto converted = std::to_chars(
        buffer.data(),
        buffer.data() + buffer.size(),
        value,
        std::chars_format::general,
        std::numeric_limits<double>::max_digits10);
    if (converted.ec != std::errc{}) {
        throw std::invalid_argument("floating-point telemetry cannot be represented in JSON");
    }
    output.append(buffer.data(), converted.ptr);
}

void append_json_name(std::string& output, std::string_view name, bool& first)
{
    if (!first) {
        output.push_back(',');
    }
    first = false;
    append_json_string(output, name);
    output.push_back(':');
}

void append_scenario_value(std::string& output, const ScenarioValue& value)
{
    output.push_back('{');
    bool first = true;
    append_json_name(output, "name", first);
    append_json_string(output, value.name);
    append_json_name(output, "value", first);
    append_json_number(output, value.value);
    append_json_name(output, "unit", first);
    append_json_string(output, value.unit);
    output.push_back('}');
}

void append_values(std::string& output, const std::vector<ScenarioValue>& values)
{
    output.push_back('[');
    for (std::size_t index = 0; index < values.size(); ++index) {
        if (index != 0) {
            output.push_back(',');
        }
        append_scenario_value(output, values[index]);
    }
    output.push_back(']');
}

void append_violation(std::string& output, const ScenarioViolation& violation)
{
    output.push_back('{');
    bool first = true;
    append_json_name(output, "scenario", first);
    append_json_string(output, violation.scenario);
    append_json_name(output, "code", first);
    append_json_string(output, violation.code);
    append_json_name(output, "message", first);
    append_json_string(output, violation.message);
    append_json_name(output, "fatal", first);
    output += violation.fatal ? "true" : "false";
    append_json_name(output, "tick", first);
    append_json_integer(output, violation.tick);
    append_json_name(output, "handle", first);
    output.push_back('{');
    bool handle_first = true;
    append_json_name(output, "index", handle_first);
    append_json_integer(output, violation.handle.index);
    append_json_name(output, "generation", handle_first);
    append_json_integer(output, violation.handle.generation);
    output.push_back('}');
    append_json_name(output, "values", first);
    append_values(output, violation.values);
    append_json_name(output, "details", first);
    output.push_back('[');
    for (std::size_t index = 0; index < violation.details.size(); ++index) {
        if (index != 0) {
            output.push_back(',');
        }
        output.push_back('{');
        bool detail_first = true;
        append_json_name(output, "name", detail_first);
        append_json_string(output, violation.details[index].name);
        append_json_name(output, "value", detail_first);
        append_json_string(output, violation.details[index].value);
        output.push_back('}');
    }
    output.push_back(']');
    output.push_back('}');
}

void append_warning(std::string& output, const ScenarioWarning& warning)
{
    output.push_back('{');
    bool first = true;
    append_json_name(output, "code", first);
    append_json_string(output, warning.code);
    append_json_name(output, "message", first);
    append_json_string(output, warning.message);
    append_json_name(output, "details", first);
    output.push_back('[');
    for (std::size_t index = 0; index < warning.details.size(); ++index) {
        if (index != 0) {
            output.push_back(',');
        }
        output.push_back('{');
        bool detail_first = true;
        append_json_name(output, "name", detail_first);
        append_json_string(output, warning.details[index].name);
        append_json_name(output, "value", detail_first);
        append_json_string(output, warning.details[index].value);
        output.push_back('}');
    }
    output.push_back(']');
    output.push_back('}');
}

[[nodiscard]] std::string_view capability_status_name(CapabilityStatus status)
{
    switch (status) {
    case CapabilityStatus::Pass:
        return "pass";
    case CapabilityStatus::Fallback:
        return "fallback";
    case CapabilityStatus::Blocked:
        return "blocked";
    }
    return "blocked";
}

void append_capability_row(std::string& output, const CapabilityRow& row)
{
    output.push_back('{');
    bool first = true;
    append_json_name(output, "capability", first);
    append_json_string(output, row.capability);
    append_json_name(output, "status", first);
    append_json_string(output, capability_status_name(row.status));
    append_json_name(output, "fallback", first);
    if (row.fallback.empty()) {
        output += "null";
    } else {
        append_json_string(output, row.fallback);
    }
    append_json_name(output, "functional_status", first);
    append_json_string(output, capability_status_name(row.functional_status));
    append_json_name(output, "functional_fallback", first);
    if (row.functional_fallback.empty()) {
        output += "null";
    } else {
        append_json_string(output, row.functional_fallback);
    }
    append_json_name(output, "detail", first);
    append_json_string(output, row.detail);
    append_json_name(output, "peak_body_count", first);
    append_json_integer(output, row.peak_body_count);
    append_json_name(output, "peak_shape_count", first);
    append_json_integer(output, row.peak_shape_count);
    append_json_name(output, "peak_joint_count", first);
    append_json_integer(output, row.peak_joint_count);
    append_json_name(output, "peak_awake_count", first);
    append_json_integer(output, row.peak_awake_count);
    append_json_name(output, "peak_contact_count", first);
    append_json_integer(output, row.peak_contact_count);
    append_json_name(output, "values", first);
    append_values(output, row.values);
    append_json_name(output, "fixture_hashes", first);
    output.push_back('[');
    for (std::size_t index = 0; index < row.fixture_hashes.size(); ++index) {
        if (index != 0) {
            output.push_back(',');
        }
        append_json_integer(output, row.fixture_hashes[index]);
    }
    output.push_back(']');
    output.push_back('}');
}

void append_size_array(std::string& output, std::span<const std::size_t> values)
{
    output.push_back('[');
    for (std::size_t index = 0; index < values.size(); ++index) {
        if (index != 0) {
            output.push_back(',');
        }
        append_json_integer(output, values[index]);
    }
    output.push_back(']');
}

void append_int64_array(std::string& output, std::span<const std::int64_t> values)
{
    output.push_back('[');
    for (std::size_t index = 0; index < values.size(); ++index) {
        if (index != 0) {
            output.push_back(',');
        }
        append_json_integer(output, values[index]);
    }
    output.push_back(']');
}

void append_box3d_allocator_observation(
    std::string& output, const Box3dAllocatorObservation& allocator)
{
    output.push_back('{');
    bool first = true;
    append_json_name(output, "baseline_bytes", first);
    append_json_integer(output, allocator.baseline_bytes);
    append_json_name(output, "final_bytes", first);
    append_json_integer(output, allocator.final_bytes);
    append_json_name(output, "max_abs_delta", first);
    append_json_integer(output, allocator.max_abs_delta);
    append_json_name(output, "exact_return", first);
    output += allocator.exact_return ? "true" : "false";
    append_json_name(output, "warmup_post_teardown", first);
    append_int64_array(output, allocator.warmup_post_teardown_bytes);
    append_json_name(output, "measured_post_teardown", first);
    append_int64_array(output, allocator.measured_post_teardown_bytes);
    output.push_back('}');
}

void append_crt_observation(
    std::string& output, const CrtMemoryObservation& crt)
{
    output.push_back('{');
    bool first = true;
    append_json_name(output, "applicable", first);
    output += crt.applicable ? "true" : "false";
    append_json_name(output, "balanced", first);
    output += crt.balanced ? "true" : "false";
    append_json_name(output, "normal_block_count_delta", first);
    append_json_integer(output, crt.normal_block_count_delta);
    append_json_name(output, "normal_block_bytes_delta", first);
    append_json_integer(output, crt.normal_block_bytes_delta);
    append_json_name(output, "client_block_count_delta", first);
    append_json_integer(output, crt.client_block_count_delta);
    append_json_name(output, "client_block_bytes_delta", first);
    append_json_integer(output, crt.client_block_bytes_delta);
    output.push_back('}');
}

void append_memory_observation(
    std::string& output, const MemoryObservation& memory)
{
    output.push_back('{');
    bool first = true;
    append_json_name(output, "gate_scope", first);
    append_json_string(output, memory.gate_scope);
    append_json_name(output, "gate_status", first);
    append_json_string(output, memory.gate_status);
    append_json_name(output, "assessment_status", first);
    append_json_string(output, memory.assessment_status);
    append_json_name(output, "gate_applied", first);
    output += memory.gate_applied ? "true" : "false";
    append_json_name(output, "budget_qualified", first);
    output += memory.budget_qualified ? "true" : "false";
    append_json_name(output, "budget_scope", first);
    append_json_string(output, memory.budget_scope);
    append_json_name(output, "private_commit", first);
    output.push_back('{');
    bool private_first = true;
    const auto private_integer = [&](std::string_view name, std::size_t value) {
        append_json_name(output, name, private_first);
        append_json_integer(output, value);
    };
    const auto private_number = [&](std::string_view name, double value) {
        append_json_name(output, name, private_first);
        append_json_number(output, value);
    };
    append_json_name(output, "available", private_first);
    output += memory.private_commit_available ? "true" : "false";
    append_json_name(output, "stable", private_first);
    output += memory.private_commit_stable ? "true" : "false";
    append_json_name(output, "terminal_growth", private_first);
    output += memory.private_commit_terminal_growth ? "true" : "false";
    private_integer("baseline_last_bytes", memory.private_commit_baseline_bytes);
    private_integer("baseline_full_min_bytes", memory.private_commit_baseline_full_min_bytes);
    private_integer("baseline_central_min_bytes", memory.private_commit_baseline_central_min_bytes);
    private_integer("baseline_median_bytes", memory.private_commit_baseline_median_bytes);
    private_integer("baseline_central_max_bytes", memory.private_commit_baseline_central_max_bytes);
    private_integer("baseline_full_max_bytes", memory.private_commit_baseline_full_max_bytes);
    private_integer("final_last_bytes", memory.private_commit_final_bytes);
    private_integer("final_full_min_bytes", memory.private_commit_final_full_min_bytes);
    private_integer("final_central_min_bytes", memory.private_commit_final_central_min_bytes);
    private_integer("final_median_bytes", memory.private_commit_final_median_bytes);
    private_integer("final_central_max_bytes", memory.private_commit_final_central_max_bytes);
    private_integer("final_full_max_bytes", memory.private_commit_final_full_max_bytes);
    private_integer("peak_bytes", memory.private_commit_peak_bytes);
    private_number("growth_ratio", memory.private_commit_growth_ratio);
    private_number("instant_growth_ratio", memory.private_commit_instant_growth_ratio);
    private_number(
        "warmup_trimmed_span_ratio",
        memory.private_commit_warmup_trimmed_span_ratio);
    private_number("warmup_full_span_ratio", memory.private_commit_warmup_full_span_ratio);
    private_number(
        "measured_trimmed_span_ratio",
        memory.private_commit_measured_trimmed_span_ratio);
    private_number("measured_full_span_ratio", memory.private_commit_measured_full_span_ratio);
    append_json_name(output, "warmup_samples", private_first);
    append_size_array(output, memory.private_commit_warmup_samples);
    append_json_name(output, "measured_samples", private_first);
    append_size_array(output, memory.private_commit_cycle_samples);
    output.push_back('}');

    append_json_name(output, "working_set", first);
    output.push_back('{');
    bool working_first = true;
    const auto working_integer = [&](std::string_view name, std::size_t value) {
        append_json_name(output, name, working_first);
        append_json_integer(output, value);
    };
    const auto working_number = [&](std::string_view name, double value) {
        append_json_name(output, name, working_first);
        append_json_number(output, value);
    };
    append_json_name(output, "available", working_first);
    output += memory.working_set_available ? "true" : "false";
    append_json_name(output, "stable", working_first);
    output += memory.working_set_stable ? "true" : "false";
    append_json_name(output, "terminal_growth", working_first);
    output += memory.working_set_terminal_growth ? "true" : "false";
    append_json_name(output, "assessment_status", working_first);
    append_json_string(output, memory.working_set_assessment_status);
    append_json_name(output, "gate_status", working_first);
    append_json_string(output, memory.working_set_gate_status);
    append_json_name(output, "gate_applied", working_first);
    output += memory.working_set_gate_applied ? "true" : "false";
    append_json_name(output, "budget_qualified", working_first);
    output += memory.working_set_budget_qualified ? "true" : "false";
    append_json_name(output, "budget_scope", working_first);
    append_json_string(output, memory.working_set_budget_scope);
    working_integer("baseline_last_bytes", memory.working_set_baseline_bytes);
    working_integer("baseline_full_min_bytes", memory.working_set_baseline_full_min_bytes);
    working_integer(
        "baseline_central_min_bytes",
        memory.working_set_baseline_central_min_bytes);
    working_integer("baseline_median_bytes", memory.working_set_baseline_median_bytes);
    working_integer(
        "baseline_central_max_bytes",
        memory.working_set_baseline_central_max_bytes);
    working_integer("baseline_full_max_bytes", memory.working_set_baseline_full_max_bytes);
    working_integer("final_last_bytes", memory.working_set_final_bytes);
    working_integer("final_full_min_bytes", memory.working_set_final_full_min_bytes);
    working_integer(
        "final_central_min_bytes",
        memory.working_set_final_central_min_bytes);
    working_integer("final_median_bytes", memory.working_set_final_median_bytes);
    working_integer(
        "final_central_max_bytes",
        memory.working_set_final_central_max_bytes);
    working_integer("final_full_max_bytes", memory.working_set_final_full_max_bytes);
    working_integer("peak_bytes", memory.working_set_peak_bytes);
    working_number("growth_ratio", memory.working_set_growth_ratio);
    working_number("instant_growth_ratio", memory.working_set_instant_growth_ratio);
    working_number(
        "warmup_trimmed_span_ratio",
        memory.working_set_warmup_trimmed_span_ratio);
    working_number(
        "warmup_full_span_ratio",
        memory.working_set_warmup_full_span_ratio);
    working_number(
        "measured_trimmed_span_ratio",
        memory.working_set_measured_trimmed_span_ratio);
    working_number(
        "measured_full_span_ratio",
        memory.working_set_measured_full_span_ratio);
    append_json_name(output, "warmup_samples", working_first);
    append_size_array(output, memory.working_set_warmup_samples);
    append_json_name(output, "measured_samples", working_first);
    append_size_array(output, memory.working_set_cycle_samples);
    output.push_back('}');
    output.push_back('}');
}

void append_scenario_result(std::string& output, const ScenarioResult& result)
{
    output.push_back('{');
    bool first = true;
    const auto string_field = [&](std::string_view name, std::string_view value) {
        append_json_name(output, name, first);
        append_json_string(output, value);
    };
    const auto number_field = [&](std::string_view name, double value) {
        append_json_name(output, name, first);
        append_json_number(output, value);
    };
    const auto integer_field = [&](std::string_view name, auto value) {
        append_json_name(output, name, first);
        append_json_integer(output, value);
    };
    const auto boolean_field = [&](std::string_view name, bool value) {
        append_json_name(output, name, first);
        output += value ? "true" : "false";
    };

    string_field("name", result.name);
    integer_field("seed", result.seed);
    integer_field("substeps", result.substeps);
    integer_field("ticks", result.ticks);
    integer_field("final_hash", result.final_hash);
    append_json_name(output, "hashes", first);
    output.push_back('[');
    if (result.repeat_hashes.empty()) {
        append_json_integer(output, result.final_hash);
    } else {
        for (std::size_t index = 0; index < result.repeat_hashes.size(); ++index) {
            if (index != 0) {
                output.push_back(',');
            }
            append_json_integer(output, result.repeat_hashes[index]);
        }
    }
    output.push_back(']');
    append_json_name(output, "repeat_observations", first);
    output.push_back('[');
    for (std::size_t index = 0; index < result.repeat_observations.size(); ++index) {
        if (index != 0) {
            output.push_back(',');
        }
        const RepeatObservation& observation = result.repeat_observations[index];
        output.push_back('{');
        bool observation_first = true;
        append_json_name(output, "repeat", observation_first);
        append_json_integer(output, observation.repeat_index);
        append_json_name(output, "hash", observation_first);
        append_json_integer(output, observation.hash);
        append_json_name(output, "peak_body_count", observation_first);
        append_json_integer(output, observation.peak_body_count);
        append_json_name(output, "peak_shape_count", observation_first);
        append_json_integer(output, observation.peak_shape_count);
        append_json_name(output, "peak_joint_count", observation_first);
        append_json_integer(output, observation.peak_joint_count);
        append_json_name(output, "peak_awake_count", observation_first);
        append_json_integer(output, observation.peak_awake_count);
        append_json_name(output, "peak_contact_count", observation_first);
        append_json_integer(output, observation.peak_contact_count);
        append_json_name(output, "memory", observation_first);
        append_memory_observation(output, observation.memory);
        append_json_name(output, "box3d_allocator", observation_first);
        append_box3d_allocator_observation(output, observation.box3d_allocator);
        append_json_name(output, "crt", observation_first);
        append_crt_observation(output, observation.crt);
        output.push_back('}');
    }
    output.push_back(']');

    boolean_field("contact_before_pile_exit", result.contact_before_pile_exit);
    integer_field("ccd_primary_pass_count", result.ccd_primary_pass_count);
    integer_field("ccd_fallback_pass_count", result.ccd_fallback_pass_count);
    number_field("projectile_speed", result.projectile_speed);
    number_field("surface_separation", result.surface_separation);
    number_field("final_linear_speed", result.final_linear_speed);
    number_field("final_angular_speed", result.final_angular_speed);
    number_field("p95_linear_speed", result.p95_linear_speed);
    number_field("p95_angular_speed", result.p95_angular_speed);
    number_field("sleep_ratio", result.sleep_ratio);
    number_field("max_penetration", result.max_penetration);
    number_field("minimum_density", result.minimum_density);
    number_field("maximum_density", result.maximum_density);
    number_field("energy_at_tick_600", result.energy_at_tick_600);
    number_field("max_energy_growth_ratio", result.max_energy_growth_ratio);

    integer_field("dynamic_body_count", result.dynamic_body_count);
    integer_field("shape_count", result.shape_count);
    integer_field("joint_count", result.joint_count);
    integer_field("peak_body_count", result.peak_body_count);
    integer_field("peak_shape_count", result.peak_shape_count);
    integer_field("peak_joint_count", result.peak_joint_count);
    integer_field("peak_awake_count", result.peak_awake_count);
    integer_field("peak_contact_count", result.peak_contact_count);
    append_json_name(output, "step_ms", first);
    output.push_back('{');
    bool timing_first = true;
    append_json_name(output, "min", timing_first);
    append_json_number(output, result.step_min_ms);
    append_json_name(output, "p50", timing_first);
    append_json_number(output, result.step_p50_ms);
    append_json_name(output, "p95", timing_first);
    append_json_number(output, result.step_p95_ms);
    append_json_name(output, "max", timing_first);
    append_json_number(output, result.step_max_ms);
    output.push_back('}');

    append_json_name(output, "memory", first);
    append_memory_observation(output, make_repeat_observation(0, result).memory);
    append_json_name(output, "box3d_allocator", first);
    append_box3d_allocator_observation(output, result.box3d_allocator);
    append_json_name(output, "crt", first);
    append_crt_observation(output, result.crt);
    integer_field("warmup_ticks", result.warmup_ticks);
    integer_field("measurement_ticks", result.measurement_ticks);
    integer_field("allocator_warmup_cycles", result.allocator_warmup_cycles);
    integer_field("stress_cycles", result.stress_cycles);
    string_field("fallback", result.fallback);

    append_json_name(output, "limits", first);
    output.push_back('[');
    for (std::size_t index = 0; index < result.limits.size(); ++index) {
        if (index != 0) {
            output.push_back(',');
        }
        const ScenarioLimit& limit = result.limits[index];
        output.push_back('{');
        bool limit_first = true;
        append_json_name(output, "name", limit_first);
        append_json_string(output, limit.name);
        append_json_name(output, "comparison", limit_first);
        append_json_string(output, limit.comparison);
        append_json_name(output, "value", limit_first);
        append_json_number(output, limit.value);
        append_json_name(output, "unit", limit_first);
        append_json_string(output, limit.unit);
        output.push_back('}');
    }
    output.push_back(']');
    append_json_name(output, "metrics", first);
    append_values(output, result.metrics);
    append_json_name(output, "matrix", first);
    output.push_back('[');
    for (std::size_t index = 0; index < result.matrix.size(); ++index) {
        if (index != 0) {
            output.push_back(',');
        }
        append_capability_row(output, result.matrix[index]);
    }
    output.push_back(']');
    append_json_name(output, "warnings", first);
    output.push_back('[');
    for (std::size_t index = 0; index < result.warnings.size(); ++index) {
        if (index != 0) {
            output.push_back(',');
        }
        append_warning(output, result.warnings[index]);
    }
    output.push_back(']');
    append_json_name(output, "violations", first);
    output.push_back('[');
    for (std::size_t index = 0; index < result.violations.size(); ++index) {
        if (index != 0) {
            output.push_back(',');
        }
        append_violation(output, result.violations[index]);
    }
    output.push_back(']');
    output.push_back('}');
}

}

#if defined(NINHO_ENABLE_TEST_FACADES)
void detail::ScenarioTestFacade::reset_projectile_simulation_count() noexcept
{
    projectile_simulation_count_value = 0;
}

std::size_t detail::ScenarioTestFacade::projectile_simulation_count() noexcept
{
    return projectile_simulation_count_value;
}
#endif

void ScenarioRunner::set_emergency_json_path(std::optional<std::string> path)
{
    const std::scoped_lock lock(emergency_json_mutex);
    emergency_json_path = std::move(path);
}

std::string_view scenario_name(ScenarioKind kind)
{
    return name_of(kind);
}

std::optional<ScenarioKind> parse_scenario_kind(std::string_view name)
{
    constexpr std::array kinds{
        ScenarioKind::RadialFall,
        ScenarioKind::ProjectilePile,
        ScenarioKind::RadialPile,
        ScenarioKind::MassRatio,
        ScenarioKind::Stress,
        ScenarioKind::CapabilityMatrix,
    };
    const auto found = std::ranges::find_if(
        kinds, [&](ScenarioKind kind) { return name_of(kind) == name; });
    return found == kinds.end() ? std::nullopt : std::optional<ScenarioKind>{*found};
}

std::optional<ScenarioResult> detail::configuration_mismatch_result(
    ScenarioKind kind,
    std::uint64_t seed,
    int substeps,
    RuntimeConfiguration configuration)
{
    if (evaluate_runtime_configuration(configuration)
        != RuntimeConfigurationStatus::ConfigurationMismatch) {
        return std::nullopt;
    }
    ScenarioResult mismatch{
        .kind = kind,
        .name = std::string{scenario_name(kind)},
        .seed = seed,
        .substeps = substeps,
    };
    mismatch.violations.push_back({
        .scenario = mismatch.name,
        .code = "configuration_mismatch",
        .message = "Release scenarios require the static /MT CRT",
        .tick = 0,
        .details = {
            {"required_crt", "/MT"},
            {"detected_crt", "/MD or non-/MT"},
        },
    });
    return mismatch;
}

std::string ScenarioResult::to_json() const
{
    std::string output;
    output.reserve(4096);
    append_scenario_result(output, *this);
    return output;
}

std::string ScenarioReport::to_json() const
{
    std::string output;
    output.reserve(32768);
    output.push_back('{');
    bool first = true;
    append_json_name(output, "schema", first);
    append_json_string(output, "ninho.physics.scenario.v1");
    append_json_name(output, "tool", first);
    output += "{\"name\":\"ninho_physics_spike\",\"version\":\"0.1.0\"}";
    append_json_name(output, "dependencies", first);
    output += "{\"box3d\":{\"version\":\"0.1.0\",\"commit\":\"8441b4a06d6d09dcfb0b0f704df4d847d1437b92\"},"
              "\"godot\":{\"version\":\"4.5.1-stable\",\"commit\":\"f62fdbde15035c5576dad93e586201f4d41ef0cb\"},"
              "\"godot_cpp\":{\"version\":\"godot-4.5-stable\",\"commit\":\"e83fd0904c13356ed1d4c3d09f8bb9132bdc6b77\"}}";
    append_json_name(output, "build_type", first);
    append_json_string(output, build_type);
    append_json_name(output, "cpu", first);
    append_json_string(output, cpu);
    append_json_name(output, "configuration", first);
    output.push_back('{');
    bool configuration_first = true;
    append_json_name(output, "seed", configuration_first);
    append_json_integer(output, seed);
    append_json_name(output, "substeps", configuration_first);
    append_json_integer(output, substeps);
    append_json_name(output, "repeat", configuration_first);
    append_json_integer(output, repeat);
    append_json_name(output, "time_step", configuration_first);
    append_json_number(output, 1.0 / 60.0);
    output.push_back('}');

    append_json_name(output, "process_box3d_allocator", first);
    append_box3d_allocator_observation(output, process_box3d_allocator);

    append_json_name(output, "budget_qualification", first);
    output += "{\"status\":\"deferred\",\"target_growth_ratio\":0.05,"
              "\"warning\":\"private_commit_budget_unqualified\"}";

    append_json_name(output, "warnings", first);
    output += "[{\"code\":\"private_commit_budget_unqualified\","
              "\"message\":\"PrivateUsage budget is deferred to a packaged Release build on reference hardware\","
              "\"details\":[{\"name\":\"budget_scope\","
              "\"value\":\"future_packaged_reference_hardware\"}]}]";

    append_json_name(output, "scenarios", first);
    output.push_back('[');
    for (std::size_t index = 0; index < scenarios.size(); ++index) {
        if (index != 0) {
            output.push_back(',');
        }
        append_scenario_result(output, scenarios[index]);
    }
    output.push_back(']');

    append_json_name(output, "matrix", first);
    output.push_back('[');
    bool first_matrix_row = true;
    for (const ScenarioResult& scenario : scenarios) {
        for (const CapabilityRow& row : scenario.matrix) {
            if (!first_matrix_row) {
                output.push_back(',');
            }
            first_matrix_row = false;
            append_capability_row(output, row);
        }
    }
    output.push_back(']');

    append_json_name(output, "violations", first);
    output.push_back('[');
    bool first_violation = true;
    for (const ScenarioResult& scenario : scenarios) {
        for (const ScenarioViolation& violation : scenario.violations) {
            if (!first_violation) {
                output.push_back(',');
            }
            first_violation = false;
            append_violation(output, violation);
        }
    }
    output.push_back(']');
    append_json_name(output, "recommendation", first);
    const bool has_violations = std::ranges::any_of(
        scenarios, [](const ScenarioResult& scenario) { return !scenario.violations.empty(); });
    append_json_string(
        output,
        has_violations ? "bloquear" : "prosseguir_com_limites");
    output.push_back('}');
    return output;
}

std::optional<StateValidationFailure> validate_body_state(
    const BodyState& state, double planet_radius)
{
    if (!std::isfinite(planet_radius) || planet_radius <= 0.0) {
        throw std::invalid_argument("planet radius must be finite and positive");
    }
    if (!state.handle.valid()) {
        return StateValidationFailure{"handle", static_cast<double>(state.handle.index)};
    }
    const auto finite_component = [](float value, const char* field)
        -> std::optional<StateValidationFailure> {
        return std::isfinite(value)
            ? std::nullopt
            : std::optional<StateValidationFailure>{{field, value}};
    };
    const std::array components{
        std::pair{state.transform.position.x, "position.x"},
        std::pair{state.transform.position.y, "position.y"},
        std::pair{state.transform.position.z, "position.z"},
        std::pair{state.transform.rotation.x, "rotation.x"},
        std::pair{state.transform.rotation.y, "rotation.y"},
        std::pair{state.transform.rotation.z, "rotation.z"},
        std::pair{state.transform.rotation.w, "rotation.w"},
        std::pair{state.linear_velocity.x, "linear_velocity.x"},
        std::pair{state.linear_velocity.y, "linear_velocity.y"},
        std::pair{state.linear_velocity.z, "linear_velocity.z"},
        std::pair{state.angular_velocity.x, "angular_velocity.x"},
        std::pair{state.angular_velocity.y, "angular_velocity.y"},
        std::pair{state.angular_velocity.z, "angular_velocity.z"},
    };
    for (const auto& [value, field] : components) {
        if (const auto failure = finite_component(value, field)) {
            return failure;
        }
    }
    constexpr double quaternion_norm_tolerance = 1.0e-3;
    const Quat rotation = state.transform.rotation;
    const double rotation_norm_squared = static_cast<double>(rotation.x) * rotation.x
        + static_cast<double>(rotation.y) * rotation.y
        + static_cast<double>(rotation.z) * rotation.z
        + static_cast<double>(rotation.w) * rotation.w;
    if (std::abs(rotation_norm_squared - 1.0) > quaternion_norm_tolerance) {
        return StateValidationFailure{"rotation_norm", rotation_norm_squared};
    }
    if (!std::isfinite(state.mass) || state.mass < 0.0f) {
        return StateValidationFailure{"mass", state.mass};
    }
    const Vec3 linear = state.linear_velocity;
    const double linear_speed = std::sqrt(
        static_cast<double>(linear.x) * linear.x
        + static_cast<double>(linear.y) * linear.y
        + static_cast<double>(linear.z) * linear.z);
    if (!std::isfinite(linear_speed) || linear_speed > 100.0) {
        return StateValidationFailure{"linear_speed", linear_speed};
    }
    const Vec3 position = state.transform.position;
    const double radius = std::sqrt(
        static_cast<double>(position.x) * position.x
        + static_cast<double>(position.y) * position.y
        + static_cast<double>(position.z) * position.z);
    if (!std::isfinite(radius)) {
        return StateValidationFailure{"radius", radius};
    }
    if (radius >= 6.0 * planet_radius && !state.ejected) {
        return StateValidationFailure{"radius_without_ejection", radius};
    }
    return std::nullopt;
}

double max_rolling_energy_growth(
    std::span<const double> energies, std::size_t window, double epsilon)
{
    if (window < 2 || energies.size() < window) {
        throw std::invalid_argument("energy window must contain at least two complete samples");
    }
    if (!std::isfinite(epsilon) || epsilon <= 0.0) {
        throw std::invalid_argument("energy epsilon must be finite and positive");
    }
    for (const double energy : energies) {
        if (!std::isfinite(energy) || energy < 0.0) {
            throw std::invalid_argument("energy samples must be finite and non-negative");
        }
    }
    double maximum_growth = 0.0;
    for (std::size_t start = 0; start + window <= energies.size(); ++start) {
        const double initial = energies[start];
        const double final = energies[start + window - 1];
        const double growth = (final - initial) / std::max(initial, epsilon);
        maximum_growth = std::max(maximum_growth, growth);
    }
    return maximum_growth;
}

PrivateCommitAssessment assess_private_commit(
    std::span<const std::size_t> warmup_samples,
    std::span<const std::size_t> measured_samples)
{
    constexpr std::size_t protocol_cycles = 10;
    constexpr std::size_t plateau_start = 5;
    PrivateCommitAssessment result;
    if (warmup_samples.size() != protocol_cycles
        || measured_samples.size() != protocol_cycles
        || std::ranges::any_of(warmup_samples, [](std::size_t value) { return value == 0; })
        || std::ranges::any_of(measured_samples, [](std::size_t value) { return value == 0; })) {
        return result;
    }

    const auto summarize = [](std::span<const std::size_t> samples) {
        std::array<std::size_t, 5> tail{};
        std::ranges::copy(samples.last(5), tail.begin());
        std::ranges::sort(tail);
        return std::tuple{tail[0], tail[1], tail[2], tail[3], tail[4]};
    };
    std::tie(
        result.baseline_full_min_bytes,
        result.baseline_central_min_bytes,
        result.baseline_median_bytes,
        result.baseline_central_max_bytes,
        result.baseline_full_max_bytes) = summarize(warmup_samples.subspan(plateau_start));
    std::tie(
        result.final_full_min_bytes,
        result.final_central_min_bytes,
        result.final_median_bytes,
        result.final_central_max_bytes,
        result.final_full_max_bytes) = summarize(measured_samples.subspan(plateau_start));
    result.available = true;
    result.growth_ratio = std::max(
        0.0,
        (static_cast<double>(result.final_median_bytes)
         - static_cast<double>(result.baseline_median_bytes))
            / static_cast<double>(result.baseline_median_bytes));
    result.instant_growth_ratio = std::max(
        0.0,
        (static_cast<double>(measured_samples.back())
         - static_cast<double>(warmup_samples.back()))
            / static_cast<double>(warmup_samples.back()));
    result.warmup_trimmed_span_ratio = static_cast<double>(
        result.baseline_central_max_bytes - result.baseline_central_min_bytes)
        / static_cast<double>(result.baseline_median_bytes);
    result.warmup_full_span_ratio = static_cast<double>(
        result.baseline_full_max_bytes - result.baseline_full_min_bytes)
        / static_cast<double>(result.baseline_median_bytes);
    result.measured_trimmed_span_ratio = static_cast<double>(
        result.final_central_max_bytes - result.final_central_min_bytes)
        / static_cast<double>(result.final_median_bytes);
    result.measured_full_span_ratio = static_cast<double>(
        result.final_full_max_bytes - result.final_full_min_bytes)
        / static_cast<double>(result.final_median_bytes);
    const double terminal_threshold = 1.05 * result.baseline_median_bytes;
    result.terminal_growth = static_cast<double>(measured_samples[8]) > terminal_threshold
        && static_cast<double>(measured_samples[9]) > terminal_threshold;
    result.stable = result.warmup_trimmed_span_ratio <= 0.05
        && result.measured_trimmed_span_ratio <= 0.05;
    if (result.warmup_trimmed_span_ratio > 0.05) {
        result.status = PrivateCommitStatus::Unstable;
    } else if (result.growth_ratio > 0.05 || result.terminal_growth) {
        result.status = PrivateCommitStatus::Growth;
    } else if (result.measured_trimmed_span_ratio > 0.05) {
        result.status = PrivateCommitStatus::Unstable;
    } else {
        result.status = PrivateCommitStatus::Pass;
    }
    return result;
}

CrtMemoryObservation debug_crt_allocation_probe(bool intentional_allocation)
{
    CrtMemoryObservation result;
#if defined(_MSC_VER) && defined(_DEBUG)
    result.applicable = true;
    _CrtMemState before{};
    _CrtMemState after{};
    _CrtMemState difference{};
    _CrtMemCheckpoint(&before);
    char* allocation = intentional_allocation ? new char[64] : nullptr;
    _CrtMemCheckpoint(&after);
    _CrtMemDifference(&difference, &before, &after);
    result.normal_block_count_delta =
        static_cast<std::int64_t>(difference.lCounts[_NORMAL_BLOCK]);
    result.normal_block_bytes_delta =
        static_cast<std::int64_t>(difference.lSizes[_NORMAL_BLOCK]);
    result.client_block_count_delta =
        static_cast<std::int64_t>(difference.lCounts[_CLIENT_BLOCK]);
    result.client_block_bytes_delta =
        static_cast<std::int64_t>(difference.lSizes[_CLIENT_BLOCK]);
    result.balanced = result.normal_block_count_delta == 0
        && result.normal_block_bytes_delta == 0
        && result.client_block_count_delta == 0
        && result.client_block_bytes_delta == 0;
    delete[] allocation;
#else
    static_cast<void>(intentional_allocation);
#endif
    return result;
}

RepeatObservation make_repeat_observation(
    int repeat_index, const ScenarioResult& result)
{
    return {
        .repeat_index = repeat_index,
        .hash = result.final_hash,
        .peak_body_count = result.peak_body_count,
        .peak_shape_count = result.peak_shape_count,
        .peak_joint_count = result.peak_joint_count,
        .peak_awake_count = result.peak_awake_count,
        .peak_contact_count = result.peak_contact_count,
        .memory = {
            .gate_scope = result.private_commit_gate_scope,
            .gate_status = result.private_commit_gate_status,
            .assessment_status = result.private_commit_assessment_status,
            .gate_applied = result.private_commit_gate_applied,
            .budget_qualified = result.private_commit_budget_qualified,
            .budget_scope = result.private_commit_budget_scope,
            .private_commit_available = result.private_commit_available,
            .private_commit_stable = result.private_commit_stable,
            .private_commit_terminal_growth = result.private_commit_terminal_growth,
            .private_commit_baseline_bytes = result.private_commit_baseline_bytes,
            .private_commit_baseline_full_min_bytes = result.private_commit_baseline_full_min_bytes,
            .private_commit_baseline_central_min_bytes = result.private_commit_baseline_central_min_bytes,
            .private_commit_baseline_median_bytes = result.private_commit_baseline_median_bytes,
            .private_commit_baseline_central_max_bytes = result.private_commit_baseline_central_max_bytes,
            .private_commit_baseline_full_max_bytes = result.private_commit_baseline_full_max_bytes,
            .private_commit_final_bytes = result.private_commit_final_bytes,
            .private_commit_final_full_min_bytes = result.private_commit_final_full_min_bytes,
            .private_commit_final_central_min_bytes = result.private_commit_final_central_min_bytes,
            .private_commit_final_median_bytes = result.private_commit_final_median_bytes,
            .private_commit_final_central_max_bytes = result.private_commit_final_central_max_bytes,
            .private_commit_final_full_max_bytes = result.private_commit_final_full_max_bytes,
            .private_commit_peak_bytes = result.private_commit_peak_bytes,
            .private_commit_growth_ratio = result.private_commit_growth_ratio,
            .private_commit_instant_growth_ratio =
                result.private_commit_instant_growth_ratio,
            .private_commit_warmup_trimmed_span_ratio = result.private_commit_warmup_trimmed_span_ratio,
            .private_commit_warmup_full_span_ratio = result.private_commit_warmup_full_span_ratio,
            .private_commit_measured_trimmed_span_ratio = result.private_commit_measured_trimmed_span_ratio,
            .private_commit_measured_full_span_ratio = result.private_commit_measured_full_span_ratio,
            .private_commit_warmup_samples = result.private_commit_warmup_samples,
            .private_commit_cycle_samples = result.private_commit_cycle_samples,
            .working_set_gate_status = result.working_set_gate_status,
            .working_set_assessment_status = result.working_set_assessment_status,
            .working_set_gate_applied = result.working_set_gate_applied,
            .working_set_budget_qualified = result.working_set_budget_qualified,
            .working_set_budget_scope = result.working_set_budget_scope,
            .working_set_available = result.working_set_available,
            .working_set_stable = result.working_set_stable,
            .working_set_terminal_growth = result.working_set_terminal_growth,
            .working_set_baseline_bytes = result.working_set_baseline_bytes,
            .working_set_baseline_full_min_bytes = result.working_set_baseline_low_bytes,
            .working_set_baseline_central_min_bytes =
                result.working_set_baseline_central_low_bytes,
            .working_set_baseline_median_bytes = result.working_set_baseline_median_bytes,
            .working_set_baseline_central_max_bytes =
                result.working_set_baseline_central_high_bytes,
            .working_set_baseline_full_max_bytes = result.working_set_baseline_max_bytes,
            .working_set_final_bytes = result.working_set_final_bytes,
            .working_set_final_full_min_bytes = result.working_set_final_low_bytes,
            .working_set_final_central_min_bytes =
                result.working_set_final_central_low_bytes,
            .working_set_final_median_bytes = result.working_set_final_median_bytes,
            .working_set_final_central_max_bytes =
                result.working_set_final_central_high_bytes,
            .working_set_final_full_max_bytes = result.working_set_final_max_bytes,
            .working_set_peak_bytes = result.working_set_peak_bytes,
            .working_set_growth_ratio = result.working_set_growth_ratio,
            .working_set_instant_growth_ratio = result.working_set_instant_growth_ratio,
            .working_set_warmup_trimmed_span_ratio =
                result.working_set_warmup_trimmed_span_ratio,
            .working_set_warmup_full_span_ratio =
                result.working_set_warmup_full_span_ratio,
            .working_set_measured_trimmed_span_ratio =
                result.working_set_measured_trimmed_span_ratio,
            .working_set_measured_full_span_ratio =
                result.working_set_measured_full_span_ratio,
            .working_set_warmup_samples = result.working_set_warmup_samples,
            .working_set_cycle_samples = result.working_set_cycle_samples,
        },
        .box3d_allocator = result.box3d_allocator,
        .crt = result.crt,
    };
}

bool repeat_topology_matches(
    const RepeatObservation& expected, const RepeatObservation& actual) noexcept
{
    return expected.peak_body_count == actual.peak_body_count
        && expected.peak_shape_count == actual.peak_shape_count
        && expected.peak_joint_count == actual.peak_joint_count
        && expected.peak_awake_count == actual.peak_awake_count
        && expected.peak_contact_count == actual.peak_contact_count;
}

bool record_repeat_topology_mismatch(
    ScenarioResult& result,
    const RepeatObservation& expected,
    const RepeatObservation& actual)
{
    if (repeat_topology_matches(expected, actual)) {
        return false;
    }
    result.violations.push_back({
        .scenario = result.name,
        .code = "determinism_topology_mismatch",
        .message = "topology peaks differ inside the same executable and configuration",
        .tick = result.ticks,
        .values = {{"repeat", static_cast<double>(actual.repeat_index), "count"}},
        .details = {
            {"expected_body_shape_joint_awake_contact",
             std::to_string(expected.peak_body_count) + '/'
                 + std::to_string(expected.peak_shape_count) + '/'
                 + std::to_string(expected.peak_joint_count) + '/'
                 + std::to_string(expected.peak_awake_count) + '/'
                 + std::to_string(expected.peak_contact_count)},
            {"actual_body_shape_joint_awake_contact",
             std::to_string(actual.peak_body_count) + '/'
                 + std::to_string(actual.peak_shape_count) + '/'
                 + std::to_string(actual.peak_joint_count) + '/'
                 + std::to_string(actual.peak_awake_count) + '/'
                 + std::to_string(actual.peak_contact_count)},
        },
    });
    return true;
}

bool has_two_consecutive_samples(
    std::span<const double> samples, double threshold)
{
    if (!std::isfinite(threshold) || threshold < 0.0) {
        throw std::invalid_argument("consecutive sample threshold must be finite and non-negative");
    }
    for (std::size_t index = 1; index < samples.size(); ++index) {
        if (!std::isfinite(samples[index - 1]) || !std::isfinite(samples[index])) {
            throw std::invalid_argument("consecutive samples must be finite");
        }
        if (samples[index - 1] >= threshold && samples[index] >= threshold) {
            return true;
        }
    }
    return false;
}

CapabilityStatus classify_lifecycle_status(
    int completed_cycles,
    int invalid_handles,
    std::optional<double> private_commit_growth,
    std::optional<PrivateCommitStatus> working_set_status)
{
    static_cast<void>(private_commit_growth);
    static_cast<void>(working_set_status);
    if (completed_cycles != 10000 || invalid_handles != 0) {
        return CapabilityStatus::Blocked;
    }
    return CapabilityStatus::Pass;
}

int scenario_exit_code(
    std::span<const ScenarioResult> scenarios, bool hash_mismatch)
{
    return hash_mismatch
            || std::ranges::any_of(scenarios, [](const ScenarioResult& result) {
                   return !result.violations.empty();
               })
        ? 1
        : 0;
}

std::uint64_t hash_capability_rows(std::span<const CapabilityRow> rows)
{
    std::uint64_t hash = 14695981039346656037ull;
    const auto mix_byte = [&hash](std::uint8_t byte) {
        hash ^= byte;
        hash *= 1099511628211ull;
    };
    const auto mix_u64 = [&](std::uint64_t value) {
        for (int index = 0; index < 8; ++index) {
            mix_byte(static_cast<std::uint8_t>(value >> (index * 8)));
        }
    };
    const auto mix_string = [&](std::string_view value) {
        if (!valid_utf8(value)) {
            throw std::invalid_argument("invalid UTF-8 in capability hash field");
        }
        mix_u64(value.size());
        for (const unsigned char byte : value) {
            mix_byte(byte);
        }
    };
    std::vector<const CapabilityRow*> ordered_rows;
    ordered_rows.reserve(rows.size());
    for (const CapabilityRow& row : rows) {
        ordered_rows.push_back(&row);
    }
    std::ranges::sort(
        ordered_rows, {}, [](const CapabilityRow* row) { return row->capability; });
    for (const CapabilityRow* row : ordered_rows) {
        mix_string(row->capability);
        mix_u64(static_cast<std::uint64_t>(row->functional_status));
        mix_string(row->functional_fallback);
        mix_u64(static_cast<std::uint64_t>(row->peak_body_count));
        mix_u64(static_cast<std::uint64_t>(row->peak_shape_count));
        mix_u64(static_cast<std::uint64_t>(row->peak_joint_count));
        mix_u64(static_cast<std::uint64_t>(row->peak_awake_count));
        mix_u64(static_cast<std::uint64_t>(row->peak_contact_count));
        std::vector<const ScenarioValue*> ordered_values;
        for (const ScenarioValue& value : row->values) {
            const bool nonfunctional_value =
                value.name.find("working_set") != std::string::npos
                || value.name.find("private_commit") != std::string::npos
                || value.name.find("box3d_allocator") != std::string::npos
                || value.name.find("crt_") != std::string::npos
                || value.name.find("warning") != std::string::npos
                || value.name.find("timing") != std::string::npos
                || value.name.starts_with("step_") || value.name.ends_with("_ms");
            if (!nonfunctional_value) {
                ordered_values.push_back(&value);
            }
        }
        std::ranges::sort(ordered_values, [](const ScenarioValue* lhs, const ScenarioValue* rhs) {
            return std::tie(lhs->name, lhs->unit) < std::tie(rhs->name, rhs->unit);
        });
        for (const ScenarioValue* value : ordered_values) {
            if (!std::isfinite(value->value)) {
                throw std::invalid_argument("non-finite capability hash value");
            }
            mix_string(value->name);
            mix_string(value->unit);
            mix_u64(std::bit_cast<std::uint64_t>(value->value));
        }
        std::vector<std::uint64_t> fixture_hashes = row->fixture_hashes;
        std::ranges::sort(fixture_hashes);
        mix_u64(fixture_hashes.size());
        for (const std::uint64_t fixture_hash : fixture_hashes) {
            mix_u64(fixture_hash);
        }
    }
    return hash;
}

std::uint64_t hash_states(std::span<const BodyState> states)
{
    std::vector<BodyState> ordered(states.begin(), states.end());
    std::ranges::sort(ordered, {}, &BodyState::handle);
    std::uint64_t hash = 14695981039346656037ull;
    const auto mix = [&hash](std::int64_t value) {
        for (int index = 0; index < 8; ++index) {
            hash ^= std::uint8_t(std::uint64_t(value) >> (index * 8));
            hash *= 1099511628211ull;
        }
    };
    const auto quantize = [](float value, double scale, const char* field) {
        if (!std::isfinite(value)) {
            throw std::invalid_argument(std::string{"non-finite hash field: "} + field);
        }
        const double scaled = static_cast<double>(value) * scale;
        constexpr double minimum = -9223372036854775808.0;
        constexpr double exclusive_maximum = 9223372036854775808.0;
        if (!std::isfinite(scaled) || scaled < minimum || scaled >= exclusive_maximum) {
            throw std::invalid_argument(std::string{"out-of-range hash field: "} + field);
        }
        return static_cast<std::int64_t>(std::llround(scaled));
    };
    for (const BodyState& state : ordered) {
        mix(state.handle.index);
        mix(state.handle.generation);
        mix(quantize(state.transform.position.x, 1000, "position.x"));
        mix(quantize(state.transform.position.y, 1000, "position.y"));
        mix(quantize(state.transform.position.z, 1000, "position.z"));
        mix(quantize(state.transform.rotation.x, 32767, "rotation.x"));
        mix(quantize(state.transform.rotation.y, 32767, "rotation.y"));
        mix(quantize(state.transform.rotation.z, 32767, "rotation.z"));
        mix(quantize(state.transform.rotation.w, 32767, "rotation.w"));
        mix(quantize(state.linear_velocity.x, 1000, "linear_velocity.x"));
        mix(quantize(state.linear_velocity.y, 1000, "linear_velocity.y"));
        mix(quantize(state.linear_velocity.z, 1000, "linear_velocity.z"));
        mix(quantize(state.angular_velocity.x, 1000, "angular_velocity.x"));
        mix(quantize(state.angular_velocity.y, 1000, "angular_velocity.y"));
        mix(quantize(state.angular_velocity.z, 1000, "angular_velocity.z"));
        mix(state.awake);
        mix(state.ejected);
    }
    return hash;
}

int ScenarioRunner::watchdog_timeout_seconds(ScenarioKind kind) noexcept
{
    if (kind == ScenarioKind::Stress) {
        return NINHO_STRESS_WATCHDOG_TIMEOUT_SECONDS;
    }
    return watchdog_timeout_seconds();
}

ScenarioResult ScenarioRunner::run(
    ScenarioKind kind, std::uint64_t seed, int substeps) const
{
    constexpr std::array valid_kinds{
        ScenarioKind::RadialFall,
        ScenarioKind::ProjectilePile,
        ScenarioKind::RadialPile,
        ScenarioKind::MassRatio,
        ScenarioKind::Stress,
        ScenarioKind::CapabilityMatrix,
    };
    if (std::ranges::find(valid_kinds, kind) == valid_kinds.end()) {
        throw std::invalid_argument("unknown ScenarioKind value");
    }
    if (auto mismatch = detail::configuration_mismatch_result(
            kind, seed, substeps, current_runtime_configuration())) {
        return std::move(*mismatch);
    }
    ScenarioWatchdog watchdog(kind, seed);
    WatchdogActivation activation(watchdog);
    watchdog.checkpoint(0);
    const std::int64_t allocator_baseline = detail::box3d_allocator_byte_count();
    ScenarioResult result;
    if (kind == ScenarioKind::RadialFall) {
        result = run_radial_fall(seed, substeps);
    } else if (kind == ScenarioKind::ProjectilePile) {
        result = run_projectile_pile(seed);
    } else if (kind == ScenarioKind::RadialPile) {
        result = run_stability_pile(kind, seed, substeps);
    } else if (kind == ScenarioKind::MassRatio) {
        result = run_stability_pile(kind, seed, substeps);
    } else if (kind == ScenarioKind::Stress) {
        result = run_stress(seed, substeps);
    } else if (kind == ScenarioKind::CapabilityMatrix) {
        result = run_capability_matrix(seed, substeps);
    }
    const std::int64_t allocator_final = detail::box3d_allocator_byte_count();
    result.box3d_allocator.baseline_bytes = allocator_baseline;
    result.box3d_allocator.final_bytes = allocator_final;
    result.box3d_allocator.max_abs_delta = std::max(
        result.box3d_allocator.max_abs_delta,
        std::abs(allocator_final - allocator_baseline));
    result.box3d_allocator.exact_return = result.box3d_allocator.exact_return
        && allocator_baseline == 0 && allocator_final == allocator_baseline;
    if (!result.box3d_allocator.exact_return
        && std::ranges::none_of(result.violations, [](const ScenarioViolation& violation) {
               return violation.code == "box3d_allocator_imbalance";
           })) {
        add_violation(
            result,
            "box3d_allocator_imbalance",
            "Box3 allocator did not return exactly to the isolated process baseline",
            result.ticks,
            {},
            {{"baseline_bytes", static_cast<double>(allocator_baseline), "bytes"},
             {"final_bytes", static_cast<double>(allocator_final), "bytes"}});
    }
    return result;
}

}
