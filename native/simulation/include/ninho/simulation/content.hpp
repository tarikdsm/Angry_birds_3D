#pragma once

#include <array>
#include <compare>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace ninho::simulation {

namespace detail {

template <typename Tag, typename Representation>
class StrongId {
public:
    using representation_type = Representation;

    constexpr StrongId() noexcept = default;
    constexpr explicit StrongId(Representation value) noexcept : value_(value) {}
    [[nodiscard]] constexpr Representation value() const noexcept { return value_; }
    constexpr auto operator<=>(const StrongId&) const noexcept = default;

private:
    Representation value_{};
};

struct EntityIdTag;
struct PartIdTag;
struct MaterialIdTag;
struct SurfaceIdTag;
struct AbilityIdTag;
struct BirdArchetypeIdTag;
struct EnemyArchetypeIdTag;
struct WeakpointIdTag;
struct JointIdTag;
struct EventIdTag;
struct TickIndexTag;

}

using EntityId = detail::StrongId<detail::EntityIdTag, std::uint32_t>;
using PartId = detail::StrongId<detail::PartIdTag, std::uint32_t>;
using MaterialId = detail::StrongId<detail::MaterialIdTag, std::uint32_t>;
using SurfaceId = detail::StrongId<detail::SurfaceIdTag, std::uint32_t>;
using AbilityId = detail::StrongId<detail::AbilityIdTag, std::uint32_t>;
using BirdArchetypeId = detail::StrongId<detail::BirdArchetypeIdTag, std::uint32_t>;
using EnemyArchetypeId = detail::StrongId<detail::EnemyArchetypeIdTag, std::uint32_t>;
using WeakpointId = detail::StrongId<detail::WeakpointIdTag, std::uint32_t>;
using JointId = detail::StrongId<detail::JointIdTag, std::uint32_t>;
using EventId = detail::StrongId<detail::EventIdTag, std::uint64_t>;
using TickIndex = detail::StrongId<detail::TickIndexTag, std::uint64_t>;

enum class ContentErrorCode {
    None,
    InvalidJson,
    DuplicateKey,
    ResourceLimit,
    InvalidType,
    InvalidNumber,
    MissingField,
    UnknownKey,
    InvalidEnum,
    OutOfRange,
    DuplicateId,
    MissingReference,
    EmptyAssembly,
    VisualMismatch,
    InvalidInvariant,
    InternalError,
};

struct ContentError {
    ContentErrorCode code{ContentErrorCode::None};
    std::string pointer;
    std::string message;
};

template <typename T>
struct ContentResult {
    T value{};
    ContentError error{};
    [[nodiscard]] bool ok() const noexcept { return error.code == ContentErrorCode::None; }
    explicit operator bool() const noexcept { return ok(); }
};

enum class MaterialResponse { Fibrous, Masonry, Brittle };
enum class BodyType { Static, Dynamic };
enum class ShapeType { Box, Sphere };
enum class JointKind { PineFit, GlassClamp, Mortar };
enum class ObjectiveKind { NeutralizeEntity };

struct MaterialDefinition {
    MaterialId id;
    std::string key;
    MaterialResponse response{};
    double density_kg_m3{};
    double friction{};
    double restitution{};
    double toughness{};
};

struct PhysicsSurfaceDefinition {
    SurfaceId id;
    std::string key;
    double density_kg_m3{};
    double friction{};
    double restitution{};
};

struct MaterialCatalog {
    std::uint32_t schema_version{};
    std::vector<MaterialDefinition> materials;
    std::vector<PhysicsSurfaceDefinition> surfaces;
};

struct AbilityArchetype {
    AbilityId id;
    std::string key;
    std::string kind;
    std::uint32_t arm_ticks{};
    std::uint32_t duration_ticks{};
    double radius_m{};
    double max_body_mass_kg{};
    std::uint32_t max_bodies{};
    double max_acceleration_m_s2{};
    double pulse_speed_m_s{};
};

struct BirdArchetype {
    BirdArchetypeId id;
    std::string key;
    AbilityId ability_id;
    SurfaceId surface_id;
    double mass_kg{};
    double density_kg_m3{};
    double radius_m{};
    double friction{};
    double restitution{};
    bool bullet{};
};

struct WeakpointProfile {
    WeakpointId id;
    std::string key;
    std::array<double, 3> protected_direction{};
    double protected_cone_deg{};
    double protected_multiplier{};
    double exposed_multiplier{};
};

struct EnemyArchetype {
    EnemyArchetypeId id;
    std::string key;
    WeakpointId weakpoint_id;
    SurfaceId surface_id;
    double mass_kg{};
    double integrity{};
    double damage_energy_j_per_kg{};
    double max_damage{};
};

struct ArchetypeCatalog {
    std::uint32_t schema_version{};
    std::vector<AbilityArchetype> abilities;
    std::vector<BirdArchetype> birds;
    std::vector<WeakpointProfile> weakpoints;
    std::vector<EnemyArchetype> enemies;
};

struct PlanetDefinition {
    EntityId entity_id;
    double radius_m{};
    double surface_gravity_m_s2{};
    SurfaceId surface_id;
    std::string visual_id;
};

struct LaunchRingDefinition {
    std::string id;
    double shell_offset_m{};
    double theta_min_deg{};
    double theta_max_deg{};
    double phase_speed_min_m_s{};
    double phase_speed_max_m_s{};
    double default_speed_m_s{};
    std::string radial_formula;
};

struct BirdRosterEntry {
    BirdArchetypeId bird_archetype_id;
    std::uint32_t count{};
};

struct TransformDefinition {
    std::array<double, 3> position_m{};
    std::array<double, 4> rotation_xyzw{};
};

struct ShapeDefinition {
    ShapeType type{};
    std::array<double, 3> half_extents_m{};
    double radius_m{};

    bool operator==(const ShapeDefinition&) const = default;
};

struct VisualDefinition {
    std::string asset_id;
    std::array<double, 3> bounds_m{};
};

struct BodyDefinition {
    std::uint32_t body_id{};
    EntityId entity_id;
    PartId part_id;
    BodyType body_type{};
    std::optional<MaterialId> material_id;
    std::optional<SurfaceId> surface_id;
    std::optional<EnemyArchetypeId> enemy_archetype_id;
    std::optional<std::uint32_t> assembly_id;
    double density_kg_m3{};
    TransformDefinition transform;
    ShapeDefinition shape;
    VisualDefinition visual;
};

struct JointDefinition {
    JointId id;
    std::uint32_t assembly_id{};
    JointKind kind{};
    std::uint32_t body_a_id{};
    std::uint32_t body_b_id{};
    double force_limit_n{};
    double torque_limit_nm{};
};

struct AssemblyDefinition {
    std::uint32_t id{};
    std::string key;
    std::vector<std::uint32_t> body_ids;
    std::vector<JointId> joint_ids;
};

struct ObjectiveDefinition {
    std::uint32_t id{};
    ObjectiveKind kind{};
    EntityId target_entity_id;
};

struct LevelManifest {
    std::uint32_t schema_version{};
    std::string id;
    PlanetDefinition planet;
    LaunchRingDefinition launch_ring;
    // Set keyed by bird_archetype_id: one count entry per archetype.
    std::vector<BirdRosterEntry> bird_roster;
    std::vector<std::uint32_t> free_body_ids;
    std::vector<BodyDefinition> bodies;
    std::vector<JointDefinition> joints;
    std::vector<AssemblyDefinition> assemblies;
    std::vector<ObjectiveDefinition> objectives;
};

struct ContentBundle {
    MaterialCatalog materials;
    ArchetypeCatalog archetypes;
    LevelManifest level;
};

[[nodiscard]] ContentResult<MaterialCatalog> parse_material_catalog(std::string_view text) noexcept;
[[nodiscard]] ContentResult<ArchetypeCatalog> parse_archetype_catalog(std::string_view text) noexcept;
[[nodiscard]] ContentResult<LevelManifest> parse_level_manifest(std::string_view text) noexcept;
[[nodiscard]] ContentResult<ContentBundle> make_content_bundle(
    const MaterialCatalog&, const ArchetypeCatalog&, const LevelManifest&) noexcept;

[[nodiscard]] std::string to_canonical_json(const MaterialCatalog&);
[[nodiscard]] std::string to_canonical_json(const ArchetypeCatalog&);
[[nodiscard]] std::string to_canonical_json(const LevelManifest&);
[[nodiscard]] std::string_view content_error_code_name(ContentErrorCode) noexcept;

}
