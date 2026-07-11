#include "ninho/simulation/content.hpp"

#include <nlohmann/json.hpp>

static_assert(NLOHMANN_JSON_VERSION_MAJOR == 3);
static_assert(NLOHMANN_JSON_VERSION_MINOR == 11);
static_assert(NLOHMANN_JSON_VERSION_PATCH == 3);
static_assert(sizeof(ninho::simulation::MaterialId) == sizeof(std::uint32_t));
