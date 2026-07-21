#include "test_framework.hpp"

#include "ninho/simulation/content.hpp"
#include "ninho/simulation/session.hpp"
#include "product_v2_reader.hpp"

#include <nlohmann/json.hpp>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <numbers>
#include <ranges>
#include <sstream>
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

json layout_projection(json level)
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
    NINHO_SIM_REQUIRE(farm.ok());
    NINHO_SIM_REQUIRE(orbital.ok());
    NINHO_SIM_REQUIRE(make_product_v2_content_bundle(materials.value,
        archetypes.value, campaign.value, farm.value).ok());
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
    const std::array pig_entities{2001U, 2002U, 2003U, 2004U};
    for (std::size_t index = 0; index < pig_entities.size(); ++index) {
        const auto found = std::ranges::find(farm.bodies, EntityId{pig_entities[index]},
            &BodyDefinition::entity_id);
        NINHO_SIM_REQUIRE(found != farm.bodies.end());
        NINHO_SIM_REQUIRE(found->enemy_archetype_id == EnemyArchetypeId{2});
        NINHO_SIM_REQUIRE(farm.objectives[index].target_entity_id
            == EntityId{pig_entities[index]});
        const double volume = 4.0 * std::numbers::pi
            * std::pow(found->shape.radius_m, 3.0) / 3.0;
        NINHO_SIM_REQUIRE(std::abs(volume * found->density_kg_m3 - 65.0) < 0.001);
    }
    NINHO_SIM_REQUIRE((body(farm, 2).transform.position_m
        == std::array{2.6, 0.55, -3.4}));
    NINHO_SIM_REQUIRE((body(farm, 3).transform.position_m
        == std::array{7.4, 1.05, 1.8}));
    NINHO_SIM_REQUIRE((body(farm, 4).transform.position_m
        == std::array{13.0, 1.4, 0.0}));
    NINHO_SIM_REQUIRE((body(farm, 5).transform.position_m
        == std::array{17.0, 0.55, -2.5}));
    NINHO_SIM_REQUIRE((body(farm, 58).transform.position_m
        == std::array{16.2, 1.1, -1.6}));
    NINHO_SIM_REQUIRE((body(farm, 59).transform.position_m
        == std::array{17.6, 1.0, -3.3}));
    for (const auto& joint : farm.joints) {
        if (joint.kind != JointKind::SteelDuctile) continue;
        const auto ductile = [&](std::uint32_t body_id) {
            return body(farm, body_id).material_id == MaterialId{17};
        };
        NINHO_SIM_REQUIRE(ductile(joint.body_a_id) != ductile(joint.body_b_id));
    }
    const auto created = SimulationSession::create(
        product.materials, product.archetypes, product.farm);
    NINHO_SIM_REQUIRE(created.ok());
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

    const auto require_mutation = [&](std::string_view section,
                                      std::string_view field) {
        json changed = farm;
        if (section == "world") changed["world"][field] = 1.0;
        else if (section == "slingshot") changed["slingshot"][field] = 1.0;
        else {
            changed[section][0][field] = changed[section][0][field].is_number()
                ? json(987654.0) : json("mutation");
        }
        NINHO_SIM_REQUIRE(layout_hash(changed) != layout_hash(farm));
    };
    require_mutation("world", "acceleration_m_s2");
    require_mutation("slingshot", "spring_constant_n_m");
    require_mutation("bodies", "density_kg_m3");
    require_mutation("joints", "force_limit_n");
    require_mutation("triggers", "damage_threshold");
    require_mutation("objectives", "target_entity_id");
    json transform_changed = farm;
    transform_changed["bodies"][0]["transform"]["position_m"][0] = 2.0;
    NINHO_SIM_REQUIRE(layout_hash(transform_changed) != layout_hash(farm));
    json shape_changed = farm;
    shape_changed["bodies"][0]["shape"]["half_extents_m"][0] = 3.0;
    NINHO_SIM_REQUIRE(layout_hash(shape_changed) != layout_hash(farm));

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
