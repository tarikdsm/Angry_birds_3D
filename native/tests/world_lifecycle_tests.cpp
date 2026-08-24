#include "test_framework.hpp"
#include "physics_world_test_facade.hpp"

#include "box3d_world_lifecycle_probe.hpp"

#include <ninho/physics/physics_world.hpp>
#include <ninho/physics/world_bounds.hpp>

#include <algorithm>
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <mutex>
#include <thread>
#include <vector>
#include <stdexcept>
#include <type_traits>
#include <utility>

using namespace ninho::physics;

template<class T>
concept HasLegacyPlanetRadius = requires(T value) { value.planet_radius; };

template<class T>
concept HasLegacySurfaceGravity = requires(T value) { value.surface_gravity; };

template<class T>
concept HasLegacyRadialGravity = requires(T value) { value.radial_gravity; };

template<class T>
concept HasLegacySixRadiusRemoval = requires(T value) { value.remove_beyond_six_r; };

static_assert(!HasLegacyPlanetRadius<WorldConfig>);
static_assert(!HasLegacySurfaceGravity<WorldConfig>);
static_assert(!HasLegacyRadialGravity<BodyDesc>);
static_assert(!HasLegacySixRadiusRemoval<BodyDesc>);

static_assert(!std::is_copy_constructible_v<PhysicsWorld>);
static_assert(!std::is_copy_assignable_v<PhysicsWorld>);
static_assert(std::is_nothrow_move_constructible_v<PhysicsWorld>);
static_assert(std::is_nothrow_move_assignable_v<PhysicsWorld>);

NINHO_TEST("contract Box3D world remains single worker")
{
    const PhysicsWorld world(WorldConfig{});
    NINHO_REQUIRE(detail::PhysicsWorldTestFacade::worker_count(world) == 1);
}

NINHO_TEST("world applies one uniform gravity sample to every runtime dynamic body")
{
    PhysicsWorld world(WorldConfig{
        .gravity = UniformGravityConfig{.acceleration_m_s2 = {0.0f, -9.0f, 0.0f}},
        .bounds = NoWorldBounds{},
    });
    NINHO_REQUIRE((world.gravity_at({17.0f, -3.0f, 2.0f}) == Vec3{0.0f, -9.0f, 0.0f}));
    NINHO_REQUIRE(detail::PhysicsWorldTestFacade::box3d_gravity(world) == Vec3{});

    world.step();
    const BodyHandle body = world.create_body(
        BodyDesc::dynamic_sphere(0.5f, {{0.0f, 5.0f, 0.0f}, {}}, 1.0f)).value;
    world.step();
    const auto state = world.state(body);
    NINHO_REQUIRE(state.has_value());
    NINHO_REQUIRE_NEAR(state->linear_velocity.y, -9.0f / 60.0f, 1.0e-5f);
}

NINHO_TEST("world publishes one exact typed config and preselects evaluator strategies")
{
    const RadialGravityConfig radial{
        .center_m = {},
        .reference_radius_m = 10.0f,
        .reference_acceleration_m_s2 = 9.0f,
    };
    const SphericalWorldBounds sphere{
        .center_m = {},
        .removal_radius_m = 60.0f,
    };
    PhysicsWorld orbital(WorldConfig{.gravity = radial, .bounds = sphere});
    NINHO_REQUIRE(std::get<RadialGravityConfig>(orbital.config().gravity).center_m == radial.center_m);
    NINHO_REQUIRE(std::get<RadialGravityConfig>(orbital.config().gravity).reference_radius_m
        == radial.reference_radius_m);
    NINHO_REQUIRE(std::get<SphericalWorldBounds>(orbital.config().bounds).removal_radius_m
        == sphere.removal_radius_m);

    PhysicsWorld terrestrial(WorldConfig{
        .gravity = UniformGravityConfig{.acceleration_m_s2 = {0.0f, -9.81f, 0.0f}},
        .bounds = AabbWorldBounds{.minimum_m = {-1.0f, -1.0f, -1.0f},
            .maximum_m = {1.0f, 1.0f, 1.0f}},
    });
    NINHO_REQUIRE(detail::PhysicsWorldTestFacade::gravity_strategy(orbital)
        != detail::PhysicsWorldTestFacade::gravity_strategy(terrestrial));
    NINHO_REQUIRE(detail::PhysicsWorldTestFacade::bounds_strategy(orbital)
        != detail::PhysicsWorldTestFacade::bounds_strategy(terrestrial));
    NINHO_REQUIRE(detail::PhysicsWorldTestFacade::ejection_strategy(orbital)
        != detail::PhysicsWorldTestFacade::ejection_strategy(terrestrial));
}

NINHO_TEST("world rejects authored dynamic gravity opt out")
{
    PhysicsWorld world(WorldConfig{
        .gravity = UniformGravityConfig{.acceleration_m_s2 = {0.0f, -9.0f, 0.0f}},
        .bounds = NoWorldBounds{},
    });
    BodyDesc dynamic = BodyDesc::dynamic_sphere(0.5f, {{0.0f, 5.0f, 0.0f}, {}}, 1.0f);
    dynamic.affected_by_world_gravity = false;
    NINHO_REQUIRE(world.create_body(dynamic).status.code == StatusCode::InvalidArgument);

    BodyDesc anchored = BodyDesc::static_box({0.5f, 0.5f, 0.5f}, {});
    anchored.affected_by_world_gravity = false;
    const BodyHandle anchored_handle = world.create_body(anchored).value;
    world.step();
    const auto anchored_state = world.state(anchored_handle);
    NINHO_REQUIRE(anchored_state.has_value());
    NINHO_REQUIRE(anchored_state->transform.position == Vec3{});
    NINHO_REQUIRE(anchored_state->linear_velocity == Vec3{});
}

NINHO_TEST("world AABB exit removal follows body policy")
{
    PhysicsWorld world(WorldConfig{
        .gravity = UniformGravityConfig{},
        .bounds = AabbWorldBounds{
            .minimum_m = {-1.0f, -1.0f, -1.0f},
            .maximum_m = {1.0f, 1.0f, 1.0f},
        },
    });
    BodyDesc removed = BodyDesc::dynamic_sphere(0.1f, {{2.0f, 0.0f, 0.0f}, {}}, 1.0f);
    removed.world_exit_policy = WorldExitPolicy::RemoveOutsideBounds;
    BodyDesc retained = BodyDesc::dynamic_sphere(0.1f, {{3.0f, 0.0f, 0.0f}, {}}, 1.0f);
    retained.world_exit_policy = WorldExitPolicy::KeepOutsideBounds;
    const BodyHandle removed_handle = world.create_body(removed).value;
    const BodyHandle retained_handle = world.create_body(retained).value;

    world.step();
    NINHO_REQUIRE(world.state(removed_handle)->exited_world);
    NINHO_REQUIRE(world.state(retained_handle)->exited_world);
    NINHO_REQUIRE(!world.state(removed_handle)->ejected);
    NINHO_REQUIRE(!world.state(retained_handle)->ejected);
    world.step();
    NINHO_REQUIRE(!world.state(removed_handle).has_value());
    NINHO_REQUIRE(world.state(retained_handle).has_value());
}

NINHO_TEST("world publishes radial ejection and spherical exit on the same tick")
{
    const auto config = make_legacy_radial_world_config({.surface_gravity = 0.0f});
    const auto run = [&](WorldExitPolicy policy) {
        PhysicsWorld world(config);
        BodyDesc body = BodyDesc::dynamic_sphere(
            0.1f, {{40.0f, 0.0f, 0.0f}, {}}, 1.0f);
        body.linear_velocity = {40.0f, 0.0f, 0.0f};
        body.world_exit_policy = policy;
        const BodyHandle handle = world.create_body(body).value;
        for (int tick = 0; tick < 29; ++tick) {
            world.step();
            NINHO_REQUIRE(!world.state(handle)->ejected);
            NINHO_REQUIRE(!world.state(handle)->exited_world);
        }
        world.step();
        const auto crossing = world.state(handle);
        NINHO_REQUIRE(crossing.has_value());
        NINHO_REQUIRE(crossing->ejected);
        NINHO_REQUIRE(crossing->exited_world);
        world.step();
        return world.state(handle).has_value();
    };

    NINHO_REQUIRE(run(WorldExitPolicy::KeepOutsideBounds));
    NINHO_REQUIRE(!run(WorldExitPolicy::RemoveOutsideBounds));
}

NINHO_TEST("uniform world preserves deterministic sleep and explicit wake")
{
    const WorldConfig config{
        .gravity = UniformGravityConfig{},
        .bounds = NoWorldBounds{},
    };
    const auto run = [&] {
        PhysicsWorld world(config);
        const BodyHandle body = world.create_body(
            BodyDesc::dynamic_sphere(0.5f, {}, 1.0f)).value;
        for (int tick = 0; tick < 120; ++tick) {
            world.step();
        }
        NINHO_REQUIRE(!world.state(body)->awake);
        NINHO_REQUIRE(world.apply_impulse(body, {1.0f, 0.0f, 0.0f}, {}, true).ok());
        world.step();
        NINHO_REQUIRE(world.state(body)->awake);
        return *world.state(body);
    };

    const BodyState first = run();
    const BodyState second = run();
    NINHO_REQUIRE(first.handle == second.handle);
    NINHO_REQUIRE(first.transform == second.transform);
    NINHO_REQUIRE(first.linear_velocity == second.linear_velocity);
}

NINHO_TEST("world rejects destroyed handle after slot reuse")
{
    PhysicsWorld world(WorldConfig{});
    BodyDesc box = BodyDesc::dynamic_box({0.5f, 0.5f, 0.5f}, {{0, 15, 0}, {}}, 520.0f);
    const BodyHandle old = world.create_body(box).value;
    world.step();
    NINHO_REQUIRE(world.destroy_body(old).ok());
    world.step();
    const BodyHandle replacement = world.create_body(box).value;
    NINHO_REQUIRE(old.index == replacement.index);
    NINHO_REQUIRE(old.generation != replacement.generation);
    NINHO_REQUIRE(world.apply_impulse(old, {1, 0, 0}, {0, 0, 0}).code == StatusCode::InvalidHandle);
}

NINHO_TEST("world dynamic sphere falls toward center and settles on planet")
{
    WorldConfig config = make_legacy_radial_world_config(
        {.substeps = 4, .planet_radius = 10, .surface_gravity = 9});
    PhysicsWorld world(config);
    world.create_body(BodyDesc::static_sphere(10.0f, {{0, 0, 0}, {}}));
    const auto ball =
        world.create_body(BodyDesc::dynamic_sphere(0.4f, {{0, 15, 0}, {}}, 520.0f)).value;
    for (int i = 0; i < 600; ++i) {
        world.step();
    }
    const auto state = world.state(ball);
    NINHO_REQUIRE(state.has_value());
    NINHO_REQUIRE_NEAR(length(state->transform.position), 10.4f, 0.03f);
    NINHO_REQUIRE(length(state->linear_velocity) < 0.05f);
}

NINHO_TEST("world snapshot contains only live public handles")
{
    PhysicsWorld world(WorldConfig{});
    auto a = world.create_body(BodyDesc::dynamic_sphere(0.4f, {{0, 15, 0}, {}}, 520)).value;
    auto b = world.create_body(BodyDesc::dynamic_box({1, 1, 1}, {{0, 18, 0}, {}}, 520)).value;
    world.step();
    world.destroy_body(a);
    world.step();
    NINHO_REQUIRE(world.states().size() == 1);
    NINHO_REQUIRE(world.states()[0].handle == b);
}

NINHO_TEST("contract configuration validation rejects unsafe values")
{
    const auto rejected = [](WorldConfig config) {
        try {
            PhysicsWorld physics(config);
        } catch (const std::invalid_argument&) {
            return true;
        }
        return false;
    };

    WorldConfig config{};
    config.time_step = 0.0f;
    NINHO_REQUIRE(rejected(config));
    config = {};
    config.substeps = 0;
    NINHO_REQUIRE(rejected(config));
    config = {};
    config.gravity = RadialGravityConfig{.reference_radius_m = 0.0f};
    NINHO_REQUIRE(rejected(config));
    config = {};
    config.gravity = RadialGravityConfig{.reference_acceleration_m_s2 = -1.0f};
    NINHO_REQUIRE(rejected(config));
    config = {};
    config.max_bodies = 0;
    NINHO_REQUIRE(rejected(config));
    config = {};
    config.time_step = std::numeric_limits<float>::quiet_NaN();
    NINHO_REQUIRE(rejected(config));
    config = {};
    config.gravity = RadialGravityConfig{
        .reference_radius_m = std::numeric_limits<float>::max(),
    };
    NINHO_REQUIRE(rejected(config));
}

NINHO_TEST("contract configuration rejects surface gravity above the acceleration ceiling")
{
    const auto rejected = [](float surface_gravity) {
        try {
            PhysicsWorld physics(make_legacy_radial_world_config(
                {.surface_gravity = surface_gravity}));
        } catch (const std::invalid_argument&) {
            return true;
        }
        return false;
    };

    NINHO_REQUIRE(!rejected(18.0f));
    NINHO_REQUIRE(rejected(18.01f));
}

NINHO_TEST("contract body validation rejects unsafe primitive data")
{
    PhysicsWorld physics(WorldConfig{});

    BodyDesc empty_dynamic;
    empty_dynamic.type = BodyType::Dynamic;
    NINHO_REQUIRE(physics.create_body(empty_dynamic).status.code == StatusCode::InvalidArgument);

    BodyDesc zero_radius = BodyDesc::dynamic_sphere(0.0f, {}, 1.0f);
    NINHO_REQUIRE(physics.create_body(zero_radius).status.code == StatusCode::InvalidArgument);

    BodyDesc zero_extent = BodyDesc::dynamic_box({1.0f, 0.0f, 1.0f}, {}, 1.0f);
    NINHO_REQUIRE(physics.create_body(zero_extent).status.code == StatusCode::InvalidArgument);

    BodyDesc zero_density = BodyDesc::dynamic_sphere(1.0f, {}, 0.0f);
    NINHO_REQUIRE(physics.create_body(zero_density).status.code == StatusCode::InvalidArgument);

    BodyDesc bad_velocity = BodyDesc::dynamic_sphere(1.0f, {}, 1.0f);
    bad_velocity.linear_velocity.x = std::numeric_limits<float>::infinity();
    NINHO_REQUIRE(physics.create_body(bad_velocity).status.code == StatusCode::InvalidArgument);

    BodyDesc bad_transform = BodyDesc::dynamic_sphere(1.0f, {}, 1.0f);
    bad_transform.transform.rotation.w = 2.0f;
    NINHO_REQUIRE(physics.create_body(bad_transform).status.code == StatusCode::InvalidArgument);

    BodyDesc bad_material = BodyDesc::dynamic_sphere(1.0f, {}, 1.0f);
    bad_material.shapes[0].friction = -0.1f;
    NINHO_REQUIRE(physics.create_body(bad_material).status.code == StatusCode::InvalidArgument);

    BodyDesc bad_type = BodyDesc::dynamic_sphere(1.0f, {}, 1.0f);
    bad_type.type = static_cast<BodyType>(99);
    NINHO_REQUIRE(physics.create_body(bad_type).status.code == StatusCode::InvalidArgument);
}

NINHO_TEST("contract pimpl move retains ownership and live handles")
{
    PhysicsWorld source(WorldConfig{});
    BodyDesc body = BodyDesc::dynamic_sphere(0.5f, {}, 10.0f);
    body.world_exit_policy = WorldExitPolicy::KeepOutsideBounds;
    const BodyHandle handle = source.create_body(body).value;

    PhysicsWorld destination(std::move(source));
    destination.step();
    NINHO_REQUIRE(destination.state(handle).has_value());
}

NINHO_TEST("contract load validation rejects non finite force and impulse")
{
    PhysicsWorld physics(WorldConfig{});
    BodyDesc body = BodyDesc::dynamic_sphere(0.5f, {}, 10.0f);
    const BodyHandle handle = physics.create_body(body).value;
    const float infinity = std::numeric_limits<float>::infinity();
    NINHO_REQUIRE(
        physics.apply_force(handle, {infinity, 0, 0}, {}).code == StatusCode::InvalidArgument);
    NINHO_REQUIRE(
        physics.apply_impulse(handle, {}, {0, infinity, 0}).code == StatusCode::InvalidArgument);
}

NINHO_TEST("contract invalid hull and empty compound geometry is explicit")
{
    PhysicsWorld physics(WorldConfig{});

    BodyDesc hull;
    hull.type = BodyType::Dynamic;
    hull.shapes.push_back(ShapeDesc{.geometry = HullShape{{{0, 0, 0}, {1, 0, 0}, {0, 1, 0}}, {}}});
    NINHO_REQUIRE(physics.create_body(hull).status.code == StatusCode::InvalidArgument);

    BodyDesc compound;
    compound.type = BodyType::Dynamic;
    compound.shapes.push_back(ShapeDesc{.geometry = CompoundShape{}});
    NINHO_REQUIRE(physics.create_body(compound).status.code == StatusCode::InvalidArgument);
}

NINHO_TEST("contract queue commands preserve order and cancel pre tick creation")
{
    PhysicsWorld physics(WorldConfig{});
    BodyDesc body = BodyDesc::dynamic_sphere(0.5f, {}, 10.0f);
    body.world_exit_policy = WorldExitPolicy::KeepOutsideBounds;

    const BodyHandle accelerated = physics.create_body(body).value;
    NINHO_REQUIRE(physics.apply_impulse(accelerated, {10, 0, 0}, {}).ok());
    physics.step();
    NINHO_REQUIRE(physics.state(accelerated)->linear_velocity.x > 0.0f);

    const BodyHandle cancelled = physics.create_body(body).value;
    NINHO_REQUIRE(physics.destroy_body(cancelled).ok());
    physics.step();
    NINHO_REQUIRE(!physics.state(cancelled).has_value());
    const BodyHandle reused = physics.create_body(body).value;
    NINHO_REQUIRE(cancelled.index == reused.index);
    NINHO_REQUIRE(cancelled.generation != reused.generation);
}

NINHO_TEST("contract capacity counts reserved handles and permits reuse after destruction")
{
    PhysicsWorld physics(WorldConfig{.max_bodies = 1});
    BodyDesc body = BodyDesc::dynamic_sphere(0.5f, {}, 10.0f);
    body.world_exit_policy = WorldExitPolicy::KeepOutsideBounds;

    const auto rejected = physics.create_body(BodyDesc::dynamic_sphere(0.0f, {}, 10.0f));
    NINHO_REQUIRE(rejected.status.code == StatusCode::InvalidArgument);
    const auto first = physics.create_body(body);
    NINHO_REQUIRE(first.status.ok());
    NINHO_REQUIRE(physics.create_body(body).status.code == StatusCode::CapacityExceeded);
    NINHO_REQUIRE(physics.destroy_body(first.value).ok());
    NINHO_REQUIRE(physics.create_body(body).status.code == StatusCode::CapacityExceeded);
    physics.step();
    const auto replacement = physics.create_body(body);
    NINHO_REQUIRE(replacement.status.ok());
    NINHO_REQUIRE(replacement.value.index == first.value.index);
    NINHO_REQUIRE(replacement.value.generation != first.value.generation);
}

NINHO_TEST("contract capsule primitive with local transform contributes dynamic mass")
{
    PhysicsWorld physics(WorldConfig{});
    BodyDesc capsule;
    capsule.type = BodyType::Dynamic;
    capsule.world_exit_policy = WorldExitPolicy::KeepOutsideBounds;
    capsule.shapes.push_back(ShapeDesc{
        .geometry = CapsuleShape{.half_height = 1.0f,
                                 .radius = 0.25f,
                                 .local = {{0.5f, 0.0f, 0.0f}, {}}},
        .density = 20.0f,
    });
    const auto created = physics.create_body(capsule);
    NINHO_REQUIRE(created.status.ok());
    physics.step();
    NINHO_REQUIRE(physics.state(created.value)->mass > 0.0f);
}

NINHO_TEST("contract outward motion beyond four radii becomes sticky ejection after half second")
{
    PhysicsWorld physics(make_legacy_radial_world_config(
        {.planet_radius = 10.0f, .surface_gravity = 0.0f}));
    BodyDesc body = BodyDesc::dynamic_sphere(0.5f, {{0, 41, 0}, {}}, 10.0f);
    body.linear_velocity = {0, 3, 0};
    body.world_exit_policy = WorldExitPolicy::KeepOutsideBounds;
    const BodyHandle handle = physics.create_body(body).value;

    for (int tick = 0; tick < 29; ++tick) {
        physics.step();
        NINHO_REQUIRE(physics.state(handle).has_value());
        NINHO_REQUIRE(!physics.state(handle)->ejected);
    }
    physics.step();
    NINHO_REQUIRE(physics.state(handle)->ejected);
}

NINHO_TEST("contract slow outward tick resets ejection duration window")
{
    PhysicsWorld physics(make_legacy_radial_world_config(
        {.planet_radius = 10.0f, .surface_gravity = 0.0f}));
    BodyDesc body = BodyDesc::dynamic_sphere(0.5f, {{0, 41, 0}, {}}, 10.0f);
    body.linear_velocity = {0, 3, 0};
    body.world_exit_policy = WorldExitPolicy::KeepOutsideBounds;
    const BodyHandle handle = physics.create_body(body).value;

    for (int tick = 0; tick < 29; ++tick) {
        physics.step();
        NINHO_REQUIRE(!physics.state(handle)->ejected);
    }

    const BodyState fast_state = *physics.state(handle);
    NINHO_REQUIRE(physics.apply_impulse(
        handle,
        {0, -2.0f * fast_state.mass, 0},
        fast_state.transform.position).ok());
    physics.step();
    NINHO_REQUIRE(!physics.state(handle)->ejected);

    const BodyState slow_state = *physics.state(handle);
    NINHO_REQUIRE(physics.apply_impulse(
        handle,
        {0, 2.0f * slow_state.mass, 0},
        slow_state.transform.position).ok());
    for (int tick = 0; tick < 29; ++tick) {
        physics.step();
        NINHO_REQUIRE(!physics.state(handle)->ejected);
    }
    physics.step();
    NINHO_REQUIRE(physics.state(handle)->ejected);
}

NINHO_TEST("contract spherical six radius world exit does not fabricate ejection")
{
    const WorldBounds bounds{SphericalWorldBounds{.center_m = {}, .removal_radius_m = 60.0f}};
    NINHO_REQUIRE(!bounds.contains({0.0f, 60.0f, 0.0f}));

    PhysicsWorld physics(make_legacy_radial_world_config(
        {.planet_radius = 10.0f, .surface_gravity = 0.0f}));
    BodyDesc body = BodyDesc::dynamic_sphere(0.5f, {{0, 61, 0}, {}}, 10.0f);
    const BodyHandle handle = physics.create_body(body).value;

    physics.step();
    NINHO_REQUIRE(physics.state(handle).has_value());
    NINHO_REQUIRE(!physics.state(handle)->ejected);

    physics.step();
    NINHO_REQUIRE(!physics.state(handle).has_value());
    NINHO_REQUIRE(physics.states().empty());
}

// Box3D supports simulating separate worlds on separate threads, but only if
// the application serializes world creation and destruction; see
// docs/foundation.md, "Multithreading Multiple Worlds", and the check-then-act
// on the global b3_worlds table in b3CreateWorld. These two tests pin the guard
// that makes parallel replays legal.
NINHO_TEST("contract concurrent worlds never overlap inside the Box3D lifecycle")
{
    constexpr std::size_t thread_count = 8U;
    constexpr std::size_t rounds = 24U;

    detail::reset_box3d_world_lifecycle_peak_concurrency();

    // NINHO_REQUIRE throws, and an exception escaping a std::thread terminates
    // the process, so workers only record and the assertions run after join.
    std::atomic<std::size_t> single_worker_worlds{0U};
    std::atomic<std::size_t> ready{0U};
    std::atomic<bool> start{false};
    std::vector<std::thread> workers;
    workers.reserve(thread_count);
    for (std::size_t worker = 0U; worker < thread_count; ++worker) {
        workers.emplace_back([&] {
            ready.fetch_add(1U);
            while (!start.load()) {
            }
            for (std::size_t round = 0U; round < rounds; ++round) {
                const PhysicsWorld world(WorldConfig{});
                if (detail::PhysicsWorldTestFacade::worker_count(world) == 1) {
                    single_worker_worlds.fetch_add(1U);
                }
            }
        });
    }
    while (ready.load() < thread_count) {
    }
    start.store(true);
    for (std::thread& worker : workers) {
        worker.join();
    }

    NINHO_REQUIRE(single_worker_worlds.load() == thread_count * rounds);
    NINHO_REQUIRE(detail::box3d_world_lifecycle_peak_concurrency() == 1U);
}

NINHO_TEST("contract concurrent worlds keep distinct simultaneous Box3D slots")
{
    constexpr std::size_t thread_count = 8U;

    std::atomic<std::size_t> constructed{0U};
    std::mutex collected_mutex;
    std::vector<std::uint64_t> collected;
    std::vector<std::thread> workers;
    workers.reserve(thread_count);
    for (std::size_t worker = 0U; worker < thread_count; ++worker) {
        workers.emplace_back([&] {
            const PhysicsWorld world(WorldConfig{});
            const std::uint64_t identity =
                detail::PhysicsWorldTestFacade::box3d_world_identity(world);
            {
                const std::lock_guard<std::mutex> guard{collected_mutex};
                collected.push_back(identity);
            }
            // Hold every world alive until all of them exist, so the slots are
            // genuinely simultaneous rather than recycled one after another.
            constructed.fetch_add(1U);
            while (constructed.load() < thread_count) {
            }
        });
    }
    for (std::thread& worker : workers) {
        worker.join();
    }

    NINHO_REQUIRE(collected.size() == thread_count);
    std::sort(collected.begin(), collected.end());
    NINHO_REQUIRE(
        std::adjacent_find(collected.begin(), collected.end()) == collected.end());
}
