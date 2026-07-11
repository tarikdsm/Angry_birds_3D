#pragma once

#include "ninho/simulation/session.hpp"

#include <ninho/physics/physics_world.hpp>

#include <cstdint>
#include <deque>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

namespace ninho::simulation {

namespace detail {
[[nodiscard]] std::int64_t canonical_quantize(double value);
}

struct SimulationSession::Impl {
    struct QueuedCommand {
        std::uint64_t sequence{};
        PlayerCommand command;
    };

    struct ProjectileState {
        EntityId entity_id;
        BirdArchetypeId archetype_id;
        AbilityId ability_id;
        ninho::physics::BodyHandle physics_handle;
        bool bullet{};
        TickIndex launch_tick{};
        std::optional<TickIndex> ability_start_tick;
        std::optional<TickIndex> ability_end_tick;
        std::uint32_t age_ticks{};
        std::uint32_t rest_ticks{};
        bool finished{};
        bool pending_destroy{};
        bool ability_requested{};
        bool ability_active{};
    };

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
        bool neutralized{};
    };

    struct JointRecord {
        StructuralJointSnapshot snapshot;
        ninho::physics::JointHandle physics_handle;
    };

    explicit Impl(ContentBundle source);

    [[nodiscard]] SessionStatus build() noexcept;
    [[nodiscard]] SessionStatus process_commands();
    [[nodiscard]] SessionStatus create_projectile();
    void update_fsm_before_step();
    void update_fsm_after_step();
    [[nodiscard]] SessionStatus apply_gravity_field_before_step();
    void finish_gravity_field_after_step();
    void remove_confirmed_runtime_body_records();
    void publish_event(DomainEventKind, EntityId = {}, BirdArchetypeId = {},
        CommandRejectionReason = CommandRejectionReason::None);
    void publish_ability_event(DomainEventKind, const BodyRecord* = nullptr,
        double weight = 0.0, ninho::physics::Vec3 force = {},
        ninho::physics::Vec3 impulse = {});
    [[nodiscard]] const BirdArchetype* next_bird_archetype() const noexcept;
    [[nodiscard]] const AbilityArchetype* ability_archetype(AbilityId) const noexcept;
    [[nodiscard]] std::optional<JointEndpoint> domain_identity(
        ninho::physics::BodyHandle) const noexcept;
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
    bool force_settled_for_testing{};
#endif
    std::uint32_t remaining_birds{};
    std::uint64_t next_command_sequence{1};
    std::uint64_t next_event_sequence{1};
    std::uint64_t last_processed_command_sequence{};
    std::uint32_t launch_count{};
    std::uint32_t resolution_rest_ticks{};
    bool objective_complete{};
    std::deque<QueuedCommand> command_queue;
    std::vector<BirdRosterEntry> roster_remaining;
    std::optional<ProjectileState> projectile;
    std::vector<BodyRecord> body_records;
    std::vector<JointRecord> joint_records;
    std::vector<EntitySnapshot> entity_snapshots;
    std::vector<StructuralJointSnapshot> joint_snapshots;
    std::vector<DomainEvent> domain_events;
    std::vector<std::uint8_t> canonical_bytes;
    std::uint64_t canonical_hash{};
};

}
