#include "product_v2_fixture_writer.hpp"

#include <fstream>
#include <set>
#include <stdexcept>
#include <string_view>
#include <system_error>
#include <utility>
#include <vector>

namespace ninho::simulation::test {
namespace {

struct PublishEntry {
    std::filesystem::path destination;
    std::filesystem::path stage;
    std::filesystem::path backup;
    std::string_view bytes;
    bool had_destination{};
    bool backed_up{};
    bool promoted{};
};

[[noreturn]] void fail(std::string message)
{
    throw std::runtime_error(std::move(message));
}

void remove_if_present(const std::filesystem::path& path,
    bool& complete) noexcept
{
    std::error_code error;
    const bool exists = std::filesystem::exists(path, error);
    if (error) {
        complete = false;
        return;
    }
    if (exists && !std::filesystem::remove(path, error)) {
        complete = false;
    }
    if (error) {
        complete = false;
    }
}

void rename_without_throw(const std::filesystem::path& source,
    const std::filesystem::path& destination, bool& complete) noexcept
{
    std::error_code error;
    std::filesystem::rename(source, destination, error);
    if (error) {
        complete = false;
    }
}

void cleanup_stages(std::span<PublishEntry> entries) noexcept
{
    bool ignored = true;
    for (PublishEntry& entry : entries) {
        remove_if_present(entry.stage, ignored);
    }
}

bool rollback(std::span<PublishEntry> entries) noexcept
{
    bool complete = true;
    for (auto iterator = entries.rbegin(); iterator != entries.rend(); ++iterator) {
        PublishEntry& entry = *iterator;
        if (entry.promoted) {
            remove_if_present(entry.destination, complete);
            if (entry.backed_up) {
                rename_without_throw(entry.backup, entry.destination, complete);
                entry.backed_up = false;
            }
            entry.promoted = false;
        } else if (entry.backed_up) {
            rename_without_throw(entry.backup, entry.destination, complete);
            entry.backed_up = false;
        }
        remove_if_present(entry.stage, complete);
    }
    return complete;
}

void write_stage(const PublishEntry& entry)
{
    std::ofstream output{entry.stage,
        std::ios::binary | std::ios::out | std::ios::trunc};
    if (!output.is_open()) {
        fail("could not open sibling fixture staging file");
    }
    output.write(entry.bytes.data(),
        static_cast<std::streamsize>(entry.bytes.size()));
    output.flush();
    if (!output.good()) {
        fail("could not flush complete fixture staging file");
    }
    output.close();
    if (output.fail()) {
        fail("could not close complete fixture staging file");
    }
}

}

void publish_fixture_documents(const std::filesystem::path& output_directory,
    std::span<const FixtureDocument> documents, FixturePublishOptions options)
{
    if (!output_directory.is_absolute()) {
        fail("fixture output directory must be an explicit absolute path");
    }
    if (!std::filesystem::is_directory(output_directory)) {
        fail("fixture output directory does not exist");
    }
    if (documents.empty()) {
        fail("fixture publication batch cannot be empty");
    }
    if (!options.force
        && std::filesystem::directory_iterator{output_directory}
            != std::filesystem::directory_iterator{}) {
        fail("fixture output directory is not empty; pass --force to replace goldens");
    }

    std::set<std::filesystem::path> filenames;
    std::vector<PublishEntry> entries;
    entries.reserve(documents.size());
    for (const FixtureDocument& document : documents) {
        if (document.filename.empty()
            || document.filename != document.filename.filename()
            || document.filename == "." || document.filename == "..") {
            fail("fixture document filename must be a single safe path component");
        }
        if (!filenames.insert(document.filename).second) {
            fail("fixture publication batch contains a duplicate filename");
        }

        const std::filesystem::path destination =
            output_directory / document.filename;
        const bool destination_exists = std::filesystem::exists(destination);
        if (destination_exists
            && !std::filesystem::is_regular_file(destination)) {
            fail("fixture destination exists but is not a regular file");
        }
        const std::string filename = document.filename.string();
        const std::filesystem::path stage = output_directory
            / ("." + filename + ".ninho-stage");
        const std::filesystem::path backup = output_directory
            / ("." + filename + ".ninho-backup");
        if (std::filesystem::exists(stage)
            || std::filesystem::exists(backup)) {
            fail("fixture staging path already exists");
        }
        entries.push_back({
            .destination = destination,
            .stage = stage,
            .backup = backup,
            .bytes = document.bytes,
            .had_destination = destination_exists,
        });
    }

    try {
        for (const PublishEntry& entry : entries) {
            write_stage(entry);
        }

        for (PublishEntry& entry : entries) {
            if (!entry.had_destination) {
                continue;
            }
            std::filesystem::rename(entry.destination, entry.backup);
            entry.backed_up = true;
        }

        for (std::size_t index = 0U; index < entries.size(); ++index) {
            if (options.fail_before_promotion_index
                && *options.fail_before_promotion_index == index) {
                fail("injected fixture promotion failure");
            }
            PublishEntry& entry = entries[index];
            std::filesystem::rename(entry.stage, entry.destination);
            entry.promoted = true;
        }
    } catch (...) {
        if (!rollback(entries)) {
            fail("fixture publication failed and rollback was incomplete");
        }
        throw;
    }

    bool backups_removed = true;
    for (PublishEntry& entry : entries) {
        remove_if_present(entry.backup, backups_removed);
    }
    if (!backups_removed) {
        fail("fixtures were published but a sibling backup could not be removed");
    }
    cleanup_stages(entries);
}

}
