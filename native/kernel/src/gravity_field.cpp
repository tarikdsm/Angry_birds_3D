#include <ninho/physics/gravity_field.hpp>

#include <ninho/physics/physics_limits.hpp>

#include <algorithm>
#include <cmath>
#include <limits>
#include <stdexcept>
#include <utility>
#include <variant>

namespace ninho::physics {
namespace {

constexpr float minimum_direction_radius = 0.001f;
constexpr float inner_radius_fraction = 0.6f;

template<class... Visitors>
struct Overloaded : Visitors... {
    using Visitors::operator()...;
};

template<class... Visitors>
Overloaded(Visitors...) -> Overloaded<Visitors...>;

[[nodiscard]] bool acceleration_within_ceiling(Vec3 acceleration) noexcept
{
    if (!is_finite(acceleration)) {
        return false;
    }
    const double x = acceleration.x;
    const double y = acceleration.y;
    const double z = acceleration.z;
    const double ceiling = maximum_radial_acceleration;
    return x * x + y * y + z * z <= ceiling * ceiling;
}

[[nodiscard]] GravityFieldConfig validated_config(GravityFieldConfig config)
{
    std::visit(
        Overloaded{
            [](const UniformGravityConfig& uniform) {
                if (!acceleration_within_ceiling(uniform.acceleration_m_s2)) {
                    throw std::invalid_argument(
                        "uniform gravity acceleration must be finite and within the acceleration ceiling");
                }
            },
            [](const RadialGravityConfig& radial) {
                if (!is_finite(radial.center_m)) {
                    throw std::invalid_argument("radial gravity center must be finite");
                }
                if (!std::isfinite(radial.reference_radius_m)
                    || radial.reference_radius_m <= 0.0f) {
                    throw std::invalid_argument(
                        "radial gravity reference radius must be finite and positive");
                }
                if (!std::isfinite(radial.reference_acceleration_m_s2)
                    || radial.reference_acceleration_m_s2 < 0.0f
                    || radial.reference_acceleration_m_s2 > maximum_radial_acceleration) {
                    throw std::invalid_argument(
                        "radial gravity reference acceleration must be finite, non-negative, and within the acceleration ceiling");
                }
                const double gravity_scale =
                    static_cast<double>(radial.reference_acceleration_m_s2)
                    * static_cast<double>(radial.reference_radius_m)
                    * static_cast<double>(radial.reference_radius_m);
                if (gravity_scale > std::numeric_limits<float>::max()) {
                    throw std::invalid_argument(
                        "radial gravity configuration exceeds finite float range");
                }
            },
        },
        config);
    return config;
}

[[nodiscard]] Vec3 robust_radial_acceleration(
    const RadialGravityConfig& config,
    Vec3 position) noexcept
{
    if (!is_finite(position)) {
        return {};
    }

    const double x = static_cast<double>(config.center_m.x) - position.x;
    const double y = static_cast<double>(config.center_m.y) - position.y;
    const double z = static_cast<double>(config.center_m.z) - position.z;
    const double distance = std::sqrt(x * x + y * y + z * z);
    if (!std::isfinite(distance) || distance < minimum_direction_radius) {
        return {};
    }

    const double inner_radius = inner_radius_fraction * config.reference_radius_m;
    const double denominator = std::max(distance * distance, inner_radius * inner_radius);
    const double magnitude = std::min(
        static_cast<double>(config.reference_acceleration_m_s2)
            * config.reference_radius_m * config.reference_radius_m / denominator,
        static_cast<double>(maximum_radial_acceleration));
    const double scale = magnitude / distance;
    return {
        static_cast<float>(x * scale),
        static_cast<float>(y * scale),
        static_cast<float>(z * scale),
    };
}

[[nodiscard]] Vec3 radial_acceleration(
    const RadialGravityConfig& config,
    Vec3 position) noexcept
{
    if (config.reference_acceleration_m_s2 == 0.0f || !is_finite(position)) {
        return {};
    }

    const Vec3 toward_center = config.center_m - position;
    const float distance = length(toward_center);
    if (!is_finite(toward_center) || !std::isfinite(distance)) {
        return robust_radial_acceleration(config, position);
    }
    if (distance < minimum_direction_radius) {
        return {};
    }

    const float inner_radius = inner_radius_fraction * config.reference_radius_m;
    const float distance_squared = distance * distance;
    const float denominator = std::max(distance_squared, inner_radius * inner_radius);
    const float magnitude = std::min(
        config.reference_acceleration_m_s2 * config.reference_radius_m
            * config.reference_radius_m / denominator,
        maximum_radial_acceleration);
    return toward_center * (magnitude / distance);
}

}

GravityField::GravityField(GravityFieldConfig config)
    : config_(validated_config(std::move(config)))
{
    std::visit(
        [this](const auto& selected) {
            using Config = std::decay_t<decltype(selected)>;
            if constexpr (std::is_same_v<Config, UniformGravityConfig>) {
                uniform_acceleration_ = selected.acceleration_m_s2;
                evaluator_ = evaluate_uniform;
            } else {
                radial_config_ = selected;
                evaluator_ = evaluate_radial;
            }
        },
        config_);
}

Vec3 GravityField::acceleration_at(Vec3 position_m) const noexcept
{
    return evaluator_(*this, position_m);
}

Vec3 GravityField::evaluate_uniform(
    const GravityField& field, Vec3) noexcept
{
    return field.uniform_acceleration_;
}

Vec3 GravityField::evaluate_radial(
    const GravityField& field, Vec3 position_m) noexcept
{
    return radial_acceleration(field.radial_config_, position_m);
}

const GravityFieldConfig& GravityField::config() const noexcept
{
    return config_;
}

}
