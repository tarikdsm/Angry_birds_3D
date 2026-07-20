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
    static bool ability_requested(const SimulationSession&);
    static bool ability_active(const SimulationSession&);
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
};

}
}
