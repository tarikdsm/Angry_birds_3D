#pragma once

#include "ninho/simulation/content.hpp"

#include <ninho/physics/physics_types.hpp>

#include <optional>
#include <span>
#include <vector>

namespace ninho::simulation::detail {

struct CrushBody {
    EntityId entity_id{};
    PartId part_id{};
    double mass_kg{};
    double gravity_m_s2{};
    bool neutralized{};
    ninho::physics::Vec3 position_m{};
    ninho::physics::Vec3 normal{};
};

struct CrushContactLoad {
    EntityId entity_id{};
    PartId part_id{};
    double total_normal_impulse_n_s{};
    EntityId cause_entity_id{};
};

struct CrushRuntime {
    EntityId entity_id{};
    PartId part_id{};
    std::uint32_t streak{};
    double excess_delta_v_m_s{};
    EventId cause_event_id{};
    EntityId load_cause_entity_id{};

    bool operator==(const CrushRuntime&) const = default;
};

struct CrushDamagePlan {
    EntityId target_entity_id{};
    PartId target_part_id{};
    ninho::physics::Vec3 position_m{};
    ninho::physics::Vec3 normal{};
    double external_energy_j{};
    double predicted_damage{};
    EntityId cause_entity_id{};
};

class CrushDamageSystem {
public:
    [[nodiscard]] std::vector<CrushDamagePlan> update(TickIndex, double dt,
        std::span<const CrushBody>, std::span<const CrushContactLoad>);
    [[nodiscard]] bool record_cause(EntityId, PartId, EventId) noexcept;
    [[nodiscard]] std::optional<CrushRuntime> state(EntityId, PartId) const;
    [[nodiscard]] std::span<const CrushRuntime> states() const noexcept { return states_; }
#if defined(NINHO_ENABLE_TEST_FACADES)
    [[nodiscard]] bool set_load_cause_for_testing(
        EntityId, PartId, EntityId) noexcept;
#endif

private:
    std::vector<CrushRuntime> states_;
};

}
