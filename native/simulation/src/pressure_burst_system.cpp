#include "pressure_burst_system.hpp"

#include <algorithm>
#include <cmath>
#include <ranges>
#include <tuple>

namespace ninho::simulation::detail {
namespace {

SessionStatus invalid_burst(const char* message)
{
    return {{ContentErrorCode::InvalidInvariant, "/pressure_burst", message}};
}

bool valid_definition(const PressureBurstDefinition& value) noexcept
{
    return std::isfinite(value.radius_m) && value.radius_m > 0.0
        && std::isfinite(value.impulse_n_s) && value.impulse_n_s > 0.0
        && std::isfinite(value.energy_j) && value.energy_j > 0.0
        && value.max_bodies >= 1U && value.max_bodies <= 32U;
}

}

PressureBurstPlanResult PressureBurstSystem::plan(
    const ninho::physics::PhysicsWorld& physics,
    std::span<const PressureBurstBody> bodies,
    std::span<const PressureBurstRequest> requests)
{
    PressureBurstPlanResult result;
    try {
        std::vector<const PressureBurstRequest*> ordered_requests;
        ordered_requests.reserve(requests.size());
        for (const PressureBurstRequest& request : requests) {
            if (!ninho::physics::is_finite(request.origin_m)
                || !valid_definition(request.definition)) {
                result.status = invalid_burst("burst request is not finite and bounded");
                return result;
            }
            ordered_requests.push_back(&request);
        }
        std::ranges::sort(ordered_requests, [](const auto* lhs, const auto* rhs) {
            return std::tuple{lhs->origin_kind, lhs->origin_identity,
                       lhs->source_entity_id, lhs->source_part_id,
                       lhs->environmental_trigger_id}
                < std::tuple{rhs->origin_kind, rhs->origin_identity,
                       rhs->source_entity_id, rhs->source_part_id,
                       rhs->environmental_trigger_id};
        });
        result.value.requests.reserve(ordered_requests.size());
        for (const PressureBurstRequest* request : ordered_requests) {
            PlannedPressureBurst planned{.request = *request};
            const auto overlaps = physics.overlap_sphere(request->origin_m,
                static_cast<float>(request->definition.radius_m));
            std::vector<const PressureBurstBody*> candidates;
            candidates.reserve(overlaps.size());
            for (const ninho::physics::QueryHit& hit : overlaps) {
                const auto found = std::ranges::find(
                    bodies, hit.body, &PressureBurstBody::physics_handle);
                if (found == bodies.end()
                    || found->entity_id == request->source_entity_id) {
                    continue;
                }
                candidates.push_back(&*found);
            }
            std::ranges::sort(candidates, [](const auto* lhs, const auto* rhs) {
                return std::pair{lhs->entity_id, lhs->part_id}
                    < std::pair{rhs->entity_id, rhs->part_id};
            });
            candidates.erase(std::unique(candidates.begin(), candidates.end(),
                [](const auto* lhs, const auto* rhs) {
                    return lhs->entity_id == rhs->entity_id
                        && lhs->part_id == rhs->part_id;
                }), candidates.end());
            if (candidates.size() > request->definition.max_bodies) {
                candidates.resize(request->definition.max_bodies);
            }

            std::vector<ninho::physics::BodyHandle> ignored_source_handles;
            for (const PressureBurstBody& body : bodies) {
                if (body.entity_id == request->source_entity_id) {
                    ignored_source_handles.push_back(body.physics_handle);
                }
            }
            std::ranges::sort(ignored_source_handles);
            planned.effects.reserve(candidates.size());
            for (const PressureBurstBody* body : candidates) {
                const auto state = physics.state(body->physics_handle);
                if (!state) {
                    result.status = invalid_burst("burst target body is not live");
                    return result;
                }
                const ninho::physics::Vec3 offset =
                    state->world_center_of_mass - request->origin_m;
                const double distance = ninho::physics::length(offset);
                const double t = std::clamp(
                    distance / request->definition.radius_m, 0.0, 1.0);
                const double falloff = 1.0 - (3.0 * t * t - 2.0 * t * t * t);
                if (falloff <= 0.0) {
                    continue;
                }
                if (request->definition.line_of_sight) {
                    auto ignored = ignored_source_handles;
                    ignored.push_back(body->physics_handle);
                    std::ranges::sort(ignored);
                    if (physics.cast_segment(request->origin_m,
                            state->world_center_of_mass, ignored)) {
                        continue;
                    }
                }
                const double physical_mass = state->mass > 0.0f
                    ? static_cast<double>(state->mass) : body->structural_mass_kg;
                if (!std::isfinite(physical_mass) || physical_mass <= 0.0) {
                    continue;
                }
                const double central_impulse = std::min(
                    request->definition.impulse_n_s, physical_mass * 12.0);
                const double central_energy = std::min(
                    request->definition.energy_j, physical_mass * 90.0);
                const auto radial = distance > 1.0e-8
                    ? offset / static_cast<float>(distance)
                    : ninho::physics::Vec3{1.0f, 0.0f, 0.0f};
                const auto impulse = body->body_type == BodyType::Dynamic
                    ? radial * static_cast<float>(central_impulse * falloff)
                    : ninho::physics::Vec3{};
                const double energy = central_energy * falloff;
                if (!ninho::physics::is_finite(impulse) || !std::isfinite(energy)) {
                    result.status = invalid_burst("burst effect overflowed");
                    return result;
                }
                planned.effects.push_back({body->entity_id, body->part_id,
                    body->physics_handle, state->world_center_of_mass, radial,
                    impulse, energy});
            }
            result.value.requests.push_back(std::move(planned));
        }
        return result;
    } catch (const std::exception& error) {
        result.status = {{ContentErrorCode::InternalError,
            "/pressure_burst", error.what()}};
        return result;
    }
}

SessionStatus PressureBurstSystem::commit_impulses(
    ninho::physics::PhysicsWorld& physics, const PressureBurstPlan& plan)
{
    std::vector<ninho::physics::CentralImpulse> impulses;
    for (const PlannedPressureBurst& request : plan.requests) {
        for (const PressureBurstEffect& effect : request.effects) {
            if (effect.impulse_n_s != ninho::physics::Vec3{}) {
                impulses.push_back({effect.physics_handle, effect.impulse_n_s});
            }
        }
    }
    // The handle alone is not a total order and the reduction below is a float
    // accumulation: with three impulses on the same body in the same tick, an
    // unstable sort would let the association of the sum -- and therefore its
    // last bit -- depend on the introsort of the toolchain. Stable sorting
    // keeps the deterministic plan order as the tie-break.
    std::ranges::stable_sort(impulses, {}, &ninho::physics::CentralImpulse::body);
    std::vector<ninho::physics::CentralImpulse> reduced;
    reduced.reserve(impulses.size());
    for (const auto& impulse : impulses) {
        if (reduced.empty() || reduced.back().body != impulse.body) {
            reduced.push_back(impulse);
            continue;
        }
        reduced.back().impulse = reduced.back().impulse + impulse.impulse;
        if (!ninho::physics::is_finite(reduced.back().impulse)) {
            return {{ContentErrorCode::InvalidInvariant,
                "/pressure_burst", "combined burst impulse overflowed"}};
        }
    }
    const auto status = physics.apply_central_impulses(reduced);
    if (!status.ok()) {
        return {{ContentErrorCode::InternalError,
            "/pressure_burst", status.message}};
    }
    return {};
}

}
