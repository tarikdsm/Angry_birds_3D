#include "ninho/simulation/content.hpp"

#include "product_v2_reader.hpp"

#include <algorithm>
#include <cmath>
#include <limits>
#include <set>
#include <stdexcept>
#include <unordered_map>
#include <unordered_set>
#include <utility>

namespace ninho::simulation
{
namespace
{
using namespace detail::v2content;
BodyType body_type(std::string_view value, const std::string& pointer)
{
    if (value == "static")
        return BodyType::Static;
    if (value == "dynamic")
        return BodyType::Dynamic;
    fail(ContentErrorCode::InvalidEnum, pointer, "invalid body type");
}

JointKind joint_kind(std::string_view value, const std::string& pointer)
{
    if (value == "pine_fit")
        return JointKind::PineFit;
    if (value == "glass_clamp")
        return JointKind::GlassClamp;
    if (value == "mortar")
        return JointKind::Mortar;
    fail(ContentErrorCode::InvalidEnum, pointer, "invalid joint kind");
}

ShapeDefinition parse_shape(const json& item, const std::string& pointer, std::size_t depth = 0U)
{
    if (depth > 4U)
        fail(ContentErrorCode::ResourceLimit, pointer, "compound shape depth exceeded");
    object(item, pointer);
    const auto type = text(item, "type", pointer);
    ShapeDefinition result;
    if (type == "box")
    {
        keys(item, pointer, {"type", "half_extents_m"}, {"local_transform"});
        result.type = ShapeType::Box;
        result.half_extents_m = vector<3>(item, "half_extents_m", pointer, 0.0001, 1000.0);
    }
    else if (type == "sphere")
    {
        keys(item, pointer, {"type", "radius_m"}, {"local_transform"});
        result.type = ShapeType::Sphere;
        result.radius_m = number(item, "radius_m", pointer, 0.0, 1000.0, false);
    }
    else if (type == "capsule")
    {
        keys(item, pointer, {"type", "radius_m", "half_height_m"}, {"local_transform"});
        result.type = ShapeType::Capsule;
        result.radius_m = number(item, "radius_m", pointer, 0.0, 1000.0, false);
        result.half_height_m = number(item, "half_height_m", pointer, 0.0, 1000.0, false);
    }
    else if (type == "convex_hull")
    {
        keys(item, pointer, {"type", "vertices_m"}, {"local_transform"});
        result.type = ShapeType::ConvexHull;
        const auto vertices_pointer = child(pointer, "vertices_m");
        const auto& vertices = member(item, "vertices_m", pointer);
        array(vertices, vertices_pointer, max_hull_vertices, true);
        if (vertices.size() < 4U)
            fail(ContentErrorCode::InvalidInvariant, vertices_pointer,
                 "convex hull requires four vertices");
        for (std::size_t i = 0; i < vertices.size(); ++i)
        {
            json holder{{"v", vertices[i]}};
            result.vertices_m.push_back(
                vector<3>(holder, "v", indexed(vertices_pointer, i), -1000.0, 1000.0));
        }
        const auto& a = result.vertices_m[0];
        bool volume = false;
        for (std::size_t i = 1; i + 2 < result.vertices_m.size() && !volume; ++i)
        {
            for (std::size_t j = i + 1; j + 1 < result.vertices_m.size() && !volume; ++j)
            {
                for (std::size_t k = j + 1; k < result.vertices_m.size(); ++k)
                {
                    const auto& b = result.vertices_m[i];
                    const auto& c = result.vertices_m[j];
                    const auto& d = result.vertices_m[k];
                    const double bx = b[0] - a[0], by = b[1] - a[1], bz = b[2] - a[2];
                    const double cx = c[0] - a[0], cy = c[1] - a[1], cz = c[2] - a[2];
                    const double dx = d[0] - a[0], dy = d[1] - a[1], dz = d[2] - a[2];
                    const double determinant = bx * (cy * dz - cz * dy) - by * (cx * dz - cz * dx) +
                                               bz * (cx * dy - cy * dx);
                    if (std::abs(determinant) > 1e-9)
                    {
                        volume = true;
                        break;
                    }
                }
            }
        }
        if (!volume)
            fail(ContentErrorCode::InvalidInvariant, vertices_pointer, "convex hull is degenerate");
    }
    else if (type == "compound")
    {
        keys(item, pointer, {"type", "children"}, {"local_transform"});
        result.type = ShapeType::Compound;
        const auto children_pointer = child(pointer, "children");
        const auto& children = member(item, "children", pointer);
        array(children, children_pointer, max_compound_children, true);
        result.children.reserve(children.size());
        for (std::size_t i = 0; i < children.size(); ++i)
            result.children.push_back(
                parse_shape(children[i], indexed(children_pointer, i), depth + 1U));
    }
    else
    {
        fail(ContentErrorCode::InvalidEnum, child(pointer, "type"), "invalid shape type");
    }
    if (item.contains("local_transform"))
    {
        const auto local_pointer = child(pointer, "local_transform");
        const auto& local = member(item, "local_transform", pointer);
        keys(local, local_pointer, {"position_m", "rotation_xyzw"});
        result.local_position_m =
            vector<3>(local, "position_m", local_pointer, -1000.0, 1000.0);
        result.local_rotation_xyzw =
            vector<4>(local, "rotation_xyzw", local_pointer, -1.0, 1.0);
        const auto& q = result.local_rotation_xyzw;
        const double length =
            std::sqrt(q[0] * q[0] + q[1] * q[1] + q[2] * q[2] + q[3] * q[3]);
        if (std::abs(length - 1.0) > 1e-6)
            fail(ContentErrorCode::InvalidInvariant,
                 child(local_pointer, "rotation_xyzw"), "rotation must be normalized");
    }
    return result;
}

TransformDefinition parse_transform(const json& item, const std::string& pointer)
{
    keys(item, pointer, {"position_m", "rotation_xyzw"});
    TransformDefinition result;
    result.position_m = vector<3>(item, "position_m", pointer, -100000.0, 100000.0);
    result.rotation_xyzw = vector<4>(item, "rotation_xyzw", pointer, -1.0, 1.0);
    const auto& q = result.rotation_xyzw;
    const double length = std::sqrt(q[0] * q[0] + q[1] * q[1] + q[2] * q[2] + q[3] * q[3]);
    if (std::abs(length - 1.0) > 1e-6)
        fail(ContentErrorCode::InvalidInvariant, child(pointer, "rotation_xyzw"),
             "rotation must be normalized");
    return result;
}

} // namespace

ContentResult<LevelManifest> parse_level_manifest_v2(std::string_view input) noexcept
{
    return boundary<LevelManifest>(
        [&]
        {
            const auto root = parse_json(input, level_max_bytes);
            object(root, "");
            schema_v2(root);
            keys(root, "",
                 {"schema_version", "id", "world_id", "region_id", "camera_profile_id",
                  "presentation_profile_id", "world", "slingshot", "bird_queue", "scoring",
                  "free_body_ids", "bodies", "joints", "assemblies", "triggers", "objectives",
                  "settle_policy", "watchdog_ticks"});
            LevelManifest result;
            result.schema_version = result.source_schema_version = 2U;
            result.id = text(root, "id", "");
            result.world_id = text(root, "world_id", "");
            result.region_id = text(root, "region_id", "");
            result.camera_profile_id = text(root, "camera_profile_id", "");
            result.presentation_profile_id = text(root, "presentation_profile_id", "");
            const auto& world = member(root, "world", "");
            const auto world_kind = text(world, "kind", "/world");
            if (world_kind == "uniform")
            {
                keys(world, "/world", {"kind", "acceleration_m_s2", "bounds"});
                UniformWorldDefinition value;
                value.acceleration_m_s2 =
                    vector<3>(world, "acceleration_m_s2", "/world", -1000.0, 1000.0);
                const double length =
                    std::sqrt(value.acceleration_m_s2[0] * value.acceleration_m_s2[0] +
                              value.acceleration_m_s2[1] * value.acceleration_m_s2[1] +
                              value.acceleration_m_s2[2] * value.acceleration_m_s2[2]);
                if (length <= 1e-9)
                    fail(ContentErrorCode::OutOfRange, "/world/acceleration_m_s2",
                         "gravity cannot be zero");
                const auto& bounds = member(world, "bounds", "/world");
                keys(bounds, "/world/bounds", {"min_m", "max_m"});
                value.bounds_min_m =
                    vector<3>(bounds, "min_m", "/world/bounds", -100000.0, 100000.0);
                value.bounds_max_m =
                    vector<3>(bounds, "max_m", "/world/bounds", -100000.0, 100000.0);
                for (std::size_t i = 0; i < 3; ++i)
                    if (value.bounds_min_m[i] >= value.bounds_max_m[i])
                        fail(ContentErrorCode::InvalidInvariant, indexed("/world/bounds/max_m", i),
                             "bounds must increase");
                result.world = value;
            }
            else if (world_kind == "radial")
            {
                keys(world, "/world",
                     {"kind", "center_m", "reference_radius_m", "reference_acceleration_m_s2",
                      "bounds"});
                RadialWorldDefinition value;
                value.center_m = vector<3>(world, "center_m", "/world", -100000.0, 100000.0);
                value.reference_radius_m =
                    number(world, "reference_radius_m", "/world", 0.0, 100000.0, false);
                value.reference_acceleration_m_s2 =
                    number(world, "reference_acceleration_m_s2", "/world", 0.0, 1000.0, false);
                const auto& bounds = member(world, "bounds", "/world");
                keys(bounds, "/world/bounds", {"radius_m"});
                value.bounds_radius_m = number(bounds, "radius_m", "/world/bounds",
                                               value.reference_radius_m, 1000000.0, false);
                result.world = value;
            }
            else
                fail(ContentErrorCode::InvalidEnum, "/world/kind", "invalid world kind");
            const auto& launcher = member(root, "slingshot", "");
            keys(launcher, "/slingshot",
                 {"asset_id", "rest_position_m", "rest_rotation_xyzw", "spring_constant_n_m",
                  "energy_efficiency", "minimum_extension_m", "maximum_extension_m", "plane_policy",
                  "projectile_clearance_m", "speed_ceiling_m_s"});
            result.slingshot.asset_id = text(launcher, "asset_id", "/slingshot");
            result.slingshot.rest_position_m =
                vector<3>(launcher, "rest_position_m", "/slingshot", -100000.0, 100000.0);
            result.slingshot.rest_rotation_xyzw =
                vector<4>(launcher, "rest_rotation_xyzw", "/slingshot", -1.0, 1.0);
            const auto& q = result.slingshot.rest_rotation_xyzw;
            if (std::abs(std::sqrt(q[0] * q[0] + q[1] * q[1] + q[2] * q[2] + q[3] * q[3]) - 1.0) >
                1e-6)
                fail(ContentErrorCode::InvalidInvariant, "/slingshot/rest_rotation_xyzw",
                     "rotation must be normalized");
            result.slingshot.spring_constant_n_m =
                number(launcher, "spring_constant_n_m", "/slingshot", 0.0, 1e9, false);
            result.slingshot.energy_efficiency =
                number(launcher, "energy_efficiency", "/slingshot", 0.0, 1.0, false);
            result.slingshot.minimum_extension_m =
                number(launcher, "minimum_extension_m", "/slingshot", 0.0, 100.0);
            result.slingshot.maximum_extension_m =
                number(launcher, "maximum_extension_m", "/slingshot", 0.0, 100.0, false);
            if (result.slingshot.minimum_extension_m >= result.slingshot.maximum_extension_m)
                fail(ContentErrorCode::InvalidInvariant, "/slingshot/maximum_extension_m",
                     "extension range is invalid");
            result.slingshot.plane_policy = text(launcher, "plane_policy", "/slingshot");
            if (result.slingshot.plane_policy != "gravity_vertical_camera_yaw")
                fail(ContentErrorCode::InvalidEnum, "/slingshot/plane_policy",
                     "invalid plane policy");
            result.slingshot.projectile_clearance_m =
                number(launcher, "projectile_clearance_m", "/slingshot", 0.0, 100.0);
            result.slingshot.speed_ceiling_m_s =
                number(launcher, "speed_ceiling_m_s", "/slingshot", 0.0, 1000.0, false);
            const auto& queue = member(root, "bird_queue", "");
            array(queue, "/bird_queue", max_bird_queue, true);
            for (std::size_t i = 0; i < queue.size(); ++i)
                result.bird_queue.push_back(
                    BirdArchetypeId{uint_value(queue[i], indexed("/bird_queue", i))});
            const auto& scoring = member(root, "scoring", "");
            keys(scoring, "/scoring",
                 {"pig_points", "unused_bird_points", "star_thresholds", "chain_window_ticks",
                  "chain_multiplier_step", "max_chain_multiplier"});
            result.scoring.pig_points = uint(scoring, "pig_points", "/scoring", 0U, 1000000U);
            result.scoring.unused_bird_points =
                uint(scoring, "unused_bird_points", "/scoring", 0U, 1000000U);
            const auto& stars = member(scoring, "star_thresholds", "/scoring");
            array(stars, "/scoring/star_thresholds", 3U, true);
            if (stars.size() != 3U)
                fail(ContentErrorCode::OutOfRange, "/scoring/star_thresholds",
                     "exactly three thresholds required");
            for (std::size_t i = 0; i < 3U; ++i)
            {
                result.scoring.star_thresholds[i] =
                    uint_value(stars[i], indexed("/scoring/star_thresholds", i));
                if (i > 0 &&
                    result.scoring.star_thresholds[i] <= result.scoring.star_thresholds[i - 1])
                    fail(ContentErrorCode::InvalidInvariant, indexed("/scoring/star_thresholds", i),
                         "thresholds must strictly increase");
            }
            result.scoring.chain_window_ticks =
                uint(scoring, "chain_window_ticks", "/scoring", 1U, 3600U);
            result.scoring.chain_multiplier_step =
                number(scoring, "chain_multiplier_step", "/scoring", 0.0, 10.0);
            result.scoring.max_chain_multiplier =
                number(scoring, "max_chain_multiplier", "/scoring", 1.0, 100.0);
            const auto& free_ids = member(root, "free_body_ids", "");
            array(free_ids, "/free_body_ids", 500U);
            for (std::size_t i = 0; i < free_ids.size(); ++i)
                result.free_body_ids.push_back(
                    uint_value(free_ids[i], indexed("/free_body_ids", i)));
            std::unordered_set<std::uint32_t> body_ids;
            std::unordered_set<std::uint32_t> entity_ids;
            std::unordered_set<std::uint64_t> entity_parts;
            const auto& bodies = member(root, "bodies", "");
            array(bodies, "/bodies", 500U);
            for (std::size_t i = 0; i < bodies.size(); ++i)
            {
                const auto pointer = indexed("/bodies", i);
                const auto& item = bodies[i];
                keys(item, pointer,
                     {"body_id", "entity_id", "part_id", "body_type", "affected_by_world_gravity",
                      "material_id", "surface_id", "enemy_archetype_id", "density_kg_m3",
                      "transform", "shape", "visual"});
                BodyDefinition value;
                value.body_id = uint(item, "body_id", pointer);
                unique_id(body_ids, value.body_id, child(pointer, "body_id"));
                value.entity_id = id<EntityId>(item, "entity_id", pointer);
                if (value.entity_id.value() & runtime_entity_bit)
                    fail(ContentErrorCode::OutOfRange, child(pointer, "entity_id"),
                         "entity id uses runtime namespace");
                entity_ids.insert(value.entity_id.value());
                value.part_id = id<PartId>(item, "part_id", pointer);
                const auto entity_part =
                    (static_cast<std::uint64_t>(value.entity_id.value()) << 32U) |
                    value.part_id.value();
                if (!entity_parts.insert(entity_part).second)
                    fail(ContentErrorCode::DuplicateId, child(pointer, "part_id"),
                         "duplicate entity and part pair");
                value.body_type =
                    body_type(text(item, "body_type", pointer), child(pointer, "body_type"));
                value.affected_by_world_gravity =
                    boolean(item, "affected_by_world_gravity", pointer);
                if (value.body_type == BodyType::Dynamic && !value.affected_by_world_gravity)
                    fail(ContentErrorCode::InvalidInvariant,
                         child(pointer, "affected_by_world_gravity"),
                         "dynamic bodies must use world gravity");
                value.material_id = nullable_id<MaterialId>(item, "material_id", pointer);
                value.surface_id = nullable_id<SurfaceId>(item, "surface_id", pointer);
                value.enemy_archetype_id =
                    nullable_id<EnemyArchetypeId>(item, "enemy_archetype_id", pointer);
                if (value.material_id.has_value() == value.surface_id.has_value())
                    fail(ContentErrorCode::InvalidInvariant, pointer,
                         "body requires exactly one material or surface");
                value.density_kg_m3 = number(item, "density_kg_m3", pointer, 0.0, 30000.0,
                                             value.body_type == BodyType::Static);
                value.transform = parse_transform(member(item, "transform", pointer),
                                                  child(pointer, "transform"));
                value.shape = parse_shape(member(item, "shape", pointer), child(pointer, "shape"));
                const auto visual_pointer = child(pointer, "visual");
                const auto& visual = member(item, "visual", pointer);
                keys(visual, visual_pointer, {"asset_id", "bounds_m"});
                value.visual.asset_id = text(visual, "asset_id", visual_pointer);
                value.visual.bounds_m =
                    vector<3>(visual, "bounds_m", visual_pointer, 0.0001, 2000.0);
                result.bodies.push_back(std::move(value));
            }
            std::unordered_set<std::uint32_t> joint_ids;
            const auto& joints = member(root, "joints", "");
            array(joints, "/joints", 250U);
            for (std::size_t i = 0; i < joints.size(); ++i)
            {
                const auto pointer = indexed("/joints", i);
                const auto& item = joints[i];
                keys(item, pointer,
                     {"id", "assembly_id", "kind", "body_a_id", "body_b_id", "force_limit_n",
                      "torque_limit_nm"});
                JointDefinition value;
                value.id = id<JointId>(item, "id", pointer);
                unique_id(joint_ids, value.id.value(), child(pointer, "id"));
                value.assembly_id = uint(item, "assembly_id", pointer);
                value.kind = joint_kind(text(item, "kind", pointer), child(pointer, "kind"));
                value.body_a_id = uint(item, "body_a_id", pointer);
                value.body_b_id = uint(item, "body_b_id", pointer);
                if (value.body_a_id == value.body_b_id)
                    fail(ContentErrorCode::InvalidInvariant, pointer,
                         "joint endpoints must differ");
                value.force_limit_n = number(item, "force_limit_n", pointer, 0.0, 1e12, false);
                value.torque_limit_nm = number(item, "torque_limit_nm", pointer, 0.0, 1e12, false);
                result.joints.push_back(value);
            }
            std::unordered_set<std::uint32_t> assembly_ids;
            const auto& assemblies = member(root, "assemblies", "");
            array(assemblies, "/assemblies", 128U);
            std::size_t memberships = 0U;
            for (std::size_t i = 0; i < assemblies.size(); ++i)
            {
                const auto pointer = indexed("/assemblies", i);
                const auto& item = assemblies[i];
                keys(item, pointer, {"id", "key", "body_ids", "joint_ids"});
                AssemblyDefinition value;
                value.id = uint(item, "id", pointer);
                unique_id(assembly_ids, value.id, child(pointer, "id"));
                value.key = text(item, "key", pointer);
                const auto& body_list = member(item, "body_ids", pointer);
                array(body_list, child(pointer, "body_ids"), 500U, true);
                for (std::size_t j = 0; j < body_list.size(); ++j)
                    value.body_ids.push_back(
                        uint_value(body_list[j], indexed(child(pointer, "body_ids"), j)));
                const auto& joint_list = member(item, "joint_ids", pointer);
                array(joint_list, child(pointer, "joint_ids"), 250U, true);
                for (std::size_t j = 0; j < joint_list.size(); ++j)
                    value.joint_ids.push_back(JointId{
                        uint_value(joint_list[j], indexed(child(pointer, "joint_ids"), j))});
                memberships += value.body_ids.size() + value.joint_ids.size();
                if (memberships > 1000U)
                    fail(ContentErrorCode::ResourceLimit, pointer,
                         "assembly membership limit exceeded");
                result.assemblies.push_back(std::move(value));
            }
            std::unordered_map<std::uint32_t, std::size_t> body_indexes;
            for (std::size_t i = 0; i < result.bodies.size(); ++i)
                body_indexes.emplace(result.bodies[i].body_id, i);
            std::unordered_map<std::uint32_t, std::size_t> joint_indexes;
            for (std::size_t i = 0; i < result.joints.size(); ++i)
                joint_indexes.emplace(result.joints[i].id.value(), i);
            std::unordered_set<std::uint32_t> owned_bodies;
            for (std::size_t i = 0; i < result.free_body_ids.size(); ++i)
            {
                const auto value = result.free_body_ids[i];
                if (!body_indexes.contains(value))
                    fail(ContentErrorCode::MissingReference, indexed("/free_body_ids", i),
                         "body reference not found");
                if (!owned_bodies.insert(value).second)
                    fail(ContentErrorCode::DuplicateId, indexed("/free_body_ids", i),
                         "duplicate body ownership");
            }
            std::unordered_set<std::uint32_t> owned_joints;
            for (std::size_t i = 0; i < result.assemblies.size(); ++i)
            {
                auto& assembly = result.assemblies[i];
                std::unordered_set<std::uint32_t> local_bodies, local_joints;
                for (std::size_t j = 0; j < assembly.body_ids.size(); ++j)
                {
                    const auto value = assembly.body_ids[j];
                    const auto pointer = indexed(child(indexed("/assemblies", i), "body_ids"), j);
                    if (!body_indexes.contains(value))
                        fail(ContentErrorCode::MissingReference, pointer,
                             "body reference not found");
                    if (!local_bodies.insert(value).second || !owned_bodies.insert(value).second)
                        fail(ContentErrorCode::DuplicateId, pointer, "duplicate body ownership");
                    result.bodies[body_indexes.at(value)].assembly_id = assembly.id;
                }
                for (std::size_t j = 0; j < assembly.joint_ids.size(); ++j)
                {
                    const auto value = assembly.joint_ids[j].value();
                    const auto pointer = indexed(child(indexed("/assemblies", i), "joint_ids"), j);
                    if (!joint_indexes.contains(value))
                        fail(ContentErrorCode::MissingReference, pointer,
                             "joint reference not found");
                    if (!local_joints.insert(value).second || !owned_joints.insert(value).second)
                        fail(ContentErrorCode::DuplicateId, pointer, "duplicate joint ownership");
                    if (result.joints[joint_indexes.at(value)].assembly_id != assembly.id)
                        fail(ContentErrorCode::InvalidInvariant, pointer,
                             "joint assembly ownership differs");
                }
            }
            for (std::size_t i = 0; i < result.joints.size(); ++i)
            {
                const auto& value = result.joints[i];
                if (!assembly_ids.contains(value.assembly_id))
                    fail(ContentErrorCode::MissingReference, indexed("/joints", i) + "/assembly_id",
                         "assembly reference not found");
                if (!body_indexes.contains(value.body_a_id))
                    fail(ContentErrorCode::MissingReference, indexed("/joints", i) + "/body_a_id",
                         "body reference not found");
                if (!body_indexes.contains(value.body_b_id))
                    fail(ContentErrorCode::MissingReference, indexed("/joints", i) + "/body_b_id",
                         "body reference not found");
            }
            if (owned_bodies.size() != result.bodies.size())
                fail(ContentErrorCode::InvalidInvariant, "/bodies",
                     "every body must have exactly one ownership");
            if (owned_joints.size() != result.joints.size())
                fail(ContentErrorCode::InvalidInvariant, "/joints",
                     "every joint must have exactly one ownership");
            std::unordered_set<std::uint32_t> trigger_ids;
            const auto& triggers = member(root, "triggers", "");
            array(triggers, "/triggers", max_triggers);
            for (std::size_t i = 0; i < triggers.size(); ++i)
            {
                const auto pointer = indexed("/triggers", i);
                const auto& item = triggers[i];
                keys(item, pointer,
                     {"id", "target_entity_id", "kind", "damage_threshold", "fuse_ticks",
                      "cooldown_ticks", "pressure_burst"});
                EnvironmentalTriggerDefinition value;
                value.id = uint(item, "id", pointer);
                unique_id(trigger_ids, value.id, child(pointer, "id"));
                value.target_entity_id = id<EntityId>(item, "target_entity_id", pointer);
                if (text(item, "kind", pointer) != "damage_threshold")
                    fail(ContentErrorCode::InvalidEnum, child(pointer, "kind"),
                         "only damage_threshold triggers are supported");
                value.damage_threshold =
                    number(item, "damage_threshold", pointer, 0.0, 1000000.0, false);
                value.fuse_ticks = uint(item, "fuse_ticks", pointer, 0U, 36000U);
                value.cooldown_ticks = uint(item, "cooldown_ticks", pointer, 0U, 36000U);
                const auto burst_pointer = child(pointer, "pressure_burst");
                const auto& burst = member(item, "pressure_burst", pointer);
                keys(burst, burst_pointer,
                     {"radius_m", "impulse_n_s", "energy_j", "line_of_sight", "max_bodies"});
                value.pressure_burst.radius_m =
                    number(burst, "radius_m", burst_pointer, 0.0, 1000.0, false);
                value.pressure_burst.impulse_n_s =
                    number(burst, "impulse_n_s", burst_pointer, 0.0, 1e9, false);
                value.pressure_burst.energy_j =
                    number(burst, "energy_j", burst_pointer, 0.0, 1e12, false);
                value.pressure_burst.line_of_sight = boolean(burst, "line_of_sight", burst_pointer);
                value.pressure_burst.max_bodies =
                    uint(burst, "max_bodies", burst_pointer, 1U, 500U);
                if (!entity_ids.contains(value.target_entity_id.value()))
                    fail(ContentErrorCode::MissingReference, child(pointer, "target_entity_id"),
                         "trigger target not found");
                result.triggers.push_back(value);
            }
            std::unordered_set<std::uint32_t> objective_ids;
            const auto& objectives = member(root, "objectives", "");
            array(objectives, "/objectives", 64U, true);
            for (std::size_t i = 0; i < objectives.size(); ++i)
            {
                const auto pointer = indexed("/objectives", i);
                const auto& item = objectives[i];
                keys(item, pointer, {"id", "kind", "target_entity_id"});
                ObjectiveDefinition value;
                value.id = uint(item, "id", pointer);
                unique_id(objective_ids, value.id, child(pointer, "id"));
                if (text(item, "kind", pointer) != "neutralize_entity")
                    fail(ContentErrorCode::InvalidEnum, child(pointer, "kind"),
                         "invalid objective kind");
                value.kind = ObjectiveKind::NeutralizeEntity;
                value.target_entity_id = id<EntityId>(item, "target_entity_id", pointer);
                if (!entity_ids.contains(value.target_entity_id.value()))
                    fail(ContentErrorCode::MissingReference, child(pointer, "target_entity_id"),
                         "objective target not found");
                result.objectives.push_back(value);
            }
            const auto& settle = member(root, "settle_policy", "");
            keys(settle, "/settle_policy",
                 {"linear_speed_m_s", "angular_speed_rad_s", "rest_ticks"});
            result.settle_policy.linear_speed_m_s =
                number(settle, "linear_speed_m_s", "/settle_policy", 0.0, 100.0, false);
            result.settle_policy.angular_speed_rad_s =
                number(settle, "angular_speed_rad_s", "/settle_policy", 0.0, 100.0, false);
            result.settle_policy.rest_ticks =
                uint(settle, "rest_ticks", "/settle_policy", 1U, 3600U);
            result.watchdog_ticks = uint(root, "watchdog_ticks", "", 1U, 36000U);
            if (result.watchdog_ticks <= result.settle_policy.rest_ticks)
                fail(ContentErrorCode::InvalidInvariant, "/watchdog_ticks",
                     "watchdog must exceed settle window");
            return result;
        });
}

} // namespace ninho::simulation
