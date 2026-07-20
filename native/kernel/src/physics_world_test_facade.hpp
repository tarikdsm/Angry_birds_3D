#pragma once

#if !defined(NINHO_ENABLE_TEST_FACADES)
#error "PhysicsWorldTestFacade is available only in test builds"
#endif

#include <array>
#include <cstddef>
#include <ninho/physics/physics_types.hpp>
#include <vector>

namespace ninho::physics {

class PhysicsWorld;

namespace detail {

class PhysicsWorldTestFacade {
public:
    [[nodiscard]] static int worker_count(const PhysicsWorld&);
    [[nodiscard]] static Vec3 box3d_gravity(const PhysicsWorld&);
    [[nodiscard]] static int gravity_strategy(const PhysicsWorld&);
    [[nodiscard]] static int bounds_strategy(const PhysicsWorld&);
    [[nodiscard]] static int ejection_strategy(const PhysicsWorld&);
    [[nodiscard]] static std::array<float, 9> local_inertia(
        const PhysicsWorld&, BodyHandle);
    [[nodiscard]] static Vec3 local_center(const PhysicsWorld&, BodyHandle);
    [[nodiscard]] static std::vector<std::uint64_t> shape_keys(
        const PhysicsWorld&, BodyHandle);
    [[nodiscard]] static std::vector<float> shape_densities(
        const PhysicsWorld&, BodyHandle);
    [[nodiscard]] static std::vector<float> base_shape_densities(
        const PhysicsWorld&, BodyHandle);
    [[nodiscard]] static float base_mass(const PhysicsWorld&, BodyHandle);
    [[nodiscard]] static float mass_scale(const PhysicsWorld&, BodyHandle);
    static void set_base_density(
        PhysicsWorld&, BodyHandle, std::size_t shape_index, float density);
    static void fail_next_mass_scale_postcondition(PhysicsWorld&);
    static void fail_initial_commit_after(PhysicsWorld&, std::size_t applied_command_count);
};

}
}
