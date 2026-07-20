#pragma once

#include "ninho/simulation/content.hpp"

#include <optional>

namespace ninho::simulation::detail {

[[nodiscard]] std::optional<ContentError> validate_level_semantics(
    const ArchetypeCatalog&, const LevelManifest&);

}
