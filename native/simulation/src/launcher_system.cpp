#include "launcher_system.hpp"

#include <algorithm>
#include <cmath>
#include <stdexcept>
#include <type_traits>
#include <utility>

namespace ninho::simulation::detail {
namespace {

constexpr double basis_quantum = 1.0e-4;
constexpr double pull_quantum = 1.0e-3;
constexpr double speed_quantum = 1.0e-2;

[[nodiscard]] ContentError error(
    ContentErrorCode code, std::string pointer, std::string message)
{
    return {code, std::move(pointer), std::move(message)};
}

[[nodiscard]] double quantize(double value, double quantum) noexcept
{
    return std::round(value / quantum) * quantum;
}

[[nodiscard]] double quantize_down(double value, double quantum) noexcept
{
    double units = std::floor(value / quantum);
    double result = units * quantum;
    if (result > value) {
        result = (--units) * quantum;
    }
    return std::max(0.0, result);
}

[[nodiscard]] ninho::physics::Vec3 quantized_vector(
    ninho::physics::Vec3 value) noexcept
{
    return {
        static_cast<float>(quantize(value.x, basis_quantum)),
        static_cast<float>(quantize(value.y, basis_quantum)),
        static_cast<float>(quantize(value.z, basis_quantum)),
    };
}

[[nodiscard]] ninho::physics::Vec3 quantized_unit(
    ninho::physics::Vec3 value) noexcept
{
    return ninho::physics::normalized_or_zero(quantized_vector(value));
}

[[nodiscard]] ninho::physics::Vec3 vector_from(
    const std::array<double, 3>& value) noexcept
{
    return {static_cast<float>(value[0]), static_cast<float>(value[1]),
        static_cast<float>(value[2])};
}

[[nodiscard]] ninho::physics::Vec3 quantized_position(
    const std::array<double, 3>& value) noexcept
{
    return {
        static_cast<float>(quantize(value[0], pull_quantum)),
        static_cast<float>(quantize(value[1], pull_quantum)),
        static_cast<float>(quantize(value[2], pull_quantum)),
    };
}

void clamp_quantized_pull(double& horizontal, double& vertical, double maximum) noexcept
{
    horizontal = quantize(horizontal, pull_quantum);
    vertical = quantize(vertical, pull_quantum);
    double extension = std::hypot(horizontal, vertical);
    if (extension <= maximum) {
        return;
    }
    horizontal = quantize(horizontal * maximum / extension, pull_quantum);
    vertical = quantize(vertical * maximum / extension, pull_quantum);
    extension = std::hypot(horizontal, vertical);
    while (extension > maximum) {
        double& component = std::abs(horizontal) >= std::abs(vertical)
            ? horizontal : vertical;
        component -= std::copysign(pull_quantum, component);
        extension = std::hypot(horizontal, vertical);
    }
}

}

LauncherSystem::LauncherSystem(WorldDefinition world, SlingshotDefinition definition)
    : world_(std::move(world))
    , definition_(std::move(definition))
{
}

ContentResult<LauncherState> LauncherSystem::begin_grab(
    ninho::physics::Vec3 camera_right, LauncherProjectile projectile)
{
    if (grabbed_) {
        return {{}, error(ContentErrorCode::InvalidInvariant, "/launcher",
            "an active launcher frame cannot be relocked")};
    }
    if (!ninho::physics::is_finite(camera_right)) {
        return {{}, error(ContentErrorCode::InvalidNumber, "/launcher/camera_right",
            "camera right must be finite")};
    }
    if (!std::isfinite(projectile.mass_kg) || projectile.mass_kg <= 0.0
        || !std::isfinite(projectile.speed_cap_m_s) || projectile.speed_cap_m_s <= 0.0) {
        return {{}, error(ContentErrorCode::InvalidNumber, "/launcher/projectile",
            "projectile mass and speed cap must be finite and positive")};
    }

    const auto rest = quantized_position(definition_.rest_position_m);
    ninho::physics::Vec3 up = std::visit([&](const auto& world) {
        using World = std::decay_t<decltype(world)>;
        if constexpr (std::is_same_v<World, UniformWorldDefinition>) {
            return ninho::physics::Vec3{0.0F, 1.0F, 0.0F};
        } else {
            return rest - vector_from(world.center_m);
        }
    }, world_);
    up = quantized_unit(up);
    const auto quantized_camera_right = quantized_vector(camera_right);
    const auto accepted_camera_right =
        ninho::physics::normalized_or_zero(quantized_camera_right);
    if (ninho::physics::length(up) < 0.999F
        || ninho::physics::length(accepted_camera_right) < 0.999F) {
        return {{}, error(ContentErrorCode::InvalidInvariant, "/launcher/camera_right",
            "launcher basis must be non-zero")};
    }
    const auto projected = quantized_camera_right
        - up * ninho::physics::dot(quantized_camera_right, up);
    const auto horizontal = quantized_unit(projected);
    if (ninho::physics::length(horizontal) < 0.999F) {
        return {{}, error(ContentErrorCode::InvalidInvariant, "/launcher/camera_right",
            "camera right must not be parallel to launcher up")};
    }
    const auto plane_normal = quantized_unit(ninho::physics::cross(horizontal, up));
    if (ninho::physics::length(plane_normal) < 0.999F) {
        return {{}, error(ContentErrorCode::InvalidInvariant, "/launcher/plane_normal",
            "launcher plane must be representable")};
    }

    projectile_ = projectile;
    state_ = LauncherState{
        .rest_position_m = rest,
        .camera_right = accepted_camera_right,
        .up = up,
        .horizontal = horizontal,
        .plane_normal = plane_normal,
        .deadzone_m = definition_.minimum_extension_m,
        .maximum_extension_m = definition_.maximum_extension_m,
    };
    grabbed_ = true;
    return {*state_, {}};
}

ContentResult<LauncherState> LauncherSystem::set_pull(
    double horizontal_m, double vertical_m)
{
    if (!grabbed_ || !state_) {
        return {{}, error(ContentErrorCode::InvalidInvariant, "/launcher/pull",
            "pull requires an active grab")};
    }
    if (!std::isfinite(horizontal_m) || !std::isfinite(vertical_m)) {
        return {{}, error(ContentErrorCode::InvalidNumber, "/launcher/pull",
            "pull components must be finite")};
    }
    clamp_quantized_pull(horizontal_m, vertical_m, definition_.maximum_extension_m);
    state_->pull_horizontal_m = horizontal_m;
    state_->pull_vertical_m = vertical_m;
    update_solved_state();
    return {*state_, {}};
}

void LauncherSystem::update_solved_state() noexcept
{
    LauncherState& state = *state_;
    state.extension_m = std::hypot(state.pull_horizontal_m, state.pull_vertical_m);
    state.spring_energy_j = 0.5 * definition_.spring_constant_n_m
        * state.extension_m * state.extension_m;
    state.launch_energy_j = definition_.energy_efficiency * state.spring_energy_j;
    const auto displacement = state.horizontal
            * static_cast<float>(state.pull_horizontal_m)
        + state.up * static_cast<float>(state.pull_vertical_m);
    state.launch_direction = quantized_unit(displacement * -1.0F);
    const double uncapped_speed = state.extension_m * std::sqrt(
        definition_.energy_efficiency * definition_.spring_constant_n_m
        / projectile_.mass_kg);
    const double effective_cap = std::min(
        projectile_.speed_cap_m_s, definition_.speed_ceiling_m_s);
    state.predicted_speed_m_s = std::min(
        quantize_down(effective_cap, speed_quantum),
        quantize(uncapped_speed, speed_quantum));
}

ContentResult<LauncherSolution> LauncherSystem::solution() const
{
    if (!state_) {
        return {{}, error(ContentErrorCode::InvalidInvariant, "/launcher",
            "launcher solution requires an accepted frame")};
    }
    return {{
        .launchable = state_->extension_m >= state_->deadzone_m,
        .origin_m = state_->rest_position_m,
        .direction = state_->launch_direction,
        .speed_m_s = state_->predicted_speed_m_s,
    }, {}};
}

void LauncherSystem::cancel_grab() noexcept
{
    state_.reset();
    grabbed_ = false;
}

void LauncherSystem::complete_release() noexcept
{
    state_.reset();
    grabbed_ = false;
}

const std::optional<LauncherState>& LauncherSystem::state() const noexcept
{
    return state_;
}

}
