#include "test_framework.hpp"

#include "ninho/simulation/content.hpp"
#include "ninho/simulation/session.hpp"
#include "physics_world_test_facade.hpp"
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

json sorted_by(json values, std::string_view key)
{
    std::sort(values.begin(), values.end(), [&](const json& lhs, const json& rhs) {
        return lhs.at(key) < rhs.at(key);
    });
    return values;
}

void normalize_reals(json& value)
{
    if (value.is_number_float()) {
        const double source = value.get<double>();
        NINHO_SIM_REQUIRE(std::isfinite(source));
        value = static_cast<std::int64_t>(std::llround(source * 100000.0));
        return;
    }
    if (value.is_array()) {
        for (auto& child : value) normalize_reals(child);
        return;
    }
    if (value.is_object()) {
        for (auto& [key, child] : value.items()) {
            static_cast<void>(key);
            normalize_reals(child);
        }
    }
}

json layout_projection_untyped(json level)
{
    const json bodies = sorted_by(level.at("bodies"), "body_id");
    json projected_bodies = json::array();
    for (const auto& body : bodies) {
        projected_bodies.push_back(json::array({
            body.at("body_id"), body.at("entity_id"), body.at("part_id"),
            body.at("body_type"), body.at("affected_by_world_gravity"),
            body.at("material_id"), body.at("surface_id"),
            body.at("enemy_archetype_id"), body.at("density_kg_m3"),
            body.at("transform"), body.at("shape"), body.at("visual"),
            body.contains("fracture_pattern") ? body.at("fracture_pattern") : json(nullptr),
        }));
    }
    const json joints = sorted_by(level.at("joints"), "id");
    json projected_joints = json::array();
    for (const auto& joint : joints) {
        projected_joints.push_back(json::array({joint.at("id"),
            joint.at("assembly_id"), joint.at("kind"), joint.at("body_a_id"),
            joint.at("body_b_id"), joint.at("force_limit_n"),
            joint.at("torque_limit_nm")}));
    }
    const json assemblies = sorted_by(level.at("assemblies"), "id");
    json projected_assemblies = json::array();
    for (const auto& assembly : assemblies) {
        json body_ids = assembly.at("body_ids");
        json joint_ids = assembly.at("joint_ids");
        std::sort(body_ids.begin(), body_ids.end());
        std::sort(joint_ids.begin(), joint_ids.end());
        projected_assemblies.push_back(json::array({assembly.at("id"),
            assembly.at("key"), std::move(body_ids), std::move(joint_ids)}));
    }
    const json triggers = sorted_by(level.at("triggers"), "id");
    json projected_triggers = json::array();
    for (const auto& trigger : triggers) {
        projected_triggers.push_back(json::array({trigger.at("id"),
            trigger.at("target_entity_id"), trigger.at("kind"),
            trigger.at("damage_threshold"), trigger.at("fuse_ticks"),
            trigger.at("cooldown_ticks"), trigger.at("pressure_burst")}));
    }
    const json objectives = sorted_by(level.at("objectives"), "id");
    json projected_objectives = json::array();
    for (const auto& objective : objectives) {
        projected_objectives.push_back(json::array({objective.at("id"),
            objective.at("kind"), objective.at("target_entity_id")}));
    }
    json result{
        {"level_id", level.at("id")},
        {"world", level.at("world")},
        {"slingshot", level.at("slingshot")},
        {"free_body_ids", level.at("free_body_ids")},
        {"bodies", std::move(projected_bodies)},
        {"joints", std::move(projected_joints)},
        {"assemblies", std::move(projected_assemblies)},
        {"triggers", std::move(projected_triggers)},
        {"objectives", std::move(projected_objectives)},
    };
    std::sort(result["free_body_ids"].begin(), result["free_body_ids"].end());
    normalize_reals(result);
    return result;
}

json layout_projection(const LevelManifest& level)
{
    return layout_projection_untyped(json::parse(to_canonical_json(level)));
}

json layout_projection(const json& source)
{
    const auto typed = parse_level_manifest_v2(source.dump());
    if (!typed.ok()) {
        throw std::runtime_error("invalid layout probe at " + typed.error.pointer
            + ": " + typed.error.message);
    }
    return layout_projection(typed.value);
}

std::string layout_hash(const json& level)
{
    const std::uint64_t hash = fnv1a64(layout_projection(level).dump());
    std::ostringstream rendered;
    rendered << "fnv1a64:" << std::hex << hash;
    return rendered.str();
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

NINHO_SIM_TEST("product v2 levels freeze the complete farm inventory and physics")
{
    const LoadedProduct product = load_product();
    const LevelManifest& farm = product.farm;
    NINHO_SIM_REQUIRE(farm.bodies.size() == 72U);
    NINHO_SIM_REQUIRE(std::ranges::count(farm.bodies, BodyType::Dynamic,
        &BodyDefinition::body_type) == 54);
    NINHO_SIM_REQUIRE(farm.joints.size() == 52U);
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
    for (const auto& definition : farm.bodies) {
        if (definition.body_type == BodyType::Dynamic) {
            NINHO_SIM_REQUIRE(definition.affected_by_world_gravity);
            if (definition.material_id) {
                const auto material = std::ranges::find(product.materials.materials,
                    *definition.material_id, &MaterialDefinition::id);
                NINHO_SIM_REQUIRE(material != product.materials.materials.end());
                NINHO_SIM_REQUIRE(definition.density_kg_m3 == material->density_kg_m3);
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
        pair_key(6U, 8U), pair_key(6U, 9U), pair_key(15U, 16U),
        pair_key(20U, 24U), pair_key(20U, 25U), pair_key(48U, 49U),
        pair_key(48U, 50U), pair_key(48U, 51U), pair_key(49U, 50U),
        pair_key(49U, 51U), pair_key(50U, 51U)};
    for (std::size_t left = 0; left < farm.bodies.size(); ++left) {
        for (std::size_t right = left + 1U; right < farm.bodies.size(); ++right) {
            const BodyDefinition& a = farm.bodies[left];
            const BodyDefinition& b = farm.bodies[right];
            if (a.body_type == BodyType::Static && b.body_type == BodyType::Static) continue;
            if (deliberate_interlocks.contains(pair_key(a.body_id, b.body_id))) continue;
            const auto a_bounds = body_leaf_bounds(a);
            const auto b_bounds = body_leaf_bounds(b);
            for (const TestLeaf& a_leaf : a_bounds) {
                for (const TestLeaf& b_leaf : b_bounds) {
                    if (strictly_overlaps(a_leaf, b_leaf)) {
                        throw std::runtime_error("unexpected initial overlap between bodies "
                            + std::to_string(a.body_id) + " and "
                            + std::to_string(b.body_id));
                    }
                }
            }
        }
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
    NINHO_SIM_REQUIRE((body(farm, 2).transform.position_m
        == std::array{3.2, 0.61, -2.5}));
    NINHO_SIM_REQUIRE((body(farm, 3).transform.position_m
        == std::array{7.4, 0.61, 1.8}));
    NINHO_SIM_REQUIRE((body(farm, 4).transform.position_m
        == std::array{13.0, 1.83, 0.0}));
    NINHO_SIM_REQUIRE((body(farm, 5).transform.position_m
        == std::array{18.8, 0.61, -2.6}));
    NINHO_SIM_REQUIRE((body(farm, 58).transform.position_m
        == std::array{15.1, 1.4, -1.0}));
    NINHO_SIM_REQUIRE((body(farm, 59).transform.position_m
        == std::array{17.3, 2.0, -2.7}));
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
        {"bodies", 72}, {"dynamic_bodies", 54}, {"joints", 52},
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
        NINHO_SIM_REQUIRE(typed.ok());
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
    shape_changed["bodies"][61]["shape"]["half_extents_m"][0] = 1.9;
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
