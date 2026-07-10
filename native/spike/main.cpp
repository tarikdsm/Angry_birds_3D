#include <ninho/physics/scenario.hpp>

#include <algorithm>
#include <array>
#include <charconv>
#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <exception>
#include <fstream>
#include <iostream>
#include <limits>
#include <optional>
#include <string>
#include <string_view>
#include <system_error>
#include <vector>

using namespace ninho::physics;

namespace {

struct Options {
    bool all{};
    std::optional<ScenarioKind> scenario;
    std::uint64_t seed{1};
    int substeps{4};
    int repeat{1};
    std::optional<std::filesystem::path> json_path;
};

[[nodiscard]] bool parse_unsigned(std::string_view text, std::uint64_t& value)
{
    if (text.empty() || text.front() == '-') {
        return false;
    }
    const auto parsed = std::from_chars(text.data(), text.data() + text.size(), value);
    return parsed.ec == std::errc{} && parsed.ptr == text.data() + text.size();
}

[[nodiscard]] bool parse_positive_int(std::string_view text, int& value)
{
    std::uint64_t parsed{};
    if (!parse_unsigned(text, parsed) || parsed == 0
        || parsed > static_cast<std::uint64_t>(std::numeric_limits<int>::max())) {
        return false;
    }
    value = static_cast<int>(parsed);
    return true;
}

[[nodiscard]] std::optional<Options> parse_options(
    int argc, char** argv, std::string& diagnostic)
{
    Options options;
    bool saw_seed = false;
    bool saw_substeps = false;
    bool saw_repeat = false;
    bool saw_json = false;
    for (int index = 1; index < argc; ++index) {
        const std::string_view option = argv[index];
        const auto require_value = [&](std::string_view name) -> std::optional<std::string_view> {
            if (index + 1 >= argc) {
                diagnostic = std::string{name} + " requires a value";
                return std::nullopt;
            }
            ++index;
            const std::string_view value = argv[index];
            if (value.empty() || value.starts_with("--")) {
                diagnostic = std::string{name} + " requires a non-empty value";
                return std::nullopt;
            }
            return value;
        };

        if (option == "--all") {
            if (options.all) {
                diagnostic = "--all may be specified only once";
                return std::nullopt;
            }
            options.all = true;
        } else if (option == "--scenario") {
            if (options.scenario) {
                diagnostic = "--scenario may be specified only once";
                return std::nullopt;
            }
            const auto value = require_value(option);
            if (!value) {
                return std::nullopt;
            }
            options.scenario = parse_scenario_kind(*value);
            if (!options.scenario) {
                diagnostic = "unknown scenario: " + std::string{*value};
                return std::nullopt;
            }
        } else if (option == "--seed") {
            if (saw_seed) {
                diagnostic = "--seed may be specified only once";
                return std::nullopt;
            }
            saw_seed = true;
            const auto value = require_value(option);
            if (!value || !parse_unsigned(*value, options.seed)) {
                if (diagnostic.empty()) {
                    diagnostic = "--seed must be an unsigned 64-bit integer";
                }
                return std::nullopt;
            }
        } else if (option == "--substeps") {
            if (saw_substeps) {
                diagnostic = "--substeps may be specified only once";
                return std::nullopt;
            }
            saw_substeps = true;
            const auto value = require_value(option);
            if (!value || !parse_positive_int(*value, options.substeps)) {
                if (diagnostic.empty()) {
                    diagnostic = "--substeps must be a positive integer";
                }
                return std::nullopt;
            }
        } else if (option == "--repeat") {
            if (saw_repeat) {
                diagnostic = "--repeat may be specified only once";
                return std::nullopt;
            }
            saw_repeat = true;
            const auto value = require_value(option);
            if (!value || !parse_positive_int(*value, options.repeat)) {
                if (diagnostic.empty()) {
                    diagnostic = "--repeat must be a positive integer";
                }
                return std::nullopt;
            }
        } else if (option == "--json") {
            if (saw_json) {
                diagnostic = "--json may be specified only once";
                return std::nullopt;
            }
            saw_json = true;
            const auto value = require_value(option);
            if (!value) {
                return std::nullopt;
            }
            options.json_path = std::filesystem::path{std::string{*value}};
        } else {
            diagnostic = "unknown option: " + std::string{option};
            return std::nullopt;
        }
    }
    if (options.all == options.scenario.has_value()) {
        diagnostic = "select exactly one of --all or --scenario <name>";
        return std::nullopt;
    }
    return options;
}

[[nodiscard]] std::string detect_cpu()
{
#ifdef _MSC_VER
    char* identifier = nullptr;
    std::size_t length = 0;
    if (_dupenv_s(&identifier, &length, "PROCESSOR_IDENTIFIER") == 0
        && identifier != nullptr && length > 1) {
        const std::string result{identifier};
        std::free(identifier);
        return result;
    }
    std::free(identifier);
#else
    if (const char* identifier = std::getenv("PROCESSOR_IDENTIFIER")) {
        if (*identifier != '\0') {
            return identifier;
        }
    }
#endif
    return "unknown";
}

[[nodiscard]] std::vector<ScenarioKind> selected_scenarios(const Options& options)
{
    if (options.scenario) {
        return {*options.scenario};
    }
    return {
        ScenarioKind::RadialFall,
        ScenarioKind::ProjectilePile,
        ScenarioKind::RadialPile,
        ScenarioKind::MassRatio,
        ScenarioKind::Stress,
        ScenarioKind::CapabilityMatrix,
    };
}

void add_determinism_violation(
    ScenarioResult& result,
    int repeat_index,
    std::uint64_t expected,
    std::uint64_t actual)
{
    result.violations.push_back({
        .scenario = result.name,
        .code = "determinism_hash_mismatch",
        .message = "canonical hashes differ inside the same executable and configuration",
        .tick = result.ticks,
        .values = {{"repeat", static_cast<double>(repeat_index), "count"}},
        .details = {
            {"expected_hash", std::to_string(expected)},
            {"actual_hash", std::to_string(actual)},
        },
    });
}

[[nodiscard]] bool write_binary_utf8(
    const std::filesystem::path& path, std::string_view document)
{
    std::error_code error;
    if (!path.parent_path().empty()) {
        std::filesystem::create_directories(path.parent_path(), error);
        if (error) {
            return false;
        }
    }
    std::ofstream stream(path, std::ios::binary | std::ios::trunc);
    if (!stream) {
        return false;
    }
    stream.write(document.data(), static_cast<std::streamsize>(document.size()));
    stream.flush();
    return stream.good();
}

}

int main(int argc, char** argv)
{
    std::string diagnostic;
    const auto options = parse_options(argc, argv, diagnostic);
    if (!options) {
        std::cerr << "error: " << diagnostic << '\n'
                  << "usage: ninho_physics_spike (--all | --scenario <name>) "
                     "[--seed <uint64>] [--substeps <positive>] "
                     "[--repeat <positive>] [--json <path>]\n";
        return 2;
    }

    ScenarioReport report{
        .build_type = NINHO_BUILD_TYPE,
        .cpu = detect_cpu(),
        .seed = options->seed,
        .substeps = options->substeps,
        .repeat = options->repeat,
    };
    ScenarioRunner::set_emergency_json_path(
        options->json_path
            ? std::optional<std::string>{options->json_path->string()}
            : std::nullopt);
    ScenarioRunner runner;
    bool hash_mismatch = false;
    for (const ScenarioKind kind : selected_scenarios(*options)) {
        std::optional<ScenarioResult> first_result;
        std::vector<std::uint64_t> hashes;
        hashes.reserve(static_cast<std::size_t>(options->repeat));
        for (int repeat_index = 1; repeat_index <= options->repeat; ++repeat_index) {
            const auto started = std::chrono::steady_clock::now();
            ScenarioResult current;
            try {
                current = runner.run(kind, options->seed, options->substeps);
            } catch (const std::exception& error) {
                current.kind = kind;
                current.name = std::string{scenario_name(kind)};
                current.seed = options->seed;
                current.substeps = options->substeps;
                current.violations.push_back({
                    .scenario = current.name,
                    .code = "scenario_exception",
                    .message = error.what(),
                });
            }
            const double elapsed = std::chrono::duration<double>(
                                       std::chrono::steady_clock::now() - started)
                                       .count();
            if (elapsed > 60.0) {
                current.violations.push_back({
                    .scenario = current.name,
                    .code = "scenario_timeout",
                    .message = "scenario exceeded the fixed 60 second timeout",
                    .tick = current.ticks,
                    .values = {{"elapsed", elapsed, "s"}},
                });
            }
            hashes.push_back(current.final_hash);
            if (!first_result) {
                first_result = std::move(current);
            } else {
                first_result->violations.insert(
                    first_result->violations.end(),
                    current.violations.begin(),
                    current.violations.end());
                if (current.final_hash != hashes.front()) {
                    hash_mismatch = true;
                    add_determinism_violation(
                        *first_result,
                        repeat_index,
                        hashes.front(),
                        current.final_hash);
                }
            }
        }
        first_result->repeat_hashes = std::move(hashes);
        report.scenarios.push_back(std::move(*first_result));
    }

    std::string document;
    try {
        document = report.to_json();
    } catch (const std::exception& error) {
        std::cerr << "error: failed to serialize report: " << error.what() << '\n';
        return 1;
    }
    if (options->json_path) {
        if (!write_binary_utf8(*options->json_path, document)) {
            std::cerr << "error: could not write JSON report: "
                      << options->json_path->string() << '\n';
            return 1;
        }
    } else {
        std::cout << document << '\n';
    }

    return scenario_exit_code(report.scenarios, hash_mismatch);
}
