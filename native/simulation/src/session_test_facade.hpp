#pragma once

#if !defined(NINHO_ENABLE_TEST_FACADES)
#error "SessionTestFacade is available only in test builds"
#endif

#include <cstdint>
#include <string>

namespace ninho::simulation {

class SimulationSession;

namespace detail {

class SessionTestFacade {
public:
    static void fail_next_canonical_refresh(SimulationSession&, std::string message);
    static void override_next_snapshot_mass(SimulationSession&, double mass_kg);
    static std::int64_t quantize_canonical(double value);
};

}
}
