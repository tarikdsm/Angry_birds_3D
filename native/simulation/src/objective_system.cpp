#include "session_internal.hpp"

#include <algorithm>
#include <ranges>

namespace ninho::simulation {

void SimulationSession::Impl::evaluate_objectives_after_step()
{
    if (objective_complete || bundle.level.objectives.empty()) {
        return;
    }
    objective_complete = std::ranges::all_of(
        bundle.level.objectives, [&](const ObjectiveDefinition& objective) {
            if (objective.kind != ObjectiveKind::NeutralizeEntity) {
                return false;
            }
            bool found_target = false;
            for (const BodyRecord& body : body_records) {
                if (body.entity_id != objective.target_entity_id) {
                    continue;
                }
                found_target = true;
                if (!body.neutralized) {
                    return false;
                }
            }
            return found_target;
        });
}

}
