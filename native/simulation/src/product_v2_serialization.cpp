#include "ninho/simulation/content.hpp"

#include "product_v2_reader.hpp"

#include <utility>

namespace ninho::simulation
{
namespace
{
using namespace detail::v2content;
json shape_json(const ShapeDefinition& shape)
{
    json result;
    switch (shape.type)
    {
    case ShapeType::Box:
        result = {{"type", "box"}, {"half_extents_m", shape.half_extents_m}};
        break;
    case ShapeType::Sphere:
        result = {{"type", "sphere"}, {"radius_m", shape.radius_m}};
        break;
    case ShapeType::Capsule:
        result = {{"type", "capsule"},
                  {"radius_m", shape.radius_m},
                  {"half_height_m", shape.half_height_m}};
        break;
    case ShapeType::ConvexHull:
        result = {{"type", "convex_hull"}, {"vertices_m", shape.vertices_m}};
        break;
    case ShapeType::Compound:
    {
        json children = json::array();
        for (const auto& value : shape.children)
            children.push_back(shape_json(value));
        result = {{"type", "compound"}, {"children", std::move(children)}};
        break;
    }
    }
    result["local_transform"] = {
        {"position_m", shape.local_position_m},
        {"rotation_xyzw", shape.local_rotation_xyzw},
    };
    return result;
}

const char* response_name(MaterialResponse response)
{
    switch (response)
    {
    case MaterialResponse::Fibrous:
        return "fibrous";
    case MaterialResponse::Masonry:
        return "masonry";
    case MaterialResponse::Brittle:
        return "brittle";
    case MaterialResponse::Compressible:
        return "compressible";
    case MaterialResponse::Ductile:
        return "ductile";
    }
    return "invalid";
}

const char* joint_name(JointKind kind)
{
    switch (kind)
    {
    case JointKind::PineFit:
        return "pine_fit";
    case JointKind::GlassClamp:
        return "glass_clamp";
    case JointKind::Mortar:
        return "mortar";
    }
    return "invalid";
}

json ability_payload_json(const AbilityArchetype& ability)
{
    switch (ability.kind_v2)
    {
    case AbilityKind::GravityField:
    {
        const auto& p = std::get<AbilityArchetype::GravityFieldPayload>(ability.payload);
        return {{"arm_ticks", p.arm_ticks},
                {"duration_ticks", p.duration_ticks},
                {"radius_m", p.radius_m},
                {"max_body_mass_kg", p.max_body_mass_kg},
                {"max_bodies", p.max_bodies},
                {"max_acceleration_m_s2", p.max_acceleration_m_s2},
                {"pulse_speed_m_s", p.pulse_speed_m_s}};
    }
    case AbilityKind::MassBoost:
    {
        const auto& p = std::get<AbilityArchetype::MassBoostPayload>(ability.payload);
        return {{"duration_ticks", p.duration_ticks}, {"mass_multiplier", p.mass_multiplier}};
    }
    case AbilityKind::SpeedBoost:
    {
        const auto& p = std::get<AbilityArchetype::SpeedBoostPayload>(ability.payload);
        return {{"impulse_m_s", p.impulse_m_s}};
    }
    case AbilityKind::Explosion:
    {
        const auto& p = std::get<AbilityArchetype::ExplosionPayload>(ability.payload);
        return {{"radius_m", p.radius_m},
                {"impulse_n_s", p.impulse_n_s},
                {"energy_j", p.energy_j},
                {"max_bodies", p.max_bodies}};
    }
    case AbilityKind::Split:
    {
        const auto& p = std::get<AbilityArchetype::SplitPayload>(ability.payload);
        return {{"child_count", p.child_count},
                {"spread_angle_deg", p.spread_angle_deg},
                {"child_speed_multiplier", p.child_speed_multiplier}};
    }
    case AbilityKind::LegacyGravityField:
        break;
    }
    return nullptr;
}

} // namespace

namespace detail
{

std::string to_canonical_json_v2(const MaterialCatalog& catalog)
{
    json root{{"schema_version", 2}, {"materials", json::array()}, {"surfaces", json::array()}};
    for (const auto& item : catalog.materials)
        root["materials"].push_back({{"id", item.id.value()},
                                     {"key", item.key},
                                     {"response", response_name(item.response)},
                                     {"density_kg_m3", item.density_kg_m3},
                                     {"friction", item.friction},
                                     {"restitution", item.restitution},
                                     {"toughness", item.toughness}});
    for (const auto& item : catalog.surfaces)
        root["surfaces"].push_back({{"id", item.id.value()},
                                    {"key", item.key},
                                    {"density_kg_m3", item.density_kg_m3},
                                    {"friction", item.friction},
                                    {"restitution", item.restitution}});
    return root.dump();
}

std::string to_canonical_json_v2(const ArchetypeCatalog& catalog)
{
    json root{{"schema_version", 2},
              {"presentation_ids", catalog.presentation_ids},
              {"score_ids", catalog.score_ids},
              {"abilities", json::array()},
              {"birds", json::array()},
              {"weakpoints", json::array()},
              {"enemies", json::array()}};
    for (const auto& item : catalog.abilities)
        root["abilities"].push_back({{"id", item.id.value()},
                                     {"key", item.key},
                                     {"kind", item.kind},
                                     {"payload", ability_payload_json(item)}});
    for (const auto& item : catalog.birds)
        root["birds"].push_back({{"id", item.id.value()},
                                 {"key", item.key},
                                 {"projectile_visual_id", item.projectile_visual_id},
                                 {"ability_id", item.ability_id.value()},
                                 {"surface_id", item.surface_id.value()},
                                 {"mass_kg", item.mass_kg},
                                 {"radius_m", item.radius_m},
                                 {"friction", item.friction},
                                 {"restitution", item.restitution},
                                 {"bullet", item.bullet},
                                 {"launch_speed_cap_m_s", item.launch_speed_cap_m_s},
                                 {"score_id", item.score_id},
                                 {"icon_id", item.icon_id},
                                 {"animation_id", item.animation_id}});
    for (const auto& item : catalog.weakpoints)
        root["weakpoints"].push_back({{"id", item.id.value()},
                                      {"key", item.key},
                                      {"protected_direction", item.protected_direction},
                                      {"protected_cone_deg", item.protected_cone_deg},
                                      {"protected_multiplier", item.protected_multiplier},
                                      {"exposed_multiplier", item.exposed_multiplier}});
    for (const auto& item : catalog.enemies)
        root["enemies"].push_back({{"id", item.id.value()},
                                   {"key", item.key},
                                   {"weakpoint_id", item.weakpoint_id.value()},
                                   {"surface_id", item.surface_id.value()},
                                   {"mass_kg", item.mass_kg},
                                   {"integrity", item.integrity},
                                   {"damage_energy_j_per_kg", item.damage_energy_j_per_kg},
                                   {"max_damage", item.max_damage}});
    return root.dump();
}

std::string to_canonical_json_v2(const LevelManifest& level)
{
    json world;
    if (const auto* value = std::get_if<UniformWorldDefinition>(&level.world))
        world = {{"kind", "uniform"},
                 {"acceleration_m_s2", value->acceleration_m_s2},
                 {"bounds", {{"min_m", value->bounds_min_m}, {"max_m", value->bounds_max_m}}}};
    else
    {
        const auto& radial = std::get<RadialWorldDefinition>(level.world);
        world = {{"kind", "radial"},
                 {"center_m", radial.center_m},
                 {"reference_radius_m", radial.reference_radius_m},
                 {"reference_acceleration_m_s2", radial.reference_acceleration_m_s2},
                 {"bounds", {{"radius_m", radial.bounds_radius_m}}}};
    }
    json queue = json::array();
    for (const auto value : level.bird_queue)
        queue.push_back(value.value());
    json root{{"schema_version", 2},
              {"id", level.id},
              {"world_id", level.world_id},
              {"region_id", level.region_id},
              {"camera_profile_id", level.camera_profile_id},
              {"presentation_profile_id", level.presentation_profile_id},
              {"world", std::move(world)},
              {"slingshot",
               {{"asset_id", level.slingshot.asset_id},
                {"rest_position_m", level.slingshot.rest_position_m},
                {"rest_rotation_xyzw", level.slingshot.rest_rotation_xyzw},
                {"spring_constant_n_m", level.slingshot.spring_constant_n_m},
                {"energy_efficiency", level.slingshot.energy_efficiency},
                {"minimum_extension_m", level.slingshot.minimum_extension_m},
                {"maximum_extension_m", level.slingshot.maximum_extension_m},
                {"plane_policy", level.slingshot.plane_policy},
                {"projectile_clearance_m", level.slingshot.projectile_clearance_m},
                {"speed_ceiling_m_s", level.slingshot.speed_ceiling_m_s}}},
              {"bird_queue", std::move(queue)},
              {"scoring",
               {{"pig_points", level.scoring.pig_points},
                {"unused_bird_points", level.scoring.unused_bird_points},
                {"star_thresholds", level.scoring.star_thresholds},
                {"chain_window_ticks", level.scoring.chain_window_ticks},
                {"chain_multiplier_step", level.scoring.chain_multiplier_step},
                {"max_chain_multiplier", level.scoring.max_chain_multiplier}}},
              {"free_body_ids", level.free_body_ids},
              {"bodies", json::array()},
              {"joints", json::array()},
              {"assemblies", json::array()},
              {"triggers", json::array()},
              {"objectives", json::array()},
              {"settle_policy",
               {{"linear_speed_m_s", level.settle_policy.linear_speed_m_s},
                {"angular_speed_rad_s", level.settle_policy.angular_speed_rad_s},
                {"rest_ticks", level.settle_policy.rest_ticks}}},
              {"watchdog_ticks", level.watchdog_ticks}};
    for (const auto& item : level.bodies)
        root["bodies"].push_back(
            {{"body_id", item.body_id},
             {"entity_id", item.entity_id.value()},
             {"part_id", item.part_id.value()},
             {"body_type", item.body_type == BodyType::Static ? "static" : "dynamic"},
             {"affected_by_world_gravity", item.affected_by_world_gravity},
             {"material_id", item.material_id ? json(item.material_id->value()) : json(nullptr)},
             {"surface_id", item.surface_id ? json(item.surface_id->value()) : json(nullptr)},
             {"enemy_archetype_id",
              item.enemy_archetype_id ? json(item.enemy_archetype_id->value()) : json(nullptr)},
             {"density_kg_m3", item.density_kg_m3},
             {"transform",
              {{"position_m", item.transform.position_m},
               {"rotation_xyzw", item.transform.rotation_xyzw}}},
             {"shape", shape_json(item.shape)},
             {"visual", {{"asset_id", item.visual.asset_id}, {"bounds_m", item.visual.bounds_m}}}});
    for (const auto& item : level.joints)
        root["joints"].push_back({{"id", item.id.value()},
                                  {"assembly_id", item.assembly_id},
                                  {"kind", joint_name(item.kind)},
                                  {"body_a_id", item.body_a_id},
                                  {"body_b_id", item.body_b_id},
                                  {"force_limit_n", item.force_limit_n},
                                  {"torque_limit_nm", item.torque_limit_nm}});
    for (const auto& item : level.assemblies)
    {
        json joint_ids = json::array();
        for (const auto value : item.joint_ids)
            joint_ids.push_back(value.value());
        root["assemblies"].push_back({{"id", item.id},
                                      {"key", item.key},
                                      {"body_ids", item.body_ids},
                                      {"joint_ids", std::move(joint_ids)}});
    }
    for (const auto& item : level.triggers)
        root["triggers"].push_back({{"id", item.id},
                                    {"target_entity_id", item.target_entity_id.value()},
                                    {"kind", "damage_threshold"},
                                    {"damage_threshold", item.damage_threshold},
                                    {"fuse_ticks", item.fuse_ticks},
                                    {"cooldown_ticks", item.cooldown_ticks},
                                    {"pressure_burst",
                                     {{"radius_m", item.pressure_burst.radius_m},
                                      {"impulse_n_s", item.pressure_burst.impulse_n_s},
                                      {"energy_j", item.pressure_burst.energy_j},
                                      {"line_of_sight", item.pressure_burst.line_of_sight},
                                      {"max_bodies", item.pressure_burst.max_bodies}}}});
    for (const auto& item : level.objectives)
        root["objectives"].push_back({{"id", item.id},
                                      {"kind", "neutralize_entity"},
                                      {"target_entity_id", item.target_entity_id.value()}});
    return root.dump();
}

} // namespace detail

std::string to_canonical_json(const CampaignManifest& campaign)
{
    json root{{"schema_version", 2},
              {"default_world_id", campaign.default_world_id},
              {"world_order", campaign.world_order},
              {"scene_ids", campaign.scene_ids},
              {"diorama_ids", campaign.diorama_ids},
              {"text_ids", campaign.text_ids},
              {"worlds", json::array()}};
    for (const auto& world : campaign.worlds)
    {
        json levels = json::array();
        for (const auto& level : world.levels)
            levels.push_back({{"id", level.id},
                              {"region_id", level.region_id},
                              {"camera_profile_id", level.camera_profile_id},
                              {"presentation_profile_id", level.presentation_profile_id},
                              {"scene_id", level.scene_id},
                              {"unlock_after_level_id", level.unlock_after_level_id
                                                            ? json(*level.unlock_after_level_id)
                                                            : json(nullptr)}});
        root["worlds"].push_back({{"id", world.id},
                                  {"diorama_id", world.diorama_id},
                                  {"text_id", world.text_id},
                                  {"default_level_id", world.default_level_id},
                                  {"level_order", world.level_order},
                                  {"levels", std::move(levels)}});
    }
    return root.dump();
}

} // namespace ninho::simulation
