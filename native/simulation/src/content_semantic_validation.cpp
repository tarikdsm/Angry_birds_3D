#include "content_semantic_validation.hpp"
#include "ability_system.hpp"
#include "shape_volume.hpp"

#include <array>
#include <cmath>
#include <numbers>
#include <queue>
#include <ranges>
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

bool box3d_sphere_mass_is_safe(double mass_kg, double radius_m) noexcept
{
    // Keep this chain equivalent to create_projectile() and Box3D's
    // b3ComputeSphereMass()/b3UpdateBodyMassData(). A positive double is not
    // sufficient: narrowing, the float volume product, or 1 / mass can still
    // underflow or overflow at runtime.
    const double source_volume_m3 = 4.0 / 3.0 * std::numbers::pi
        * radius_m * radius_m * radius_m;
    const double source_density_kg_m3 = mass_kg / source_volume_m3;
    const float runtime_radius_m = static_cast<float>(radius_m);
    const float runtime_density_kg_m3 = static_cast<float>(source_density_kg_m3);
    const float runtime_volume_m3 = 4.0F / 3.0F * std::numbers::pi_v<float>
        * runtime_radius_m * runtime_radius_m * runtime_radius_m;
    const float runtime_mass_kg = runtime_volume_m3 * runtime_density_kg_m3;
    const float runtime_inverse_mass_kg = 1.0F / runtime_mass_kg;
    const auto positive_finite = [](auto value) {
        return std::isfinite(value) && value > 0;
    };
    return positive_finite(mass_kg)
        && positive_finite(radius_m)
        && positive_finite(source_volume_m3)
        && positive_finite(source_density_kg_m3)
        && positive_finite(runtime_radius_m)
        && positive_finite(runtime_density_kg_m3)
        && positive_finite(runtime_volume_m3)
        && positive_finite(runtime_mass_kg)
        && positive_finite(runtime_inverse_mass_kg);
}

std::optional<ContentError> validate_bird_runtime_physics(
    const BirdArchetype& bird, std::string_view pointer)
{
    const std::string mass_pointer = std::string{pointer} + "/mass_kg";
    if (!std::isfinite(bird.mass_kg)) {
        return error(ContentErrorCode::InvalidNumber, mass_pointer,
            "bird mass must be finite");
    }
    if (bird.mass_kg <= 0.0 || bird.mass_kg > 100000.0) {
        return error(ContentErrorCode::OutOfRange, mass_pointer,
            "bird mass is out of range");
    }

    const std::string radius_pointer = std::string{pointer} + "/radius_m";
    if (!std::isfinite(bird.radius_m)) {
        return error(ContentErrorCode::InvalidNumber, radius_pointer,
            "bird radius must be finite");
    }
    if (bird.radius_m <= 0.0 || bird.radius_m > 100.0) {
        return error(ContentErrorCode::OutOfRange, radius_pointer,
            "bird radius is out of range");
    }

    if (!box3d_sphere_mass_is_safe(bird.mass_kg, bird.radius_m)) {
        return error(ContentErrorCode::OutOfRange, mass_pointer,
            "bird mass and radius are unsafe for the Box3D runtime");
    }
    return std::nullopt;
}

std::optional<ContentError> validate_split_child_runtime_physics(
    const BirdArchetype& bird, std::string_view pointer)
{
    const double child_mass_kg = bird.mass_kg / 3.0;
    const double child_radius_m = bird.radius_m * std::cbrt(1.0 / 3.0);
    if (!box3d_sphere_mass_is_safe(child_mass_kg, child_radius_m)) {
        return error(ContentErrorCode::OutOfRange,
            std::string{pointer} + "/mass_kg",
            "split child mass and radius are unsafe for the Box3D runtime");
    }
    return std::nullopt;
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
        if (found_enemy->second->damage_model == EnemyDamageModel::TerrestrialPig) {
            const double physical_mass = detail::shape_volume_m3(body.shape)
                * body.density_kg_m3;
            if (!std::isfinite(physical_mass)
                || std::abs(physical_mass - 65.0) > 0.065) {
                return error(ContentErrorCode::InvalidInvariant,
                    indexed("/bodies", i) + "/density_kg_m3",
                    "terrestrial pig body mass must be 65 kg within 0.1 percent");
            }
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

std::optional<ContentError> validate_product_v2_session_content(
    const MaterialCatalog& materials, const ArchetypeCatalog& archetypes,
    const LevelManifest& level)
{
    const auto duplicate = [](std::string pointer, std::string label) {
        return error(ContentErrorCode::DuplicateId, std::move(pointer),
            "duplicate " + std::move(label) + " id");
    };
    std::unordered_set<std::uint32_t> material_ids;
    std::unordered_set<std::uint32_t> surface_ids;
    std::unordered_set<std::uint32_t> ability_ids;
    std::unordered_set<std::uint32_t> bird_ids;
    std::unordered_set<std::uint32_t> weakpoint_ids;
    std::unordered_set<std::uint32_t> enemy_ids;
    std::unordered_map<std::uint32_t, const WeakpointProfile*> weakpoints;
    std::unordered_map<std::uint32_t, const PhysicsSurfaceDefinition*> surfaces;
    for (std::size_t index = 0; index < materials.materials.size(); ++index) {
        if (!material_ids.insert(materials.materials[index].id.value()).second) {
            return duplicate(indexed("/materials", index) + "/id", "material");
        }
    }
    for (std::size_t index = 0; index < materials.surfaces.size(); ++index) {
        if (!surface_ids.insert(materials.surfaces[index].id.value()).second) {
            return duplicate(indexed("/surfaces", index) + "/id", "surface");
        }
        surfaces.emplace(materials.surfaces[index].id.value(),
            &materials.surfaces[index]);
    }
    for (std::size_t index = 0; index < archetypes.abilities.size(); ++index) {
        if (const auto definition_error = AbilitySystem::validate_definition(
                archetypes.abilities[index], indexed("/abilities", index))) {
            return definition_error;
        }
        if (!ability_ids.insert(archetypes.abilities[index].id.value()).second) {
            return duplicate(indexed("/abilities", index) + "/id", "ability");
        }
    }
    for (std::size_t index = 0; index < archetypes.weakpoints.size(); ++index) {
        if (!weakpoint_ids.insert(archetypes.weakpoints[index].id.value()).second) {
            return duplicate(indexed("/weakpoints", index) + "/id", "weakpoint");
        }
        weakpoints.emplace(archetypes.weakpoints[index].id.value(),
            &archetypes.weakpoints[index]);
    }
    const std::unordered_set<std::string> presentation_ids(
        archetypes.presentation_ids.begin(), archetypes.presentation_ids.end());
    const std::unordered_set<std::string> score_ids(
        archetypes.score_ids.begin(), archetypes.score_ids.end());
    for (std::size_t index = 0; index < archetypes.birds.size(); ++index) {
        const BirdArchetype& bird = archetypes.birds[index];
        const std::string pointer = indexed("/birds", index);
        if (const auto physics_error = validate_bird_runtime_physics(bird, pointer)) {
            return physics_error;
        }
        if (!bird_ids.insert(bird.id.value()).second) {
            return duplicate(pointer + "/id", "bird");
        }
        if (!ability_ids.contains(bird.ability_id.value())) {
            return error(ContentErrorCode::MissingReference, pointer + "/ability_id",
                "ability reference not found");
        }
        const auto bird_ability = std::ranges::find(
            archetypes.abilities, bird.ability_id, &AbilityArchetype::id);
        if (bird_ability != archetypes.abilities.end()
            && bird_ability->kind_v2 == AbilityKind::Split) {
            if (const auto child_error =
                    validate_split_child_runtime_physics(bird, pointer)) {
                return child_error;
            }
            if (!presentation_ids.contains("CHR_BlueChild")) {
                return error(ContentErrorCode::MissingReference,
                    "/presentation_ids",
                    "split child presentation CHR_BlueChild is not registered");
            }
        }
        if (!surface_ids.contains(bird.surface_id.value())) {
            return error(ContentErrorCode::MissingReference, pointer + "/surface_id",
                "surface reference not found");
        }
        const std::array<std::pair<std::string_view, const std::string*>, 3>
            presentation_references{{
                {"projectile_visual_id", &bird.projectile_visual_id},
                {"icon_id", &bird.icon_id},
                {"animation_id", &bird.animation_id},
            }};
        for (const auto& [field, reference] : presentation_references) {
            if (!presentation_ids.contains(*reference)) {
                return error(ContentErrorCode::MissingReference,
                    pointer + '/' + std::string{field}, "presentation reference not found");
            }
        }
        if (!score_ids.contains(bird.score_id)) {
            return error(ContentErrorCode::MissingReference, pointer + "/score_id",
                "score reference not found");
        }
    }
    for (std::size_t index = 0; index < archetypes.enemies.size(); ++index) {
        const EnemyArchetype& enemy = archetypes.enemies[index];
        const std::string pointer = indexed("/enemies", index);
        if (!enemy_ids.insert(enemy.id.value()).second) {
            return duplicate(pointer + "/id", "enemy");
        }
        if (!weakpoint_ids.contains(enemy.weakpoint_id.value())) {
            return error(ContentErrorCode::MissingReference, pointer + "/weakpoint_id",
                "weakpoint reference not found");
        }
        if (!surface_ids.contains(enemy.surface_id.value())) {
            return error(ContentErrorCode::MissingReference, pointer + "/surface_id",
                "surface reference not found");
        }
        if (enemy.damage_model == EnemyDamageModel::TerrestrialPig) {
            const auto* weakpoint = weakpoints.at(enemy.weakpoint_id.value());
            const auto* surface = surfaces.at(enemy.surface_id.value());
            const bool calibrated = std::abs(enemy.mass_kg - 65.0) <= 0.065
                && enemy.damage_energy_j_per_kg == 18.0
                && enemy.max_damage == 70.0
                && weakpoint->protected_multiplier == 1.0
                && weakpoint->exposed_multiplier == 1.0
                && surface->friction == 0.65
                && surface->restitution == 0.05;
            if (!calibrated) {
                return error(ContentErrorCode::InvalidInvariant, pointer,
                    "terrestrial pig physics calibration is invalid");
            }
        }
    }
    if (level.bird_queue.empty()) {
        return error(ContentErrorCode::OutOfRange, "/bird_queue",
            "bird queue must contain at least one bird");
    }
    if (level.bird_queue.size() > 32U) {
        return error(ContentErrorCode::ResourceLimit, "/bird_queue",
            "bird queue capacity exceeded");
    }
    for (std::size_t index = 0; index < level.bird_queue.size(); ++index) {
        if (!bird_ids.contains(level.bird_queue[index].value())) {
            return error(ContentErrorCode::MissingReference,
                indexed("/bird_queue", index), "bird reference not found");
        }
    }
    std::unordered_set<std::uint32_t> body_ids;
    std::unordered_set<std::uint64_t> entity_parts;
    std::unordered_set<std::uint32_t> entity_ids;
    std::unordered_map<std::uint32_t, std::size_t> entity_body_counts;
    std::size_t authored_fragment_count = 0U;
    for (std::size_t index = 0; index < level.bodies.size(); ++index) {
        const BodyDefinition& body = level.bodies[index];
        const std::string pointer = indexed("/bodies", index);
        if (!body_ids.insert(body.body_id).second) {
            return duplicate(pointer + "/body_id", "body");
        }
        const std::uint64_t identity =
            (static_cast<std::uint64_t>(body.entity_id.value()) << 32U)
            | body.part_id.value();
        if (!entity_parts.insert(identity).second) {
            return duplicate(pointer + "/part_id", "entity part");
        }
        entity_ids.insert(body.entity_id.value());
        ++entity_body_counts[body.entity_id.value()];
        if (body.material_id.has_value() == body.surface_id.has_value()) {
            return error(ContentErrorCode::InvalidInvariant, pointer,
                "body requires exactly one material or surface");
        }
        if (body.material_id && !material_ids.contains(body.material_id->value())) {
            return error(ContentErrorCode::MissingReference, pointer + "/material_id",
                "material reference not found");
        }
        if (body.surface_id && !surface_ids.contains(body.surface_id->value())) {
            return error(ContentErrorCode::MissingReference, pointer + "/surface_id",
                "surface reference not found");
        }
        if (body.enemy_archetype_id
            && !enemy_ids.contains(body.enemy_archetype_id->value())) {
            return error(ContentErrorCode::MissingReference,
                pointer + "/enemy_archetype_id", "enemy reference not found");
        }
        if (body.fracture_pattern) {
            if (body.enemy_archetype_id || !body.material_id) {
                return error(ContentErrorCode::InvalidInvariant,
                    pointer + "/fracture_pattern",
                    "fracture patterns are allowed only on material bodies");
            }
            authored_fragment_count +=
                body.fracture_pattern->physical_fragments.size();
            if (authored_fragment_count > 80U) {
                return error(ContentErrorCode::ResourceLimit,
                    pointer + "/fracture_pattern/physical_fragments",
                    "active physical fragment capacity exceeded");
            }
            double mass = 0.0;
            std::unordered_set<std::uint32_t> ordinals;
            for (const auto& fragment :
                body.fracture_pattern->physical_fragments) {
                if (fragment.ordinal == 0U
                    || !ordinals.insert(fragment.ordinal).second
                    || !std::isfinite(fragment.density_kg_m3)
                    || fragment.density_kg_m3 <= 0.0
                    || fragment.visual_id.empty()) {
                    return error(ContentErrorCode::InvalidInvariant,
                        pointer + "/fracture_pattern/physical_fragments",
                        "physical fragment descriptor is invalid");
                }
                mass += detail::shape_volume_m3(fragment.shape)
                    * fragment.density_kg_m3;
            }
            const double parent_mass = detail::shape_volume_m3(body.shape)
                * body.density_kg_m3;
            if (body.fracture_pattern->physical_fragments.empty()
                || parent_mass <= 0.0
                || std::abs(mass - parent_mass) > parent_mass * 0.001) {
                return error(ContentErrorCode::InvalidInvariant,
                    pointer + "/fracture_pattern",
                    "physical fragment mass must equal parent mass within 0.1 percent");
            }
        }
    }
    std::unordered_set<std::uint32_t> trigger_ids;
    for (std::size_t index = 0; index < level.triggers.size(); ++index) {
        const EnvironmentalTriggerDefinition& trigger = level.triggers[index];
        const std::string pointer = indexed("/triggers", index);
        if (!trigger_ids.insert(trigger.id).second) {
            return duplicate(pointer + "/id", "trigger");
        }
        if (!entity_ids.contains(trigger.target_entity_id.value())) {
            return error(ContentErrorCode::MissingReference,
                pointer + "/target_entity_id", "target entity reference not found");
        }
        if (entity_body_counts[trigger.target_entity_id.value()] != 1U) {
            return error(ContentErrorCode::InvalidInvariant,
                pointer + "/target_entity_id",
                "trigger target must resolve to exactly one body");
        }
        if (!std::isfinite(trigger.damage_threshold)
            || trigger.damage_threshold <= 0.0
            || trigger.damage_threshold > 1.0e6) {
            return error(ContentErrorCode::OutOfRange,
                pointer + "/damage_threshold", "number out of range");
        }
        if (trigger.fuse_ticks > 36000U || trigger.cooldown_ticks > 36000U) {
            return error(ContentErrorCode::OutOfRange, pointer,
                "trigger tick count is out of range");
        }
        const auto& burst = trigger.pressure_burst;
        const std::string burst_pointer = pointer + "/pressure_burst";
        if (!std::isfinite(burst.radius_m) || burst.radius_m <= 0.0
            || burst.radius_m > 1000.0) {
            return error(ContentErrorCode::OutOfRange,
                burst_pointer + "/radius_m", "number out of range");
        }
        if (!std::isfinite(burst.impulse_n_s) || burst.impulse_n_s <= 0.0
            || burst.impulse_n_s > 1.0e9) {
            return error(ContentErrorCode::OutOfRange,
                burst_pointer + "/impulse_n_s", "number out of range");
        }
        if (!std::isfinite(burst.energy_j) || burst.energy_j <= 0.0
            || burst.energy_j > 1.0e12) {
            return error(ContentErrorCode::OutOfRange,
                burst_pointer + "/energy_j", "number out of range");
        }
        if (burst.max_bodies < 1U || burst.max_bodies > 32U) {
            return error(ContentErrorCode::OutOfRange,
                burst_pointer + "/max_bodies", "integer out of range");
        }
    }
    return validate_level_semantics(archetypes, level);
}

}
