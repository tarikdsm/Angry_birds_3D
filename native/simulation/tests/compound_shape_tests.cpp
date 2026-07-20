#include "test_framework.hpp"

#include "box3d_allocator_probe.hpp"
#include "session_test_facade.hpp"
#include "ninho/simulation/session.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <ranges>
#include <string>
#include <vector>

namespace {

using namespace ninho::simulation;

MaterialCatalog materials()
{
    MaterialCatalog result;
    result.schema_version = result.source_schema_version = 2U;
    result.surfaces.push_back({SurfaceId{1002}, "pig", 480.0, 0.4, 0.1});
    return result;
}

ArchetypeCatalog archetypes()
{
    ArchetypeCatalog result;
    result.schema_version = result.source_schema_version = 2U;
    return result;
}

ShapeDefinition box(double x, double y, double z)
{
    ShapeDefinition result;
    result.type = ShapeType::Box;
    result.half_extents_m = {x, y, z};
    return result;
}

ShapeDefinition sphere(double radius)
{
    ShapeDefinition result;
    result.type = ShapeType::Sphere;
    result.radius_m = radius;
    return result;
}

ShapeDefinition capsule(double radius, double half_height)
{
    ShapeDefinition result;
    result.type = ShapeType::Capsule;
    result.radius_m = radius;
    result.half_height_m = half_height;
    return result;
}

ShapeDefinition hull(std::vector<std::array<double, 3>> vertices)
{
    ShapeDefinition result;
    result.type = ShapeType::ConvexHull;
    result.vertices_m = std::move(vertices);
    return result;
}

ShapeDefinition compound(std::vector<ShapeDefinition> children)
{
    ShapeDefinition result;
    result.type = ShapeType::Compound;
    result.children = std::move(children);
    return result;
}

BodyDefinition dynamic_body(std::uint32_t id, ShapeDefinition shape,
    std::array<double, 3> position, std::string visual_id)
{
    BodyDefinition result;
    result.body_id = id;
    result.entity_id = EntityId{id};
    result.part_id = PartId{1};
    result.body_type = BodyType::Dynamic;
    result.surface_id = SurfaceId{1002};
    result.density_kg_m3 = 480.0;
    result.transform.position_m = position;
    result.transform.rotation_xyzw = {0.0, 0.0, 0.0, 1.0};
    result.shape = std::move(shape);
    result.visual.asset_id = std::move(visual_id);
    result.visual.bounds_m = {2.0, 2.0, 2.0};
    result.affected_by_world_gravity = true;
    return result;
}

LevelManifest level_with(std::vector<BodyDefinition> bodies)
{
    LevelManifest result;
    result.schema_version = result.source_schema_version = 2U;
    result.id = "shape_lab";
    result.world_id = "earth";
    result.world = UniformWorldDefinition{
        .acceleration_m_s2 = {0.0, -9.81, 0.0},
        .bounds_min_m = {-100.0, -100.0, -100.0},
        .bounds_max_m = {100.0, 100.0, 100.0},
    };
    result.bodies = std::move(bodies);
    return result;
}

ContentResult<std::unique_ptr<SimulationSession>> create(LevelManifest level)
{
    if (level.free_body_ids.empty() && level.assemblies.empty()) {
        for (const BodyDefinition& definition : level.bodies) {
            level.free_body_ids.push_back(definition.body_id);
        }
    }
    return SimulationSession::create(materials(), archetypes(), level);
}

const EntitySnapshot& snapshot(const SimulationSession& session, EntityId id)
{
    const auto found = std::ranges::find(session.snapshots(), id,
        &EntitySnapshot::entity_id);
    NINHO_SIM_REQUIRE(found != session.snapshots().end());
    return *found;
}

}

NINHO_SIM_TEST("compound shape capsule hull and compound preserve shape mass and visual id")
{
    const auto tetrahedron = hull({
        {-0.5, -0.5, -0.5},
        {0.5, -0.5, -0.5},
        {-0.5, 0.5, -0.5},
        {-0.5, -0.5, 0.5},
    });
    constexpr double pig_half_depth = 65.0 / 240.0;
    auto pig_left = box(0.25, 0.125, pig_half_depth);
    pig_left.local_position_m = {-0.4, 0.0, 0.0};
    auto pig_right = box(0.25, 0.125, pig_half_depth);
    pig_right.local_position_m = {0.6, 0.2, 0.0};
    pig_right.local_rotation_xyzw = {
        0.0, 0.0, 0.7071067811865476, 0.7071067811865476};
    const auto pig = compound({pig_left, pig_right});
    auto created = create(level_with({
        dynamic_body(1, capsule(0.25, 0.5), {-4.0, 4.0, 0.0}, "capsule"),
        dynamic_body(2, tetrahedron, {0.0, 4.0, 0.0}, "hull"),
        dynamic_body(3, pig, {4.0, 4.0, 0.0}, "pig_compound"),
    }));
    NINHO_SIM_REQUIRE(created.ok());
    NINHO_SIM_REQUIRE(created.value->physics_metrics().body_count == 3);
    NINHO_SIM_REQUIRE(created.value->physics_metrics().shape_count == 4);

    const auto& capsule_snapshot = snapshot(*created.value, EntityId{1});
    NINHO_SIM_REQUIRE(capsule_snapshot.shape.type == ShapeType::Capsule);
    NINHO_SIM_REQUIRE(capsule_snapshot.shape.radius_m == 0.25);
    NINHO_SIM_REQUIRE(capsule_snapshot.shape.half_height_m == 0.5);
    NINHO_SIM_REQUIRE(capsule_snapshot.visual_id == "capsule");
    const auto capsule_bounds = detail::SessionTestFacade::body_bounds(
        *created.value, EntityId{1}, PartId{1});
    NINHO_SIM_REQUIRE(capsule_bounds.has_value());
    NINHO_SIM_REQUIRE(std::abs(capsule_bounds->lower.x - -4.27f) <= 1.0e-4f);
    NINHO_SIM_REQUIRE(std::abs(capsule_bounds->lower.y - 3.23f) <= 1.0e-4f);
    NINHO_SIM_REQUIRE(std::abs(capsule_bounds->upper.x - -3.73f) <= 1.0e-4f);
    NINHO_SIM_REQUIRE(std::abs(capsule_bounds->upper.y - 4.77f) <= 1.0e-4f);

    const auto& hull_snapshot = snapshot(*created.value, EntityId{2});
    NINHO_SIM_REQUIRE(hull_snapshot.shape == tetrahedron);
    NINHO_SIM_REQUIRE(hull_snapshot.visual_id == "hull");
    const auto hull_bounds = detail::SessionTestFacade::body_bounds(
        *created.value, EntityId{2}, PartId{1});
    NINHO_SIM_REQUIRE(hull_bounds.has_value());
    NINHO_SIM_REQUIRE(std::abs(hull_bounds->lower.x - -0.52f) <= 1.0e-4f);
    NINHO_SIM_REQUIRE(std::abs(hull_bounds->upper.z - 0.52f) <= 1.0e-4f);

    const auto& pig_snapshot = snapshot(*created.value, EntityId{3});
    NINHO_SIM_REQUIRE(pig_snapshot.shape == pig);
    NINHO_SIM_REQUIRE(pig_snapshot.visual_id == "pig_compound");
    NINHO_SIM_REQUIRE(std::abs(pig_snapshot.mass_kg - 65.0) <= 0.065);
    const auto pig_bounds = detail::SessionTestFacade::body_bounds(
        *created.value, EntityId{3}, PartId{1});
    NINHO_SIM_REQUIRE(pig_bounds.has_value());
    NINHO_SIM_REQUIRE(std::abs(pig_bounds->lower.x - 3.33f) <= 1.0e-4f);
    NINHO_SIM_REQUIRE(std::abs(pig_bounds->lower.y - 3.855f) <= 1.0e-4f);
    NINHO_SIM_REQUIRE(std::abs(pig_bounds->upper.x - 4.745f) <= 1.0e-4f);
    NINHO_SIM_REQUIRE(std::abs(pig_bounds->upper.y - 4.47f) <= 1.0e-4f);
    const float bounds_center_x = (pig_bounds->lower.x + pig_bounds->upper.x) * 0.5f;
    const float bounds_center_y = (pig_bounds->lower.y + pig_bounds->upper.y) * 0.5f;
    NINHO_SIM_REQUIRE(std::abs(bounds_center_x - 4.0375f) <= 1.0e-4f);
    NINHO_SIM_REQUIRE(std::abs(bounds_center_y - 4.1625f) <= 1.0e-4f);

    NINHO_SIM_REQUIRE(created.value->canonical_state_v2().empty());
    NINHO_SIM_REQUIRE(created.value->canonical_hash_v2() == 0U);
    const auto initial_snapshots = std::vector<EntitySnapshot>{
        created.value->snapshots().begin(), created.value->snapshots().end()};
    const auto initial_metrics = created.value->physics_metrics();
    const auto initial_allocator_bytes =
        ninho::physics::detail::box3d_allocator_byte_count();
    for (int restart = 0; restart < 20; ++restart) {
        NINHO_SIM_REQUIRE(created.value->restart().ok());
        NINHO_SIM_REQUIRE(created.value->canonical_state_v2().empty());
        NINHO_SIM_REQUIRE(created.value->canonical_hash_v2() == 0U);
        NINHO_SIM_REQUIRE(std::ranges::equal(
            created.value->snapshots(), initial_snapshots));
        NINHO_SIM_REQUIRE(created.value->physics_metrics().body_count
            == initial_metrics.body_count);
        NINHO_SIM_REQUIRE(created.value->physics_metrics().shape_count
            == initial_metrics.shape_count);
        NINHO_SIM_REQUIRE(
            ninho::physics::detail::box3d_allocator_byte_count()
            == initial_allocator_bytes);
    }
}

NINHO_SIM_TEST("compound shape accepts a valid convex hull at the 64 vertex limit")
{
    std::vector<std::array<double, 3>> vertices;
    vertices.reserve(64U);
    constexpr double pi = 3.14159265358979323846;
    for (double z : {-0.5, 0.5}) {
        for (std::size_t index = 0; index < 32U; ++index) {
            const double angle = 2.0 * pi * static_cast<double>(index) / 32.0;
            vertices.push_back({std::cos(angle), std::sin(angle), z});
        }
    }
    const auto created = create(level_with({
        dynamic_body(1, hull(std::move(vertices)), {}, "hull_64"),
    }));
    NINHO_SIM_REQUIRE(created.ok());
    NINHO_SIM_REQUIRE(created.value->physics_metrics().body_count == 1);
    NINHO_SIM_REQUIRE(created.value->physics_metrics().shape_count == 1);
}

NINHO_SIM_TEST("compound shape rejects expanded primitive budget before physics creation")
{
    std::vector<ShapeDefinition> root_children;
    root_children.reserve(16U);
    for (std::size_t group = 0; group < 16U; ++group) {
        std::vector<ShapeDefinition> leaf_compounds;
        leaf_compounds.reserve(16U);
        for (std::size_t leaf = 0; leaf < 16U; ++leaf) {
            const std::size_t leaf_size = group == 15U && leaf == 15U ? 3U : 2U;
            leaf_compounds.push_back(compound(
                std::vector<ShapeDefinition>(leaf_size, box(0.1, 0.1, 0.1))));
        }
        root_children.push_back(compound(std::move(leaf_compounds)));
    }

    const auto allocator_before = ninho::physics::detail::box3d_allocator_byte_count();
    const auto created = create(level_with({
        dynamic_body(1, compound(std::move(root_children)), {}, "over_budget"),
    }));
    NINHO_SIM_REQUIRE(!created.ok());
    NINHO_SIM_REQUIRE(created.error.code == ContentErrorCode::ResourceLimit);
    NINHO_SIM_REQUIRE(created.error.pointer == "/bodies/0/shape");
    NINHO_SIM_REQUIRE(
        ninho::physics::detail::box3d_allocator_byte_count() == allocator_before);
}

NINHO_SIM_TEST("compound shape rejects hull capacity before physics creation")
{
    std::vector<std::array<double, 3>> vertices;
    for (std::size_t index = 0; index < 65U; ++index) {
        const double angle = static_cast<double>(index) * 0.1;
        vertices.push_back({std::cos(angle), std::sin(angle),
            static_cast<double>(index % 3U) - 1.0});
    }
    const auto created = create(level_with({
        dynamic_body(1, hull(std::move(vertices)), {}, "oversized_hull"),
    }));
    NINHO_SIM_REQUIRE(!created.ok());
    NINHO_SIM_REQUIRE(created.error.code == ContentErrorCode::ResourceLimit);
    NINHO_SIM_REQUIRE(created.error.pointer == "/bodies/0/shape/vertices_m");
}

NINHO_SIM_TEST("compound shape rejects non convex hull before physics creation")
{
    const auto non_convex = hull({
        {0.0, 0.0, 0.0},
        {1.0, 0.0, 0.0},
        {0.0, 1.0, 0.0},
        {0.0, 0.0, 1.0},
        {0.1, 0.1, 0.1},
    });
    const auto created = create(level_with({
        dynamic_body(1, non_convex, {}, "non_convex_hull"),
    }));
    NINHO_SIM_REQUIRE(!created.ok());
    NINHO_SIM_REQUIRE(created.error.code == ContentErrorCode::InvalidInvariant);
    NINHO_SIM_REQUIRE(created.error.pointer == "/bodies/0/shape/vertices_m");
}

NINHO_SIM_TEST("compound shape rejects invalid typed local transform before physics creation")
{
    auto invalid_transform = box(0.5, 0.5, 0.5);
    invalid_transform.local_rotation_xyzw = {0.0, 0.0, 0.0, 0.0};
    const auto allocator_before = ninho::physics::detail::box3d_allocator_byte_count();
    const auto created = create(level_with({
        dynamic_body(1, invalid_transform, {}, "invalid_transform"),
    }));
    NINHO_SIM_REQUIRE(!created.ok());
    NINHO_SIM_REQUIRE(created.error.code == ContentErrorCode::InvalidInvariant);
    NINHO_SIM_REQUIRE(
        created.error.pointer == "/bodies/0/shape/local_transform/rotation_xyzw");
    NINHO_SIM_REQUIRE(
        ninho::physics::detail::box3d_allocator_byte_count() == allocator_before);
}

NINHO_SIM_TEST("compound shape capacity failure is atomic during reconfigure")
{
    const auto valid_level = level_with({
        dynamic_body(1, sphere(0.5), {0.0, 2.0, 0.0}, "valid"),
    });
    auto created = create(valid_level);
    NINHO_SIM_REQUIRE(created.ok());
    const auto initial_hash = created.value->canonical_hash_v2();
    const auto initial_snapshots = std::vector<EntitySnapshot>{
        created.value->snapshots().begin(), created.value->snapshots().end()};

    std::vector<BodyDefinition> too_many;
    too_many.reserve(501U);
    for (std::uint32_t index = 0; index < 501U; ++index) {
        too_many.push_back(dynamic_body(index + 1U, sphere(0.1),
            {static_cast<double>(index), 2.0, 0.0}, "capacity"));
    }
    const auto status = created.value->reconfigure(
        materials(), archetypes(), level_with(std::move(too_many)));
    NINHO_SIM_REQUIRE(!status.ok());
    NINHO_SIM_REQUIRE(status.error.code == ContentErrorCode::ResourceLimit);
    NINHO_SIM_REQUIRE(created.value->canonical_hash_v2() == initial_hash);
    NINHO_SIM_REQUIRE(std::ranges::equal(created.value->snapshots(), initial_snapshots));
}
