#pragma once

#include <ninho/physics/physics_types.hpp>

#include <variant>

namespace ninho::physics {

struct UniformGravityConfig {
    Vec3 acceleration_m_s2{};
};

struct RadialGravityConfig {
    Vec3 center_m{};
    float reference_radius_m{10.0f};
    float reference_acceleration_m_s2{9.0f};
};

using GravityFieldConfig = std::variant<UniformGravityConfig, RadialGravityConfig>;

class GravityField {
public:
    explicit GravityField(GravityFieldConfig config);

    [[nodiscard]] Vec3 acceleration_at(Vec3 position_m) const noexcept;
    [[nodiscard]] const GravityFieldConfig& config() const noexcept;

private:
    GravityFieldConfig config_;
};

}
