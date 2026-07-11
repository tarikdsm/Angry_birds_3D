#pragma once

#include <ninho/physics/physics_world.hpp>

#include <godot_cpp/variant/basis.hpp>
#include <godot_cpp/variant/quaternion.hpp>
#include <godot_cpp/variant/transform3d.hpp>
#include <godot_cpp/variant/vector3.hpp>

#include <algorithm>
#include <bit>
#include <cmath>
#include <cstdint>
#include <optional>

namespace ninho::extension::detail {

[[nodiscard]] std::optional<float> checked_finite_float(double value) noexcept;
[[nodiscard]] std::optional<float> checked_positive_float(double value) noexcept;

[[nodiscard]] inline std::optional<physics::Vec3> to_kernel_checked(
    const godot::Vector3& value) noexcept
{
    const auto x = checked_finite_float(value.x);
    const auto y = checked_finite_float(value.y);
    const auto z = checked_finite_float(value.z);
    if (!x || !y || !z) {
        return std::nullopt;
    }
    return physics::Vec3{*x, *y, *z};
}

[[nodiscard]] inline godot::Vector3 to_godot(physics::Vec3 value) noexcept
{
    return {value.x, value.y, value.z};
}

[[nodiscard]] inline std::optional<physics::Quat> to_kernel_checked(
    const godot::Quaternion& value) noexcept
{
    const auto x = checked_finite_float(value.x);
    const auto y = checked_finite_float(value.y);
    const auto z = checked_finite_float(value.z);
    const auto w = checked_finite_float(value.w);
    if (!x || !y || !z || !w) {
        return std::nullopt;
    }
    return physics::Quat{*x, *y, *z, *w};
}

[[nodiscard]] inline godot::Quaternion to_godot(physics::Quat value) noexcept
{
    return {value.x, value.y, value.z, value.w};
}

[[nodiscard]] inline std::optional<physics::Transform> to_kernel_checked(
    const godot::Transform3D& value) noexcept
{
    constexpr double rigid_tolerance = 1.0e-4;
    const auto position = to_kernel_checked(value.origin);
    if (!position) {
        return std::nullopt;
    }

    // Kernel transforms are rigid. Reject scale, shear, reflection and
    // degeneracy explicitly instead of silently discarding them as a quaternion.
    const godot::Vector3 columns[] = {
        value.basis.get_column(0),
        value.basis.get_column(1),
        value.basis.get_column(2),
    };
    const auto finite_column = [](const godot::Vector3& column) noexcept {
        return std::isfinite(column.x) && std::isfinite(column.y) && std::isfinite(column.z);
    };
    for (const godot::Vector3& column : columns) {
        if (!finite_column(column)) {
            return std::nullopt;
        }
        const double length = std::sqrt(
            static_cast<double>(column.x) * column.x
            + static_cast<double>(column.y) * column.y
            + static_cast<double>(column.z) * column.z);
        if (!std::isfinite(length) || std::abs(length - 1.0) > rigid_tolerance) {
            return std::nullopt;
        }
    }
    const auto dot = [](const godot::Vector3& lhs, const godot::Vector3& rhs) noexcept {
        return static_cast<double>(lhs.x) * rhs.x + static_cast<double>(lhs.y) * rhs.y
            + static_cast<double>(lhs.z) * rhs.z;
    };
    if (std::abs(dot(columns[0], columns[1])) > rigid_tolerance
        || std::abs(dot(columns[0], columns[2])) > rigid_tolerance
        || std::abs(dot(columns[1], columns[2])) > rigid_tolerance) {
        return std::nullopt;
    }
    const double determinant =
        static_cast<double>(columns[0].x)
            * (static_cast<double>(columns[1].y) * columns[2].z
                - static_cast<double>(columns[1].z) * columns[2].y)
        - static_cast<double>(columns[1].x)
            * (static_cast<double>(columns[0].y) * columns[2].z
                - static_cast<double>(columns[0].z) * columns[2].y)
        + static_cast<double>(columns[2].x)
            * (static_cast<double>(columns[0].y) * columns[1].z
                - static_cast<double>(columns[0].z) * columns[1].y);
    if (!std::isfinite(determinant) || determinant <= 0.0
        || std::abs(determinant - 1.0) > rigid_tolerance) {
        return std::nullopt;
    }

    const godot::Quaternion quaternion = value.basis.get_quaternion();
    if (!std::isfinite(quaternion.x) || !std::isfinite(quaternion.y)
        || !std::isfinite(quaternion.z) || !std::isfinite(quaternion.w)) {
        return std::nullopt;
    }
    const double norm = std::sqrt(
        static_cast<double>(quaternion.x) * quaternion.x
        + static_cast<double>(quaternion.y) * quaternion.y
        + static_cast<double>(quaternion.z) * quaternion.z
        + static_cast<double>(quaternion.w) * quaternion.w);
    if (!std::isfinite(norm) || norm <= 0.0 || std::abs(norm - 1.0) > rigid_tolerance) {
        return std::nullopt;
    }
    const auto rotation_x = checked_finite_float(quaternion.x / norm);
    const auto rotation_y = checked_finite_float(quaternion.y / norm);
    const auto rotation_z = checked_finite_float(quaternion.z / norm);
    const auto rotation_w = checked_finite_float(quaternion.w / norm);
    if (!rotation_x || !rotation_y || !rotation_z || !rotation_w) {
        return std::nullopt;
    }
    return physics::Transform{
        *position,
        {*rotation_x, *rotation_y, *rotation_z, *rotation_w},
    };
}

[[nodiscard]] inline godot::Transform3D to_godot(const physics::Transform& value) noexcept
{
    return {godot::Basis{to_godot(value.rotation)}, to_godot(value.position)};
}

[[nodiscard]] inline bool is_finite(const godot::Vector3& value) noexcept
{
    return std::isfinite(value.x) && std::isfinite(value.y) && std::isfinite(value.z);
}

[[nodiscard]] inline bool is_finite(const godot::Quaternion& value) noexcept
{
    return std::isfinite(value.x) && std::isfinite(value.y) && std::isfinite(value.z)
        && std::isfinite(value.w);
}

[[nodiscard]] inline bool is_finite(const godot::Transform3D& value) noexcept
{
    return to_kernel_checked(value).has_value();
}

[[nodiscard]] inline std::int64_t pack_handle(physics::BodyHandle handle) noexcept
{
    if (!handle.valid()) {
        return 0;
    }
    const std::uint64_t packed =
        (static_cast<std::uint64_t>(handle.generation) << 32U) | handle.index;
    return std::bit_cast<std::int64_t>(packed);
}

[[nodiscard]] inline physics::BodyHandle unpack_handle(std::int64_t packed) noexcept
{
    if (packed == 0) {
        return {};
    }
    const std::uint64_t bits = std::bit_cast<std::uint64_t>(packed);
    const physics::BodyHandle handle{
        static_cast<std::uint32_t>(bits & 0xFFFFFFFFULL),
        static_cast<std::uint32_t>(bits >> 32U),
    };
    return handle.valid() ? handle : physics::BodyHandle{};
}

[[nodiscard]] inline physics::WorldConfig make_world_config(
    float radius, float surface_gravity) noexcept
{
    physics::WorldConfig config;
    config.planet_radius = radius;
    config.surface_gravity = surface_gravity;
    return config;
}

[[nodiscard]] inline physics::BodyDesc make_planet_desc(float radius)
{
    return physics::BodyDesc::static_sphere(radius, {});
}

[[nodiscard]] inline physics::BodyDesc make_box_desc(
    physics::Vec3 full_size, const physics::Transform& transform, float density)
{
    return physics::BodyDesc::dynamic_box(full_size * 0.5F, transform, density);
}

[[nodiscard]] inline physics::BodyDesc make_projectile_desc(
    float radius,
    const physics::Transform& transform,
    physics::Vec3 velocity)
{
    physics::BodyDesc desc = physics::BodyDesc::dynamic_sphere(radius, transform, 1.0F);
    desc.linear_velocity = velocity;
    desc.bullet = true;
    return desc;
}

[[nodiscard]] inline physics::Status apply_impulse_checked(
    physics::PhysicsWorld& world,
    std::int64_t packed_handle,
    const godot::Vector3& impulse,
    const godot::Vector3& world_point)
{
    const physics::BodyHandle handle = unpack_handle(packed_handle);
    if (!handle.valid()) {
        return {physics::StatusCode::InvalidHandle, "packed body handle is invalid"};
    }
    if (!is_finite(impulse) || !is_finite(world_point)) {
        return {
            physics::StatusCode::InvalidArgument,
            "impulse and world point must be finite",
        };
    }
    const auto kernel_impulse = to_kernel_checked(impulse);
    const auto kernel_point = to_kernel_checked(world_point);
    if (!kernel_impulse || !kernel_point) {
        return {
            physics::StatusCode::InvalidArgument,
            "impulse and world point cannot be represented by the kernel",
        };
    }
    return world.apply_impulse(handle, *kernel_impulse, *kernel_point);
}

struct TickSchedule {
    bool ok{};
    int tick_count{};
};

class FixedStepAccumulator {
public:
    [[nodiscard]] TickSchedule schedule(double delta, double time_step) noexcept
    {
        if (!std::isfinite(delta) || delta < 0.0 || !std::isfinite(time_step)
            || time_step <= 0.0) {
            reset();
            return {};
        }

        accumulator_ = std::min(accumulator_ + delta, max_accumulator);
        const double tolerance = time_step * 1.0e-9;
        const int available = static_cast<int>(std::floor((accumulator_ + tolerance) / time_step));
        const int tick_count = std::min(available, max_ticks_per_frame);
        accumulator_ = std::max(0.0, accumulator_ - tick_count * time_step);
        return {.ok = true, .tick_count = tick_count};
    }

    void reset() noexcept { accumulator_ = 0.0; }

private:
    static constexpr double max_accumulator = 0.1;
    static constexpr int max_ticks_per_frame = 4;
    double accumulator_{};
};

}
