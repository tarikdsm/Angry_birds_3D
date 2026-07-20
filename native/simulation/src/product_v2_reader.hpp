#pragma once

#include "ninho/simulation/content.hpp"

#include <nlohmann/json.hpp>

#include <array>
#include <cstdint>
#include <exception>
#include <limits>
#include <optional>
#include <set>
#include <string>
#include <string_view>
#include <unordered_set>
#include <utility>
#include <vector>

namespace ninho::simulation::detail::v2content {

using json = nlohmann::json;

inline constexpr std::size_t catalog_max_bytes = 256U * 1024U;
inline constexpr std::size_t level_max_bytes = 1024U * 1024U;
inline constexpr std::size_t max_nesting = 16U;
inline constexpr std::size_t max_hull_vertices = 64U;
inline constexpr std::size_t max_compound_children = 16U;
inline constexpr std::size_t max_bird_queue = 32U;
inline constexpr std::size_t max_triggers = 64U;
inline constexpr std::uint32_t runtime_entity_bit = 0x80000000U;

struct ParseFailure final : std::exception {
    ContentError error;
    explicit ParseFailure(ContentError value) : error(std::move(value)) {}
};

[[noreturn]] void fail(ContentErrorCode, std::string pointer, std::string message);
[[nodiscard]] std::string child(const std::string&, std::string_view token);
[[nodiscard]] std::string indexed(const std::string&, std::size_t index);
[[nodiscard]] json parse_json(std::string_view, std::size_t byte_limit);
void object(const json&, const std::string& pointer);
void array(const json&, const std::string& pointer, std::size_t maximum,
    bool non_empty = false);
void keys(const json&, const std::string& pointer,
    std::initializer_list<std::string_view> allowed);
[[nodiscard]] const json& member(const json&, std::string_view key,
    const std::string& pointer);
[[nodiscard]] std::string text(const json&, std::string_view key,
    const std::string& pointer);
[[nodiscard]] bool boolean(const json&, std::string_view key,
    const std::string& pointer);
[[nodiscard]] double number(const json&, std::string_view key, const std::string& pointer,
    double minimum, double maximum, bool minimum_inclusive = true);
[[nodiscard]] double number_value(const json&, const std::string& pointer,
    double minimum, double maximum, bool minimum_inclusive = true);
[[nodiscard]] std::uint32_t uint_value(const json&, const std::string& pointer,
    std::uint32_t minimum = 1U,
    std::uint32_t maximum = std::numeric_limits<std::uint32_t>::max());
[[nodiscard]] std::uint32_t uint(const json&, std::string_view key,
    const std::string& pointer, std::uint32_t minimum = 1U,
    std::uint32_t maximum = std::numeric_limits<std::uint32_t>::max());
void schema_v2(const json&);
[[nodiscard]] std::vector<std::string> string_set(const json&, std::string_view key,
    std::size_t maximum, bool non_empty = true);
void unique_id(std::unordered_set<std::uint32_t>&, std::uint32_t,
    const std::string& pointer);

template <std::size_t N>
std::array<double, N> vector(const json& value, std::string_view key,
    const std::string& pointer, double minimum, double maximum)
{
    const auto value_pointer = child(pointer, key);
    const auto& item = member(value, key, pointer);
    array(item, value_pointer, N, true);
    if (item.size() != N) fail(ContentErrorCode::OutOfRange, value_pointer,
        "vector has wrong length");
    std::array<double, N> result{};
    for (std::size_t i = 0; i < N; ++i) {
        result[i] = number_value(item[i], indexed(value_pointer, i), minimum, maximum);
    }
    return result;
}

template <typename Id>
Id id(const json& value, std::string_view key, const std::string& pointer)
{
    return Id{uint(value, key, pointer)};
}

template <typename Id>
std::optional<Id> nullable_id(const json& value, std::string_view key,
    const std::string& pointer)
{
    const auto& item = member(value, key, pointer);
    if (item.is_null()) return std::nullopt;
    return Id{uint_value(item, child(pointer, key))};
}

template <typename T, typename Function>
ContentResult<T> boundary(Function&& function) noexcept
{
    try {
        return {std::forward<Function>(function)(), {}};
    } catch (const ParseFailure& failure) {
        return {{}, failure.error};
    } catch (...) {
        return {{}, {ContentErrorCode::InternalError, "", "content processing failed"}};
    }
}

}
