#pragma once

#include "ninho/simulation/content.hpp"
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
#if defined(NINHO_ENABLE_TEST_FACADES)
class SessionTestFacade;
#endif
}

enum class SessionPhase : std::uint8_t {
    Inspection,
    Aim,
    FlightAbility,
    Resolution,
    Evaluation,
    Result,
    Faulted,
};

enum class Outcome : std::uint8_t {
    None,
    Victory,
    Defeat,
};

struct SessionState {
    TickIndex tick{};
    SessionPhase phase{SessionPhase::Inspection};
    Outcome outcome{Outcome::None};
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
    [[nodiscard]] SessionStatus tick();

    [[nodiscard]] std::span<const EntitySnapshot> snapshots() const noexcept;
    [[nodiscard]] std::span<const StructuralJointSnapshot> structural_joints() const noexcept;
    [[nodiscard]] std::span<const DomainEvent> events() const noexcept;
    [[nodiscard]] const SessionState& state() const noexcept;
    [[nodiscard]] std::uint32_t birds_remaining() const noexcept;
    [[nodiscard]] ninho::physics::WorldMetrics physics_metrics() const noexcept;
    [[nodiscard]] const std::vector<std::uint8_t>& canonical_state_v1() const noexcept;
    [[nodiscard]] std::uint64_t canonical_hash_v1() const noexcept;

private:
#if defined(NINHO_ENABLE_TEST_FACADES)
    friend class detail::SessionTestFacade;
#endif
    struct Impl;
    explicit SimulationSession(std::unique_ptr<Impl>) noexcept;
    std::unique_ptr<Impl> impl_;
};

}
