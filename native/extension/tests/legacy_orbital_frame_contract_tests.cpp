#include "test_framework.hpp"

#include <array>
#include <filesystem>
#include <fstream>
#include <regex>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

namespace {

std::string read_source_file(std::string_view relative_path)
{
    std::ifstream stream{std::filesystem::path{NINHO_SOURCE_DIR} / relative_path,
        std::ios::binary};
    NINHO_REQUIRE(stream.is_open());
    std::ostringstream content;
    content << stream.rdbuf();
    return content.str();
}

std::string frame_dictionary_body(const std::string& source)
{
    constexpr std::string_view signature =
        "[[nodiscard]] godot::Dictionary frame_dictionary("
        "const detail::SessionFrameData& frame)";
    const std::size_t signature_position = source.find(signature);
    NINHO_REQUIRE(signature_position != std::string::npos);
    NINHO_REQUIRE(source.find(signature, signature_position + signature.size())
        == std::string::npos);
    const std::size_t opening_brace = source.find('{', signature_position + signature.size());
    NINHO_REQUIRE(opening_brace != std::string::npos);

    std::size_t depth{};
    for (std::size_t index = opening_brace; index < source.size(); ++index) {
        if (source[index] == '{') {
            ++depth;
        } else if (source[index] == '}') {
            NINHO_REQUIRE(depth > 0U);
            --depth;
            if (depth == 0U) {
                return source.substr(opening_brace + 1U, index - opening_brace - 1U);
            }
        }
    }
    NINHO_REQUIRE(false);
    return {};
}

NINHO_TEST("legacy orbital frame contract freezes exact top level dictionary keys")
{
    const std::string source = read_source_file("native/extension/src/orbital_session_node.cpp");
    const std::string body = frame_dictionary_body(source);
    const std::regex assignment{R"(result\[([^\]]+)\]\s*=)"};
    std::vector<std::string> keys;
    std::size_t assignment_count{};
    for (auto match = std::sregex_iterator{body.begin(), body.end(), assignment};
         match != std::sregex_iterator{}; ++match) {
        ++assignment_count;
        const std::string expression = (*match)[1].str();
        NINHO_REQUIRE(expression.size() >= 2U);
        NINHO_REQUIRE(expression.front() == '"' && expression.back() == '"');
        keys.push_back(expression.substr(1U, expression.size() - 2U));
    }

    const std::array expected{
        std::string_view{"tick"},
        std::string_view{"ticks_executed"},
        std::string_view{"phase"},
        std::string_view{"outcome"},
        std::string_view{"birds_remaining"},
        std::string_view{"snapshots"},
        std::string_view{"events"},
        std::string_view{"objectives_complete"},
        std::string_view{"objective_targets"},
        std::string_view{"ability_readiness"},
        std::string_view{"ability_armed"},
        std::string_view{"trajectory_preview"},
        std::string_view{"aim_envelope"},
        std::string_view{"metrics"},
        std::string_view{"discarded_time_seconds"},
    };
    NINHO_REQUIRE(assignment_count == expected.size());
    NINHO_REQUIRE(keys.size() == expected.size());
    for (std::size_t index = 0; index < expected.size(); ++index) {
        NINHO_REQUIRE(keys[index] == expected[index]);
    }
}

}
