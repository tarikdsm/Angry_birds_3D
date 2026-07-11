#include "ninho/simulation/session.hpp"

static_assert(
    ninho::simulation::SessionState{}.phase == ninho::simulation::SessionPhase::Inspection);
static_assert(ninho::simulation::SessionState{}.outcome == ninho::simulation::Outcome::None);
