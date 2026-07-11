#pragma once

#include <cstddef>
#include <filesystem>
#include <string>

namespace ninho::physics::detail {

struct ReplayConformanceResult {
    bool saved{};
    bool loaded{};
    bool validated{};
    bool temporary_file_removed{};
    std::size_t bytes{};
    std::string error;
};

[[nodiscard]] ReplayConformanceResult validate_box3d_replay(
    const std::filesystem::path& temporary_path);

}
