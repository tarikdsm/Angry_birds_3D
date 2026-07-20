#include "ninho/simulation/content.hpp"

#include "content_semantic_validation.hpp"
#include "product_v2_reader.hpp"

#include <unordered_set>
#include <utility>

namespace ninho::simulation
{
namespace
{
using namespace detail::v2content;
} // namespace

ContentResult<CampaignManifest> parse_campaign_manifest(std::string_view input) noexcept
{
    return boundary<CampaignManifest>(
        [&]
        {
            const auto root = parse_json(input, catalog_max_bytes);
            object(root, "");
            schema_v2(root);
            keys(root, "",
                 {"schema_version", "default_world_id", "world_order", "scene_ids", "diorama_ids",
                  "text_ids", "worlds"});
            CampaignManifest result;
            result.schema_version = result.source_schema_version = 2U;
            result.default_world_id = text(root, "default_world_id", "");
            result.world_order = string_set(root, "world_order", 16U);
            result.scene_ids = string_set(root, "scene_ids", 128U);
            result.diorama_ids = string_set(root, "diorama_ids", 32U);
            result.text_ids = string_set(root, "text_ids", 64U);
            const std::unordered_set<std::string> scenes(result.scene_ids.begin(),
                                                         result.scene_ids.end()),
                dioramas(result.diorama_ids.begin(), result.diorama_ids.end()),
                texts(result.text_ids.begin(), result.text_ids.end());
            const auto& worlds = member(root, "worlds", "");
            array(worlds, "/worlds", 16U, true);
            if (worlds.size() != result.world_order.size())
                fail(ContentErrorCode::InvalidInvariant, "/world_order",
                     "world order must enumerate worlds");
            std::unordered_set<std::string> world_ids;
            for (std::size_t i = 0; i < worlds.size(); ++i)
            {
                const auto pointer = indexed("/worlds", i);
                const auto& item = worlds[i];
                keys(item, pointer,
                     {"id", "diorama_id", "text_id", "default_level_id", "level_order", "levels"});
                CampaignWorldDefinition world;
                world.id = text(item, "id", pointer);
                if (!world_ids.insert(world.id).second)
                    fail(ContentErrorCode::DuplicateId, child(pointer, "id"), "duplicate world id");
                if (result.world_order[i] != world.id)
                    fail(ContentErrorCode::InvalidInvariant, indexed("/world_order", i),
                         "world order differs from worlds");
                world.diorama_id = text(item, "diorama_id", pointer);
                if (!dioramas.contains(world.diorama_id))
                    fail(ContentErrorCode::MissingReference, child(pointer, "diorama_id"),
                         "diorama reference not found");
                world.text_id = text(item, "text_id", pointer);
                if (!texts.contains(world.text_id))
                    fail(ContentErrorCode::MissingReference, child(pointer, "text_id"),
                         "text reference not found");
                world.default_level_id = text(item, "default_level_id", pointer);
                const auto& order = member(item, "level_order", pointer);
                array(order, child(pointer, "level_order"), 64U, true);
                std::unordered_set<std::string> ordered;
                for (std::size_t j = 0; j < order.size(); ++j)
                {
                    if (!order[j].is_string())
                        fail(ContentErrorCode::InvalidType,
                             indexed(child(pointer, "level_order"), j), "expected string");
                    auto value = order[j].get<std::string>();
                    if (value.empty() || !ordered.insert(value).second)
                        fail(ContentErrorCode::DuplicateId,
                             indexed(child(pointer, "level_order"), j), "invalid level order");
                    world.level_order.push_back(std::move(value));
                }
                const auto& levels = member(item, "levels", pointer);
                array(levels, child(pointer, "levels"), 64U, true);
                if (levels.size() != world.level_order.size())
                    fail(ContentErrorCode::InvalidInvariant, child(pointer, "level_order"),
                         "level order must enumerate levels");
                std::unordered_set<std::string> level_ids;
                for (std::size_t j = 0; j < levels.size(); ++j)
                {
                    const auto level_pointer = indexed(child(pointer, "levels"), j);
                    const auto& level_item = levels[j];
                    keys(level_item, level_pointer,
                         {"id", "region_id", "camera_profile_id", "presentation_profile_id",
                          "scene_id", "unlock_after_level_id"});
                    CampaignLevelDefinition level;
                    level.id = text(level_item, "id", level_pointer);
                    if (!level_ids.insert(level.id).second)
                        fail(ContentErrorCode::DuplicateId, child(level_pointer, "id"),
                             "duplicate level id");
                    if (world.level_order[j] != level.id)
                        fail(ContentErrorCode::InvalidInvariant,
                             indexed(child(pointer, "level_order"), j),
                             "level order differs from levels");
                    level.region_id = text(level_item, "region_id", level_pointer);
                    level.camera_profile_id = text(level_item, "camera_profile_id", level_pointer);
                    level.presentation_profile_id =
                        text(level_item, "presentation_profile_id", level_pointer);
                    level.scene_id = text(level_item, "scene_id", level_pointer);
                    if (!scenes.contains(level.scene_id))
                        fail(ContentErrorCode::MissingReference, child(level_pointer, "scene_id"),
                             "scene reference not found");
                    const auto& unlock = member(level_item, "unlock_after_level_id", level_pointer);
                    if (unlock.is_null())
                    {
                        if (j != 0U)
                            fail(ContentErrorCode::InvalidInvariant,
                                 child(level_pointer, "unlock_after_level_id"),
                                 "only first level may be initially unlocked");
                    }
                    else
                    {
                        if (!unlock.is_string())
                            fail(ContentErrorCode::InvalidType,
                                 child(level_pointer, "unlock_after_level_id"),
                                 "expected string or null");
                        level.unlock_after_level_id = unlock.get<std::string>();
                        if (j == 0U || *level.unlock_after_level_id != world.level_order[j - 1U])
                            fail(ContentErrorCode::MissingReference,
                                 child(level_pointer, "unlock_after_level_id"),
                                 "unlock must reference previous level");
                    }
                    world.levels.push_back(std::move(level));
                }
                if (!level_ids.contains(world.default_level_id))
                    fail(ContentErrorCode::MissingReference, child(pointer, "default_level_id"),
                         "default level not found");
                result.worlds.push_back(std::move(world));
            }
            if (result.world_order.size() != 2U || result.world_order[0] != "earth" ||
                result.world_order[1] != "orbital")
                fail(ContentErrorCode::InvalidInvariant, "/world_order/0",
                     "campaign must order earth before orbital");
            if (result.default_world_id != "earth" || !world_ids.contains(result.default_world_id))
                fail(ContentErrorCode::MissingReference, "/default_world_id",
                     "default world not found");
            return result;
        });
}

ContentResult<CampaignManifest> parse_campaign_manifest_v2(std::string_view input) noexcept
{
    return parse_campaign_manifest(input);
}

ContentResult<ProductV2ContentBundle>
make_product_v2_content_bundle(const MaterialCatalog& materials, const ArchetypeCatalog& archetypes,
                               const CampaignManifest& campaign,
                               const LevelManifest& level) noexcept
{
    return boundary<ProductV2ContentBundle>(
        [&]
        {
            if (materials.source_schema_version != 2U || archetypes.source_schema_version != 2U ||
                campaign.source_schema_version != 2U || level.source_schema_version != 2U)
                fail(ContentErrorCode::InvalidInvariant, "/schema_version",
                     "product v2 bundle requires only schema version 2");
            std::unordered_set<std::uint32_t> material_ids, surface_ids, bird_ids, enemy_ids;
            for (const auto& value : materials.materials)
                material_ids.insert(value.id.value());
            for (const auto& value : materials.surfaces)
                surface_ids.insert(value.id.value());
            for (const auto& value : archetypes.birds)
                bird_ids.insert(value.id.value());
            for (const auto& value : archetypes.enemies)
                enemy_ids.insert(value.id.value());
            for (std::size_t i = 0; i < archetypes.birds.size(); ++i)
                if (!surface_ids.contains(archetypes.birds[i].surface_id.value()))
                    fail(ContentErrorCode::MissingReference, indexed("/birds", i) + "/surface_id",
                         "surface reference not found");
            for (std::size_t i = 0; i < archetypes.enemies.size(); ++i)
                if (!surface_ids.contains(archetypes.enemies[i].surface_id.value()))
                    fail(ContentErrorCode::MissingReference, indexed("/enemies", i) + "/surface_id",
                         "surface reference not found");
            for (std::size_t i = 0; i < level.bird_queue.size(); ++i)
                if (!bird_ids.contains(level.bird_queue[i].value()))
                    fail(ContentErrorCode::MissingReference, indexed("/bird_queue", i),
                         "bird reference not found");
            for (std::size_t i = 0; i < level.bodies.size(); ++i)
            {
                const auto& body = level.bodies[i];
                if (body.material_id && !material_ids.contains(body.material_id->value()))
                    fail(ContentErrorCode::MissingReference, indexed("/bodies", i) + "/material_id",
                         "material reference not found");
                if (body.surface_id && !surface_ids.contains(body.surface_id->value()))
                    fail(ContentErrorCode::MissingReference, indexed("/bodies", i) + "/surface_id",
                         "surface reference not found");
                if (body.enemy_archetype_id &&
                    !enemy_ids.contains(body.enemy_archetype_id->value()))
                    fail(ContentErrorCode::MissingReference,
                         indexed("/bodies", i) + "/enemy_archetype_id",
                         "enemy reference not found");
            }
            const CampaignWorldDefinition* world = nullptr;
            for (const auto& value : campaign.worlds)
                if (value.id == level.world_id)
                {
                    world = &value;
                    break;
                }
            if (!world)
                fail(ContentErrorCode::MissingReference, "/world_id", "world reference not found");
            const CampaignLevelDefinition* registered = nullptr;
            for (const auto& value : world->levels)
                if (value.id == level.id)
                {
                    registered = &value;
                    break;
                }
            if (!registered)
                fail(ContentErrorCode::MissingReference, "/id", "level reference not found");
            if (registered->region_id != level.region_id)
                fail(ContentErrorCode::MissingReference, "/region_id",
                     "region reference not found");
            if (registered->camera_profile_id != level.camera_profile_id)
                fail(ContentErrorCode::MissingReference, "/camera_profile_id",
                     "camera profile reference not found");
            if (registered->presentation_profile_id != level.presentation_profile_id)
                fail(ContentErrorCode::MissingReference, "/presentation_profile_id",
                     "presentation profile reference not found");
            if (const auto semantic_error = detail::validate_level_semantics(archetypes, level))
                fail(semantic_error->code, semantic_error->pointer, semantic_error->message);
            return ProductV2ContentBundle{materials, archetypes, campaign, level};
        });
}

} // namespace ninho::simulation
