#pragma once

#include "ninho/simulation/content.hpp"

#include <cstdint>
#include <limits>

namespace ninho::simulation::detail {

inline constexpr std::uint64_t physics_surface_namespace = std::uint64_t{1} << 63U;
static_assert(physics_surface_namespace > std::numeric_limits<std::uint32_t>::max());

[[nodiscard]] constexpr std::uint64_t physics_material_tag(MaterialId id) noexcept
{
    return static_cast<std::uint64_t>(id.value());
}

[[nodiscard]] constexpr std::uint64_t physics_surface_tag(SurfaceId id) noexcept
{
    return physics_surface_namespace | static_cast<std::uint64_t>(id.value());
}

}
