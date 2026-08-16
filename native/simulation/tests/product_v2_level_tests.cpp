#include "test_framework.hpp"

#include "ninho/simulation/content.hpp"
#include "ninho/simulation/session.hpp"
#include "physics_world_test_facade.hpp"
#include "product_v2_layout_fixture.hpp"
#include "product_v2_reader.hpp"
#include "shape_volume.hpp"

#include <ninho/physics/physics_world.hpp>

#include <nlohmann/json.hpp>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <limits>
#include <numbers>
#include <ranges>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace
{

using json = nlohmann::json;
using namespace ninho::simulation;

constexpr std::string_view materials_path =
    "game/data/materials/product_v2.materials.json";
constexpr std::string_view archetypes_path =
    "game/data/archetypes/product_v2.archetypes.json";
constexpr std::string_view campaign_path =
    "game/data/worlds/world_catalog.v2.json";
constexpr std::string_view farm_path =
    "game/data/levels/earth/farm_reaction.level.json";
constexpr std::string_view orbital_path =
    "game/data/levels/orbital/first_orbit_v2.level.json";
constexpr std::string_view assets_path =
    "game/data/assets/product_v2.assets.json";
constexpr std::string_view feedback_path =
    "game/data/feedback/product_v2.feedback.json";
constexpr std::string_view fixture_path =
    "native/simulation/tests/fixtures/product_v2/farm_reaction_layout_v1.json";

std::string read_file(std::string_view relative)
{
    std::ifstream stream{std::filesystem::path{NINHO_SOURCE_DIR} / relative,
        std::ios::binary};
    NINHO_SIM_REQUIRE(stream.is_open());
    std::ostringstream contents;
    contents << stream.rdbuf();
    return contents.str();
}

std::uint64_t fnv1a64(std::string_view bytes)
{
    std::uint64_t result = 14695981039346656037ULL;
    for (const unsigned char byte : bytes) {
        result ^= byte;
        result *= 1099511628211ULL;
    }
    return result;
}

json parsed(std::string_view path)
{
    return json::parse(read_file(path));
}

ContentResult<json> validate_assets(std::string_view source)
{
    using namespace ninho::simulation::detail::v2content;
    return boundary<json>([&] {
        json root = parse_json(source, catalog_max_bytes);
        keys(root, "", {"schema_version", "assets"});
        schema_v2(root);
        const auto& assets = member(root, "assets", "");
        array(assets, "/assets", 512U, true);
        std::unordered_set<std::string> ids;
        for (std::size_t index = 0; index < assets.size(); ++index) {
            const std::string pointer = indexed("/assets", index);
            const auto& asset = assets[index];
            keys(asset, pointer, {"id", "kind", "status", "resource_path",
                "node_path", "presentation_only"});
            const std::string id = text(asset, "id", pointer);
            if (!ids.insert(id).second)
                fail(ContentErrorCode::DuplicateId, child(pointer, "id"),
                    "duplicate asset id");
            static_cast<void>(text(asset, "kind", pointer));
            const std::string status = text(asset, "status", pointer);
            if (status != "planned" && status != "required")
                fail(ContentErrorCode::InvalidEnum, child(pointer, "status"),
                    "invalid asset status");
            const std::string path = text(asset, "resource_path", pointer);
            if ((!path.starts_with("res://assets/product_v2/")
                    && !path.starts_with("res://assets/vertical_slice/"))
                || path.find("..") != std::string::npos)
                fail(ContentErrorCode::InvalidInvariant,
                    child(pointer, "resource_path"), "asset path is outside its roots");
            static_cast<void>(text(asset, "node_path", pointer));
            static_cast<void>(boolean(asset, "presentation_only", pointer));
        }
        return root;
    });
}

ContentResult<json> validate_feedback(std::string_view source)
{
    using namespace ninho::simulation::detail::v2content;
    return boundary<json>([&] {
        json root = parse_json(source, catalog_max_bytes);
        keys(root, "", {"schema_version", "budgets", "material_profiles",
            "event_profiles", "outcome_profiles", "profiles"});
        schema_v2(root);
        const auto& budgets = member(root, "budgets", "");
        keys(budgets, "/budgets", {"vfx_pool_size", "audio_voice_pool_size",
            "fragment_pool_size", "max_particles_per_slot", "max_particles_total",
            "max_fragments_total", "glass_screen_coverage_limit",
            "first_impact_limit_ms"});
        for (const auto key : {"vfx_pool_size", "audio_voice_pool_size",
            "fragment_pool_size", "max_particles_per_slot", "max_particles_total",
            "max_fragments_total"})
            static_cast<void>(uint(budgets, key, "/budgets", 1U, 100000U));
        static_cast<void>(number(budgets, "glass_screen_coverage_limit",
            "/budgets", 0.0, 1.0));
        static_cast<void>(number(budgets, "first_impact_limit_ms",
            "/budgets", 0.0, 10000.0, false));

        const auto& materials = member(root, "material_profiles", "");
        keys(materials, "/material_profiles", {"1", "5", "9", "13", "17"});
        for (const auto key : {"1", "5", "9", "13", "17"})
            static_cast<void>(text(materials, key, "/material_profiles"));

        constexpr std::array event_keys{
            "bird_launched", "ability_activation_requested", "command_rejected",
            "ability_started", "ability_affected_body", "ability_pulse",
            "ability_ended", "damage_applied", "entity_neutralized",
            "joint_overloaded", "piece_fracture_triggered", "joint_broken",
            "piece_fractured", "mass_changed", "speed_changed", "projectile_split",
            "projectile_spawned", "explosion_fuse_armed", "pressure_burst",
            "environmental_trigger_armed", "environmental_trigger_detonated",
            "material_yielded", "crush_damage_applied", "score_awarded",
            "chain_changed", "stars_awarded"};
        const auto& events = member(root, "event_profiles", "");
        object(events, "/event_profiles");
        const std::unordered_set<std::string_view> expected_events(
            event_keys.begin(), event_keys.end());
        for (const auto& [key, ignored] : events.items()) {
            static_cast<void>(ignored);
            if (!expected_events.contains(key))
                fail(ContentErrorCode::UnknownKey, child("/event_profiles", key),
                    "unknown event profile key");
        }

        const auto& outcomes = member(root, "outcome_profiles", "");
        keys(outcomes, "/outcome_profiles", {"victory", "defeat"});
        const auto& profiles = member(root, "profiles", "");
        object(profiles, "/profiles");
        std::unordered_set<std::string> referenced;
        for (const auto event : event_keys)
            referenced.insert(text(events, event, "/event_profiles"));
        referenced.insert(text(outcomes, "victory", "/outcome_profiles"));
        referenced.insert(text(outcomes, "defeat", "/outcome_profiles"));
        for (const auto material : {"1", "5", "9", "13", "17"})
            referenced.insert(text(materials, material, "/material_profiles"));
        for (const auto& [key, profile] : profiles.items()) {
            const std::string pointer = "/profiles/" + key;
            keys(profile, pointer, {"color", "particles", "lifetime_s",
                "size_m", "audio"});
            const std::string color = text(profile, "color", pointer);
            if (color.size() != 7U || color.front() != '#')
                fail(ContentErrorCode::InvalidInvariant, child(pointer, "color"),
                    "profile color must use RGB hex syntax");
            static_cast<void>(uint(profile, "particles", pointer, 1U, 4096U));
            static_cast<void>(number(profile, "lifetime_s", pointer,
                0.0, 60.0, false));
            static_cast<void>(number(profile, "size_m", pointer,
                0.0, 100.0, false));
            const auto& audio = member(profile, "audio", pointer);
            if (!audio.is_string())
                fail(ContentErrorCode::InvalidType, child(pointer, "audio"),
                    "audio id must be a string");
        }
        for (const auto& reference : referenced) {
            if (!profiles.contains(reference))
                fail(ContentErrorCode::MissingReference, "/profiles",
                    "referenced feedback profile is missing");
        }
        return root;
    });
}

void require_exact_keys(const json& value,
    std::initializer_list<std::string_view> expected)
{
    NINHO_SIM_REQUIRE(value.is_object());
    std::unordered_set<std::string> keys;
    for (const auto key : expected) keys.emplace(key);
    NINHO_SIM_REQUIRE(value.size() == keys.size());
    for (const auto& [key, ignored] : value.items()) {
        static_cast<void>(ignored);
        NINHO_SIM_REQUIRE(keys.contains(key));
    }
}

json layout_projection(const json& source)
{
    const auto typed = parse_level_manifest_v2(source.dump());
    if (!typed.ok()) {
        throw std::runtime_error("invalid layout probe at " + typed.error.pointer
            + ": " + typed.error.message);
    }
    return test::product_v2_layout_projection(typed.value);
}

std::string layout_hash(const json& level)
{
    const auto typed = parse_level_manifest_v2(level.dump());
    if (!typed.ok()) {
        throw std::runtime_error("invalid layout probe at " + typed.error.pointer
            + ": " + typed.error.message);
    }
    return test::product_v2_layout_hash(typed.value);
}

struct LoadedProduct
{
    MaterialCatalog materials;
    ArchetypeCatalog archetypes;
    CampaignManifest campaign;
    LevelManifest farm;
    LevelManifest orbital;
};

LoadedProduct load_product()
{
    const auto materials = parse_material_catalog_v2(read_file(materials_path));
    const auto archetypes = parse_archetype_catalog_v2(read_file(archetypes_path));
    const auto campaign = parse_campaign_manifest_v2(read_file(campaign_path));
    const auto farm = parse_level_manifest_v2(read_file(farm_path));
    const auto orbital = parse_level_manifest_v2(read_file(orbital_path));
    NINHO_SIM_REQUIRE(materials.ok());
    NINHO_SIM_REQUIRE(archetypes.ok());
    NINHO_SIM_REQUIRE(campaign.ok());
    if (!farm.ok()) {
        throw std::runtime_error("invalid farm fixture at " + farm.error.pointer
            + ": " + farm.error.message);
    }
    NINHO_SIM_REQUIRE(orbital.ok());
    const auto farm_bundle = make_product_v2_content_bundle(materials.value,
        archetypes.value, campaign.value, farm.value);
    if (!farm_bundle.ok()) {
        throw std::runtime_error("invalid farm bundle at "
            + farm_bundle.error.pointer + ": " + farm_bundle.error.message);
    }
    NINHO_SIM_REQUIRE(make_product_v2_content_bundle(materials.value,
        archetypes.value, campaign.value, orbital.value).ok());
    return {materials.value, archetypes.value, campaign.value,
        farm.value, orbital.value};
}

const BodyDefinition& body(const LevelManifest& level, std::uint32_t id)
{
    const auto found = std::ranges::find(level.bodies, id, &BodyDefinition::body_id);
    NINHO_SIM_REQUIRE(found != level.bodies.end());
    return *found;
}

struct TestPose
{
    std::array<double, 3> position{};
    std::array<double, 4> rotation{0.0, 0.0, 0.0, 1.0};
};

struct TestAabb
{
    std::array<double, 3> minimum{
        std::numeric_limits<double>::infinity(),
        std::numeric_limits<double>::infinity(),
        std::numeric_limits<double>::infinity()};
    std::array<double, 3> maximum{
        -std::numeric_limits<double>::infinity(),
        -std::numeric_limits<double>::infinity(),
        -std::numeric_limits<double>::infinity()};
};

struct TestLeaf
{
    TestAabb bounds;
    ShapeType type{ShapeType::Box};
    TestPose pose;
    std::array<double, 3> half_extents{};
};

std::array<double, 3> rotate_vector(
    const std::array<double, 4>& q, const std::array<double, 3>& value)
{
    const std::array cross{
        q[1] * value[2] - q[2] * value[1],
        q[2] * value[0] - q[0] * value[2],
        q[0] * value[1] - q[1] * value[0]};
    const std::array twice_cross{2.0 * cross[0], 2.0 * cross[1], 2.0 * cross[2]};
    const std::array second_cross{
        q[1] * twice_cross[2] - q[2] * twice_cross[1],
        q[2] * twice_cross[0] - q[0] * twice_cross[2],
        q[0] * twice_cross[1] - q[1] * twice_cross[0]};
    return {value[0] + q[3] * twice_cross[0] + second_cross[0],
        value[1] + q[3] * twice_cross[1] + second_cross[1],
        value[2] + q[3] * twice_cross[2] + second_cross[2]};
}

std::array<double, 4> multiply_rotation(
    const std::array<double, 4>& a, const std::array<double, 4>& b)
{
    return {
        a[3] * b[0] + a[0] * b[3] + a[1] * b[2] - a[2] * b[1],
        a[3] * b[1] - a[0] * b[2] + a[1] * b[3] + a[2] * b[0],
        a[3] * b[2] + a[0] * b[1] - a[1] * b[0] + a[2] * b[3],
        a[3] * b[3] - a[0] * b[0] - a[1] * b[1] - a[2] * b[2]};
}

TestPose compose_pose(const TestPose& parent,
    const std::array<double, 3>& local_position,
    const std::array<double, 4>& local_rotation)
{
    const auto rotated = rotate_vector(parent.rotation, local_position);
    return {{parent.position[0] + rotated[0], parent.position[1] + rotated[1],
                parent.position[2] + rotated[2]},
        multiply_rotation(parent.rotation, local_rotation)};
}

void include_point(TestAabb& bounds, const std::array<double, 3>& point)
{
    for (std::size_t axis = 0; axis < 3U; ++axis) {
        bounds.minimum[axis] = std::min(bounds.minimum[axis], point[axis]);
        bounds.maximum[axis] = std::max(bounds.maximum[axis], point[axis]);
    }
}

void append_leaf_bounds(
    const ShapeDefinition& shape, const TestPose& parent, std::vector<TestLeaf>& output)
{
    const TestPose pose = compose_pose(
        parent, shape.local_position_m, shape.local_rotation_xyzw);
    if (shape.type == ShapeType::Compound) {
        for (const ShapeDefinition& child : shape.children) {
            append_leaf_bounds(child, pose, output);
        }
        return;
    }
    TestAabb bounds;
    if (shape.type == ShapeType::Sphere) {
        for (std::size_t axis = 0; axis < 3U; ++axis) {
            bounds.minimum[axis] = pose.position[axis] - shape.radius_m;
            bounds.maximum[axis] = pose.position[axis] + shape.radius_m;
        }
    } else if (shape.type == ShapeType::Capsule) {
        const auto axis = rotate_vector(pose.rotation, {0.0, shape.half_height_m, 0.0});
        for (std::size_t index = 0; index < 3U; ++index) {
            bounds.minimum[index] = pose.position[index]
                - std::abs(axis[index]) - shape.radius_m;
            bounds.maximum[index] = pose.position[index]
                + std::abs(axis[index]) + shape.radius_m;
        }
    } else if (shape.type == ShapeType::Box) {
        for (double x : {-shape.half_extents_m[0], shape.half_extents_m[0]}) {
            for (double y : {-shape.half_extents_m[1], shape.half_extents_m[1]}) {
                for (double z : {-shape.half_extents_m[2], shape.half_extents_m[2]}) {
                    const auto rotated = rotate_vector(pose.rotation, {x, y, z});
                    include_point(bounds, {pose.position[0] + rotated[0],
                        pose.position[1] + rotated[1], pose.position[2] + rotated[2]});
                }
            }
        }
    } else {
        for (const auto& vertex : shape.vertices_m) {
            const auto rotated = rotate_vector(pose.rotation, vertex);
            include_point(bounds, {pose.position[0] + rotated[0],
                pose.position[1] + rotated[1], pose.position[2] + rotated[2]});
        }
    }
    output.push_back({bounds, shape.type, pose, shape.half_extents_m});
}

std::vector<TestLeaf> body_leaf_bounds(const BodyDefinition& definition)
{
    std::vector<TestLeaf> result;
    append_leaf_bounds(definition.shape,
        {definition.transform.position_m, definition.transform.rotation_xyzw}, result);
    return result;
}

std::vector<TestLeaf> body_local_leaf_bounds(const BodyDefinition& definition)
{
    std::vector<TestLeaf> result;
    append_leaf_bounds(definition.shape, TestPose{}, result);
    return result;
}

double dot(const std::array<double, 3>& a, const std::array<double, 3>& b)
{
    return a[0] * b[0] + a[1] * b[1] + a[2] * b[2];
}

std::array<double, 3> cross(
    const std::array<double, 3>& a, const std::array<double, 3>& b)
{
    return {a[1] * b[2] - a[2] * b[1],
        a[2] * b[0] - a[0] * b[2],
        a[0] * b[1] - a[1] * b[0]};
}

bool box_strictly_overlaps(const TestLeaf& a, const TestLeaf& b)
{
    constexpr double tolerance_m = 1.0e-5;
    const std::array a_axes{
        rotate_vector(a.pose.rotation, {1.0, 0.0, 0.0}),
        rotate_vector(a.pose.rotation, {0.0, 1.0, 0.0}),
        rotate_vector(a.pose.rotation, {0.0, 0.0, 1.0})};
    const std::array b_axes{
        rotate_vector(b.pose.rotation, {1.0, 0.0, 0.0}),
        rotate_vector(b.pose.rotation, {0.0, 1.0, 0.0}),
        rotate_vector(b.pose.rotation, {0.0, 0.0, 1.0})};
    const std::array center_delta{b.pose.position[0] - a.pose.position[0],
        b.pose.position[1] - a.pose.position[1],
        b.pose.position[2] - a.pose.position[2]};
    std::vector<std::array<double, 3>> candidate_axes;
    candidate_axes.reserve(15U);
    candidate_axes.insert(candidate_axes.end(), a_axes.begin(), a_axes.end());
    candidate_axes.insert(candidate_axes.end(), b_axes.begin(), b_axes.end());
    for (const auto& a_axis : a_axes) {
        for (const auto& b_axis : b_axes) {
            const auto axis = cross(a_axis, b_axis);
            const double length_squared = dot(axis, axis);
            if (length_squared <= 1.0e-20) continue;
            const double inverse_length = 1.0 / std::sqrt(length_squared);
            candidate_axes.push_back({axis[0] * inverse_length,
                axis[1] * inverse_length, axis[2] * inverse_length});
        }
    }
    for (const auto& axis : candidate_axes) {
        double radius_a = 0.0;
        double radius_b = 0.0;
        for (std::size_t index = 0; index < 3U; ++index) {
            radius_a += a.half_extents[index] * std::abs(dot(a_axes[index], axis));
            radius_b += b.half_extents[index] * std::abs(dot(b_axes[index], axis));
        }
        const double penetration = radius_a + radius_b
            - std::abs(dot(center_delta, axis));
        if (penetration <= tolerance_m) return false;
    }
    return true;
}

bool sphere_strictly_overlaps_box(const TestLeaf& sphere, const TestLeaf& box)
{
    constexpr double tolerance_m = 1.0e-5;
    const double radius_m = (sphere.bounds.maximum[0]
        - sphere.bounds.minimum[0]) * 0.5;
    const std::array center_delta{
        sphere.pose.position[0] - box.pose.position[0],
        sphere.pose.position[1] - box.pose.position[1],
        sphere.pose.position[2] - box.pose.position[2]};
    const std::array box_axes{
        rotate_vector(box.pose.rotation, {1.0, 0.0, 0.0}),
        rotate_vector(box.pose.rotation, {0.0, 1.0, 0.0}),
        rotate_vector(box.pose.rotation, {0.0, 0.0, 1.0})};
    double squared_distance_m2 = 0.0;
    for (std::size_t axis = 0; axis < 3U; ++axis) {
        const double coordinate_m = dot(center_delta, box_axes[axis]);
        const double outside_m = std::max(
            std::abs(coordinate_m) - box.half_extents[axis], 0.0);
        squared_distance_m2 += outside_m * outside_m;
    }
    const double strict_radius_m = radius_m - tolerance_m;
    return strict_radius_m > 0.0
        && squared_distance_m2 < strict_radius_m * strict_radius_m;
}

bool strictly_overlaps(const TestLeaf& a, const TestLeaf& b)
{
    constexpr double tolerance_m = 1.0e-5;
    for (std::size_t axis = 0; axis < 3U; ++axis) {
        const double overlap = std::min(a.bounds.maximum[axis], b.bounds.maximum[axis])
            - std::max(a.bounds.minimum[axis], b.bounds.minimum[axis]);
        if (overlap <= tolerance_m) return false;
    }
    if (a.type == ShapeType::Box && b.type == ShapeType::Box)
        return box_strictly_overlaps(a, b);
    if (a.type == ShapeType::Sphere && b.type == ShapeType::Box)
        return sphere_strictly_overlaps_box(a, b);
    if (a.type == ShapeType::Box && b.type == ShapeType::Sphere)
        return sphere_strictly_overlaps_box(b, a);
    return true;
}

bool is_idle_breakage_event(DomainEventKind kind)
{
    switch (kind) {
    case DomainEventKind::JointOverloaded:
    case DomainEventKind::JointBroken:
    case DomainEventKind::PieceFractureTriggered:
    case DomainEventKind::PieceFractured:
    case DomainEventKind::EnvironmentalTriggerArmed:
    case DomainEventKind::EnvironmentalTriggerDetonated:
    case DomainEventKind::DamageApplied:
    case DomainEventKind::CrushDamageApplied:
    case DomainEventKind::EntityNeutralized:
    case DomainEventKind::MaterialYielded:
        return true;
    default:
        return false;
    }
}

NINHO_SIM_TEST("product v2 levels load both closed bundles and preserve catalogs")
{
    const LoadedProduct product = load_product();
    NINHO_SIM_REQUIRE(product.materials.materials.size() == 5U);
    const std::array expected_materials{
        std::pair{1U, MaterialResponse::Fibrous},
        std::pair{5U, MaterialResponse::Masonry},
        std::pair{9U, MaterialResponse::Brittle},
        std::pair{13U, MaterialResponse::Compressible},
        std::pair{17U, MaterialResponse::Ductile},
    };
    for (std::size_t index = 0; index < expected_materials.size(); ++index) {
        NINHO_SIM_REQUIRE(product.materials.materials[index].id.value()
            == expected_materials[index].first);
        NINHO_SIM_REQUIRE(product.materials.materials[index].response
            == expected_materials[index].second);
    }
    NINHO_SIM_REQUIRE(product.archetypes.abilities.size() == 5U);
    NINHO_SIM_REQUIRE(product.archetypes.birds.size() == 5U);
    NINHO_SIM_REQUIRE((product.farm.bird_queue
        == std::vector<BirdArchetypeId>{BirdArchetypeId{2}, BirdArchetypeId{3},
            BirdArchetypeId{4}, BirdArchetypeId{5}}));
    NINHO_SIM_REQUIRE((product.orbital.bird_queue
        == std::vector<BirdArchetypeId>{BirdArchetypeId{1}, BirdArchetypeId{1},
            BirdArchetypeId{1}}));
    NINHO_SIM_REQUIRE(product.campaign.default_world_id == "earth");
    NINHO_SIM_REQUIRE((product.campaign.world_order
        == std::vector<std::string>{"earth", "orbital"}));
    NINHO_SIM_REQUIRE(std::ranges::none_of(product.farm.bird_queue,
        [](BirdArchetypeId id) { return id.value() > 5U; }));
    const auto farm_session = SimulationSession::create(
        product.materials, product.archetypes, product.farm);
    const auto orbital_session = SimulationSession::create(
        product.materials, product.archetypes, product.orbital);
    NINHO_SIM_REQUIRE(farm_session.ok());
    NINHO_SIM_REQUIRE(orbital_session.ok());
    const auto farm_hash = farm_session.value->canonical_hash_v3();
    const auto orbital_hash = orbital_session.value->canonical_hash_v3();
    NINHO_SIM_REQUIRE(farm_hash != 0U && orbital_hash != 0U);
    for (int iteration = 0; iteration < 50; ++iteration) {
        NINHO_SIM_REQUIRE(farm_session.value->canonical_hash_v3() == farm_hash);
        NINHO_SIM_REQUIRE(orbital_session.value->canonical_hash_v3() == orbital_hash);
    }
}

NINHO_SIM_TEST("product v2 levels align every visual bound with recursive collider aabb")
{
    constexpr double tolerance_m = 1.0e-6;
    const LevelManifest& farm = load_product().farm;
    std::ostringstream mismatches;
    std::size_t mismatch_count = 0U;
    for (const BodyDefinition& definition : farm.bodies) {
        const auto leaves = body_local_leaf_bounds(definition);
        NINHO_SIM_REQUIRE(!leaves.empty());
        TestAabb collider_bounds;
        for (const TestLeaf& leaf : leaves) {
            for (std::size_t axis = 0; axis < 3U; ++axis) {
                collider_bounds.minimum[axis] = std::min(
                    collider_bounds.minimum[axis], leaf.bounds.minimum[axis]);
                collider_bounds.maximum[axis] = std::max(
                    collider_bounds.maximum[axis], leaf.bounds.maximum[axis]);
            }
        }
        for (std::size_t axis = 0; axis < 3U; ++axis) {
            const double collider_size = collider_bounds.maximum[axis]
                - collider_bounds.minimum[axis];
            const double visual_size = definition.visual.bounds_m[axis];
            if (std::abs(visual_size - collider_size) <= tolerance_m) continue;
            mismatches << std::setprecision(17)
                       << " body=" << definition.body_id << " axis=" << axis
                       << " visual=" << visual_size
                       << " collider=" << collider_size << ';';
            ++mismatch_count;
        }
    }
    if (mismatch_count != 0U) {
        throw std::runtime_error("visual/collider AABB mismatches (tolerance 1e-6 m):"
            + mismatches.str());
    }
}

NINHO_SIM_TEST("product v2 levels freeze the complete farm inventory and physics")
{
    const LoadedProduct product = load_product();
    const LevelManifest& farm = product.farm;
    NINHO_SIM_REQUIRE(farm.bodies.size() == 106U);
    NINHO_SIM_REQUIRE(std::ranges::count(farm.bodies, BodyType::Dynamic,
        &BodyDefinition::body_type) == 86);
    NINHO_SIM_REQUIRE(farm.joints.size() == 49U);
    NINHO_SIM_REQUIRE(farm.assemblies.size() == 8U);
    NINHO_SIM_REQUIRE(farm.triggers.size() == 2U);
    NINHO_SIM_REQUIRE(farm.objectives.size() == 4U);
    const auto& world = std::get<UniformWorldDefinition>(farm.world);
    NINHO_SIM_REQUIRE((world.acceleration_m_s2 == std::array{0.0, -9.81, 0.0}));
    NINHO_SIM_REQUIRE((world.bounds_min_m == std::array{-24.0, -12.0, -12.0}));
    NINHO_SIM_REQUIRE((world.bounds_max_m == std::array{48.0, 32.0, 12.0}));
    NINHO_SIM_REQUIRE(farm.slingshot.spring_constant_n_m == 5200.0);
    NINHO_SIM_REQUIRE(farm.slingshot.energy_efficiency == 0.90);
    NINHO_SIM_REQUIRE(farm.slingshot.maximum_extension_m == 4.25);
    NINHO_SIM_REQUIRE(farm.slingshot.minimum_extension_m == 0.20);
    NINHO_SIM_REQUIRE((farm.scoring.star_thresholds
        == std::array<std::uint32_t, 3>{1U, 38000U, 50000U}));
    const BodyDefinition& side_bale = body(farm, 23U);
    NINHO_SIM_REQUIRE(side_bale.shape.type == ShapeType::Compound);
    NINHO_SIM_REQUIRE(side_bale.shape.children.size() == 2U);
    NINHO_SIM_REQUIRE(side_bale.density_kg_m3 == 109.77468250716919);
    const auto side_bale_properties = detail::shape_mass_properties(side_bale.shape);
    NINHO_SIM_REQUIRE(std::abs(side_bale_properties.volume_m3 - 0.644424)
        < 1.0e-12);
    NINHO_SIM_REQUIRE(std::abs(side_bale_properties.volume_m3
        * side_bale.density_kg_m3 - 70.74144) < 1.0e-9);
    for (const double component : side_bale_properties.center_of_mass_m) {
        NINHO_SIM_REQUIRE(std::abs(component) < 1.0e-12);
    }
    NINHO_SIM_REQUIRE((side_bale.visual.bounds_m
        == std::array{1.16, 0.84, 0.7707902298850575}));
    const BodyDefinition& silo_output = body(farm, 45U);
    const auto silo_output_properties = detail::shape_mass_properties(silo_output.shape);
    NINHO_SIM_REQUIRE(std::abs(silo_output_properties.volume_m3 - 0.12)
        < 1.0e-12);
    NINHO_SIM_REQUIRE(std::abs(silo_output_properties.volume_m3
        * silo_output.density_kg_m3 - 62.4) < 1.0e-9);
    NINHO_SIM_REQUIRE(std::abs(silo_output_properties.center_of_mass_m[0]
        - 0.885588235294118) < 1.0e-12);
    NINHO_SIM_REQUIRE(std::abs(silo_output_properties.center_of_mass_m[1]
        + 1.285) < 1.0e-12);
    NINHO_SIM_REQUIRE(std::abs(silo_output_properties.center_of_mass_m[2]
        - 0.0) < 1.0e-12);
    NINHO_SIM_REQUIRE((silo_output.shape.children[2].local_rotation_xyzw
        == std::array{0.008726535498373935, 0.0, 0.0,
            0.9999619230641713}));
    NINHO_SIM_REQUIRE((silo_output.visual.bounds_m
        == std::array{3.5, 4.4125, 3.000098371642568}));
    const BodyDefinition& silo_base = body(farm, 36U);
    NINHO_SIM_REQUIRE(silo_base.shape.type == ShapeType::Compound);
    NINHO_SIM_REQUIRE(silo_base.shape.children.size() == 5U);
    const ShapeDefinition& pig_catch = silo_base.shape.children[4];
    NINHO_SIM_REQUIRE(pig_catch.type == ShapeType::Box);
    NINHO_SIM_REQUIRE((pig_catch.half_extents_m
        == std::array{0.05, 0.31, 0.20}));
    NINHO_SIM_REQUIRE((pig_catch.local_position_m
        == std::array{-0.53, 1.14, -0.52}));
    NINHO_SIM_REQUIRE((pig_catch.local_rotation_xyzw
        == std::array{0.0, 0.0, 0.0, 1.0}));
    NINHO_SIM_REQUIRE((silo_base.visual.bounds_m
        == std::array{4.2, 1.6, 4.2}));
    const BodyDefinition& fuel_tank = body(farm, 58U);
    NINHO_SIM_REQUIRE(fuel_tank.shape.type == ShapeType::Compound);
    NINHO_SIM_REQUIRE(fuel_tank.shape.children.size() == 12U);
    for (const std::size_t index : {4U, 5U, 8U, 10U}) {
        NINHO_SIM_REQUIRE(
            fuel_tank.shape.children[index].local_position_m[1] == -0.785);
    }
    for (const std::size_t index : {6U, 7U, 9U, 11U}) {
        NINHO_SIM_REQUIRE(
            fuel_tank.shape.children[index].local_position_m[1] == 1.085);
    }
    const auto fuel_tank_properties = detail::shape_mass_properties(fuel_tank.shape);
    NINHO_SIM_REQUIRE(std::abs(fuel_tank_properties.volume_m3 - 0.006144)
        < 1.0e-12);
    NINHO_SIM_REQUIRE(std::abs(fuel_tank_properties.volume_m3
        * fuel_tank.density_kg_m3 - 47.9232) < 1.0e-9);
    NINHO_SIM_REQUIRE(std::abs(fuel_tank_properties.center_of_mass_m[0])
        < 1.0e-12);
    NINHO_SIM_REQUIRE(std::abs(fuel_tank_properties.center_of_mass_m[1] - 0.15)
        < 1.0e-12);
    NINHO_SIM_REQUIRE(std::abs(fuel_tank_properties.center_of_mass_m[2])
        < 1.0e-12);
    const BodyDefinition& pressure_tank = body(farm, 59U);
    NINHO_SIM_REQUIRE(pressure_tank.shape.type == ShapeType::Compound);
    NINHO_SIM_REQUIRE(pressure_tank.shape.children.size() == 13U);
    constexpr std::array<std::array<double, 3>, 12> pressure_authored_centers{{
        {-0.465, 0.0, -0.465}, {-0.465, 0.0, 0.465},
        {0.465, 0.0, -0.465}, {0.465, 0.0, 0.465},
        {0.0, -0.465, -0.465}, {0.0, -0.465, 0.465},
        {0.0, 0.465, -0.465}, {0.0, 0.465, 0.465},
        {-0.465, -0.465, 0.0}, {-0.465, 0.465, 0.0},
        {0.465, -0.465, 0.0}, {0.465, 0.465, 0.0},
    }};
    for (std::size_t child = 0U; child < pressure_authored_centers.size(); ++child) {
        for (std::size_t axis = 0U; axis < 3U; ++axis) {
            NINHO_SIM_REQUIRE(std::abs(pressure_tank.shape.children[child]
                    .local_position_m[axis]
                - pressure_authored_centers[child][axis]) < 1.0e-12);
        }
    }
    for (const std::size_t child : {4U, 5U, 6U, 7U}) {
        NINHO_SIM_REQUIRE((pressure_tank.shape.children[child].half_extents_m
            == std::array{0.2025, 0.01, 0.01}));
    }
    const ShapeDefinition& pressure_ballast = pressure_tank.shape.children[12];
    NINHO_SIM_REQUIRE(pressure_ballast.type == ShapeType::Box);
    NINHO_SIM_REQUIRE((pressure_ballast.half_extents_m
        == std::array{0.05, 0.05, 0.042}));
    NINHO_SIM_REQUIRE((pressure_ballast.local_position_m
        == std::array{-0.25, 0.0, 0.34}));
    NINHO_SIM_REQUIRE((pressure_ballast.local_rotation_xyzw
        == std::array{0.0, 0.0, 0.0, 1.0}));
    const auto pressure_tank_properties =
        detail::shape_mass_properties(pressure_tank.shape);
    NINHO_SIM_REQUIRE(std::abs(pressure_tank_properties.volume_m3 - 0.004512)
        < 1.0e-12);
    NINHO_SIM_REQUIRE(std::abs(pressure_tank_properties.volume_m3
        * pressure_tank.density_kg_m3 - 35.1936) < 1.0e-9);
    NINHO_SIM_REQUIRE(std::abs(pressure_tank_properties.center_of_mass_m[0]
        + 0.04654255319148936) < 1.0e-12);
    NINHO_SIM_REQUIRE(std::abs(pressure_tank_properties.center_of_mass_m[1])
        < 1.0e-12);
    NINHO_SIM_REQUIRE(std::abs(pressure_tank_properties.center_of_mass_m[2]
        - 0.06329787234042553) < 1.0e-12);
    NINHO_SIM_REQUIRE((pressure_tank.visual.bounds_m
        == std::array{0.95, 0.96, 0.95}));
    std::size_t authored_primitive_count = 0U;
    for (const BodyDefinition& definition : farm.bodies) {
        authored_primitive_count += body_local_leaf_bounds(definition).size();
    }
    NINHO_SIM_REQUIRE(authored_primitive_count == 256U);
    NINHO_SIM_REQUIRE(authored_primitive_count <= 256U);
    for (const auto& definition : farm.bodies) {
        if (definition.body_type == BodyType::Dynamic) {
            NINHO_SIM_REQUIRE(definition.affected_by_world_gravity);
            if (definition.material_id) {
                const auto material = std::ranges::find(product.materials.materials,
                    *definition.material_id, &MaterialDefinition::id);
                NINHO_SIM_REQUIRE(material != product.materials.materials.end());
                if (definition.body_id != 23U && definition.body_id != 74U) {
                    NINHO_SIM_REQUIRE(
                        definition.density_kg_m3 == material->density_kg_m3);
                }
            }
        }
    }
    const auto require_stable_proxy = [&](const auto& self,
                                          const ShapeDefinition& shape) -> void {
        if (shape.type == ShapeType::Box) {
            NINHO_SIM_REQUIRE(std::ranges::all_of(shape.half_extents_m,
                [](double extent) { return std::isfinite(extent) && extent >= 0.01; }));
        } else if (shape.type == ShapeType::Sphere) {
            NINHO_SIM_REQUIRE(std::isfinite(shape.radius_m) && shape.radius_m > 0.0);
        } else if (shape.type == ShapeType::Capsule) {
            NINHO_SIM_REQUIRE(std::isfinite(shape.radius_m) && shape.radius_m > 0.0);
            NINHO_SIM_REQUIRE(
                std::isfinite(shape.half_height_m) && shape.half_height_m > 0.0);
        } else if (shape.type == ShapeType::ConvexHull) {
            for (const auto& vertex : shape.vertices_m) {
                NINHO_SIM_REQUIRE(std::ranges::all_of(
                    vertex, [](double value) { return std::isfinite(value); }));
            }
        } else {
            for (const ShapeDefinition& child : shape.children) self(self, child);
        }
    };
    for (const BodyDefinition& definition : farm.bodies) {
        require_stable_proxy(require_stable_proxy, definition.shape);
        if (!definition.fracture_pattern) continue;
        for (const auto& fragment : definition.fracture_pattern->physical_fragments) {
            require_stable_proxy(require_stable_proxy, fragment.shape);
        }
    }
    const auto pair_key = [](std::uint32_t a, std::uint32_t b) {
        const auto [low, high] = std::minmax(a, b);
        return (static_cast<std::uint64_t>(low) << 32U) | high;
    };
    const std::unordered_set<std::uint64_t> deliberate_interlocks{
        // Encaixe estrutural medido por SAT: travessa/gaiola e rampa/suportes.
        pair_key(15U, 16U),
        // As três pás penetram 0,05 m no hub esférico, sem se tocarem entre si.
        pair_key(48U, 49U), pair_key(48U, 50U), pair_key(48U, 51U)};
    const auto bodies_strictly_overlap = [&](std::uint32_t a_id,
                                             std::uint32_t b_id) {
        const auto a_bounds = body_leaf_bounds(body(farm, a_id));
        const auto b_bounds = body_leaf_bounds(body(farm, b_id));
        for (const TestLeaf& a_leaf : a_bounds) {
            for (const TestLeaf& b_leaf : b_bounds) {
                if (strictly_overlaps(a_leaf, b_leaf)) return true;
            }
        }
        return false;
    };
    std::ostringstream stale_interlocks;
    std::size_t stale_interlock_count = 0U;
    for (std::uint64_t key : deliberate_interlocks) {
        const auto a_id = static_cast<std::uint32_t>(key >> 32U);
        const auto b_id = static_cast<std::uint32_t>(key);
        if (bodies_strictly_overlap(a_id, b_id)) continue;
        stale_interlocks << ' ' << a_id << '-' << b_id;
        ++stale_interlock_count;
    }
    if (stale_interlock_count != 0U) {
        throw std::runtime_error("stale deliberate interlock allowlist entries:"
            + stale_interlocks.str());
    }
    std::ostringstream unexpected_overlaps;
    std::size_t unexpected_overlap_count = 0U;
    for (std::size_t left = 0; left < farm.bodies.size(); ++left) {
        for (std::size_t right = left + 1U; right < farm.bodies.size(); ++right) {
            const BodyDefinition& a = farm.bodies[left];
            const BodyDefinition& b = farm.bodies[right];
            if (deliberate_interlocks.contains(pair_key(a.body_id, b.body_id))) continue;
            const auto a_bounds = body_leaf_bounds(a);
            const auto b_bounds = body_leaf_bounds(b);
            bool pair_overlaps = false;
            std::size_t overlapping_a_leaf = 0U;
            std::size_t overlapping_b_leaf = 0U;
            double minimum_aabb_depth_m = std::numeric_limits<double>::infinity();
            for (std::size_t a_index = 0; a_index < a_bounds.size(); ++a_index) {
                for (std::size_t b_index = 0; b_index < b_bounds.size(); ++b_index) {
                    const TestLeaf& a_leaf = a_bounds[a_index];
                    const TestLeaf& b_leaf = b_bounds[b_index];
                    if (strictly_overlaps(a_leaf, b_leaf)) {
                        pair_overlaps = true;
                        overlapping_a_leaf = a_index;
                        overlapping_b_leaf = b_index;
                        for (std::size_t axis = 0; axis < 3U; ++axis) {
                            minimum_aabb_depth_m = std::min(minimum_aabb_depth_m,
                                std::min(a_leaf.bounds.maximum[axis],
                                    b_leaf.bounds.maximum[axis])
                                    - std::max(a_leaf.bounds.minimum[axis],
                                        b_leaf.bounds.minimum[axis]));
                        }
                        break;
                    }
                }
                if (pair_overlaps) break;
            }
            if (pair_overlaps) {
                unexpected_overlaps << ' ' << a.body_id << '-' << b.body_id
                    << "[leaves=" << overlapping_a_leaf << '/'
                    << overlapping_b_leaf << ",aabb_depth_m="
                    << std::setprecision(17) << minimum_aabb_depth_m << ']';
                ++unexpected_overlap_count;
            }
        }
    }
    if (unexpected_overlap_count != 0U) {
        throw std::runtime_error("unexpected initial overlaps:"
            + unexpected_overlaps.str());
    }
    for (const AssemblyDefinition& assembly : farm.assemblies) {
        std::unordered_map<std::uint32_t, std::vector<std::uint32_t>> adjacency;
        for (std::uint32_t body_id : assembly.body_ids) adjacency[body_id];
        for (JointId joint_id : assembly.joint_ids) {
            const auto joint = std::ranges::find(farm.joints, joint_id,
                &JointDefinition::id);
            NINHO_SIM_REQUIRE(joint != farm.joints.end());
            NINHO_SIM_REQUIRE(adjacency.contains(joint->body_a_id));
            NINHO_SIM_REQUIRE(adjacency.contains(joint->body_b_id));
            adjacency[joint->body_a_id].push_back(joint->body_b_id);
            adjacency[joint->body_b_id].push_back(joint->body_a_id);
        }
        std::unordered_set<std::uint32_t> visited;
        std::vector<std::uint32_t> pending{assembly.body_ids.front()};
        while (!pending.empty()) {
            const std::uint32_t current = pending.back();
            pending.pop_back();
            if (!visited.insert(current).second) continue;
            for (std::uint32_t neighbor : adjacency[current]) pending.push_back(neighbor);
        }
        NINHO_SIM_REQUIRE(visited.size() == assembly.body_ids.size());
    }
    const std::array pig_entities{2001U, 2002U, 2003U, 2004U};
    for (std::size_t index = 0; index < pig_entities.size(); ++index) {
        const auto found = std::ranges::find(farm.bodies, EntityId{pig_entities[index]},
            &BodyDefinition::entity_id);
        NINHO_SIM_REQUIRE(found != farm.bodies.end());
        NINHO_SIM_REQUIRE(found->enemy_archetype_id == EnemyArchetypeId{2});
        NINHO_SIM_REQUIRE(farm.objectives[index].target_entity_id
            == EntityId{pig_entities[index]});
        NINHO_SIM_REQUIRE(found->shape.type == ShapeType::Compound);
        NINHO_SIM_REQUIRE(found->shape.children.size() == 2U);
        NINHO_SIM_REQUIRE(found->shape.children[0].type == ShapeType::Capsule);
        NINHO_SIM_REQUIRE(found->shape.children[1].type == ShapeType::Sphere);
        const auto properties = detail::shape_mass_properties(found->shape);
        NINHO_SIM_REQUIRE(properties.volume_m3 > 0.0);
        NINHO_SIM_REQUIRE(properties.center_of_mass_m[0] > 0.01);
        NINHO_SIM_REQUIRE(std::isfinite(properties.center_of_mass_m[0]));
        NINHO_SIM_REQUIRE(std::abs(
            properties.volume_m3 * found->density_kg_m3 - 65.0) < 0.001);
        NINHO_SIM_REQUIRE(found->shape.children[0].radius_m > 0.0);
        NINHO_SIM_REQUIRE(found->shape.children[0].half_height_m > 0.0);
        NINHO_SIM_REQUIRE(found->shape.children[1].radius_m > 0.0);

        const auto physics_transform = [](const ShapeDefinition& shape) {
            return ninho::physics::Transform{
                {static_cast<float>(shape.local_position_m[0]),
                    static_cast<float>(shape.local_position_m[1]),
                    static_cast<float>(shape.local_position_m[2])},
                {static_cast<float>(shape.local_rotation_xyzw[0]),
                    static_cast<float>(shape.local_rotation_xyzw[1]),
                    static_cast<float>(shape.local_rotation_xyzw[2]),
                    static_cast<float>(shape.local_rotation_xyzw[3])}};
        };
        ninho::physics::WorldConfig config;
        config.gravity = ninho::physics::UniformGravityConfig{{}};
        config.bounds = ninho::physics::NoWorldBounds{};
        ninho::physics::PhysicsWorld physics{config};
        ninho::physics::BodyDesc pig;
        pig.type = ninho::physics::BodyType::Dynamic;
        pig.shapes.push_back({
            .geometry = ninho::physics::CompoundShape{{
                ninho::physics::CapsuleShape{
                    static_cast<float>(found->shape.children[0].half_height_m),
                    static_cast<float>(found->shape.children[0].radius_m),
                    physics_transform(found->shape.children[0])},
                ninho::physics::SphereShape{
                    static_cast<float>(found->shape.children[1].radius_m),
                    physics_transform(found->shape.children[1])},
            }},
            .density = static_cast<float>(found->density_kg_m3),
        });
        const auto created_pig = physics.create_body(pig);
        NINHO_SIM_REQUIRE(static_cast<bool>(created_pig));
        NINHO_SIM_REQUIRE(physics.commit_pending_initial_state().ok());
        const auto inertia = ninho::physics::detail::PhysicsWorldTestFacade::
            local_inertia(physics, created_pig.value);
        NINHO_SIM_REQUIRE(std::ranges::all_of(
            inertia, [](float value) { return std::isfinite(value); }));
        NINHO_SIM_REQUIRE(inertia[0] > 0.0F && inertia[4] > 0.0F
            && inertia[8] > 0.0F);
        NINHO_SIM_REQUIRE(ninho::physics::detail::PhysicsWorldTestFacade::
            local_center(physics, created_pig.value).x > 0.01F);
    }
    for (const auto& joint : farm.joints) {
        if (joint.kind != JointKind::SteelDuctile) continue;
        const auto ductile = [&](std::uint32_t body_id) {
            return body(farm, body_id).material_id == MaterialId{17};
        };
        NINHO_SIM_REQUIRE(ductile(joint.body_a_id) != ductile(joint.body_b_id));
    }
    auto created = SimulationSession::create(
        product.materials, product.archetypes, product.farm);
    NINHO_SIM_REQUIRE(created.ok());
}

NINHO_SIM_TEST("product v2 levels preserve normative farm landmarks")
{
    const LevelManifest& farm = load_product().farm;
    const std::array normative_landmarks{
        std::pair{2U, std::array{2.6, 0.55, -3.4}},
        std::pair{3U, std::array{7.4, 1.05, 1.8}},
        std::pair{4U, std::array{13.0, 1.4, 0.0}},
        std::pair{5U, std::array{17.0, 0.55, -2.5}},
        std::pair{14U, std::array{6.7, 2.2, 0.0}},
        std::pair{15U, std::array{4.4, 4.2, 0.0}},
        std::pair{16U, std::array{5.8, 4.0, 0.0}},
        std::pair{20U, std::array{8.8, 1.8, 0.0}},
        std::pair{21U, std::array{7.8, 3.4, -0.75}},
        std::pair{22U, std::array{7.8, 3.4, 0.0}},
        std::pair{23U, std::array{7.8, 3.4, 0.75}},
        std::pair{24U, std::array{10.5, 0.8, -1.0}},
        std::pair{25U, std::array{10.5, 0.8, 1.0}},
        std::pair{26U, std::array{6.8, 3.2, 0.0}},
        std::pair{58U, std::array{16.2, 1.1, -1.6}},
        std::pair{59U, std::array{17.6, 1.0, -3.3}},
    };
    for (const auto& [body_id, expected_position] : normative_landmarks) {
        NINHO_SIM_REQUIRE(body(farm, body_id).transform.position_m
            == expected_position);
    }
}

NINHO_SIM_TEST("product v2 levels preserve normative farm joint limits")
{
    const LevelManifest& farm = load_product().farm;
    for (const auto& joint : farm.joints) {
        const std::pair expected_limits = [&] {
            switch (joint.kind) {
            case JointKind::PineFit: return std::pair{5500.0, 900.0};
            case JointKind::GlassClamp: return std::pair{2200.0, 350.0};
            case JointKind::Mortar: return std::pair{1400.0, 160.0};
            case JointKind::StrawBind: return std::pair{800.0, 90.0};
            case JointKind::SteelDuctile: return std::pair{7500.0, 900.0};
            }
            throw std::runtime_error("unknown normative joint kind");
        }();
        NINHO_SIM_REQUIRE(joint.force_limit_n == expected_limits.first);
        NINHO_SIM_REQUIRE(joint.torque_limit_nm == expected_limits.second);
    }
}

NINHO_SIM_TEST("product v2 levels keep the farm intact for 120 idle ticks")
{
    LoadedProduct product = load_product();
    auto created = SimulationSession::create(
        product.materials, product.archetypes, product.farm);
    NINHO_SIM_REQUIRE(created.ok());
    auto session = std::move(created.value);
    for (std::uint32_t tick = 0; tick < 120U; ++tick) {
        const SessionStatus status = session->tick();
        if (!status.ok()) {
            throw std::runtime_error("idle tick " + std::to_string(tick)
                + " faulted at " + status.error.pointer + ": "
                + status.error.message);
        }
        for (const DomainEvent& event : session->events()) {
            if (!is_idle_breakage_event(event.kind)) continue;
            throw std::runtime_error("idle breakage at tick "
                + std::to_string(tick) + ", kind "
                + std::to_string(static_cast<unsigned>(event.kind))
                + ", entity " + std::to_string(event.entity_id.value())
                + ", affected "
                + std::to_string(event.affected_entity_id.value())
                + ", joint " + std::to_string(event.joint_id.value())
                + ", ratio " + std::to_string(event.joint_load_ratio));
        }
    }
    NINHO_SIM_REQUIRE(std::ranges::all_of(
        session->structural_joints(), &StructuralJointSnapshot::active));
    NINHO_SIM_REQUIRE(std::ranges::none_of(
        session->objective_target_statuses(), &ObjectiveTargetStatus::neutralized));
}

NINHO_SIM_TEST("product v2 levels freeze farm layout hash and semantic reorder")
{
    const json farm = parsed(farm_path);
    const json fixture = parsed(fixture_path);
    require_exact_keys(fixture,
        {"schema_version", "level_id", "layout_hash", "counts", "projection"});
    NINHO_SIM_REQUIRE(fixture.at("schema_version") == 1);
    NINHO_SIM_REQUIRE(fixture.at("level_id") == farm.at("id"));
    NINHO_SIM_REQUIRE(fixture.at("projection") == layout_projection(farm));
    NINHO_SIM_REQUIRE(fixture.at("layout_hash") == layout_hash(farm));
    const json expected_counts{
        {"bodies", 106}, {"dynamic_bodies", 86}, {"joints", 49},
        {"assemblies", 8}, {"triggers", 2}, {"objectives", 4}};
    NINHO_SIM_REQUIRE(fixture.at("counts") == expected_counts);

    json integer_spelling = farm;
    integer_spelling["world"]["bounds"]["max_m"][0] = 48;
    integer_spelling["slingshot"]["energy_efficiency"] = 1;
    json real_spelling = farm;
    real_spelling["slingshot"]["energy_efficiency"] = 1.0;
    NINHO_SIM_REQUIRE(layout_hash(integer_spelling) == layout_hash(real_spelling));
    json negative_zero = farm;
    negative_zero["world"]["acceleration_m_s2"][0] = -0.0;
    json positive_zero = farm;
    positive_zero["world"]["acceleration_m_s2"][0] = 0;
    NINHO_SIM_REQUIRE(layout_hash(negative_zero) == layout_hash(positive_zero));

    const LoadedProduct product = load_product();
    const auto require_valid_mutation = [&](const json& changed) {
        const auto typed = parse_level_manifest_v2(changed.dump());
        if (!typed.ok()) {
            throw std::runtime_error("invalid semantic hash mutation at "
                + typed.error.pointer + ": " + typed.error.message);
        }
        NINHO_SIM_REQUIRE(make_product_v2_content_bundle(product.materials,
            product.archetypes, product.campaign, typed.value).ok());
        NINHO_SIM_REQUIRE(layout_hash(changed) != layout_hash(farm));
    };
    json world_changed = farm;
    world_changed["world"]["acceleration_m_s2"][1] = -9.80;
    require_valid_mutation(world_changed);
    json launcher_changed = farm;
    launcher_changed["slingshot"]["spring_constant_n_m"] = 5100.0;
    require_valid_mutation(launcher_changed);
    json joint_changed = farm;
    joint_changed["joints"][0]["force_limit_n"] = 5400.0;
    require_valid_mutation(joint_changed);
    json trigger_changed = farm;
    trigger_changed["triggers"][0]["damage_threshold"] = 41.0;
    require_valid_mutation(trigger_changed);
    json objective_changed = farm;
    std::swap(objective_changed["objectives"][0]["target_entity_id"],
        objective_changed["objectives"][1]["target_entity_id"]);
    require_valid_mutation(objective_changed);
    json transform_changed = farm;
    transform_changed["bodies"][61]["transform"]["position_m"][0] = 3.1;
    require_valid_mutation(transform_changed);
    json shape_changed = farm;
    shape_changed["bodies"][11]["shape"]["half_extents_m"][0] = 1.4;
    require_valid_mutation(shape_changed);

    json reordered = farm;
    for (const auto key : {"bodies", "joints", "assemblies", "triggers", "objectives"})
        std::reverse(reordered[key].begin(), reordered[key].end());
    std::reverse(reordered["free_body_ids"].begin(), reordered["free_body_ids"].end());
    for (auto& assembly : reordered["assemblies"]) {
        std::reverse(assembly["body_ids"].begin(), assembly["body_ids"].end());
        std::reverse(assembly["joint_ids"].begin(), assembly["joint_ids"].end());
    }
    NINHO_SIM_REQUIRE(layout_hash(reordered) == layout_hash(farm));
}

NINHO_SIM_TEST("product v2 levels isolate the spherical pressure hammer probe")
{
    const LevelManifest& farm = load_product().farm;
    const auto hammer = std::ranges::find(
        farm.bodies, 74U, &BodyDefinition::body_id);
    NINHO_SIM_REQUIRE(hammer != farm.bodies.end());
    NINHO_SIM_REQUIRE(hammer->entity_id == EntityId{3074U});
    NINHO_SIM_REQUIRE(hammer->part_id == PartId{1U});
    NINHO_SIM_REQUIRE(hammer->body_type == BodyType::Dynamic);
    NINHO_SIM_REQUIRE(hammer->affected_by_world_gravity);
    NINHO_SIM_REQUIRE(hammer->material_id == MaterialId{17U});
    NINHO_SIM_REQUIRE(hammer->density_kg_m3 == 520.0);
    NINHO_SIM_REQUIRE(!hammer->assembly_id.has_value());
    NINHO_SIM_REQUIRE(!hammer->fracture_pattern.has_value());
    NINHO_SIM_REQUIRE((hammer->transform.position_m
        == std::array{18.55, 1.5, -3.55}));
    NINHO_SIM_REQUIRE((hammer->transform.rotation_xyzw
        == std::array{0.0, 0.0, 0.0, 1.0}));
    NINHO_SIM_REQUIRE(hammer->shape.type == ShapeType::Sphere);
    NINHO_SIM_REQUIRE(hammer->shape.radius_m == 0.42);
    const auto hammer_properties = detail::shape_mass_properties(hammer->shape);
    NINHO_SIM_REQUIRE(std::abs(hammer_properties.volume_m3
        * hammer->density_kg_m3 - 161.3763261199513) < 1.0e-9);
    NINHO_SIM_REQUIRE(hammer->visual.asset_id == "KIT_Farm_Metal");
    NINHO_SIM_REQUIRE((hammer->visual.bounds_m
        == std::array{0.84, 0.84, 0.84}));

    const auto pedestal = std::ranges::find(
        farm.bodies, 75U, &BodyDefinition::body_id);
    NINHO_SIM_REQUIRE(pedestal != farm.bodies.end());
    NINHO_SIM_REQUIRE(pedestal->entity_id == EntityId{3075U});
    NINHO_SIM_REQUIRE(pedestal->body_type == BodyType::Static);
    NINHO_SIM_REQUIRE(!pedestal->affected_by_world_gravity);
    NINHO_SIM_REQUIRE(pedestal->material_id == MaterialId{17U});
    NINHO_SIM_REQUIRE(pedestal->density_kg_m3 == 0.0);
    NINHO_SIM_REQUIRE(!pedestal->assembly_id.has_value());
    NINHO_SIM_REQUIRE((pedestal->transform.position_m
        == std::array{18.55, 1.03, -3.55}));
    NINHO_SIM_REQUIRE(pedestal->shape.type == ShapeType::Box);
    NINHO_SIM_REQUIRE((pedestal->shape.half_extents_m
        == std::array{0.20, 0.05, 0.20}));
    NINHO_SIM_REQUIRE((pedestal->visual.bounds_m
        == std::array{0.40, 0.10, 0.40}));
    NINHO_SIM_REQUIRE(std::ranges::find(farm.free_body_ids, 74U)
        != farm.free_body_ids.end());
    NINHO_SIM_REQUIRE(std::ranges::find(farm.free_body_ids, 75U)
        != farm.free_body_ids.end());
    NINHO_SIM_REQUIRE(std::abs((hammer->transform.position_m[1]
            - hammer->shape.radius_m)
        - (pedestal->transform.position_m[1]
            + pedestal->shape.half_extents_m[1])) < 1.0e-12);
}

NINHO_SIM_TEST("product v2 levels isolate the thirty panel ballistic rack")
{
    const LoadedProduct product = load_product();
    const LevelManifest& farm = product.farm;
    const BodyDefinition& frame = body(farm, 76U);
    NINHO_SIM_REQUIRE(frame.entity_id == EntityId{3076U});
    NINHO_SIM_REQUIRE(frame.body_type == BodyType::Static);
    NINHO_SIM_REQUIRE(!frame.affected_by_world_gravity);
    NINHO_SIM_REQUIRE(frame.material_id == MaterialId{17U});
    NINHO_SIM_REQUIRE(frame.density_kg_m3 == 0.0);
    NINHO_SIM_REQUIRE((frame.transform.position_m == std::array{
        21.2484082701803, 2.31216908527308, -4.33094896033987}));
    NINHO_SIM_REQUIRE((frame.transform.rotation_xyzw == std::array{
        0.0, 0.140389383769565, 0.0, 0.990096369513999}));
    NINHO_SIM_REQUIRE(frame.shape.type == ShapeType::Compound);
    NINHO_SIM_REQUIRE(frame.shape.children.size() == 4U);
    NINHO_SIM_REQUIRE((frame.shape.children[0].half_extents_m
        == std::array{0.01, 0.23, 0.42}));
    NINHO_SIM_REQUIRE((frame.shape.children[0].local_position_m
        == std::array{0.22, 0.0, 0.0}));
    NINHO_SIM_REQUIRE((frame.shape.children[1].half_extents_m
        == std::array{0.12, 0.01, 0.42}));
    NINHO_SIM_REQUIRE((frame.shape.children[1].local_position_m
        == std::array{0.11, -0.19, 0.0}));
    NINHO_SIM_REQUIRE((frame.shape.children[2].half_extents_m
        == std::array{0.12, 0.23, 0.01}));
    NINHO_SIM_REQUIRE((frame.shape.children[2].local_position_m
        == std::array{0.11, 0.0, -0.36}));
    NINHO_SIM_REQUIRE((frame.shape.children[3].half_extents_m
        == std::array{0.12, 0.23, 0.01}));
    NINHO_SIM_REQUIRE((frame.shape.children[3].local_position_m
        == std::array{0.11, 0.0, 0.36}));
    NINHO_SIM_REQUIRE((frame.visual.bounds_m
        == std::array{0.24, 0.46, 0.84}));
    NINHO_SIM_REQUIRE(std::ranges::find(farm.free_body_ids, 76U)
        != farm.free_body_ids.end());

    constexpr std::array center{
        21.2484082701803, 2.31216908527308, -4.33094896033987};
    constexpr std::array local_z{0.277998038377107, 0.0, 0.960581641849604};
    for (std::uint32_t index = 0U; index < 30U; ++index) {
        const BodyDefinition& panel = body(farm, 77U + index);
        const double row_offset = (static_cast<double>(index / 5U) - 2.5) * 0.06;
        const double column_offset =
            (static_cast<double>(index % 5U) - 2.0) * 0.15;
        const std::array expected_position{
            center[0] + local_z[0] * column_offset,
            center[1] + row_offset,
            center[2] + local_z[2] * column_offset};
        NINHO_SIM_REQUIRE(panel.entity_id == EntityId{3077U + index});
        NINHO_SIM_REQUIRE(panel.body_type == BodyType::Dynamic);
        NINHO_SIM_REQUIRE(panel.affected_by_world_gravity);
        NINHO_SIM_REQUIRE(panel.material_id == MaterialId{9U});
        NINHO_SIM_REQUIRE(panel.density_kg_m3 == 2450.0);
        for (std::size_t axis = 0U; axis < 3U; ++axis) {
            NINHO_SIM_REQUIRE(std::abs(panel.transform.position_m[axis]
                - expected_position[axis]) < 1.0e-12);
        }
        NINHO_SIM_REQUIRE((panel.transform.rotation_xyzw == std::array{
            0.0, 0.140389383769565, 0.0, 0.990096369513999}));
        NINHO_SIM_REQUIRE(panel.shape.type == ShapeType::Box);
        NINHO_SIM_REQUIRE((panel.shape.half_extents_m
            == std::array{0.01, 0.03, 0.05}));
        NINHO_SIM_REQUIRE((panel.visual.bounds_m
            == std::array{0.02, 0.06, 0.10}));
        NINHO_SIM_REQUIRE(panel.fracture_pattern.has_value());
        NINHO_SIM_REQUIRE(
            panel.fracture_pattern->physical_fragments.size() == 2U);
        for (std::size_t fragment = 0U; fragment < 2U; ++fragment) {
            const auto& piece = panel.fracture_pattern->physical_fragments[fragment];
            NINHO_SIM_REQUIRE(piece.shape.type == ShapeType::Box);
            NINHO_SIM_REQUIRE((piece.shape.half_extents_m
                == std::array{0.01, 0.03, 0.025}));
            NINHO_SIM_REQUIRE(piece.density_kg_m3 == 2450.0);
            NINHO_SIM_REQUIRE(std::abs(piece.local_transform.position_m[2]
                - (fragment == 0U ? -0.025 : 0.025)) < 1.0e-12);
        }
        const auto properties = detail::shape_mass_properties(panel.shape);
        NINHO_SIM_REQUIRE(std::abs(properties.volume_m3
            * panel.density_kg_m3 - 0.294) < 1.0e-12);
        NINHO_SIM_REQUIRE(std::ranges::find(farm.free_body_ids, panel.body_id)
            != farm.free_body_ids.end());
    }
    std::size_t physical_fragment_budget = 0U;
    for (const BodyDefinition& definition : farm.bodies) {
        if (definition.fracture_pattern) {
            physical_fragment_budget +=
                definition.fracture_pattern->physical_fragments.size();
        }
    }
    NINHO_SIM_REQUIRE(physical_fragment_budget == 68U);
    NINHO_SIM_REQUIRE(physical_fragment_budget <= 80U);

    auto created = SimulationSession::create(
        product.materials, product.archetypes, product.farm);
    NINHO_SIM_REQUIRE(created.ok());
    auto session = std::move(created.value);
    for (std::uint32_t tick = 0U; tick < 120U; ++tick) {
        NINHO_SIM_REQUIRE(session->tick().ok());
        NINHO_SIM_REQUIRE(session->events().empty());
    }
}

NINHO_SIM_TEST("product v2 levels preserve legacy orbital bodies and bytes")
{
    NINHO_SIM_REQUIRE(fnv1a64(read_file("game/data/materials/vertical_slice.materials.json"))
        == 16946741593610183880ULL);
    NINHO_SIM_REQUIRE(fnv1a64(read_file("game/data/archetypes/vertical_slice.archetypes.json"))
        == 3954763813230204496ULL);
    NINHO_SIM_REQUIRE(fnv1a64(read_file("game/data/levels/first_orbit.level.json"))
        == 1573200512146290868ULL);
    const auto legacy = parse_level_manifest_v1(
        read_file("game/data/levels/first_orbit.level.json"));
    const LoadedProduct product = load_product();
    NINHO_SIM_REQUIRE(legacy.ok());
    NINHO_SIM_REQUIRE(product.orbital.bodies.size() == 23U);
    NINHO_SIM_REQUIRE(product.orbital.joints.size() == legacy.value.joints.size());
    NINHO_SIM_REQUIRE(product.orbital.objectives.size() == legacy.value.objectives.size());
    for (std::uint32_t id = 1U; id <= 22U; ++id) {
        const BodyDefinition& before = body(legacy.value, id);
        const BodyDefinition& after = body(product.orbital, id);
        NINHO_SIM_REQUIRE(before.entity_id == after.entity_id);
        NINHO_SIM_REQUIRE(before.part_id == after.part_id);
        NINHO_SIM_REQUIRE(before.body_type == after.body_type);
        NINHO_SIM_REQUIRE(before.material_id == after.material_id);
        NINHO_SIM_REQUIRE(before.surface_id == after.surface_id);
        NINHO_SIM_REQUIRE(before.enemy_archetype_id == after.enemy_archetype_id);
        NINHO_SIM_REQUIRE(before.density_kg_m3 == after.density_kg_m3);
        NINHO_SIM_REQUIRE(before.transform.position_m == after.transform.position_m);
        NINHO_SIM_REQUIRE(before.transform.rotation_xyzw == after.transform.rotation_xyzw);
        NINHO_SIM_REQUIRE(before.shape == after.shape);
        NINHO_SIM_REQUIRE(before.visual.asset_id == after.visual.asset_id);
        NINHO_SIM_REQUIRE(before.visual.bounds_m == after.visual.bounds_m);
    }
    for (std::size_t index = 0; index < legacy.value.joints.size(); ++index) {
        const auto& before = legacy.value.joints[index];
        const auto& after = product.orbital.joints[index];
        NINHO_SIM_REQUIRE(before.id == after.id);
        NINHO_SIM_REQUIRE(before.assembly_id == after.assembly_id);
        NINHO_SIM_REQUIRE(before.kind == after.kind);
        NINHO_SIM_REQUIRE(before.body_a_id == after.body_a_id);
        NINHO_SIM_REQUIRE(before.body_b_id == after.body_b_id);
        NINHO_SIM_REQUIRE(before.force_limit_n == after.force_limit_n);
        NINHO_SIM_REQUIRE(before.torque_limit_nm == after.torque_limit_nm);
    }
    NINHO_SIM_REQUIRE(legacy.value.objectives.front().target_entity_id
        == product.orbital.objectives.front().target_entity_id);
    const auto& radial = std::get<RadialWorldDefinition>(product.orbital.world);
    NINHO_SIM_REQUIRE((radial.center_m == std::array{0.0, 0.0, 0.0}));
    NINHO_SIM_REQUIRE(radial.reference_radius_m == 10.0);
    NINHO_SIM_REQUIRE(radial.reference_acceleration_m_s2 == 9.0);
    NINHO_SIM_REQUIRE(radial.bounds_radius_m == 60.0);
    NINHO_SIM_REQUIRE(product.orbital.slingshot.spring_constant_n_m == 2200.0);
    NINHO_SIM_REQUIRE(body(product.orbital, 23).body_type == BodyType::Static);
    NINHO_SIM_REQUIRE(!body(product.orbital, 23).affected_by_world_gravity);
}

NINHO_SIM_TEST("product v2 levels close asset and feedback registries")
{
    const std::string assets_source = read_file(assets_path);
    const auto validated_assets = validate_assets(assets_source);
    NINHO_SIM_REQUIRE(validated_assets.ok());
    const json& assets = validated_assets.value;
    require_exact_keys(assets, {"schema_version", "assets"});
    NINHO_SIM_REQUIRE(assets.at("schema_version") == 2);
    std::unordered_map<std::string, json> registry;
    for (const auto& asset : assets.at("assets")) {
        require_exact_keys(asset, {"id", "kind", "status", "resource_path",
            "node_path", "presentation_only"});
        const std::string id = asset.at("id");
        NINHO_SIM_REQUIRE(registry.emplace(id, asset).second);
        const std::string resource_path = asset.at("resource_path");
        NINHO_SIM_REQUIRE(resource_path.starts_with("res://assets/product_v2/")
            || resource_path.starts_with("res://assets/vertical_slice/"));
        NINHO_SIM_REQUIRE(resource_path.find("..") == std::string::npos);
        NINHO_SIM_REQUIRE(asset.at("status") == "planned"
            || asset.at("status") == "required");
        if (id.starts_with("FRAG_") || id == "KIT_Farm_Metal")
            NINHO_SIM_REQUIRE(!asset.at("node_path").get<std::string>().empty());
    }
    NINHO_SIM_REQUIRE(registry.contains("CHR_BlueChild"));
    NINHO_SIM_REQUIRE(registry.contains("KIT_Farm_Metal"));
    NINHO_SIM_REQUIRE(registry.contains("DEV_SlingshotFarm"));
    NINHO_SIM_REQUIRE(registry.contains("DEV_SlingshotOrbital"));
    const auto archetypes = parse_archetype_catalog_v2(read_file(archetypes_path));
    NINHO_SIM_REQUIRE(archetypes.ok());
    for (const auto& presentation_id : archetypes.value.presentation_ids)
        NINHO_SIM_REQUIRE(registry.contains(presentation_id));
    for (const auto path : {farm_path, orbital_path}) {
        const json level = parsed(path);
        NINHO_SIM_REQUIRE(registry.contains(level.at("slingshot").at("asset_id")));
        for (const auto& physical : level.at("bodies")) {
            const std::string visual = physical.at("visual").at("asset_id");
            NINHO_SIM_REQUIRE(registry.contains(visual));
            NINHO_SIM_REQUIRE(!registry.at(visual).at("presentation_only").get<bool>());
            if (physical.contains("fracture_pattern")) {
                for (const auto& fragment : physical.at("fracture_pattern")
                    .at("physical_fragments"))
                    NINHO_SIM_REQUIRE(registry.contains(fragment.at("visual_id")));
            }
        }
    }

    const std::string feedback_source = read_file(feedback_path);
    const auto validated_feedback = validate_feedback(feedback_source);
    NINHO_SIM_REQUIRE(validated_feedback.ok());
    const json& feedback = validated_feedback.value;
    require_exact_keys(feedback, {"schema_version", "budgets", "material_profiles",
        "event_profiles", "outcome_profiles", "profiles"});
    NINHO_SIM_REQUIRE(feedback.at("schema_version") == 2);
    for (const auto id : {"1", "5", "9", "13", "17"})
        NINHO_SIM_REQUIRE(feedback.at("material_profiles").contains(id));
    for (const auto event : {"pressure_burst", "environmental_trigger_armed",
        "environmental_trigger_detonated", "material_yielded", "crush_damage_applied",
        "score_awarded", "chain_changed", "stars_awarded"})
        NINHO_SIM_REQUIRE(feedback.at("event_profiles").contains(event));

    const auto require_error = [](const auto& result, ContentErrorCode code,
                                  std::string_view pointer) {
        NINHO_SIM_REQUIRE(!result.ok());
        NINHO_SIM_REQUIRE(result.error.code == code);
        NINHO_SIM_REQUIRE(result.error.pointer == pointer);
    };
    json invalid_assets = assets;
    invalid_assets["rogue"] = true;
    require_error(validate_assets(invalid_assets.dump()), ContentErrorCode::UnknownKey,
        "/rogue");
    invalid_assets = assets;
    invalid_assets["assets"][0].erase("status");
    require_error(validate_assets(invalid_assets.dump()), ContentErrorCode::MissingField,
        "/assets/0/status");
    invalid_assets = assets;
    invalid_assets["assets"][0]["resource_path"] = "res://outside/file.glb";
    require_error(validate_assets(invalid_assets.dump()),
        ContentErrorCode::InvalidInvariant, "/assets/0/resource_path");
    const std::string duplicate_assets = assets_source.substr(0, 1)
        + "\"schema_version\":2," + assets_source.substr(1);
    require_error(validate_assets(duplicate_assets), ContentErrorCode::DuplicateKey,
        "/schema_version");

    json invalid_feedback = feedback;
    invalid_feedback["event_profiles"]["rogue"] = "score";
    require_error(validate_feedback(invalid_feedback.dump()), ContentErrorCode::UnknownKey,
        "/event_profiles/rogue");
    invalid_feedback = feedback;
    invalid_feedback["budgets"].erase("fragment_pool_size");
    require_error(validate_feedback(invalid_feedback.dump()), ContentErrorCode::MissingField,
        "/budgets/fragment_pool_size");
}

}
