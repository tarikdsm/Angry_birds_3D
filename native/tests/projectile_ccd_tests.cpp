#include "test_framework.hpp"
#include "scenario_test_facade.hpp"

#include <ninho/physics/physics_world.hpp>
#include <ninho/physics/scenario.hpp>

#include <cstdint>
#include <ranges>

using namespace ninho::physics;

namespace {

struct ThinTargetOutcome {
    bool projectile_contact{};
    float projectile_x{};
};

[[nodiscard]] ThinTargetOutcome run_thin_dynamic_target(bool bullet)
{
    constexpr float target_center_x = 1.0f;
    constexpr float target_half_thickness = 0.025f;
    constexpr float projectile_radius = 0.05f;
    constexpr float projectile_speed = 90.0f;
    PhysicsWorld world(WorldConfig{
        .substeps = 4,
        .surface_gravity = 0.0f,
        .max_bodies = 2,
    });

    BodyDesc target = BodyDesc::dynamic_box(
        {target_half_thickness, 2.0f, 2.0f},
        {{target_center_x, 5.0f, 0.0f}, {}},
        100'000.0f);
    target.radial_gravity = false;
    target.remove_beyond_six_r = false;
    const auto target_result = world.create_body(target);

    BodyDesc projectile = BodyDesc::dynamic_sphere(
        projectile_radius, {{0.0f, 5.0f, 0.0f}, {}}, 1.0f);
    projectile.linear_velocity = {projectile_speed, 0.0f, 0.0f};
    projectile.bullet = bullet;
    projectile.enable_sleep = false;
    projectile.radial_gravity = false;
    projectile.remove_beyond_six_r = false;
    const auto projectile_result = world.create_body(projectile);

    NINHO_REQUIRE(target_result && projectile_result);
    bool projectile_contact = false;
    for (int tick = 0; tick < 2; ++tick) {
        world.step();
        for (const ContactHit& hit : world.contact_hits()) {
            projectile_contact = projectile_contact
                || (hit.a == projectile_result.value && hit.b == target_result.value)
                || (hit.a == target_result.value && hit.b == projectile_result.value);
        }
    }
    const auto state = world.state(projectile_result.value);
    NINHO_REQUIRE(state.has_value());
    return {
        .projectile_contact = projectile_contact,
        .projectile_x = state->transform.position.x,
    };
}

}

NINHO_TEST("bullet flag isolates continuous collision against a thin dynamic target")
{
    const ThinTargetOutcome discrete = run_thin_dynamic_target(false);
    const ThinTargetOutcome continuous = run_thin_dynamic_target(true);

    NINHO_REQUIRE(!discrete.projectile_contact);
    NINHO_REQUIRE(discrete.projectile_x > 1.1f);
    NINHO_REQUIRE(continuous.projectile_x < 1.0f);
    NINHO_REQUIRE(continuous.projectile_contact);
}

NINHO_TEST("projectile scenario repeats execute fresh simulations")
{
    detail::ScenarioTestFacade::reset_projectile_simulation_count();
    const ScenarioRunner runner;

    static_cast<void>(runner.run(ScenarioKind::ProjectilePile, 1, 4));
    const std::size_t after_first_run =
        detail::ScenarioTestFacade::projectile_simulation_count();
    static_cast<void>(runner.run(ScenarioKind::ProjectilePile, 1, 4));

    NINHO_REQUIRE(after_first_run > 0);
    NINHO_REQUIRE(
        detail::ScenarioTestFacade::projectile_simulation_count() > after_first_run);
}

NINHO_TEST("ccd capability repeats execute fresh simulations")
{
    detail::ScenarioTestFacade::reset_projectile_simulation_count();
    const ScenarioRunner runner;

    static_cast<void>(runner.run(ScenarioKind::CapabilityMatrix, 1, 4));
    const std::size_t after_first_run =
        detail::ScenarioTestFacade::projectile_simulation_count();
    static_cast<void>(runner.run(ScenarioKind::CapabilityMatrix, 1, 4));

    NINHO_REQUIRE(after_first_run > 0);
    NINHO_REQUIRE(
        detail::ScenarioTestFacade::projectile_simulation_count() > after_first_run);
}

NINHO_TEST("bullet contacts pile in all fixed seeds")
{
    for (std::uint64_t seed = 1; seed <= 20; ++seed) {
        const auto result =
            ScenarioRunner{}.run(ScenarioKind::ProjectilePile, seed, 4);
        NINHO_REQUIRE(result.contact_before_pile_exit);
        NINHO_REQUIRE(result.violations.empty());
    }
}

NINHO_TEST("projectile gate uses only the approved fixture and fallback")
{
    const auto result = ScenarioRunner{}.run(ScenarioKind::ProjectilePile, 1, 4);
    NINHO_REQUIRE(result.dynamic_body_count == 121);
    NINHO_REQUIRE(result.shape_count == 123);
    NINHO_REQUIRE(result.joint_count == 0);
    NINHO_REQUIRE(result.peak_body_count == 123);
    NINHO_REQUIRE(result.peak_shape_count == 123);
    NINHO_REQUIRE(result.peak_joint_count == 0);
    NINHO_REQUIRE(result.ccd_primary_pass_count >= 0);
    NINHO_REQUIRE(result.ccd_primary_pass_count <= 20);
    if (result.ccd_primary_pass_count == 20) {
        NINHO_REQUIRE(result.fallback.empty());
        NINHO_REQUIRE(result.projectile_speed == 35.0);
        NINHO_REQUIRE(result.substeps == 4);
        NINHO_REQUIRE(result.ccd_fallback_pass_count == 0);
    } else {
        NINHO_REQUIRE(result.fallback == "speed30_substeps6");
        NINHO_REQUIRE(result.projectile_speed == 30.0);
        NINHO_REQUIRE(result.substeps == 6);
        NINHO_REQUIRE(result.ccd_fallback_pass_count == 20);
    }
    NINHO_REQUIRE(result.violations.empty());
}

NINHO_TEST("projectile gate reports an unevaluated fallback without a false pass")
{
    const auto result = ScenarioRunner{}.run(ScenarioKind::ProjectilePile, 1, 4);
    const auto limit = std::ranges::find(
        result.limits, "fallback_seed_passes", &ScenarioLimit::name);
    const auto evaluated = std::ranges::find(
        result.metrics, "fallback_seed_evaluated", &ScenarioValue::name);
    NINHO_REQUIRE(limit != result.limits.end());
    NINHO_REQUIRE(evaluated != result.metrics.end());
    if (result.ccd_primary_pass_count == 20) {
        NINHO_REQUIRE(limit->comparison == "==");
        NINHO_REQUIRE(limit->value == 0.0);
        NINHO_REQUIRE(evaluated->value == 0.0);
    } else {
        NINHO_REQUIRE(limit->value == 20.0);
        NINHO_REQUIRE(evaluated->value == 1.0);
    }
}
