#pragma once

#include <cmath>
#include <compare>
#include <cstdint>

namespace ninho::physics {

struct Vec3 {
    float x{};
    float y{};
    float z{};

    auto operator<=>(const Vec3&) const = default;
};

struct Quat {
    float x{};
    float y{};
    float z{};
    float w{1.0f};

    auto operator<=>(const Quat&) const = default;
};

struct Transform {
    Vec3 position{};
    Quat rotation{};

    auto operator<=>(const Transform&) const = default;
};

[[nodiscard]] constexpr Vec3 operator+(Vec3 lhs, Vec3 rhs) noexcept
{
    return {lhs.x + rhs.x, lhs.y + rhs.y, lhs.z + rhs.z};
}

[[nodiscard]] constexpr Vec3 operator-(Vec3 lhs, Vec3 rhs) noexcept
{
    return {lhs.x - rhs.x, lhs.y - rhs.y, lhs.z - rhs.z};
}

[[nodiscard]] constexpr Vec3 operator-(Vec3 vector) noexcept
{
    return {-vector.x, -vector.y, -vector.z};
}

[[nodiscard]] constexpr Vec3 operator*(Vec3 vector, float scalar) noexcept
{
    return {vector.x * scalar, vector.y * scalar, vector.z * scalar};
}

[[nodiscard]] constexpr Vec3 operator*(float scalar, Vec3 vector) noexcept
{
    return vector * scalar;
}

[[nodiscard]] constexpr Vec3 operator/(Vec3 vector, float scalar) noexcept
{
    return {vector.x / scalar, vector.y / scalar, vector.z / scalar};
}

[[nodiscard]] constexpr float dot(Vec3 lhs, Vec3 rhs) noexcept
{
    return lhs.x * rhs.x + lhs.y * rhs.y + lhs.z * rhs.z;
}

[[nodiscard]] constexpr Vec3 cross(Vec3 lhs, Vec3 rhs) noexcept
{
    return {
        lhs.y * rhs.z - lhs.z * rhs.y,
        lhs.z * rhs.x - lhs.x * rhs.z,
        lhs.x * rhs.y - lhs.y * rhs.x,
    };
}

[[nodiscard]] inline float length(Vec3 vector) noexcept
{
    return std::sqrt(dot(vector, vector));
}

[[nodiscard]] inline Vec3 normalized_or_zero(Vec3 vector) noexcept
{
    const float magnitude = length(vector);
    return magnitude == 0.0f ? Vec3{} : vector / magnitude;
}

[[nodiscard]] inline bool is_finite(Vec3 vector) noexcept
{
    return std::isfinite(vector.x) && std::isfinite(vector.y) && std::isfinite(vector.z);
}

struct BodyHandle {
    std::uint32_t index{};
    std::uint32_t generation{};

    [[nodiscard]] constexpr bool valid() const noexcept
    {
        return index != 0 && generation != 0;
    }

    auto operator<=>(const BodyHandle&) const = default;
};

struct JointHandle {
    std::uint32_t index{};
    std::uint32_t generation{};

    [[nodiscard]] constexpr bool valid() const noexcept
    {
        return index != 0 && generation != 0;
    }

    auto operator<=>(const JointHandle&) const = default;
};

}
