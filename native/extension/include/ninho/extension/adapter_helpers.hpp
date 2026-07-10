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

namespace ninho::extension::detail {

[[nodiscard]] inline physics::Vec3 to_kernel(const godot::Vector3& value) noexcept
{
    return {
        static_cast<float>(value.x),
        static_cast<float>(value.y),
        static_cast<float>(value.z),
    };
}

[[nodiscard]] inline godot::Vector3 to_godot(physics::Vec3 value) noexcept
{
    return {value.x, value.y, value.z};
}

[[nodiscard]] inline physics::Quat to_kernel(const godot::Quaternion& value) noexcept
{
    return {
        static_cast<float>(value.x),
        static_cast<float>(value.y),
        static_cast<float>(value.z),
        static_cast<float>(value.w),
    };
}

[[nodiscard]] inline godot::Quaternion to_godot(physics::Quat value) noexcept
{
    return {value.x, value.y, value.z, value.w};
}

[[nodiscard]] inline physics::Transform to_kernel(const godot::Transform3D& value) noexcept
{
    return {to_kernel(value.origin), to_kernel(value.basis.get_quaternion())};
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
    return is_finite(value.origin) && is_finite(value.basis.get_quaternion());
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
    double radius, double surface_gravity) noexcept
{
    physics::WorldConfig config;
    config.planet_radius = static_cast<float>(radius);
    config.surface_gravity = static_cast<float>(surface_gravity);
    return config;
}

[[nodiscard]] inline physics::BodyDesc make_planet_desc(float radius)
{
    return physics::BodyDesc::static_sphere(radius, {});
}

[[nodiscard]] inline physics::BodyDesc make_box_desc(
    const godot::Vector3& full_size, const godot::Transform3D& transform, double density)
{
    return physics::BodyDesc::dynamic_box(
        to_kernel(full_size) * 0.5F, to_kernel(transform), static_cast<float>(density));
}

[[nodiscard]] inline physics::BodyDesc make_projectile_desc(
    double radius,
    const godot::Transform3D& transform,
    const godot::Vector3& velocity)
{
    physics::BodyDesc desc = physics::BodyDesc::dynamic_sphere(
        static_cast<float>(radius), to_kernel(transform), 1.0F);
    desc.linear_velocity = to_kernel(velocity);
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
    return world.apply_impulse(handle, to_kernel(impulse), to_kernel(world_point));
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
