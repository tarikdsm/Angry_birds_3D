#include "crush_damage_system.hpp"

#include <algorithm>
#include <cmath>
#include <ranges>

namespace ninho::simulation::detail {
namespace {

double quantize_ratio(double value) noexcept
{
    return std::round(value * 100000.0) / 100000.0;
}

}

std::vector<CrushDamagePlan> CrushDamageSystem::update(TickIndex, double dt,
    std::span<const CrushBody> bodies, std::span<const CrushContactLoad> contacts)
{
    std::vector<CrushBody> ordered{bodies.begin(), bodies.end()};
    std::ranges::sort(ordered, [](const auto& lhs, const auto& rhs) {
        return std::pair{lhs.entity_id, lhs.part_id}
            < std::pair{rhs.entity_id, rhs.part_id};
    });
    std::vector<CrushDamagePlan> plans;
    for (const auto& body : ordered) {
        auto runtime = std::ranges::find_if(states_, [&](const auto& value) {
            return value.entity_id == body.entity_id && value.part_id == body.part_id;
        });
        if (runtime == states_.end()) {
            states_.push_back({body.entity_id, body.part_id});
            std::ranges::sort(states_, [](const auto& lhs, const auto& rhs) {
                return std::pair{lhs.entity_id, lhs.part_id}
                    < std::pair{rhs.entity_id, rhs.part_id};
            });
            runtime = std::ranges::find_if(states_, [&](const auto& value) {
                return value.entity_id == body.entity_id && value.part_id == body.part_id;
            });
        }
        double impulse = 0.0;
        for (const auto& contact : contacts) {
            if (contact.entity_id == body.entity_id && contact.part_id == body.part_id
                && std::isfinite(contact.total_normal_impulse_n_s)
                && contact.total_normal_impulse_n_s > 0.0) {
                impulse += contact.total_normal_impulse_n_s;
            }
        }
        const double weight_impulse = body.mass_kg * body.gravity_m_s2 * dt;
        if (body.neutralized || !std::isfinite(dt) || dt <= 0.0
            || !std::isfinite(weight_impulse) || weight_impulse <= 0.0) {
            runtime->streak = 0U;
            runtime->excess_delta_v_m_s = 0.0;
            continue;
        }
        const double ratio = quantize_ratio(impulse / weight_impulse);
        if (!(ratio > 4.0)) {
            runtime->streak = 0U;
            runtime->excess_delta_v_m_s = 0.0;
            continue;
        }
        ++runtime->streak;
        runtime->excess_delta_v_m_s +=
            (ratio - 4.0) * body.gravity_m_s2 * dt;
        runtime->excess_delta_v_m_s =
            std::round(runtime->excess_delta_v_m_s * 100000.0) / 100000.0;
        if (runtime->streak == 21U) {
            const double specific_energy = 18.0
                + 0.5 * runtime->excess_delta_v_m_s * runtime->excess_delta_v_m_s;
            plans.push_back({body.entity_id, body.part_id, body.position_m, body.normal,
                body.mass_kg * specific_energy,
                std::clamp((specific_energy - 18.0) * 0.9, 0.0, 70.0)});
            runtime->streak = 0U;
            runtime->excess_delta_v_m_s = 0.0;
        }
    }
    return plans;
}

std::optional<CrushRuntime> CrushDamageSystem::state(EntityId entity, PartId part) const
{
    const auto found = std::ranges::find_if(states_, [&](const auto& value) {
        return value.entity_id == entity && value.part_id == part;
    });
    return found == states_.end() ? std::nullopt : std::optional{*found};
}

bool CrushDamageSystem::record_cause(
    EntityId entity, PartId part, EventId cause) noexcept
{
    const auto found = std::ranges::find_if(states_, [&](const auto& value) {
        return value.entity_id == entity && value.part_id == part;
    });
    if (found == states_.end() || cause == EventId{}) {
        return false;
    }
    found->cause_event_id = cause;
    return true;
}

}
