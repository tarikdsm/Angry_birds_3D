#include "ninho/simulation/content.hpp"

#include "ability_system.hpp"
#include "product_v2_reader.hpp"

#include <limits>
#include <unordered_set>
#include <utility>

namespace ninho::simulation
{
namespace
{
using namespace detail::v2content;
MaterialResponse material_response(std::string_view value, const std::string& pointer)
{
    if (value == "fibrous")
        return MaterialResponse::Fibrous;
    if (value == "masonry")
        return MaterialResponse::Masonry;
    if (value == "brittle")
        return MaterialResponse::Brittle;
    if (value == "compressible")
        return MaterialResponse::Compressible;
    if (value == "ductile")
        return MaterialResponse::Ductile;
    fail(ContentErrorCode::InvalidEnum, pointer, "invalid material response");
}

AbilityArchetype parse_ability(const json& item, const std::string& pointer)
{
    keys(item, pointer, {"id", "key", "kind", "payload"});
    AbilityArchetype result;
    result.id = id<AbilityId>(item, "id", pointer);
    result.key = text(item, "key", pointer);
    result.kind = text(item, "kind", pointer);
    const auto payload_pointer = child(pointer, "payload");
    const auto& payload = member(item, "payload", pointer);
    if (result.kind == "gravity_field")
    {
        result.kind_v2 = AbilityKind::GravityField;
        keys(payload, payload_pointer,
             {"arm_ticks", "duration_ticks", "radius_m", "max_body_mass_kg", "max_bodies",
              "max_acceleration_m_s2", "pulse_speed_m_s"});
        AbilityArchetype::GravityFieldPayload value;
        value.arm_ticks = uint(payload, "arm_ticks", payload_pointer, 1U, 3600U);
        value.duration_ticks = uint(payload, "duration_ticks", payload_pointer, 1U, 3600U);
        value.radius_m = number(payload, "radius_m", payload_pointer, 0.0, 100.0, false);
        value.max_body_mass_kg =
            number(payload, "max_body_mass_kg", payload_pointer, 0.0, 100000.0, false);
        value.max_bodies = uint(payload, "max_bodies", payload_pointer, 1U, 500U);
        value.max_acceleration_m_s2 =
            number(payload, "max_acceleration_m_s2", payload_pointer, 0.0, 1000.0, false);
        value.pulse_speed_m_s =
            number(payload, "pulse_speed_m_s", payload_pointer, 0.0, 1000.0, false);
        result.arm_ticks = value.arm_ticks;
        result.duration_ticks = value.duration_ticks;
        result.radius_m = value.radius_m;
        result.max_body_mass_kg = value.max_body_mass_kg;
        result.max_bodies = value.max_bodies;
        result.max_acceleration_m_s2 = value.max_acceleration_m_s2;
        result.pulse_speed_m_s = value.pulse_speed_m_s;
        result.payload = value;
    }
    else if (result.kind == "mass_boost")
    {
        result.kind_v2 = AbilityKind::MassBoost;
        keys(payload, payload_pointer, {"duration_ticks", "mass_multiplier"});
        const AbilityArchetype::MassBoostPayload value{
            uint(payload, "duration_ticks", payload_pointer, 0U,
                std::numeric_limits<std::uint32_t>::max()),
            number(payload, "mass_multiplier", payload_pointer,
                -std::numeric_limits<double>::max(),
                std::numeric_limits<double>::max())};
        result.arm_ticks = 9U;
        result.duration_ticks = value.duration_ticks;
        result.payload = value;
    }
    else if (result.kind == "speed_boost")
    {
        result.kind_v2 = AbilityKind::SpeedBoost;
        keys(payload, payload_pointer, {"impulse_m_s"});
        result.payload = AbilityArchetype::SpeedBoostPayload{
            number(payload, "impulse_m_s", payload_pointer, 0.0, 1000.0, false)};
    }
    else if (result.kind == "explosion")
    {
        result.kind_v2 = AbilityKind::Explosion;
        keys(payload, payload_pointer, {"radius_m", "impulse_n_s", "energy_j", "max_bodies"});
        result.payload = AbilityArchetype::ExplosionPayload{
            number(payload, "radius_m", payload_pointer, 0.0, 100.0, false),
            number(payload, "impulse_n_s", payload_pointer, 0.0, 1e9, false),
            number(payload, "energy_j", payload_pointer, 0.0, 1e12, false),
            uint(payload, "max_bodies", payload_pointer, 1U, 500U)};
    }
    else if (result.kind == "split")
    {
        result.kind_v2 = AbilityKind::Split;
        keys(payload, payload_pointer,
             {"child_count", "spread_angle_deg", "child_speed_multiplier"});
        result.payload = AbilityArchetype::SplitPayload{
            uint(payload, "child_count", payload_pointer, 2U, 16U),
            number(payload, "spread_angle_deg", payload_pointer, 0.0, 180.0, false),
            number(payload, "child_speed_multiplier", payload_pointer, 0.0, 2.0, false)};
    }
    else
    {
        fail(ContentErrorCode::InvalidEnum, child(pointer, "kind"), "invalid ability kind");
    }
    if (const auto definition_error = detail::AbilitySystem::validate_definition(
            result, pointer)) {
        fail(definition_error->code, definition_error->pointer,
            definition_error->message);
    }
    return result;
}

} // namespace

ContentResult<MaterialCatalog> parse_material_catalog_v2(std::string_view input) noexcept
{
    return boundary<MaterialCatalog>(
        [&]
        {
            const auto root = parse_json(input, catalog_max_bytes);
            object(root, "");
            schema_v2(root);
            keys(root, "", {"schema_version", "materials", "surfaces"});
            MaterialCatalog result;
            result.schema_version = result.source_schema_version = 2U;
            const auto& materials = member(root, "materials", "");
            array(materials, "/materials", 64U, true);
            std::unordered_set<std::uint32_t> ids;
            std::unordered_set<std::string> names;
            for (std::size_t i = 0; i < materials.size(); ++i)
            {
                const auto pointer = indexed("/materials", i);
                const auto& item = materials[i];
                keys(item, pointer,
                     {"id", "key", "response", "density_kg_m3", "friction", "restitution",
                      "toughness"});
                MaterialDefinition value;
                value.id = id<MaterialId>(item, "id", pointer);
                unique_id(ids, value.id.value(), child(pointer, "id"));
                value.key = text(item, "key", pointer);
                if (!names.insert(value.key).second)
                    fail(ContentErrorCode::DuplicateId, child(pointer, "key"), "duplicate key");
                value.response =
                    material_response(text(item, "response", pointer), child(pointer, "response"));
                value.density_kg_m3 = number(item, "density_kg_m3", pointer, 0.0, 30000.0, false);
                value.friction = number(item, "friction", pointer, 0.0, 1.0);
                value.restitution = number(item, "restitution", pointer, 0.0, 1.0);
                value.toughness = number(item, "toughness", pointer, 0.0, 1.0, false);
                result.materials.push_back(std::move(value));
            }
            ids.clear();
            const auto& surfaces = member(root, "surfaces", "");
            array(surfaces, "/surfaces", 64U, true);
            for (std::size_t i = 0; i < surfaces.size(); ++i)
            {
                const auto pointer = indexed("/surfaces", i);
                const auto& item = surfaces[i];
                keys(item, pointer, {"id", "key", "density_kg_m3", "friction", "restitution"});
                PhysicsSurfaceDefinition value;
                value.id = id<SurfaceId>(item, "id", pointer);
                unique_id(ids, value.id.value(), child(pointer, "id"));
                if (value.id.value() < 1000U)
                    fail(ContentErrorCode::OutOfRange, child(pointer, "id"),
                         "surface id must be reserved");
                value.key = text(item, "key", pointer);
                if (!names.insert(value.key).second)
                    fail(ContentErrorCode::DuplicateId, child(pointer, "key"), "duplicate key");
                value.density_kg_m3 = number(item, "density_kg_m3", pointer, 0.0, 30000.0, false);
                value.friction = number(item, "friction", pointer, 0.0, 1.0);
                value.restitution = number(item, "restitution", pointer, 0.0, 1.0);
                result.surfaces.push_back(std::move(value));
            }
            return result;
        });
}

ContentResult<ArchetypeCatalog> parse_archetype_catalog_v2(std::string_view input) noexcept
{
    return boundary<ArchetypeCatalog>(
        [&]
        {
            const auto root = parse_json(input, catalog_max_bytes);
            object(root, "");
            schema_v2(root);
            keys(root, "",
                 {"schema_version", "presentation_ids", "score_ids", "abilities", "birds",
                  "weakpoints", "enemies"});
            ArchetypeCatalog result;
            result.schema_version = result.source_schema_version = 2U;
            result.presentation_ids = string_set(root, "presentation_ids", 128U);
            result.score_ids = string_set(root, "score_ids", 64U);
            const std::unordered_set<std::string> presentation(result.presentation_ids.begin(),
                                                               result.presentation_ids.end());
            const std::unordered_set<std::string> scores(result.score_ids.begin(),
                                                         result.score_ids.end());
            std::unordered_set<std::uint32_t> ability_ids;
            const auto& abilities = member(root, "abilities", "");
            array(abilities, "/abilities", 32U, true);
            for (std::size_t i = 0; i < abilities.size(); ++i)
            {
                auto value = parse_ability(abilities[i], indexed("/abilities", i));
                unique_id(ability_ids, value.id.value(), indexed("/abilities", i) + "/id");
                result.abilities.push_back(std::move(value));
            }
            std::unordered_set<std::uint32_t> bird_ids;
            const auto& birds = member(root, "birds", "");
            array(birds, "/birds", 32U, true);
            for (std::size_t i = 0; i < birds.size(); ++i)
            {
                const auto pointer = indexed("/birds", i);
                const auto& item = birds[i];
                keys(item, pointer,
                     {"id", "key", "projectile_visual_id", "ability_id", "surface_id", "mass_kg",
                      "radius_m", "friction", "restitution", "bullet", "launch_speed_cap_m_s",
                      "score_id", "icon_id", "animation_id"});
                BirdArchetype value;
                value.id = id<BirdArchetypeId>(item, "id", pointer);
                unique_id(bird_ids, value.id.value(), child(pointer, "id"));
                value.key = text(item, "key", pointer);
                value.projectile_visual_id = text(item, "projectile_visual_id", pointer);
                value.ability_id = id<AbilityId>(item, "ability_id", pointer);
                value.surface_id = id<SurfaceId>(item, "surface_id", pointer);
                value.mass_kg = number(item, "mass_kg", pointer, 0.0, 100000.0, false);
                value.radius_m = number(item, "radius_m", pointer, 0.0, 100.0, false);
                value.friction = number(item, "friction", pointer, 0.0, 1.0);
                value.restitution = number(item, "restitution", pointer, 0.0, 1.0);
                value.bullet = boolean(item, "bullet", pointer);
                value.launch_speed_cap_m_s =
                    number(item, "launch_speed_cap_m_s", pointer, 0.0, 1000.0, false);
                value.score_id = text(item, "score_id", pointer);
                value.icon_id = text(item, "icon_id", pointer);
                value.animation_id = text(item, "animation_id", pointer);
                if (!ability_ids.contains(value.ability_id.value()))
                    fail(ContentErrorCode::MissingReference, child(pointer, "ability_id"),
                         "ability reference not found");
                for (const auto& [field, ref] :
                     std::array<std::pair<std::string_view, const std::string*>, 3>{
                         {{"projectile_visual_id", &value.projectile_visual_id},
                          {"icon_id", &value.icon_id},
                          {"animation_id", &value.animation_id}}})
                    if (!presentation.contains(*ref))
                        fail(ContentErrorCode::MissingReference, child(pointer, field),
                             "presentation reference not found");
                if (!scores.contains(value.score_id))
                    fail(ContentErrorCode::MissingReference, child(pointer, "score_id"),
                         "score reference not found");
                result.birds.push_back(std::move(value));
            }
            std::unordered_set<std::uint32_t> weakpoint_ids;
            const auto& weakpoints = member(root, "weakpoints", "");
            array(weakpoints, "/weakpoints", 32U);
            for (std::size_t i = 0; i < weakpoints.size(); ++i)
            {
                const auto pointer = indexed("/weakpoints", i);
                const auto& item = weakpoints[i];
                keys(item, pointer,
                     {"id", "key", "protected_direction", "protected_cone_deg",
                      "protected_multiplier", "exposed_multiplier"});
                WeakpointProfile value;
                value.id = id<WeakpointId>(item, "id", pointer);
                unique_id(weakpoint_ids, value.id.value(), child(pointer, "id"));
                value.key = text(item, "key", pointer);
                value.protected_direction =
                    vector<3>(item, "protected_direction", pointer, -1.0, 1.0);
                const auto& d = value.protected_direction;
                const double length = std::sqrt(d[0] * d[0] + d[1] * d[1] + d[2] * d[2]);
                if (std::abs(length - 1.0) > 1e-6)
                    fail(ContentErrorCode::InvalidInvariant, child(pointer, "protected_direction"),
                         "direction must be normalized");
                value.protected_cone_deg =
                    number(item, "protected_cone_deg", pointer, 0.0, 180.0, false);
                value.protected_multiplier =
                    number(item, "protected_multiplier", pointer, 0.0, 10.0);
                value.exposed_multiplier = number(item, "exposed_multiplier", pointer, 0.0, 10.0);
                result.weakpoints.push_back(std::move(value));
            }
            std::unordered_set<std::uint32_t> enemy_ids;
            const auto& enemies = member(root, "enemies", "");
            array(enemies, "/enemies", 32U);
            for (std::size_t i = 0; i < enemies.size(); ++i)
            {
                const auto pointer = indexed("/enemies", i);
                const auto& item = enemies[i];
                keys(item, pointer,
                     {"id", "key", "weakpoint_id", "surface_id", "mass_kg", "integrity",
                      "damage_energy_j_per_kg", "max_damage"});
                EnemyArchetype value;
                value.id = id<EnemyArchetypeId>(item, "id", pointer);
                unique_id(enemy_ids, value.id.value(), child(pointer, "id"));
                value.key = text(item, "key", pointer);
                value.weakpoint_id = id<WeakpointId>(item, "weakpoint_id", pointer);
                value.surface_id = id<SurfaceId>(item, "surface_id", pointer);
                value.mass_kg = number(item, "mass_kg", pointer, 0.0, 100000.0, false);
                value.integrity = number(item, "integrity", pointer, 0.0, 100000.0, false);
                value.damage_energy_j_per_kg =
                    number(item, "damage_energy_j_per_kg", pointer, 0.0, 100000.0, false);
                value.max_damage = number(item, "max_damage", pointer, 0.0, value.integrity, false);
                if (!weakpoint_ids.contains(value.weakpoint_id.value()))
                    fail(ContentErrorCode::MissingReference, child(pointer, "weakpoint_id"),
                         "weakpoint reference not found");
                result.enemies.push_back(std::move(value));
            }
            return result;
        });
}

} // namespace ninho::simulation
