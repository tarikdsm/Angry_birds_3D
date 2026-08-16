#pragma once

#include <cstddef>
#include <filesystem>
#include <optional>
#include <span>
#include <string>

namespace ninho::simulation::test {

struct FixtureDocument {
    std::filesystem::path filename;
    std::string bytes;
};

struct FixturePublishOptions {
    bool force{};
    // Deterministic fault injection for the test-only fixture tool.  The
    // production emitter leaves this unset.
    std::optional<std::size_t> fail_before_promotion_index;
};

// Stages every document beside its destination before changing any published
// file. Existing destinations are accepted only with force; any promotion
// failure restores the whole prior batch.
void publish_fixture_documents(const std::filesystem::path& output_directory,
    std::span<const FixtureDocument> documents,
    FixturePublishOptions options = {});

}
