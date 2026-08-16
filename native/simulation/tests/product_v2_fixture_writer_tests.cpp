#include "test_framework.hpp"
#include "product_v2_fixture_writer.hpp"

#include <atomic>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

class TemporaryDirectory {
public:
    TemporaryDirectory()
    {
        static std::atomic_uint64_t sequence{};
        const auto stamp = std::chrono::steady_clock::now()
                               .time_since_epoch().count();
        path_ = std::filesystem::temp_directory_path()
            / ("ninho-fixture-writer-"
                + std::to_string(stamp) + "-"
                + std::to_string(sequence.fetch_add(1U)));
        NINHO_SIM_REQUIRE(std::filesystem::create_directory(path_));
    }

    ~TemporaryDirectory()
    {
        std::error_code ignored;
        std::filesystem::remove_all(path_, ignored);
    }

    const std::filesystem::path& path() const noexcept { return path_; }

private:
    std::filesystem::path path_;
};

void write_text(const std::filesystem::path& path, std::string_view bytes)
{
    std::ofstream output{path, std::ios::binary | std::ios::trunc};
    NINHO_SIM_REQUIRE(output.is_open());
    output.write(bytes.data(), static_cast<std::streamsize>(bytes.size()));
    output.close();
    NINHO_SIM_REQUIRE(!output.fail());
}

std::string read_text(const std::filesystem::path& path)
{
    std::ifstream input{path, std::ios::binary};
    NINHO_SIM_REQUIRE(input.is_open());
    std::ostringstream bytes;
    bytes << input.rdbuf();
    return bytes.str();
}

bool has_staging_artifact(const std::filesystem::path& directory)
{
    for (const auto& entry : std::filesystem::directory_iterator{directory}) {
        const std::string name = entry.path().filename().string();
        if (name.find(".ninho-stage") != std::string::npos
            || name.find(".ninho-backup") != std::string::npos) {
            return true;
        }
    }
    return false;
}

NINHO_SIM_TEST("product v2 fixture writer publishes a fully staged batch")
{
    using namespace ninho::simulation::test;
    TemporaryDirectory directory;
    const std::vector documents{
        FixtureDocument{"a.json", "alpha\n"},
        FixtureDocument{"b.json", "beta\n"},
    };

    publish_fixture_documents(directory.path(), documents);

    NINHO_SIM_REQUIRE(read_text(directory.path() / "a.json") == "alpha\n");
    NINHO_SIM_REQUIRE(read_text(directory.path() / "b.json") == "beta\n");
    NINHO_SIM_REQUIRE(!has_staging_artifact(directory.path()));
}

NINHO_SIM_TEST("product v2 fixture writer refuses a nonempty destination without force")
{
    using namespace ninho::simulation::test;
    TemporaryDirectory directory;
    write_text(directory.path() / "golden.json", "accepted\n");
    const std::vector documents{
        FixtureDocument{"new.json", "replacement\n"},
    };

    bool rejected = false;
    try {
        publish_fixture_documents(directory.path(), documents);
    } catch (const std::runtime_error&) {
        rejected = true;
    }

    NINHO_SIM_REQUIRE(rejected);
    NINHO_SIM_REQUIRE(read_text(directory.path() / "golden.json")
        == "accepted\n");
    NINHO_SIM_REQUIRE(!std::filesystem::exists(directory.path() / "new.json"));
    NINHO_SIM_REQUIRE(!has_staging_artifact(directory.path()));
}

NINHO_SIM_TEST("product v2 fixture writer rolls back every golden after promotion failure")
{
    using namespace ninho::simulation::test;
    TemporaryDirectory directory;
    write_text(directory.path() / "a.json", "accepted-a\n");
    write_text(directory.path() / "b.json", "accepted-b\n");
    const std::vector documents{
        FixtureDocument{"a.json", "candidate-a\n"},
        FixtureDocument{"b.json", "candidate-b\n"},
    };

    bool rejected = false;
    try {
        publish_fixture_documents(directory.path(), documents,
            {.force = true, .fail_before_promotion_index = 1U});
    } catch (const std::runtime_error&) {
        rejected = true;
    }

    NINHO_SIM_REQUIRE(rejected);
    NINHO_SIM_REQUIRE(read_text(directory.path() / "a.json")
        == "accepted-a\n");
    NINHO_SIM_REQUIRE(read_text(directory.path() / "b.json")
        == "accepted-b\n");
    NINHO_SIM_REQUIRE(!has_staging_artifact(directory.path()));
}

NINHO_SIM_TEST("product v2 fixture writer applies the safe policy to layout-only output")
{
    using namespace ninho::simulation::test;
    TemporaryDirectory directory;
    write_text(directory.path() / "farm_reaction_layout_v1.json", "accepted\n");
    const std::vector documents{
        FixtureDocument{"farm_reaction_layout_v1.json", "candidate\n"},
    };

    bool rejected = false;
    try {
        publish_fixture_documents(directory.path(), documents);
    } catch (const std::runtime_error&) {
        rejected = true;
    }
    NINHO_SIM_REQUIRE(rejected);
    NINHO_SIM_REQUIRE(read_text(
        directory.path() / "farm_reaction_layout_v1.json") == "accepted\n");
}

}
