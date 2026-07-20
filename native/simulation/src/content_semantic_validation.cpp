#include "content_semantic_validation.hpp"

#include <queue>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

namespace ninho::simulation::detail {
namespace {

std::string indexed(std::string_view pointer, std::size_t index)
{
    return std::string{pointer} + '/' + std::to_string(index);
}

ContentError error(ContentErrorCode code, std::string pointer, std::string message)
{
    return {code, std::move(pointer), std::move(message)};
}

struct EntitySummary {
    std::unordered_set<std::uint32_t> enemy_archetype_ids;
    bool has_non_enemy_part{};
};

}

std::optional<ContentError> validate_level_semantics(
    const ArchetypeCatalog& archetypes, const LevelManifest& level)
{
    std::unordered_map<std::uint32_t, const EnemyArchetype*> enemies;
    for (const auto& enemy : archetypes.enemies) {
        enemies.emplace(enemy.id.value(), &enemy);
    }

    std::unordered_set<std::uint32_t> body_ids;
    std::unordered_map<std::uint32_t, EntitySummary> entities;
    for (std::size_t i = 0; i < level.bodies.size(); ++i) {
        const auto& body = level.bodies[i];
        body_ids.insert(body.body_id);
        auto& entity = entities[body.entity_id.value()];
        if (!body.enemy_archetype_id) {
            entity.has_non_enemy_part = true;
            continue;
        }
        const auto found_enemy = enemies.find(body.enemy_archetype_id->value());
        if (found_enemy == enemies.end()) {
            return error(ContentErrorCode::MissingReference,
                indexed("/bodies", i) + "/enemy_archetype_id",
                "enemy archetype reference not found");
        }
        if (body.body_type != BodyType::Dynamic) {
            return error(ContentErrorCode::InvalidInvariant,
                indexed("/bodies", i) + "/body_type", "enemy body must be dynamic");
        }
        if (body.material_id || !body.surface_id) {
            return error(ContentErrorCode::InvalidInvariant,
                indexed("/bodies", i) + "/material_id",
                "enemy body must reference only a surface");
        }
        if (*body.surface_id != found_enemy->second->surface_id) {
            return error(ContentErrorCode::InvalidInvariant,
                indexed("/bodies", i) + "/surface_id",
                "enemy body surface differs from its archetype");
        }
        entity.enemy_archetype_ids.insert(body.enemy_archetype_id->value());
    }

    for (const auto& [entity_id, summary] : entities) {
        static_cast<void>(entity_id);
        if (!summary.enemy_archetype_ids.empty()
            && (summary.has_non_enemy_part || summary.enemy_archetype_ids.size() != 1U)) {
            return error(ContentErrorCode::InvalidInvariant, "/bodies",
                "enemy entity must contain exactly one enemy archetype and no non-enemy parts");
        }
    }

    std::unordered_set<std::uint32_t> assembly_ids;
    for (const auto& assembly : level.assemblies) assembly_ids.insert(assembly.id);
    std::unordered_map<std::uint32_t, const JointDefinition*> joints;
    for (std::size_t i = 0; i < level.joints.size(); ++i) {
        const auto& joint = level.joints[i];
        joints.emplace(joint.id.value(), &joint);
        if (!assembly_ids.contains(joint.assembly_id)) {
            return error(ContentErrorCode::MissingReference,
                indexed("/joints", i) + "/assembly_id", "assembly reference not found");
        }
        if (!body_ids.contains(joint.body_a_id)) {
            return error(ContentErrorCode::MissingReference,
                indexed("/joints", i) + "/body_a_id", "body reference not found");
        }
        if (!body_ids.contains(joint.body_b_id)) {
            return error(ContentErrorCode::MissingReference,
                indexed("/joints", i) + "/body_b_id", "body reference not found");
        }
    }

    std::unordered_map<std::uint32_t, std::uint32_t> body_owners;
    std::unordered_set<std::uint32_t> free_bodies;
    for (std::size_t i = 0; i < level.free_body_ids.size(); ++i) {
        const auto id = level.free_body_ids[i];
        if (!body_ids.contains(id)) {
            return error(ContentErrorCode::MissingReference, indexed("/free_body_ids", i),
                "body reference not found");
        }
        if (!free_bodies.insert(id).second) {
            return error(ContentErrorCode::InvalidInvariant, indexed("/free_body_ids", i),
                "duplicate free body membership");
        }
    }
    std::unordered_map<std::uint32_t, std::uint32_t> joint_owners;
    for (std::size_t i = 0; i < level.assemblies.size(); ++i) {
        const auto& assembly = level.assemblies[i];
        for (std::size_t j = 0; j < assembly.body_ids.size(); ++j) {
            const auto body_id = assembly.body_ids[j];
            const auto pointer = indexed(indexed("/assemblies", i) + "/body_ids", j);
            if (!body_ids.contains(body_id)) {
                return error(ContentErrorCode::MissingReference, pointer,
                    "body reference not found");
            }
            if (free_bodies.contains(body_id)
                || !body_owners.emplace(body_id, assembly.id).second) {
                return error(ContentErrorCode::InvalidInvariant, pointer,
                    "body has multiple memberships");
            }
        }
        for (std::size_t j = 0; j < assembly.joint_ids.size(); ++j) {
            const auto joint_id = assembly.joint_ids[j].value();
            const auto pointer = indexed(indexed("/assemblies", i) + "/joint_ids", j);
            if (!joints.contains(joint_id)) {
                return error(ContentErrorCode::MissingReference, pointer,
                    "joint reference not found");
            }
            if (!joint_owners.emplace(joint_id, assembly.id).second) {
                return error(ContentErrorCode::InvalidInvariant, pointer,
                    "joint has multiple memberships");
            }
        }
    }

    for (std::size_t i = 0; i < level.bodies.size(); ++i) {
        const auto body_id = level.bodies[i].body_id;
        if (!body_owners.contains(body_id) && !free_bodies.contains(body_id)) {
            return error(ContentErrorCode::InvalidInvariant,
                indexed("/bodies", i) + "/body_id", "body membership is not declared");
        }
    }
    for (std::size_t i = 0; i < level.joints.size(); ++i) {
        const auto& joint = level.joints[i];
        const auto owner = joint_owners.find(joint.id.value());
        if (owner == joint_owners.end() || owner->second != joint.assembly_id) {
            return error(ContentErrorCode::InvalidInvariant,
                indexed("/joints", i) + "/assembly_id", "joint membership is contradictory");
        }
        const auto owner_a = body_owners.find(joint.body_a_id);
        if (owner_a == body_owners.end() || owner_a->second != joint.assembly_id) {
            return error(ContentErrorCode::InvalidInvariant,
                indexed("/joints", i) + "/body_a_id",
                "joint endpoint is outside its assembly");
        }
        const auto owner_b = body_owners.find(joint.body_b_id);
        if (owner_b == body_owners.end() || owner_b->second != joint.assembly_id) {
            return error(ContentErrorCode::InvalidInvariant,
                indexed("/joints", i) + "/body_b_id",
                "joint endpoint is outside its assembly");
        }
    }

    for (std::size_t i = 0; i < level.assemblies.size(); ++i) {
        const auto& assembly = level.assemblies[i];
        if (assembly.body_ids.empty()) {
            return error(ContentErrorCode::InvalidInvariant, indexed("/assemblies", i),
                "assembly must contain at least one body");
        }
        std::unordered_map<std::uint32_t, std::vector<std::uint32_t>> adjacency;
        for (const auto body_id : assembly.body_ids) adjacency.try_emplace(body_id);
        for (const auto joint_id : assembly.joint_ids) {
            const auto& joint = *joints.at(joint_id.value());
            adjacency[joint.body_a_id].push_back(joint.body_b_id);
            adjacency[joint.body_b_id].push_back(joint.body_a_id);
        }
        std::unordered_set<std::uint32_t> visited;
        std::queue<std::uint32_t> pending;
        pending.push(assembly.body_ids.front());
        visited.insert(assembly.body_ids.front());
        while (!pending.empty()) {
            const auto current = pending.front();
            pending.pop();
            for (const auto neighbor : adjacency[current]) {
                if (visited.insert(neighbor).second) pending.push(neighbor);
            }
        }
        if (visited.size() != assembly.body_ids.size()) {
            return error(ContentErrorCode::InvalidInvariant, indexed("/assemblies", i),
                "assembly graph is disconnected");
        }
    }

    for (std::size_t i = 0; i < level.objectives.size(); ++i) {
        const auto found = entities.find(level.objectives[i].target_entity_id.value());
        if (found == entities.end() || found->second.has_non_enemy_part
            || found->second.enemy_archetype_ids.size() != 1U) {
            return error(ContentErrorCode::MissingReference,
                indexed("/objectives", i) + "/target_entity_id",
                "objective target must resolve to one enemy entity");
        }
    }
    return std::nullopt;
}

}
