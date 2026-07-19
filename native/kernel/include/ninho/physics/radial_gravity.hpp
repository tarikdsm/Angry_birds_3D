#pragma once

#include <ninho/physics/physics_limits.hpp>
#include <ninho/physics/physics_types.hpp>

#include <cstdint>
#include <unordered_map>

namespace ninho::physics {

struct RadialGravityConfig {
    Vec3 center{};
    float radius{10.0f};
    float surface_acceleration{9.0f};
};

class RadialGravity {
public:
    explicit RadialGravity(RadialGravityConfig config)
        : config_(config)
    {
    }

    [[nodiscard]] Vec3 acceleration(Vec3 position) const noexcept;

private:
    RadialGravityConfig config_;
};

class EjectionTracker {
public:
    bool update(BodyHandle body, float radius, float radial_speed, float dt, float planet_radius);
    void reset(BodyHandle body);

private:
    std::unordered_map<std::uint64_t, float> elapsed_;
};

}
