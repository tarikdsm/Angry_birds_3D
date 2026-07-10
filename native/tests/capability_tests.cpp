#include "test_framework.hpp"

#include <ninho/physics/physics_world.hpp>

#include <algorithm>
#include <cmath>
#include <limits>
#include <ranges>
#include <set>
#include <type_traits>

using namespace ninho::physics;

static_assert(!JointHandle{}.valid());
static_assert(!JointHandle{1, 0}.valid());
static_assert(!JointHandle{0, 1}.valid());
static_assert(JointHandle{1, 1}.valid());
static_assert(std::is_same_v<QueryShape, PrimitiveShape>);

namespace {

[[nodiscard]] bool finite_query_hit(const QueryHit& hit)
{
    return hit.body.valid() && is_finite(hit.point) && is_finite(hit.normal)
        && std::isfinite(hit.fraction) && hit.fraction >= 0.0f && hit.fraction <= 1.0f;
}

[[nodiscard]] bool finite_reaction(const JointReaction& reaction)
{
    return reaction.joint.valid() && is_finite(reaction.force) && is_finite(reaction.torque)
        && std::isfinite(reaction.linear_separation)
        && std::isfinite(reaction.angular_separation);
}

}

NINHO_TEST("capability compound body has expected mass and can be queried")
{
    PhysicsWorld world(WorldConfig{.surface_gravity = 0});
    BodyDesc body{.type = BodyType::Dynamic, .transform = {{0, 5, 0}, {}}};
    body.shapes = {
        ShapeDesc{.geometry = BoxShape{{0.5f, 0.5f, 0.5f}, {{-0.75f, 0, 0}, {}}},
                  .density = 500,
                  .material_id = 11},
        ShapeDesc{.geometry = BoxShape{{0.5f, 0.5f, 0.5f}, {{0.75f, 0, 0}, {}}},
                  .density = 500,
                  .material_id = 22},
    };
    const auto handle = world.create_body(body).value;
    world.step();
    const auto hits = world.overlap_sphere({0, 5, 0}, 2.0f);
    NINHO_REQUIRE(hits.size() == 1);
    NINHO_REQUIRE(hits.front().body == handle);
    NINHO_REQUIRE(finite_query_hit(hits.front()));
    NINHO_REQUIRE(hits.front().material_id == 11 || hits.front().material_id == 22);
    NINHO_REQUIRE(world.state(handle)->mass > 0.0f);
}

NINHO_TEST("capability sphere cast returns first hit")
{
    PhysicsWorld world(WorldConfig{.surface_gravity = 0});
    BodyDesc high_desc = BodyDesc::static_box({1, 1, 1}, {{0, 8, 0}, {}});
    high_desc.shapes.front().material_id = 81;
    const auto high = world.create_body(high_desc).value;
    BodyDesc low_desc = BodyDesc::static_box({1, 1, 1}, {{0, 4, 0}, {}});
    low_desc.shapes.front().material_id = 41;
    world.create_body(low_desc);
    world.step();
    const auto hit = world.cast_sphere({0, 12, 0}, 0.25f, {0, -12, 0});
    NINHO_REQUIRE(hit.has_value());
    NINHO_REQUIRE(hit->body == high);
    NINHO_REQUIRE(finite_query_hit(*hit));
    NINHO_REQUIRE(length(hit->normal) > 0.9f);
    NINHO_REQUIRE(hit->material_id == 81);
}

NINHO_TEST("capability shape cast resolves quantized ties by public handle")
{
    PhysicsWorld world(WorldConfig{.surface_gravity = 0});
    BodyDesc first_desc = BodyDesc::static_box({1, 1, 1}, {{0, 4, 0}, {}});
    first_desc.shapes.front().material_id = 10;
    const BodyHandle first = world.create_body(first_desc).value;
    BodyDesc second_desc = BodyDesc::static_box({1, 1, 1}, {{0, 4, 0}, {}});
    second_desc.shapes.front().material_id = 20;
    const BodyHandle second = world.create_body(second_desc).value;
    world.step();
    NINHO_REQUIRE(first < second);
    const auto hit = world.cast_sphere({0, 8, 0}, 0.25f, {0, -8, 0});
    NINHO_REQUIRE(hit.has_value());
    NINHO_REQUIRE(hit->body == first);
    NINHO_REQUIRE(hit->material_id == 10);
}

NINHO_TEST("capability distance joint exposes finite force and destroys safely")
{
    PhysicsWorld world(WorldConfig{.surface_gravity = 0});
    auto a = world.create_body(
        BodyDesc::dynamic_box({0.5f, 0.5f, 0.5f}, {{-1, 5, 0}, {}}, 500)).value;
    auto b = world.create_body(
        BodyDesc::dynamic_box({0.5f, 0.5f, 0.5f}, {{1, 5, 0}, {}}, 500)).value;
    world.step();
    auto joint = world.create_joint(DistanceJointDesc{.a = a, .b = b, .length = 2}).value;
    world.apply_impulse(b, {100, 0, 0}, {1, 5, 0});
    world.step();
    NINHO_REQUIRE(!world.joint_reactions().empty());
    NINHO_REQUIRE(world.joint_reactions().front().joint == joint);
    NINHO_REQUIRE(finite_reaction(world.joint_reactions().front()));
    NINHO_REQUIRE(length(world.joint_reactions().front().force) > 0.0f);
    NINHO_REQUIRE(is_finite(world.joint_reactions().front().torque));
    NINHO_REQUIRE(world.destroy_joint(joint).ok());
    NINHO_REQUIRE(world.destroy_joint(joint).code == StatusCode::InvalidHandle);
}

NINHO_TEST("capability weld joint keeps loaded bodies together")
{
    PhysicsWorld world(WorldConfig{.surface_gravity = 0});
    auto a = world.create_body(
        BodyDesc::dynamic_box({0.5f, 0.5f, 0.5f}, {{0, 5, 0}, {}}, 500)).value;
    auto b = world.create_body(
        BodyDesc::dynamic_box({0.5f, 0.5f, 0.5f}, {{0, 6, 0}, {}}, 500)).value;
    world.step();
    auto joint = world.create_joint(WeldJointDesc{.a = a, .b = b}).value;
    for (int i = 0; i < 120; ++i) {
        world.apply_force(b, {500, 0, 0}, {0, 6, 0});
        world.step();
    }
    auto reaction = world.joint_reaction(joint);
    NINHO_REQUIRE(reaction.has_value());
    NINHO_REQUIRE(finite_reaction(*reaction));
    NINHO_REQUIRE(length(reaction->force) > 0.0f);
    NINHO_REQUIRE(is_finite(reaction->torque));
    NINHO_REQUIRE(std::abs(reaction->linear_separation) < 0.02f);
}

NINHO_TEST("capability hit events are copied before mutation")
{
    PhysicsWorld world(WorldConfig{.surface_gravity = 0});
    BodyDesc wall_desc = BodyDesc::static_box({2, 0.5f, 2}, {{0, 5, 0}, {}});
    wall_desc.shapes.front().material_id = 101;
    auto wall = world.create_body(wall_desc).value;
    auto ball_desc = BodyDesc::dynamic_sphere(0.4f, {{0, 10, 0}, {}}, 520);
    ball_desc.linear_velocity = {0, -20, 0};
    ball_desc.bullet = true;
    ball_desc.shapes.front().material_id = 202;
    const BodyHandle ball = world.create_body(ball_desc).value;
    for (int i = 0; i < 30 && world.contact_hits().empty(); ++i) {
        world.step();
    }
    NINHO_REQUIRE(!world.contact_hits().empty());
    const ContactHit retained = world.contact_hits().front();
    NINHO_REQUIRE(retained.a == std::min(wall, ball));
    NINHO_REQUIRE(retained.b == std::max(wall, ball));
    NINHO_REQUIRE(is_finite(retained.normal));
    NINHO_REQUIRE(length(retained.normal) > 0.9f);
    NINHO_REQUIRE((
        std::set<std::uint64_t>{retained.material_a, retained.material_b}
        == std::set<std::uint64_t>{101, 202}));
    world.destroy_body(wall);
    world.step();
    NINHO_REQUIRE(retained.approach_speed > 0.0f);
    NINHO_REQUIRE(retained.effective_mass > 0.0f);
    NINHO_REQUIRE(retained.derived_energy > 0.0f);
    NINHO_REQUIRE(is_finite(retained.point));
}

NINHO_TEST("capability invalid hull is rejected before Box3D")
{
    PhysicsWorld world(WorldConfig{});
    BodyDesc body{.type = BodyType::Dynamic};
    body.shapes = {ShapeDesc{
        .geometry = HullShape{{{0, 0, 0}, {1, 0, 0}, {0, 1, 0}, {0, 0, 0}}, {}},
        .density = 500}};
    const auto result = world.create_body(body);
    NINHO_REQUIRE(result.status.code == StatusCode::InvalidArgument);
}

NINHO_TEST("capability three meter shape cast hits transformed hull")
{
    PhysicsWorld world(WorldConfig{.surface_gravity = 0});
    BodyDesc target{.type = BodyType::Static, .transform = {{0, 0, 0}, {}}};
    target.shapes = {ShapeDesc{
        .geometry = HullShape{{{-0.5f, -0.5f, -0.5f},
                               {0.5f, -0.5f, -0.5f},
                               {0, 0.5f, -0.5f},
                               {0, 0, 0.5f}},
                              {{0.2f, 0, 0}, {}}},
        .density = 1,
        .material_id = 303}};
    const auto handle = world.create_body(target).value;
    world.step();
    const SphereShape query{.radius = 0.1f};
    const Vec3 origin{0, 2.5f, 0};
    const Vec3 translation{0, -3, 0};
    auto hit = world.cast_shape(query, {origin, {}}, translation);
    NINHO_REQUIRE(hit.has_value());
    NINHO_REQUIRE(hit->body == handle);
    NINHO_REQUIRE(finite_query_hit(*hit));
    NINHO_REQUIRE(hit->material_id == 303);
    const Vec3 inside = origin + translation * std::min(1.0f, hit->fraction + 0.05f);
    const auto overlaps = world.overlap_shape(query, {inside, {}});
    NINHO_REQUIRE(std::ranges::any_of(
        overlaps, [&](const QueryHit& value) { return value.body == handle; }));
}

NINHO_TEST("capability eight hull compound reports mass bounds and contact")
{
    PhysicsWorld world(WorldConfig{.surface_gravity = 0});
    BodyDesc compound{.type = BodyType::Dynamic, .transform = {{0, 3, 0}, {}}};
    for (int i = 0; i < 8; ++i) {
        compound.shapes.push_back(ShapeDesc{
            .geometry = BoxShape{{0.25f, 0.25f, 0.25f}, {{(i - 3.5f) * 0.5f, 0, 0}, {}}},
            .density = 500,
        });
    }
    const auto body = world.create_body(compound).value;
    world.create_body(BodyDesc::static_box({4, 0.25f, 4}, {{0, 0, 0}, {}}));
    world.step();
    auto bounds = world.body_bounds(body);
    NINHO_REQUIRE(bounds.has_value());
    NINHO_REQUIRE(bounds->upper.x - bounds->lower.x > 3.5f);
    NINHO_REQUIRE(world.state(body)->mass > 0.0f);
    for (int i = 0; i < 180 && world.contact_hits().empty(); ++i) {
        world.apply_force(body, {0, -4000, 0}, {0, 3, 0});
        world.step();
    }
    NINHO_REQUIRE(!world.contact_hits().empty());
    NINHO_REQUIRE(world.contact_hits().front().a.valid());
    NINHO_REQUIRE(world.contact_hits().front().b.valid());
}

NINHO_TEST("capability hit events deduplicate substeps and joint load crosses threshold")
{
    PhysicsWorld world(WorldConfig{.substeps = 6, .surface_gravity = 0});
    auto a = world.create_body(
        BodyDesc::static_box({0.5f, 0.5f, 0.5f}, {{0, 0, 0}, {}})).value;
    auto b = world.create_body(
        BodyDesc::dynamic_box({0.5f, 0.5f, 0.5f}, {{0, 1, 0}, {}}, 500)).value;
    world.step();
    auto joint = world.create_joint(DistanceJointDesc{.a = a, .b = b, .length = 1}).value;
    world.create_body(BodyDesc::static_box({0.25f, 2, 2}, {{5, 5, 0}, {}}));
    auto projectile = BodyDesc::dynamic_sphere(0.25f, {{0, 5, 0}, {}}, 520);
    projectile.linear_velocity = {20, 0, 0};
    projectile.bullet = true;
    world.create_body(projectile);
    for (int i = 0; i < 30 && world.contact_hits().empty(); ++i) {
        world.step();
    }
    NINHO_REQUIRE(!world.contact_hits().empty());
    std::set<std::pair<BodyHandle, BodyHandle>> hit_pairs;
    for (const auto& hit : world.contact_hits()) {
        NINHO_REQUIRE(hit.a.valid() && hit.b.valid());
        NINHO_REQUIRE(hit.a < hit.b);
        NINHO_REQUIRE(hit_pairs.insert({hit.a, hit.b}).second);
        NINHO_REQUIRE(is_finite(hit.point) && is_finite(hit.normal));
        NINHO_REQUIRE(hit.approach_speed > 0.0f);
        NINHO_REQUIRE(hit.effective_mass > 0.0f);
        NINHO_REQUIRE(hit.derived_energy > 0.0f);
    }
    float previous = 0;
    for (int load = 1000; load <= 12000; load += 1000) {
        world.apply_force(b, {float(load), 0, 0}, {0, 1, 0});
        world.step();
        const auto reaction = world.joint_reaction(joint);
        NINHO_REQUIRE(reaction.has_value());
        NINHO_REQUIRE(finite_reaction(*reaction));
        const float current = length(reaction->force);
        NINHO_REQUIRE(current + 50.0f >= previous);
        previous = current;
    }
    NINHO_REQUIRE(previous > 10000.0f);
}

NINHO_TEST("capability CompoundShape expands children and query ordering is stable")
{
    PhysicsWorld world(WorldConfig{.surface_gravity = 0});
    BodyDesc compound{.type = BodyType::Dynamic, .transform = {{0, 4, 0}, {}}};
    compound.shapes.push_back(ShapeDesc{
        .geometry = CompoundShape{{
            BoxShape{{0.4f, 0.4f, 0.4f}, {{-0.6f, 0, 0}, {}}},
            BoxShape{{0.4f, 0.4f, 0.4f}, {{0.6f, 0, 0}, {}}},
        }},
        .density = 250,
        .material_id = 707,
    });
    const BodyHandle compound_handle = world.create_body(compound).value;
    BodyDesc neighbor = BodyDesc::static_sphere(0.3f, {{0, 5.5f, 0}, {}});
    neighbor.shapes.front().material_id = 808;
    const BodyHandle neighbor_handle = world.create_body(neighbor).value;
    world.step();

    const auto hits = world.overlap_sphere({0, 4.75f, 0}, 2.0f);
    NINHO_REQUIRE(hits.size() == 2);
    NINHO_REQUIRE(hits[0].body == std::min(compound_handle, neighbor_handle));
    NINHO_REQUIRE(hits[1].body == std::max(compound_handle, neighbor_handle));
    NINHO_REQUIRE(hits[0].body != hits[1].body);
    NINHO_REQUIRE(std::ranges::any_of(hits, [](const QueryHit& hit) {
        return hit.body.valid() && hit.material_id == 707;
    }));
    NINHO_REQUIRE(world.state(compound_handle)->mass > 0.0f);
}

NINHO_TEST("capability invalid joint and query inputs are rejected")
{
    PhysicsWorld world(WorldConfig{.surface_gravity = 0});
    auto a = world.create_body(BodyDesc::dynamic_sphere(0.5f, {}, 10)).value;
    auto b = world.create_body(BodyDesc::dynamic_sphere(0.5f, {{2, 0, 0}, {}}, 10)).value;
    world.step();
    DistanceJointDesc invalid_joint{.a = a, .b = b, .length = 2};
    invalid_joint.hertz = std::numeric_limits<float>::quiet_NaN();
    NINHO_REQUIRE(world.create_joint(invalid_joint).status.code == StatusCode::InvalidArgument);
    NINHO_REQUIRE(
        world.create_joint(DistanceJointDesc{.a = {}, .b = b, .length = 2}).status.code
        == StatusCode::InvalidHandle);
    NINHO_REQUIRE(world.destroy_joint({}).code == StatusCode::InvalidHandle);

    const SphereShape invalid_sphere{.radius = -1.0f};
    NINHO_REQUIRE(world.overlap_shape(invalid_sphere, {}).empty());
    NINHO_REQUIRE(!world.cast_shape(invalid_sphere, {}, {1, 0, 0}).has_value());
    const SphereShape sphere{.radius = 0.5f};
    NINHO_REQUIRE(!world.cast_shape(
                             sphere,
                             {},
                             {std::numeric_limits<float>::infinity(), 0, 0})
                       .has_value());
}

NINHO_TEST("capability body destruction invalidates attached joint handle")
{
    PhysicsWorld world(WorldConfig{.surface_gravity = 0});
    const auto a = world.create_body(BodyDesc::dynamic_sphere(0.5f, {}, 10)).value;
    const auto b = world.create_body(BodyDesc::dynamic_sphere(0.5f, {{2, 0, 0}, {}}, 10)).value;
    world.step();
    const JointHandle joint =
        world.create_joint(DistanceJointDesc{.a = a, .b = b, .length = 2}).value;
    world.step();
    NINHO_REQUIRE(world.joint_reaction(joint).has_value());
    NINHO_REQUIRE(world.destroy_body(a).ok());
    world.step();
    NINHO_REQUIRE(!world.joint_reaction(joint).has_value());
    NINHO_REQUIRE(world.destroy_joint(joint).code == StatusCode::InvalidHandle);
    NINHO_REQUIRE(world.metrics().joint_count == 0);
}

NINHO_TEST("capability joint capacity is explicit and generation safe")
{
    PhysicsWorld world(WorldConfig{.surface_gravity = 0});
    const auto a = world.create_body(BodyDesc::static_sphere(0.5f, {})).value;
    const auto b = world.create_body(BodyDesc::dynamic_sphere(0.5f, {{2, 0, 0}, {}}, 10)).value;
    world.step();
    JointHandle first{};
    for (int index = 0; index < 250; ++index) {
        const auto created = world.create_joint(DistanceJointDesc{.a = a, .b = b, .length = 2});
        NINHO_REQUIRE(created.status.ok());
        if (index == 0) {
            first = created.value;
        }
    }
    NINHO_REQUIRE(
        world.create_joint(DistanceJointDesc{.a = a, .b = b, .length = 2}).status.code
        == StatusCode::CapacityExceeded);
    world.step();
    NINHO_REQUIRE(world.destroy_joint(first).ok());
    world.step();
    const auto replacement =
        world.create_joint(DistanceJointDesc{.a = a, .b = b, .length = 2});
    NINHO_REQUIRE(replacement.status.ok());
    NINHO_REQUIRE(replacement.value.index == first.index);
    NINHO_REQUIRE(replacement.value.generation != first.generation);
}

NINHO_TEST("capability bounds metrics and stale handles remain public only")
{
    PhysicsWorld world(WorldConfig{.surface_gravity = 0});
    const BodyHandle body =
        world.create_body(BodyDesc::dynamic_box({1, 2, 3}, {{0, 5, 0}, {}}, 10)).value;
    NINHO_REQUIRE(world.metrics().body_count == 0);
    world.step();
    const auto bounds = world.body_bounds(body);
    NINHO_REQUIRE(bounds.has_value());
    NINHO_REQUIRE(is_finite(bounds->lower) && is_finite(bounds->upper));
    NINHO_REQUIRE(bounds->lower.x < bounds->upper.x);
    const WorldMetrics metrics = world.metrics();
    NINHO_REQUIRE(metrics.body_count == 1);
    NINHO_REQUIRE(metrics.shape_count == 1);
    NINHO_REQUIRE(metrics.joint_count == 0);
    NINHO_REQUIRE(metrics.contact_count >= 0);
    NINHO_REQUIRE(metrics.awake_count == 1);
    NINHO_REQUIRE(std::isfinite(metrics.step_ms) && metrics.step_ms >= 0.0);
    NINHO_REQUIRE(world.destroy_body(body).ok());
    NINHO_REQUIRE(!world.body_bounds(body).has_value());
    world.step();
    NINHO_REQUIRE(world.metrics().body_count == 0);
}

NINHO_TEST("capability metrics exclude contacts attached to pending body destruction")
{
    PhysicsWorld world(WorldConfig{.surface_gravity = 0});
    world.create_body(BodyDesc::static_box({2, 0.5f, 2}, {{0, 0, 0}, {}}));
    const BodyHandle body =
        world.create_body(BodyDesc::dynamic_box({0.5f, 0.5f, 0.5f}, {{0, 1, 0}, {}}, 10)).value;
    world.step();
    NINHO_REQUIRE(world.metrics().contact_count > 0);
    NINHO_REQUIRE(world.destroy_body(body).ok());
    NINHO_REQUIRE(world.metrics().contact_count == 0);
}
