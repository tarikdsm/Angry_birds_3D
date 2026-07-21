#pragma once

#include "ninho/simulation/content.hpp"

#include <optional>
#include <string_view>

namespace ninho::simulation::detail {

enum class DuctileEndpointOwner : std::uint8_t {
    None = 0,
    A = 1,
    B = 2,
    Ambiguous = 3,
};

[[nodiscard]] DuctileEndpointOwner resolve_ductile_endpoint_owner(
    const MaterialCatalog&, std::optional<MaterialId>,
    std::optional<MaterialId>) noexcept;

[[nodiscard]] std::optional<ContentError> validate_level_semantics(
    const ArchetypeCatalog&, const LevelManifest&);
[[nodiscard]] bool box3d_sphere_mass_is_safe(
    double mass_kg, double radius_m) noexcept;
[[nodiscard]] std::optional<ContentError> validate_bird_runtime_physics(
    const BirdArchetype&, std::string_view pointer);
[[nodiscard]] std::optional<ContentError> validate_split_child_runtime_physics(
    const BirdArchetype&, std::string_view pointer);
[[nodiscard]] std::optional<ContentError> validate_product_v2_session_content(
    const MaterialCatalog&, const ArchetypeCatalog&, const LevelManifest&);

}
