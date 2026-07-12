#include "session_internal.hpp"
#include "material_mapping.hpp"

#include <algorithm>
#include <cmath>
#include <exception>
#include <ranges>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>

namespace ninho::simulation {
namespace {

[[nodiscard]] ninho::physics::Vec3 vector_from(const std::array<double, 3>& value)
{
    return {static_cast<float>(value[0]), static_cast<float>(value[1]), static_cast<float>(value[2])};
}

[[nodiscard]] ninho::physics::Quat quaternion_from(const std::array<double, 4>& value)
{
    return {static_cast<float>(value[0]), static_cast<float>(value[1]),
        static_cast<float>(value[2]), static_cast<float>(value[3])};
}

[[nodiscard]] ninho::physics::Transform transform_from(const TransformDefinition& value)
{
    return {vector_from(value.position_m), quaternion_from(value.rotation_xyzw)};
}

[[nodiscard]] ninho::physics::Quat conjugate(ninho::physics::Quat value) noexcept
{
    return {-value.x, -value.y, -value.z, value.w};
}

[[nodiscard]] ninho::physics::Quat multiply(
    ninho::physics::Quat lhs, ninho::physics::Quat rhs) noexcept
{
    return {
        lhs.w * rhs.x + lhs.x * rhs.w + lhs.y * rhs.z - lhs.z * rhs.y,
        lhs.w * rhs.y - lhs.x * rhs.z + lhs.y * rhs.w + lhs.z * rhs.x,
        lhs.w * rhs.z + lhs.x * rhs.y - lhs.y * rhs.x + lhs.z * rhs.w,
        lhs.w * rhs.w - lhs.x * rhs.x - lhs.y * rhs.y - lhs.z * rhs.z,
    };
}

[[nodiscard]] ninho::physics::Vec3 rotate(
    ninho::physics::Quat rotation, ninho::physics::Vec3 value) noexcept
{
    const ninho::physics::Vec3 q{rotation.x, rotation.y, rotation.z};
    const ninho::physics::Vec3 twice_cross = 2.0f * ninho::physics::cross(q, value);
    return value + rotation.w * twice_cross + ninho::physics::cross(q, twice_cross);
}

[[nodiscard]] ninho::physics::Transform local_frame(
    const ninho::physics::Transform& body,
    ninho::physics::Vec3 world_midpoint) noexcept
{
    const auto inverse = conjugate(body.rotation);
    return {rotate(inverse, world_midpoint - body.position), multiply(inverse, {})};
}

[[nodiscard]] const MaterialDefinition* find_material(
    const MaterialCatalog& catalog, MaterialId id)
{
    const auto found = std::ranges::find(catalog.materials, id, &MaterialDefinition::id);
    return found == catalog.materials.end() ? nullptr : &*found;
}

[[nodiscard]] const PhysicsSurfaceDefinition* find_surface(
    const MaterialCatalog& catalog, SurfaceId id)
{
    const auto found = std::ranges::find(catalog.surfaces, id, &PhysicsSurfaceDefinition::id);
    return found == catalog.surfaces.end() ? nullptr : &*found;
}

[[nodiscard]] ninho::physics::ShapeDesc make_shape(
    const BodyDefinition& body, const MaterialCatalog& materials)
{
    ninho::physics::ShapeDesc result;
    if (body.shape.type == ShapeType::Box) {
        result.geometry = ninho::physics::BoxShape{vector_from(body.shape.half_extents_m), {}};
    } else {
        result.geometry = ninho::physics::SphereShape{static_cast<float>(body.shape.radius_m), {}};
    }
    result.density = static_cast<float>(body.density_kg_m3);
    if (body.material_id) {
        const auto* material = find_material(materials, *body.material_id);
        result.friction = static_cast<float>(material->friction);
        result.restitution = static_cast<float>(material->restitution);
        result.material_id = detail::physics_material_tag(*body.material_id);
    } else {
        const auto* surface = find_surface(materials, *body.surface_id);
        result.friction = static_cast<float>(surface->friction);
        result.restitution = static_cast<float>(surface->restitution);
        result.material_id = detail::physics_surface_tag(*body.surface_id);
    }
    return result;
}

[[nodiscard]] ninho::physics::BodyDesc make_body(
    const BodyDefinition& body, const MaterialCatalog& materials, bool participates_in_joint)
{
    ninho::physics::BodyDesc result;
    result.type = body.body_type == BodyType::Dynamic
        ? ninho::physics::BodyType::Dynamic
        : ninho::physics::BodyType::Static;
    result.transform = transform_from(body.transform);
    result.shapes.push_back(make_shape(body, materials));
    result.radial_gravity = body.body_type == BodyType::Dynamic;
    // A physics-side body removal also invalidates every attached solver joint.
    // Keep jointed bodies under simulation ownership so public/canonical joint
    // state can never claim an invalid solver constraint is still active.
    result.remove_beyond_six_r = body.body_type == BodyType::Dynamic
        && !participates_in_joint;
    result.name = body.visual.asset_id;
    return result;
}

[[nodiscard]] SessionStatus build_failure(std::string message) noexcept
{
    return {{ContentErrorCode::InternalError, "", std::move(message)}};
}

[[nodiscard]] SessionStatus identity_collision(std::string pointer) noexcept
{
    return {{ContentErrorCode::InvalidInvariant, std::move(pointer),
        "planet and level bodies must have unique (entity_id, part_id) identities"}};
}

}

SimulationSession::Impl::Impl(ContentBundle source)
    : bundle(std::move(source))
    , physics({.planet_radius = static_cast<float>(bundle.level.planet.radius_m),
          .surface_gravity = static_cast<float>(bundle.level.planet.surface_gravity_m_s2)})
{
}

SessionStatus SimulationSession::Impl::build() noexcept
{
    try {
        remaining_birds = 0;
        roster_remaining = bundle.level.bird_roster;
        std::ranges::sort(roster_remaining, {}, &BirdRosterEntry::bird_archetype_id);
        for (const BirdRosterEntry& entry : bundle.level.bird_roster) {
            remaining_birds += entry.count;
        }

        std::unordered_map<std::uint32_t, ninho::physics::BodyHandle> handles_by_body_id;
        std::unordered_map<std::uint32_t, const BodyDefinition*> bodies_by_id;
        handles_by_body_id.reserve(bundle.level.bodies.size());
        bodies_by_id.reserve(bundle.level.bodies.size());
        std::unordered_set<std::uint32_t> jointed_body_ids;
        jointed_body_ids.reserve(bundle.level.joints.size() * 2U);
        for (const JointDefinition& joint : bundle.level.joints) {
            jointed_body_ids.insert(joint.body_a_id);
            jointed_body_ids.insert(joint.body_b_id);
        }

        struct PendingBody {
            EntityId entity_id;
            PartId part_id;
            const BodyDefinition* level_body{};
            std::size_t source_index{};
        };
        std::vector<PendingBody> pending_bodies;
        pending_bodies.reserve(bundle.level.bodies.size() + 1U);
        pending_bodies.push_back(
            {bundle.level.planet.entity_id, PartId{0}, nullptr, 0U});
        for (std::size_t index = 0; index < bundle.level.bodies.size(); ++index) {
            const BodyDefinition& body = bundle.level.bodies[index];
            pending_bodies.push_back({body.entity_id, body.part_id, &body, index});
            bodies_by_id.emplace(body.body_id, &body);
        }
        std::ranges::sort(pending_bodies, [](const auto& lhs, const auto& rhs) {
            return std::pair{lhs.entity_id, lhs.part_id}
                < std::pair{rhs.entity_id, rhs.part_id};
        });
        for (std::size_t index = 1; index < pending_bodies.size(); ++index) {
            const auto& previous = pending_bodies[index - 1U];
            const auto& current = pending_bodies[index];
            if (previous.entity_id == current.entity_id && previous.part_id == current.part_id) {
                const std::string pointer = current.level_body == nullptr
                    ? "/planet/entity_id"
                    : "/bodies/" + std::to_string(current.source_index) + "/entity_id";
                return identity_collision(pointer);
            }
        }

        const auto* planet_surface = find_surface(
            bundle.materials, bundle.level.planet.surface_id);
        if (planet_surface == nullptr) {
            return build_failure("planet surface was not resolved");
        }
        for (const PendingBody& pending : pending_bodies) {
            if (pending.level_body == nullptr) {
                ninho::physics::BodyDesc planet = ninho::physics::BodyDesc::static_sphere(
                    static_cast<float>(bundle.level.planet.radius_m), {});
                planet.shapes.front().density = static_cast<float>(planet_surface->density_kg_m3);
                planet.shapes.front().friction = static_cast<float>(planet_surface->friction);
                planet.shapes.front().restitution = static_cast<float>(planet_surface->restitution);
                planet.shapes.front().material_id =
                    detail::physics_surface_tag(bundle.level.planet.surface_id);
                planet.name = bundle.level.planet.visual_id;
                const auto created = physics.create_body(planet);
                if (!created) {
                    return build_failure(created.status.message);
                }
                body_records.push_back({0,
                    bundle.level.planet.entity_id,
                    PartId{0},
                    BodyType::Static,
                    std::nullopt,
                    bundle.level.planet.surface_id,
                    std::nullopt,
                    {.type = ShapeType::Sphere, .radius_m = bundle.level.planet.radius_m},
                    bundle.level.planet.visual_id,
                    created.value});
                continue;
            }

            const BodyDefinition& body = *pending.level_body;
            const auto created = physics.create_body(make_body(body, bundle.materials,
                jointed_body_ids.contains(body.body_id)));
            if (!created) {
                return build_failure(created.status.message);
            }
            handles_by_body_id.emplace(body.body_id, created.value);
            body_records.push_back({body.body_id,
                body.entity_id,
                body.part_id,
                body.body_type,
                body.material_id,
                body.surface_id,
                body.enemy_archetype_id,
                body.shape,
                body.visual.asset_id,
                created.value});
        }

        std::vector<const JointDefinition*> ordered_joints;
        ordered_joints.reserve(bundle.level.joints.size());
        for (const JointDefinition& joint : bundle.level.joints) {
            ordered_joints.push_back(&joint);
        }
        std::ranges::sort(ordered_joints, {}, [](const auto* joint) { return joint->id; });
        for (const JointDefinition* joint : ordered_joints) {
            const BodyDefinition& body_a = *bodies_by_id.at(joint->body_a_id);
            const BodyDefinition& body_b = *bodies_by_id.at(joint->body_b_id);
            const auto transform_a = transform_from(body_a.transform);
            const auto transform_b = transform_from(body_b.transform);
            const auto midpoint = (transform_a.position + transform_b.position) * 0.5f;
            const auto frame_a = local_frame(transform_a, midpoint);
            const auto frame_b = local_frame(transform_b, midpoint);
            const ninho::physics::WeldJointDesc description{
                .a = handles_by_body_id.at(joint->body_a_id),
                .b = handles_by_body_id.at(joint->body_b_id),
                .frame_a = frame_a,
                .frame_b = frame_b,
                .hertz = 8.0f,
                .damping_ratio = 1.0f,
                .collide_connected = false,
            };
            const auto created = physics.create_joint(description);
            if (!created) {
                return build_failure(created.status.message);
            }
            joint_records.push_back({
                {joint->id,
                    {body_a.entity_id, body_a.part_id},
                    {body_b.entity_id, body_b.part_id},
                    joint->kind,
                    frame_a,
                    frame_b,
                    joint->force_limit_n,
                    joint->torque_limit_nm,
                    true},
                created.value});
        }

        const auto committed = physics.commit_pending_initial_state();
        if (!committed.ok()) {
            return build_failure(committed.message);
        }
        rebuild_snapshots();
        refresh_canonical_state();
        return {};
    } catch (const std::exception& error) {
        return build_failure(error.what());
    } catch (...) {
        return build_failure("unknown session bootstrap failure");
    }
}

void SimulationSession::Impl::rebuild_snapshots()
{
    entity_snapshots.clear();
    entity_snapshots.reserve(body_records.size());
    for (const BodyRecord& record : body_records) {
        const auto state = physics.state(record.physics_handle);
        if (!state) {
            continue;
        }
        entity_snapshots.push_back({record.entity_id,
            record.part_id,
            record.body_type,
            record.material_id,
            record.surface_id,
            record.enemy_archetype_id,
            record.shape,
            record.visual_id,
            state->transform,
            state->linear_velocity,
            state->angular_velocity,
            state->mass,
            state->awake,
            state->ejected});
    }
    std::ranges::sort(entity_snapshots, [](const auto& lhs, const auto& rhs) {
        return std::pair{lhs.entity_id, lhs.part_id} < std::pair{rhs.entity_id, rhs.part_id};
    });
    joint_snapshots.clear();
    joint_snapshots.reserve(joint_records.size());
    for (const JointRecord& record : joint_records) {
        joint_snapshots.push_back(record.snapshot);
    }
}

ContentResult<std::unique_ptr<SimulationSession>> SimulationSession::create(
    const MaterialCatalog& materials,
    const ArchetypeCatalog& archetypes,
    const LevelManifest& level)
{
    const auto content = make_content_bundle(materials, archetypes, level);
    if (!content.ok()) {
        return {{}, content.error};
    }
    try {
        auto implementation = std::make_unique<Impl>(content.value);
        const SessionStatus status = implementation->build();
        if (!status.ok()) {
            return {{}, status.error};
        }
        return {std::unique_ptr<SimulationSession>{
                    new SimulationSession{std::move(implementation)}},
            {}};
    } catch (const std::exception& error) {
        return {{}, {ContentErrorCode::InternalError, "", error.what()}};
    } catch (...) {
        return {{}, {ContentErrorCode::InternalError, "", "unknown session creation failure"}};
    }
}

}
