#pragma once

#include "ninho/simulation/content.hpp"
#include "ninho/simulation/events.hpp"
#include "ninho/simulation/session.hpp"

#include <ninho/physics/physics_world.hpp>

#include <span>
#include <vector>

namespace ninho::simulation::detail {

enum class PressureBurstOriginKind : std::uint8_t {
    ManualAbility = 0,
    FuseAbility = 1,
    EnvironmentalTrigger = 2,
};

struct PressureBurstBody {
    EntityId entity_id{};
    PartId part_id{};
    ninho::physics::BodyHandle physics_handle{};
    BodyType body_type{BodyType::Static};
    double structural_mass_kg{};
};

struct PressureBurstRequest {
    PressureBurstOriginKind origin_kind{PressureBurstOriginKind::ManualAbility};
    std::uint32_t origin_identity{};
    EntityId source_entity_id{};
    PartId source_part_id{};
    ninho::physics::Vec3 origin_m{};
    PressureBurstDefinition definition{};
    EventId cause_event_id{};
    std::uint32_t environmental_trigger_id{};
};

struct PressureBurstEffect {
    EntityId target_entity_id{};
    PartId target_part_id{};
    ninho::physics::BodyHandle physics_handle{};
    ninho::physics::Vec3 position_m{};
    ninho::physics::Vec3 normal_source_to_target{};
    ninho::physics::Vec3 impulse_n_s{};
    double energy_j{};
};

struct PlannedPressureBurst {
    PressureBurstRequest request;
    std::vector<PressureBurstEffect> effects;
};

struct PressureBurstPlan {
    std::vector<PlannedPressureBurst> requests;
};

struct PressureBurstPlanResult {
    PressureBurstPlan value;
    SessionStatus status;
    [[nodiscard]] bool ok() const noexcept { return status.ok(); }
};

class PressureBurstSystem {
public:
    [[nodiscard]] static PressureBurstPlanResult plan(
        const ninho::physics::PhysicsWorld&,
        std::span<const PressureBurstBody>,
        std::span<const PressureBurstRequest>);
    [[nodiscard]] static SessionStatus commit_impulses(
        ninho::physics::PhysicsWorld&, const PressureBurstPlan&);
};

}
