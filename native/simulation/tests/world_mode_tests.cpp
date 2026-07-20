#include "test_framework.hpp"

#include "session_test_facade.hpp"
#include "ninho/simulation/session.hpp"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <ranges>
#include <vector>

namespace {

using namespace ninho::simulation;

MaterialCatalog v2_materials()
{
    MaterialCatalog result;
    result.schema_version = result.source_schema_version = 2U;
    result.materials.push_back({MaterialId{1}, "wood", MaterialResponse::Fibrous,
        500.0, 0.5, 0.1, 1.0});
    result.surfaces.push_back({SurfaceId{1001}, "ground", 1000.0, 0.7, 0.0});
    result.surfaces.push_back({SurfaceId{1002}, "pig", 480.0, 0.4, 0.1});
    return result;
}

ArchetypeCatalog v2_archetypes()
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

BodyDefinition body(std::uint32_t body_id, std::uint32_t entity_id,
    BodyType type, ShapeDefinition shape, std::array<double, 3> position,
    std::string visual_id, bool affected_by_world_gravity)
{
    BodyDefinition result;
    result.body_id = body_id;
    result.entity_id = EntityId{entity_id};
    result.part_id = PartId{1};
    result.body_type = type;
    result.surface_id = type == BodyType::Static ? SurfaceId{1001} : SurfaceId{1002};
    result.density_kg_m3 = type == BodyType::Static ? 1000.0 : 480.0;
    result.transform.position_m = position;
    result.transform.rotation_xyzw = {0.0, 0.0, 0.0, 1.0};
    result.shape = std::move(shape);
    result.visual.asset_id = std::move(visual_id);
    result.visual.bounds_m = {1.0, 1.0, 1.0};
    result.affected_by_world_gravity = affected_by_world_gravity;
    return result;
}

LevelManifest uniform_level()
{
    LevelManifest result;
    result.schema_version = result.source_schema_version = 2U;
    result.id = "farm";
    result.world_id = "earth";
    result.world = UniformWorldDefinition{
        .acceleration_m_s2 = {0.0, -9.81, 0.0},
        .bounds_min_m = {-24.0, -12.0, -12.0},
        .bounds_max_m = {48.0, 32.0, 12.0},
    };
    return result;
}

LevelManifest radial_level()
{
    LevelManifest result;
    result.schema_version = result.source_schema_version = 2U;
    result.id = "orbital_v2";
    result.world_id = "orbital";
    result.world = RadialWorldDefinition{
        .center_m = {10.0, 0.0, 0.0},
        .reference_radius_m = 2.0,
        .reference_acceleration_m_s2 = 9.0,
        .bounds_radius_m = 12.0,
    };
    return result;
}

ContentResult<std::unique_ptr<SimulationSession>> create(LevelManifest level)
{
    if (level.free_body_ids.empty() && level.assemblies.empty()) {
        for (const BodyDefinition& definition : level.bodies) {
            level.free_body_ids.push_back(definition.body_id);
        }
    }
    return SimulationSession::create(v2_materials(), v2_archetypes(), level);
}

ContentResult<std::unique_ptr<SimulationSession>> create(
    ArchetypeCatalog archetypes, LevelManifest level)
{
    if (level.free_body_ids.empty() && level.assemblies.empty()) {
        for (const BodyDefinition& definition : level.bodies) {
            level.free_body_ids.push_back(definition.body_id);
        }
    }
    return SimulationSession::create(v2_materials(), archetypes, level);
}

const EntitySnapshot& snapshot(const SimulationSession& session, EntityId entity)
{
    const auto found = std::ranges::find(session.snapshots(), entity,
        &EntitySnapshot::entity_id);
    NINHO_SIM_REQUIRE(found != session.snapshots().end());
    return *found;
}

}

NINHO_SIM_TEST("world mode uniform builds only declared ground scenery and dynamic bodies")
{
    auto level = uniform_level();
    level.bodies.push_back(body(30, 30, BodyType::Static,
        box(8.0, 0.5, 3.0), {0.0, -1.0, 0.0}, "ground", false));
    level.bodies.push_back(body(10, 10, BodyType::Dynamic,
        sphere(0.5), {0.0, 4.0, 0.0}, "falling_pig", true));
    level.bodies.push_back(body(20, 20, BodyType::Static,
        box(0.5, 2.0, 0.5), {3.0, 1.0, 0.0}, "scenery", false));
    level.bodies.back().density_kg_m3 = 0.0;

    auto created = create(level);
    NINHO_SIM_REQUIRE(created.ok());
    NINHO_SIM_REQUIRE(created.value->snapshots().size() == 3U);
    NINHO_SIM_REQUIRE(std::ranges::is_sorted(created.value->snapshots(), {},
        [](const EntitySnapshot& item) { return item.entity_id; }));
    NINHO_SIM_REQUIRE(snapshot(*created.value, EntityId{30}).visual_id == "ground");
    NINHO_SIM_REQUIRE(snapshot(*created.value, EntityId{20}).visual_id == "scenery");
    NINHO_SIM_REQUIRE(!snapshot(*created.value, EntityId{10}).exited_world);
    NINHO_SIM_REQUIRE(detail::SessionTestFacade::affected_by_world_gravity(
        *created.value, EntityId{10}, PartId{1}));
    NINHO_SIM_REQUIRE(!detail::SessionTestFacade::affected_by_world_gravity(
        *created.value, EntityId{20}, PartId{1}));
    const auto before = snapshot(*created.value, EntityId{10}).transform.position.y;
    NINHO_SIM_REQUIRE(created.value->tick().ok());
    NINHO_SIM_REQUIRE(snapshot(*created.value, EntityId{10}).transform.position.y < before);
}

NINHO_SIM_TEST("world mode radial uses declared planet and preserves declared gravity center")
{
    auto level = radial_level();
    level.bodies.push_back(body(50, 50, BodyType::Static,
        sphere(2.0), {10.0, 0.0, 0.0}, "declared_planet", false));
    level.bodies.push_back(body(10, 10, BodyType::Dynamic,
        sphere(0.25), {13.0, 0.0, 0.0}, "radial_probe", true));

    auto created = create(level);
    NINHO_SIM_REQUIRE(created.ok());
    NINHO_SIM_REQUIRE(created.value->snapshots().size() == 2U);
    NINHO_SIM_REQUIRE(snapshot(*created.value, EntityId{50}).visual_id == "declared_planet");
    const auto gravity_right = detail::SessionTestFacade::gravity_at(
        *created.value, {13.0f, 0.0f, 0.0f});
    const auto gravity_left = detail::SessionTestFacade::gravity_at(
        *created.value, {7.0f, 0.0f, 0.0f});
    NINHO_SIM_REQUIRE(gravity_right.x < 0.0f && gravity_left.x > 0.0f);
    NINHO_SIM_REQUIRE(std::abs(gravity_right.y) <= 1.0e-6f);
    const auto before = snapshot(*created.value, EntityId{10}).transform.position.x;
    NINHO_SIM_REQUIRE(created.value->tick().ok());
    NINHO_SIM_REQUIRE(snapshot(*created.value, EntityId{10}).transform.position.x < before);
}

NINHO_SIM_TEST("world mode publishes bounds exit separately from orbital ejection")
{
    auto level = uniform_level();
    level.bodies.push_back(body(1, 1, BodyType::Dynamic,
        sphere(0.5), {60.0, 0.0, 0.0}, "outside", true));
    auto created = create(level);
    NINHO_SIM_REQUIRE(created.ok());
    NINHO_SIM_REQUIRE(created.value->tick().ok());
    const auto& outside = snapshot(*created.value, EntityId{1});
    NINHO_SIM_REQUIRE(outside.exited_world);
    NINHO_SIM_REQUIRE(!outside.ejected);
}

NINHO_SIM_TEST("world mode v2 body order is canonical and independent of declaration order")
{
    auto level = uniform_level();
    level.bodies.push_back(body(2, 300, BodyType::Static,
        box(1.0, 1.0, 1.0), {0.0, 0.0, 0.0}, "last", false));
    level.bodies.push_back(body(1, 100, BodyType::Static,
        box(1.0, 1.0, 1.0), {2.0, 0.0, 0.0}, "first", false));
    auto reversed = level;
    std::ranges::reverse(reversed.bodies);

    auto first = create(level);
    auto second = create(reversed);
    NINHO_SIM_REQUIRE(first.ok() && second.ok());
    NINHO_SIM_REQUIRE(std::ranges::equal(first.value->snapshots(), second.value->snapshots()));
    NINHO_SIM_REQUIRE(first.value->canonical_state_v2().empty());
    NINHO_SIM_REQUIRE(first.value->canonical_hash_v2() == 0U);
}

NINHO_SIM_TEST("world mode v2 never publishes legacy canonical state v2")
{
    auto level = uniform_level();
    level.bodies.push_back(body(1, 1, BodyType::Dynamic,
        sphere(0.5), {0.0, 3.0, 0.0}, "pig", true));
    auto created = create(level);
    NINHO_SIM_REQUIRE(created.ok());
    NINHO_SIM_REQUIRE(created.value->canonical_state_v2().empty());
    NINHO_SIM_REQUIRE(created.value->canonical_hash_v2() == 0U);
    NINHO_SIM_REQUIRE(created.value->tick().ok());
    NINHO_SIM_REQUIRE(created.value->canonical_state_v2().empty());
    NINHO_SIM_REQUIRE(created.value->canonical_hash_v2() == 0U);
    NINHO_SIM_REQUIRE(created.value->restart().ok());
    NINHO_SIM_REQUIRE(created.value->canonical_state_v2().empty());
    NINHO_SIM_REQUIRE(created.value->canonical_hash_v2() == 0U);
}

NINHO_SIM_TEST("world mode v2 rejects orphan queue enemy weakpoint and enemy body references")
{
    auto orphan_queue = uniform_level();
    orphan_queue.bird_queue.push_back(BirdArchetypeId{999});
    auto rejected = create(orphan_queue);
    NINHO_SIM_REQUIRE(!rejected.ok());
    NINHO_SIM_REQUIRE(rejected.error.code == ContentErrorCode::MissingReference);
    NINHO_SIM_REQUIRE(rejected.error.pointer == "/bird_queue/0");

    auto orphan_weakpoint_archetypes = v2_archetypes();
    orphan_weakpoint_archetypes.enemies.push_back({EnemyArchetypeId{1}, "pig",
        WeakpointId{999}, SurfaceId{1002}, 65.0, 100.0, 2.5, 50.0});
    rejected = create(orphan_weakpoint_archetypes, uniform_level());
    NINHO_SIM_REQUIRE(!rejected.ok());
    NINHO_SIM_REQUIRE(rejected.error.code == ContentErrorCode::MissingReference);
    NINHO_SIM_REQUIRE(rejected.error.pointer == "/enemies/0/weakpoint_id");

    auto orphan_enemy_body = uniform_level();
    auto pig = body(1, 1, BodyType::Dynamic,
        sphere(0.5), {0.0, 3.0, 0.0}, "pig", true);
    pig.enemy_archetype_id = EnemyArchetypeId{999};
    orphan_enemy_body.bodies.push_back(std::move(pig));
    rejected = create(orphan_enemy_body);
    NINHO_SIM_REQUIRE(!rejected.ok());
    NINHO_SIM_REQUIRE(rejected.error.code == ContentErrorCode::MissingReference);
    NINHO_SIM_REQUIRE(rejected.error.pointer == "/bodies/0/enemy_archetype_id");
}

NINHO_SIM_TEST("world mode rejects v2 dynamic body with gravity disabled at build boundary")
{
    auto level = uniform_level();
    level.bodies.push_back(body(1, 1, BodyType::Dynamic,
        sphere(0.5), {0.0, 3.0, 0.0}, "invalid_dynamic", false));
    const auto created = create(level);
    NINHO_SIM_REQUIRE(!created.ok());
    NINHO_SIM_REQUIRE(created.error.code == ContentErrorCode::InvalidInvariant);
    NINHO_SIM_REQUIRE(created.error.pointer == "/bodies/0/affected_by_world_gravity");
}

NINHO_SIM_TEST("world mode runtime dynamic body inherits gravity independently of initial list")
{
    auto level = uniform_level();
    level.bodies.push_back(body(1, 1, BodyType::Static,
        box(4.0, 0.25, 4.0), {0.0, -2.0, 0.0}, "ground", false));
    auto created = create(level);
    NINHO_SIM_REQUIRE(created.ok());
    NINHO_SIM_REQUIRE(detail::SessionTestFacade::add_dynamic_sphere(
        *created.value, EntityId{0x80000001U}, PartId{1}, {0.0f, 5.0f, 0.0f}, 1.0));
    NINHO_SIM_REQUIRE(created.value->tick().ok());
    const auto first_y = snapshot(*created.value, EntityId{0x80000001U}).transform.position.y;
    NINHO_SIM_REQUIRE(created.value->tick().ok());
    NINHO_SIM_REQUIRE(
        snapshot(*created.value, EntityId{0x80000001U}).transform.position.y < first_y);
}

NINHO_SIM_TEST("world mode failed reconfigure is atomic and twenty restarts do not drift")
{
    auto level = uniform_level();
    level.bodies.push_back(body(1, 1, BodyType::Dynamic,
        sphere(0.5), {0.0, 3.0, 0.0}, "pig", true));
    auto created = create(level);
    NINHO_SIM_REQUIRE(created.ok());
    NINHO_SIM_REQUIRE(created.value->canonical_state_v2().empty());
    NINHO_SIM_REQUIRE(created.value->canonical_hash_v2() == 0U);
    const auto initial_snapshots = std::vector<EntitySnapshot>{
        created.value->snapshots().begin(), created.value->snapshots().end()};
    const auto initial_metrics = created.value->physics_metrics();

    auto invalid = level;
    invalid.bodies.front().affected_by_world_gravity = false;
    const auto failed = created.value->reconfigure(
        v2_materials(), v2_archetypes(), invalid);
    NINHO_SIM_REQUIRE(!failed.ok());
    NINHO_SIM_REQUIRE(created.value->canonical_state_v2().empty());
    NINHO_SIM_REQUIRE(created.value->canonical_hash_v2() == 0U);
    NINHO_SIM_REQUIRE(std::ranges::equal(created.value->snapshots(), initial_snapshots));

    for (int iteration = 0; iteration < 20; ++iteration) {
        NINHO_SIM_REQUIRE(created.value->tick().ok());
        NINHO_SIM_REQUIRE(created.value->restart().ok());
        NINHO_SIM_REQUIRE(created.value->canonical_state_v2().empty());
        NINHO_SIM_REQUIRE(created.value->canonical_hash_v2() == 0U);
        NINHO_SIM_REQUIRE(std::ranges::equal(created.value->snapshots(), initial_snapshots));
        NINHO_SIM_REQUIRE(created.value->physics_metrics().body_count
            == initial_metrics.body_count);
        NINHO_SIM_REQUIRE(created.value->physics_metrics().shape_count
            == initial_metrics.shape_count);
    }
}
