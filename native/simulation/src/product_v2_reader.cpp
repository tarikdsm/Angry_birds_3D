#include "product_v2_reader.hpp"

#include <cmath>
#include <functional>

namespace ninho::simulation::detail::v2content {
namespace {

struct DuplicateGuard {
    struct Context {
        bool array{};
        std::string pointer;
        std::unordered_set<std::string> keys;
        std::string key;
        std::size_t index{};
    };
    std::vector<Context> stack;

    std::string consume()
    {
        if (stack.empty()) return "";
        auto& parent = stack.back();
        if (parent.array) return indexed(parent.pointer, parent.index++);
        auto result = child(parent.pointer, parent.key);
        parent.key.clear();
        return result;
    }

    bool operator()(int, json::parse_event_t event, json& parsed)
    {
        if (event == json::parse_event_t::object_start
            || event == json::parse_event_t::array_start) {
            auto pointer = consume();
            if (stack.size() >= max_nesting) {
                fail(ContentErrorCode::ResourceLimit, std::move(pointer),
                    "JSON nesting limit exceeded");
            }
            stack.push_back({event == json::parse_event_t::array_start,
                std::move(pointer), {}, {}, 0U});
        } else if (event == json::parse_event_t::key) {
            auto& context = stack.back();
            auto key = parsed.get<std::string>();
            if (!context.keys.insert(key).second) {
                fail(ContentErrorCode::DuplicateKey, child(context.pointer, key),
                    "duplicate JSON key");
            }
            context.key = std::move(key);
        } else if (event == json::parse_event_t::value) {
            static_cast<void>(consume());
        } else if (event == json::parse_event_t::object_end
            || event == json::parse_event_t::array_end) {
            stack.pop_back();
        }
        return true;
    }
};

void require_runtime_representable(double value, const std::string& pointer)
{
    if (value == 0.0) return;
    const auto runtime = static_cast<float>(value);
    if (!std::isfinite(runtime) || runtime == 0.0F) {
        fail(ContentErrorCode::OutOfRange, pointer,
            "nonzero value is not representable by the runtime");
    }
}

}

[[noreturn]] void fail(ContentErrorCode code, std::string pointer, std::string message)
{
    throw ParseFailure{{code, std::move(pointer), std::move(message)}};
}

std::string child(const std::string& pointer, std::string_view token)
{
    std::string escaped;
    for (const char c : token) {
        if (c == '~') escaped += "~0";
        else if (c == '/') escaped += "~1";
        else escaped += c;
    }
    return pointer + '/' + escaped;
}

std::string indexed(const std::string& pointer, std::size_t index)
{
    return pointer + '/' + std::to_string(index);
}

json parse_json(std::string_view input, std::size_t byte_limit)
{
    if (input.size() > byte_limit) {
        fail(ContentErrorCode::ResourceLimit, "", "document byte limit exceeded");
    }
    try {
        DuplicateGuard guard;
        return json::parse(input.begin(), input.end(), std::ref(guard), true, false);
    } catch (const ParseFailure&) {
        throw;
    } catch (const json::out_of_range&) {
        fail(ContentErrorCode::InvalidNumber, "", "JSON number is not representable");
    } catch (const json::exception&) {
        fail(ContentErrorCode::InvalidJson, "", "malformed JSON document");
    }
}

void object(const json& value, const std::string& pointer)
{
    if (!value.is_object()) fail(ContentErrorCode::InvalidType, pointer, "expected object");
}

void array(const json& value, const std::string& pointer, std::size_t maximum,
    bool non_empty)
{
    if (!value.is_array()) fail(ContentErrorCode::InvalidType, pointer, "expected array");
    if (value.size() > maximum) fail(ContentErrorCode::ResourceLimit, pointer,
        "collection limit exceeded");
    if (non_empty && value.empty()) fail(ContentErrorCode::OutOfRange, pointer,
        "collection cannot be empty");
}

void keys(const json& value, const std::string& pointer,
    std::initializer_list<std::string_view> allowed)
{
    object(value, pointer);
    std::set<std::string, std::less<>> expected;
    for (const auto key : allowed) expected.emplace(key);
    for (auto it = value.begin(); it != value.end(); ++it) {
        if (!expected.contains(it.key())) {
            fail(ContentErrorCode::UnknownKey, child(pointer, it.key()), "unknown key");
        }
    }
    for (const auto key : allowed) {
        if (!value.contains(key)) fail(ContentErrorCode::MissingField, child(pointer, key),
            "missing required field");
    }
}

const json& member(const json& value, std::string_view key, const std::string& pointer)
{
    if (!value.contains(key)) fail(ContentErrorCode::MissingField, child(pointer, key),
        "missing required field");
    return value.at(key);
}

std::string text(const json& value, std::string_view key, const std::string& pointer)
{
    const auto& item = member(value, key, pointer);
    if (!item.is_string()) fail(ContentErrorCode::InvalidType, child(pointer, key),
        "expected string");
    auto result = item.get<std::string>();
    if (result.empty() || result.size() > 128U) fail(ContentErrorCode::OutOfRange,
        child(pointer, key), "string length out of range");
    return result;
}

bool boolean(const json& value, std::string_view key, const std::string& pointer)
{
    const auto& item = member(value, key, pointer);
    if (!item.is_boolean()) fail(ContentErrorCode::InvalidType, child(pointer, key),
        "expected boolean");
    return item.get<bool>();
}

double number(const json& value, std::string_view key, const std::string& pointer,
    double minimum, double maximum, bool minimum_inclusive)
{
    return number_value(member(value, key, pointer), child(pointer, key), minimum, maximum,
        minimum_inclusive);
}

double number_value(const json& item, const std::string& value_pointer,
    double minimum, double maximum, bool minimum_inclusive)
{
    if (!item.is_number()) fail(ContentErrorCode::InvalidType, value_pointer,
        "expected number");
    double result{};
    try {
        result = item.get<double>();
    } catch (...) {
        fail(ContentErrorCode::InvalidNumber, value_pointer, "number is not representable");
    }
    if (!std::isfinite(result)) fail(ContentErrorCode::InvalidNumber, value_pointer,
        "number must be finite");
    const bool below = minimum_inclusive ? result < minimum : result <= minimum;
    if (below || result > maximum) fail(ContentErrorCode::OutOfRange, value_pointer,
        "number out of range");
    require_runtime_representable(result, value_pointer);
    return result;
}

std::uint32_t uint_value(const json& item, const std::string& pointer,
    std::uint32_t minimum, std::uint32_t maximum)
{
    if (!item.is_number_integer() && !item.is_number_unsigned()) {
        fail(ContentErrorCode::InvalidType, pointer, "expected unsigned integer");
    }
    std::int64_t value{};
    try {
        value = item.get<std::int64_t>();
    } catch (...) {
        fail(ContentErrorCode::OutOfRange, pointer, "integer out of range");
    }
    if (value < minimum || static_cast<std::uint64_t>(value) > maximum) {
        fail(ContentErrorCode::OutOfRange, pointer, "integer out of range");
    }
    return static_cast<std::uint32_t>(value);
}

std::uint32_t uint(const json& value, std::string_view key, const std::string& pointer,
    std::uint32_t minimum, std::uint32_t maximum)
{
    return uint_value(member(value, key, pointer), child(pointer, key), minimum, maximum);
}

void schema_v2(const json& root)
{
    if (uint(root, "schema_version", "", 1U, 2U) != 2U) {
        fail(ContentErrorCode::OutOfRange, "/schema_version", "expected schema version 2");
    }
}

std::vector<std::string> string_set(const json& root, std::string_view key,
    std::size_t maximum, bool non_empty)
{
    const auto pointer = child("", key);
    const auto& values = member(root, key, "");
    array(values, pointer, maximum, non_empty);
    std::vector<std::string> result;
    std::unordered_set<std::string> unique;
    for (std::size_t i = 0; i < values.size(); ++i) {
        if (!values[i].is_string()) fail(ContentErrorCode::InvalidType,
            indexed(pointer, i), "expected string");
        auto value = values[i].get<std::string>();
        if (value.empty() || value.size() > 128U) fail(ContentErrorCode::OutOfRange,
            indexed(pointer, i), "string length out of range");
        if (!unique.insert(value).second) fail(ContentErrorCode::DuplicateId,
            indexed(pointer, i), "duplicate id");
        result.push_back(std::move(value));
    }
    return result;
}

void unique_id(std::unordered_set<std::uint32_t>& values, std::uint32_t value,
    const std::string& pointer)
{
    if (!values.insert(value).second) fail(ContentErrorCode::DuplicateId, pointer,
        "duplicate id");
}

}
