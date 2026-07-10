#pragma once

#include <ninho/physics/physics_types.hpp>

#include <box3d/box3d.h>

namespace ninho::physics::detail {

[[nodiscard]] inline b3Vec3 to_box3d_vector(Vec3 value) noexcept
{
    return {value.x, value.y, value.z};
}

[[nodiscard]] inline b3Pos to_box3d_position(Vec3 value) noexcept
{
    return {value.x, value.y, value.z};
}

[[nodiscard]] inline b3Quat to_box3d(Quat value) noexcept
{
    return {{value.x, value.y, value.z}, value.w};
}

[[nodiscard]] inline b3Transform to_box3d_local(Transform value) noexcept
{
    return {to_box3d_vector(value.position), to_box3d(value.rotation)};
}

[[nodiscard]] inline Vec3 from_box3d_vector(b3Vec3 value) noexcept
{
    return {value.x, value.y, value.z};
}

[[nodiscard]] inline Vec3 from_box3d_position(b3Pos value) noexcept
{
    return {
        static_cast<float>(value.x),
        static_cast<float>(value.y),
        static_cast<float>(value.z),
    };
}

[[nodiscard]] inline Quat from_box3d_quaternion(b3Quat value) noexcept
{
    return {value.v.x, value.v.y, value.v.z, value.s};
}

[[nodiscard]] inline Transform from_box3d_world(b3WorldTransform value) noexcept
{
    return {from_box3d_position(value.p), from_box3d_quaternion(value.q)};
}

}
