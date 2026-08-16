#include "product_v2_layout_fixture.hpp"

#include <nlohmann/json.hpp>

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <stdexcept>
#include <sstream>
#include <string_view>
#include <utility>

namespace ninho::simulation::test {
namespace {

using json = nlohmann::json;

std::uint64_t fnv1a64(std::string_view bytes)
{
    std::uint64_t result = 14695981039346656037ULL;
    for (const unsigned char byte : bytes) {
        result ^= byte;
        result *= 1099511628211ULL;
    }
    return result;
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
        if (!std::isfinite(source)) {
            throw std::runtime_error("layout projection contains a non-finite real");
        }
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

json project_untyped(json level)
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
            body.contains("fracture_pattern")
                ? body.at("fracture_pattern") : json(nullptr),
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

}

nlohmann::json product_v2_layout_projection(const LevelManifest& level)
{
    return project_untyped(nlohmann::json::parse(to_canonical_json(level)));
}

std::string product_v2_layout_hash(const LevelManifest& level)
{
    const std::uint64_t hash = fnv1a64(product_v2_layout_projection(level).dump());
    std::ostringstream rendered;
    rendered << "fnv1a64:" << std::hex << hash;
    return rendered.str();
}

nlohmann::json product_v2_layout_fixture(const LevelManifest& level)
{
    const std::size_t dynamic_bodies = static_cast<std::size_t>(std::ranges::count(
        level.bodies, BodyType::Dynamic, &BodyDefinition::body_type));
    return {
        {"schema_version", 1U},
        {"level_id", level.id},
        {"layout_hash", product_v2_layout_hash(level)},
        {"counts", {
            {"bodies", level.bodies.size()},
            {"dynamic_bodies", dynamic_bodies},
            {"joints", level.joints.size()},
            {"assemblies", level.assemblies.size()},
            {"triggers", level.triggers.size()},
            {"objectives", level.objectives.size()},
        }},
        {"projection", product_v2_layout_projection(level)},
    };
}

std::string product_v2_layout_fixture_document(const LevelManifest& level)
{
    return product_v2_layout_fixture(level).dump() + '\n';
}

}
