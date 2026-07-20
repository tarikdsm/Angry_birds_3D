#include "session_internal.hpp"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <limits>
#include <ranges>
#include <stdexcept>
#include <string_view>
#include <type_traits>
#include <utility>
#include <vector>

namespace ninho::simulation::detail {

std::int64_t canonical_quantize(double value)
{
    if (!std::isfinite(value)) {
        throw std::invalid_argument("canonical numeric value must be finite");
    }
    constexpr long double scale = 100000.0L;
    const long double scaled = static_cast<long double>(value) * scale;
    if (!std::isfinite(scaled)) {
        throw std::range_error("canonical numeric value exceeds fixed-point range");
    }
    const long double rounded = std::round(scaled);
    const long double signed_limit = std::ldexp(1.0L, 63);
    if (rounded < -signed_limit || rounded >= signed_limit) {
        throw std::range_error("canonical numeric value exceeds fixed-point range");
    }
    const auto fixed = static_cast<std::int64_t>(rounded);
    return fixed == 0 ? std::int64_t{0} : fixed;
}

}

namespace ninho::simulation {
namespace {

class CanonicalWriter {
public:
    explicit CanonicalWriter(std::size_t reserve_bytes = 0U)
    {
        bytes.reserve(reserve_bytes);
    }

    template <typename Integer>
        requires std::is_integral_v<Integer>
    void integer(Integer value)
    {
        using Unsigned = std::make_unsigned_t<Integer>;
        Unsigned bits = static_cast<Unsigned>(value);
        for (std::size_t index = 0; index < sizeof(Unsigned); ++index) {
            bytes.push_back(static_cast<std::uint8_t>(bits & Unsigned{0xff}));
            bits >>= 8U;
        }
    }

    void boolean(bool value) { integer<std::uint8_t>(value ? 1U : 0U); }

    void text(std::string_view value)
    {
        if (value.size() > std::numeric_limits<std::uint32_t>::max()) {
            throw std::range_error("canonical string exceeds uint32 length");
        }
        integer<std::uint32_t>(static_cast<std::uint32_t>(value.size()));
        bytes.insert(bytes.end(), value.begin(), value.end());
    }

    void blob(const std::vector<std::uint8_t>& value)
    {
        if (value.size() > std::numeric_limits<std::uint32_t>::max()) {
            throw std::range_error("canonical blob exceeds uint32 length");
        }
        integer<std::uint32_t>(static_cast<std::uint32_t>(value.size()));
        bytes.insert(bytes.end(), value.begin(), value.end());
    }

    void quantized(double value) { integer(detail::canonical_quantize(value)); }

    void vector(ninho::physics::Vec3 value)
    {
        quantized(value.x);
        quantized(value.y);
        quantized(value.z);
    }

    void transform(const ninho::physics::Transform& value)
    {
        vector(value.position);
        quantized(value.rotation.x);
        quantized(value.rotation.y);
        quantized(value.rotation.z);
        quantized(value.rotation.w);
    }

    std::vector<std::uint8_t> bytes;
};

template <typename Id>
void identifier(CanonicalWriter& writer, Id id)
{
    writer.integer(id.value());
}

template <typename Id>
void optional_identifier(CanonicalWriter& writer, const std::optional<Id>& id)
{
    writer.boolean(id.has_value());
    if (id) {
        identifier(writer, *id);
    }
}

template <typename T, typename Key>
std::vector<const T*> ordered_by(const std::vector<T>& values, Key T::* member)
{
    std::vector<const T*> ordered;
    ordered.reserve(values.size());
    for (const T& value : values) {
        ordered.push_back(&value);
    }
    std::ranges::sort(ordered, [member](const T* lhs, const T* rhs) {
        return lhs->*member < rhs->*member;
    });
    return ordered;
}

template <typename Integer>
std::vector<Integer> ordered_values(const std::vector<Integer>& values)
{
    std::vector<Integer> ordered = values;
    std::ranges::sort(ordered);
    return ordered;
}

void write_transform(CanonicalWriter& writer, const TransformDefinition& transform)
{
    for (const double value : transform.position_m) {
        writer.quantized(value);
    }
    for (const double value : transform.rotation_xyzw) {
        writer.quantized(value);
    }
}

void write_shape(CanonicalWriter& writer, const ShapeDefinition& shape)
{
    writer.integer(static_cast<std::uint8_t>(shape.type));
    for (const double value : shape.half_extents_m) {
        writer.quantized(value);
    }
    writer.quantized(shape.radius_m);
}

std::vector<std::uint8_t> canonical_material_catalog(const MaterialCatalog& catalog)
{
    if (catalog.source_schema_version != 1U) {
        throw std::invalid_argument("canonical_state_v2 requires schema version 1 content");
    }
    CanonicalWriter writer;
    writer.integer(catalog.schema_version);
    const auto materials = ordered_by(catalog.materials, &MaterialDefinition::id);
    writer.integer<std::uint32_t>(static_cast<std::uint32_t>(materials.size()));
    for (const MaterialDefinition* material : materials) {
        identifier(writer, material->id);
        writer.text(material->key);
        writer.integer(static_cast<std::uint8_t>(material->response));
        writer.quantized(material->density_kg_m3);
        writer.quantized(material->friction);
        writer.quantized(material->restitution);
        writer.quantized(material->toughness);
    }
    const auto surfaces = ordered_by(catalog.surfaces, &PhysicsSurfaceDefinition::id);
    writer.integer<std::uint32_t>(static_cast<std::uint32_t>(surfaces.size()));
    for (const PhysicsSurfaceDefinition* surface : surfaces) {
        identifier(writer, surface->id);
        writer.text(surface->key);
        writer.quantized(surface->density_kg_m3);
        writer.quantized(surface->friction);
        writer.quantized(surface->restitution);
    }
    return std::move(writer.bytes);
}

std::vector<std::uint8_t> canonical_archetype_catalog(const ArchetypeCatalog& catalog)
{
    if (catalog.source_schema_version != 1U) {
        throw std::invalid_argument("canonical_state_v2 requires schema version 1 content");
    }
    CanonicalWriter writer;
    writer.integer(catalog.schema_version);
    const auto abilities = ordered_by(catalog.abilities, &AbilityArchetype::id);
    writer.integer<std::uint32_t>(static_cast<std::uint32_t>(abilities.size()));
    for (const AbilityArchetype* ability : abilities) {
        identifier(writer, ability->id);
        writer.text(ability->key);
        writer.text(ability->kind);
        writer.integer(ability->arm_ticks);
        writer.integer(ability->duration_ticks);
        writer.quantized(ability->radius_m);
        writer.quantized(ability->max_body_mass_kg);
        writer.integer(ability->max_bodies);
        writer.quantized(ability->max_acceleration_m_s2);
        writer.quantized(ability->pulse_speed_m_s);
    }
    const auto birds = ordered_by(catalog.birds, &BirdArchetype::id);
    writer.integer<std::uint32_t>(static_cast<std::uint32_t>(birds.size()));
    for (const BirdArchetype* bird : birds) {
        identifier(writer, bird->id);
        writer.text(bird->key);
        identifier(writer, bird->ability_id);
        identifier(writer, bird->surface_id);
        writer.quantized(bird->mass_kg);
        writer.quantized(bird->density_kg_m3);
        writer.quantized(bird->radius_m);
        writer.quantized(bird->friction);
        writer.quantized(bird->restitution);
        writer.boolean(bird->bullet);
    }
    const auto weakpoints = ordered_by(catalog.weakpoints, &WeakpointProfile::id);
    writer.integer<std::uint32_t>(static_cast<std::uint32_t>(weakpoints.size()));
    for (const WeakpointProfile* weakpoint : weakpoints) {
        identifier(writer, weakpoint->id);
        writer.text(weakpoint->key);
        for (const double component : weakpoint->protected_direction) {
            writer.quantized(component);
        }
        writer.quantized(weakpoint->protected_cone_deg);
        writer.quantized(weakpoint->protected_multiplier);
        writer.quantized(weakpoint->exposed_multiplier);
    }
    const auto enemies = ordered_by(catalog.enemies, &EnemyArchetype::id);
    writer.integer<std::uint32_t>(static_cast<std::uint32_t>(enemies.size()));
    for (const EnemyArchetype* enemy : enemies) {
        identifier(writer, enemy->id);
        writer.text(enemy->key);
        identifier(writer, enemy->weakpoint_id);
        identifier(writer, enemy->surface_id);
        writer.quantized(enemy->mass_kg);
        writer.quantized(enemy->integrity);
        writer.quantized(enemy->damage_energy_j_per_kg);
        writer.quantized(enemy->max_damage);
    }
    return std::move(writer.bytes);
}

std::vector<std::uint8_t> canonical_level_manifest(const LevelManifest& level)
{
    if (level.source_schema_version != 1U) {
        throw std::invalid_argument("canonical_state_v2 requires schema version 1 content");
    }
    CanonicalWriter writer;
    writer.integer(level.schema_version);
    writer.text(level.id);
    identifier(writer, level.planet.entity_id);
    writer.quantized(level.planet.radius_m);
    writer.quantized(level.planet.surface_gravity_m_s2);
    identifier(writer, level.planet.surface_id);
    writer.text(level.planet.visual_id);

    writer.text(level.launch_ring.id);
    writer.quantized(level.launch_ring.shell_offset_m);
    writer.quantized(level.launch_ring.theta_min_deg);
    writer.quantized(level.launch_ring.theta_max_deg);
    writer.quantized(level.launch_ring.phase_speed_min_m_s);
    writer.quantized(level.launch_ring.phase_speed_max_m_s);
    writer.quantized(level.launch_ring.default_speed_m_s);
    writer.text(level.launch_ring.radial_formula);

    const auto roster = ordered_by(level.bird_roster, &BirdRosterEntry::bird_archetype_id);
    writer.integer<std::uint32_t>(static_cast<std::uint32_t>(roster.size()));
    for (const BirdRosterEntry* entry : roster) {
        identifier(writer, entry->bird_archetype_id);
        writer.integer(entry->count);
    }
    const auto free_body_ids = ordered_values(level.free_body_ids);
    writer.integer<std::uint32_t>(static_cast<std::uint32_t>(free_body_ids.size()));
    for (const std::uint32_t id : free_body_ids) {
        writer.integer(id);
    }

    const auto bodies = ordered_by(level.bodies, &BodyDefinition::body_id);
    writer.integer<std::uint32_t>(static_cast<std::uint32_t>(bodies.size()));
    for (const BodyDefinition* body : bodies) {
        writer.integer(body->body_id);
        identifier(writer, body->entity_id);
        identifier(writer, body->part_id);
        writer.integer(static_cast<std::uint8_t>(body->body_type));
        optional_identifier(writer, body->material_id);
        optional_identifier(writer, body->surface_id);
        optional_identifier(writer, body->enemy_archetype_id);
        writer.boolean(body->assembly_id.has_value());
        if (body->assembly_id) {
            writer.integer(*body->assembly_id);
        }
        writer.quantized(body->density_kg_m3);
        write_transform(writer, body->transform);
        write_shape(writer, body->shape);
        writer.text(body->visual.asset_id);
        for (const double value : body->visual.bounds_m) {
            writer.quantized(value);
        }
    }

    const auto joints = ordered_by(level.joints, &JointDefinition::id);
    writer.integer<std::uint32_t>(static_cast<std::uint32_t>(joints.size()));
    for (const JointDefinition* joint : joints) {
        identifier(writer, joint->id);
        writer.integer(joint->assembly_id);
        writer.integer(static_cast<std::uint8_t>(joint->kind));
        writer.integer(joint->body_a_id);
        writer.integer(joint->body_b_id);
        writer.quantized(joint->force_limit_n);
        writer.quantized(joint->torque_limit_nm);
    }

    const auto assemblies = ordered_by(level.assemblies, &AssemblyDefinition::id);
    writer.integer<std::uint32_t>(static_cast<std::uint32_t>(assemblies.size()));
    for (const AssemblyDefinition* assembly : assemblies) {
        writer.integer(assembly->id);
        writer.text(assembly->key);
        const auto body_ids = ordered_values(assembly->body_ids);
        writer.integer<std::uint32_t>(static_cast<std::uint32_t>(body_ids.size()));
        for (const std::uint32_t id : body_ids) {
            writer.integer(id);
        }
        const auto joint_ids = ordered_values(assembly->joint_ids);
        writer.integer<std::uint32_t>(static_cast<std::uint32_t>(joint_ids.size()));
        for (const JointId id : joint_ids) {
            identifier(writer, id);
        }
    }

    const auto objectives = ordered_by(level.objectives, &ObjectiveDefinition::id);
    writer.integer<std::uint32_t>(static_cast<std::uint32_t>(objectives.size()));
    for (const ObjectiveDefinition* objective : objectives) {
        writer.integer(objective->id);
        writer.integer(static_cast<std::uint8_t>(objective->kind));
        identifier(writer, objective->target_entity_id);
    }
    return std::move(writer.bytes);
}

std::uint64_t fnv1a64(const std::vector<std::uint8_t>& bytes) noexcept
{
    std::uint64_t hash = 14695981039346656037ULL;
    for (const std::uint8_t byte : bytes) {
        hash ^= byte;
        hash *= 1099511628211ULL;
    }
    return hash;
}

}

void SimulationSession::Impl::rebuild_canonical_static_content()
{
    auto material_blob = canonical_material_catalog(bundle.materials);
    auto archetype_blob = canonical_archetype_catalog(bundle.archetypes);
    auto level_blob = canonical_level_manifest(bundle.level);
    canonical_material_blob = std::move(material_blob);
    canonical_archetype_blob = std::move(archetype_blob);
    canonical_level_blob = std::move(level_blob);
#if defined(NINHO_ENABLE_TEST_FACADES)
    ++canonical_static_content_builds;
#endif
}

std::vector<std::uint8_t> SimulationSession::Impl::serialize_canonical_state(
    const std::vector<std::uint8_t>& material_blob,
    const std::vector<std::uint8_t>& archetype_blob,
    const std::vector<std::uint8_t>& level_blob) const
{
    CanonicalWriter writer{canonical_bytes.size()};
    writer.text("canonical_state_v2");
    identifier(writer, session_state.tick);
    writer.integer(static_cast<std::uint8_t>(session_state.phase));
    writer.integer(static_cast<std::uint8_t>(session_state.outcome));
    writer.integer(remaining_birds);
    writer.integer(next_command_sequence);
    writer.integer(next_event_sequence);
    writer.text(bundle.level.id);
    writer.blob(material_blob);
    writer.blob(archetype_blob);
    writer.blob(level_blob);

    writer.integer<std::uint32_t>(static_cast<std::uint32_t>(entity_snapshots.size()));
    for (const EntitySnapshot& snapshot : entity_snapshots) {
        identifier(writer, snapshot.entity_id);
        identifier(writer, snapshot.part_id);
        writer.integer(static_cast<std::uint8_t>(snapshot.body_type));
        optional_identifier(writer, snapshot.material_id);
        optional_identifier(writer, snapshot.surface_id);
        optional_identifier(writer, snapshot.enemy_archetype_id);
        write_shape(writer, snapshot.shape);
        writer.text(snapshot.visual_id);
        writer.transform(snapshot.transform);
        writer.vector(snapshot.linear_velocity_m_s);
        writer.vector(snapshot.angular_velocity_rad_s);
        writer.quantized(snapshot.mass_kg);
        writer.boolean(snapshot.awake);
        writer.boolean(snapshot.ejected);
        writer.boolean(snapshot.is_projectile);
    }

    writer.integer<std::uint32_t>(static_cast<std::uint32_t>(joint_snapshots.size()));
    for (const StructuralJointSnapshot& joint : joint_snapshots) {
        identifier(writer, joint.id);
        identifier(writer, joint.a.entity_id);
        identifier(writer, joint.a.part_id);
        identifier(writer, joint.b.entity_id);
        identifier(writer, joint.b.part_id);
        writer.integer(static_cast<std::uint8_t>(joint.kind));
        writer.transform(joint.frame_a);
        writer.transform(joint.frame_b);
        writer.quantized(joint.force_limit_n);
        writer.quantized(joint.torque_limit_nm);
        writer.boolean(joint.active);
    }

    writer.integer<std::uint32_t>(static_cast<std::uint32_t>(domain_events.size()));
    for (const DomainEvent& event : domain_events) {
        identifier(writer, event.tick);
        identifier(writer, event.id);
        writer.integer(static_cast<std::uint8_t>(event.kind));
        identifier(writer, event.entity_id);
        identifier(writer, event.bird_archetype_id);
        writer.integer(static_cast<std::uint8_t>(event.rejection_reason));
        identifier(writer, event.ability_id);
        identifier(writer, event.affected_entity_id);
        identifier(writer, event.affected_part_id);
        writer.quantized(event.weight);
        writer.vector(event.force_n);
        writer.vector(event.impulse_n_s);
        identifier(writer, event.part_id);
        writer.vector(event.position_m);
        writer.vector(event.normal);
        writer.quantized(event.energy_j);
        writer.quantized(event.damage);
        writer.integer(static_cast<std::uint8_t>(event.damage_classification));
        writer.integer(static_cast<std::uint8_t>(event.neutralization_cause));
        identifier(writer, event.cause_event_id);
        identifier(writer, event.joint_id);
        identifier(writer, event.material_id);
        writer.quantized(event.joint_load_ratio);
        writer.quantized(event.fracture_ratio);
    }

    writer.integer<std::uint32_t>(
        static_cast<std::uint32_t>(damage_system.states().size()));
    for (const detail::DamageState& damage : damage_system.states()) {
        identifier(writer, damage.entity_id);
        identifier(writer, damage.part_id);
        optional_identifier(writer, damage.material_id);
        optional_identifier(writer, damage.enemy_archetype_id);
        writer.quantized(damage.material_damage_energy_j);
        writer.quantized(damage.remaining_integrity);
        writer.boolean(damage.was_ejected);
        writer.boolean(damage.neutralized);
    }

    writer.boolean(session_state.aim.has_value());
    if (session_state.aim) {
        writer.vector(session_state.aim->origin_m);
        writer.vector(session_state.aim->tangent_direction);
        writer.quantized(session_state.aim->speed_m_s);
    }
    writer.boolean(session_state.last_impact_m.has_value());
    if (session_state.last_impact_m) {
        writer.vector(*session_state.last_impact_m);
    }
    writer.integer(last_processed_command_sequence);
    writer.integer(launch_count);
    writer.integer(resolution_rest_ticks);
    writer.boolean(objective_complete);
    writer.integer<std::uint32_t>(static_cast<std::uint32_t>(fractured_pieces.size()));
    for (const auto& [entity, part] : fractured_pieces) {
        identifier(writer, entity);
        identifier(writer, part);
    }
    writer.integer<std::uint32_t>(static_cast<std::uint32_t>(pending_joint_breaks.size()));
    for (const PendingJointBreak& pending : pending_joint_breaks) {
        identifier(writer, pending.joint_id);
        identifier(writer, pending.cause_event_id);
    }
    writer.integer<std::uint32_t>(static_cast<std::uint32_t>(pending_piece_fractures.size()));
    for (const PendingPieceFracture& pending : pending_piece_fractures) {
        identifier(writer, pending.entity_id);
        identifier(writer, pending.part_id);
        identifier(writer, pending.material_id);
        identifier(writer, pending.incident_joint_id);
        identifier(writer, pending.cause_event_id);
        writer.vector(pending.position_m);
    }
    writer.integer<std::uint32_t>(static_cast<std::uint32_t>(roster_remaining.size()));
    for (const BirdRosterEntry& entry : roster_remaining) {
        identifier(writer, entry.bird_archetype_id);
        writer.integer(entry.count);
    }
    writer.integer<std::uint32_t>(static_cast<std::uint32_t>(command_queue.size()));
    for (const QueuedCommand& queued : command_queue) {
        writer.integer(queued.sequence);
        writer.integer<std::uint8_t>(static_cast<std::uint8_t>(queued.command.index()));
        if (const auto* update = std::get_if<SetAimCommand>(&queued.command)) {
            writer.vector(update->aim.origin_m);
            writer.vector(update->aim.tangent_direction);
            writer.quantized(update->aim.speed_m_s);
        }
    }
    writer.boolean(projectile.has_value());
    if (projectile) {
        identifier(writer, projectile->entity_id);
        identifier(writer, projectile->archetype_id);
        identifier(writer, projectile->ability_id);
        writer.boolean(projectile->bullet);
        identifier(writer, projectile->launch_tick);
        writer.boolean(projectile->ability_start_tick.has_value());
        if (projectile->ability_start_tick) {
            identifier(writer, *projectile->ability_start_tick);
        }
        writer.boolean(projectile->ability_end_tick.has_value());
        if (projectile->ability_end_tick) {
            identifier(writer, *projectile->ability_end_tick);
        }
        writer.integer(projectile->age_ticks);
        writer.integer(projectile->rest_ticks);
        writer.boolean(projectile->finished);
        writer.boolean(projectile->pending_destroy);
        writer.boolean(projectile->ability_requested);
        writer.boolean(projectile->ability_active);
    }
    writer.integer<std::uint32_t>(static_cast<std::uint32_t>(joint_records.size()));
    for (const JointRecord& joint : joint_records) {
        identifier(writer, joint.snapshot.id);
        writer.integer(joint.consecutive_overload_ticks);
    }
    return std::move(writer.bytes);
}

#if defined(NINHO_ENABLE_TEST_FACADES)
std::vector<std::uint8_t> SimulationSession::Impl::canonical_state_uncached_for_testing() const
{
    return serialize_canonical_state(
        canonical_material_catalog(bundle.materials),
        canonical_archetype_catalog(bundle.archetypes),
        canonical_level_manifest(bundle.level));
}
#endif

void SimulationSession::Impl::refresh_canonical_state()
{
#if defined(NINHO_ENABLE_TEST_FACADES)
    if (canonical_refresh_failure_for_testing) {
        std::string message = std::move(*canonical_refresh_failure_for_testing);
        canonical_refresh_failure_for_testing.reset();
        throw std::runtime_error(message);
    }
#endif
    std::vector<std::uint8_t> next_bytes = serialize_canonical_state(
        canonical_material_blob, canonical_archetype_blob, canonical_level_blob);
    const std::uint64_t next_hash = fnv1a64(next_bytes);
    canonical_bytes = std::move(next_bytes);
    canonical_hash = next_hash;
}

}
