#pragma once

#if !defined(NINHO_ENABLE_TEST_FACADES)
#error "ScenarioTestFacade is available only in test builds"
#endif

#include <cstddef>

namespace ninho::physics::detail {

class ScenarioTestFacade {
public:
    static void reset_projectile_simulation_count() noexcept;
    [[nodiscard]] static std::size_t projectile_simulation_count() noexcept;
};

}
