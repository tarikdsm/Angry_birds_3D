#pragma once

#if !defined(NINHO_ENABLE_TEST_FACADES)
#error "SessionTestFacade is available only in test builds"
#endif

#include <cstddef>
#include <cstdint>
#include <string>
#include <ninho/physics/physics_types.hpp>
#include "ninho/simulation/content.hpp"

namespace ninho::simulation {

class SimulationSession;

namespace detail {

class SessionTestFacade {
public:
    static void fail_next_canonical_refresh(SimulationSession&, std::string message);
    static void override_next_snapshot_mass(SimulationSession&, double mass_kg);
    static std::int64_t quantize_canonical(double value);
    static std::uint64_t last_processed_command_sequence(const SimulationSession&);
    static bool projectile_is_bullet(const SimulationSession&);
    static void finish_projectile(SimulationSession&);
    static void complete_objective(SimulationSession&);
    static void set_ability_active(SimulationSession&, bool);
    static void age_projectile(SimulationSession&, std::uint32_t age_ticks);
    static bool ability_requested(const SimulationSession&);
    static bool ability_active(const SimulationSession&);
    static bool impulse_entity(SimulationSession&, EntityId, ninho::physics::Vec3);
    static bool add_static_sphere(
        SimulationSession&, EntityId, ninho::physics::Vec3, double radius_m);
    static TickIndex projectile_launch_tick(const SimulationSession&);
    static TickIndex ability_end_tick(const SimulationSession&);
    static BirdArchetypeId next_bird_archetype_id(const SimulationSession&);
    static AbilityId ability_id(const SimulationSession&);
    static void set_launch_ordinal(SimulationSession&, std::uint32_t);
    static std::size_t body_record_count(const SimulationSession&);
};

}
}
