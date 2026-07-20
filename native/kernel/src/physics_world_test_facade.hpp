#pragma once

#if !defined(NINHO_ENABLE_TEST_FACADES)
#error "PhysicsWorldTestFacade is available only in test builds"
#endif

#include <cstddef>
#include <ninho/physics/physics_types.hpp>

namespace ninho::physics {

class PhysicsWorld;

namespace detail {

class PhysicsWorldTestFacade {
public:
    [[nodiscard]] static int worker_count(const PhysicsWorld&);
    [[nodiscard]] static Vec3 box3d_gravity(const PhysicsWorld&);
    static void fail_initial_commit_after(PhysicsWorld&, std::size_t applied_command_count);
};

}
}
