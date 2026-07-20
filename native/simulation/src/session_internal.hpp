#pragma once

#include "ninho/simulation/session.hpp"
#include "ability_system.hpp"
#include "damage_system.hpp"
#include "launcher_system.hpp"

#include <ninho/physics/physics_world.hpp>

#include <cstddef>
#include <cstdint>
#include <deque>
#include <optional>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace ninho::simulation {

namespace detail {
inline constexpr std::uint32_t runtime_entity_namespace_bit = 0x80000000U;
inline constexpr std::uint32_t split_child_entity_namespace_begin = 0xC0000000U;
inline constexpr std::uint32_t original_projectile_ordinal_limit =
    split_child_entity_namespace_begin - runtime_entity_namespace_bit;

[[nodiscard]] std::int64_t canonical_quantize(double value);
[[nodiscard]] std::uint8_t canonical_tag_of(SessionPhase value);
[[nodiscard]] std::uint8_t canonical_tag_of(Outcome value);
[[nodiscard]] std::uint8_t canonical_tag_of(DomainEventKind value);
[[nodiscard]] std::uint8_t canonical_tag_of(CommandRejectionReason value);
[[nodiscard]] std::uint8_t canonical_tag_of(NeutralizationCause value);
[[nodiscard]] std::uint8_t canonical_tag_of(DamageClassification value);
[[nodiscard]] std::uint8_t canonical_tag_of(MaterialResponse value);
[[nodiscard]] std::uint8_t canonical_tag_of(BodyType value);
[[nodiscard]] std::uint8_t canonical_tag_of(ShapeType value);
[[nodiscard]] std::uint8_t canonical_tag_of(JointKind value);
[[nodiscard]] std::uint8_t canonical_tag_of(ObjectiveKind value);
[[nodiscard]] std::uint8_t canonical_tag_of(AbilityKind value);
[[nodiscard]] std::uint8_t canonical_tag_of(EnvironmentalTriggerKind value);
[[nodiscard]] std::uint8_t canonical_tag_of(const PlayerCommand& value);
[[nodiscard]] std::uint8_t canonical_tag_of(const WorldDefinition& value);
[[nodiscard]] std::uint8_t canonical_tag_of(const AbilityRuntime& value);
}

struct SimulationSession::Impl : detail::AbilityLifecycleHooks {
    struct QueuedCommand {
        std::uint64_t sequence{};
        PlayerCommand command;
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
        bool is_projectile{};
        bool affected_by_world_gravity{};
    };

    struct JointRecord {
        StructuralJointSnapshot snapshot;
        ninho::physics::JointHandle physics_handle;
        std::uint8_t consecutive_overload_ticks{};
    };

    struct PendingJointBreak {
        JointId joint_id;
        EventId cause_event_id;
    };

    struct PendingPieceFracture {
        EntityId entity_id;
        PartId part_id;
        MaterialId material_id;
        JointId incident_joint_id;
        EventId cause_event_id;
        ninho::physics::Vec3 position_m;
    };

    explicit Impl(ContentBundle source);

    [[nodiscard]] SessionStatus build() noexcept;
    [[nodiscard]] SessionStatus process_commands();
    [[nodiscard]] SessionStatus create_projectile(const AimState& launch_state);
    void update_fsm_before_step();
    void update_fsm_after_step();
    [[nodiscard]] SessionStatus apply_ability_before_step();
    [[nodiscard]] SessionStatus process_ability_after_step();
    [[nodiscard]] SessionStatus finish_ability_after_step();
    [[nodiscard]] SessionStatus apply_before_step(ShotState&,
        const GravityFieldAbilityDefinition&, GravityFieldAbilityRuntime&) override;
    [[nodiscard]] SessionStatus finish_after_step(ShotState&,
        const GravityFieldAbilityDefinition&, GravityFieldAbilityRuntime&) override;
    [[nodiscard]] SessionStatus apply_before_step(ShotState&,
        const MassBoostAbilityDefinition&, MassBoostAbilityRuntime&) override;
    [[nodiscard]] SessionStatus finish_after_step(ShotState&,
        const MassBoostAbilityDefinition&, MassBoostAbilityRuntime&) override;
    void observe_speed_boost_direction(
        ShotState&, SpeedBoostAbilityRuntime&) noexcept;
    [[nodiscard]] SessionStatus apply_before_step(ShotState&,
        const SpeedBoostAbilityDefinition&, SpeedBoostAbilityRuntime&) override;
    [[nodiscard]] SessionStatus finish_after_step(ShotState&,
        const SpeedBoostAbilityDefinition&, SpeedBoostAbilityRuntime&) override;
    [[nodiscard]] SessionStatus preflight_split_ability(
        ShotState&, const SplitAbilityDefinition&);
    [[nodiscard]] SessionStatus apply_before_step(ShotState&,
        const SplitAbilityDefinition&, SplitAbilityRuntime&) override;
    [[nodiscard]] SessionStatus finish_after_step(ShotState&,
        const SplitAbilityDefinition&, SplitAbilityRuntime&) override;
    void retire_finished_projectiles();
    void process_damage_after_step();
    void publish_damage_outcomes(std::span<const detail::DamageOutcome>);
    void apply_pending_fractures_before_step();
    void evaluate_fractures_after_step();
    void evaluate_objectives_after_step();
    void remove_confirmed_runtime_body_records();
    void publish_event(DomainEventKind, EntityId = {}, BirdArchetypeId = {},
        CommandRejectionReason = CommandRejectionReason::None);
    void publish_ability_event(DomainEventKind, const BodyRecord* = nullptr,
        double weight = 0.0, ninho::physics::Vec3 force = {},
        ninho::physics::Vec3 impulse = {},
        ninho::physics::Vec3 delta_velocity = {});
    [[nodiscard]] const BirdArchetype* next_bird_archetype() const noexcept;
    [[nodiscard]] const AbilityArchetype* ability_archetype(AbilityId) const noexcept;
    [[nodiscard]] std::optional<JointEndpoint> domain_identity(
        ninho::physics::BodyHandle) const noexcept;
    void rebuild_snapshots();
    void rebuild_canonical_static_content();
    [[nodiscard]] std::vector<std::uint8_t> serialize_canonical_state(
        const std::vector<std::uint8_t>& material_blob,
        const std::vector<std::uint8_t>& archetype_blob,
        const std::vector<std::uint8_t>& level_blob) const;
    [[nodiscard]] std::vector<std::uint8_t> serialize_canonical_state_v3() const;
    void refresh_canonical_state();
#if defined(NINHO_ENABLE_TEST_FACADES)
    [[nodiscard]] std::vector<std::uint8_t> canonical_state_uncached_for_testing() const;
#endif

    ContentBundle bundle;
    ninho::physics::PhysicsWorld physics;
    detail::DamageSystem damage_system;
    std::optional<detail::LauncherSystem> launcher_system;
    detail::AbilitySystem ability_system;
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
    std::optional<ShotState> shot;
    std::vector<BodyRecord> body_records;
    std::vector<JointRecord> joint_records;
    std::vector<PendingJointBreak> pending_joint_breaks;
    std::vector<PendingPieceFracture> pending_piece_fractures;
    std::vector<std::pair<EntityId, PartId>> fractured_pieces;
#if defined(NINHO_ENABLE_TEST_FACADES)
    struct JointRatioOverride {
        double ratio{};
    };
    std::unordered_map<std::uint32_t, JointRatioOverride>
        joint_ratio_overrides_for_testing;
    struct PieceFractureRequest {
        EntityId entity_id;
        PartId part_id;
        ninho::physics::Vec3 position_m;
        bool tie_first_two_incident{};
    };
    std::vector<PieceFractureRequest> piece_fracture_requests_for_testing;
#endif
    std::vector<EntitySnapshot> entity_snapshots;
    std::vector<EntitySnapshot> entity_snapshot_scratch;
    std::vector<const ninho::physics::BodyState*> physics_state_by_handle_index;
    std::unordered_map<std::uint64_t, std::size_t> snapshot_index_by_identity;
    std::vector<StructuralJointSnapshot> joint_snapshots;
    std::vector<DomainEvent> domain_events;
    // ContentBundle is immutable for one Impl. Reconfigure atomically swaps in
    // a newly built Impl with a fresh set of these canonical blobs.
    std::vector<std::uint8_t> canonical_material_blob;
    std::vector<std::uint8_t> canonical_archetype_blob;
    std::vector<std::uint8_t> canonical_level_blob;
#if defined(NINHO_ENABLE_TEST_FACADES)
    std::size_t canonical_static_content_builds{};
    std::size_t snapshot_rebuilds{};
    std::size_t snapshot_visual_id_copies{};
#endif
    std::vector<std::uint8_t> canonical_bytes;
    std::uint64_t canonical_hash{};
    std::vector<std::uint8_t> canonical_v3_bytes;
    std::uint64_t canonical_v3_hash{};
};

}
