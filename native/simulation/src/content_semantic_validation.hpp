#pragma once

#include "ninho/simulation/content.hpp"

#include <optional>
#include <string_view>

namespace ninho::simulation::detail {

[[nodiscard]] std::optional<ContentError> validate_level_semantics(
    const ArchetypeCatalog&, const LevelManifest&);
[[nodiscard]] bool box3d_sphere_mass_is_safe(
    double mass_kg, double radius_m) noexcept;
[[nodiscard]] std::optional<ContentError> validate_bird_runtime_physics(
    const BirdArchetype&, std::string_view pointer);
[[nodiscard]] std::optional<ContentError> validate_product_v2_session_content(
    const MaterialCatalog&, const ArchetypeCatalog&, const LevelManifest&);

}
