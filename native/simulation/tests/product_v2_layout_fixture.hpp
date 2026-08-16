#pragma once

#include "ninho/simulation/content.hpp"

#include <nlohmann/json_fwd.hpp>

#include <string>

namespace ninho::simulation::test {

nlohmann::json product_v2_layout_projection(const LevelManifest& level);
std::string product_v2_layout_hash(const LevelManifest& level);
nlohmann::json product_v2_layout_fixture(const LevelManifest& level);
std::string product_v2_layout_fixture_document(const LevelManifest& level);

}
