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
    const auto repeated = ScenarioRunner{}.run(ScenarioKind::CapabilityMatrix, 5, 4);
    const auto radial = ScenarioRunner{}.run(ScenarioKind::RadialPile, 5, 4);
    NINHO_REQUIRE(result.final_hash == repeated.final_hash);
    NINHO_REQUIRE(result.final_hash != radial.final_hash);
}

NINHO_TEST("capability row hash covers every canonical field and ignores value order")
{
    std::vector<CapabilityRow> rows{
        {.capability = "a",
         .status = CapabilityStatus::Pass,
         .values = {{"z", 2, "m"}, {"a", 1, "m"}},
         .fixture_hashes = {11, 22}},
        {.capability = "b",
         .status = CapabilityStatus::Fallback,
         .fallback = "approved",
         .values = {{"value", 3, "N"}},
         .fixture_hashes = {33}},
    };
    const std::uint64_t baseline = hash_capability_rows(rows);
    std::swap(rows[0].values[0], rows[0].values[1]);
    std::swap(rows[0], rows[1]);
    NINHO_REQUIRE(hash_capability_rows(rows) == baseline);
    std::swap(rows[0], rows[1]);

    for (std::size_t index = 0; index < rows.size(); ++index) {
        auto changed = rows;
        changed[index].values.front().value += 1;
        NINHO_REQUIRE(hash_capability_rows(changed) != baseline);
    }
    auto changed = rows;
    changed[0].capability = "changed";
    NINHO_REQUIRE(hash_capability_rows(changed) != baseline);
    changed = rows;
    changed[0].status = CapabilityStatus::Blocked;
    NINHO_REQUIRE(hash_capability_rows(changed) != baseline);
    changed = rows;
    changed[0].fallback = "changed";
    NINHO_REQUIRE(hash_capability_rows(changed) != baseline);
    changed = rows;
    changed[0].fixture_hashes[0] += 1;
    NINHO_REQUIRE(hash_capability_rows(changed) != baseline);
}

NINHO_TEST("joint fallback requires two consecutive qualifying ticks")
{
    const std::array isolated{0.02, 0.0, 0.03};
    const std::array consecutive{0.0, 0.02, 0.03};
    NINHO_REQUIRE(!has_two_consecutive_samples(isolated, 0.01));
    NINHO_REQUIRE(has_two_consecutive_samples(consecutive, 0.01));
}

NINHO_TEST("lifecycle cannot pass without working set evidence")
{
    NINHO_REQUIRE(
        classify_lifecycle_status(10000, 0, std::nullopt)
        == CapabilityStatus::Blocked);
    NINHO_REQUIRE(
        classify_lifecycle_status(10000, 0, 0.01)
        == CapabilityStatus::Pass);
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

    const auto& compound = row(result, "hulls_compounds");
    NINHO_REQUIRE_NEAR(value(compound, "mass"), value(compound, "expected_mass"), 1.0e-4);
    NINHO_REQUIRE_NEAR(value(compound, "bounds_lower_x"), -1.62, 1.0e-4);
    NINHO_REQUIRE_NEAR(value(compound, "bounds_upper_x"), 1.62, 1.0e-4);
    NINHO_REQUIRE(value(compound, "contacted") == 1.0);
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
