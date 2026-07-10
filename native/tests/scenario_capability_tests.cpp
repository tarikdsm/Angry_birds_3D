#include "test_framework.hpp"

#include <ninho/physics/scenario.hpp>

#include <algorithm>
#include <array>
#include <string_view>

using namespace ninho::physics;

namespace {

const CapabilityRow& row(const ScenarioResult& result, std::string_view capability)
{
    const auto found = std::find_if(
        result.matrix.begin(), result.matrix.end(), [&](const CapabilityRow& value) {
            return value.capability == capability;
        });
    NINHO_REQUIRE(found != result.matrix.end());
    return *found;
}

double value(const CapabilityRow& capability, std::string_view name)
{
    const auto found = std::find_if(
        capability.values.begin(), capability.values.end(), [&](const ScenarioValue& metric) {
            return metric.name == name;
        });
    NINHO_REQUIRE(found != capability.values.end());
    return found->value;
}

const ScenarioResult& matrix_result()
{
    static const ScenarioResult result =
        ScenarioRunner{}.run(ScenarioKind::CapabilityMatrix, 5, 4);
    return result;
}

}

NINHO_TEST("capability matrix reports every mandatory row without blocked status")
{
    const auto& result = matrix_result();
    const std::array expected{
        "ccd_dynamic_dynamic",
        "shape_cast_overlap",
        "contact_hit_events",
        "joint_force_torque",
        "hulls_compounds",
        "radial_sleep",
        "batch_lifecycle",
        "upstream_replay",
    };
    NINHO_REQUIRE(result.matrix.size() == expected.size());
    for (const char* capability : expected) {
        NINHO_REQUIRE(row(result, capability).status != CapabilityStatus::Blocked);
    }
    NINHO_REQUIRE(result.violations.empty());
}

NINHO_TEST("capability matrix keeps query contact and joint proof values")
{
    const auto& result = matrix_result();
    const auto& query = row(result, "shape_cast_overlap");
    NINHO_REQUIRE(query.status == CapabilityStatus::Pass);
    NINHO_REQUIRE(query.fallback.empty());
    NINHO_REQUIRE(value(query, "cast_distance") == 3.0);
    NINHO_REQUIRE(value(query, "first_handle_matches_overlap") == 1.0);

    const auto& contact = row(result, "contact_hit_events");
    NINHO_REQUIRE(
        contact.status == CapabilityStatus::Pass
        || (contact.status == CapabilityStatus::Fallback
            && contact.fallback == "derived_relative_energy"));
    NINHO_REQUIRE(value(contact, "approach_speed") > 0.0);
    NINHO_REQUIRE(value(contact, "effective_mass") > 0.0);
    NINHO_REQUIRE(value(contact, "derived_energy") > 0.0);
    NINHO_REQUIRE(value(contact, "unique_pairs") >= 1.0);

    const auto& joint = row(result, "joint_force_torque");
    NINHO_REQUIRE(joint.status != CapabilityStatus::Blocked);
    NINHO_REQUIRE(value(joint, "monotonic_tolerance") == 50.0);
    NINHO_REQUIRE(value(joint, "maximum_force") > 10000.0);
}

NINHO_TEST("capability matrix performs lifecycle and official replay proof")
{
    const auto& result = matrix_result();
    const auto& lifecycle = row(result, "batch_lifecycle");
    NINHO_REQUIRE(value(lifecycle, "generation_cycles") == 10000.0);
    NINHO_REQUIRE(value(lifecycle, "invalid_handles") == 0.0);

    const auto& replay = row(result, "upstream_replay");
    NINHO_REQUIRE(
        replay.status == CapabilityStatus::Pass
        || (replay.status == CapabilityStatus::Fallback
            && replay.fallback == "input_metric_replay"));
    NINHO_REQUIRE(value(replay, "temporary_file_removed") == 1.0);
}
