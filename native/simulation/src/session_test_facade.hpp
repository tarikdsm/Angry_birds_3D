#pragma once

#if !defined(NINHO_ENABLE_TEST_FACADES)
#error "SessionTestFacade is available only in test builds"
#endif

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>
#include <ninho/physics/physics_world.hpp>
#include "ninho/simulation/content.hpp"

namespace ninho::simulation {

class SimulationSession;
struct EntitySnapshot;

namespace detail {

class SessionTestFacade {
public:
    static void fail_next_canonical_refresh(SimulationSession&, std::string message);
    static void override_next_snapshot_mass(SimulationSession&, double mass_kg);
    static std::int64_t quantize_canonical(double value);
    static std::uint64_t last_processed_command_sequence(const SimulationSession&);
    static bool projectile_is_bullet(const SimulationSession&);
    static void finish_projectile(SimulationSession&);
    static void finish_projectile(SimulationSession&, EntityId);
    static bool append_projectile_body_for_testing(SimulationSession&, EntityId,
        ninho::physics::Vec3 position, ninho::physics::Vec3 linear_velocity);
    static std::uint32_t projectile_age(const SimulationSession&, EntityId);
    static bool has_body_record(const SimulationSession&, EntityId);
    static void complete_objective(SimulationSession&);
    static void set_ability_active(SimulationSession&, bool);
    static void age_projectile(SimulationSession&, std::uint32_t age_ticks);
    static void age_projectile(
        SimulationSession&, EntityId, std::uint32_t age_ticks);
    static bool ability_requested(const SimulationSession&);
    static bool ability_active(const SimulationSession&);
    static void clear_speed_boost_direction_for_testing(SimulationSession&);
    static void set_speed_boost_direction_for_testing(
        SimulationSession&, ninho::physics::Vec3);
    static bool set_last_speed_changed_delta_for_testing(
        SimulationSession&, ninho::physics::Vec3);
    static int collision_group(const SimulationSession&, EntityId);
    static bool set_split_child_id_for_testing(
        SimulationSession&, std::size_t index, EntityId);
    static bool set_split_grace_end_for_testing(
        SimulationSession&, TickIndex);
    static void invalidate_locked_plane_for_testing(SimulationSession&);
    static bool impulse_entity(SimulationSession&, EntityId, ninho::physics::Vec3);
    static bool add_static_sphere(
        SimulationSession&, EntityId, ninho::physics::Vec3, double radius_m);
    static bool add_dynamic_sphere(SimulationSession&, EntityId, PartId,
        ninho::physics::Vec3, double mass_kg,
        ninho::physics::Vec3 linear_velocity = {}, double radius_m = 0.1);
    static bool affected_by_world_gravity(
        const SimulationSession&, EntityId, PartId);
    static ninho::physics::Vec3 gravity_at(
        const SimulationSession&, ninho::physics::Vec3);
    static std::optional<ninho::physics::Aabb> body_bounds(
        const SimulationSession&, EntityId, PartId);
    static std::optional<ninho::physics::Vec3> body_center_of_mass(
        const SimulationSession&, EntityId, PartId);
    static bool set_body_neutralized(
        SimulationSession&, EntityId, PartId, bool neutralized);
    static void override_joint_ratio_after_solver(SimulationSession&, JointId, double ratio);
    static void override_joint_ratio_without_new_cause_after_solver(
        SimulationSession&, JointId, double ratio);
    static void fracture_piece_after_solver(
        SimulationSession&, EntityId, PartId, ninho::physics::Vec3 position);
    static void fracture_piece_at_incident_tie_after_solver(
        SimulationSession&, EntityId, PartId);
    static TickIndex projectile_launch_tick(const SimulationSession&);
    static TickIndex ability_end_tick(const SimulationSession&);
    static BirdArchetypeId next_bird_archetype_id(const SimulationSession&);
    static AbilityId ability_id(const SimulationSession&);
    static void set_launch_ordinal(SimulationSession&, std::uint32_t);
    static std::uint32_t launch_ordinal(const SimulationSession&);
    static std::size_t remaining_body_capacity(const SimulationSession&);
    static std::size_t body_record_count(const SimulationSession&);
    static std::size_t canonical_static_content_build_count(const SimulationSession&);
    static std::vector<std::uint8_t> canonical_state_uncached(const SimulationSession&);
    static void refresh_canonical_state(SimulationSession&);
    static std::size_t snapshot_rebuild_count(const SimulationSession&);
    static std::size_t snapshot_visual_copy_count(const SimulationSession&);
    static std::vector<EntitySnapshot> snapshots_uncached(const SimulationSession&);
    static void rebuild_snapshots(SimulationSession&);
    static bool set_snapshot_exited_world(
        SimulationSession&, EntityId, PartId, bool exited_world);
    static bool append_projectile_for_testing(SimulationSession&, EntityId);
    static void set_shot_runtime_for_testing(SimulationSession&, bool consumed,
        bool active, std::optional<TickIndex> start_tick,
        std::optional<TickIndex> end_tick);
    static void inject_explosion_contact(
        SimulationSession&, float approach_speed_m_s,
        float total_normal_impulse_n_s);
    static void fail_next_pressure_burst_after_plan(SimulationSession&);
    static std::size_t pending_pressure_burst_count(const SimulationSession&);
    static std::size_t pending_external_damage_count(const SimulationSession&);
    static bool shift_explosion_fuse_due_tick(SimulationSession&);
    static bool shift_explosion_detonation_tick(SimulationSession&);
    static bool toggle_explosion_detonated(SimulationSession&);
    static bool bump_trigger_initiating_damage_event(SimulationSession&, std::uint32_t);
    static bool toggle_trigger_armed(SimulationSession&, std::uint32_t);
    static bool shift_trigger_cooldown_until_tick(SimulationSession&, std::uint32_t);
    static bool bump_trigger_accumulated_damage(SimulationSession&, std::uint32_t);
    static bool shift_trigger_captured_origin(SimulationSession&, std::uint32_t);
    static bool queue_external_damage(SimulationSession&, EntityId target,
        PartId target_part, double energy_j, EventId cause_event_id);
    static void inject_crush_load(SimulationSession&, EntityId, PartId,
        double total_normal_impulse_n_s);
    static bool set_crush_load_cause_for_testing(
        SimulationSession&, EntityId, PartId, EntityId);
    static bool bump_score_total(SimulationSession&);
    static bool bump_score_root(SimulationSession&);
    static bool shift_score_tick(SimulationSession&);
    static bool bump_score_chain(SimulationSession&);
    static bool append_score_identity(SimulationSession&);
    static bool reverse_score_identities(SimulationSession&);
    static bool toggle_score_terminal_gate(SimulationSession&);
    static bool bump_score_stars(SimulationSession&);
};

}
}
