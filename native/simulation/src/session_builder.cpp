#include "session_internal.hpp"
#include "material_mapping.hpp"

#include <algorithm>
#include <array>
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

[[nodiscard]] ninho::physics::WorldConfig make_world_config(const LevelManifest& level)
{
    if (level.source_schema_version == 1U) {
        return ninho::physics::make_legacy_radial_world_config({
            .planet_radius = static_cast<float>(level.planet.radius_m),
            .surface_gravity = static_cast<float>(level.planet.surface_gravity_m_s2),
        });
    }
    return std::visit(
        [](const auto& world) -> ninho::physics::WorldConfig {
            using World = std::decay_t<decltype(world)>;
            if constexpr (std::is_same_v<World, UniformWorldDefinition>) {
                return {
                    .max_bodies = 500U,
                    .gravity = ninho::physics::UniformGravityConfig{
                        vector_from(world.acceleration_m_s2)},
                    .bounds = ninho::physics::AabbWorldBounds{
                        vector_from(world.bounds_min_m), vector_from(world.bounds_max_m)},
                };
            } else {
                return {
                    .max_bodies = 500U,
                    .gravity = ninho::physics::RadialGravityConfig{
                        .center_m = vector_from(world.center_m),
                        .reference_radius_m = static_cast<float>(world.reference_radius_m),
                        .reference_acceleration_m_s2 =
                            static_cast<float>(world.reference_acceleration_m_s2),
                    },
                    .bounds = ninho::physics::SphericalWorldBounds{
                        .center_m = vector_from(world.center_m),
                        .removal_radius_m = static_cast<float>(world.bounds_radius_m),
                    },
                };
            }
        },
        level.world);
}

[[nodiscard]] bool finite_vector(const std::array<double, 3>& value) noexcept
{
    return std::ranges::all_of(value, [](double component) {
        return std::isfinite(component);
    });
}

[[nodiscard]] std::array<double, 3> subtract(
    const std::array<double, 3>& lhs, const std::array<double, 3>& rhs) noexcept
{
    return {lhs[0] - rhs[0], lhs[1] - rhs[1], lhs[2] - rhs[2]};
}

[[nodiscard]] double determinant(const std::array<double, 3>& a,
    const std::array<double, 3>& b, const std::array<double, 3>& c) noexcept
{
    return a[0] * (b[1] * c[2] - b[2] * c[1])
        - a[1] * (b[0] * c[2] - b[2] * c[0])
        + a[2] * (b[0] * c[1] - b[1] * c[0]);
}

[[nodiscard]] bool contained_by_tetrahedron(const std::array<double, 3>& point,
    const std::array<double, 3>& a, const std::array<double, 3>& b,
    const std::array<double, 3>& c, const std::array<double, 3>& d) noexcept
{
    const auto ba = subtract(b, a);
    const auto ca = subtract(c, a);
    const auto da = subtract(d, a);
    const auto pa = subtract(point, a);
    const double denominator = determinant(ba, ca, da);
    constexpr double epsilon = 1.0e-10;
    if (std::abs(denominator) <= epsilon) {
        return false;
    }
    const double u = determinant(pa, ca, da) / denominator;
    const double v = determinant(ba, pa, da) / denominator;
    const double w = determinant(ba, ca, pa) / denominator;
    const double t = 1.0 - u - v - w;
    return u >= -epsilon && v >= -epsilon && w >= -epsilon && t >= -epsilon
        && u <= 1.0 + epsilon && v <= 1.0 + epsilon
        && w <= 1.0 + epsilon && t <= 1.0 + epsilon;
}

[[nodiscard]] bool has_redundant_hull_vertex(
    const std::vector<std::array<double, 3>>& vertices) noexcept
{
    if (vertices.size() <= 4U) {
        return false;
    }
    for (std::size_t candidate = 0; candidate < vertices.size(); ++candidate) {
        for (std::size_t a = 0; a < vertices.size(); ++a) {
            if (a == candidate) continue;
            for (std::size_t b = a + 1U; b < vertices.size(); ++b) {
                if (b == candidate) continue;
                for (std::size_t c = b + 1U; c < vertices.size(); ++c) {
                    if (c == candidate) continue;
                    for (std::size_t d = c + 1U; d < vertices.size(); ++d) {
                        if (d == candidate) continue;
                        if (contained_by_tetrahedron(vertices[candidate],
                                vertices[a], vertices[b], vertices[c], vertices[d])) {
                            return true;
                        }
                    }
                }
            }
        }
    }
    return false;
}

[[nodiscard]] std::optional<ContentError> validate_shape(
    const ShapeDefinition& shape, const std::string& pointer, std::size_t depth = 0U)
{
    const auto positive = [](double value) {
        return std::isfinite(value) && value > 0.0;
    };
    switch (shape.type) {
    case ShapeType::Box:
        if (!std::ranges::all_of(shape.half_extents_m, positive)) {
            return ContentError{ContentErrorCode::InvalidInvariant,
                pointer + "/half_extents_m", "box half extents must be positive and finite"};
        }
        return std::nullopt;
    case ShapeType::Sphere:
        if (!positive(shape.radius_m)) {
            return ContentError{ContentErrorCode::InvalidInvariant,
                pointer + "/radius_m", "sphere radius must be positive and finite"};
        }
        return std::nullopt;
    case ShapeType::Capsule:
        if (!positive(shape.radius_m) || !positive(shape.half_height_m)) {
            return ContentError{ContentErrorCode::InvalidInvariant, pointer,
                "capsule dimensions must be positive and finite"};
        }
        return std::nullopt;
    case ShapeType::ConvexHull: {
        const std::string vertices_pointer = pointer + "/vertices_m";
        if (shape.vertices_m.size() > 64U) {
            return ContentError{ContentErrorCode::ResourceLimit, vertices_pointer,
                "convex hull vertex capacity exceeded"};
        }
        if (shape.vertices_m.size() < 4U
            || !std::ranges::all_of(shape.vertices_m, finite_vector)) {
            return ContentError{ContentErrorCode::InvalidInvariant, vertices_pointer,
                "convex hull requires at least four finite vertices"};
        }
        for (std::size_t index = 0; index < shape.vertices_m.size(); ++index) {
            if (std::ranges::find(shape.vertices_m.begin(),
                    shape.vertices_m.begin() + static_cast<std::ptrdiff_t>(index),
                    shape.vertices_m[index])
                != shape.vertices_m.begin() + static_cast<std::ptrdiff_t>(index)) {
                return ContentError{ContentErrorCode::InvalidInvariant, vertices_pointer,
                    "convex hull vertices must be unique"};
            }
        }
        bool has_volume = false;
        for (std::size_t b = 1U; b + 2U < shape.vertices_m.size() && !has_volume; ++b) {
            for (std::size_t c = b + 1U; c + 1U < shape.vertices_m.size() && !has_volume; ++c) {
                for (std::size_t d = c + 1U; d < shape.vertices_m.size(); ++d) {
                    has_volume = std::abs(determinant(
                        subtract(shape.vertices_m[b], shape.vertices_m[0]),
                        subtract(shape.vertices_m[c], shape.vertices_m[0]),
                        subtract(shape.vertices_m[d], shape.vertices_m[0]))) > 1.0e-10;
                    if (has_volume) break;
                }
            }
        }
        if (!has_volume || has_redundant_hull_vertex(shape.vertices_m)) {
            return ContentError{ContentErrorCode::InvalidInvariant, vertices_pointer,
                "vertices must be the vertices of a convex volume"};
        }
        return std::nullopt;
    }
    case ShapeType::Compound:
        if (depth >= 4U) {
            return ContentError{ContentErrorCode::ResourceLimit, pointer,
                "compound shape depth exceeded"};
        }
        if (shape.children.empty()) {
            return ContentError{ContentErrorCode::InvalidInvariant, pointer + "/children",
                "compound shape requires children"};
        }
        if (shape.children.size() > 16U) {
            return ContentError{ContentErrorCode::ResourceLimit, pointer + "/children",
                "compound child capacity exceeded"};
        }
        for (std::size_t index = 0; index < shape.children.size(); ++index) {
            if (const auto error = validate_shape(shape.children[index],
                    pointer + "/children/" + std::to_string(index), depth + 1U)) {
                return error;
            }
        }
        return std::nullopt;
    }
    return ContentError{ContentErrorCode::InvalidInvariant, pointer + "/type",
        "shape type is invalid"};
}

void append_primitives(const ShapeDefinition& shape,
    std::vector<ninho::physics::PrimitiveShape>& output)
{
    switch (shape.type) {
    case ShapeType::Box:
        output.push_back(ninho::physics::BoxShape{vector_from(shape.half_extents_m), {}});
        break;
    case ShapeType::Sphere:
        output.push_back(ninho::physics::SphereShape{
            static_cast<float>(shape.radius_m), {}});
        break;
    case ShapeType::Capsule:
        output.push_back(ninho::physics::CapsuleShape{
            static_cast<float>(shape.half_height_m),
            static_cast<float>(shape.radius_m), {}});
        break;
    case ShapeType::ConvexHull: {
        std::vector<ninho::physics::Vec3> vertices;
        vertices.reserve(shape.vertices_m.size());
        std::ranges::transform(shape.vertices_m, std::back_inserter(vertices), vector_from);
        output.push_back(ninho::physics::HullShape{std::move(vertices), {}});
        break;
    }
    case ShapeType::Compound:
        for (const ShapeDefinition& child : shape.children) {
            append_primitives(child, output);
        }
        break;
    }
}

[[nodiscard]] ninho::physics::ShapeDesc make_shape(
    const BodyDefinition& body, const MaterialCatalog& materials)
{
    ninho::physics::ShapeDesc result;
    std::vector<ninho::physics::PrimitiveShape> primitives;
    append_primitives(body.shape, primitives);
    if (body.shape.type == ShapeType::Compound) {
        result.geometry = ninho::physics::CompoundShape{std::move(primitives)};
    } else {
        result.geometry = std::visit([](auto primitive) -> ninho::physics::ShapeGeometry {
            return primitive;
        }, std::move(primitives.front()));
    }
    // Box3D requires a positive shape density even for static bodies, whose
    // solver mass remains zero. Schema v2 deliberately permits static density 0.
    result.density = static_cast<float>(
        body.body_type == BodyType::Static && body.density_kg_m3 == 0.0
            ? 1.0 : body.density_kg_m3);
    if (body.material_id) {
        const auto* material = find_material(materials, *body.material_id);
        result.friction = static_cast<float>(material->friction);
        result.restitution = static_cast<float>(material->restitution);
    } else {
        const auto* surface = find_surface(materials, *body.surface_id);
        result.friction = static_cast<float>(surface->friction);
        result.restitution = static_cast<float>(surface->restitution);
    }
    result.material_id = detail::physics_material_tag(body);
    return result;
}

[[nodiscard]] ninho::physics::WorldExitPolicy world_exit_policy_for(
    const LevelManifest& level, const BodyDefinition& body, bool participates_in_joint) noexcept
{
    if (body.body_type == BodyType::Static || participates_in_joint) {
        return ninho::physics::WorldExitPolicy::KeepOutsideBounds;
    }
    if (level.source_schema_version == 1U) {
        return ninho::physics::WorldExitPolicy::RemoveOutsideBounds;
    }
    return std::visit([](const auto&) {
        return ninho::physics::WorldExitPolicy::RemoveOutsideBounds;
    }, level.world);
}

[[nodiscard]] ninho::physics::BodyDesc make_body(
    const BodyDefinition& body, const MaterialCatalog& materials,
    const LevelManifest& level, bool participates_in_joint)
{
    ninho::physics::BodyDesc result;
    result.type = body.body_type == BodyType::Dynamic
        ? ninho::physics::BodyType::Dynamic
        : ninho::physics::BodyType::Static;
    result.transform = transform_from(body.transform);
    result.shapes.push_back(make_shape(body, materials));
    result.affected_by_world_gravity = body.body_type == BodyType::Dynamic
        ? body.affected_by_world_gravity : false;
    // A physics-side body removal also invalidates every attached solver joint.
    // Keep jointed bodies under simulation ownership so public/canonical joint
    // state can never claim an invalid solver constraint is still active.
    result.world_exit_policy = world_exit_policy_for(level, body, participates_in_joint);
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
    , physics(make_world_config(bundle.level))
{
}

SessionStatus SimulationSession::Impl::build() noexcept
{
    try {
        remaining_birds = 0U;
        if (bundle.level.source_schema_version == 1U) {
            roster_remaining = bundle.level.bird_roster;
            std::ranges::sort(roster_remaining, {}, &BirdRosterEntry::bird_archetype_id);
            for (const BirdRosterEntry& entry : bundle.level.bird_roster) {
                remaining_birds += entry.count;
            }
        } else {
            remaining_birds = static_cast<std::uint32_t>(bundle.level.bird_queue.size());
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
        const bool legacy = bundle.level.source_schema_version == 1U;
        pending_bodies.reserve(bundle.level.bodies.size() + (legacy ? 1U : 0U));
        if (legacy) {
            pending_bodies.push_back(
                {bundle.level.planet.entity_id, PartId{0}, nullptr, 0U});
        }
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

        const auto* planet_surface = legacy
            ? find_surface(bundle.materials, bundle.level.planet.surface_id) : nullptr;
        if (legacy && planet_surface == nullptr) return build_failure("planet surface was not resolved");
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
                    created.value, false, false, false});
                continue;
            }

            const BodyDefinition& body = *pending.level_body;
            const auto created = physics.create_body(make_body(body, bundle.materials,
                bundle.level, jointed_body_ids.contains(body.body_id)));
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
                created.value,
                false,
                false,
                body.body_type == BodyType::Dynamic && body.affected_by_world_gravity});
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
        if (legacy) {
            rebuild_canonical_static_content();
        }
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
    const auto identity_key = [](EntityId entity, PartId part) {
        return (static_cast<std::uint64_t>(entity.value()) << 32U)
            | static_cast<std::uint64_t>(part.value());
    };

    const auto states = physics.states();
    std::size_t state_index_size = 1U;
    for (const ninho::physics::BodyState& state : states) {
        state_index_size = std::max(
            state_index_size, static_cast<std::size_t>(state.handle.index) + 1U);
    }
    physics_state_by_handle_index.assign(state_index_size, nullptr);
    for (const ninho::physics::BodyState& state : states) {
        physics_state_by_handle_index[state.handle.index] = &state;
    }

    snapshot_index_by_identity.clear();
    snapshot_index_by_identity.reserve(entity_snapshots.size());
    for (std::size_t index = 0; index < entity_snapshots.size(); ++index) {
        const EntitySnapshot& snapshot = entity_snapshots[index];
        snapshot_index_by_identity.emplace(
            identity_key(snapshot.entity_id, snapshot.part_id), index);
    }

    entity_snapshot_scratch.clear();
    entity_snapshot_scratch.reserve(body_records.size());
    for (const BodyRecord& record : body_records) {
        if (record.physics_handle.index >= physics_state_by_handle_index.size()) {
            continue;
        }
        const ninho::physics::BodyState* state =
            physics_state_by_handle_index[record.physics_handle.index];
        if (state == nullptr || state->handle != record.physics_handle) {
            continue;
        }

        const auto previous = snapshot_index_by_identity.find(
            identity_key(record.entity_id, record.part_id));
        if (previous == snapshot_index_by_identity.end()) {
            entity_snapshot_scratch.push_back({record.entity_id,
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
                state->ejected,
                state->exited_world,
                record.is_projectile});
#if defined(NINHO_ENABLE_TEST_FACADES)
            ++snapshot_visual_id_copies;
#endif
            continue;
        }

        entity_snapshot_scratch.push_back(
            std::move(entity_snapshots[previous->second]));
        EntitySnapshot& snapshot = entity_snapshot_scratch.back();
        snapshot.entity_id = record.entity_id;
        snapshot.part_id = record.part_id;
        snapshot.body_type = record.body_type;
        snapshot.material_id = record.material_id;
        snapshot.surface_id = record.surface_id;
        snapshot.enemy_archetype_id = record.enemy_archetype_id;
        snapshot.shape = record.shape;
        if (snapshot.visual_id != record.visual_id) {
            snapshot.visual_id = record.visual_id;
#if defined(NINHO_ENABLE_TEST_FACADES)
            ++snapshot_visual_id_copies;
#endif
        }
        snapshot.transform = state->transform;
        snapshot.linear_velocity_m_s = state->linear_velocity;
        snapshot.angular_velocity_rad_s = state->angular_velocity;
        snapshot.mass_kg = state->mass;
        snapshot.awake = state->awake;
        snapshot.ejected = state->ejected;
        snapshot.exited_world = state->exited_world;
        snapshot.is_projectile = record.is_projectile;
    }
    std::ranges::sort(entity_snapshot_scratch, [](const auto& lhs, const auto& rhs) {
        return std::pair{lhs.entity_id, lhs.part_id} < std::pair{rhs.entity_id, rhs.part_id};
    });
    entity_snapshots.swap(entity_snapshot_scratch);
    joint_snapshots.clear();
    joint_snapshots.reserve(joint_records.size());
    for (const JointRecord& record : joint_records) {
        joint_snapshots.push_back(record.snapshot);
    }
#if defined(NINHO_ENABLE_TEST_FACADES)
    ++snapshot_rebuilds;
#endif
}

ContentResult<std::unique_ptr<SimulationSession>> SimulationSession::create(
    const MaterialCatalog& materials,
    const ArchetypeCatalog& archetypes,
    const LevelManifest& level)
{
    ContentBundle content;
    if (materials.source_schema_version == 1U
        && archetypes.source_schema_version == 1U
        && level.source_schema_version == 1U) {
        const auto legacy = make_content_bundle(materials, archetypes, level);
        if (!legacy.ok()) return {{}, legacy.error};
        content = legacy.value;
    } else if (materials.source_schema_version == 2U
        && archetypes.source_schema_version == 2U
        && level.source_schema_version == 2U) {
        if (level.bodies.size() > 500U) {
            return {{}, {ContentErrorCode::ResourceLimit, "/bodies",
                "physics body capacity has been exceeded"}};
        }
        for (std::size_t index = 0; index < level.bodies.size(); ++index) {
            const BodyDefinition& body = level.bodies[index];
            const std::string pointer = "/bodies/" + std::to_string(index);
            if (body.body_type == BodyType::Dynamic && !body.affected_by_world_gravity) {
                return {{}, {ContentErrorCode::InvalidInvariant,
                    pointer + "/affected_by_world_gravity",
                    "dynamic bodies must use world gravity"}};
            }
            if (const auto error = validate_shape(body.shape, pointer + "/shape")) {
                return {{}, *error};
            }
            if (!std::isfinite(body.density_kg_m3) || body.density_kg_m3 < 0.0
                || (body.body_type == BodyType::Dynamic && body.density_kg_m3 == 0.0)) {
                return {{}, {ContentErrorCode::InvalidInvariant, pointer + "/density_kg_m3",
                    "body density must be positive and finite"}};
            }
            if (body.material_id) {
                if (find_material(materials, *body.material_id) == nullptr) {
                    return {{}, {ContentErrorCode::MissingReference,
                        pointer + "/material_id", "material reference not found"}};
                }
            } else if (!body.surface_id
                || find_surface(materials, *body.surface_id) == nullptr) {
                return {{}, {ContentErrorCode::MissingReference,
                    pointer + "/surface_id", "surface reference not found"}};
            }
        }
        content = {materials, archetypes, level};
    } else {
        return {{}, {ContentErrorCode::InvalidInvariant, "/schema_version",
            "session content documents must use one schema version"}};
    }
    try {
        auto implementation = std::make_unique<Impl>(std::move(content));
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
