#include "test_framework.hpp"
#include "physics_world_test_facade.hpp"

#include <ninho/physics/physics_world.hpp>

#include <limits>
#include <stdexcept>
#include <type_traits>
#include <utility>

using namespace ninho::physics;

static_assert(!std::is_copy_constructible_v<PhysicsWorld>);
static_assert(!std::is_copy_assignable_v<PhysicsWorld>);
static_assert(std::is_nothrow_move_constructible_v<PhysicsWorld>);
static_assert(std::is_nothrow_move_assignable_v<PhysicsWorld>);

NINHO_TEST("contract Box3D world remains single worker")
{
    const PhysicsWorld world(WorldConfig{});
    NINHO_REQUIRE(detail::PhysicsWorldTestFacade::worker_count(world) == 1);
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
    WorldConfig config{.substeps = 4, .planet_radius = 10, .surface_gravity = 9};
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
    config.planet_radius = 0.0f;
    NINHO_REQUIRE(rejected(config));
    config = {};
    config.surface_gravity = -1.0f;
    NINHO_REQUIRE(rejected(config));
    config = {};
    config.max_bodies = 0;
    NINHO_REQUIRE(rejected(config));
    config = {};
    config.time_step = std::numeric_limits<float>::quiet_NaN();
    NINHO_REQUIRE(rejected(config));
    config = {};
    config.planet_radius = std::numeric_limits<float>::max();
    NINHO_REQUIRE(rejected(config));
}

NINHO_TEST("contract configuration rejects surface gravity above the acceleration ceiling")
{
    const auto rejected = [](float surface_gravity) {
        try {
            PhysicsWorld physics(WorldConfig{.surface_gravity = surface_gravity});
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
    body.radial_gravity = false;
    body.remove_beyond_six_r = false;
    const BodyHandle handle = source.create_body(body).value;

    PhysicsWorld destination(std::move(source));
    destination.step();
    NINHO_REQUIRE(destination.state(handle).has_value());
}

NINHO_TEST("contract load validation rejects non finite force and impulse")
{
    PhysicsWorld physics(WorldConfig{});
    BodyDesc body = BodyDesc::dynamic_sphere(0.5f, {}, 10.0f);
    body.radial_gravity = false;
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
    body.radial_gravity = false;
    body.remove_beyond_six_r = false;

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
    body.radial_gravity = false;
    body.remove_beyond_six_r = false;

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
    capsule.radial_gravity = false;
    capsule.remove_beyond_six_r = false;
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
    PhysicsWorld physics(WorldConfig{.planet_radius = 10.0f, .surface_gravity = 0.0f});
    BodyDesc body = BodyDesc::dynamic_sphere(0.5f, {{0, 41, 0}, {}}, 10.0f);
    body.linear_velocity = {0, 3, 0};
    body.remove_beyond_six_r = false;
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
    PhysicsWorld physics(WorldConfig{.planet_radius = 10.0f, .surface_gravity = 0.0f});
    BodyDesc body = BodyDesc::dynamic_sphere(0.5f, {{0, 41, 0}, {}}, 10.0f);
    body.linear_velocity = {0, 3, 0};
    body.remove_beyond_six_r = false;
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

NINHO_TEST("contract six radius removal does not fabricate ejection")
{
    PhysicsWorld physics(WorldConfig{.planet_radius = 10.0f, .surface_gravity = 0.0f});
    BodyDesc body = BodyDesc::dynamic_sphere(0.5f, {{0, 61, 0}, {}}, 10.0f);
    const BodyHandle handle = physics.create_body(body).value;

    physics.step();
    NINHO_REQUIRE(physics.state(handle).has_value());
    NINHO_REQUIRE(!physics.state(handle)->ejected);

    physics.step();
    NINHO_REQUIRE(!physics.state(handle).has_value());
    NINHO_REQUIRE(physics.states().empty());
}
