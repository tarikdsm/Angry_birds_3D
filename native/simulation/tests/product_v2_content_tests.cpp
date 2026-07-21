#include "test_framework.hpp"

#include "ninho/simulation/content.hpp"
#include "ninho/simulation/session.hpp"
#include "session_test_facade.hpp"

#include <nlohmann/json.hpp>

#include <array>
#include <cmath>
#include <memory>
#include <ranges>
#include <string>
#include <string_view>

namespace
{

using json = nlohmann::json;
using namespace ninho::simulation;

json material_catalog()
{
    return {{"schema_version", 2},
            {"materials", json::array({{{"id", 1},
                                        {"key", "hay"},
                                        {"response", "compressible"},
                                        {"density_kg_m3", 120.0},
                                        {"friction", 0.6},
                                        {"restitution", 0.1},
                                        {"toughness", 0.2}},
                                       {{"id", 2},
                                        {"key", "steel"},
                                        {"response", "ductile"},
                                        {"density_kg_m3", 7800.0},
                                        {"friction", 0.4},
                                        {"restitution", 0.05},
                                        {"toughness", 0.8}}})},
            {"surfaces", json::array({{{"id", 1001},
                                       {"key", "bird"},
                                       {"density_kg_m3", 400.0},
                                       {"friction", 0.35},
                                       {"restitution", 0.25}},
                                      {{"id", 1002},
                                       {"key", "pig"},
                                       {"density_kg_m3", 480.0},
                                       {"friction", 0.45},
                                       {"restitution", 0.1}}})}};
}

json gravity_payload()
{
    return {{"arm_ticks", 1},         {"duration_ticks", 60},
            {"radius_m", 5.0},        {"max_body_mass_kg", 1000.0},
            {"max_bodies", 16},       {"max_acceleration_m_s2", 20.0},
            {"pulse_speed_m_s", 30.0}};
}

json ability(std::uint32_t id, std::string_view kind, json payload)
{
    return {{"id", id},
            {"key", std::string{kind} + "_ability"},
            {"kind", kind},
            {"payload", std::move(payload)}};
}

json archetype_catalog()
{
    return {{"schema_version", 2},
            {"presentation_ids", json::array({"visual_red", "icon_red", "anim_red"})},
            {"score_ids", json::array({"unused_bird"})},
            {"abilities", json::array({ability(1, "gravity_field", gravity_payload()),
                                       ability(2, "mass_boost",
                                               {{"duration_ticks", 60}, {"mass_multiplier", 2.0}}),
                                       ability(3, "speed_boost", {{"impulse_m_s", 12.0}}),
                                       ability(4, "explosion",
                                               {{"radius_m", 5.0},
                                                {"impulse_n_s", 2000.0},
                                                {"energy_j", 5000.0},
                                                {"max_bodies", 32}}),
                                       ability(5, "split",
                                               {{"child_count", 3},
                                                {"spread_angle_deg", 24.0},
                                                {"child_speed_multiplier", 0.9}})})},
            {"birds", json::array({{{"id", 1},
                                    {"key", "red"},
                                    {"projectile_visual_id", "visual_red"},
                                    {"ability_id", 1},
                                    {"surface_id", 1001},
                                    {"mass_kg", 140.0},
                                    {"radius_m", 0.45},
                                    {"friction", 0.35},
                                    {"restitution", 0.25},
                                    {"bullet", true},
                                    {"launch_speed_cap_m_s", 40.0},
                                    {"score_id", "unused_bird"},
                                    {"icon_id", "icon_red"},
                                    {"animation_id", "anim_red"}}})},
            {"weakpoints", json::array({{{"id", 1},
                                         {"key", "pig_front"},
                                         {"protected_direction", json::array({-1.0, 0.0, 0.0})},
                                         {"protected_cone_deg", 30.0},
                                         {"protected_multiplier", 0.5},
                                         {"exposed_multiplier", 1.25}}})},
            {"enemies", json::array({{{"id", 1},
                                      {"key", "pig"},
                                      {"weakpoint_id", 1},
                                      {"surface_id", 1002},
                                      {"mass_kg", 480.0},
                                      {"integrity", 100.0},
                                      {"damage_energy_j_per_kg", 2.5},
                                      {"max_damage", 50.0}}})}};
}

json campaign_manifest()
{
    return {{"schema_version", 2},
            {"default_world_id", "earth"},
            {"world_order", json::array({"earth", "orbital"})},
            {"scene_ids", json::array({"scene_earth", "scene_orbital"})},
            {"diorama_ids", json::array({"diorama_earth", "diorama_orbital"})},
            {"text_ids", json::array({"text_earth", "text_orbital"})},
            {"worlds",
             json::array({{{"id", "earth"},
                           {"diorama_id", "diorama_earth"},
                           {"text_id", "text_earth"},
                           {"default_level_id", "farm"},
                           {"level_order", json::array({"farm"})},
                           {"levels", json::array({{{"id", "farm"},
                                                    {"region_id", "earth_farm"},
                                                    {"camera_profile_id", "camera_earth"},
                                                    {"presentation_profile_id", "present_earth"},
                                                    {"scene_id", "scene_earth"},
                                                    {"unlock_after_level_id", nullptr}}})}},
                          {{"id", "orbital"},
                           {"diorama_id", "diorama_orbital"},
                           {"text_id", "text_orbital"},
                           {"default_level_id", "first_orbit"},
                           {"level_order", json::array({"first_orbit"})},
                           {"levels", json::array({{{"id", "first_orbit"},
                                                    {"region_id", "aster"},
                                                    {"camera_profile_id", "camera_orbital"},
                                                    {"presentation_profile_id", "present_orbital"},
                                                    {"scene_id", "scene_orbital"},
                                                    {"unlock_after_level_id", nullptr}}})}}})}};
}

json box_shape() { return {{"type", "box"}, {"half_extents_m", json::array({0.5, 0.5, 0.5})}}; }

json level_manifest(std::string_view gravity_kind = "uniform")
{
    json world = gravity_kind == "uniform"
                     ? json{{"kind", "uniform"},
                            {"acceleration_m_s2", json::array({0.0, -9.81, 0.0})},
                            {"bounds",
                             {{"min_m", json::array({-24.0, -12.0, -12.0})},
                              {"max_m", json::array({48.0, 32.0, 12.0})}}}}
                     : json{{"kind", "radial"},
                            {"center_m", json::array({0.0, 0.0, 0.0})},
                            {"reference_radius_m", 10.0},
                            {"reference_acceleration_m_s2", 9.0},
                            {"bounds", {{"radius_m", 60.0}}}};
    return {{"schema_version", 2},
            {"id", gravity_kind == "uniform" ? "farm" : "first_orbit"},
            {"world_id", gravity_kind == "uniform" ? "earth" : "orbital"},
            {"region_id", gravity_kind == "uniform" ? "earth_farm" : "aster"},
            {"camera_profile_id", gravity_kind == "uniform" ? "camera_earth" : "camera_orbital"},
            {"presentation_profile_id",
             gravity_kind == "uniform" ? "present_earth" : "present_orbital"},
            {"world", std::move(world)},
            {"slingshot",
             {{"asset_id", "slingshot"},
              {"rest_position_m", json::array({-16.0, 2.0, 0.0})},
              {"rest_rotation_xyzw", json::array({0.0, 0.0, 0.0, 1.0})},
              {"spring_constant_n_m", 1400.0},
              {"energy_efficiency", 0.8},
              {"minimum_extension_m", 0.1},
              {"maximum_extension_m", 4.0},
              {"plane_policy", "gravity_vertical_camera_yaw"},
              {"projectile_clearance_m", 0.2},
              {"speed_ceiling_m_s", 40.0}}},
            {"bird_queue", json::array({1, 1})},
            {"scoring",
             {{"pig_points", 5000},
              {"unused_bird_points", 10000},
              {"star_thresholds", json::array({10000, 20000, 30000})},
              {"chain_window_ticks", 45},
              {"chain_multiplier_step", 0.25},
              {"max_chain_multiplier", 3.0}}},
            {"free_body_ids", json::array({1})},
            {"bodies",
             json::array({{{"body_id", 1},
                           {"entity_id", 100},
                           {"part_id", 1},
                           {"body_type", "dynamic"},
                           {"affected_by_world_gravity", true},
                           {"material_id", nullptr},
                           {"surface_id", 1002},
                           {"enemy_archetype_id", 1},
                           {"density_kg_m3", 480.0},
                           {"transform",
                            {{"position_m", json::array({0.0, 0.0, 0.0})},
                             {"rotation_xyzw", json::array({0.0, 0.0, 0.0, 1.0})}}},
                           {"shape", box_shape()},
                           {"visual",
                            {{"asset_id", "box"}, {"bounds_m", json::array({1.0, 1.0, 1.0})}}}}})},
            {"joints", json::array()},
            {"assemblies", json::array()},
            {"triggers", json::array({{{"id", 1},
                                       {"target_entity_id", 100},
                                       {"kind", "damage_threshold"},
                                       {"damage_threshold", 20.0},
                                       {"fuse_ticks", 0},
                                       {"cooldown_ticks", 0},
                                       {"pressure_burst",
                                        {{"radius_m", 4.0},
                                         {"impulse_n_s", 1200.0},
                                         {"energy_j", 3000.0},
                                         {"line_of_sight", true},
                                         {"max_bodies", 16}}}}})},
            {"objectives",
             json::array({{{"id", 1}, {"kind", "neutralize_entity"}, {"target_entity_id", 100}}})},
            {"settle_policy",
             {{"linear_speed_m_s", 0.05}, {"angular_speed_rad_s", 0.05}, {"rest_ticks", 60}}},
            {"watchdog_ticks", 1500}};
}

template <typename Result>
void require_error(const Result& result, ContentErrorCode code, std::string_view pointer)
{
    NINHO_SIM_REQUIRE(!result.ok());
    NINHO_SIM_REQUIRE(result.error.code == code);
    NINHO_SIM_REQUIRE(result.error.pointer == pointer);
}

json static_body(std::uint32_t body_id, std::uint32_t entity_id)
{
    auto body = level_manifest()["bodies"][0];
    body["body_id"] = body_id;
    body["entity_id"] = entity_id;
    body["body_type"] = "static";
    body["affected_by_world_gravity"] = false;
    body["material_id"] = 1;
    body["surface_id"] = nullptr;
    body["enemy_archetype_id"] = nullptr;
    body["density_kg_m3"] = 120.0;
    return body;
}

json level_with_two_assemblies()
{
    auto level = level_manifest();
    level["bodies"].push_back(static_body(2, 200));
    level["bodies"].push_back(static_body(3, 300));
    level["bodies"].push_back(static_body(4, 400));
    level["free_body_ids"] = json::array();
    level["joints"] = json::array({
        {{"id", 1}, {"assembly_id", 1}, {"kind", "pine_fit"}, {"body_a_id", 1},
         {"body_b_id", 2}, {"force_limit_n", 1000.0}, {"torque_limit_nm", 100.0}},
        {{"id", 2}, {"assembly_id", 2}, {"kind", "pine_fit"}, {"body_a_id", 3},
         {"body_b_id", 4}, {"force_limit_n", 1000.0}, {"torque_limit_nm", 100.0}},
    });
    level["assemblies"] = json::array({
        {{"id", 1}, {"key", "first"}, {"body_ids", json::array({1, 2})},
         {"joint_ids", json::array({1})}},
        {{"id", 2}, {"key", "second"}, {"body_ids", json::array({3, 4})},
         {"joint_ids", json::array({2})}},
    });
    return level;
}

NINHO_SIM_TEST("product v2 content parses every closed variant")
{
    const auto materials = parse_material_catalog_v2(material_catalog().dump());
    const auto archetypes = parse_archetype_catalog_v2(archetype_catalog().dump());
    const auto campaign = parse_campaign_manifest(campaign_manifest().dump());
    const auto uniform = parse_level_manifest_v2(level_manifest("uniform").dump());
    const auto radial = parse_level_manifest_v2(level_manifest("radial").dump());
    NINHO_SIM_REQUIRE(materials.ok() && archetypes.ok() && campaign.ok() && uniform.ok() &&
                      radial.ok());
    NINHO_SIM_REQUIRE(materials.value.source_schema_version == 2U);
    NINHO_SIM_REQUIRE(archetypes.value.abilities.size() == 5U);
    NINHO_SIM_REQUIRE(archetypes.value.abilities.at(0).kind_v2 == AbilityKind::GravityField);
    NINHO_SIM_REQUIRE(archetypes.value.abilities.at(1).kind_v2 == AbilityKind::MassBoost);
    NINHO_SIM_REQUIRE(archetypes.value.abilities.at(1).arm_ticks == 9U);
    NINHO_SIM_REQUIRE(archetypes.value.abilities.at(1).duration_ticks == 60U);
    NINHO_SIM_REQUIRE(uniform.value.world.index() != radial.value.world.index());
    NINHO_SIM_REQUIRE(uniform.value.bird_queue ==
                      std::vector<BirdArchetypeId>({BirdArchetypeId{1}, BirdArchetypeId{1}}));
    NINHO_SIM_REQUIRE(make_product_v2_content_bundle(materials.value, archetypes.value,
                                                     campaign.value, uniform.value)
                          .ok());
}

NINHO_SIM_TEST("product v2 content rejects world and launcher contract violations")
{
    auto level = level_manifest();
    level["world"]["kind"] = "flat_earth";
    require_error(parse_level_manifest_v2(level.dump()), ContentErrorCode::InvalidEnum,
                  "/world/kind");
    level = level_manifest("radial");
    level["world"]["acceleration_m_s2"] = json::array({0, -9.81, 0});
    require_error(parse_level_manifest_v2(level.dump()), ContentErrorCode::UnknownKey,
                  "/world/acceleration_m_s2");
    level = level_manifest("uniform");
    level["world"]["center_m"] = json::array({0.0, 0.0, 0.0});
    require_error(parse_level_manifest_v2(level.dump()), ContentErrorCode::UnknownKey,
                  "/world/center_m");
    for (const auto key :
         {"spring_constant_n_m", "energy_efficiency", "minimum_extension_m", "maximum_extension_m"})
    {
        level = level_manifest();
        level["slingshot"].erase(key);
        require_error(parse_level_manifest_v2(level.dump()), ContentErrorCode::MissingField,
                      std::string{"/slingshot/"} + key);
    }
    level = level_manifest();
    level["slingshot"]["energy_efficiency"] = 0.0;
    require_error(parse_level_manifest_v2(level.dump()), ContentErrorCode::OutOfRange,
                  "/slingshot/energy_efficiency");
    level = level_manifest();
    level["slingshot"]["energy_efficiency"] = 1.0;
    NINHO_SIM_REQUIRE(parse_level_manifest_v2(level.dump()).ok());
    level = level_manifest();
    level["slingshot"]["energy_efficiency"] = 1.01;
    require_error(parse_level_manifest_v2(level.dump()), ContentErrorCode::OutOfRange,
                  "/slingshot/energy_efficiency");
}

NINHO_SIM_TEST("product v2 content rejects ability shape scoring and gravity violations")
{
    auto archetypes = archetype_catalog();
    archetypes["abilities"][1]["payload"]["radius_m"] = 2.0;
    require_error(parse_archetype_catalog_v2(archetypes.dump()), ContentErrorCode::UnknownKey,
                  "/abilities/1/payload/radius_m");
    archetypes = archetype_catalog();
    archetypes["abilities"][3]["payload"]["max_bodies"] = 33;
    require_error(parse_archetype_catalog_v2(archetypes.dump()), ContentErrorCode::OutOfRange,
                  "/abilities/3/payload/max_bodies");
    auto level = level_manifest();
    level["bodies"][0]["shape"] = {
        {"type", "convex_hull"},
        {"vertices_m", json::array({json::array({0, 0, 0}), json::array({1, 0, 0}),
                                    json::array({2, 0, 0}), json::array({3, 0, 0})})}};
    require_error(parse_level_manifest_v2(level.dump()), ContentErrorCode::InvalidInvariant,
                  "/bodies/0/shape/vertices_m");
    level = level_manifest();
    level["bodies"][0]["shape"] = {{"type", "compound"}, {"children", json::array()}};
    require_error(parse_level_manifest_v2(level.dump()), ContentErrorCode::OutOfRange,
                  "/bodies/0/shape/children");
    level = level_manifest();
    level["scoring"]["star_thresholds"] = json::array({100, 100, 200});
    require_error(parse_level_manifest_v2(level.dump()), ContentErrorCode::InvalidInvariant,
                  "/scoring/star_thresholds/1");
    level = level_manifest();
    level["bodies"][0]["affected_by_world_gravity"] = false;
    require_error(parse_level_manifest_v2(level.dump()), ContentErrorCode::InvalidInvariant,
                  "/bodies/0/affected_by_world_gravity");
    level = level_manifest();
    level["bodies"][0]["shape"] =
        {{"type", "capsule"}, {"radius_m", 0.25}, {"half_height_m", 0.5}};
    NINHO_SIM_REQUIRE(parse_level_manifest_v2(level.dump()).ok());
}

NINHO_SIM_TEST("product v2 content rejects orphan queue presentation and campaign references")
{
    const auto materials = parse_material_catalog_v2(material_catalog().dump());
    const auto archetypes = parse_archetype_catalog_v2(archetype_catalog().dump());
    const auto campaign = parse_campaign_manifest(campaign_manifest().dump());
    auto level_json = level_manifest();
    level_json["bird_queue"] = json::array();
    require_error(parse_level_manifest_v2(level_json.dump()), ContentErrorCode::OutOfRange,
                  "/bird_queue");
    level_json = level_manifest();
    level_json["bird_queue"][0] = 999;
    const auto orphan_queue = parse_level_manifest_v2(level_json.dump());
    NINHO_SIM_REQUIRE(orphan_queue.ok());
    require_error(make_product_v2_content_bundle(materials.value, archetypes.value, campaign.value,
                                                 orphan_queue.value),
                  ContentErrorCode::MissingReference, "/bird_queue/0");
    for (const auto field : {"projectile_visual_id", "score_id", "icon_id", "animation_id"})
    {
        auto archetype_json = archetype_catalog();
        archetype_json["birds"][0][field] = "missing";
        require_error(parse_archetype_catalog_v2(archetype_json.dump()),
                      ContentErrorCode::MissingReference, std::string{"/birds/0/"} + field);
    }
    for (const auto field : {"world_id", "region_id", "camera_profile_id",
                             "presentation_profile_id"})
    {
        level_json = level_manifest();
        level_json[field] = "missing";
        const auto orphan_level = parse_level_manifest_v2(level_json.dump());
        NINHO_SIM_REQUIRE(orphan_level.ok());
        require_error(make_product_v2_content_bundle(materials.value, archetypes.value,
                                                     campaign.value, orphan_level.value),
                      ContentErrorCode::MissingReference, std::string{"/"} + field);
    }
}

NINHO_SIM_TEST("product v2 content rejects invalid campaign order defaults unlock and presentation")
{
    auto campaign = campaign_manifest();
    campaign["world_order"] = json::array({"orbital", "earth"});
    require_error(parse_campaign_manifest(campaign.dump()), ContentErrorCode::InvalidInvariant,
                  "/world_order/0");
    campaign = campaign_manifest();
    campaign["default_world_id"] = "missing";
    require_error(parse_campaign_manifest(campaign.dump()), ContentErrorCode::MissingReference,
                  "/default_world_id");
    campaign = campaign_manifest();
    campaign["worlds"][0]["levels"][0]["unlock_after_level_id"] = "missing";
    require_error(parse_campaign_manifest(campaign.dump()), ContentErrorCode::MissingReference,
                  "/worlds/0/levels/0/unlock_after_level_id");
    for (const auto field : {"scene_id", "diorama_id", "text_id"})
    {
        campaign = campaign_manifest();
        if (std::string_view{field} == "scene_id")
            campaign["worlds"][0]["levels"][0][field] = "missing";
        else
            campaign["worlds"][0][field] = "missing";
        require_error(parse_campaign_manifest(campaign.dump()), ContentErrorCode::MissingReference,
                      std::string{"/worlds/0/"} +
                          (std::string_view{field} == "scene_id" ? "levels/0/" : "") + field);
    }
}

NINHO_SIM_TEST("product v2 content rejects trigger schema and resource violations")
{
    auto level = level_manifest();
    level["triggers"][0]["target_entity_id"] = 999;
    require_error(parse_level_manifest_v2(level.dump()), ContentErrorCode::MissingReference,
                  "/triggers/0/target_entity_id");
    level = level_manifest();
    level["triggers"].push_back(level["triggers"][0]);
    require_error(parse_level_manifest_v2(level.dump()), ContentErrorCode::DuplicateId,
                  "/triggers/1/id");
    for (const auto kind : {"temperature", "unknown"})
    {
        level = level_manifest();
        level["triggers"][0]["kind"] = kind;
        require_error(parse_level_manifest_v2(level.dump()), ContentErrorCode::InvalidEnum,
                      "/triggers/0/kind");
    }
    level = level_manifest();
    level["triggers"][0]["pressure_burst"]["radius_m"] = 1e300;
    require_error(parse_level_manifest_v2(level.dump()), ContentErrorCode::OutOfRange,
                  "/triggers/0/pressure_burst/radius_m");
    level = level_manifest();
    level["triggers"][0]["pressure_burst"]["max_bodies"] = 33;
    require_error(parse_level_manifest_v2(level.dump()), ContentErrorCode::OutOfRange,
                  "/triggers/0/pressure_burst/max_bodies");
}

NINHO_SIM_TEST("product v2 content rejects enemy body and objective semantic contradictions")
{
    const auto materials = parse_material_catalog_v2(material_catalog().dump());
    const auto archetypes = parse_archetype_catalog_v2(archetype_catalog().dump());
    const auto campaign = parse_campaign_manifest(campaign_manifest().dump());
    NINHO_SIM_REQUIRE(materials.ok() && archetypes.ok() && campaign.ok());
    const auto bundle_for = [&](const json& source) {
        const auto level = parse_level_manifest_v2(source.dump());
        NINHO_SIM_REQUIRE(level.ok());
        return make_product_v2_content_bundle(materials.value, archetypes.value, campaign.value,
                                              level.value);
    };

    auto level = level_manifest();
    level["bodies"][0]["body_type"] = "static";
    level["bodies"][0]["affected_by_world_gravity"] = false;
    require_error(bundle_for(level), ContentErrorCode::InvalidInvariant, "/bodies/0/body_type");

    level = level_manifest();
    level["bodies"][0]["material_id"] = 1;
    level["bodies"][0]["surface_id"] = nullptr;
    require_error(bundle_for(level), ContentErrorCode::InvalidInvariant, "/bodies/0/material_id");

    level = level_manifest();
    level["bodies"].push_back(static_body(2, 200));
    level["free_body_ids"].push_back(2);
    level["objectives"][0]["target_entity_id"] = 200;
    require_error(bundle_for(level), ContentErrorCode::MissingReference,
                  "/objectives/0/target_entity_id");
}

NINHO_SIM_TEST("product v2 content rejects joint ownership and disconnected assemblies")
{
    const auto materials = parse_material_catalog_v2(material_catalog().dump());
    const auto archetypes = parse_archetype_catalog_v2(archetype_catalog().dump());
    const auto campaign = parse_campaign_manifest(campaign_manifest().dump());
    const auto bundle_for = [&](const json& source) {
        const auto level = parse_level_manifest_v2(source.dump());
        NINHO_SIM_REQUIRE(level.ok());
        return make_product_v2_content_bundle(materials.value, archetypes.value, campaign.value,
                                              level.value);
    };

    auto level = level_with_two_assemblies();
    level["joints"][0]["body_b_id"] = 3;
    require_error(bundle_for(level), ContentErrorCode::InvalidInvariant, "/joints/0/body_b_id");

    level = level_with_two_assemblies();
    level["joints"].erase(1);
    level["assemblies"].erase(1);
    level["free_body_ids"] = json::array({3, 4});
    level["joints"][0]["body_b_id"] = 3;
    require_error(bundle_for(level), ContentErrorCode::InvalidInvariant, "/joints/0/body_b_id");

    level = level_with_two_assemblies();
    level["bodies"].erase(3);
    level["joints"].erase(1);
    level["assemblies"].erase(1);
    level["assemblies"][0]["body_ids"] = json::array({1, 2, 3});
    require_error(bundle_for(level), ContentErrorCode::InvalidInvariant, "/assemblies/0");
}

NINHO_SIM_TEST("product v2 content rejects vector float underflow and duplicate JSON keys")
{
    auto level = level_manifest();
    level["slingshot"]["rest_position_m"][0] = 1e-300;
    require_error(parse_level_manifest_v2(level.dump()), ContentErrorCode::OutOfRange,
                  "/slingshot/rest_position_m/0");

    level = level_manifest();
    level["slingshot"]["rest_position_m"][0] = 1e300;
    require_error(parse_level_manifest_v2(level.dump()), ContentErrorCode::OutOfRange,
                  "/slingshot/rest_position_m/0");

    auto duplicate_root = material_catalog().dump();
    const auto root_marker = duplicate_root.find("\"schema_version\":2");
    NINHO_SIM_REQUIRE(root_marker != std::string::npos);
    duplicate_root.insert(root_marker, "\"schema_version\":2,");
    require_error(parse_material_catalog_v2(duplicate_root), ContentErrorCode::DuplicateKey,
                  "/schema_version");

    auto duplicate_nested = level_manifest().dump();
    const auto nested_marker = duplicate_nested.find("\"radius_m\":4.0");
    NINHO_SIM_REQUIRE(nested_marker != std::string::npos);
    duplicate_nested.insert(nested_marker, "\"radius_m\":4.0,");
    require_error(parse_level_manifest_v2(duplicate_nested), ContentErrorCode::DuplicateKey,
                  "/triggers/0/pressure_burst/radius_m");
}

NINHO_SIM_TEST("product v2 content is fail closed and preserves versioned round trips")
{
    auto material = material_catalog();
    material["materials"][0]["surprise"] = true;
    require_error(parse_material_catalog(material.dump()), ContentErrorCode::UnknownKey,
                  "/materials/0/surprise");
    auto campaign_with_unknown = campaign_manifest();
    campaign_with_unknown["worlds"][0]["levels"][0]["surprise"] = true;
    require_error(parse_campaign_manifest(campaign_with_unknown.dump()), ContentErrorCode::UnknownKey,
                  "/worlds/0/levels/0/surprise");
    auto non_finite = level_manifest().dump();
    const auto marker = non_finite.find("1400.0");
    NINHO_SIM_REQUIRE(marker != std::string::npos);
    non_finite.replace(marker, 6U, "1e400");
    const auto non_finite_result = parse_level_manifest_v2(non_finite);
    NINHO_SIM_REQUIRE(!non_finite_result.ok());
    NINHO_SIM_REQUIRE(non_finite_result.error.code == ContentErrorCode::InvalidNumber ||
                      non_finite_result.error.code == ContentErrorCode::InvalidJson);
    const auto v2_materials = parse_material_catalog(material_catalog().dump());
    const auto v2_archetypes = parse_archetype_catalog(archetype_catalog().dump());
    const auto v2_level = parse_level_manifest(level_manifest().dump());
    const auto v2_campaign = parse_campaign_manifest(campaign_manifest().dump());
    NINHO_SIM_REQUIRE(v2_materials.ok() && v2_archetypes.ok() && v2_level.ok() && v2_campaign.ok());
    const auto material_json = to_canonical_json(v2_materials.value);
    const auto archetype_json = to_canonical_json(v2_archetypes.value);
    const auto level_json = to_canonical_json(v2_level.value);
    const auto campaign_json = to_canonical_json(v2_campaign.value);
    NINHO_SIM_REQUIRE(to_canonical_json(parse_material_catalog_v2(material_json).value) ==
                      material_json);
    NINHO_SIM_REQUIRE(to_canonical_json(parse_archetype_catalog_v2(archetype_json).value) ==
                      archetype_json);
    NINHO_SIM_REQUIRE(to_canonical_json(parse_level_manifest_v2(level_json).value) == level_json);
    NINHO_SIM_REQUIRE(to_canonical_json(parse_campaign_manifest(campaign_json).value) ==
                      campaign_json);
    NINHO_SIM_REQUIRE(!parse_material_catalog_v1(material_catalog().dump()).ok());
    NINHO_SIM_REQUIRE(!parse_level_manifest_v2(R"({"schema_version":1})").ok());
    require_error(make_content_bundle(v2_materials.value, v2_archetypes.value, v2_level.value),
                  ContentErrorCode::InvalidInvariant, "/schema_version");
}

NINHO_SIM_TEST("product v2 content enforces closed hull compound queue and trigger budgets")
{
    auto level = level_manifest();
    json vertices = json::array({json::array({0, 0, 0}), json::array({1, 0, 0}),
                                 json::array({0, 1, 0}), json::array({0, 0, 1})});
    while (vertices.size() < 64U)
        vertices.push_back(vertices.back());
    level["bodies"][0]["shape"] = {{"type", "convex_hull"}, {"vertices_m", vertices}};
    NINHO_SIM_REQUIRE(parse_level_manifest_v2(level.dump()).ok());
    level["bodies"][0]["shape"]["vertices_m"].push_back(json::array({1, 1, 1}));
    require_error(parse_level_manifest_v2(level.dump()), ContentErrorCode::ResourceLimit,
                  "/bodies/0/shape/vertices_m");

    level = level_manifest();
    level["bodies"][0]["shape"] = {{"type", "compound"}, {"children", json::array()}};
    for (int i = 0; i < 16; ++i)
        level["bodies"][0]["shape"]["children"].push_back(box_shape());
    NINHO_SIM_REQUIRE(parse_level_manifest_v2(level.dump()).ok());
    level["bodies"][0]["shape"]["children"].push_back(box_shape());
    require_error(parse_level_manifest_v2(level.dump()), ContentErrorCode::ResourceLimit,
                  "/bodies/0/shape/children");

    level = level_manifest();
    while (level["bird_queue"].size() < 32U)
        level["bird_queue"].push_back(1);
    NINHO_SIM_REQUIRE(parse_level_manifest_v2(level.dump()).ok());
    level["bird_queue"].push_back(1);
    require_error(parse_level_manifest_v2(level.dump()), ContentErrorCode::ResourceLimit,
                  "/bird_queue");

    level = level_manifest();
    const auto trigger = level["triggers"][0];
    while (level["triggers"].size() < 64U)
    {
        auto next = trigger;
        next["id"] = level["triggers"].size() + 1U;
        level["triggers"].push_back(std::move(next));
    }
    NINHO_SIM_REQUIRE(parse_level_manifest_v2(level.dump()).ok());
    auto overflow = trigger;
    overflow["id"] = 65;
    level["triggers"].push_back(std::move(overflow));
    require_error(parse_level_manifest_v2(level.dump()), ContentErrorCode::ResourceLimit,
                  "/triggers");
}

NINHO_SIM_TEST("product v2 content preserves optional compound child local transforms")
{
    auto level = level_manifest();
    level["bodies"][0]["shape"] = {
        {"type", "compound"},
        {"children", json::array({
            {{"type", "box"},
             {"half_extents_m", json::array({0.25, 0.125, 0.25})},
             {"local_transform",
              {{"position_m", json::array({-0.4, 0.0, 0.0})},
               {"rotation_xyzw", json::array({0.0, 0.0, 0.0, 1.0})}}}},
            {{"type", "box"},
             {"half_extents_m", json::array({0.25, 0.125, 0.25})},
             {"local_transform",
              {{"position_m", json::array({0.6, 0.2, 0.0})},
               {"rotation_xyzw", json::array({0.0, 0.0, 0.7071067811865476,
                                                0.7071067811865476})}}}},
        })},
    };
    const auto parsed = parse_level_manifest_v2(level.dump());
    if (!parsed.ok()) {
        ninho::simulation::test::fail(__FILE__, __LINE__,
            parsed.error.pointer + ": " + parsed.error.message);
    }
    const auto canonical = to_canonical_json(parsed.value);
    const auto reparsed = parse_level_manifest_v2(canonical);
    NINHO_SIM_REQUIRE(reparsed.ok());
    NINHO_SIM_REQUIRE(reparsed.value.bodies.front().shape
        == parsed.value.bodies.front().shape);
}

NINHO_SIM_TEST("product v2 content uses a typed terrestrial pig damage model")
{
    auto archetypes = archetype_catalog();
    archetypes["weakpoints"][0]["protected_multiplier"] = 1.0;
    archetypes["weakpoints"][0]["exposed_multiplier"] = 1.0;
    archetypes["enemies"][0]["damage_model"] = "terrestrial_pig";
    archetypes["enemies"][0]["mass_kg"] = 65.0;
    archetypes["enemies"][0]["damage_energy_j_per_kg"] = 18.0;
    archetypes["enemies"][0]["max_damage"] = 70.0;
    const auto parsed = parse_archetype_catalog_v2(archetypes.dump());
    NINHO_SIM_REQUIRE(parsed.ok());
    NINHO_SIM_REQUIRE(parsed.value.enemies.front().damage_model
        == EnemyDamageModel::TerrestrialPig);
    const auto roundtrip = parse_archetype_catalog_v2(to_canonical_json(parsed.value));
    NINHO_SIM_REQUIRE(roundtrip.ok());
    NINHO_SIM_REQUIRE(roundtrip.value.enemies.front().damage_model
        == EnemyDamageModel::TerrestrialPig);
}

NINHO_SIM_TEST("product v2 content accepts mass conserving authored fracture patterns only on materials")
{
    auto level = level_manifest();
    level["bodies"].push_back(static_body(2, 200));
    level["free_body_ids"].push_back(2);
    auto& body = level["bodies"][1];
    body["body_type"] = "dynamic";
    body["affected_by_world_gravity"] = true;
    body["density_kg_m3"] = 480.0;
    body["material_id"] = 1;
    body["surface_id"] = nullptr;
    body["enemy_archetype_id"] = nullptr;
    body["fracture_pattern"] = {
        {"physical_fragments", json::array({
            {{"ordinal", 1}, {"shape", {{"type", "box"},
                {"half_extents_m", json::array({0.25, 0.5, 0.5})}}},
             {"local_transform", {{"position_m", json::array({-0.25, 0.0, 0.0})},
                {"rotation_xyzw", json::array({0.0, 0.0, 0.0, 1.0})}}},
             {"density_kg_m3", 480.0}, {"visual_id", "fragment_left"}},
            {{"ordinal", 2}, {"shape", {{"type", "box"},
                {"half_extents_m", json::array({0.25, 0.5, 0.5})}}},
             {"local_transform", {{"position_m", json::array({0.25, 0.0, 0.0})},
                {"rotation_xyzw", json::array({0.0, 0.0, 0.0, 1.0})}}},
             {"density_kg_m3", 480.0}, {"visual_id", "fragment_right"}}
        })},
        {"cosmetic_asset_ids", json::array({"dust_puff"})}
    };
    const auto parsed = parse_level_manifest_v2(level.dump());
    if (!parsed.ok()) {
        ninho::simulation::test::fail(__FILE__, __LINE__,
            parsed.error.pointer + ": " + parsed.error.message);
    }
    NINHO_SIM_REQUIRE(parsed.value.bodies.at(1).fracture_pattern.has_value());
    NINHO_SIM_REQUIRE(parsed.value.bodies.at(1).fracture_pattern->physical_fragments.size()
        == 2U);
    const auto roundtrip = parse_level_manifest_v2(to_canonical_json(parsed.value));
    NINHO_SIM_REQUIRE(roundtrip.ok());

    auto shifted = level;
    shifted["bodies"][1]["fracture_pattern"]["physical_fragments"][0]
        ["local_transform"]["position_m"] = json::array({-0.1, 0.0, 0.0});
    shifted["bodies"][1]["fracture_pattern"]["physical_fragments"][1]
        ["local_transform"]["position_m"] = json::array({0.4, 0.0, 0.0});
    const auto shifted_level = parse_level_manifest_v2(shifted.dump());
    const auto materials = parse_material_catalog_v2(material_catalog().dump());
    auto archetypes_json = archetype_catalog();
    archetypes_json["abilities"] = json::array({archetypes_json["abilities"][0]});
    const auto archetypes = parse_archetype_catalog_v2(archetypes_json.dump());
    NINHO_SIM_REQUIRE(shifted_level.ok() && materials.ok() && archetypes.ok());
    auto baseline = SimulationSession::create(
        materials.value, archetypes.value, parsed.value);
    if (!baseline.ok()) {
        ninho::simulation::test::fail(__FILE__, __LINE__,
            baseline.error.pointer + ": " + baseline.error.message);
    }
    NINHO_SIM_REQUIRE(!SimulationSession::create(
        materials.value, archetypes.value, shifted_level.value).ok());

    level["bodies"][1]["enemy_archetype_id"] = 1;
    level["bodies"][1]["surface_id"] = 1002;
    level["bodies"][1]["material_id"] = nullptr;
    NINHO_SIM_REQUIRE(!parse_level_manifest_v2(level.dump()).ok());
}

NINHO_SIM_TEST("product v2 physical canonical ignores cosmetic fracture assets")
{
    auto level_json = level_manifest();
    auto body = static_body(2, 200);
    body["body_type"] = "dynamic";
    body["affected_by_world_gravity"] = true;
    body["density_kg_m3"] = 480.0;
    body["fracture_pattern"] = {
        {"physical_fragments", json::array({
            {{"ordinal", 1}, {"shape", {{"type", "box"},
                {"half_extents_m", json::array({0.25, 0.5, 0.5})}}},
             {"local_transform", {{"position_m", json::array({-0.25, 0.0, 0.0})},
                {"rotation_xyzw", json::array({0.0, 0.0, 0.0, 1.0})}}},
             {"density_kg_m3", 480.0}, {"visual_id", "left"}},
            {{"ordinal", 2}, {"shape", {{"type", "box"},
                {"half_extents_m", json::array({0.25, 0.5, 0.5})}}},
             {"local_transform", {{"position_m", json::array({0.25, 0.0, 0.0})},
                {"rotation_xyzw", json::array({0.0, 0.0, 0.0, 1.0})}}},
             {"density_kg_m3", 480.0}, {"visual_id", "right"}}
        })},
        {"cosmetic_asset_ids", json::array({"dust"})}
    };
    level_json["bodies"].push_back(body);
    level_json["free_body_ids"].push_back(2);

    const auto materials = parse_material_catalog_v2(material_catalog().dump());
    auto archetypes_json = archetype_catalog();
    archetypes_json["abilities"] = json::array({archetypes_json["abilities"][0]});
    const auto archetypes = parse_archetype_catalog_v2(archetypes_json.dump());
    const auto dust_level = parse_level_manifest_v2(level_json.dump());
    NINHO_SIM_REQUIRE(materials.ok() && archetypes.ok() && dust_level.ok());
    auto dust = SimulationSession::create(
        materials.value, archetypes.value, dust_level.value);
    NINHO_SIM_REQUIRE(dust.ok());

    level_json["bodies"][1]["fracture_pattern"]["cosmetic_asset_ids"]
        = json::array({"spark"});
    const auto spark_level = parse_level_manifest_v2(level_json.dump());
    NINHO_SIM_REQUIRE(spark_level.ok());
    auto spark = SimulationSession::create(
        materials.value, archetypes.value, spark_level.value);
    NINHO_SIM_REQUIRE(spark.ok());

    NINHO_SIM_REQUIRE(dust.value->canonical_state_v3()
        == spark.value->canonical_state_v3());
}

NINHO_SIM_TEST("product v2 content conserves authored convex hull mass by hull volume not its aabb")
{
    auto level = level_manifest();
    auto body = static_body(2, 200);
    body["body_type"] = "dynamic";
    body["affected_by_world_gravity"] = true;
    body["density_kg_m3"] = 600.0;
    body["shape"] = {{"type", "convex_hull"},
        {"vertices_m", json::array({
            json::array({0.0, 0.0, 0.0}), json::array({1.0, 0.0, 0.0}),
            json::array({0.0, 1.0, 0.0}), json::array({0.0, 0.0, 1.0})})}};
    body["fracture_pattern"] = {
        {"physical_fragments", json::array({{
            {"ordinal", 1},
            {"shape", {{"type", "box"},
                {"half_extents_m", json::array({0.5, 0.5, 0.5})}}},
            {"local_transform", {{"position_m", json::array({0.0, 0.0, 0.0})},
                {"rotation_xyzw", json::array({0.0, 0.0, 0.0, 1.0})}}},
            {"density_kg_m3", 100.0}, {"visual_id", "mass_equivalent"}}})},
        {"cosmetic_asset_ids", json::array()}};
    level["bodies"].push_back(std::move(body));
    level["free_body_ids"].push_back(2);

    const auto parsed = parse_level_manifest_v2(level.dump());
    if (!parsed.ok()) {
        ninho::simulation::test::fail(__FILE__, __LINE__,
            parsed.error.pointer + ": " + parsed.error.message);
    }

    level["bodies"][1]["density_kg_m3"] = 100.0;
    level["bodies"][1]["shape"] = {{"type", "convex_hull"},
        {"vertices_m", json::array({
            json::array({-1.0, -1.0, -1.0}), json::array({1.0, -1.0, -1.0}),
            json::array({-1.0, 1.0, -1.0}), json::array({1.0, 1.0, -1.0}),
            json::array({-1.0, -1.0, 1.0}), json::array({1.0, -1.0, 1.0}),
            json::array({-1.0, 1.0, 1.0}), json::array({1.0, 1.0, 1.0})})}};
    level["bodies"][1]["fracture_pattern"]["physical_fragments"][0]
        ["shape"]["half_extents_m"] = json::array({1.0, 1.0, 1.0});
    NINHO_SIM_REQUIRE(parse_level_manifest_v2(level.dump()).ok());
}

NINHO_SIM_TEST("product v2 content integrates ductile yield recreation and later break without duplicate overload")
{
    auto materials_json = material_catalog();
    materials_json["materials"].push_back({
        {"id", 3}, {"key", "wood"}, {"response", "fibrous"},
        {"density_kg_m3", 520.0}, {"friction", 0.6},
        {"restitution", 0.1}, {"toughness", 0.5}});
    auto level_json = level_manifest();
    auto first = static_body(2, 200);
    auto second = static_body(3, 300);
    for (auto* body : {&first, &second}) {
        (*body)["body_type"] = "dynamic";
        (*body)["affected_by_world_gravity"] = true;
        (*body)["shape"] = box_shape();
    }
    first["material_id"] = 3;
    first["density_kg_m3"] = 520.0;
    second["material_id"] = 2;
    second["density_kg_m3"] = 7800.0;
    first["transform"]["position_m"] = json::array({0.0, 3.0, 0.0});
    second["transform"]["position_m"] = json::array({0.0, 4.0, 0.0});
    level_json["bodies"].push_back(first);
    level_json["bodies"].push_back(second);
    level_json["joints"] = json::array({{{"id", 1}, {"assembly_id", 1},
        {"kind", "steel_ductile"}, {"body_a_id", 2}, {"body_b_id", 3},
        {"force_limit_n", 7500.0}, {"torque_limit_nm", 900.0}}});
    level_json["assemblies"] = json::array({{{"id", 1}, {"key", "steel_pair"},
        {"body_ids", json::array({2, 3})}, {"joint_ids", json::array({1})}}});
    level_json["triggers"] = json::array();
    const auto materials = parse_material_catalog_v2(materials_json.dump());
    auto archetypes_source = archetype_catalog();
    archetypes_source["abilities"] = json::array({archetypes_source["abilities"][0]});
    const auto archetypes = parse_archetype_catalog_v2(archetypes_source.dump());
    const auto level = parse_level_manifest_v2(level_json.dump());
    NINHO_SIM_REQUIRE(materials.ok() && archetypes.ok() && level.ok());
    auto created = SimulationSession::create(
        materials.value, archetypes.value, level.value);
    if (!created.ok()) ninho::simulation::test::fail(__FILE__, __LINE__,
        created.error.pointer + ": " + created.error.message);
    auto session = std::move(created.value);
    const auto initial_hash = session->canonical_hash_v3();

    NINHO_SIM_REQUIRE(detail::SessionTestFacade::queue_external_damage(
        *session, EntityId{100}, PartId{1}, 100.0, EventId{501}));
    detail::SessionTestFacade::override_joint_ratio_after_solver(
        *session, JointId{1}, 0.0);
    NINHO_SIM_REQUIRE(session->tick().ok());
    NINHO_SIM_REQUIRE(std::ranges::count(session->events(),
        DomainEventKind::DamageApplied, &DomainEvent::kind) == 1);

    NINHO_SIM_REQUIRE(detail::SessionTestFacade::queue_external_damage(
        *session, EntityId{100}, PartId{1}, 100.0, EventId{502}));
    detail::SessionTestFacade::override_joint_ratio_after_solver(
        *session, JointId{1}, 1.0);
    NINHO_SIM_REQUIRE(session->tick().ok());
    NINHO_SIM_REQUIRE(std::ranges::count(session->events(),
        DomainEventKind::MaterialYielded, &DomainEvent::kind) == 1);
    NINHO_SIM_REQUIRE(std::ranges::count(session->events(),
        DomainEventKind::JointOverloaded, &DomainEvent::kind) == 0);
    const auto yielded_event = std::ranges::find(
        session->events(), DomainEventKind::MaterialYielded, &DomainEvent::kind);
    NINHO_SIM_REQUIRE(yielded_event != session->events().end());
    NINHO_SIM_REQUIRE(yielded_event->cause_event_id == EventId{});
    NINHO_SIM_REQUIRE(yielded_event->affected_entity_id == EntityId{300});
    NINHO_SIM_REQUIRE(yielded_event->affected_part_id == PartId{1});
    NINHO_SIM_REQUIRE(yielded_event->material_id == MaterialId{2});
    const auto score_event = std::ranges::find(
        session->events(), DomainEventKind::ScoreAwarded, &DomainEvent::kind);
    NINHO_SIM_REQUIRE(score_event != session->events().end());
    NINHO_SIM_REQUIRE(score_event->scoring_identity_kind
        == ScoringIdentityKind::MaterialPiece);
    NINHO_SIM_REQUIRE(score_event->affected_entity_id == EntityId{300});
    NINHO_SIM_REQUIRE(score_event->affected_part_id == PartId{1});
    NINHO_SIM_REQUIRE(score_event->base_points == 220U);
    NINHO_SIM_REQUIRE(score_event->awarded_points == 220U);
    NINHO_SIM_REQUIRE(session->structural_joints().front().active);
    const auto yielded_hash = session->canonical_hash_v3();
    NINHO_SIM_REQUIRE(yielded_hash != initial_hash);

    detail::SessionTestFacade::override_joint_ratio_after_solver(
        *session, JointId{1}, 0.0);
    NINHO_SIM_REQUIRE(session->tick().ok());
    NINHO_SIM_REQUIRE(session->structural_joints().front().id == JointId{1});
    NINHO_SIM_REQUIRE(session->structural_joints().front().active);
    const auto recreated_hash = session->canonical_hash_v3();
    NINHO_SIM_REQUIRE(recreated_hash != yielded_hash);

    detail::SessionTestFacade::override_joint_ratio_after_solver(
        *session, JointId{1}, 7500.0 / 3200.0);
    NINHO_SIM_REQUIRE(session->tick().ok());
    NINHO_SIM_REQUIRE(std::ranges::count(session->events(),
        DomainEventKind::JointBroken, &DomainEvent::kind) == 0);
    NINHO_SIM_REQUIRE(session->tick().ok());
    NINHO_SIM_REQUIRE(std::ranges::count(session->events(),
        DomainEventKind::JointBroken, &DomainEvent::kind) == 1);
    NINHO_SIM_REQUIRE(!session->structural_joints().front().active);
    NINHO_SIM_REQUIRE(session->canonical_hash_v3() != recreated_hash);
}

NINHO_SIM_TEST("product v2 content scores the ductile owner when sheet is endpoint a")
{
    auto materials_json = material_catalog();
    materials_json["materials"].push_back({
        {"id", 3}, {"key", "wood"}, {"response", "fibrous"},
        {"density_kg_m3", 520.0}, {"friction", 0.6},
        {"restitution", 0.1}, {"toughness", 0.5}});
    auto level_json = level_manifest();
    auto sheet = static_body(2, 200);
    auto wood = static_body(3, 300);
    for (auto* body : {&sheet, &wood}) {
        (*body)["body_type"] = "dynamic";
        (*body)["affected_by_world_gravity"] = true;
        (*body)["shape"] = box_shape();
    }
    sheet["material_id"] = 2;
    sheet["density_kg_m3"] = 7800.0;
    wood["material_id"] = 3;
    wood["density_kg_m3"] = 520.0;
    level_json["bodies"].push_back(sheet);
    level_json["bodies"].push_back(wood);
    level_json["joints"] = json::array({{{"id", 1}, {"assembly_id", 1},
        {"kind", "steel_ductile"}, {"body_a_id", 2}, {"body_b_id", 3},
        {"force_limit_n", 7500.0}, {"torque_limit_nm", 900.0}}});
    level_json["assemblies"] = json::array({{{"id", 1}, {"key", "sheet_a"},
        {"body_ids", json::array({2, 3})}, {"joint_ids", json::array({1})}}});
    level_json["triggers"] = json::array();

    const auto materials = parse_material_catalog_v2(materials_json.dump());
    auto archetypes_json = archetype_catalog();
    archetypes_json["abilities"] = json::array({archetypes_json["abilities"][0]});
    const auto archetypes = parse_archetype_catalog_v2(archetypes_json.dump());
    const auto level = parse_level_manifest_v2(level_json.dump());
    NINHO_SIM_REQUIRE(materials.ok() && archetypes.ok() && level.ok());
    auto created = SimulationSession::create(
        materials.value, archetypes.value, level.value);
    NINHO_SIM_REQUIRE(created.ok());
    detail::SessionTestFacade::override_joint_ratio_after_solver(
        *created.value, JointId{1}, 1.0);
    NINHO_SIM_REQUIRE(created.value->tick().ok());
    const auto yielded = std::ranges::find(created.value->events(),
        DomainEventKind::MaterialYielded, &DomainEvent::kind);
    const auto awarded = std::ranges::find(created.value->events(),
        DomainEventKind::ScoreAwarded, &DomainEvent::kind);
    NINHO_SIM_REQUIRE(yielded != created.value->events().end());
    NINHO_SIM_REQUIRE(awarded != created.value->events().end());
    NINHO_SIM_REQUIRE(yielded->affected_entity_id == EntityId{200});
    NINHO_SIM_REQUIRE(yielded->material_id == MaterialId{2});
    NINHO_SIM_REQUIRE(awarded->affected_entity_id == EntityId{200});
    NINHO_SIM_REQUIRE(awarded->base_points == 220U);
}

NINHO_SIM_TEST("product v2 content rejects ambiguous ductile joint ownership atomically")
{
    auto materials_json = material_catalog();
    materials_json["materials"].push_back({
        {"id", 3}, {"key", "wood"}, {"response", "fibrous"},
        {"density_kg_m3", 520.0}, {"friction", 0.6},
        {"restitution", 0.1}, {"toughness", 0.5}});
    const auto materials = parse_material_catalog_v2(materials_json.dump());
    auto archetypes_json = archetype_catalog();
    archetypes_json["abilities"] = json::array({archetypes_json["abilities"][0]});
    const auto archetypes = parse_archetype_catalog_v2(archetypes_json.dump());
    NINHO_SIM_REQUIRE(materials.ok() && archetypes.ok());

    const auto manifest = [](std::uint32_t material_a,
                              std::uint32_t material_b) {
        auto level_json = level_manifest();
        auto first = static_body(2, 200);
        auto second = static_body(3, 300);
        for (auto* body : {&first, &second}) {
            (*body)["body_type"] = "dynamic";
            (*body)["affected_by_world_gravity"] = true;
            (*body)["shape"] = box_shape();
        }
        first["material_id"] = material_a;
        first["density_kg_m3"] = material_a == 2U ? 7800.0 : 520.0;
        second["material_id"] = material_b;
        second["density_kg_m3"] = material_b == 2U ? 7800.0 : 520.0;
        level_json["bodies"].push_back(first);
        level_json["bodies"].push_back(second);
        level_json["joints"] = json::array({{{"id", 1}, {"assembly_id", 1},
            {"kind", "steel_ductile"}, {"body_a_id", 2}, {"body_b_id", 3},
            {"force_limit_n", 7500.0}, {"torque_limit_nm", 900.0}}});
        level_json["assemblies"] = json::array({{{"id", 1}, {"key", "ownership"},
            {"body_ids", json::array({2, 3})}, {"joint_ids", json::array({1})}}});
        level_json["triggers"] = json::array();
        const auto parsed = parse_level_manifest_v2(level_json.dump());
        NINHO_SIM_REQUIRE(parsed.ok());
        return parsed.value;
    };

    const LevelManifest valid = manifest(2U, 3U);
    auto session = SimulationSession::create(
        materials.value, archetypes.value, valid);
    NINHO_SIM_REQUIRE(session.ok());
    const auto canonical = session.value->canonical_state_v3();
    for (const LevelManifest invalid : {manifest(3U, 3U), manifest(2U, 2U)}) {
        const auto rejected = SimulationSession::create(
            materials.value, archetypes.value, invalid);
        require_error(rejected, ContentErrorCode::InvalidInvariant,
            "/joints/0/kind");
        const SessionStatus status = session.value->reconfigure(
            materials.value, archetypes.value, invalid);
        NINHO_SIM_REQUIRE(!status.ok());
        NINHO_SIM_REQUIRE(status.error.pointer == "/joints/0/kind");
        NINHO_SIM_REQUIRE(session.value->canonical_state_v3() == canonical);
    }
}

NINHO_SIM_TEST("product v2 content integrates compound pig mass crush causality and reset")
{
    auto materials_json = material_catalog();
    materials_json["surfaces"][1]["density_kg_m3"] = 65.0;
    materials_json["surfaces"][1]["friction"] = 0.65;
    materials_json["surfaces"][1]["restitution"] = 0.05;
    auto archetypes_json = archetype_catalog();
    archetypes_json["weakpoints"][0]["protected_multiplier"] = 1.0;
    archetypes_json["weakpoints"][0]["exposed_multiplier"] = 1.0;
    archetypes_json["enemies"][0]["damage_model"] = "terrestrial_pig";
    archetypes_json["enemies"][0]["mass_kg"] = 65.0;
    archetypes_json["enemies"][0]["damage_energy_j_per_kg"] = 18.0;
    archetypes_json["enemies"][0]["max_damage"] = 70.0;
    archetypes_json["abilities"] = json::array({archetypes_json["abilities"][0]});
    auto level_json = level_manifest();
    level_json["bodies"][0]["density_kg_m3"] = 65.0;
    level_json["bodies"][0]["shape"] = {
        {"type", "compound"}, {"children", json::array({
            {{"type", "box"}, {"half_extents_m", json::array({0.25, 0.5, 0.5})},
             {"local_transform", {{"position_m", json::array({-0.25, 0.0, 0.0})},
                {"rotation_xyzw", json::array({0.0, 0.0, 0.0, 1.0})}}}},
            {{"type", "box"}, {"half_extents_m", json::array({0.25, 0.5, 0.5})},
             {"local_transform", {{"position_m", json::array({0.25, 0.0, 0.0})},
                {"rotation_xyzw", json::array({0.0, 0.0, 0.0, 1.0})}}}}
        })}};
    const auto materials = parse_material_catalog_v2(materials_json.dump());
    const auto archetypes = parse_archetype_catalog_v2(archetypes_json.dump());
    const auto level = parse_level_manifest_v2(level_json.dump());
    NINHO_SIM_REQUIRE(materials.ok() && archetypes.ok() && level.ok());
    auto created = SimulationSession::create(
        materials.value, archetypes.value, level.value);
    if (!created.ok()) ninho::simulation::test::fail(__FILE__, __LINE__,
        created.error.pointer + ": " + created.error.message);
    auto session = std::move(created.value);
    const auto pig = std::ranges::find(session->snapshots(),
        EnemyArchetypeId{1}, &EntitySnapshot::enemy_archetype_id);
    NINHO_SIM_REQUIRE(pig != session->snapshots().end());
    NINHO_SIM_REQUIRE(std::abs(pig->mass_kg - 65.0) <= 0.065);
    const auto initial_hash = session->canonical_hash_v3();
    const double load = pig->mass_kg * 9.81 / 60.0 * 5.0;
    std::optional<DomainEvent> crush;
    std::optional<DomainEvent> damage;
    for (int tick = 0; tick < 21; ++tick) {
        detail::SessionTestFacade::inject_crush_load(
            *session, pig->entity_id, pig->part_id, load);
        NINHO_SIM_REQUIRE(session->tick().ok());
        if (tick == 0) {
            NINHO_SIM_REQUIRE(session->canonical_hash_v3() != initial_hash);
        }
        for (const auto& event : session->events()) {
            if (event.kind == DomainEventKind::CrushDamageApplied) crush = event;
            if (event.kind == DomainEventKind::DamageApplied
                && crush && event.cause_event_id == crush->id) damage = event;
        }
    }
    NINHO_SIM_REQUIRE(crush.has_value() && damage.has_value());
    NINHO_SIM_REQUIRE(damage->cause_event_id == crush->id);
    NINHO_SIM_REQUIRE(std::abs(damage->damage - 5.3055) < 0.02);
    NINHO_SIM_REQUIRE(session->canonical_hash_v3() != initial_hash);

    auto invalid_mass = level.value;
    invalid_mass.bodies.front().density_kg_m3 = 64.0;
    NINHO_SIM_REQUIRE(!SimulationSession::create(
        materials.value, archetypes.value, invalid_mass).ok());
}

NINHO_SIM_TEST("product v2 terrestrial pig mass is authoritative per multi body entity")
{
    auto materials_json = material_catalog();
    materials_json["surfaces"][1]["density_kg_m3"] = 65.0;
    materials_json["surfaces"][1]["friction"] = 0.65;
    materials_json["surfaces"][1]["restitution"] = 0.05;
    auto archetypes_json = archetype_catalog();
    archetypes_json["weakpoints"][0]["protected_multiplier"] = 1.0;
    archetypes_json["weakpoints"][0]["exposed_multiplier"] = 1.0;
    archetypes_json["enemies"][0]["damage_model"] = "terrestrial_pig";
    archetypes_json["enemies"][0]["mass_kg"] = 65.0;
    archetypes_json["enemies"][0]["damage_energy_j_per_kg"] = 18.0;
    archetypes_json["enemies"][0]["max_damage"] = 70.0;
    archetypes_json["abilities"] = json::array({archetypes_json["abilities"][0]});
    auto level_json = level_manifest();
    level_json["triggers"] = json::array();
    level_json["bodies"][0]["density_kg_m3"] = 32.5;
    auto second_part = level_json["bodies"][0];
    second_part["body_id"] = 2;
    second_part["part_id"] = 2;
    second_part["transform"]["position_m"] = json::array({1.25, 0.0, 0.0});
    level_json["bodies"].push_back(second_part);
    level_json["free_body_ids"].push_back(2);

    const auto materials = parse_material_catalog_v2(materials_json.dump());
    const auto archetypes = parse_archetype_catalog_v2(archetypes_json.dump());
    const auto level = parse_level_manifest_v2(level_json.dump());
    NINHO_SIM_REQUIRE(materials.ok() && archetypes.ok() && level.ok());
    auto created = SimulationSession::create(
        materials.value, archetypes.value, level.value);
    if (!created.ok()) ninho::simulation::test::fail(__FILE__, __LINE__,
        created.error.pointer + ": " + created.error.message);

    double entity_mass = 0.0;
    for (const auto& snapshot : created.value->snapshots()) {
        if (snapshot.entity_id == EntityId{100}) entity_mass += snapshot.mass_kg;
    }
    NINHO_SIM_REQUIRE(std::abs(entity_mass - 65.0) <= 0.065);

    const double half_of_five_weights = entity_mass * 9.81 / 60.0 * 2.5;
    for (int tick = 0; tick < 21; ++tick) {
        detail::SessionTestFacade::inject_crush_load(
            *created.value, EntityId{100}, PartId{1}, half_of_five_weights);
        detail::SessionTestFacade::inject_crush_load(
            *created.value, EntityId{100}, PartId{2}, half_of_five_weights);
        NINHO_SIM_REQUIRE(created.value->tick().ok());
    }
    const auto crush_count = std::ranges::count(created.value->events(),
        DomainEventKind::CrushDamageApplied, &DomainEvent::kind);
    NINHO_SIM_REQUIRE(crush_count == 1);
    const auto crush = std::ranges::find(created.value->events(),
        DomainEventKind::CrushDamageApplied, &DomainEvent::kind);
    NINHO_SIM_REQUIRE(crush != created.value->events().end());
    const auto damage = std::ranges::find_if(created.value->events(),
        [&](const DomainEvent& event) {
            return event.kind == DomainEventKind::DamageApplied
                && event.cause_event_id == crush->id;
        });
    NINHO_SIM_REQUIRE(damage != created.value->events().end());
    NINHO_SIM_REQUIRE(std::abs(damage->damage - 5.3055) < 0.02);
}

NINHO_SIM_TEST("product v2 content replaces an authored fracture with physical children under gravity")
{
    auto level_json = level_manifest();
    auto parent = static_body(2, 200);
    parent["body_type"] = "dynamic";
    parent["affected_by_world_gravity"] = true;
    parent["density_kg_m3"] = 120.0;
    parent["transform"]["position_m"] = json::array({0.0, 5.0, 0.0});
    parent["fracture_pattern"] = {
        {"physical_fragments", json::array({
            {{"ordinal", 1}, {"shape", {{"type", "box"},
                {"half_extents_m", json::array({0.25, 0.5, 0.5})}}},
             {"local_transform", {{"position_m", json::array({-0.25, 0.0, 0.0})},
                {"rotation_xyzw", json::array({0.0, 0.0, 0.0, 1.0})}}},
             {"density_kg_m3", 120.0}, {"visual_id", "left"}},
            {{"ordinal", 2}, {"shape", {{"type", "box"},
                {"half_extents_m", json::array({0.25, 0.5, 0.5})}}},
             {"local_transform", {{"position_m", json::array({0.25, 0.0, 0.0})},
                {"rotation_xyzw", json::array({0.0, 0.0, 0.0, 1.0})}}},
             {"density_kg_m3", 120.0}, {"visual_id", "right"}}
        })}, {"cosmetic_asset_ids", json::array({"dust"})}};
    auto anchor = static_body(3, 300);
    anchor["transform"]["position_m"] = json::array({0.0, 4.0, 0.0});
    level_json["bodies"].push_back(parent);
    level_json["bodies"].push_back(anchor);
    level_json["joints"] = json::array({{{"id", 1}, {"assembly_id", 1},
        {"kind", "pine_fit"}, {"body_a_id", 2}, {"body_b_id", 3},
        {"force_limit_n", 1000000.0}, {"torque_limit_nm", 1000000.0}}});
    level_json["assemblies"] = json::array({{{"id", 1}, {"key", "fracturable"},
        {"body_ids", json::array({2, 3})}, {"joint_ids", json::array({1})}}});
    const auto materials = parse_material_catalog_v2(material_catalog().dump());
    auto archetypes_source = archetype_catalog();
    archetypes_source["abilities"] = json::array({archetypes_source["abilities"][0]});
    const auto archetypes = parse_archetype_catalog_v2(archetypes_source.dump());
    const auto level = parse_level_manifest_v2(level_json.dump());
    NINHO_SIM_REQUIRE(materials.ok() && archetypes.ok() && level.ok());
    auto created = SimulationSession::create(
        materials.value, archetypes.value, level.value);
    if (!created.ok()) ninho::simulation::test::fail(__FILE__, __LINE__,
        created.error.pointer + ": " + created.error.message);
    auto session = std::move(created.value);
    detail::SessionTestFacade::fracture_piece_after_solver(
        *session, EntityId{200}, PartId{1}, {0.0f, 5.0f, 0.0f});
    detail::SessionTestFacade::override_joint_ratio_after_solver(
        *session, JointId{1}, 0.0);
    NINHO_SIM_REQUIRE(session->tick().ok());
    NINHO_SIM_REQUIRE(session->tick().ok());
    std::vector<EntitySnapshot> children;
    for (const auto& snapshot : session->snapshots()) {
        if (snapshot.entity_id == EntityId{200}) children.push_back(snapshot);
    }
    NINHO_SIM_REQUIRE(children.size() == 2U);
    NINHO_SIM_REQUIRE(std::ranges::none_of(children, [](const auto& child) {
        return child.part_id == PartId{1};
    }));
    const double mass = children[0].mass_kg + children[1].mass_kg;
    NINHO_SIM_REQUIRE(std::abs(mass - 120.0) < 0.12);
    NINHO_SIM_REQUIRE(children[0].linear_velocity_m_s.y < 0.0f);
    NINHO_SIM_REQUIRE(children[1].linear_velocity_m_s.y < 0.0f);
}

NINHO_SIM_TEST("product v2 fracture velocity uses each authored fragment center of mass")
{
    auto level_json = level_manifest();
    auto parent = static_body(2, 200);
    parent["body_type"] = "dynamic";
    parent["affected_by_world_gravity"] = true;
    parent["density_kg_m3"] = 600.0;
    parent["transform"]["position_m"] = json::array({10.0, 15.0, 0.0});
    const json off_center_hull = {{"type", "convex_hull"},
        {"vertices_m", json::array({
            json::array({0.0, 0.0, 0.0}), json::array({1.0, 0.0, 0.0}),
            json::array({0.0, 1.0, 0.0}), json::array({0.0, 0.0, 1.0})})}};
    parent["shape"] = off_center_hull;
    parent["fracture_pattern"] = {
        {"physical_fragments", json::array({{{"ordinal", 1},
            {"shape", off_center_hull},
            {"local_transform", {{"position_m", json::array({0.0, 0.0, 0.0})},
                {"rotation_xyzw", json::array({0.0, 0.0, 0.0, 1.0})}}},
            {"density_kg_m3", 600.0}, {"visual_id", "whole_hull"}}})},
        {"cosmetic_asset_ids", json::array()}};
    auto anchor = static_body(3, 300);
    anchor["transform"]["position_m"] = json::array({10.0, 13.0, 0.0});
    level_json["bodies"].push_back(parent);
    level_json["bodies"].push_back(anchor);
    level_json["joints"] = json::array({{{"id", 1}, {"assembly_id", 1},
        {"kind", "pine_fit"}, {"body_a_id", 2}, {"body_b_id", 3},
        {"force_limit_n", 1000000000.0}, {"torque_limit_nm", 1000000000.0}}});
    level_json["assemblies"] = json::array({{{"id", 1}, {"key", "fracturable"},
        {"body_ids", json::array({2, 3})}, {"joint_ids", json::array({1})}}});

    const auto materials = parse_material_catalog_v2(material_catalog().dump());
    auto archetypes_json = archetype_catalog();
    archetypes_json["abilities"] = json::array({archetypes_json["abilities"][0]});
    const auto archetypes = parse_archetype_catalog_v2(archetypes_json.dump());
    const auto level = parse_level_manifest_v2(level_json.dump());
    NINHO_SIM_REQUIRE(materials.ok() && archetypes.ok() && level.ok());
    auto created = SimulationSession::create(
        materials.value, archetypes.value, level.value);
    if (!created.ok()) ninho::simulation::test::fail(__FILE__, __LINE__,
        created.error.pointer + ": " + created.error.message);
    auto session = std::move(created.value);

    NINHO_SIM_REQUIRE(detail::SessionTestFacade::impulse_entity(
        *session, EntityId{200}, {0.0f, 10000.0f, 0.0f}));
    detail::SessionTestFacade::fracture_piece_after_solver(
        *session, EntityId{200}, PartId{1}, {10.0f, 15.0f, 0.0f});
    detail::SessionTestFacade::override_joint_ratio_after_solver(
        *session, JointId{1}, 0.0);
    NINHO_SIM_REQUIRE(session->tick().ok());
    const auto parent_snapshot = std::ranges::find(
        session->snapshots(), EntityId{200}, &EntitySnapshot::entity_id);
    NINHO_SIM_REQUIRE(parent_snapshot != session->snapshots().end());
    NINHO_SIM_REQUIRE(ninho::physics::length(
        parent_snapshot->angular_velocity_rad_s) > 1.0e-4f);
    const auto parent_center = detail::SessionTestFacade::body_center_of_mass(
        *session, EntityId{200}, PartId{1});
    NINHO_SIM_REQUIRE(parent_center.has_value());
    NINHO_SIM_REQUIRE(ninho::physics::length(ninho::physics::cross(
        parent_snapshot->angular_velocity_rad_s,
        parent_snapshot->transform.position - *parent_center)) > 1.0e-2f);

    const auto parent_velocity = parent_snapshot->linear_velocity_m_s;
    const auto gravity = detail::SessionTestFacade::gravity_at(*session, *parent_center);
    NINHO_SIM_REQUIRE(session->tick().ok());
    const auto child = std::ranges::find(
        session->snapshots(), EntityId{200}, &EntitySnapshot::entity_id);
    NINHO_SIM_REQUIRE(child != session->snapshots().end());
    const auto expected = parent_velocity + gravity * (1.0f / 60.0f);
    NINHO_SIM_REQUIRE(ninho::physics::length(
        child->linear_velocity_m_s - expected) < 1.0e-3f);
}

NINHO_SIM_TEST("product v2 allows large authored fracture catalogs but caps active fragments atomically")
{
    auto level_json = level_manifest();
    const auto patterned_body = [](std::uint32_t body_id,
        std::uint32_t entity_id, double x) {
        auto body = static_body(body_id, entity_id);
        body["body_type"] = "dynamic";
        body["affected_by_world_gravity"] = true;
        body["density_kg_m3"] = 120.0;
        body["transform"]["position_m"] = json::array({x, 8.0, 0.0});
        json fragments = json::array();
        constexpr std::size_t count = 41U;
        const double half_width = 0.5 / static_cast<double>(count);
        for (std::size_t index = 0; index < count; ++index) {
            const double center = -0.5 + half_width
                + 2.0 * half_width * static_cast<double>(index);
            fragments.push_back({
                {"ordinal", index + 1U},
                {"shape", {{"type", "box"},
                    {"half_extents_m", json::array({half_width, 0.5, 0.5})}}},
                {"local_transform", {{"position_m", json::array({center, 0.0, 0.0})},
                    {"rotation_xyzw", json::array({0.0, 0.0, 0.0, 1.0})}}},
                {"density_kg_m3", 120.0},
                {"visual_id", "fragment_" + std::to_string(index + 1U)}});
        }
        body["fracture_pattern"] = {
            {"physical_fragments", std::move(fragments)},
            {"cosmetic_asset_ids", json::array()}};
        return body;
    };
    auto first = patterned_body(2, 200, 8.0);
    auto second = patterned_body(3, 300, 18.0);
    auto first_anchor = static_body(4, 201);
    first_anchor["transform"]["position_m"] = json::array({8.0, 6.0, 0.0});
    auto second_anchor = static_body(5, 301);
    second_anchor["transform"]["position_m"] = json::array({18.0, 6.0, 0.0});
    level_json["bodies"].push_back(first);
    level_json["bodies"].push_back(second);
    level_json["bodies"].push_back(first_anchor);
    level_json["bodies"].push_back(second_anchor);
    level_json["joints"] = json::array({
        {{"id", 1}, {"assembly_id", 1}, {"kind", "pine_fit"},
            {"body_a_id", 2}, {"body_b_id", 4},
            {"force_limit_n", 1000000000.0}, {"torque_limit_nm", 1000000000.0}},
        {{"id", 2}, {"assembly_id", 2}, {"kind", "pine_fit"},
            {"body_a_id", 3}, {"body_b_id", 5},
            {"force_limit_n", 1000000000.0}, {"torque_limit_nm", 1000000000.0}}});
    level_json["assemblies"] = json::array({
        {{"id", 1}, {"key", "first"}, {"body_ids", json::array({2, 4})},
            {"joint_ids", json::array({1})}},
        {{"id", 2}, {"key", "second"}, {"body_ids", json::array({3, 5})},
            {"joint_ids", json::array({2})}}});

    const auto materials = parse_material_catalog_v2(material_catalog().dump());
    auto archetypes_json = archetype_catalog();
    archetypes_json["abilities"] = json::array({archetypes_json["abilities"][0]});
    const auto archetypes = parse_archetype_catalog_v2(archetypes_json.dump());
    const auto level = parse_level_manifest_v2(level_json.dump());
    NINHO_SIM_REQUIRE(materials.ok() && archetypes.ok() && level.ok());
    auto created = SimulationSession::create(
        materials.value, archetypes.value, level.value);
    if (!created.ok()) ninho::simulation::test::fail(__FILE__, __LINE__,
        created.error.pointer + ": " + created.error.message);
    auto session = std::move(created.value);

    detail::SessionTestFacade::fracture_piece_after_solver(
        *session, EntityId{200}, PartId{1}, {8.0f, 8.0f, 0.0f});
    detail::SessionTestFacade::fracture_piece_after_solver(
        *session, EntityId{300}, PartId{1}, {18.0f, 8.0f, 0.0f});
    detail::SessionTestFacade::override_joint_ratio_after_solver(
        *session, JointId{1}, 0.0);
    detail::SessionTestFacade::override_joint_ratio_after_solver(
        *session, JointId{2}, 0.0);
    NINHO_SIM_REQUIRE(session->tick().ok());
    NINHO_SIM_REQUIRE(session->tick().ok());

    const auto first_count = std::ranges::count(
        session->snapshots(), EntityId{200}, &EntitySnapshot::entity_id);
    const auto second_count = std::ranges::count(
        session->snapshots(), EntityId{300}, &EntitySnapshot::entity_id);
    NINHO_SIM_REQUIRE(first_count == 41);
    NINHO_SIM_REQUIRE(second_count == 1);
    const auto second_parent = std::ranges::find(
        session->snapshots(), EntityId{300}, &EntitySnapshot::entity_id);
    NINHO_SIM_REQUIRE(second_parent != session->snapshots().end());
    NINHO_SIM_REQUIRE(second_parent->part_id == PartId{1});
}

NINHO_SIM_TEST("product v2 content releases physical fragment budget after bounds cleanup across waves")
{
    auto level_json = level_manifest();
    const auto patterned_body = [](std::uint32_t body_id,
        std::uint32_t entity_id, double x, std::size_t count) {
        auto body = static_body(body_id, entity_id);
        body["body_type"] = "dynamic";
        body["affected_by_world_gravity"] = true;
        body["density_kg_m3"] = 120.0;
        body["transform"]["position_m"] = json::array({x, -11.99, 0.0});
        json fragments = json::array();
        const double half_width = 0.5 / static_cast<double>(count);
        for (std::size_t index = 0; index < count; ++index) {
            const double center = -0.5 + half_width
                + 2.0 * half_width * static_cast<double>(index);
            fragments.push_back({
                {"ordinal", index + 1U},
                {"shape", {{"type", "box"},
                    {"half_extents_m", json::array({half_width, 0.5, 0.5})}}},
                {"local_transform", {{"position_m", json::array({center, 0.0, 0.0})},
                    {"rotation_xyzw", json::array({0.0, 0.0, 0.0, 1.0})}}},
                {"density_kg_m3", 120.0},
                {"visual_id", "fragment_" + std::to_string(index + 1U)}});
        }
        body["fracture_pattern"] = {
            {"physical_fragments", std::move(fragments)},
            {"cosmetic_asset_ids", json::array()}};
        return body;
    };

    constexpr std::array<EntityId, 3> entities{
        EntityId{200}, EntityId{300}, EntityId{400}};
    constexpr std::array<JointId, 3> joints{
        JointId{1}, JointId{2}, JointId{3}};
    for (std::size_t index = 0; index < entities.size(); ++index) {
        const std::uint32_t body_id = 2U + static_cast<std::uint32_t>(index);
        const std::uint32_t anchor_id = 5U + static_cast<std::uint32_t>(index);
        const std::uint32_t anchor_entity = 201U
            + 100U * static_cast<std::uint32_t>(index);
        const double x = 8.0 + 10.0 * static_cast<double>(index);
        const std::size_t fragment_count = index < 2U ? 80U : 1U;
        level_json["bodies"].push_back(patterned_body(
            body_id, entities[index].value(), x, fragment_count));
        auto anchor = static_body(anchor_id, anchor_entity);
        anchor["transform"]["position_m"] = json::array({x, -9.0, 0.0});
        level_json["bodies"].push_back(std::move(anchor));
        level_json["joints"].push_back({
            {"id", joints[index].value()},
            {"assembly_id", index + 1U},
            {"kind", "pine_fit"},
            {"body_a_id", body_id},
            {"body_b_id", anchor_id},
            {"force_limit_n", 1000000000.0},
            {"torque_limit_nm", 1000000000.0}});
        level_json["assemblies"].push_back({
            {"id", index + 1U},
            {"key", "wave_" + std::to_string(index + 1U)},
            {"body_ids", json::array({body_id, anchor_id})},
            {"joint_ids", json::array({joints[index].value()})}});
    }

    const auto materials = parse_material_catalog_v2(material_catalog().dump());
    auto archetypes_json = archetype_catalog();
    archetypes_json["abilities"] = json::array({archetypes_json["abilities"][0]});
    const auto archetypes = parse_archetype_catalog_v2(archetypes_json.dump());
    const auto level = parse_level_manifest_v2(level_json.dump());
    NINHO_SIM_REQUIRE(materials.ok() && archetypes.ok() && level.ok());
    auto created = SimulationSession::create(
        materials.value, archetypes.value, level.value);
    if (!created.ok()) ninho::simulation::test::fail(__FILE__, __LINE__,
        created.error.pointer + ": " + created.error.message);
    auto session = std::move(created.value);
    const std::size_t initial_body_records =
        detail::SessionTestFacade::body_record_count(*session);

    for (std::size_t index = 0; index < entities.size(); ++index) {
        detail::SessionTestFacade::fracture_piece_after_solver(
            *session, entities[index], PartId{1},
            {8.0f + 10.0f * static_cast<float>(index), -11.99f, 0.0f});
        detail::SessionTestFacade::override_joint_ratio_after_solver(
            *session, joints[index], 0.0);
    }
    NINHO_SIM_REQUIRE(session->tick().ok());
    std::array<EventId, 3> trigger_ids{};
    for (const DomainEvent& event : session->events()) {
        if (event.kind != DomainEventKind::PieceFractureTriggered) {
            continue;
        }
        const auto found = std::ranges::find(
            entities, event.affected_entity_id);
        NINHO_SIM_REQUIRE(found != entities.end());
        trigger_ids[static_cast<std::size_t>(found - entities.begin())] = event.id;
    }
    NINHO_SIM_REQUIRE(std::ranges::none_of(trigger_ids,
        [](EventId id) { return id == EventId{}; }));

    const auto snapshot_count = [&](EntityId entity) {
        return std::ranges::count(
            session->snapshots(), entity, &EntitySnapshot::entity_id);
    };
    const auto require_wave = [&](std::size_t wave, std::size_t expected_count) {
        NINHO_SIM_REQUIRE(snapshot_count(entities[wave])
            == static_cast<std::ptrdiff_t>(expected_count));
        std::vector<PartId> identities;
        for (const EntitySnapshot& snapshot : session->snapshots()) {
            if (snapshot.entity_id == entities[wave]) {
                NINHO_SIM_REQUIRE((snapshot.part_id.value() & 0x80000000U) != 0U);
                identities.push_back(snapshot.part_id);
            }
        }
        std::ranges::sort(identities);
        NINHO_SIM_REQUIRE(std::ranges::adjacent_find(identities) == identities.end());
        const auto fractured = std::ranges::find_if(
            session->events(), [&](const DomainEvent& event) {
                return event.kind == DomainEventKind::PieceFractured
                    && event.affected_entity_id == entities[wave];
            });
        NINHO_SIM_REQUIRE(fractured != session->events().end());
        NINHO_SIM_REQUIRE(fractured->cause_event_id == trigger_ids[wave]);
    };

    NINHO_SIM_REQUIRE(session->tick().ok());
    require_wave(0U, 80U);
    NINHO_SIM_REQUIRE(snapshot_count(entities[1]) == 1);
    NINHO_SIM_REQUIRE(snapshot_count(entities[2]) == 1);

    for (int tick = 0; tick < 30 && snapshot_count(entities[0]) != 0; ++tick) {
        NINHO_SIM_REQUIRE(session->tick().ok());
    }
    NINHO_SIM_REQUIRE(snapshot_count(entities[0]) == 0);
    NINHO_SIM_REQUIRE(detail::SessionTestFacade::body_record_count(*session)
        == initial_body_records - 1U);
    NINHO_SIM_REQUIRE(session->tick().ok());
    require_wave(1U, 80U);
    NINHO_SIM_REQUIRE(snapshot_count(entities[2]) == 1);

    for (int tick = 0; tick < 30 && snapshot_count(entities[1]) != 0; ++tick) {
        NINHO_SIM_REQUIRE(session->tick().ok());
    }
    NINHO_SIM_REQUIRE(snapshot_count(entities[1]) == 0);
    NINHO_SIM_REQUIRE(detail::SessionTestFacade::body_record_count(*session)
        == initial_body_records - 2U);
    NINHO_SIM_REQUIRE(session->tick().ok());
    require_wave(2U, 1U);
}

} // namespace
