#pragma once

#include "ninho/simulation/session.hpp"

#include <ninho/physics/physics_world.hpp>

#include <cstdint>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

namespace ninho::simulation {

namespace detail {
[[nodiscard]] std::int64_t canonical_quantize(double value);
}

struct SimulationSession::Impl {
    struct BodyRecord {
        std::uint32_t manifest_body_id{};
        EntityId entity_id;
        PartId part_id;
        BodyType body_type{};
        std::optional<MaterialId> material_id;
        std::optional<SurfaceId> surface_id;
        std::optional<EnemyArchetypeId> enemy_archetype_id;
        ShapeDefinition shape;
        std::string visual_id;
        ninho::physics::BodyHandle physics_handle;
    };

    struct JointRecord {
        StructuralJointSnapshot snapshot;
        ninho::physics::JointHandle physics_handle;
    };

    explicit Impl(ContentBundle source);

    [[nodiscard]] SessionStatus build() noexcept;
    void rebuild_snapshots();
    void refresh_canonical_state();

    ContentBundle bundle;
    ninho::physics::PhysicsWorld physics;
    SessionState session_state;
    std::optional<SessionStatus> latched_fault;
    std::optional<SessionStatus> emergency_fault{
        SessionStatus{{ContentErrorCode::InternalError, "", "session tick failed"}}};
#if defined(NINHO_ENABLE_TEST_FACADES)
    std::optional<std::string> canonical_refresh_failure_for_testing;
    std::optional<double> snapshot_mass_override_for_testing;
#endif
    std::uint32_t remaining_birds{};
    std::uint64_t next_command_sequence{1};
    std::uint64_t next_event_sequence{1};
    std::vector<BodyRecord> body_records;
    std::vector<JointRecord> joint_records;
    std::vector<EntitySnapshot> entity_snapshots;
    std::vector<StructuralJointSnapshot> joint_snapshots;
    std::vector<DomainEvent> domain_events;
    std::vector<std::uint8_t> canonical_bytes;
    std::uint64_t canonical_hash{};
};

}
