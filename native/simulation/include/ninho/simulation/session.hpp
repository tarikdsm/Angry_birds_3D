#pragma once

#include "ninho/simulation/content.hpp"
#include "ninho/simulation/commands.hpp"
#include "ninho/simulation/events.hpp"

#include <ninho/physics/physics_types.hpp>
#include <ninho/physics/physics_world.hpp>

#include <cstdint>
#include <memory>
#include <optional>
#include <span>
#include <string>
#include <vector>

namespace ninho::simulation {

namespace detail {
struct FractureAccess;
#if defined(NINHO_ENABLE_TEST_FACADES)
class SessionTestFacade;
#endif
}

enum class SessionPhase : std::uint8_t {
    Inspection = 0,
    Aim = 1,
    FlightAbility = 2,
    Resolution = 3,
    Evaluation = 4,
    Result = 5,
    Faulted = 6,
    Grabbed = 7,
};

enum class Outcome : std::uint8_t {
    None = 0,
    Victory = 1,
    Defeat = 2,
};

struct LauncherState {
    ninho::physics::Vec3 rest_position_m{};
    ninho::physics::Vec3 camera_right{};
    ninho::physics::Vec3 up{};
    ninho::physics::Vec3 horizontal{};
    ninho::physics::Vec3 plane_normal{};
    double pull_horizontal_m{};
    double pull_vertical_m{};
    double extension_m{};
    double spring_energy_j{};
    double launch_energy_j{};
    ninho::physics::Vec3 launch_direction{};
    double predicted_speed_m_s{};
    double deadzone_m{};
    double maximum_extension_m{};

    bool operator==(const LauncherState&) const = default;
};

struct SessionState {
    TickIndex tick{};
    SessionPhase phase{SessionPhase::Inspection};
    Outcome outcome{Outcome::None};
    std::optional<AimState> aim;
    std::optional<LauncherState> launcher;
    std::optional<ninho::physics::Vec3> last_impact_m;
};

struct ObjectiveTargetStatus {
    EntityId entity_id{};
    double current_integrity{};
    double maximum_integrity{};
    bool neutralized{};

    bool operator==(const ObjectiveTargetStatus&) const = default;
};

enum class AbilityReadiness : std::uint8_t {
    Unavailable = 0,
    Arming = 1,
    Armed = 2,
    Active = 3,
    Spent = 4,
};

struct LockedLaunchPlaneView {
    ninho::physics::Vec3 camera_right{};
    ninho::physics::Vec3 up{};
    ninho::physics::Vec3 horizontal{};
    ninho::physics::Vec3 plane_normal{};

    bool operator==(const LockedLaunchPlaneView&) const = default;
};

struct ShotStateView {
    std::uint64_t shot_id{};
    BirdArchetypeId bird_archetype_id{};
    AbilityId ability_id{};
    TickIndex launch_tick{};
    LockedLaunchPlaneView locked_plane;
    double pull_horizontal_m{};
    double pull_vertical_m{};
    bool activation_consumed{};
    AbilityReadiness ability_readiness{AbilityReadiness::Unavailable};
    std::vector<EntityId> projectile_ids;

    bool operator==(const ShotStateView&) const = default;
};

struct SessionStatus {
    ContentError error{};

    [[nodiscard]] bool ok() const noexcept
    {
        return error.code == ContentErrorCode::None;
    }
};

struct EntitySnapshot {
    EntityId entity_id;
    PartId part_id;
    BodyType body_type{};
    std::optional<MaterialId> material_id;
    std::optional<SurfaceId> surface_id;
    std::optional<EnemyArchetypeId> enemy_archetype_id;
    ShapeDefinition shape;
    std::string visual_id;
    ninho::physics::Transform transform;
    ninho::physics::Vec3 linear_velocity_m_s;
    ninho::physics::Vec3 angular_velocity_rad_s;
    double mass_kg{};
    bool awake{};
    bool ejected{};
    // Prepared for canonical_state_v3. The legacy canonical_state_v2 serializer
    // deliberately continues to omit this field.
    bool exited_world{};
    bool is_projectile{};

    bool operator==(const EntitySnapshot&) const = default;
};

struct JointEndpoint {
    EntityId entity_id;
    PartId part_id;

    auto operator<=>(const JointEndpoint&) const = default;
};

struct StructuralJointSnapshot {
    JointId id;
    JointEndpoint a;
    JointEndpoint b;
    JointKind kind{};
    ninho::physics::Transform frame_a;
    ninho::physics::Transform frame_b;
    double force_limit_n{};
    double torque_limit_nm{};
    bool active{true};

    bool operator==(const StructuralJointSnapshot&) const = default;
};

struct TrajectoryHit {
    EntityId entity_id{};
    PartId part_id{};
    ninho::physics::Vec3 point_m{};
    ninho::physics::Vec3 normal{};
};

struct TrajectoryPreview {
    SessionStatus status;
    AimState quantized_aim;
    std::vector<ninho::physics::Vec3> samples;
    std::optional<TrajectoryHit> first_hit;
    std::uint64_t canonical_hash{};
};

class SimulationSession {
public:
    // Mutating operations may allocate while producing status diagnostics. The
    // GDExtension adapter is the exception boundary and must catch before the ABI.
    static ContentResult<std::unique_ptr<SimulationSession>> create(
        const MaterialCatalog&, const ArchetypeCatalog&, const LevelManifest&);

    ~SimulationSession();
    SimulationSession(SimulationSession&&) noexcept;
    SimulationSession& operator=(SimulationSession&&) noexcept;
    SimulationSession(const SimulationSession&) = delete;
    SimulationSession& operator=(const SimulationSession&) = delete;

    [[nodiscard]] SessionStatus reconfigure(
        const MaterialCatalog&, const ArchetypeCatalog&, const LevelManifest&);
    [[nodiscard]] SessionStatus restart();
    [[nodiscard]] SessionStatus enqueue(PlayerCommand);
    [[nodiscard]] SessionStatus tick();
    [[nodiscard]] ContentResult<AimState> quantize_aim(const AimState&) const;
    [[nodiscard]] TrajectoryPreview preview(const AimState&) const;
    [[nodiscard]] TrajectoryPreview preview() const;

    // Non-owning views of buffers published by this session. Any non-const
    // operation on the session, as well as moving or destroying it, may
    // invalidate a previously returned view. Copy elements that must outlive
    // that boundary.
    [[nodiscard]] std::span<const EntitySnapshot> snapshots() const noexcept;
    [[nodiscard]] std::span<const StructuralJointSnapshot> structural_joints() const noexcept;
    [[nodiscard]] std::span<const DomainEvent> events() const noexcept;
    [[nodiscard]] const SessionState& state() const noexcept;
    [[nodiscard]] std::uint32_t birds_remaining() const noexcept;
    [[nodiscard]] bool objectives_complete() const noexcept;
    [[nodiscard]] std::vector<ObjectiveTargetStatus> objective_target_statuses() const;
    [[nodiscard]] AbilityReadiness ability_readiness() const noexcept;
    // Owning value snapshot of the authoritative shot. No pointer or span into
    // the session survives this call. Allocation failures propagate to the
    // caller, and GDExtension adapters must keep this call inside their ABI
    // exception boundary.
    [[nodiscard]] std::optional<ShotStateView> shot_state() const;
    [[nodiscard]] ninho::physics::WorldMetrics physics_metrics() const noexcept;
    // Versioned contracts never alias one another: schema-v1 sessions publish
    // only v2, while schema-v2 sessions publish only v3. The inactive version
    // is always an empty byte vector with hash zero.
    [[nodiscard]] const std::vector<std::uint8_t>& canonical_state_v2() const noexcept;
    [[nodiscard]] std::uint64_t canonical_hash_v2() const noexcept;
    [[nodiscard]] const std::vector<std::uint8_t>& canonical_state_v3() const noexcept;
    [[nodiscard]] std::uint64_t canonical_hash_v3() const noexcept;

private:
    friend struct detail::FractureAccess;
#if defined(NINHO_ENABLE_TEST_FACADES)
    friend class detail::SessionTestFacade;
#endif
    struct Impl;
    explicit SimulationSession(std::unique_ptr<Impl>) noexcept;
    std::unique_ptr<Impl> impl_;
};

}
