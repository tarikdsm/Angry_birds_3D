#include "ninho/simulation/content.hpp"

#include <nlohmann/json.hpp>

#include <algorithm>
#include <cmath>
#include <limits>
#include <set>
#include <stdexcept>
#include <unordered_map>
#include <unordered_set>
#include <utility>

namespace ninho::simulation
{
namespace
{

using json = nlohmann::json;

constexpr std::size_t kCatalogMaxBytes = 256U * 1024U;
constexpr std::size_t kLevelMaxBytes = 1024U * 1024U;
constexpr std::size_t kMaxNesting = 16U;
constexpr std::size_t kMaxHullVertices = 64U;
constexpr std::size_t kMaxCompoundChildren = 16U;
constexpr std::size_t kMaxBirdQueue = 32U;
constexpr std::size_t kMaxTriggers = 64U;
constexpr std::uint32_t kRuntimeEntityBit = 0x80000000U;

struct Failure final : std::exception
{
    ContentError error;
    explicit Failure(ContentError value) : error(std::move(value)) {}
};

[[noreturn]] void fail(ContentErrorCode code, std::string pointer, std::string message)
{
    throw Failure{{code, std::move(pointer), std::move(message)}};
}

std::string child(const std::string& pointer, std::string_view token)
{
    std::string escaped;
    for (const char c : token)
    {
        if (c == '~')
            escaped += "~0";
        else if (c == '/')
            escaped += "~1";
        else
            escaped += c;
    }
    return pointer + '/' + escaped;
}

std::string indexed(const std::string& pointer, std::size_t index)
{
    return pointer + '/' + std::to_string(index);
}

struct DuplicateGuard
{
    struct Context
    {
        bool array{};
        std::string pointer;
        std::unordered_set<std::string> keys;
        std::string key;
        std::size_t index{};
    };
    std::vector<Context> stack;

    std::string consume()
    {
        if (stack.empty())
            return "";
        auto& parent = stack.back();
        if (parent.array)
            return indexed(parent.pointer, parent.index++);
        auto result = child(parent.pointer, parent.key);
        parent.key.clear();
        return result;
    }

    bool operator()(int, json::parse_event_t event, json& parsed)
    {
        if (event == json::parse_event_t::object_start || event == json::parse_event_t::array_start)
        {
            auto pointer = consume();
            if (stack.size() >= kMaxNesting)
            {
                fail(ContentErrorCode::ResourceLimit, std::move(pointer),
                     "JSON nesting limit exceeded");
            }
            stack.push_back(
                {event == json::parse_event_t::array_start, std::move(pointer), {}, {}, 0U});
        }
        else if (event == json::parse_event_t::key)
        {
            auto& context = stack.back();
            auto key = parsed.get<std::string>();
            if (!context.keys.insert(key).second)
            {
                fail(ContentErrorCode::DuplicateKey, child(context.pointer, key),
                     "duplicate JSON key");
            }
            context.key = std::move(key);
        }
        else if (event == json::parse_event_t::value)
        {
            static_cast<void>(consume());
        }
        else if (event == json::parse_event_t::object_end ||
                 event == json::parse_event_t::array_end)
        {
            stack.pop_back();
        }
        return true;
    }
};

json parse_json(std::string_view text, std::size_t byte_limit)
{
    if (text.size() > byte_limit)
        fail(ContentErrorCode::ResourceLimit, "", "document byte limit exceeded");
    try
    {
        DuplicateGuard guard;
        return json::parse(text.begin(), text.end(), std::ref(guard), true, false);
    }
    catch (const Failure&)
    {
        throw;
    }
    catch (const json::out_of_range&)
    {
        fail(ContentErrorCode::InvalidNumber, "", "JSON number is not representable");
    }
    catch (const json::exception&)
    {
        fail(ContentErrorCode::InvalidJson, "", "malformed JSON document");
    }
}

void object(const json& value, const std::string& pointer)
{
    if (!value.is_object())
        fail(ContentErrorCode::InvalidType, pointer, "expected object");
}

void array(const json& value, const std::string& pointer, std::size_t maximum,
           bool non_empty = false)
{
    if (!value.is_array())
        fail(ContentErrorCode::InvalidType, pointer, "expected array");
    if (value.size() > maximum)
        fail(ContentErrorCode::ResourceLimit, pointer, "collection limit exceeded");
    if (non_empty && value.empty())
        fail(ContentErrorCode::OutOfRange, pointer, "collection cannot be empty");
}

void keys(const json& value, const std::string& pointer,
          std::initializer_list<std::string_view> allowed)
{
    object(value, pointer);
    std::set<std::string, std::less<>> expected;
    for (const auto key : allowed)
        expected.emplace(key);
    for (auto it = value.begin(); it != value.end(); ++it)
    {
        if (!expected.contains(it.key()))
            fail(ContentErrorCode::UnknownKey, child(pointer, it.key()), "unknown key");
    }
    for (const auto key : allowed)
    {
        if (!value.contains(key))
            fail(ContentErrorCode::MissingField, child(pointer, key), "missing required field");
    }
}

const json& member(const json& value, std::string_view key, const std::string& pointer)
{
    if (!value.contains(key))
        fail(ContentErrorCode::MissingField, child(pointer, key), "missing required field");
    return value.at(key);
}

std::string text(const json& value, std::string_view key, const std::string& pointer)
{
    const auto& item = member(value, key, pointer);
    if (!item.is_string())
        fail(ContentErrorCode::InvalidType, child(pointer, key), "expected string");
    auto result = item.get<std::string>();
    if (result.empty() || result.size() > 128U)
        fail(ContentErrorCode::OutOfRange, child(pointer, key), "string length out of range");
    return result;
}

bool boolean(const json& value, std::string_view key, const std::string& pointer)
{
    const auto& item = member(value, key, pointer);
    if (!item.is_boolean())
        fail(ContentErrorCode::InvalidType, child(pointer, key), "expected boolean");
    return item.get<bool>();
}

double number(const json& value, std::string_view key, const std::string& pointer, double minimum,
              double maximum, bool minimum_inclusive = true)
{
    const auto value_pointer = child(pointer, key);
    const auto& item = member(value, key, pointer);
    if (!item.is_number())
        fail(ContentErrorCode::InvalidType, value_pointer, "expected number");
    double result{};
    try
    {
        result = item.get<double>();
    }
    catch (...)
    {
        fail(ContentErrorCode::InvalidNumber, value_pointer, "number is not representable");
    }
    if (!std::isfinite(result))
        fail(ContentErrorCode::InvalidNumber, value_pointer, "number must be finite");
    const bool below = minimum_inclusive ? result < minimum : result <= minimum;
    if (below || result > maximum)
        fail(ContentErrorCode::OutOfRange, value_pointer, "number out of range");
    if (result > 0.0)
    {
        const auto runtime = static_cast<float>(result);
        if (!std::isfinite(runtime) || runtime <= 0.0F)
            fail(ContentErrorCode::OutOfRange, value_pointer,
                 "positive value is not representable by the runtime");
    }
    return result;
}

std::uint32_t uint_value(const json& item, const std::string& pointer, std::uint32_t minimum = 1U,
                         std::uint32_t maximum = std::numeric_limits<std::uint32_t>::max())
{
    if (!item.is_number_integer() && !item.is_number_unsigned())
        fail(ContentErrorCode::InvalidType, pointer, "expected unsigned integer");
    std::int64_t value{};
    try
    {
        value = item.get<std::int64_t>();
    }
    catch (...)
    {
        fail(ContentErrorCode::OutOfRange, pointer, "integer out of range");
    }
    if (value < minimum || static_cast<std::uint64_t>(value) > maximum)
        fail(ContentErrorCode::OutOfRange, pointer, "integer out of range");
    return static_cast<std::uint32_t>(value);
}

std::uint32_t uint(const json& value, std::string_view key, const std::string& pointer,
                   std::uint32_t minimum = 1U,
                   std::uint32_t maximum = std::numeric_limits<std::uint32_t>::max())
{
    return uint_value(member(value, key, pointer), child(pointer, key), minimum, maximum);
}

template <std::size_t N>
std::array<double, N> vector(const json& value, std::string_view key, const std::string& pointer,
                             double minimum, double maximum)
{
    const auto value_pointer = child(pointer, key);
    const auto& item = member(value, key, pointer);
    array(item, value_pointer, N, true);
    if (item.size() != N)
        fail(ContentErrorCode::OutOfRange, value_pointer, "vector has wrong length");
    std::array<double, N> result{};
    for (std::size_t i = 0; i < N; ++i)
    {
        if (!item[i].is_number())
            fail(ContentErrorCode::InvalidType, indexed(value_pointer, i), "expected number");
        try
        {
            result[i] = item[i].get<double>();
        }
        catch (...)
        {
            fail(ContentErrorCode::InvalidNumber, indexed(value_pointer, i),
                 "number is not representable");
        }
        if (!std::isfinite(result[i]))
            fail(ContentErrorCode::InvalidNumber, indexed(value_pointer, i),
                 "number must be finite");
        if (result[i] < minimum || result[i] > maximum)
            fail(ContentErrorCode::OutOfRange, indexed(value_pointer, i), "number out of range");
    }
    return result;
}

template <typename T, typename Function> ContentResult<T> boundary(Function&& function) noexcept
{
    try
    {
        return {std::forward<Function>(function)(), {}};
    }
    catch (const Failure& failure)
    {
        return {{}, failure.error};
    }
    catch (...)
    {
        return {{}, {ContentErrorCode::InternalError, "", "content processing failed"}};
    }
}

void schema_v2(const json& root)
{
    if (uint(root, "schema_version", "", 1U, 2U) != 2U)
        fail(ContentErrorCode::OutOfRange, "/schema_version", "expected schema version 2");
}

std::vector<std::string> string_set(const json& root, std::string_view key, std::size_t maximum,
                                    bool non_empty = true)
{
    const auto pointer = child("", key);
    const auto& values = member(root, key, "");
    array(values, pointer, maximum, non_empty);
    std::vector<std::string> result;
    std::unordered_set<std::string> unique;
    for (std::size_t i = 0; i < values.size(); ++i)
    {
        if (!values[i].is_string())
            fail(ContentErrorCode::InvalidType, indexed(pointer, i), "expected string");
        auto value = values[i].get<std::string>();
        if (value.empty() || value.size() > 128U)
            fail(ContentErrorCode::OutOfRange, indexed(pointer, i), "string length out of range");
        if (!unique.insert(value).second)
            fail(ContentErrorCode::DuplicateId, indexed(pointer, i), "duplicate id");
        result.push_back(std::move(value));
    }
    return result;
}

MaterialResponse material_response(std::string_view value, const std::string& pointer)
{
    if (value == "fibrous")
        return MaterialResponse::Fibrous;
    if (value == "masonry")
        return MaterialResponse::Masonry;
    if (value == "brittle")
        return MaterialResponse::Brittle;
    if (value == "compressible")
        return MaterialResponse::Compressible;
    if (value == "ductile")
        return MaterialResponse::Ductile;
    fail(ContentErrorCode::InvalidEnum, pointer, "invalid material response");
}

BodyType body_type(std::string_view value, const std::string& pointer)
{
    if (value == "static")
        return BodyType::Static;
    if (value == "dynamic")
        return BodyType::Dynamic;
    fail(ContentErrorCode::InvalidEnum, pointer, "invalid body type");
}

JointKind joint_kind(std::string_view value, const std::string& pointer)
{
    if (value == "pine_fit")
        return JointKind::PineFit;
    if (value == "glass_clamp")
        return JointKind::GlassClamp;
    if (value == "mortar")
        return JointKind::Mortar;
    fail(ContentErrorCode::InvalidEnum, pointer, "invalid joint kind");
}

template <typename Id> Id id(const json& value, std::string_view key, const std::string& pointer)
{
    return Id{uint(value, key, pointer)};
}

template <typename Id>
std::optional<Id> nullable_id(const json& value, std::string_view key, const std::string& pointer)
{
    const auto& item = member(value, key, pointer);
    if (item.is_null())
        return std::nullopt;
    return Id{uint_value(item, child(pointer, key))};
}

void unique_id(std::unordered_set<std::uint32_t>& values, std::uint32_t value,
               const std::string& pointer)
{
    if (!values.insert(value).second)
        fail(ContentErrorCode::DuplicateId, pointer, "duplicate id");
}

AbilityArchetype parse_ability(const json& item, const std::string& pointer)
{
    keys(item, pointer, {"id", "key", "kind", "payload"});
    AbilityArchetype result;
    result.id = id<AbilityId>(item, "id", pointer);
    result.key = text(item, "key", pointer);
    result.kind = text(item, "kind", pointer);
    const auto payload_pointer = child(pointer, "payload");
    const auto& payload = member(item, "payload", pointer);
    if (result.kind == "gravity_field")
    {
        result.kind_v2 = AbilityKind::GravityField;
        keys(payload, payload_pointer,
             {"arm_ticks", "duration_ticks", "radius_m", "max_body_mass_kg", "max_bodies",
              "max_acceleration_m_s2", "pulse_speed_m_s"});
        AbilityArchetype::GravityFieldPayload value;
        value.arm_ticks = uint(payload, "arm_ticks", payload_pointer, 1U, 3600U);
        value.duration_ticks = uint(payload, "duration_ticks", payload_pointer, 1U, 3600U);
        value.radius_m = number(payload, "radius_m", payload_pointer, 0.0, 100.0, false);
        value.max_body_mass_kg =
            number(payload, "max_body_mass_kg", payload_pointer, 0.0, 100000.0, false);
        value.max_bodies = uint(payload, "max_bodies", payload_pointer, 1U, 500U);
        value.max_acceleration_m_s2 =
            number(payload, "max_acceleration_m_s2", payload_pointer, 0.0, 1000.0, false);
        value.pulse_speed_m_s =
            number(payload, "pulse_speed_m_s", payload_pointer, 0.0, 1000.0, false);
        result.arm_ticks = value.arm_ticks;
        result.duration_ticks = value.duration_ticks;
        result.radius_m = value.radius_m;
        result.max_body_mass_kg = value.max_body_mass_kg;
        result.max_bodies = value.max_bodies;
        result.max_acceleration_m_s2 = value.max_acceleration_m_s2;
        result.pulse_speed_m_s = value.pulse_speed_m_s;
        result.payload = value;
    }
    else if (result.kind == "mass_boost")
    {
        result.kind_v2 = AbilityKind::MassBoost;
        keys(payload, payload_pointer, {"duration_ticks", "mass_multiplier"});
        result.payload = AbilityArchetype::MassBoostPayload{
            uint(payload, "duration_ticks", payload_pointer, 1U, 3600U),
            number(payload, "mass_multiplier", payload_pointer, 1.0, 20.0, false)};
    }
    else if (result.kind == "speed_boost")
    {
        result.kind_v2 = AbilityKind::SpeedBoost;
        keys(payload, payload_pointer, {"impulse_m_s"});
        result.payload = AbilityArchetype::SpeedBoostPayload{
            number(payload, "impulse_m_s", payload_pointer, 0.0, 1000.0, false)};
    }
    else if (result.kind == "explosion")
    {
        result.kind_v2 = AbilityKind::Explosion;
        keys(payload, payload_pointer, {"radius_m", "impulse_n_s", "energy_j", "max_bodies"});
        result.payload = AbilityArchetype::ExplosionPayload{
            number(payload, "radius_m", payload_pointer, 0.0, 100.0, false),
            number(payload, "impulse_n_s", payload_pointer, 0.0, 1e9, false),
            number(payload, "energy_j", payload_pointer, 0.0, 1e12, false),
            uint(payload, "max_bodies", payload_pointer, 1U, 500U)};
    }
    else if (result.kind == "split")
    {
        result.kind_v2 = AbilityKind::Split;
        keys(payload, payload_pointer,
             {"child_count", "spread_angle_deg", "child_speed_multiplier"});
        result.payload = AbilityArchetype::SplitPayload{
            uint(payload, "child_count", payload_pointer, 2U, 16U),
            number(payload, "spread_angle_deg", payload_pointer, 0.0, 180.0, false),
            number(payload, "child_speed_multiplier", payload_pointer, 0.0, 2.0, false)};
    }
    else
    {
        fail(ContentErrorCode::InvalidEnum, child(pointer, "kind"), "invalid ability kind");
    }
    return result;
}

ShapeDefinition parse_shape(const json& item, const std::string& pointer, std::size_t depth = 0U)
{
    if (depth > 4U)
        fail(ContentErrorCode::ResourceLimit, pointer, "compound shape depth exceeded");
    object(item, pointer);
    const auto type = text(item, "type", pointer);
    ShapeDefinition result;
    if (type == "box")
    {
        keys(item, pointer, {"type", "half_extents_m"});
        result.type = ShapeType::Box;
        result.half_extents_m = vector<3>(item, "half_extents_m", pointer, 0.0001, 1000.0);
    }
    else if (type == "sphere")
    {
        keys(item, pointer, {"type", "radius_m"});
        result.type = ShapeType::Sphere;
        result.radius_m = number(item, "radius_m", pointer, 0.0, 1000.0, false);
    }
    else if (type == "capsule")
    {
        keys(item, pointer, {"type", "radius_m", "half_height_m"});
        result.type = ShapeType::Capsule;
        result.radius_m = number(item, "radius_m", pointer, 0.0, 1000.0, false);
        result.half_height_m = number(item, "half_height_m", pointer, 0.0, 1000.0, false);
    }
    else if (type == "convex_hull")
    {
        keys(item, pointer, {"type", "vertices_m"});
        result.type = ShapeType::ConvexHull;
        const auto vertices_pointer = child(pointer, "vertices_m");
        const auto& vertices = member(item, "vertices_m", pointer);
        array(vertices, vertices_pointer, kMaxHullVertices, true);
        if (vertices.size() < 4U)
            fail(ContentErrorCode::InvalidInvariant, vertices_pointer,
                 "convex hull requires four vertices");
        for (std::size_t i = 0; i < vertices.size(); ++i)
        {
            json holder{{"v", vertices[i]}};
            result.vertices_m.push_back(
                vector<3>(holder, "v", indexed(vertices_pointer, i), -1000.0, 1000.0));
        }
        const auto& a = result.vertices_m[0];
        bool volume = false;
        for (std::size_t i = 1; i + 2 < result.vertices_m.size() && !volume; ++i)
        {
            for (std::size_t j = i + 1; j + 1 < result.vertices_m.size() && !volume; ++j)
            {
                for (std::size_t k = j + 1; k < result.vertices_m.size(); ++k)
                {
                    const auto& b = result.vertices_m[i];
                    const auto& c = result.vertices_m[j];
                    const auto& d = result.vertices_m[k];
                    const double bx = b[0] - a[0], by = b[1] - a[1], bz = b[2] - a[2];
                    const double cx = c[0] - a[0], cy = c[1] - a[1], cz = c[2] - a[2];
                    const double dx = d[0] - a[0], dy = d[1] - a[1], dz = d[2] - a[2];
                    const double determinant = bx * (cy * dz - cz * dy) - by * (cx * dz - cz * dx) +
                                               bz * (cx * dy - cy * dx);
                    if (std::abs(determinant) > 1e-9)
                    {
                        volume = true;
                        break;
                    }
                }
            }
        }
        if (!volume)
            fail(ContentErrorCode::InvalidInvariant, vertices_pointer, "convex hull is degenerate");
    }
    else if (type == "compound")
    {
        keys(item, pointer, {"type", "children"});
        result.type = ShapeType::Compound;
        const auto children_pointer = child(pointer, "children");
        const auto& children = member(item, "children", pointer);
        array(children, children_pointer, kMaxCompoundChildren, true);
        result.children.reserve(children.size());
        for (std::size_t i = 0; i < children.size(); ++i)
            result.children.push_back(
                parse_shape(children[i], indexed(children_pointer, i), depth + 1U));
    }
    else
    {
        fail(ContentErrorCode::InvalidEnum, child(pointer, "type"), "invalid shape type");
    }
    return result;
}

TransformDefinition parse_transform(const json& item, const std::string& pointer)
{
    keys(item, pointer, {"position_m", "rotation_xyzw"});
    TransformDefinition result;
    result.position_m = vector<3>(item, "position_m", pointer, -100000.0, 100000.0);
    result.rotation_xyzw = vector<4>(item, "rotation_xyzw", pointer, -1.0, 1.0);
    const auto& q = result.rotation_xyzw;
    const double length = std::sqrt(q[0] * q[0] + q[1] * q[1] + q[2] * q[2] + q[3] * q[3]);
    if (std::abs(length - 1.0) > 1e-6)
        fail(ContentErrorCode::InvalidInvariant, child(pointer, "rotation_xyzw"),
             "rotation must be normalized");
    return result;
}

json shape_json(const ShapeDefinition& shape)
{
    switch (shape.type)
    {
    case ShapeType::Box:
        return {{"type", "box"}, {"half_extents_m", shape.half_extents_m}};
    case ShapeType::Sphere:
        return {{"type", "sphere"}, {"radius_m", shape.radius_m}};
    case ShapeType::Capsule:
        return {{"type", "capsule"},
                {"radius_m", shape.radius_m},
                {"half_height_m", shape.half_height_m}};
    case ShapeType::ConvexHull:
        return {{"type", "convex_hull"}, {"vertices_m", shape.vertices_m}};
    case ShapeType::Compound:
    {
        json children = json::array();
        for (const auto& value : shape.children)
            children.push_back(shape_json(value));
        return {{"type", "compound"}, {"children", std::move(children)}};
    }
    }
    return nullptr;
}

const char* response_name(MaterialResponse response)
{
    switch (response)
    {
    case MaterialResponse::Fibrous:
        return "fibrous";
    case MaterialResponse::Masonry:
        return "masonry";
    case MaterialResponse::Brittle:
        return "brittle";
    case MaterialResponse::Compressible:
        return "compressible";
    case MaterialResponse::Ductile:
        return "ductile";
    }
    return "invalid";
}

const char* joint_name(JointKind kind)
{
    switch (kind)
    {
    case JointKind::PineFit:
        return "pine_fit";
    case JointKind::GlassClamp:
        return "glass_clamp";
    case JointKind::Mortar:
        return "mortar";
    }
    return "invalid";
}

json ability_payload_json(const AbilityArchetype& ability)
{
    switch (ability.kind_v2)
    {
    case AbilityKind::GravityField:
    {
        const auto& p = std::get<AbilityArchetype::GravityFieldPayload>(ability.payload);
        return {{"arm_ticks", p.arm_ticks},
                {"duration_ticks", p.duration_ticks},
                {"radius_m", p.radius_m},
                {"max_body_mass_kg", p.max_body_mass_kg},
                {"max_bodies", p.max_bodies},
                {"max_acceleration_m_s2", p.max_acceleration_m_s2},
                {"pulse_speed_m_s", p.pulse_speed_m_s}};
    }
    case AbilityKind::MassBoost:
    {
        const auto& p = std::get<AbilityArchetype::MassBoostPayload>(ability.payload);
        return {{"duration_ticks", p.duration_ticks}, {"mass_multiplier", p.mass_multiplier}};
    }
    case AbilityKind::SpeedBoost:
    {
        const auto& p = std::get<AbilityArchetype::SpeedBoostPayload>(ability.payload);
        return {{"impulse_m_s", p.impulse_m_s}};
    }
    case AbilityKind::Explosion:
    {
        const auto& p = std::get<AbilityArchetype::ExplosionPayload>(ability.payload);
        return {{"radius_m", p.radius_m},
                {"impulse_n_s", p.impulse_n_s},
                {"energy_j", p.energy_j},
                {"max_bodies", p.max_bodies}};
    }
    case AbilityKind::Split:
    {
        const auto& p = std::get<AbilityArchetype::SplitPayload>(ability.payload);
        return {{"child_count", p.child_count},
                {"spread_angle_deg", p.spread_angle_deg},
                {"child_speed_multiplier", p.child_speed_multiplier}};
    }
    case AbilityKind::LegacyGravityField:
        break;
    }
    return nullptr;
}

} // namespace

ContentResult<MaterialCatalog> parse_material_catalog_v2(std::string_view input) noexcept
{
    return boundary<MaterialCatalog>(
        [&]
        {
            const auto root = parse_json(input, kCatalogMaxBytes);
            object(root, "");
            schema_v2(root);
            keys(root, "", {"schema_version", "materials", "surfaces"});
            MaterialCatalog result;
            result.schema_version = result.source_schema_version = 2U;
            const auto& materials = member(root, "materials", "");
            array(materials, "/materials", 64U, true);
            std::unordered_set<std::uint32_t> ids;
            std::unordered_set<std::string> names;
            for (std::size_t i = 0; i < materials.size(); ++i)
            {
                const auto pointer = indexed("/materials", i);
                const auto& item = materials[i];
                keys(item, pointer,
                     {"id", "key", "response", "density_kg_m3", "friction", "restitution",
                      "toughness"});
                MaterialDefinition value;
                value.id = id<MaterialId>(item, "id", pointer);
                unique_id(ids, value.id.value(), child(pointer, "id"));
                value.key = text(item, "key", pointer);
                if (!names.insert(value.key).second)
                    fail(ContentErrorCode::DuplicateId, child(pointer, "key"), "duplicate key");
                value.response =
                    material_response(text(item, "response", pointer), child(pointer, "response"));
                value.density_kg_m3 = number(item, "density_kg_m3", pointer, 0.0, 30000.0, false);
                value.friction = number(item, "friction", pointer, 0.0, 1.0);
                value.restitution = number(item, "restitution", pointer, 0.0, 1.0);
                value.toughness = number(item, "toughness", pointer, 0.0, 1.0, false);
                result.materials.push_back(std::move(value));
            }
            ids.clear();
            const auto& surfaces = member(root, "surfaces", "");
            array(surfaces, "/surfaces", 64U, true);
            for (std::size_t i = 0; i < surfaces.size(); ++i)
            {
                const auto pointer = indexed("/surfaces", i);
                const auto& item = surfaces[i];
                keys(item, pointer, {"id", "key", "density_kg_m3", "friction", "restitution"});
                PhysicsSurfaceDefinition value;
                value.id = id<SurfaceId>(item, "id", pointer);
                unique_id(ids, value.id.value(), child(pointer, "id"));
                if (value.id.value() < 1000U)
                    fail(ContentErrorCode::OutOfRange, child(pointer, "id"),
                         "surface id must be reserved");
                value.key = text(item, "key", pointer);
                if (!names.insert(value.key).second)
                    fail(ContentErrorCode::DuplicateId, child(pointer, "key"), "duplicate key");
                value.density_kg_m3 = number(item, "density_kg_m3", pointer, 0.0, 30000.0, false);
                value.friction = number(item, "friction", pointer, 0.0, 1.0);
                value.restitution = number(item, "restitution", pointer, 0.0, 1.0);
                result.surfaces.push_back(std::move(value));
            }
            return result;
        });
}

ContentResult<ArchetypeCatalog> parse_archetype_catalog_v2(std::string_view input) noexcept
{
    return boundary<ArchetypeCatalog>(
        [&]
        {
            const auto root = parse_json(input, kCatalogMaxBytes);
            object(root, "");
            schema_v2(root);
            keys(root, "",
                 {"schema_version", "presentation_ids", "score_ids", "abilities", "birds",
                  "weakpoints", "enemies"});
            ArchetypeCatalog result;
            result.schema_version = result.source_schema_version = 2U;
            result.presentation_ids = string_set(root, "presentation_ids", 128U);
            result.score_ids = string_set(root, "score_ids", 64U);
            const std::unordered_set<std::string> presentation(result.presentation_ids.begin(),
                                                               result.presentation_ids.end());
            const std::unordered_set<std::string> scores(result.score_ids.begin(),
                                                         result.score_ids.end());
            std::unordered_set<std::uint32_t> ability_ids;
            const auto& abilities = member(root, "abilities", "");
            array(abilities, "/abilities", 32U, true);
            for (std::size_t i = 0; i < abilities.size(); ++i)
            {
                auto value = parse_ability(abilities[i], indexed("/abilities", i));
                unique_id(ability_ids, value.id.value(), indexed("/abilities", i) + "/id");
                result.abilities.push_back(std::move(value));
            }
            std::unordered_set<std::uint32_t> bird_ids;
            const auto& birds = member(root, "birds", "");
            array(birds, "/birds", 32U, true);
            for (std::size_t i = 0; i < birds.size(); ++i)
            {
                const auto pointer = indexed("/birds", i);
                const auto& item = birds[i];
                keys(item, pointer,
                     {"id", "key", "projectile_visual_id", "ability_id", "surface_id", "mass_kg",
                      "radius_m", "friction", "restitution", "bullet", "launch_speed_cap_m_s",
                      "score_id", "icon_id", "animation_id"});
                BirdArchetype value;
                value.id = id<BirdArchetypeId>(item, "id", pointer);
                unique_id(bird_ids, value.id.value(), child(pointer, "id"));
                value.key = text(item, "key", pointer);
                value.projectile_visual_id = text(item, "projectile_visual_id", pointer);
                value.ability_id = id<AbilityId>(item, "ability_id", pointer);
                value.surface_id = id<SurfaceId>(item, "surface_id", pointer);
                value.mass_kg = number(item, "mass_kg", pointer, 0.0, 100000.0, false);
                value.radius_m = number(item, "radius_m", pointer, 0.0, 100.0, false);
                value.friction = number(item, "friction", pointer, 0.0, 1.0);
                value.restitution = number(item, "restitution", pointer, 0.0, 1.0);
                value.bullet = boolean(item, "bullet", pointer);
                value.launch_speed_cap_m_s =
                    number(item, "launch_speed_cap_m_s", pointer, 0.0, 1000.0, false);
                value.score_id = text(item, "score_id", pointer);
                value.icon_id = text(item, "icon_id", pointer);
                value.animation_id = text(item, "animation_id", pointer);
                if (!ability_ids.contains(value.ability_id.value()))
                    fail(ContentErrorCode::MissingReference, child(pointer, "ability_id"),
                         "ability reference not found");
                for (const auto& [field, ref] :
                     std::array<std::pair<std::string_view, const std::string*>, 3>{
                         {{"projectile_visual_id", &value.projectile_visual_id},
                          {"icon_id", &value.icon_id},
                          {"animation_id", &value.animation_id}}})
                    if (!presentation.contains(*ref))
                        fail(ContentErrorCode::MissingReference, child(pointer, field),
                             "presentation reference not found");
                if (!scores.contains(value.score_id))
                    fail(ContentErrorCode::MissingReference, child(pointer, "score_id"),
                         "score reference not found");
                result.birds.push_back(std::move(value));
            }
            std::unordered_set<std::uint32_t> weakpoint_ids;
            const auto& weakpoints = member(root, "weakpoints", "");
            array(weakpoints, "/weakpoints", 32U);
            for (std::size_t i = 0; i < weakpoints.size(); ++i)
            {
                const auto pointer = indexed("/weakpoints", i);
                const auto& item = weakpoints[i];
                keys(item, pointer,
                     {"id", "key", "protected_direction", "protected_cone_deg",
                      "protected_multiplier", "exposed_multiplier"});
                WeakpointProfile value;
                value.id = id<WeakpointId>(item, "id", pointer);
                unique_id(weakpoint_ids, value.id.value(), child(pointer, "id"));
                value.key = text(item, "key", pointer);
                value.protected_direction =
                    vector<3>(item, "protected_direction", pointer, -1.0, 1.0);
                const auto& d = value.protected_direction;
                const double length = std::sqrt(d[0] * d[0] + d[1] * d[1] + d[2] * d[2]);
                if (std::abs(length - 1.0) > 1e-6)
                    fail(ContentErrorCode::InvalidInvariant, child(pointer, "protected_direction"),
                         "direction must be normalized");
                value.protected_cone_deg =
                    number(item, "protected_cone_deg", pointer, 0.0, 180.0, false);
                value.protected_multiplier =
                    number(item, "protected_multiplier", pointer, 0.0, 10.0);
                value.exposed_multiplier = number(item, "exposed_multiplier", pointer, 0.0, 10.0);
                result.weakpoints.push_back(std::move(value));
            }
            std::unordered_set<std::uint32_t> enemy_ids;
            const auto& enemies = member(root, "enemies", "");
            array(enemies, "/enemies", 32U);
            for (std::size_t i = 0; i < enemies.size(); ++i)
            {
                const auto pointer = indexed("/enemies", i);
                const auto& item = enemies[i];
                keys(item, pointer,
                     {"id", "key", "weakpoint_id", "surface_id", "mass_kg", "integrity",
                      "damage_energy_j_per_kg", "max_damage"});
                EnemyArchetype value;
                value.id = id<EnemyArchetypeId>(item, "id", pointer);
                unique_id(enemy_ids, value.id.value(), child(pointer, "id"));
                value.key = text(item, "key", pointer);
                value.weakpoint_id = id<WeakpointId>(item, "weakpoint_id", pointer);
                value.surface_id = id<SurfaceId>(item, "surface_id", pointer);
                value.mass_kg = number(item, "mass_kg", pointer, 0.0, 100000.0, false);
                value.integrity = number(item, "integrity", pointer, 0.0, 100000.0, false);
                value.damage_energy_j_per_kg =
                    number(item, "damage_energy_j_per_kg", pointer, 0.0, 100000.0, false);
                value.max_damage = number(item, "max_damage", pointer, 0.0, value.integrity, false);
                if (!weakpoint_ids.contains(value.weakpoint_id.value()))
                    fail(ContentErrorCode::MissingReference, child(pointer, "weakpoint_id"),
                         "weakpoint reference not found");
                result.enemies.push_back(std::move(value));
            }
            return result;
        });
}

ContentResult<LevelManifest> parse_level_manifest_v2(std::string_view input) noexcept
{
    return boundary<LevelManifest>(
        [&]
        {
            const auto root = parse_json(input, kLevelMaxBytes);
            object(root, "");
            schema_v2(root);
            keys(root, "",
                 {"schema_version", "id", "world_id", "region_id", "camera_profile_id",
                  "presentation_profile_id", "world", "slingshot", "bird_queue", "scoring",
                  "free_body_ids", "bodies", "joints", "assemblies", "triggers", "objectives",
                  "settle_policy", "watchdog_ticks"});
            LevelManifest result;
            result.schema_version = result.source_schema_version = 2U;
            result.id = text(root, "id", "");
            result.world_id = text(root, "world_id", "");
            result.region_id = text(root, "region_id", "");
            result.camera_profile_id = text(root, "camera_profile_id", "");
            result.presentation_profile_id = text(root, "presentation_profile_id", "");
            const auto& world = member(root, "world", "");
            const auto world_kind = text(world, "kind", "/world");
            if (world_kind == "uniform")
            {
                keys(world, "/world", {"kind", "acceleration_m_s2", "bounds"});
                UniformWorldDefinition value;
                value.acceleration_m_s2 =
                    vector<3>(world, "acceleration_m_s2", "/world", -1000.0, 1000.0);
                const double length =
                    std::sqrt(value.acceleration_m_s2[0] * value.acceleration_m_s2[0] +
                              value.acceleration_m_s2[1] * value.acceleration_m_s2[1] +
                              value.acceleration_m_s2[2] * value.acceleration_m_s2[2]);
                if (length <= 1e-9)
                    fail(ContentErrorCode::OutOfRange, "/world/acceleration_m_s2",
                         "gravity cannot be zero");
                const auto& bounds = member(world, "bounds", "/world");
                keys(bounds, "/world/bounds", {"min_m", "max_m"});
                value.bounds_min_m =
                    vector<3>(bounds, "min_m", "/world/bounds", -100000.0, 100000.0);
                value.bounds_max_m =
                    vector<3>(bounds, "max_m", "/world/bounds", -100000.0, 100000.0);
                for (std::size_t i = 0; i < 3; ++i)
                    if (value.bounds_min_m[i] >= value.bounds_max_m[i])
                        fail(ContentErrorCode::InvalidInvariant, indexed("/world/bounds/max_m", i),
                             "bounds must increase");
                result.world = value;
            }
            else if (world_kind == "radial")
            {
                keys(world, "/world",
                     {"kind", "center_m", "reference_radius_m", "reference_acceleration_m_s2",
                      "bounds"});
                RadialWorldDefinition value;
                value.center_m = vector<3>(world, "center_m", "/world", -100000.0, 100000.0);
                value.reference_radius_m =
                    number(world, "reference_radius_m", "/world", 0.0, 100000.0, false);
                value.reference_acceleration_m_s2 =
                    number(world, "reference_acceleration_m_s2", "/world", 0.0, 1000.0, false);
                const auto& bounds = member(world, "bounds", "/world");
                keys(bounds, "/world/bounds", {"radius_m"});
                value.bounds_radius_m = number(bounds, "radius_m", "/world/bounds",
                                               value.reference_radius_m, 1000000.0, false);
                result.world = value;
            }
            else
                fail(ContentErrorCode::InvalidEnum, "/world/kind", "invalid world kind");
            const auto& launcher = member(root, "slingshot", "");
            keys(launcher, "/slingshot",
                 {"asset_id", "rest_position_m", "rest_rotation_xyzw", "spring_constant_n_m",
                  "energy_efficiency", "minimum_extension_m", "maximum_extension_m", "plane_policy",
                  "projectile_clearance_m", "speed_ceiling_m_s"});
            result.slingshot.asset_id = text(launcher, "asset_id", "/slingshot");
            result.slingshot.rest_position_m =
                vector<3>(launcher, "rest_position_m", "/slingshot", -100000.0, 100000.0);
            result.slingshot.rest_rotation_xyzw =
                vector<4>(launcher, "rest_rotation_xyzw", "/slingshot", -1.0, 1.0);
            const auto& q = result.slingshot.rest_rotation_xyzw;
            if (std::abs(std::sqrt(q[0] * q[0] + q[1] * q[1] + q[2] * q[2] + q[3] * q[3]) - 1.0) >
                1e-6)
                fail(ContentErrorCode::InvalidInvariant, "/slingshot/rest_rotation_xyzw",
                     "rotation must be normalized");
            result.slingshot.spring_constant_n_m =
                number(launcher, "spring_constant_n_m", "/slingshot", 0.0, 1e9, false);
            result.slingshot.energy_efficiency =
                number(launcher, "energy_efficiency", "/slingshot", 0.0, 1.0, false);
            result.slingshot.minimum_extension_m =
                number(launcher, "minimum_extension_m", "/slingshot", 0.0, 100.0);
            result.slingshot.maximum_extension_m =
                number(launcher, "maximum_extension_m", "/slingshot", 0.0, 100.0, false);
            if (result.slingshot.minimum_extension_m >= result.slingshot.maximum_extension_m)
                fail(ContentErrorCode::InvalidInvariant, "/slingshot/maximum_extension_m",
                     "extension range is invalid");
            result.slingshot.plane_policy = text(launcher, "plane_policy", "/slingshot");
            if (result.slingshot.plane_policy != "gravity_vertical_camera_yaw")
                fail(ContentErrorCode::InvalidEnum, "/slingshot/plane_policy",
                     "invalid plane policy");
            result.slingshot.projectile_clearance_m =
                number(launcher, "projectile_clearance_m", "/slingshot", 0.0, 100.0);
            result.slingshot.speed_ceiling_m_s =
                number(launcher, "speed_ceiling_m_s", "/slingshot", 0.0, 1000.0, false);
            const auto& queue = member(root, "bird_queue", "");
            array(queue, "/bird_queue", kMaxBirdQueue, true);
            for (std::size_t i = 0; i < queue.size(); ++i)
                result.bird_queue.push_back(
                    BirdArchetypeId{uint_value(queue[i], indexed("/bird_queue", i))});
            const auto& scoring = member(root, "scoring", "");
            keys(scoring, "/scoring",
                 {"pig_points", "unused_bird_points", "star_thresholds", "chain_window_ticks",
                  "chain_multiplier_step", "max_chain_multiplier"});
            result.scoring.pig_points = uint(scoring, "pig_points", "/scoring", 0U, 1000000U);
            result.scoring.unused_bird_points =
                uint(scoring, "unused_bird_points", "/scoring", 0U, 1000000U);
            const auto& stars = member(scoring, "star_thresholds", "/scoring");
            array(stars, "/scoring/star_thresholds", 3U, true);
            if (stars.size() != 3U)
                fail(ContentErrorCode::OutOfRange, "/scoring/star_thresholds",
                     "exactly three thresholds required");
            for (std::size_t i = 0; i < 3U; ++i)
            {
                result.scoring.star_thresholds[i] =
                    uint_value(stars[i], indexed("/scoring/star_thresholds", i));
                if (i > 0 &&
                    result.scoring.star_thresholds[i] <= result.scoring.star_thresholds[i - 1])
                    fail(ContentErrorCode::InvalidInvariant, indexed("/scoring/star_thresholds", i),
                         "thresholds must strictly increase");
            }
            result.scoring.chain_window_ticks =
                uint(scoring, "chain_window_ticks", "/scoring", 1U, 3600U);
            result.scoring.chain_multiplier_step =
                number(scoring, "chain_multiplier_step", "/scoring", 0.0, 10.0);
            result.scoring.max_chain_multiplier =
                number(scoring, "max_chain_multiplier", "/scoring", 1.0, 100.0);
            const auto& free_ids = member(root, "free_body_ids", "");
            array(free_ids, "/free_body_ids", 500U);
            for (std::size_t i = 0; i < free_ids.size(); ++i)
                result.free_body_ids.push_back(
                    uint_value(free_ids[i], indexed("/free_body_ids", i)));
            std::unordered_set<std::uint32_t> body_ids;
            std::unordered_set<std::uint32_t> entity_ids;
            std::unordered_set<std::uint64_t> entity_parts;
            const auto& bodies = member(root, "bodies", "");
            array(bodies, "/bodies", 500U);
            for (std::size_t i = 0; i < bodies.size(); ++i)
            {
                const auto pointer = indexed("/bodies", i);
                const auto& item = bodies[i];
                keys(item, pointer,
                     {"body_id", "entity_id", "part_id", "body_type", "affected_by_world_gravity",
                      "material_id", "surface_id", "enemy_archetype_id", "density_kg_m3",
                      "transform", "shape", "visual"});
                BodyDefinition value;
                value.body_id = uint(item, "body_id", pointer);
                unique_id(body_ids, value.body_id, child(pointer, "body_id"));
                value.entity_id = id<EntityId>(item, "entity_id", pointer);
                if (value.entity_id.value() & kRuntimeEntityBit)
                    fail(ContentErrorCode::OutOfRange, child(pointer, "entity_id"),
                         "entity id uses runtime namespace");
                entity_ids.insert(value.entity_id.value());
                value.part_id = id<PartId>(item, "part_id", pointer);
                const auto entity_part =
                    (static_cast<std::uint64_t>(value.entity_id.value()) << 32U) |
                    value.part_id.value();
                if (!entity_parts.insert(entity_part).second)
                    fail(ContentErrorCode::DuplicateId, child(pointer, "part_id"),
                         "duplicate entity and part pair");
                value.body_type =
                    body_type(text(item, "body_type", pointer), child(pointer, "body_type"));
                value.affected_by_world_gravity =
                    boolean(item, "affected_by_world_gravity", pointer);
                if (value.body_type == BodyType::Dynamic && !value.affected_by_world_gravity)
                    fail(ContentErrorCode::InvalidInvariant,
                         child(pointer, "affected_by_world_gravity"),
                         "dynamic bodies must use world gravity");
                value.material_id = nullable_id<MaterialId>(item, "material_id", pointer);
                value.surface_id = nullable_id<SurfaceId>(item, "surface_id", pointer);
                value.enemy_archetype_id =
                    nullable_id<EnemyArchetypeId>(item, "enemy_archetype_id", pointer);
                if (value.material_id.has_value() == value.surface_id.has_value())
                    fail(ContentErrorCode::InvalidInvariant, pointer,
                         "body requires exactly one material or surface");
                value.density_kg_m3 = number(item, "density_kg_m3", pointer, 0.0, 30000.0,
                                             value.body_type == BodyType::Static);
                value.transform = parse_transform(member(item, "transform", pointer),
                                                  child(pointer, "transform"));
                value.shape = parse_shape(member(item, "shape", pointer), child(pointer, "shape"));
                const auto visual_pointer = child(pointer, "visual");
                const auto& visual = member(item, "visual", pointer);
                keys(visual, visual_pointer, {"asset_id", "bounds_m"});
                value.visual.asset_id = text(visual, "asset_id", visual_pointer);
                value.visual.bounds_m =
                    vector<3>(visual, "bounds_m", visual_pointer, 0.0001, 2000.0);
                result.bodies.push_back(std::move(value));
            }
            std::unordered_set<std::uint32_t> joint_ids;
            const auto& joints = member(root, "joints", "");
            array(joints, "/joints", 250U);
            for (std::size_t i = 0; i < joints.size(); ++i)
            {
                const auto pointer = indexed("/joints", i);
                const auto& item = joints[i];
                keys(item, pointer,
                     {"id", "assembly_id", "kind", "body_a_id", "body_b_id", "force_limit_n",
                      "torque_limit_nm"});
                JointDefinition value;
                value.id = id<JointId>(item, "id", pointer);
                unique_id(joint_ids, value.id.value(), child(pointer, "id"));
                value.assembly_id = uint(item, "assembly_id", pointer);
                value.kind = joint_kind(text(item, "kind", pointer), child(pointer, "kind"));
                value.body_a_id = uint(item, "body_a_id", pointer);
                value.body_b_id = uint(item, "body_b_id", pointer);
                if (value.body_a_id == value.body_b_id)
                    fail(ContentErrorCode::InvalidInvariant, pointer,
                         "joint endpoints must differ");
                value.force_limit_n = number(item, "force_limit_n", pointer, 0.0, 1e12, false);
                value.torque_limit_nm = number(item, "torque_limit_nm", pointer, 0.0, 1e12, false);
                result.joints.push_back(value);
            }
            std::unordered_set<std::uint32_t> assembly_ids;
            const auto& assemblies = member(root, "assemblies", "");
            array(assemblies, "/assemblies", 128U);
            std::size_t memberships = 0U;
            for (std::size_t i = 0; i < assemblies.size(); ++i)
            {
                const auto pointer = indexed("/assemblies", i);
                const auto& item = assemblies[i];
                keys(item, pointer, {"id", "key", "body_ids", "joint_ids"});
                AssemblyDefinition value;
                value.id = uint(item, "id", pointer);
                unique_id(assembly_ids, value.id, child(pointer, "id"));
                value.key = text(item, "key", pointer);
                const auto& body_list = member(item, "body_ids", pointer);
                array(body_list, child(pointer, "body_ids"), 500U, true);
                for (std::size_t j = 0; j < body_list.size(); ++j)
                    value.body_ids.push_back(
                        uint_value(body_list[j], indexed(child(pointer, "body_ids"), j)));
                const auto& joint_list = member(item, "joint_ids", pointer);
                array(joint_list, child(pointer, "joint_ids"), 250U, true);
                for (std::size_t j = 0; j < joint_list.size(); ++j)
                    value.joint_ids.push_back(JointId{
                        uint_value(joint_list[j], indexed(child(pointer, "joint_ids"), j))});
                memberships += value.body_ids.size() + value.joint_ids.size();
                if (memberships > 1000U)
                    fail(ContentErrorCode::ResourceLimit, pointer,
                         "assembly membership limit exceeded");
                result.assemblies.push_back(std::move(value));
            }
            std::unordered_map<std::uint32_t, std::size_t> body_indexes;
            for (std::size_t i = 0; i < result.bodies.size(); ++i)
                body_indexes.emplace(result.bodies[i].body_id, i);
            std::unordered_map<std::uint32_t, std::size_t> joint_indexes;
            for (std::size_t i = 0; i < result.joints.size(); ++i)
                joint_indexes.emplace(result.joints[i].id.value(), i);
            std::unordered_set<std::uint32_t> owned_bodies;
            for (std::size_t i = 0; i < result.free_body_ids.size(); ++i)
            {
                const auto value = result.free_body_ids[i];
                if (!body_indexes.contains(value))
                    fail(ContentErrorCode::MissingReference, indexed("/free_body_ids", i),
                         "body reference not found");
                if (!owned_bodies.insert(value).second)
                    fail(ContentErrorCode::DuplicateId, indexed("/free_body_ids", i),
                         "duplicate body ownership");
            }
            std::unordered_set<std::uint32_t> owned_joints;
            for (std::size_t i = 0; i < result.assemblies.size(); ++i)
            {
                auto& assembly = result.assemblies[i];
                std::unordered_set<std::uint32_t> local_bodies, local_joints;
                for (std::size_t j = 0; j < assembly.body_ids.size(); ++j)
                {
                    const auto value = assembly.body_ids[j];
                    const auto pointer = indexed(child(indexed("/assemblies", i), "body_ids"), j);
                    if (!body_indexes.contains(value))
                        fail(ContentErrorCode::MissingReference, pointer,
                             "body reference not found");
                    if (!local_bodies.insert(value).second || !owned_bodies.insert(value).second)
                        fail(ContentErrorCode::DuplicateId, pointer, "duplicate body ownership");
                    result.bodies[body_indexes.at(value)].assembly_id = assembly.id;
                }
                for (std::size_t j = 0; j < assembly.joint_ids.size(); ++j)
                {
                    const auto value = assembly.joint_ids[j].value();
                    const auto pointer = indexed(child(indexed("/assemblies", i), "joint_ids"), j);
                    if (!joint_indexes.contains(value))
                        fail(ContentErrorCode::MissingReference, pointer,
                             "joint reference not found");
                    if (!local_joints.insert(value).second || !owned_joints.insert(value).second)
                        fail(ContentErrorCode::DuplicateId, pointer, "duplicate joint ownership");
                    if (result.joints[joint_indexes.at(value)].assembly_id != assembly.id)
                        fail(ContentErrorCode::InvalidInvariant, pointer,
                             "joint assembly ownership differs");
                }
            }
            for (std::size_t i = 0; i < result.joints.size(); ++i)
            {
                const auto& value = result.joints[i];
                if (!assembly_ids.contains(value.assembly_id))
                    fail(ContentErrorCode::MissingReference, indexed("/joints", i) + "/assembly_id",
                         "assembly reference not found");
                if (!body_indexes.contains(value.body_a_id))
                    fail(ContentErrorCode::MissingReference, indexed("/joints", i) + "/body_a_id",
                         "body reference not found");
                if (!body_indexes.contains(value.body_b_id))
                    fail(ContentErrorCode::MissingReference, indexed("/joints", i) + "/body_b_id",
                         "body reference not found");
            }
            if (owned_bodies.size() != result.bodies.size())
                fail(ContentErrorCode::InvalidInvariant, "/bodies",
                     "every body must have exactly one ownership");
            if (owned_joints.size() != result.joints.size())
                fail(ContentErrorCode::InvalidInvariant, "/joints",
                     "every joint must have exactly one ownership");
            std::unordered_set<std::uint32_t> trigger_ids;
            const auto& triggers = member(root, "triggers", "");
            array(triggers, "/triggers", kMaxTriggers);
            for (std::size_t i = 0; i < triggers.size(); ++i)
            {
                const auto pointer = indexed("/triggers", i);
                const auto& item = triggers[i];
                keys(item, pointer,
                     {"id", "target_entity_id", "kind", "damage_threshold", "fuse_ticks",
                      "cooldown_ticks", "pressure_burst"});
                EnvironmentalTriggerDefinition value;
                value.id = uint(item, "id", pointer);
                unique_id(trigger_ids, value.id, child(pointer, "id"));
                value.target_entity_id = id<EntityId>(item, "target_entity_id", pointer);
                if (text(item, "kind", pointer) != "damage_threshold")
                    fail(ContentErrorCode::InvalidEnum, child(pointer, "kind"),
                         "only damage_threshold triggers are supported");
                value.damage_threshold =
                    number(item, "damage_threshold", pointer, 0.0, 1000000.0, false);
                value.fuse_ticks = uint(item, "fuse_ticks", pointer, 0U, 36000U);
                value.cooldown_ticks = uint(item, "cooldown_ticks", pointer, 0U, 36000U);
                const auto burst_pointer = child(pointer, "pressure_burst");
                const auto& burst = member(item, "pressure_burst", pointer);
                keys(burst, burst_pointer,
                     {"radius_m", "impulse_n_s", "energy_j", "line_of_sight", "max_bodies"});
                value.pressure_burst.radius_m =
                    number(burst, "radius_m", burst_pointer, 0.0, 1000.0, false);
                value.pressure_burst.impulse_n_s =
                    number(burst, "impulse_n_s", burst_pointer, 0.0, 1e9, false);
                value.pressure_burst.energy_j =
                    number(burst, "energy_j", burst_pointer, 0.0, 1e12, false);
                value.pressure_burst.line_of_sight = boolean(burst, "line_of_sight", burst_pointer);
                value.pressure_burst.max_bodies =
                    uint(burst, "max_bodies", burst_pointer, 1U, 500U);
                if (!entity_ids.contains(value.target_entity_id.value()))
                    fail(ContentErrorCode::MissingReference, child(pointer, "target_entity_id"),
                         "trigger target not found");
                result.triggers.push_back(value);
            }
            std::unordered_set<std::uint32_t> objective_ids;
            const auto& objectives = member(root, "objectives", "");
            array(objectives, "/objectives", 64U, true);
            for (std::size_t i = 0; i < objectives.size(); ++i)
            {
                const auto pointer = indexed("/objectives", i);
                const auto& item = objectives[i];
                keys(item, pointer, {"id", "kind", "target_entity_id"});
                ObjectiveDefinition value;
                value.id = uint(item, "id", pointer);
                unique_id(objective_ids, value.id, child(pointer, "id"));
                if (text(item, "kind", pointer) != "neutralize_entity")
                    fail(ContentErrorCode::InvalidEnum, child(pointer, "kind"),
                         "invalid objective kind");
                value.kind = ObjectiveKind::NeutralizeEntity;
                value.target_entity_id = id<EntityId>(item, "target_entity_id", pointer);
                if (!entity_ids.contains(value.target_entity_id.value()))
                    fail(ContentErrorCode::MissingReference, child(pointer, "target_entity_id"),
                         "objective target not found");
                result.objectives.push_back(value);
            }
            const auto& settle = member(root, "settle_policy", "");
            keys(settle, "/settle_policy",
                 {"linear_speed_m_s", "angular_speed_rad_s", "rest_ticks"});
            result.settle_policy.linear_speed_m_s =
                number(settle, "linear_speed_m_s", "/settle_policy", 0.0, 100.0, false);
            result.settle_policy.angular_speed_rad_s =
                number(settle, "angular_speed_rad_s", "/settle_policy", 0.0, 100.0, false);
            result.settle_policy.rest_ticks =
                uint(settle, "rest_ticks", "/settle_policy", 1U, 3600U);
            result.watchdog_ticks = uint(root, "watchdog_ticks", "", 1U, 36000U);
            if (result.watchdog_ticks <= result.settle_policy.rest_ticks)
                fail(ContentErrorCode::InvalidInvariant, "/watchdog_ticks",
                     "watchdog must exceed settle window");
            return result;
        });
}

ContentResult<CampaignManifest> parse_campaign_manifest(std::string_view input) noexcept
{
    return boundary<CampaignManifest>(
        [&]
        {
            const auto root = parse_json(input, kCatalogMaxBytes);
            object(root, "");
            schema_v2(root);
            keys(root, "",
                 {"schema_version", "default_world_id", "world_order", "scene_ids", "diorama_ids",
                  "text_ids", "worlds"});
            CampaignManifest result;
            result.schema_version = result.source_schema_version = 2U;
            result.default_world_id = text(root, "default_world_id", "");
            result.world_order = string_set(root, "world_order", 16U);
            result.scene_ids = string_set(root, "scene_ids", 128U);
            result.diorama_ids = string_set(root, "diorama_ids", 32U);
            result.text_ids = string_set(root, "text_ids", 64U);
            const std::unordered_set<std::string> scenes(result.scene_ids.begin(),
                                                         result.scene_ids.end()),
                dioramas(result.diorama_ids.begin(), result.diorama_ids.end()),
                texts(result.text_ids.begin(), result.text_ids.end());
            const auto& worlds = member(root, "worlds", "");
            array(worlds, "/worlds", 16U, true);
            if (worlds.size() != result.world_order.size())
                fail(ContentErrorCode::InvalidInvariant, "/world_order",
                     "world order must enumerate worlds");
            std::unordered_set<std::string> world_ids;
            for (std::size_t i = 0; i < worlds.size(); ++i)
            {
                const auto pointer = indexed("/worlds", i);
                const auto& item = worlds[i];
                keys(item, pointer,
                     {"id", "diorama_id", "text_id", "default_level_id", "level_order", "levels"});
                CampaignWorldDefinition world;
                world.id = text(item, "id", pointer);
                if (!world_ids.insert(world.id).second)
                    fail(ContentErrorCode::DuplicateId, child(pointer, "id"), "duplicate world id");
                if (result.world_order[i] != world.id)
                    fail(ContentErrorCode::InvalidInvariant, indexed("/world_order", i),
                         "world order differs from worlds");
                world.diorama_id = text(item, "diorama_id", pointer);
                if (!dioramas.contains(world.diorama_id))
                    fail(ContentErrorCode::MissingReference, child(pointer, "diorama_id"),
                         "diorama reference not found");
                world.text_id = text(item, "text_id", pointer);
                if (!texts.contains(world.text_id))
                    fail(ContentErrorCode::MissingReference, child(pointer, "text_id"),
                         "text reference not found");
                world.default_level_id = text(item, "default_level_id", pointer);
                const auto& order = member(item, "level_order", pointer);
                array(order, child(pointer, "level_order"), 64U, true);
                std::unordered_set<std::string> ordered;
                for (std::size_t j = 0; j < order.size(); ++j)
                {
                    if (!order[j].is_string())
                        fail(ContentErrorCode::InvalidType,
                             indexed(child(pointer, "level_order"), j), "expected string");
                    auto value = order[j].get<std::string>();
                    if (value.empty() || !ordered.insert(value).second)
                        fail(ContentErrorCode::DuplicateId,
                             indexed(child(pointer, "level_order"), j), "invalid level order");
                    world.level_order.push_back(std::move(value));
                }
                const auto& levels = member(item, "levels", pointer);
                array(levels, child(pointer, "levels"), 64U, true);
                if (levels.size() != world.level_order.size())
                    fail(ContentErrorCode::InvalidInvariant, child(pointer, "level_order"),
                         "level order must enumerate levels");
                std::unordered_set<std::string> level_ids;
                for (std::size_t j = 0; j < levels.size(); ++j)
                {
                    const auto level_pointer = indexed(child(pointer, "levels"), j);
                    const auto& level_item = levels[j];
                    keys(level_item, level_pointer,
                         {"id", "region_id", "camera_profile_id", "presentation_profile_id",
                          "scene_id", "unlock_after_level_id"});
                    CampaignLevelDefinition level;
                    level.id = text(level_item, "id", level_pointer);
                    if (!level_ids.insert(level.id).second)
                        fail(ContentErrorCode::DuplicateId, child(level_pointer, "id"),
                             "duplicate level id");
                    if (world.level_order[j] != level.id)
                        fail(ContentErrorCode::InvalidInvariant,
                             indexed(child(pointer, "level_order"), j),
                             "level order differs from levels");
                    level.region_id = text(level_item, "region_id", level_pointer);
                    level.camera_profile_id = text(level_item, "camera_profile_id", level_pointer);
                    level.presentation_profile_id =
                        text(level_item, "presentation_profile_id", level_pointer);
                    level.scene_id = text(level_item, "scene_id", level_pointer);
                    if (!scenes.contains(level.scene_id))
                        fail(ContentErrorCode::MissingReference, child(level_pointer, "scene_id"),
                             "scene reference not found");
                    const auto& unlock = member(level_item, "unlock_after_level_id", level_pointer);
                    if (unlock.is_null())
                    {
                        if (j != 0U)
                            fail(ContentErrorCode::InvalidInvariant,
                                 child(level_pointer, "unlock_after_level_id"),
                                 "only first level may be initially unlocked");
                    }
                    else
                    {
                        if (!unlock.is_string())
                            fail(ContentErrorCode::InvalidType,
                                 child(level_pointer, "unlock_after_level_id"),
                                 "expected string or null");
                        level.unlock_after_level_id = unlock.get<std::string>();
                        if (j == 0U || *level.unlock_after_level_id != world.level_order[j - 1U])
                            fail(ContentErrorCode::MissingReference,
                                 child(level_pointer, "unlock_after_level_id"),
                                 "unlock must reference previous level");
                    }
                    world.levels.push_back(std::move(level));
                }
                if (!level_ids.contains(world.default_level_id))
                    fail(ContentErrorCode::MissingReference, child(pointer, "default_level_id"),
                         "default level not found");
                result.worlds.push_back(std::move(world));
            }
            if (result.world_order.size() != 2U || result.world_order[0] != "earth" ||
                result.world_order[1] != "orbital")
                fail(ContentErrorCode::InvalidInvariant, "/world_order/0",
                     "campaign must order earth before orbital");
            if (result.default_world_id != "earth" || !world_ids.contains(result.default_world_id))
                fail(ContentErrorCode::MissingReference, "/default_world_id",
                     "default world not found");
            return result;
        });
}

ContentResult<CampaignManifest> parse_campaign_manifest_v2(std::string_view input) noexcept
{
    return parse_campaign_manifest(input);
}

ContentResult<ProductV2ContentBundle>
make_product_v2_content_bundle(const MaterialCatalog& materials, const ArchetypeCatalog& archetypes,
                               const CampaignManifest& campaign,
                               const LevelManifest& level) noexcept
{
    return boundary<ProductV2ContentBundle>(
        [&]
        {
            if (materials.source_schema_version != 2U || archetypes.source_schema_version != 2U ||
                campaign.source_schema_version != 2U || level.source_schema_version != 2U)
                fail(ContentErrorCode::InvalidInvariant, "/schema_version",
                     "product v2 bundle requires only schema version 2");
            std::unordered_set<std::uint32_t> material_ids, surface_ids, bird_ids, enemy_ids;
            for (const auto& value : materials.materials)
                material_ids.insert(value.id.value());
            for (const auto& value : materials.surfaces)
                surface_ids.insert(value.id.value());
            for (const auto& value : archetypes.birds)
                bird_ids.insert(value.id.value());
            for (const auto& value : archetypes.enemies)
                enemy_ids.insert(value.id.value());
            for (std::size_t i = 0; i < archetypes.birds.size(); ++i)
                if (!surface_ids.contains(archetypes.birds[i].surface_id.value()))
                    fail(ContentErrorCode::MissingReference, indexed("/birds", i) + "/surface_id",
                         "surface reference not found");
            for (std::size_t i = 0; i < archetypes.enemies.size(); ++i)
                if (!surface_ids.contains(archetypes.enemies[i].surface_id.value()))
                    fail(ContentErrorCode::MissingReference, indexed("/enemies", i) + "/surface_id",
                         "surface reference not found");
            for (std::size_t i = 0; i < level.bird_queue.size(); ++i)
                if (!bird_ids.contains(level.bird_queue[i].value()))
                    fail(ContentErrorCode::MissingReference, indexed("/bird_queue", i),
                         "bird reference not found");
            for (std::size_t i = 0; i < level.bodies.size(); ++i)
            {
                const auto& body = level.bodies[i];
                if (body.material_id && !material_ids.contains(body.material_id->value()))
                    fail(ContentErrorCode::MissingReference, indexed("/bodies", i) + "/material_id",
                         "material reference not found");
                if (body.surface_id && !surface_ids.contains(body.surface_id->value()))
                    fail(ContentErrorCode::MissingReference, indexed("/bodies", i) + "/surface_id",
                         "surface reference not found");
                if (body.enemy_archetype_id &&
                    !enemy_ids.contains(body.enemy_archetype_id->value()))
                    fail(ContentErrorCode::MissingReference,
                         indexed("/bodies", i) + "/enemy_archetype_id",
                         "enemy reference not found");
            }
            const CampaignWorldDefinition* world = nullptr;
            for (const auto& value : campaign.worlds)
                if (value.id == level.world_id)
                {
                    world = &value;
                    break;
                }
            if (!world)
                fail(ContentErrorCode::MissingReference, "/world_id", "world reference not found");
            const CampaignLevelDefinition* registered = nullptr;
            for (const auto& value : world->levels)
                if (value.id == level.id)
                {
                    registered = &value;
                    break;
                }
            if (!registered)
                fail(ContentErrorCode::MissingReference, "/id", "level reference not found");
            if (registered->region_id != level.region_id)
                fail(ContentErrorCode::MissingReference, "/region_id",
                     "region reference not found");
            if (registered->camera_profile_id != level.camera_profile_id)
                fail(ContentErrorCode::MissingReference, "/camera_profile_id",
                     "camera profile reference not found");
            if (registered->presentation_profile_id != level.presentation_profile_id)
                fail(ContentErrorCode::MissingReference, "/presentation_profile_id",
                     "presentation profile reference not found");
            return ProductV2ContentBundle{materials, archetypes, campaign, level};
        });
}

namespace detail
{

std::string to_canonical_json_v2(const MaterialCatalog& catalog)
{
    json root{{"schema_version", 2}, {"materials", json::array()}, {"surfaces", json::array()}};
    for (const auto& item : catalog.materials)
        root["materials"].push_back({{"id", item.id.value()},
                                     {"key", item.key},
                                     {"response", response_name(item.response)},
                                     {"density_kg_m3", item.density_kg_m3},
                                     {"friction", item.friction},
                                     {"restitution", item.restitution},
                                     {"toughness", item.toughness}});
    for (const auto& item : catalog.surfaces)
        root["surfaces"].push_back({{"id", item.id.value()},
                                    {"key", item.key},
                                    {"density_kg_m3", item.density_kg_m3},
                                    {"friction", item.friction},
                                    {"restitution", item.restitution}});
    return root.dump();
}

std::string to_canonical_json_v2(const ArchetypeCatalog& catalog)
{
    json root{{"schema_version", 2},
              {"presentation_ids", catalog.presentation_ids},
              {"score_ids", catalog.score_ids},
              {"abilities", json::array()},
              {"birds", json::array()},
              {"weakpoints", json::array()},
              {"enemies", json::array()}};
    for (const auto& item : catalog.abilities)
        root["abilities"].push_back({{"id", item.id.value()},
                                     {"key", item.key},
                                     {"kind", item.kind},
                                     {"payload", ability_payload_json(item)}});
    for (const auto& item : catalog.birds)
        root["birds"].push_back({{"id", item.id.value()},
                                 {"key", item.key},
                                 {"projectile_visual_id", item.projectile_visual_id},
                                 {"ability_id", item.ability_id.value()},
                                 {"surface_id", item.surface_id.value()},
                                 {"mass_kg", item.mass_kg},
                                 {"radius_m", item.radius_m},
                                 {"friction", item.friction},
                                 {"restitution", item.restitution},
                                 {"bullet", item.bullet},
                                 {"launch_speed_cap_m_s", item.launch_speed_cap_m_s},
                                 {"score_id", item.score_id},
                                 {"icon_id", item.icon_id},
                                 {"animation_id", item.animation_id}});
    for (const auto& item : catalog.weakpoints)
        root["weakpoints"].push_back({{"id", item.id.value()},
                                      {"key", item.key},
                                      {"protected_direction", item.protected_direction},
                                      {"protected_cone_deg", item.protected_cone_deg},
                                      {"protected_multiplier", item.protected_multiplier},
                                      {"exposed_multiplier", item.exposed_multiplier}});
    for (const auto& item : catalog.enemies)
        root["enemies"].push_back({{"id", item.id.value()},
                                   {"key", item.key},
                                   {"weakpoint_id", item.weakpoint_id.value()},
                                   {"surface_id", item.surface_id.value()},
                                   {"mass_kg", item.mass_kg},
                                   {"integrity", item.integrity},
                                   {"damage_energy_j_per_kg", item.damage_energy_j_per_kg},
                                   {"max_damage", item.max_damage}});
    return root.dump();
}

std::string to_canonical_json_v2(const LevelManifest& level)
{
    json world;
    if (const auto* value = std::get_if<UniformWorldDefinition>(&level.world))
        world = {{"kind", "uniform"},
                 {"acceleration_m_s2", value->acceleration_m_s2},
                 {"bounds", {{"min_m", value->bounds_min_m}, {"max_m", value->bounds_max_m}}}};
    else
    {
        const auto& radial = std::get<RadialWorldDefinition>(level.world);
        world = {{"kind", "radial"},
                 {"center_m", radial.center_m},
                 {"reference_radius_m", radial.reference_radius_m},
                 {"reference_acceleration_m_s2", radial.reference_acceleration_m_s2},
                 {"bounds", {{"radius_m", radial.bounds_radius_m}}}};
    }
    json queue = json::array();
    for (const auto value : level.bird_queue)
        queue.push_back(value.value());
    json root{{"schema_version", 2},
              {"id", level.id},
              {"world_id", level.world_id},
              {"region_id", level.region_id},
              {"camera_profile_id", level.camera_profile_id},
              {"presentation_profile_id", level.presentation_profile_id},
              {"world", std::move(world)},
              {"slingshot",
               {{"asset_id", level.slingshot.asset_id},
                {"rest_position_m", level.slingshot.rest_position_m},
                {"rest_rotation_xyzw", level.slingshot.rest_rotation_xyzw},
                {"spring_constant_n_m", level.slingshot.spring_constant_n_m},
                {"energy_efficiency", level.slingshot.energy_efficiency},
                {"minimum_extension_m", level.slingshot.minimum_extension_m},
                {"maximum_extension_m", level.slingshot.maximum_extension_m},
                {"plane_policy", level.slingshot.plane_policy},
                {"projectile_clearance_m", level.slingshot.projectile_clearance_m},
                {"speed_ceiling_m_s", level.slingshot.speed_ceiling_m_s}}},
              {"bird_queue", std::move(queue)},
              {"scoring",
               {{"pig_points", level.scoring.pig_points},
                {"unused_bird_points", level.scoring.unused_bird_points},
                {"star_thresholds", level.scoring.star_thresholds},
                {"chain_window_ticks", level.scoring.chain_window_ticks},
                {"chain_multiplier_step", level.scoring.chain_multiplier_step},
                {"max_chain_multiplier", level.scoring.max_chain_multiplier}}},
              {"free_body_ids", level.free_body_ids},
              {"bodies", json::array()},
              {"joints", json::array()},
              {"assemblies", json::array()},
              {"triggers", json::array()},
              {"objectives", json::array()},
              {"settle_policy",
               {{"linear_speed_m_s", level.settle_policy.linear_speed_m_s},
                {"angular_speed_rad_s", level.settle_policy.angular_speed_rad_s},
                {"rest_ticks", level.settle_policy.rest_ticks}}},
              {"watchdog_ticks", level.watchdog_ticks}};
    for (const auto& item : level.bodies)
        root["bodies"].push_back(
            {{"body_id", item.body_id},
             {"entity_id", item.entity_id.value()},
             {"part_id", item.part_id.value()},
             {"body_type", item.body_type == BodyType::Static ? "static" : "dynamic"},
             {"affected_by_world_gravity", item.affected_by_world_gravity},
             {"material_id", item.material_id ? json(item.material_id->value()) : json(nullptr)},
             {"surface_id", item.surface_id ? json(item.surface_id->value()) : json(nullptr)},
             {"enemy_archetype_id",
              item.enemy_archetype_id ? json(item.enemy_archetype_id->value()) : json(nullptr)},
             {"density_kg_m3", item.density_kg_m3},
             {"transform",
              {{"position_m", item.transform.position_m},
               {"rotation_xyzw", item.transform.rotation_xyzw}}},
             {"shape", shape_json(item.shape)},
             {"visual", {{"asset_id", item.visual.asset_id}, {"bounds_m", item.visual.bounds_m}}}});
    for (const auto& item : level.joints)
        root["joints"].push_back({{"id", item.id.value()},
                                  {"assembly_id", item.assembly_id},
                                  {"kind", joint_name(item.kind)},
                                  {"body_a_id", item.body_a_id},
                                  {"body_b_id", item.body_b_id},
                                  {"force_limit_n", item.force_limit_n},
                                  {"torque_limit_nm", item.torque_limit_nm}});
    for (const auto& item : level.assemblies)
    {
        json joint_ids = json::array();
        for (const auto value : item.joint_ids)
            joint_ids.push_back(value.value());
        root["assemblies"].push_back({{"id", item.id},
                                      {"key", item.key},
                                      {"body_ids", item.body_ids},
                                      {"joint_ids", std::move(joint_ids)}});
    }
    for (const auto& item : level.triggers)
        root["triggers"].push_back({{"id", item.id},
                                    {"target_entity_id", item.target_entity_id.value()},
                                    {"kind", "damage_threshold"},
                                    {"damage_threshold", item.damage_threshold},
                                    {"fuse_ticks", item.fuse_ticks},
                                    {"cooldown_ticks", item.cooldown_ticks},
                                    {"pressure_burst",
                                     {{"radius_m", item.pressure_burst.radius_m},
                                      {"impulse_n_s", item.pressure_burst.impulse_n_s},
                                      {"energy_j", item.pressure_burst.energy_j},
                                      {"line_of_sight", item.pressure_burst.line_of_sight},
                                      {"max_bodies", item.pressure_burst.max_bodies}}}});
    for (const auto& item : level.objectives)
        root["objectives"].push_back({{"id", item.id},
                                      {"kind", "neutralize_entity"},
                                      {"target_entity_id", item.target_entity_id.value()}});
    return root.dump();
}

} // namespace detail

std::string to_canonical_json(const CampaignManifest& campaign)
{
    json root{{"schema_version", 2},
              {"default_world_id", campaign.default_world_id},
              {"world_order", campaign.world_order},
              {"scene_ids", campaign.scene_ids},
              {"diorama_ids", campaign.diorama_ids},
              {"text_ids", campaign.text_ids},
              {"worlds", json::array()}};
    for (const auto& world : campaign.worlds)
    {
        json levels = json::array();
        for (const auto& level : world.levels)
            levels.push_back({{"id", level.id},
                              {"region_id", level.region_id},
                              {"camera_profile_id", level.camera_profile_id},
                              {"presentation_profile_id", level.presentation_profile_id},
                              {"scene_id", level.scene_id},
                              {"unlock_after_level_id", level.unlock_after_level_id
                                                            ? json(*level.unlock_after_level_id)
                                                            : json(nullptr)}});
        root["worlds"].push_back({{"id", world.id},
                                  {"diorama_id", world.diorama_id},
                                  {"text_id", world.text_id},
                                  {"default_level_id", world.default_level_id},
                                  {"level_order", world.level_order},
                                  {"levels", std::move(levels)}});
    }
    return root.dump();
}

} // namespace ninho::simulation
