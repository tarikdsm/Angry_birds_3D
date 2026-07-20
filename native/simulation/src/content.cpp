#include "ninho/simulation/content.hpp"

#include "content_semantic_validation.hpp"

#include <ninho/physics/physics_limits.hpp>

#include <nlohmann/json.hpp>

#include <algorithm>
#include <cmath>
#include <limits>
#include <numbers>
#include <queue>
#include <set>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>

static_assert(NLOHMANN_JSON_VERSION_MAJOR == 3);
static_assert(NLOHMANN_JSON_VERSION_MINOR == 11);
static_assert(NLOHMANN_JSON_VERSION_PATCH == 3);

namespace ninho::simulation {
namespace detail {
std::string to_canonical_json_v2(const MaterialCatalog&);
std::string to_canonical_json_v2(const ArchetypeCatalog&);
std::string to_canonical_json_v2(const LevelManifest&);
}
namespace {

using json = nlohmann::json;

constexpr std::size_t kCatalogMaxBytes = 256U * 1024U;
constexpr std::size_t kLevelMaxBytes = 1024U * 1024U;
constexpr std::size_t kMaxNesting = 16U;
constexpr std::uint32_t kRuntimeEntityBit = 0x80000000U;

struct ParseFailure final : std::exception {
    ContentError error;
    explicit ParseFailure(ContentError value) : error(std::move(value)) {}
};

[[noreturn]] void fail(ContentErrorCode code, std::string pointer, std::string message)
{
    throw ParseFailure{{code, std::move(pointer), std::move(message)}};
}

void require_content_entity_id(EntityId id, std::string pointer)
{
    if ((id.value() & kRuntimeEntityBit) != 0U) {
        fail(ContentErrorCode::OutOfRange, std::move(pointer),
            "entity id uses the runtime-reserved high-bit namespace");
    }
}

std::string child(const std::string& pointer, std::string_view token)
{
    std::string escaped;
    escaped.reserve(token.size());
    for (const char character : token) {
        if (character == '~') {
            escaped += "~0";
        } else if (character == '/') {
            escaped += "~1";
        } else {
            escaped += character;
        }
    }
    return pointer + '/' + escaped;
}

std::string indexed(const std::string& pointer, std::size_t index)
{
    return pointer + '/' + std::to_string(index);
}

struct JsonReadGuard {
    enum class Kind { Object, Array };
    struct Context {
        Kind kind{};
        std::string pointer;
        std::unordered_set<std::string> keys;
        std::string pending_key;
        std::size_t next_index{};
    };

    std::vector<Context> stack;

    std::string consume_value_pointer()
    {
        if (stack.empty()) return "";
        auto& parent = stack.back();
        if (parent.kind == Kind::Array) {
            return indexed(parent.pointer, parent.next_index++);
        }
        const auto result = child(parent.pointer, parent.pending_key);
        parent.pending_key.clear();
        return result;
    }

    bool on_event(int, json::parse_event_t event, json& parsed)
    {
        switch (event) {
        case json::parse_event_t::object_start:
        case json::parse_event_t::array_start: {
            auto pointer = consume_value_pointer();
            if (stack.size() >= kMaxNesting) {
                fail(ContentErrorCode::ResourceLimit, std::move(pointer),
                    "JSON nesting limit exceeded");
            }
            stack.push_back({
                event == json::parse_event_t::object_start ? Kind::Object : Kind::Array,
                std::move(pointer), {}, {}, 0U});
            break;
        }
        case json::parse_event_t::key: {
            auto& object = stack.back();
            const auto key = parsed.get<std::string>();
            if (!object.keys.insert(key).second) {
                fail(ContentErrorCode::DuplicateKey, child(object.pointer, key),
                    "duplicate JSON key");
            }
            object.pending_key = key;
            break;
        }
        case json::parse_event_t::value:
            static_cast<void>(consume_value_pointer());
            break;
        case json::parse_event_t::object_end:
        case json::parse_event_t::array_end:
            stack.pop_back();
            break;
        }
        return true;
    }
};

void require_object(const json& value, const std::string& pointer)
{
    if (!value.is_object()) {
        fail(ContentErrorCode::InvalidType, pointer, "expected object");
    }
}

void require_array(const json& value, const std::string& pointer)
{
    if (!value.is_array()) {
        fail(ContentErrorCode::InvalidType, pointer, "expected array");
    }
}

void require_collection(const json& value, const std::string& pointer,
    std::size_t maximum, bool require_non_empty = false)
{
    require_array(value, pointer);
    if (value.size() > maximum) {
        fail(ContentErrorCode::ResourceLimit, pointer, "collection limit exceeded");
    }
    if (require_non_empty && value.empty()) {
        fail(ContentErrorCode::OutOfRange, pointer, "collection cannot be empty");
    }
}

void require_keys(const json& object, const std::string& pointer,
    std::initializer_list<std::string_view> allowed)
{
    require_object(object, pointer);
    std::set<std::string, std::less<>> allowed_set;
    for (const auto key : allowed) {
        allowed_set.emplace(key);
    }
    for (auto it = object.begin(); it != object.end(); ++it) {
        if (!allowed_set.contains(it.key())) {
            fail(ContentErrorCode::UnknownKey, child(pointer, it.key()), "unknown key");
        }
    }
    for (const auto key : allowed) {
        if (!object.contains(key)) {
            fail(ContentErrorCode::MissingField, child(pointer, key), "missing required field");
        }
    }
}

const json& member(const json& object, std::string_view key, const std::string& pointer)
{
    if (!object.contains(key)) {
        fail(ContentErrorCode::MissingField, child(pointer, key), "missing required field");
    }
    return object.at(key);
}

std::string read_string(const json& object, std::string_view key, const std::string& pointer)
{
    const auto& value = member(object, key, pointer);
    if (!value.is_string()) {
        fail(ContentErrorCode::InvalidType, child(pointer, key), "expected string");
    }
    auto result = value.get<std::string>();
    if (result.empty() || result.size() > 128U) {
        fail(ContentErrorCode::OutOfRange, child(pointer, key), "string length out of range");
    }
    return result;
}

bool read_bool(const json& object, std::string_view key, const std::string& pointer)
{
    const auto& value = member(object, key, pointer);
    if (!value.is_boolean()) {
        fail(ContentErrorCode::InvalidType, child(pointer, key), "expected boolean");
    }
    return value.get<bool>();
}

double read_number(const json& object, std::string_view key, const std::string& pointer,
    double minimum, double maximum, bool minimum_inclusive = true)
{
    const auto& value = member(object, key, pointer);
    if (!value.is_number()) {
        fail(ContentErrorCode::InvalidType, child(pointer, key), "expected number");
    }
    double result{};
    try {
        result = value.get<double>();
    } catch (...) {
        fail(ContentErrorCode::InvalidNumber, child(pointer, key), "number is not representable");
    }
    if (!std::isfinite(result)) {
        fail(ContentErrorCode::InvalidNumber, child(pointer, key), "number must be finite");
    }
    const bool below = minimum_inclusive ? result < minimum : result <= minimum;
    if (below || result > maximum) {
        fail(ContentErrorCode::OutOfRange, child(pointer, key), "number out of range");
    }
    if (result > 0.0) {
        const auto runtime_value = static_cast<float>(result);
        if (!std::isfinite(runtime_value) || runtime_value <= 0.0F) {
            fail(ContentErrorCode::OutOfRange, child(pointer, key),
                "positive value is not representable by the runtime");
        }
    }
    return result;
}

std::uint32_t read_uint_value(const json& value, const std::string& pointer,
    std::uint32_t minimum = 1U, std::uint32_t maximum = std::numeric_limits<std::uint32_t>::max())
{
    if (!value.is_number_unsigned() && !value.is_number_integer()) {
        fail(ContentErrorCode::InvalidType, pointer, "expected unsigned integer");
    }
    std::int64_t signed_value{};
    try {
        signed_value = value.get<std::int64_t>();
    } catch (...) {
        fail(ContentErrorCode::OutOfRange, pointer, "integer out of range");
    }
    if (signed_value < static_cast<std::int64_t>(minimum)
        || static_cast<std::uint64_t>(signed_value) > maximum) {
        fail(ContentErrorCode::OutOfRange, pointer, "integer out of range");
    }
    return static_cast<std::uint32_t>(signed_value);
}

std::uint32_t read_uint(const json& object, std::string_view key, const std::string& pointer,
    std::uint32_t minimum = 1U, std::uint32_t maximum = std::numeric_limits<std::uint32_t>::max())
{
    return read_uint_value(member(object, key, pointer), child(pointer, key), minimum, maximum);
}

template <std::size_t N>
std::array<double, N> read_vector(const json& object, std::string_view key,
    const std::string& pointer, double minimum, double maximum)
{
    const auto value_pointer = child(pointer, key);
    const auto& value = member(object, key, pointer);
    require_array(value, value_pointer);
    if (value.size() != N) {
        fail(ContentErrorCode::OutOfRange, value_pointer, "vector has wrong length");
    }
    std::array<double, N> result{};
    for (std::size_t i = 0; i < N; ++i) {
        const auto& component = value.at(i);
        if (!component.is_number()) {
            fail(ContentErrorCode::InvalidType, indexed(value_pointer, i), "expected number");
        }
        try {
            result[i] = component.get<double>();
        } catch (...) {
            fail(ContentErrorCode::InvalidNumber, indexed(value_pointer, i), "number is not representable");
        }
        if (!std::isfinite(result[i])) {
            fail(ContentErrorCode::InvalidNumber, indexed(value_pointer, i), "number must be finite");
        }
        if (result[i] < minimum || result[i] > maximum) {
            fail(ContentErrorCode::OutOfRange, indexed(value_pointer, i), "number out of range");
        }
    }
    return result;
}

template <typename Id>
Id read_id(const json& object, std::string_view key, const std::string& pointer)
{
    return Id{read_uint(object, key, pointer)};
}

template <typename Id>
std::optional<Id> read_nullable_id(const json& object, std::string_view key, const std::string& pointer)
{
    const auto& value = member(object, key, pointer);
    if (value.is_null()) {
        return std::nullopt;
    }
    if (!value.is_number_unsigned() && !value.is_number_integer()) {
        fail(ContentErrorCode::InvalidType, child(pointer, key), "expected id or null");
    }
    std::int64_t parsed{};
    try {
        parsed = value.get<std::int64_t>();
    } catch (...) {
        fail(ContentErrorCode::OutOfRange, child(pointer, key), "id out of range");
    }
    if (parsed <= 0 || static_cast<std::uint64_t>(parsed) > std::numeric_limits<std::uint32_t>::max()) {
        fail(ContentErrorCode::OutOfRange, child(pointer, key), "id out of range");
    }
    return Id{static_cast<std::uint32_t>(parsed)};
}

template <typename Id>
void reject_duplicate(std::unordered_set<std::uint32_t>& ids, Id id, const std::string& pointer)
{
    if (!ids.insert(id.value()).second) {
        fail(ContentErrorCode::DuplicateId, pointer, "duplicate id");
    }
}

MaterialResponse parse_material_response(std::string_view value, const std::string& pointer)
{
    if (value == "fibrous") return MaterialResponse::Fibrous;
    if (value == "masonry") return MaterialResponse::Masonry;
    if (value == "brittle") return MaterialResponse::Brittle;
    fail(ContentErrorCode::InvalidEnum, pointer, "invalid material response");
}

BodyType parse_body_type(std::string_view value, const std::string& pointer)
{
    if (value == "static") return BodyType::Static;
    if (value == "dynamic") return BodyType::Dynamic;
    fail(ContentErrorCode::InvalidEnum, pointer, "invalid body type");
}

JointKind parse_joint_kind(std::string_view value, const std::string& pointer)
{
    if (value == "pine_fit") return JointKind::PineFit;
    if (value == "glass_clamp") return JointKind::GlassClamp;
    if (value == "mortar") return JointKind::Mortar;
    fail(ContentErrorCode::InvalidEnum, pointer, "invalid joint kind");
}

ObjectiveKind parse_objective_kind(std::string_view value, const std::string& pointer)
{
    if (value == "neutralize_entity") return ObjectiveKind::NeutralizeEntity;
    fail(ContentErrorCode::InvalidEnum, pointer, "invalid objective kind");
}

json parse_json(std::string_view text, std::size_t byte_limit)
{
    if (text.size() > byte_limit) {
        fail(ContentErrorCode::ResourceLimit, "", "document byte limit exceeded");
    }
    try {
        JsonReadGuard guard;
        json::parser_callback_t callback = [&guard](int depth, json::parse_event_t event, json& parsed) {
            return guard.on_event(depth, event, parsed);
        };
        return json::parse(text.begin(), text.end(), std::move(callback), true, false);
    } catch (const json::out_of_range&) {
        fail(ContentErrorCode::InvalidNumber, "", "JSON number is not representable");
    } catch (const json::exception&) {
        fail(ContentErrorCode::InvalidJson, "", "malformed JSON document");
    }
}

MaterialCatalog parse_material_catalog_impl(std::string_view text)
{
    const auto root = parse_json(text, kCatalogMaxBytes);
    require_object(root, "");
    const auto source_schema_version = read_uint(root, "schema_version", "", 1U, 1U);
    require_keys(root, "", {"schema_version", "materials", "surfaces"});
    MaterialCatalog result;
    result.schema_version = result.source_schema_version = source_schema_version;

    const auto& materials = member(root, "materials", "");
    require_collection(materials, "/materials", 64U, true);
    result.materials.reserve(materials.size());
    std::unordered_set<std::uint32_t> material_ids;
    std::set<std::string> keys;
    for (std::size_t i = 0; i < materials.size(); ++i) {
        const auto pointer = indexed("/materials", i);
        const auto& item = materials.at(i);
        require_keys(item, pointer,
            {"id", "key", "response", "density_kg_m3", "friction", "restitution", "toughness"});
        MaterialDefinition definition;
        definition.id = read_id<MaterialId>(item, "id", pointer);
        reject_duplicate(material_ids, definition.id, child(pointer, "id"));
        definition.key = read_string(item, "key", pointer);
        if (!keys.insert(definition.key).second) {
            fail(ContentErrorCode::DuplicateId, child(pointer, "key"), "duplicate key");
        }
        definition.response = parse_material_response(
            read_string(item, "response", pointer), child(pointer, "response"));
        definition.density_kg_m3 = read_number(item, "density_kg_m3", pointer, 0.0, 30000.0, false);
        definition.friction = read_number(item, "friction", pointer, 0.0, 1.0);
        definition.restitution = read_number(item, "restitution", pointer, 0.0, 1.0);
        definition.toughness = read_number(item, "toughness", pointer, 0.0, 1.0, false);
        result.materials.push_back(std::move(definition));
    }

    const auto& surfaces = member(root, "surfaces", "");
    require_collection(surfaces, "/surfaces", 64U, true);
    result.surfaces.reserve(surfaces.size());
    std::unordered_set<std::uint32_t> surface_ids;
    for (std::size_t i = 0; i < surfaces.size(); ++i) {
        const auto pointer = indexed("/surfaces", i);
        const auto& item = surfaces.at(i);
        require_keys(item, pointer, {"id", "key", "density_kg_m3", "friction", "restitution"});
        PhysicsSurfaceDefinition definition;
        definition.id = read_id<SurfaceId>(item, "id", pointer);
        reject_duplicate(surface_ids, definition.id, child(pointer, "id"));
        if (definition.id.value() < 1000U) {
            fail(ContentErrorCode::OutOfRange, child(pointer, "id"), "surface id must be reserved");
        }
        definition.key = read_string(item, "key", pointer);
        if (!keys.insert(definition.key).second) {
            fail(ContentErrorCode::DuplicateId, child(pointer, "key"), "duplicate key");
        }
        definition.density_kg_m3 = read_number(item, "density_kg_m3", pointer, 0.0, 30000.0, false);
        definition.friction = read_number(item, "friction", pointer, 0.0, 1.0);
        definition.restitution = read_number(item, "restitution", pointer, 0.0, 1.0);
        result.surfaces.push_back(std::move(definition));
    }
    return result;
}

ArchetypeCatalog parse_archetype_catalog_impl(std::string_view text)
{
    const auto root = parse_json(text, kCatalogMaxBytes);
    require_object(root, "");
    const auto source_schema_version = read_uint(root, "schema_version", "", 1U, 1U);
    require_keys(root, "", {"schema_version", "abilities", "birds", "weakpoints", "enemies"});
    ArchetypeCatalog result;
    result.schema_version = result.source_schema_version = source_schema_version;

    const auto& abilities = member(root, "abilities", "");
    require_collection(abilities, "/abilities", 16U);
    result.abilities.reserve(abilities.size());
    std::unordered_set<std::uint32_t> ability_ids;
    for (std::size_t i = 0; i < abilities.size(); ++i) {
        const auto pointer = indexed("/abilities", i);
        const auto& item = abilities.at(i);
        require_keys(item, pointer, {"id", "key", "kind", "arm_ticks", "duration_ticks", "radius_m",
            "max_body_mass_kg", "max_bodies", "max_acceleration_m_s2", "pulse_speed_m_s"});
        AbilityArchetype value;
        value.id = read_id<AbilityId>(item, "id", pointer);
        reject_duplicate(ability_ids, value.id, child(pointer, "id"));
        value.key = read_string(item, "key", pointer);
        value.kind = read_string(item, "kind", pointer);
        if (value.kind != "gravity_field") {
            fail(ContentErrorCode::InvalidEnum, child(pointer, "kind"), "invalid ability kind");
        }
        value.arm_ticks = read_uint(item, "arm_ticks", pointer, 1U, 3600U);
        value.duration_ticks = read_uint(item, "duration_ticks", pointer, 1U, 3600U);
        value.radius_m = read_number(item, "radius_m", pointer, 0.0, 100.0, false);
        value.max_body_mass_kg = read_number(item, "max_body_mass_kg", pointer, 0.0, 100000.0, false);
        value.max_bodies = read_uint(item, "max_bodies", pointer, 1U, 500U);
        value.max_acceleration_m_s2 = read_number(item, "max_acceleration_m_s2", pointer, 0.0, 1000.0, false);
        value.pulse_speed_m_s = read_number(item, "pulse_speed_m_s", pointer, 0.0, 1000.0, false);
        result.abilities.push_back(std::move(value));
    }

    const auto& birds = member(root, "birds", "");
    require_collection(birds, "/birds", 16U);
    result.birds.reserve(birds.size());
    std::unordered_set<std::uint32_t> bird_ids;
    for (std::size_t i = 0; i < birds.size(); ++i) {
        const auto pointer = indexed("/birds", i);
        const auto& item = birds.at(i);
        require_keys(item, pointer, {"id", "key", "ability_id", "surface_id", "mass_kg", "density_kg_m3",
            "radius_m", "friction", "restitution", "bullet"});
        BirdArchetype value;
        value.id = read_id<BirdArchetypeId>(item, "id", pointer);
        reject_duplicate(bird_ids, value.id, child(pointer, "id"));
        value.key = read_string(item, "key", pointer);
        value.ability_id = read_id<AbilityId>(item, "ability_id", pointer);
        value.surface_id = read_id<SurfaceId>(item, "surface_id", pointer);
        value.mass_kg = read_number(item, "mass_kg", pointer, 0.0, 100000.0, false);
        value.density_kg_m3 = read_number(item, "density_kg_m3", pointer, 0.0, 30000.0, false);
        value.radius_m = read_number(item, "radius_m", pointer, 0.0, 100.0, false);
        value.friction = read_number(item, "friction", pointer, 0.0, 1.0);
        value.restitution = read_number(item, "restitution", pointer, 0.0, 1.0);
        value.bullet = read_bool(item, "bullet", pointer);
        result.birds.push_back(std::move(value));
    }

    const auto& weakpoints = member(root, "weakpoints", "");
    require_collection(weakpoints, "/weakpoints", 32U);
    result.weakpoints.reserve(weakpoints.size());
    std::unordered_set<std::uint32_t> weakpoint_ids;
    for (std::size_t i = 0; i < weakpoints.size(); ++i) {
        const auto pointer = indexed("/weakpoints", i);
        const auto& item = weakpoints.at(i);
        require_keys(item, pointer, {"id", "key", "protected_direction", "protected_cone_deg",
            "protected_multiplier", "exposed_multiplier"});
        WeakpointProfile value;
        value.id = read_id<WeakpointId>(item, "id", pointer);
        reject_duplicate(weakpoint_ids, value.id, child(pointer, "id"));
        value.key = read_string(item, "key", pointer);
        value.protected_direction = read_vector<3>(item, "protected_direction", pointer, -1.0, 1.0);
        const double length = std::sqrt(value.protected_direction[0] * value.protected_direction[0]
            + value.protected_direction[1] * value.protected_direction[1]
            + value.protected_direction[2] * value.protected_direction[2]);
        if (std::abs(length - 1.0) > 1e-6) {
            fail(ContentErrorCode::InvalidInvariant, child(pointer, "protected_direction"),
                "direction must be normalized");
        }
        value.protected_cone_deg = read_number(item, "protected_cone_deg", pointer, 0.0, 180.0, false);
        value.protected_multiplier = read_number(item, "protected_multiplier", pointer, 0.0, 10.0);
        value.exposed_multiplier = read_number(item, "exposed_multiplier", pointer, 0.0, 10.0);
        result.weakpoints.push_back(std::move(value));
    }

    const auto& enemies = member(root, "enemies", "");
    require_collection(enemies, "/enemies", 16U);
    result.enemies.reserve(enemies.size());
    std::unordered_set<std::uint32_t> enemy_ids;
    for (std::size_t i = 0; i < enemies.size(); ++i) {
        const auto pointer = indexed("/enemies", i);
        const auto& item = enemies.at(i);
        require_keys(item, pointer, {"id", "key", "weakpoint_id", "surface_id", "mass_kg", "integrity",
            "damage_energy_j_per_kg", "max_damage"});
        EnemyArchetype value;
        value.id = read_id<EnemyArchetypeId>(item, "id", pointer);
        reject_duplicate(enemy_ids, value.id, child(pointer, "id"));
        value.key = read_string(item, "key", pointer);
        value.weakpoint_id = read_id<WeakpointId>(item, "weakpoint_id", pointer);
        value.surface_id = read_id<SurfaceId>(item, "surface_id", pointer);
        value.mass_kg = read_number(item, "mass_kg", pointer, 0.0, 100000.0, false);
        value.integrity = read_number(item, "integrity", pointer, 0.0, 100000.0, false);
        value.damage_energy_j_per_kg = read_number(item, "damage_energy_j_per_kg", pointer, 0.0, 100000.0, false);
        value.max_damage = read_number(item, "max_damage", pointer, 0.0, value.integrity, false);
        result.enemies.push_back(std::move(value));
    }

    for (std::size_t i = 0; i < result.birds.size(); ++i) {
        if (!ability_ids.contains(result.birds[i].ability_id.value())) {
            fail(ContentErrorCode::MissingReference, indexed("/birds", i) + "/ability_id",
                "ability reference not found");
        }
    }
    for (std::size_t i = 0; i < result.enemies.size(); ++i) {
        if (!weakpoint_ids.contains(result.enemies[i].weakpoint_id.value())) {
            fail(ContentErrorCode::MissingReference, indexed("/enemies", i) + "/weakpoint_id",
                "weakpoint reference not found");
        }
    }
    return result;
}

ShapeDefinition parse_shape(const json& item, const std::string& pointer)
{
    require_object(item, pointer);
    const auto type = read_string(item, "type", pointer);
    ShapeDefinition value;
    if (type == "box") {
        require_keys(item, pointer, {"type", "half_extents_m"});
        value.type = ShapeType::Box;
        value.half_extents_m = read_vector<3>(item, "half_extents_m", pointer, 0.0001, 1000.0);
    } else if (type == "sphere") {
        require_keys(item, pointer, {"type", "radius_m"});
        value.type = ShapeType::Sphere;
        value.radius_m = read_number(item, "radius_m", pointer, 0.0, 1000.0, false);
    } else {
        fail(ContentErrorCode::InvalidEnum, child(pointer, "type"), "invalid shape type");
    }
    return value;
}

LevelManifest parse_level_manifest_impl(std::string_view text)
{
    const auto root = parse_json(text, kLevelMaxBytes);
    require_object(root, "");
    const auto source_schema_version = read_uint(root, "schema_version", "", 1U, 1U);
    require_keys(root, "", {"schema_version", "id", "planet", "launch_ring", "bird_roster", "free_body_ids",
        "bodies", "joints", "assemblies", "objectives"});
    LevelManifest result;
    result.schema_version = result.source_schema_version = source_schema_version;
    result.id = read_string(root, "id", "");

    const auto& planet = member(root, "planet", "");
    require_keys(planet, "/planet", {"entity_id", "radius_m", "surface_gravity_m_s2", "surface_id", "visual_id"});
    result.planet.entity_id = read_id<EntityId>(planet, "entity_id", "/planet");
    require_content_entity_id(result.planet.entity_id, "/planet/entity_id");
    result.planet.radius_m = read_number(planet, "radius_m", "/planet", 0.0, 100000.0, false);
    result.planet.surface_gravity_m_s2 = read_number(planet, "surface_gravity_m_s2", "/planet", 0.0, 1000.0, false);
    result.planet.surface_id = read_id<SurfaceId>(planet, "surface_id", "/planet");
    result.planet.visual_id = read_string(planet, "visual_id", "/planet");

    const auto& ring = member(root, "launch_ring", "");
    require_keys(ring, "/launch_ring", {"id", "shell_offset_m", "theta_min_deg", "theta_max_deg",
        "phase_speed_min_m_s", "phase_speed_max_m_s", "default_speed_m_s", "radial_formula"});
    result.launch_ring.id = read_string(ring, "id", "/launch_ring");
    result.launch_ring.shell_offset_m = read_number(ring, "shell_offset_m", "/launch_ring", 0.0, 1000.0, false);
    result.launch_ring.theta_min_deg = read_number(ring, "theta_min_deg", "/launch_ring", -180.0, 180.0);
    result.launch_ring.theta_max_deg = read_number(ring, "theta_max_deg", "/launch_ring", -180.0, 180.0);
    result.launch_ring.phase_speed_min_m_s = read_number(ring, "phase_speed_min_m_s", "/launch_ring", 8.0, 40.0);
    result.launch_ring.phase_speed_max_m_s = read_number(ring, "phase_speed_max_m_s", "/launch_ring", 8.0, 40.0);
    result.launch_ring.default_speed_m_s = read_number(ring, "default_speed_m_s", "/launch_ring", 8.0, 40.0);
    result.launch_ring.radial_formula = read_string(ring, "radial_formula", "/launch_ring");
    if (result.launch_ring.theta_min_deg >= result.launch_ring.theta_max_deg
        || result.launch_ring.phase_speed_min_m_s > result.launch_ring.default_speed_m_s
        || result.launch_ring.default_speed_m_s > result.launch_ring.phase_speed_max_m_s) {
        fail(ContentErrorCode::InvalidInvariant, "/launch_ring", "launch ring ranges are inconsistent");
    }

    const auto& roster = member(root, "bird_roster", "");
    require_collection(roster, "/bird_roster", 16U, true);
    result.bird_roster.reserve(roster.size());
    std::unordered_set<std::uint32_t> roster_archetype_ids;
    for (std::size_t i = 0; i < roster.size(); ++i) {
        const auto pointer = indexed("/bird_roster", i);
        const auto& item = roster.at(i);
        require_keys(item, pointer, {"bird_archetype_id", "count"});
        BirdRosterEntry entry{
            read_id<BirdArchetypeId>(item, "bird_archetype_id", pointer),
            read_uint(item, "count", pointer, 1U, 100U)};
        reject_duplicate(roster_archetype_ids, entry.bird_archetype_id,
            child(pointer, "bird_archetype_id"));
        result.bird_roster.push_back(entry);
    }
    const auto& free_body_ids = member(root, "free_body_ids", "");
    require_collection(free_body_ids, "/free_body_ids", 500U);
    result.free_body_ids.reserve(free_body_ids.size());
    for (std::size_t i = 0; i < free_body_ids.size(); ++i) {
        result.free_body_ids.push_back(
            read_uint_value(free_body_ids.at(i), indexed("/free_body_ids", i)));
    }

    const auto& bodies = member(root, "bodies", "");
    require_collection(bodies, "/bodies", 500U);
    result.bodies.reserve(bodies.size());
    std::unordered_set<std::uint32_t> body_ids;
    std::unordered_set<std::uint64_t> entity_parts;
    for (std::size_t i = 0; i < bodies.size(); ++i) {
        const auto pointer = indexed("/bodies", i);
        const auto& item = bodies.at(i);
        require_keys(item, pointer, {"body_id", "entity_id", "part_id", "body_type", "material_id", "surface_id",
            "enemy_archetype_id", "density_kg_m3", "transform", "shape", "visual"});
        BodyDefinition value;
        value.body_id = read_uint(item, "body_id", pointer);
        if (!body_ids.insert(value.body_id).second) {
            fail(ContentErrorCode::DuplicateId, child(pointer, "body_id"), "duplicate body id");
        }
        value.entity_id = read_id<EntityId>(item, "entity_id", pointer);
        require_content_entity_id(value.entity_id, child(pointer, "entity_id"));
        value.part_id = read_id<PartId>(item, "part_id", pointer);
        const auto entity_part_key = (static_cast<std::uint64_t>(value.entity_id.value()) << 32U)
            | value.part_id.value();
        if (!entity_parts.insert(entity_part_key).second) {
            fail(ContentErrorCode::DuplicateId, child(pointer, "part_id"),
                "duplicate entity and part pair");
        }
        value.body_type = parse_body_type(read_string(item, "body_type", pointer), child(pointer, "body_type"));
        value.material_id = read_nullable_id<MaterialId>(item, "material_id", pointer);
        value.surface_id = read_nullable_id<SurfaceId>(item, "surface_id", pointer);
        value.enemy_archetype_id = read_nullable_id<EnemyArchetypeId>(item, "enemy_archetype_id", pointer);
        if (value.material_id.has_value() == value.surface_id.has_value()) {
            fail(ContentErrorCode::InvalidInvariant, pointer, "body requires exactly one material or surface");
        }
        value.density_kg_m3 = read_number(item, "density_kg_m3", pointer,
            value.body_type == BodyType::Dynamic ? 0.0 : 0.0, 30000.0,
            value.body_type == BodyType::Static);

        const auto transform_pointer = child(pointer, "transform");
        const auto& transform = member(item, "transform", pointer);
        require_keys(transform, transform_pointer, {"position_m", "rotation_xyzw"});
        value.transform.position_m = read_vector<3>(transform, "position_m", transform_pointer, -100000.0, 100000.0);
        value.transform.rotation_xyzw = read_vector<4>(transform, "rotation_xyzw", transform_pointer, -1.0, 1.0);
        const auto& q = value.transform.rotation_xyzw;
        const double q_length = std::sqrt(q[0]*q[0] + q[1]*q[1] + q[2]*q[2] + q[3]*q[3]);
        if (std::abs(q_length - 1.0) > 1e-6) {
            fail(ContentErrorCode::InvalidInvariant, child(transform_pointer, "rotation_xyzw"),
                "rotation must be normalized");
        }

        value.shape = parse_shape(member(item, "shape", pointer), child(pointer, "shape"));
        const auto visual_pointer = child(pointer, "visual");
        const auto& visual = member(item, "visual", pointer);
        require_keys(visual, visual_pointer, {"asset_id", "bounds_m"});
        value.visual.asset_id = read_string(visual, "asset_id", visual_pointer);
        value.visual.bounds_m = read_vector<3>(visual, "bounds_m", visual_pointer, 0.0001, 2000.0);
        std::array<double, 3> collision_bounds{};
        if (value.shape.type == ShapeType::Box) {
            for (std::size_t axis = 0; axis < 3; ++axis) collision_bounds[axis] = value.shape.half_extents_m[axis] * 2.0;
        } else {
            collision_bounds.fill(value.shape.radius_m * 2.0);
        }
        for (std::size_t axis = 0; axis < 3; ++axis) {
            if (std::abs(collision_bounds[axis] - value.visual.bounds_m[axis]) > 1e-6) {
                fail(ContentErrorCode::VisualMismatch, child(visual_pointer, "bounds_m"),
                    "visual and collision bounds differ");
            }
        }
        result.bodies.push_back(std::move(value));
    }

    const auto& joints = member(root, "joints", "");
    require_collection(joints, "/joints", 250U);
    result.joints.reserve(joints.size());
    std::unordered_set<std::uint32_t> joint_ids;
    for (std::size_t i = 0; i < joints.size(); ++i) {
        const auto pointer = indexed("/joints", i);
        const auto& item = joints.at(i);
        require_keys(item, pointer, {"id", "assembly_id", "kind", "body_a_id", "body_b_id",
            "force_limit_n", "torque_limit_nm"});
        JointDefinition value;
        value.id = read_id<JointId>(item, "id", pointer);
        reject_duplicate(joint_ids, value.id, child(pointer, "id"));
        value.assembly_id = read_uint(item, "assembly_id", pointer);
        value.kind = parse_joint_kind(read_string(item, "kind", pointer), child(pointer, "kind"));
        value.body_a_id = read_uint(item, "body_a_id", pointer);
        value.body_b_id = read_uint(item, "body_b_id", pointer);
        if (value.body_a_id == value.body_b_id) {
            fail(ContentErrorCode::InvalidInvariant, pointer, "joint endpoints must differ");
        }
        value.force_limit_n = read_number(item, "force_limit_n", pointer, 0.0, 1e12, false);
        value.torque_limit_nm = read_number(item, "torque_limit_nm", pointer, 0.0, 1e12, false);
        result.joints.push_back(std::move(value));
    }

    const auto& assemblies = member(root, "assemblies", "");
    require_collection(assemblies, "/assemblies", 128U);
    result.assemblies.reserve(assemblies.size());
    std::size_t total_memberships = 0U;
    std::unordered_set<std::uint32_t> assembly_ids;
    for (std::size_t i = 0; i < assemblies.size(); ++i) {
        const auto pointer = indexed("/assemblies", i);
        const auto& item = assemblies.at(i);
        require_keys(item, pointer, {"id", "key", "body_ids", "joint_ids"});
        AssemblyDefinition value;
        value.id = read_uint(item, "id", pointer);
        if (!assembly_ids.insert(value.id).second) fail(ContentErrorCode::DuplicateId, child(pointer, "id"), "duplicate assembly id");
        value.key = read_string(item, "key", pointer);
        const auto& assembly_bodies = member(item, "body_ids", pointer);
        require_collection(assembly_bodies, child(pointer, "body_ids"), 500U);
        value.body_ids.reserve(assembly_bodies.size());
        for (std::size_t j = 0; j < assembly_bodies.size(); ++j) {
            value.body_ids.push_back(read_uint_value(assembly_bodies.at(j),
                indexed(child(pointer, "body_ids"), j)));
        }
        const auto& assembly_joints = member(item, "joint_ids", pointer);
        require_collection(assembly_joints, child(pointer, "joint_ids"), 250U);
        value.joint_ids.reserve(assembly_joints.size());
        for (std::size_t j = 0; j < assembly_joints.size(); ++j) {
            value.joint_ids.push_back(JointId{read_uint_value(assembly_joints.at(j),
                indexed(child(pointer, "joint_ids"), j))});
        }
        if (value.body_ids.empty() || value.joint_ids.empty()) {
            fail(ContentErrorCode::EmptyAssembly, pointer, "assembly body and joint lists must be non-empty");
        }
        total_memberships += value.body_ids.size() + value.joint_ids.size();
        if (total_memberships > 1000U) {
            fail(ContentErrorCode::ResourceLimit, pointer, "total assembly membership limit exceeded");
        }
        result.assemblies.push_back(std::move(value));
    }

    std::unordered_map<std::uint32_t, BodyDefinition*> parsed_bodies;
    parsed_bodies.reserve(result.bodies.size());
    for (auto& body : result.bodies) parsed_bodies.emplace(body.body_id, &body);
    for (const auto& assembly : result.assemblies) {
        for (const auto body_id : assembly.body_ids) {
            const auto found = parsed_bodies.find(body_id);
            if (found != parsed_bodies.end() && !found->second->assembly_id) {
                found->second->assembly_id = assembly.id;
            }
        }
    }

    const auto& objectives = member(root, "objectives", "");
    require_collection(objectives, "/objectives", 64U, true);
    result.objectives.reserve(objectives.size());
    std::unordered_set<std::uint32_t> objective_ids;
    for (std::size_t i = 0; i < objectives.size(); ++i) {
        const auto pointer = indexed("/objectives", i);
        const auto& item = objectives.at(i);
        require_keys(item, pointer, {"id", "kind", "target_entity_id"});
        ObjectiveDefinition value;
        value.id = read_uint(item, "id", pointer);
        if (!objective_ids.insert(value.id).second) fail(ContentErrorCode::DuplicateId, child(pointer, "id"), "duplicate objective id");
        value.kind = parse_objective_kind(read_string(item, "kind", pointer), child(pointer, "kind"));
        value.target_entity_id = read_id<EntityId>(item, "target_entity_id", pointer);
        require_content_entity_id(value.target_entity_id, child(pointer, "target_entity_id"));
        result.objectives.push_back(value);
    }
    return result;
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

const char* response_name(MaterialResponse value)
{
    switch (value) {
    case MaterialResponse::Fibrous: return "fibrous";
    case MaterialResponse::Masonry: return "masonry";
    case MaterialResponse::Brittle: return "brittle";
    case MaterialResponse::Compressible: return "compressible";
    case MaterialResponse::Ductile: return "ductile";
    }
    return "invalid";
}

const char* body_type_name(BodyType value) { return value == BodyType::Static ? "static" : "dynamic"; }
const char* joint_kind_name(JointKind value)
{
    switch (value) {
    case JointKind::PineFit: return "pine_fit";
    case JointKind::GlassClamp: return "glass_clamp";
    case JointKind::Mortar: return "mortar";
    }
    return "invalid";
}

bool nearly_equal(double value, double reference, double relative_tolerance)
{
    return std::abs(value - reference)
        <= relative_tolerance * std::max(1.0, std::abs(reference));
}

double body_volume_m3(const BodyDefinition& body)
{
    if (body.shape.type == ShapeType::Sphere) {
        return (4.0 / 3.0) * std::numbers::pi_v<double>
            * body.shape.radius_m * body.shape.radius_m * body.shape.radius_m;
    }
    return 8.0 * body.shape.half_extents_m[0]
        * body.shape.half_extents_m[1] * body.shape.half_extents_m[2];
}

}

ContentResult<std::uint32_t> detect_schema_version(std::string_view text, std::size_t byte_limit) noexcept
{
    return boundary<std::uint32_t>([&] {
        const auto root = parse_json(text, byte_limit);
        require_object(root, "");
        return read_uint(root, "schema_version", "", 1U, 2U);
    });
}

ContentResult<MaterialCatalog> parse_material_catalog_v1(std::string_view text) noexcept
{
    return boundary<MaterialCatalog>([&] { return parse_material_catalog_impl(text); });
}

ContentResult<ArchetypeCatalog> parse_archetype_catalog_v1(std::string_view text) noexcept
{
    return boundary<ArchetypeCatalog>([&] { return parse_archetype_catalog_impl(text); });
}

ContentResult<LevelManifest> parse_level_manifest_v1(std::string_view text) noexcept
{
    return boundary<LevelManifest>([&] { return parse_level_manifest_impl(text); });
}

ContentResult<MaterialCatalog> parse_material_catalog(std::string_view text) noexcept
{
    const auto version = detect_schema_version(text, kCatalogMaxBytes);
    if (!version) return {{}, version.error};
    return version.value == 1U ? parse_material_catalog_v1(text) : parse_material_catalog_v2(text);
}

ContentResult<ArchetypeCatalog> parse_archetype_catalog(std::string_view text) noexcept
{
    const auto version = detect_schema_version(text, kCatalogMaxBytes);
    if (!version) return {{}, version.error};
    return version.value == 1U ? parse_archetype_catalog_v1(text) : parse_archetype_catalog_v2(text);
}

ContentResult<LevelManifest> parse_level_manifest(std::string_view text) noexcept
{
    const auto version = detect_schema_version(text, kLevelMaxBytes);
    if (!version) return {{}, version.error};
    return version.value == 1U ? parse_level_manifest_v1(text) : parse_level_manifest_v2(text);
}

ContentResult<ContentBundle> make_content_bundle(const MaterialCatalog& materials,
    const ArchetypeCatalog& archetypes, const LevelManifest& level) noexcept
{
    return boundary<ContentBundle>([&] {
        if (materials.source_schema_version != 1U
            || archetypes.source_schema_version != 1U
            || level.source_schema_version != 1U) {
            fail(ContentErrorCode::InvalidInvariant, "/schema_version",
                "legacy bundle requires only schema version 1");
        }
        std::unordered_map<std::uint32_t, const MaterialDefinition*> material_index;
        std::unordered_map<std::uint32_t, const PhysicsSurfaceDefinition*> surface_index;
        std::unordered_map<std::uint32_t, const BirdArchetype*> bird_index;
        std::unordered_map<std::uint32_t, const EnemyArchetype*> enemy_index;
        std::unordered_map<std::uint32_t, std::size_t> enemy_positions;
        material_index.reserve(materials.materials.size());
        surface_index.reserve(materials.surfaces.size());
        bird_index.reserve(archetypes.birds.size());
        enemy_index.reserve(archetypes.enemies.size());
        for (const auto& item : materials.materials) material_index.emplace(item.id.value(), &item);
        for (const auto& item : materials.surfaces) surface_index.emplace(item.id.value(), &item);
        for (const auto& item : archetypes.birds) bird_index.emplace(item.id.value(), &item);
        for (std::size_t i = 0; i < archetypes.enemies.size(); ++i) {
            enemy_index.emplace(archetypes.enemies[i].id.value(), &archetypes.enemies[i]);
            enemy_positions.emplace(archetypes.enemies[i].id.value(), i);
        }
        const auto has_surface = [&](SurfaceId id) {
            return surface_index.contains(id.value());
        };
        const auto has_material = [&](MaterialId id) {
            return material_index.contains(id.value());
        };
        if (!std::isfinite(level.planet.surface_gravity_m_s2)) {
            fail(ContentErrorCode::InvalidNumber, "/planet/surface_gravity_m_s2",
                "surface gravity must be finite");
        }
        if (level.planet.surface_gravity_m_s2 <= 0.0
            || level.planet.surface_gravity_m_s2
                > static_cast<double>(physics::maximum_radial_acceleration)) {
            fail(ContentErrorCode::OutOfRange, "/planet/surface_gravity_m_s2",
                "surface gravity is outside the supported range");
        }
        require_content_entity_id(level.planet.entity_id, "/planet/entity_id");
        for (std::size_t i = 0; i < level.bodies.size(); ++i) {
            require_content_entity_id(
                level.bodies[i].entity_id, indexed("/bodies", i) + "/entity_id");
        }
        for (std::size_t i = 0; i < level.objectives.size(); ++i) {
            require_content_entity_id(level.objectives[i].target_entity_id,
                indexed("/objectives", i) + "/target_entity_id");
        }
        if (!has_surface(level.planet.surface_id)) {
            fail(ContentErrorCode::MissingReference, "/planet/surface_id", "surface reference not found");
        }
        for (std::size_t i = 0; i < archetypes.birds.size(); ++i) {
            const auto& bird = archetypes.birds[i];
            const auto found_surface = surface_index.find(bird.surface_id.value());
            if (found_surface == surface_index.end()) {
                fail(ContentErrorCode::MissingReference, indexed("/birds", i) + "/surface_id", "surface reference not found");
            }
            const auto& surface = *found_surface->second;
            if (!nearly_equal(bird.density_kg_m3, surface.density_kg_m3, 1e-9)) {
                fail(ContentErrorCode::InvalidInvariant, indexed("/birds", i) + "/density_kg_m3",
                    "bird density differs from its surface");
            }
            if (!nearly_equal(bird.friction, surface.friction, 1e-9)) {
                fail(ContentErrorCode::InvalidInvariant, indexed("/birds", i) + "/friction",
                    "bird friction differs from its surface");
            }
            if (!nearly_equal(bird.restitution, surface.restitution, 1e-9)) {
                fail(ContentErrorCode::InvalidInvariant, indexed("/birds", i) + "/restitution",
                    "bird restitution differs from its surface");
            }
            const double sphere_mass = (4.0 / 3.0) * std::numbers::pi_v<double>
                * bird.radius_m * bird.radius_m * bird.radius_m * bird.density_kg_m3;
            if (!nearly_equal(bird.mass_kg, sphere_mass, 0.001)) {
                fail(ContentErrorCode::InvalidInvariant, indexed("/birds", i) + "/mass_kg",
                    "bird mass differs from sphere volume times density");
            }
        }
        for (std::size_t i = 0; i < archetypes.enemies.size(); ++i) {
            if (!has_surface(archetypes.enemies[i].surface_id)) {
                fail(ContentErrorCode::MissingReference, indexed("/enemies", i) + "/surface_id", "surface reference not found");
            }
        }
        std::unordered_set<std::uint32_t> roster_archetype_ids;
        roster_archetype_ids.reserve(level.bird_roster.size());
        for (std::size_t i = 0; i < level.bird_roster.size(); ++i) {
            const auto id = level.bird_roster[i].bird_archetype_id;
            reject_duplicate(roster_archetype_ids, id,
                indexed("/bird_roster", i) + "/bird_archetype_id");
            if (!bird_index.contains(id.value())) {
                fail(ContentErrorCode::MissingReference, indexed("/bird_roster", i) + "/bird_archetype_id",
                    "bird archetype reference not found");
            }
        }
        struct EntityPhysicalSummary {
            std::unordered_set<std::uint32_t> enemy_archetype_ids;
            bool has_non_enemy_part{};
            double enemy_mass_kg{};
            std::optional<std::size_t> first_non_enemy_part;
            std::optional<std::size_t> conflicting_enemy_part;
        };
        std::unordered_set<std::uint32_t> body_ids;
        std::unordered_map<std::uint32_t, EntityPhysicalSummary> entity_summaries;
        body_ids.reserve(level.bodies.size());
        entity_summaries.reserve(level.bodies.size());
        for (std::size_t i = 0; i < level.bodies.size(); ++i) {
            const auto& body = level.bodies[i];
            body_ids.insert(body.body_id);
            if (body.material_id && !has_material(*body.material_id)) {
                fail(ContentErrorCode::MissingReference, indexed("/bodies", i) + "/material_id", "material reference not found");
            }
            if (body.surface_id && !has_surface(*body.surface_id)) {
                fail(ContentErrorCode::MissingReference, indexed("/bodies", i) + "/surface_id", "surface reference not found");
            }
            if (body.enemy_archetype_id && !enemy_index.contains(body.enemy_archetype_id->value())) {
                fail(ContentErrorCode::MissingReference, indexed("/bodies", i) + "/enemy_archetype_id",
                    "enemy archetype reference not found");
            }
            if (body.enemy_archetype_id) {
                const auto& enemy = *enemy_index.at(body.enemy_archetype_id->value());
                if (body.body_type != BodyType::Dynamic) {
                    fail(ContentErrorCode::InvalidInvariant, indexed("/bodies", i) + "/body_type",
                        "enemy body must be dynamic");
                }
                if (body.material_id || !body.surface_id) {
                    fail(ContentErrorCode::InvalidInvariant, indexed("/bodies", i) + "/material_id",
                        "enemy body must reference only a surface");
                }
                if (*body.surface_id != enemy.surface_id) {
                    fail(ContentErrorCode::InvalidInvariant, indexed("/bodies", i) + "/surface_id",
                        "enemy body surface differs from its archetype");
                }
            }
            double expected_density{};
            if (body.material_id) {
                expected_density = material_index.at(body.material_id->value())->density_kg_m3;
            } else {
                expected_density = surface_index.at(body.surface_id->value())->density_kg_m3;
            }
            if (!nearly_equal(body.density_kg_m3, expected_density, 1e-9)) {
                fail(ContentErrorCode::InvalidInvariant, indexed("/bodies", i) + "/density_kg_m3",
                    "body density differs from referenced material or surface");
            }
            auto& entity = entity_summaries[body.entity_id.value()];
            if (body.enemy_archetype_id) {
                const auto& enemy = *enemy_index.at(body.enemy_archetype_id->value());
                if (!entity.enemy_archetype_ids.empty()
                    && !entity.enemy_archetype_ids.contains(enemy.id.value())) {
                    entity.conflicting_enemy_part = i;
                }
                entity.enemy_archetype_ids.insert(enemy.id.value());
                entity.enemy_mass_kg += body_volume_m3(body) * body.density_kg_m3;
            } else {
                entity.has_non_enemy_part = true;
                if (!entity.first_non_enemy_part) entity.first_non_enemy_part = i;
            }
        }
        for (const auto& [entity_id, summary] : entity_summaries) {
            static_cast<void>(entity_id);
            if (summary.enemy_archetype_ids.empty()) continue;
            if (summary.has_non_enemy_part) {
                fail(ContentErrorCode::InvalidInvariant,
                    indexed("/bodies", *summary.first_non_enemy_part) + "/enemy_archetype_id",
                    "enemy entity cannot contain a non-enemy part");
            }
            if (summary.enemy_archetype_ids.size() != 1U) {
                fail(ContentErrorCode::InvalidInvariant,
                    indexed("/bodies", *summary.conflicting_enemy_part) + "/enemy_archetype_id",
                    "enemy entity cannot mix archetypes");
            }
            const auto enemy_id = *summary.enemy_archetype_ids.begin();
            const auto& enemy = *enemy_index.at(enemy_id);
            if (!nearly_equal(enemy.mass_kg, summary.enemy_mass_kg, 0.001)) {
                fail(ContentErrorCode::InvalidInvariant,
                    indexed("/enemies", enemy_positions.at(enemy_id)) + "/mass_kg",
                    "enemy mass differs from body volume times density");
            }
        }
        std::unordered_set<std::uint32_t> assembly_ids;
        for (const auto& assembly : level.assemblies) {
            assembly_ids.insert(assembly.id);
        }
        std::unordered_set<std::uint32_t> joint_ids;
        std::unordered_map<std::uint32_t, const JointDefinition*> joint_index;
        joint_ids.reserve(level.joints.size());
        joint_index.reserve(level.joints.size());
        for (std::size_t i = 0; i < level.joints.size(); ++i) {
            const auto& joint = level.joints[i];
            joint_ids.insert(joint.id.value());
            joint_index.emplace(joint.id.value(), &joint);
            if (!assembly_ids.contains(joint.assembly_id)) {
                fail(ContentErrorCode::MissingReference, indexed("/joints", i) + "/assembly_id",
                    "assembly reference not found");
            }
            if (!body_ids.contains(joint.body_a_id)) {
                fail(ContentErrorCode::MissingReference, indexed("/joints", i) + "/body_a_id", "body reference not found");
            }
            if (!body_ids.contains(joint.body_b_id)) {
                fail(ContentErrorCode::MissingReference, indexed("/joints", i) + "/body_b_id", "body reference not found");
            }
        }
        std::unordered_map<std::uint32_t, std::uint32_t> body_owners;
        std::unordered_set<std::uint32_t> free_bodies;
        for (std::size_t i = 0; i < level.free_body_ids.size(); ++i) {
            const auto id = level.free_body_ids[i];
            if (!body_ids.contains(id)) {
                fail(ContentErrorCode::MissingReference, indexed("/free_body_ids", i), "body reference not found");
            }
            if (!free_bodies.insert(id).second) {
                fail(ContentErrorCode::InvalidInvariant, indexed("/free_body_ids", i), "duplicate free body membership");
            }
        }
        std::unordered_map<std::uint32_t, std::uint32_t> joint_owners;
        for (std::size_t i = 0; i < level.assemblies.size(); ++i) {
            const auto& assembly = level.assemblies[i];
            for (std::size_t j = 0; j < assembly.body_ids.size(); ++j) {
                const auto body_id = assembly.body_ids[j];
                if (!body_ids.contains(body_id)) {
                    fail(ContentErrorCode::MissingReference,
                        indexed(indexed("/assemblies", i) + "/body_ids", j), "body reference not found");
                }
                if (free_bodies.contains(body_id) || !body_owners.emplace(body_id, assembly.id).second) {
                    fail(ContentErrorCode::InvalidInvariant,
                        indexed(indexed("/assemblies", i) + "/body_ids", j), "body has multiple memberships");
                }
            }
            for (std::size_t j = 0; j < assembly.joint_ids.size(); ++j) {
                const auto joint_id = assembly.joint_ids[j];
                if (!joint_ids.contains(joint_id.value())) {
                    fail(ContentErrorCode::MissingReference,
                        indexed(indexed("/assemblies", i) + "/joint_ids", j), "joint reference not found");
                }
                if (!joint_owners.emplace(joint_id.value(), assembly.id).second) {
                    fail(ContentErrorCode::InvalidInvariant,
                        indexed(indexed("/assemblies", i) + "/joint_ids", j), "joint has multiple memberships");
                }
            }
        }
        for (std::size_t i = 0; i < level.bodies.size(); ++i) {
            const auto& body = level.bodies[i];
            const auto owner = body_owners.find(body.body_id);
            if (owner == body_owners.end() && !free_bodies.contains(body.body_id)) {
                fail(ContentErrorCode::InvalidInvariant, indexed("/bodies", i) + "/body_id",
                    "body membership is not declared");
            }
            if (owner != body_owners.end()
                && (!body.assembly_id || *body.assembly_id != owner->second)) {
                fail(ContentErrorCode::InvalidInvariant, indexed("/bodies", i) + "/body_id",
                    "body membership is contradictory");
            }
        }
        for (std::size_t i = 0; i < level.joints.size(); ++i) {
            const auto& joint = level.joints[i];
            const auto owner = joint_owners.find(joint.id.value());
            if (owner == joint_owners.end()) {
                fail(ContentErrorCode::InvalidInvariant, indexed("/joints", i) + "/id",
                    "joint membership is not declared");
            }
            if (owner->second != joint.assembly_id) {
                fail(ContentErrorCode::InvalidInvariant, indexed("/joints", i) + "/assembly_id",
                    "joint membership is contradictory");
            }
            const auto owner_a = body_owners.find(joint.body_a_id);
            const auto owner_b = body_owners.find(joint.body_b_id);
            if (owner_a == body_owners.end() || owner_a->second != joint.assembly_id) {
                fail(ContentErrorCode::InvalidInvariant, indexed("/joints", i) + "/body_a_id",
                    "joint endpoint is outside its assembly");
            }
            if (owner_b == body_owners.end() || owner_b->second != joint.assembly_id) {
                fail(ContentErrorCode::InvalidInvariant, indexed("/joints", i) + "/body_b_id",
                    "joint endpoint is outside its assembly");
            }
        }
        for (std::size_t i = 0; i < level.assemblies.size(); ++i) {
            const auto& assembly = level.assemblies[i];
            std::unordered_map<std::uint32_t, std::vector<std::uint32_t>> adjacency;
            for (const auto body_id : assembly.body_ids) adjacency.try_emplace(body_id);
            for (const auto joint_id : assembly.joint_ids) {
                const auto& joint = *joint_index.at(joint_id.value());
                adjacency[joint.body_a_id].push_back(joint.body_b_id);
                adjacency[joint.body_b_id].push_back(joint.body_a_id);
            }
            std::unordered_set<std::uint32_t> visited;
            std::queue<std::uint32_t> pending;
            pending.push(assembly.body_ids.front());
            visited.insert(assembly.body_ids.front());
            while (!pending.empty()) {
                const auto current = pending.front();
                pending.pop();
                for (const auto neighbor : adjacency[current]) {
                    if (visited.insert(neighbor).second) pending.push(neighbor);
                }
            }
            if (visited.size() != assembly.body_ids.size()) {
                fail(ContentErrorCode::InvalidInvariant, indexed("/assemblies", i),
                    "assembly graph is disconnected");
            }
        }
        for (std::size_t i = 0; i < level.objectives.size(); ++i) {
            const auto found = entity_summaries.find(level.objectives[i].target_entity_id.value());
            if (found == entity_summaries.end() || found->second.has_non_enemy_part
                || found->second.enemy_archetype_ids.size() != 1U) {
                fail(ContentErrorCode::MissingReference, indexed("/objectives", i) + "/target_entity_id",
                    "objective target must resolve to one enemy entity");
            }
        }
        if (const auto semantic_error = detail::validate_level_semantics(archetypes, level)) {
            fail(semantic_error->code, semantic_error->pointer, semantic_error->message);
        }
        return ContentBundle{materials, archetypes, level};
    });
}

std::string to_canonical_json(const MaterialCatalog& catalog)
{
    if (catalog.source_schema_version == 2U) return detail::to_canonical_json_v2(catalog);
    json root{{"schema_version", catalog.schema_version}, {"materials", json::array()}, {"surfaces", json::array()}};
    for (const auto& item : catalog.materials) {
        root["materials"].push_back({{"id", item.id.value()}, {"key", item.key}, {"response", response_name(item.response)},
            {"density_kg_m3", item.density_kg_m3}, {"friction", item.friction},
            {"restitution", item.restitution}, {"toughness", item.toughness}});
    }
    for (const auto& item : catalog.surfaces) {
        root["surfaces"].push_back({{"id", item.id.value()}, {"key", item.key}, {"density_kg_m3", item.density_kg_m3},
            {"friction", item.friction}, {"restitution", item.restitution}});
    }
    return root.dump();
}

std::string to_canonical_json(const ArchetypeCatalog& catalog)
{
    if (catalog.source_schema_version == 2U) return detail::to_canonical_json_v2(catalog);
    json root{{"schema_version", catalog.schema_version}, {"abilities", json::array()}, {"birds", json::array()},
        {"weakpoints", json::array()}, {"enemies", json::array()}};
    for (const auto& item : catalog.abilities) root["abilities"].push_back({{"id", item.id.value()}, {"key", item.key}, {"kind", item.kind},
        {"arm_ticks", item.arm_ticks}, {"duration_ticks", item.duration_ticks}, {"radius_m", item.radius_m},
        {"max_body_mass_kg", item.max_body_mass_kg}, {"max_bodies", item.max_bodies},
        {"max_acceleration_m_s2", item.max_acceleration_m_s2}, {"pulse_speed_m_s", item.pulse_speed_m_s}});
    for (const auto& item : catalog.birds) root["birds"].push_back({{"id", item.id.value()}, {"key", item.key},
        {"ability_id", item.ability_id.value()}, {"surface_id", item.surface_id.value()}, {"mass_kg", item.mass_kg},
        {"density_kg_m3", item.density_kg_m3}, {"radius_m", item.radius_m}, {"friction", item.friction},
        {"restitution", item.restitution}, {"bullet", item.bullet}});
    for (const auto& item : catalog.weakpoints) root["weakpoints"].push_back({{"id", item.id.value()}, {"key", item.key},
        {"protected_direction", item.protected_direction}, {"protected_cone_deg", item.protected_cone_deg},
        {"protected_multiplier", item.protected_multiplier}, {"exposed_multiplier", item.exposed_multiplier}});
    for (const auto& item : catalog.enemies) root["enemies"].push_back({{"id", item.id.value()}, {"key", item.key},
        {"weakpoint_id", item.weakpoint_id.value()}, {"surface_id", item.surface_id.value()}, {"mass_kg", item.mass_kg},
        {"integrity", item.integrity}, {"damage_energy_j_per_kg", item.damage_energy_j_per_kg}, {"max_damage", item.max_damage}});
    return root.dump();
}

std::string to_canonical_json(const LevelManifest& level)
{
    if (level.source_schema_version == 2U) return detail::to_canonical_json_v2(level);
    json root{{"schema_version", level.schema_version}, {"id", level.id},
        {"planet", {{"entity_id", level.planet.entity_id.value()}, {"radius_m", level.planet.radius_m},
            {"surface_gravity_m_s2", level.planet.surface_gravity_m_s2}, {"surface_id", level.planet.surface_id.value()},
            {"visual_id", level.planet.visual_id}}},
        {"launch_ring", {{"id", level.launch_ring.id}, {"shell_offset_m", level.launch_ring.shell_offset_m},
            {"theta_min_deg", level.launch_ring.theta_min_deg}, {"theta_max_deg", level.launch_ring.theta_max_deg},
            {"phase_speed_min_m_s", level.launch_ring.phase_speed_min_m_s}, {"phase_speed_max_m_s", level.launch_ring.phase_speed_max_m_s},
            {"default_speed_m_s", level.launch_ring.default_speed_m_s}, {"radial_formula", level.launch_ring.radial_formula}}},
        {"bird_roster", json::array()}, {"free_body_ids", level.free_body_ids},
        {"bodies", json::array()}, {"joints", json::array()},
        {"assemblies", json::array()}, {"objectives", json::array()}};
    for (const auto& item : level.bird_roster) root["bird_roster"].push_back(
        {{"bird_archetype_id", item.bird_archetype_id.value()}, {"count", item.count}});
    for (const auto& item : level.bodies) {
        json shape = item.shape.type == ShapeType::Box
            ? json{{"type", "box"}, {"half_extents_m", item.shape.half_extents_m}}
            : json{{"type", "sphere"}, {"radius_m", item.shape.radius_m}};
        root["bodies"].push_back({{"body_id", item.body_id}, {"entity_id", item.entity_id.value()},
            {"part_id", item.part_id.value()}, {"body_type", body_type_name(item.body_type)},
            {"material_id", item.material_id ? json(item.material_id->value()) : json(nullptr)},
            {"surface_id", item.surface_id ? json(item.surface_id->value()) : json(nullptr)},
            {"enemy_archetype_id", item.enemy_archetype_id ? json(item.enemy_archetype_id->value()) : json(nullptr)},
            {"density_kg_m3", item.density_kg_m3},
            {"transform", {{"position_m", item.transform.position_m}, {"rotation_xyzw", item.transform.rotation_xyzw}}},
            {"shape", std::move(shape)}, {"visual", {{"asset_id", item.visual.asset_id}, {"bounds_m", item.visual.bounds_m}}}});
    }
    for (const auto& item : level.joints) root["joints"].push_back({{"id", item.id.value()}, {"assembly_id", item.assembly_id},
        {"kind", joint_kind_name(item.kind)}, {"body_a_id", item.body_a_id}, {"body_b_id", item.body_b_id},
        {"force_limit_n", item.force_limit_n}, {"torque_limit_nm", item.torque_limit_nm}});
    for (const auto& item : level.assemblies) {
        json joint_ids = json::array();
        for (const auto id : item.joint_ids) joint_ids.push_back(id.value());
        root["assemblies"].push_back({{"id", item.id}, {"key", item.key}, {"body_ids", item.body_ids}, {"joint_ids", std::move(joint_ids)}});
    }
    for (const auto& item : level.objectives) root["objectives"].push_back({{"id", item.id},
        {"kind", "neutralize_entity"}, {"target_entity_id", item.target_entity_id.value()}});
    return root.dump();
}

std::string_view content_error_code_name(ContentErrorCode code) noexcept
{
    switch (code) {
    case ContentErrorCode::None: return "none";
    case ContentErrorCode::InvalidJson: return "invalid_json";
    case ContentErrorCode::DuplicateKey: return "duplicate_key";
    case ContentErrorCode::ResourceLimit: return "resource_limit";
    case ContentErrorCode::InvalidType: return "invalid_type";
    case ContentErrorCode::InvalidNumber: return "invalid_number";
    case ContentErrorCode::MissingField: return "missing_field";
    case ContentErrorCode::UnknownKey: return "unknown_key";
    case ContentErrorCode::InvalidEnum: return "invalid_enum";
    case ContentErrorCode::OutOfRange: return "out_of_range";
    case ContentErrorCode::DuplicateId: return "duplicate_id";
    case ContentErrorCode::MissingReference: return "missing_reference";
    case ContentErrorCode::EmptyAssembly: return "empty_assembly";
    case ContentErrorCode::VisualMismatch: return "visual_mismatch";
    case ContentErrorCode::InvalidInvariant: return "invalid_invariant";
    case ContentErrorCode::InternalError: return "internal_error";
    }
    return "internal_error";
}

}
