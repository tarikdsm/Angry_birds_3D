#pragma once

#include <ninho/physics/gravity_field.hpp>
#include <ninho/physics/physics_types.hpp>
#include <ninho/physics/world_bounds.hpp>

#include <cstdint>
#include <unordered_map>

namespace ninho::physics {

struct LegacyRadialGravityConfig {
    Vec3 center{};
    float radius{10.0f};
    float surface_acceleration{9.0f};
};

class RadialGravity {
public:
    explicit RadialGravity(RadialGravityConfig config)
        : gravity_(GravityFieldConfig{config})
    {
    }

    explicit RadialGravity(LegacyRadialGravityConfig config)
        : RadialGravity(RadialGravityConfig{
            .center_m = config.center,
            .reference_radius_m = config.radius,
            .reference_acceleration_m_s2 = config.surface_acceleration,
        })
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

namespace detail {

enum class RadialEjectionPolicy { Disabled, Enabled };
#if defined(NINHO_ENABLE_TEST_FACADES)
class PhysicsWorldTestFacade;
#endif

struct WorldExitEvents {
    bool bounds_exit{};
    bool radial_ejection{};
};

class WorldExitTracker {
public:
    WorldExitTracker(
        WorldBoundsConfig bounds,
        RadialEjectionPolicy radial_ejection_policy,
        RadialGravityConfig radial_gravity_config);

    [[nodiscard]] WorldExitEvents update(
        BodyHandle body,
        Vec3 position_m,
        Vec3 linear_velocity_m_s,
        float dt);
    void reset(BodyHandle body);

private:
    using EjectionEvaluator = bool (*)(
        WorldExitTracker&, BodyHandle, Vec3, Vec3, float);
    [[nodiscard]] static bool disabled_ejection(
        WorldExitTracker&, BodyHandle, Vec3, Vec3, float);
    [[nodiscard]] static bool radial_ejection(
        WorldExitTracker&, BodyHandle, Vec3, Vec3, float);
#if defined(NINHO_ENABLE_TEST_FACADES)
    friend class PhysicsWorldTestFacade;
#endif
    WorldBounds bounds_;
    EjectionTracker radial_ejection_;
    Vec3 radial_center_m_{};
    float radial_reference_radius_m_{};
    EjectionEvaluator ejection_evaluator_{};
};

}

}
