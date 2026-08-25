#pragma once

#include <array>
#include <compare>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <variant>
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

enum class ContentErrorCode : std::uint8_t {
    None = 0,
    InvalidJson = 1,
    DuplicateKey = 2,
    ResourceLimit = 3,
    InvalidType = 4,
    InvalidNumber = 5,
    MissingField = 6,
    UnknownKey = 7,
    InvalidEnum = 8,
    OutOfRange = 9,
    DuplicateId = 10,
    MissingReference = 11,
    EmptyAssembly = 12,
    VisualMismatch = 13,
    InvalidInvariant = 14,
    InternalError = 15,
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

enum class MaterialResponse : std::uint8_t {
    Fibrous = 0, Masonry = 1, Brittle = 2, Compressible = 3, Ductile = 4};
enum class BodyType : std::uint8_t { Static = 0, Dynamic = 1 };
enum class ShapeType : std::uint8_t {
    Box = 0, Sphere = 1, Capsule = 2, ConvexHull = 3, Compound = 4};
enum class JointKind : std::uint8_t {
    PineFit = 0, GlassClamp = 1, Mortar = 2, StrawBind = 3, SteelDuctile = 4};
enum class EnemyDamageModel : std::uint8_t {
    LegacyDirectionalEnergy = 0, TerrestrialPig = 1};
enum class ObjectiveKind : std::uint8_t { NeutralizeEntity = 0 };
enum class AbilityKind : std::uint8_t {
    LegacyGravityField = 0,
    GravityField = 1,
    MassBoost = 2,
    SpeedBoost = 3,
    Explosion = 4,
    Split = 5,
};
enum class EnvironmentalTriggerKind : std::uint8_t { DamageThreshold = 0 };

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
    std::uint32_t source_schema_version{};
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
    AbilityKind kind_v2{AbilityKind::LegacyGravityField};
    struct GravityFieldPayload {
        std::uint32_t arm_ticks{};
        std::uint32_t duration_ticks{};
        double radius_m{};
        double max_body_mass_kg{};
        std::uint32_t max_bodies{};
        double max_acceleration_m_s2{};
        double pulse_speed_m_s{};
        bool operator==(const GravityFieldPayload&) const = default;
    };
    struct MassBoostPayload {
        std::uint32_t duration_ticks{};
        double mass_multiplier{};
        bool operator==(const MassBoostPayload&) const = default;
    };
    struct SpeedBoostPayload {
        double fallback_speed_m_s{};
        bool operator==(const SpeedBoostPayload&) const = default;
    };
    struct ExplosionPayload {
        double radius_m{};
        double impulse_n_s{};
        double energy_j{};
        std::uint32_t max_bodies{};
        bool operator==(const ExplosionPayload&) const = default;
    };
    struct SplitPayload {
        std::uint32_t child_count{};
        double spread_angle_deg{};
        double child_speed_multiplier{};
        bool operator==(const SplitPayload&) const = default;
    };
    using Payload = std::variant<GravityFieldPayload, MassBoostPayload, SpeedBoostPayload,
        ExplosionPayload, SplitPayload>;
    Payload payload{};
};

using GravityFieldAbilityDefinition = AbilityArchetype::GravityFieldPayload;
using MassBoostAbilityDefinition = AbilityArchetype::MassBoostPayload;
using SpeedBoostAbilityDefinition = AbilityArchetype::SpeedBoostPayload;
using ExplosionAbilityDefinition = AbilityArchetype::ExplosionPayload;
using SplitAbilityDefinition = AbilityArchetype::SplitPayload;

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
    std::string projectile_visual_id;
    double launch_speed_cap_m_s{};
    std::string score_id;
    std::string icon_id;
    std::string animation_id;
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
    EnemyDamageModel damage_model{EnemyDamageModel::LegacyDirectionalEnergy};
};

struct ArchetypeCatalog {
    std::uint32_t schema_version{};
    std::uint32_t source_schema_version{};
    std::vector<std::string> presentation_ids;
    std::vector<std::string> score_ids;
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
    double half_height_m{};
    std::vector<std::array<double, 3>> vertices_m;
    std::vector<ShapeDefinition> children;
    std::array<double, 3> local_position_m{};
    std::array<double, 4> local_rotation_xyzw{0.0, 0.0, 0.0, 1.0};

    bool operator==(const ShapeDefinition&) const = default;
};

struct VisualDefinition {
    std::string asset_id;
    std::array<double, 3> bounds_m{};
};

struct PhysicalFragmentDefinition {
    std::uint32_t ordinal{};
    ShapeDefinition shape;
    TransformDefinition local_transform;
    double density_kg_m3{};
    std::string visual_id;

    bool operator==(const PhysicalFragmentDefinition&) const = default;
};

struct FracturePatternDefinition {
    std::vector<PhysicalFragmentDefinition> physical_fragments;
    std::vector<std::string> cosmetic_asset_ids;

    bool operator==(const FracturePatternDefinition&) const = default;
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
    bool affected_by_world_gravity{true};
    std::optional<FracturePatternDefinition> fracture_pattern;
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

struct UniformWorldDefinition {
    std::array<double, 3> acceleration_m_s2{};
    std::array<double, 3> bounds_min_m{};
    std::array<double, 3> bounds_max_m{};
    bool operator==(const UniformWorldDefinition&) const = default;
};

struct RadialWorldDefinition {
    std::array<double, 3> center_m{};
    double reference_radius_m{};
    double reference_acceleration_m_s2{};
    double bounds_radius_m{};
    bool operator==(const RadialWorldDefinition&) const = default;
};

using WorldDefinition = std::variant<UniformWorldDefinition, RadialWorldDefinition>;

struct SlingshotDefinition {
    std::string asset_id;
    std::array<double, 3> rest_position_m{};
    std::array<double, 4> rest_rotation_xyzw{};
    double spring_constant_n_m{};
    double energy_efficiency{};
    double minimum_extension_m{};
    double maximum_extension_m{};
    std::string plane_policy;
    double projectile_clearance_m{};
    double speed_ceiling_m_s{};
};

struct ScoringDefinition {
    std::uint32_t pig_points{};
    std::uint32_t unused_bird_points{};
    std::array<std::uint32_t, 3> star_thresholds{};
    std::uint32_t chain_window_ticks{};
    double chain_multiplier_step{};
    double max_chain_multiplier{};
};

struct PressureBurstDefinition {
    double radius_m{};
    double impulse_n_s{};
    double energy_j{};
    bool line_of_sight{};
    std::uint32_t max_bodies{};
};

struct EnvironmentalTriggerDefinition {
    std::uint32_t id{};
    EntityId target_entity_id;
    EnvironmentalTriggerKind kind{EnvironmentalTriggerKind::DamageThreshold};
    double damage_threshold{};
    std::uint32_t fuse_ticks{};
    std::uint32_t cooldown_ticks{};
    PressureBurstDefinition pressure_burst;
};

struct SettlePolicyDefinition {
    double linear_speed_m_s{};
    double angular_speed_rad_s{};
    std::uint32_t rest_ticks{};
};

struct LevelManifest {
    std::uint32_t schema_version{};
    std::uint32_t source_schema_version{};
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
    std::string world_id;
    std::string region_id;
    std::string camera_profile_id;
    std::string presentation_profile_id;
    WorldDefinition world{};
    SlingshotDefinition slingshot;
    std::vector<BirdArchetypeId> bird_queue;
    ScoringDefinition scoring;
    std::vector<EnvironmentalTriggerDefinition> triggers;
    SettlePolicyDefinition settle_policy;
    std::uint32_t watchdog_ticks{};
};

struct CampaignLevelDefinition {
    std::string id;
    std::string region_id;
    std::string camera_profile_id;
    std::string presentation_profile_id;
    std::string scene_id;
    std::optional<std::string> unlock_after_level_id;
};

struct CampaignWorldDefinition {
    std::string id;
    std::string diorama_id;
    std::string text_id;
    std::string default_level_id;
    std::vector<std::string> level_order;
    std::vector<CampaignLevelDefinition> levels;
};

struct CampaignManifest {
    std::uint32_t schema_version{};
    std::uint32_t source_schema_version{};
    std::string default_world_id;
    std::vector<std::string> world_order;
    std::vector<std::string> scene_ids;
    std::vector<std::string> diorama_ids;
    std::vector<std::string> text_ids;
    std::vector<CampaignWorldDefinition> worlds;
};

struct ContentBundle {
    MaterialCatalog materials;
    ArchetypeCatalog archetypes;
    LevelManifest level;
};

struct ProductV2ContentBundle {
    MaterialCatalog materials;
    ArchetypeCatalog archetypes;
    CampaignManifest campaign;
    LevelManifest level;
};

[[nodiscard]] ContentResult<MaterialCatalog> parse_material_catalog(std::string_view text) noexcept;
[[nodiscard]] ContentResult<ArchetypeCatalog> parse_archetype_catalog(std::string_view text) noexcept;
[[nodiscard]] ContentResult<LevelManifest> parse_level_manifest(std::string_view text) noexcept;
[[nodiscard]] ContentResult<MaterialCatalog> parse_material_catalog_v1(std::string_view text) noexcept;
[[nodiscard]] ContentResult<MaterialCatalog> parse_material_catalog_v2(std::string_view text) noexcept;
[[nodiscard]] ContentResult<ArchetypeCatalog> parse_archetype_catalog_v1(std::string_view text) noexcept;
[[nodiscard]] ContentResult<ArchetypeCatalog> parse_archetype_catalog_v2(std::string_view text) noexcept;
[[nodiscard]] ContentResult<LevelManifest> parse_level_manifest_v1(std::string_view text) noexcept;
[[nodiscard]] ContentResult<LevelManifest> parse_level_manifest_v2(std::string_view text) noexcept;
[[nodiscard]] ContentResult<CampaignManifest> parse_campaign_manifest(std::string_view text) noexcept;
[[nodiscard]] ContentResult<CampaignManifest> parse_campaign_manifest_v2(std::string_view text) noexcept;
[[nodiscard]] ContentResult<ContentBundle> make_content_bundle(
    const MaterialCatalog&, const ArchetypeCatalog&, const LevelManifest&) noexcept;
[[nodiscard]] ContentResult<ProductV2ContentBundle> make_product_v2_content_bundle(
    const MaterialCatalog&, const ArchetypeCatalog&, const CampaignManifest&,
    const LevelManifest&) noexcept;

[[nodiscard]] std::string to_canonical_json(const MaterialCatalog&);
[[nodiscard]] std::string to_canonical_json(const ArchetypeCatalog&);
[[nodiscard]] std::string to_canonical_json(const LevelManifest&);
[[nodiscard]] std::string to_canonical_json(const CampaignManifest&);
[[nodiscard]] std::string_view content_error_code_name(ContentErrorCode) noexcept;

}
