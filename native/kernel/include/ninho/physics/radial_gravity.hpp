#pragma once

#include <ninho/physics/gravity_field.hpp>
#include <ninho/physics/physics_types.hpp>

#include <cstdint>
#include <unordered_map>

namespace ninho::physics {

class RadialGravity {
public:
    explicit RadialGravity(RadialGravityConfig config)
        : gravity_(GravityFieldConfig{config})
    {
    }

    [[nodiscard]] Vec3 acceleration(Vec3 position) const noexcept;

private:
    GravityField gravity_;
};

class EjectionTracker {
public:
    bool update(BodyHandle body, float radius, float radial_speed, float dt, float planet_radius);
    void reset(BodyHandle body);

private:
    std::unordered_map<std::uint64_t, float> elapsed_;
};

}
