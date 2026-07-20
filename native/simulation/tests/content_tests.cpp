#include "test_framework.hpp"

#include "ninho/simulation/content.hpp"

#include <nlohmann/json.hpp>

#include <algorithm>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <limits>
#include <sstream>
#include <string>
#include <string_view>
#include <utility>

namespace {

using json = nlohmann::json;
using namespace ninho::simulation;

std::string read_source_file(std::string_view relative_path)
{
    const auto path = std::filesystem::path{NINHO_SOURCE_DIR} / relative_path;
    std::ifstream stream{path, std::ios::binary};
    NINHO_SIM_REQUIRE(stream.is_open());
    std::ostringstream content;
    content << stream.rdbuf();
    return content.str();
}

struct RealContent {
    std::string materials_text = read_source_file("game/data/materials/vertical_slice.materials.json");
    std::string archetypes_text = read_source_file("game/data/archetypes/vertical_slice.archetypes.json");
    std::string level_text = read_source_file("game/data/levels/first_orbit.level.json");
    json materials = json::parse(materials_text);
    json archetypes = json::parse(archetypes_text);
    json level = json::parse(level_text);
};

template <typename Result>
void require_error(const Result& result, ContentErrorCode code, std::string_view pointer)
{
    NINHO_SIM_REQUIRE(!result.ok());
    NINHO_SIM_REQUIRE(result.error.code == code);
    NINHO_SIM_REQUIRE(result.error.pointer == pointer);
    NINHO_SIM_REQUIRE(!result.error.message.empty());
    NINHO_SIM_REQUIRE(result.error.message.size() < 256U);
}

NINHO_SIM_TEST("content parses the production catalogs and validates the complete bundle")
{
    const RealContent files;
    const auto materials = parse_material_catalog(files.materials_text);
    const auto archetypes = parse_archetype_catalog(files.archetypes_text);
    const auto level = parse_level_manifest(files.level_text);
    NINHO_SIM_REQUIRE(materials.ok());
    NINHO_SIM_REQUIRE(archetypes.ok());
    NINHO_SIM_REQUIRE(level.ok());

    NINHO_SIM_REQUIRE(materials.value.materials.size() == 3U);
    NINHO_SIM_REQUIRE(materials.value.materials.at(0).id == MaterialId{1});
    NINHO_SIM_REQUIRE(materials.value.materials.at(0).response == MaterialResponse::Fibrous);
    NINHO_SIM_REQUIRE(materials.value.materials.at(1).id == MaterialId{5});
    NINHO_SIM_REQUIRE(materials.value.materials.at(1).response == MaterialResponse::Masonry);
    NINHO_SIM_REQUIRE(materials.value.materials.at(2).id == MaterialId{9});
    NINHO_SIM_REQUIRE(materials.value.materials.at(2).response == MaterialResponse::Brittle);
    NINHO_SIM_REQUIRE(std::abs(materials.value.materials.at(2).toughness - 0.01) < 1e-9);
    NINHO_SIM_REQUIRE(materials.value.surfaces.size() == 4U);
    NINHO_SIM_REQUIRE(materials.value.surfaces.front().id == SurfaceId{1001});
    NINHO_SIM_REQUIRE(materials.value.surfaces.back().id == SurfaceId{1004});

    NINHO_SIM_REQUIRE(archetypes.value.abilities.size() == 1U);
    NINHO_SIM_REQUIRE(archetypes.value.birds.size() == 1U);
    NINHO_SIM_REQUIRE(archetypes.value.enemies.size() == 1U);
    NINHO_SIM_REQUIRE(archetypes.value.weakpoints.size() == 1U);
    const auto& bird = archetypes.value.birds.front();
    NINHO_SIM_REQUIRE(std::abs(bird.mass_kg - 140.0) < 1e-9);
    NINHO_SIM_REQUIRE(std::abs(bird.density_kg_m3 - 366.76) < 1e-9);
    NINHO_SIM_REQUIRE(std::abs(bird.friction - 0.35) < 1e-9);
    NINHO_SIM_REQUIRE(std::abs(bird.restitution - 0.25) < 1e-9);
    NINHO_SIM_REQUIRE(std::abs(
        archetypes.value.enemies.front().damage_energy_j_per_kg - 2.5) < 1e-9);

    NINHO_SIM_REQUIRE(level.value.schema_version == 1U);
    NINHO_SIM_REQUIRE(level.value.bird_roster.size() == 1U);
    NINHO_SIM_REQUIRE(level.value.bird_roster.front().count == 3U);
    NINHO_SIM_REQUIRE(level.value.bodies.size() == 22U);
    NINHO_SIM_REQUIRE(level.value.joints.size() == 18U);
    NINHO_SIM_REQUIRE(level.value.assemblies.size() == 2U);
    NINHO_SIM_REQUIRE(level.value.objectives.size() == 1U);
    NINHO_SIM_REQUIRE(std::abs(level.value.launch_ring.theta_min_deg + 50.0) < 1e-9);
    NINHO_SIM_REQUIRE(std::abs(level.value.launch_ring.theta_max_deg - 50.0) < 1e-9);
    NINHO_SIM_REQUIRE(level.value.launch_ring.radial_formula == "(-cos(theta),0,sin(theta))");
    const auto material_body_count = [&](MaterialId id) {
        return std::ranges::count_if(level.value.bodies,
            [&](const BodyDefinition& body) { return body.material_id == id; });
    };
    NINHO_SIM_REQUIRE(material_body_count(MaterialId{1}) == 8);
    NINHO_SIM_REQUIRE(material_body_count(MaterialId{9}) == 3);
    NINHO_SIM_REQUIRE(material_body_count(MaterialId{5}) == 9);
    for (const auto& body : level.value.bodies) {
        if (body.material_id != MaterialId{5} && body.material_id != MaterialId{9}) {
            continue;
        }
        const double mass = body.density_kg_m3 * 8.0
            * body.shape.half_extents_m[0]
            * body.shape.half_extents_m[1]
            * body.shape.half_extents_m[2];
        NINHO_SIM_REQUIRE(mass <= 150.0);
        if (body.material_id == MaterialId{5}) {
            const std::array<double, 3> expected_half_extents{0.15, 0.40, 0.16};
            const std::array<double, 3> expected_bounds{0.30, 0.80, 0.32};
            NINHO_SIM_REQUIRE(body.shape.half_extents_m == expected_half_extents);
            NINHO_SIM_REQUIRE(body.visual.bounds_m == expected_bounds);
            NINHO_SIM_REQUIRE(std::abs(mass - 138.24) < 1e-9);
        } else {
            const std::array<double, 3> expected_half_extents{0.02, 0.35, 0.45};
            const std::array<double, 3> expected_bounds{0.04, 0.70, 0.90};
            NINHO_SIM_REQUIRE(body.shape.half_extents_m == expected_half_extents);
            NINHO_SIM_REQUIRE(body.visual.bounds_m == expected_bounds);
            NINHO_SIM_REQUIRE(std::abs(mass - 61.74) < 1e-9);
        }
    }
    struct ExpectedJoint {
        JointId id;
        JointKind kind;
        double force_limit_n;
        double torque_limit_nm;
    };
    const std::array<ExpectedJoint, 18> expected_joints{{
        {JointId{1}, JointKind::PineFit, 7000.0, 1200.0},
        {JointId{2}, JointKind::PineFit, 7000.0, 1200.0},
        {JointId{3}, JointKind::PineFit, 7000.0, 1200.0},
        {JointId{4}, JointKind::PineFit, 7000.0, 1200.0},
        {JointId{5}, JointKind::PineFit, 7000.0, 1200.0},
        {JointId{6}, JointKind::PineFit, 7000.0, 1200.0},
        {JointId{7}, JointKind::PineFit, 7000.0, 1200.0},
        {JointId{8}, JointKind::GlassClamp, 3000.0, 500.0},
        {JointId{9}, JointKind::GlassClamp, 3000.0, 500.0},
        {JointId{10}, JointKind::GlassClamp, 3000.0, 500.0},
        {JointId{11}, JointKind::Mortar, 1400.0, 160.0},
        {JointId{12}, JointKind::Mortar, 1400.0, 160.0},
        {JointId{13}, JointKind::Mortar, 1400.0, 160.0},
        {JointId{14}, JointKind::Mortar, 1400.0, 160.0},
        {JointId{15}, JointKind::Mortar, 1400.0, 160.0},
        {JointId{16}, JointKind::Mortar, 1400.0, 160.0},
        {JointId{17}, JointKind::Mortar, 1400.0, 160.0},
        {JointId{18}, JointKind::Mortar, 950.0, 160.0},
    }};
    for (const ExpectedJoint& expected : expected_joints) {
        const auto& joint = level.value.joints.at(expected.id.value() - 1U);
        NINHO_SIM_REQUIRE(joint.id == expected.id);
        NINHO_SIM_REQUIRE(joint.kind == expected.kind);
        NINHO_SIM_REQUIRE(joint.force_limit_n == expected.force_limit_n);
        NINHO_SIM_REQUIRE(joint.torque_limit_nm == expected.torque_limit_nm);
    }
    struct ExpectedPose {
        std::uint32_t body_id;
        std::array<double, 3> position;
        std::array<double, 4> rotation;
    };
    const std::array<double, 4> identity_rotation{0.0, 0.0, 0.0, 1.0};
    const std::array<double, 4> quarter_turn_rotation{
        0.0, 0.0, 0.7071067811865476, 0.7071067811865476};
    const std::array<ExpectedPose, 21> expected_poses{{
        {2, {-1.7, 11.5, -0.9}, identity_rotation},
        {3, {1.7, 11.5, -0.9}, identity_rotation},
        {4, {-1.7, 11.5, 1.8}, identity_rotation},
        {5, {1.7, 11.5, 1.8}, identity_rotation},
        {6, {0.0, 12.45, 0.30}, identity_rotation},
        {7, {0.0, 12.45, 1.20}, identity_rotation},
        {8, {-2.0, 11.2, 0.0}, identity_rotation},
        {9, {2.0, 11.2, 0.0}, identity_rotation},
        {10, {-2.22, 11.90, -0.65}, identity_rotation},
        {11, {0.0, 12.67, 0.75}, quarter_turn_rotation},
        {12, {0.30, 12.67, 0.75}, quarter_turn_rotation},
        {13, {-0.30, 13.09, 0.43}, identity_rotation},
        {14, {0.0, 13.09, 0.43}, identity_rotation},
        {15, {0.30, 13.09, 0.43}, identity_rotation},
        {16, {-0.30, 13.09, 0.75}, identity_rotation},
        {17, {0.0, 13.09, 0.75}, identity_rotation},
        {18, {0.299, 13.09, 0.75}, identity_rotation},
        {19, {-0.30, 13.09, 1.07}, identity_rotation},
        {20, {0.0, 13.09, 1.07}, identity_rotation},
        {21, {0.30, 13.09, 1.07}, identity_rotation},
        {22, {-0.28, 10.95, 0.75}, quarter_turn_rotation},
    }};
    for (const ExpectedPose& expected : expected_poses) {
        const auto& body = level.value.bodies.at(expected.body_id - 1U);
        NINHO_SIM_REQUIRE(body.body_id == expected.body_id);
        NINHO_SIM_REQUIRE(body.transform.position_m == expected.position);
        NINHO_SIM_REQUIRE(body.transform.rotation_xyzw == expected.rotation);
    }
    NINHO_SIM_REQUIRE(!level.value.bodies.at(0).assembly_id.has_value());
    for (std::size_t i = 1; i <= 11; ++i) {
        NINHO_SIM_REQUIRE(level.value.bodies.at(i).assembly_id == 1U);
    }
    for (std::size_t i = 12; i <= 20; ++i) {
        NINHO_SIM_REQUIRE(level.value.bodies.at(i).assembly_id == 2U);
    }
    NINHO_SIM_REQUIRE(!level.value.bodies.at(21).assembly_id.has_value());

    const auto bundle = make_content_bundle(materials.value, archetypes.value, level.value);
    NINHO_SIM_REQUIRE(bundle.ok());
}

NINHO_SIM_TEST("content bundle enforces the kernel surface gravity ceiling")
{
    const RealContent files;
    const auto materials = parse_material_catalog(files.materials_text);
    const auto archetypes = parse_archetype_catalog(files.archetypes_text);
    const auto parsed_level = parse_level_manifest(files.level_text);
    NINHO_SIM_REQUIRE(materials.ok());
    NINHO_SIM_REQUIRE(archetypes.ok());
    NINHO_SIM_REQUIRE(parsed_level.ok());

    auto level = parsed_level.value;
    level.planet.surface_gravity_m_s2 = 18.0;
    NINHO_SIM_REQUIRE(make_content_bundle(materials.value, archetypes.value, level).ok());

    level.planet.surface_gravity_m_s2 = 18.01;
    require_error(make_content_bundle(materials.value, archetypes.value, level),
        ContentErrorCode::OutOfRange, "/planet/surface_gravity_m_s2");
}

NINHO_SIM_TEST("content fortification destructible AABB stays inside the design budget")
{
    const RealContent files;
    const auto level = parse_level_manifest(files.level_text);
    NINHO_SIM_REQUIRE(level.ok());

    std::array<double, 3> minimum{
        std::numeric_limits<double>::infinity(),
        std::numeric_limits<double>::infinity(),
        std::numeric_limits<double>::infinity()};
    std::array<double, 3> maximum{
        -std::numeric_limits<double>::infinity(),
        -std::numeric_limits<double>::infinity(),
        -std::numeric_limits<double>::infinity()};
    std::size_t destructible_count = 0;
    for (const auto& body : level.value.bodies) {
        if (!body.material_id) {
            continue;
        }
        ++destructible_count;
        std::array<double, 3> extent{};
        if (body.shape.type == ShapeType::Box) {
            const auto& q = body.transform.rotation_xyzw;
            const std::array<std::array<double, 3>, 3> rotation{{
                {{1.0 - 2.0 * (q[1] * q[1] + q[2] * q[2]),
                    2.0 * (q[0] * q[1] - q[2] * q[3]),
                    2.0 * (q[0] * q[2] + q[1] * q[3])}},
                {{2.0 * (q[0] * q[1] + q[2] * q[3]),
                    1.0 - 2.0 * (q[0] * q[0] + q[2] * q[2]),
                    2.0 * (q[1] * q[2] - q[0] * q[3])}},
                {{2.0 * (q[0] * q[2] - q[1] * q[3]),
                    2.0 * (q[1] * q[2] + q[0] * q[3]),
                    1.0 - 2.0 * (q[0] * q[0] + q[1] * q[1])}}
            }};
            for (std::size_t world_axis = 0; world_axis < 3; ++world_axis) {
                for (std::size_t local_axis = 0; local_axis < 3; ++local_axis) {
                    extent[world_axis] += std::abs(rotation[world_axis][local_axis])
                        * body.shape.half_extents_m[local_axis];
                }
            }
        } else {
            extent.fill(body.shape.radius_m);
        }
        for (std::size_t axis = 0; axis < 3; ++axis) {
            minimum[axis] = std::min(minimum[axis], body.transform.position_m[axis] - extent[axis]);
            maximum[axis] = std::max(maximum[axis], body.transform.position_m[axis] + extent[axis]);
        }
    }
    NINHO_SIM_REQUIRE(destructible_count == 20U);
    NINHO_SIM_REQUIRE(maximum[0] - minimum[0] <= 4.5 + 1e-9);
    NINHO_SIM_REQUIRE(maximum[1] - minimum[1] <= 4.5 + 1e-9);
    NINHO_SIM_REQUIRE(maximum[2] - minimum[2] <= 4.5 + 1e-9);
    NINHO_SIM_REQUIRE(std::abs(maximum[0] - minimum[0] - 4.44) < 1e-9);
    NINHO_SIM_REQUIRE(std::abs(maximum[1] - minimum[1] - 3.19) < 1e-9);
    NINHO_SIM_REQUIRE(std::abs(maximum[2] - minimum[2] - 3.20) < 1e-9);
}

NINHO_SIM_TEST("content canonical round trip preserves all three typed documents")
{
    const RealContent files;
    const auto materials = parse_material_catalog(files.materials_text);
    const auto archetypes = parse_archetype_catalog(files.archetypes_text);
    const auto level = parse_level_manifest(files.level_text);
    NINHO_SIM_REQUIRE(materials.ok() && archetypes.ok() && level.ok());
    NINHO_SIM_REQUIRE(parse_material_catalog_v1(files.materials_text).ok());
    NINHO_SIM_REQUIRE(parse_archetype_catalog_v1(files.archetypes_text).ok());
    NINHO_SIM_REQUIRE(parse_level_manifest_v1(files.level_text).ok());
    NINHO_SIM_REQUIRE(materials.value.source_schema_version == 1U);
    NINHO_SIM_REQUIRE(archetypes.value.source_schema_version == 1U);
    NINHO_SIM_REQUIRE(level.value.source_schema_version == 1U);

    const auto materials_again = parse_material_catalog(to_canonical_json(materials.value));
    const auto archetypes_again = parse_archetype_catalog(to_canonical_json(archetypes.value));
    const auto level_again = parse_level_manifest(to_canonical_json(level.value));
    NINHO_SIM_REQUIRE(materials_again.ok());
    NINHO_SIM_REQUIRE(archetypes_again.ok());
    if (!level_again.ok()) {
        ninho::simulation::test::fail(__FILE__, __LINE__,
            std::string{content_error_code_name(level_again.error.code)} + " "
                + level_again.error.pointer + " " + level_again.error.message);
    }
    NINHO_SIM_REQUIRE(to_canonical_json(materials.value) == to_canonical_json(materials_again.value));
    NINHO_SIM_REQUIRE(to_canonical_json(archetypes.value) == to_canonical_json(archetypes_again.value));
    NINHO_SIM_REQUIRE(to_canonical_json(level.value) == to_canonical_json(level_again.value));
}

NINHO_SIM_TEST("content rejects unknown keys recursively")
{
    RealContent files;
    files.level["bodies"][0]["shape"]["surprise"] = true;
    require_error(parse_level_manifest(files.level.dump()), ContentErrorCode::UnknownKey,
        "/bodies/0/shape/surprise");
}

NINHO_SIM_TEST("content errors use RFC 6901 escaping and exact array indexes")
{
    RealContent files;
    files.level["a/b~c"] = true;
    require_error(parse_level_manifest(files.level.dump()), ContentErrorCode::UnknownKey,
        "/a~1b~0c");

    files = RealContent{};
    files.level["assemblies"][0]["body_ids"][0] = "not-an-id";
    require_error(parse_level_manifest(files.level.dump()), ContentErrorCode::InvalidType,
        "/assemblies/0/body_ids/0");

    files = RealContent{};
    files.level["free_body_ids"][0] = "not-an-id";
    require_error(parse_level_manifest(files.level.dump()), ContentErrorCode::InvalidType,
        "/free_body_ids/0");
}

NINHO_SIM_TEST("content rejects missing required fields")
{
    RealContent files;
    files.materials["materials"][0].erase("toughness");
    require_error(parse_material_catalog(files.materials.dump()), ContentErrorCode::MissingField,
        "/materials/0/toughness");
}

NINHO_SIM_TEST("content rejects invalid enums")
{
    RealContent files;
    files.materials["materials"][0]["response"] = "rubbery";
    require_error(parse_material_catalog(files.materials.dump()), ContentErrorCode::InvalidEnum,
        "/materials/0/response");
}

NINHO_SIM_TEST("content rejects duplicate ids")
{
    RealContent files;
    files.materials["materials"][1]["id"] = 1;
    require_error(parse_material_catalog(files.materials.dump()), ContentErrorCode::DuplicateId,
        "/materials/1/id");
}

NINHO_SIM_TEST("content rejects duplicate bird roster archetypes independent of count order")
{
    for (const auto counts : {std::pair{1U, 2U}, std::pair{2U, 1U}}) {
        RealContent files;
        files.level["bird_roster"][0]["count"] = counts.first;
        auto duplicate = files.level["bird_roster"][0];
        duplicate["count"] = counts.second;
        files.level["bird_roster"].push_back(std::move(duplicate));
        require_error(parse_level_manifest(files.level.dump()),
            ContentErrorCode::DuplicateId, "/bird_roster/1/bird_archetype_id");
    }
}

NINHO_SIM_TEST("content bundle rejects typed duplicate bird roster archetypes in either count order")
{
    const RealContent files;
    const auto materials = parse_material_catalog(files.materials_text);
    const auto archetypes = parse_archetype_catalog(files.archetypes_text);
    const auto parsed_level = parse_level_manifest(files.level_text);
    NINHO_SIM_REQUIRE(materials.ok() && archetypes.ok() && parsed_level.ok());
    for (const auto counts : {std::pair{1U, 2U}, std::pair{2U, 1U}}) {
        LevelManifest level = parsed_level.value;
        level.bird_roster.front().count = counts.first;
        level.bird_roster.push_back(
            {level.bird_roster.front().bird_archetype_id, counts.second});
        require_error(make_content_bundle(materials.value, archetypes.value, level),
            ContentErrorCode::DuplicateId, "/bird_roster/1/bird_archetype_id");
    }
}

NINHO_SIM_TEST("content rejects duplicate entity parts and objectives that are not unique enemies")
{
    RealContent files;
    files.level["bodies"][1]["entity_id"] = files.level["bodies"][0]["entity_id"];
    files.level["bodies"][1]["part_id"] = files.level["bodies"][0]["part_id"];
    require_error(parse_level_manifest(files.level.dump()), ContentErrorCode::DuplicateId,
        "/bodies/1/part_id");

    files = RealContent{};
    files.level["objectives"][0]["target_entity_id"] = 10;
    const auto materials = parse_material_catalog(files.materials_text);
    const auto archetypes = parse_archetype_catalog(files.archetypes_text);
    const auto level = parse_level_manifest(files.level.dump());
    NINHO_SIM_REQUIRE(level.ok());
    require_error(make_content_bundle(materials.value, archetypes.value, level.value),
        ContentErrorCode::MissingReference, "/objectives/0/target_entity_id");
}

NINHO_SIM_TEST("content rejects non finite and out of range numbers")
{
    RealContent files;
    files.archetypes["birds"][0]["friction"] = 1.01;
    require_error(parse_archetype_catalog(files.archetypes.dump()), ContentErrorCode::OutOfRange,
        "/birds/0/friction");

    std::string overflow = files.materials.dump();
    const auto marker = overflow.find("520.0");
    NINHO_SIM_REQUIRE(marker != std::string::npos);
    overflow.replace(marker, 5U, "1e400");
    const auto result = parse_material_catalog(overflow);
    NINHO_SIM_REQUIRE(!result.ok());
    NINHO_SIM_REQUIRE(result.error.code == ContentErrorCode::InvalidNumber
        || result.error.code == ContentErrorCode::InvalidJson);
    NINHO_SIM_REQUIRE(result.error.message.size() < 256U);
}

NINHO_SIM_TEST("content rejects dynamic bodies without density")
{
    RealContent files;
    files.level["bodies"][1].erase("density_kg_m3");
    require_error(parse_level_manifest(files.level.dump()), ContentErrorCode::MissingField,
        "/bodies/1/density_kg_m3");
}

NINHO_SIM_TEST("content rejects orphan joints and invalid joint limits")
{
    RealContent files;
    files.level["joints"][0]["body_b_id"] = 999999;
    auto parsed = parse_level_manifest(files.level.dump());
    NINHO_SIM_REQUIRE(parsed.ok());
    const auto materials = parse_material_catalog(files.materials_text);
    const auto archetypes = parse_archetype_catalog(files.archetypes_text);
    require_error(make_content_bundle(materials.value, archetypes.value, parsed.value),
        ContentErrorCode::MissingReference, "/joints/0/body_b_id");

    files = RealContent{};
    files.level["joints"][0]["assembly_id"] = 999999;
    parsed = parse_level_manifest(files.level.dump());
    NINHO_SIM_REQUIRE(parsed.ok());
    require_error(make_content_bundle(materials.value, archetypes.value, parsed.value),
        ContentErrorCode::MissingReference, "/joints/0/assembly_id");

    files = RealContent{};
    files.level["joints"][0].erase("force_limit_n");
    require_error(parse_level_manifest(files.level.dump()), ContentErrorCode::MissingField,
        "/joints/0/force_limit_n");

    files = RealContent{};
    files.level["joints"][0]["torque_limit_nm"] = 0;
    require_error(parse_level_manifest(files.level.dump()), ContentErrorCode::OutOfRange,
        "/joints/0/torque_limit_nm");

    files = RealContent{};
    auto non_finite_limit = files.level.dump();
    const auto marker = non_finite_limit.find("7000.0");
    NINHO_SIM_REQUIRE(marker != std::string::npos);
    non_finite_limit.replace(marker, 6U, "1e400");
    const auto non_finite_result = parse_level_manifest(non_finite_limit);
    NINHO_SIM_REQUIRE(!non_finite_result.ok());
    NINHO_SIM_REQUIRE(non_finite_result.error.code == ContentErrorCode::InvalidNumber
        || non_finite_result.error.code == ContentErrorCode::InvalidJson);
}

NINHO_SIM_TEST("content rejects duplicate contradictory or disconnected assembly ownership")
{
    RealContent files;
    auto materials = parse_material_catalog(files.materials_text);
    auto archetypes = parse_archetype_catalog(files.archetypes_text);

    const auto duplicate_body_index = files.level["assemblies"][0]["body_ids"].size();
    files.level["assemblies"][0]["body_ids"].push_back(
        files.level["assemblies"][0]["body_ids"][0]);
    auto level = parse_level_manifest(files.level.dump());
    NINHO_SIM_REQUIRE(level.ok());
    require_error(make_content_bundle(materials.value, archetypes.value, level.value),
        ContentErrorCode::InvalidInvariant,
        "/assemblies/0/body_ids/" + std::to_string(duplicate_body_index));

    files = RealContent{};
    const auto duplicate_joint_index = files.level["assemblies"][0]["joint_ids"].size();
    files.level["assemblies"][0]["joint_ids"].push_back(
        files.level["assemblies"][0]["joint_ids"][0]);
    level = parse_level_manifest(files.level.dump());
    NINHO_SIM_REQUIRE(level.ok());
    require_error(make_content_bundle(materials.value, archetypes.value, level.value),
        ContentErrorCode::InvalidInvariant,
        "/assemblies/0/joint_ids/" + std::to_string(duplicate_joint_index));

    files = RealContent{};
    files.level["joints"][0]["assembly_id"] = 2;
    level = parse_level_manifest(files.level.dump());
    NINHO_SIM_REQUIRE(level.ok());
    require_error(make_content_bundle(materials.value, archetypes.value, level.value),
        ContentErrorCode::InvalidInvariant, "/joints/0/assembly_id");

    files = RealContent{};
    files.level["joints"][0]["body_b_id"] = 13;
    level = parse_level_manifest(files.level.dump());
    NINHO_SIM_REQUIRE(level.ok());
    require_error(make_content_bundle(materials.value, archetypes.value, level.value),
        ContentErrorCode::InvalidInvariant, "/joints/0/body_b_id");

    files = RealContent{};
    files.level["joints"][6]["body_a_id"] = 2;
    files.level["joints"][6]["body_b_id"] = 3;
    level = parse_level_manifest(files.level.dump());
    NINHO_SIM_REQUIRE(level.ok());
    require_error(make_content_bundle(materials.value, archetypes.value, level.value),
        ContentErrorCode::InvalidInvariant, "/assemblies/0");
}

NINHO_SIM_TEST("content rejects objectives without targets and empty assemblies")
{
    RealContent files;
    files.level["objectives"][0].erase("target_entity_id");
    require_error(parse_level_manifest(files.level.dump()), ContentErrorCode::MissingField,
        "/objectives/0/target_entity_id");

    files = RealContent{};
    files.level["assemblies"][0]["body_ids"] = json::array();
    files.level["assemblies"][0]["joint_ids"] = json::array();
    require_error(parse_level_manifest(files.level.dump()), ContentErrorCode::EmptyAssembly,
        "/assemblies/0");
}

NINHO_SIM_TEST("content rejects visual bounds inconsistent with collision geometry")
{
    RealContent files;
    files.level["bodies"][0]["visual"]["bounds_m"][0] = 99.0;
    require_error(parse_level_manifest(files.level.dump()), ContentErrorCode::VisualMismatch,
        "/bodies/0/visual/bounds_m");
}

NINHO_SIM_TEST("content bundle rejects missing material surface and archetype references")
{
    RealContent files;
    auto materials = parse_material_catalog(files.materials_text);
    auto archetypes = parse_archetype_catalog(files.archetypes_text);

    files.level["bodies"][1]["material_id"] = 777;
    auto level = parse_level_manifest(files.level.dump());
    NINHO_SIM_REQUIRE(level.ok());
    require_error(make_content_bundle(materials.value, archetypes.value, level.value),
        ContentErrorCode::MissingReference, "/bodies/1/material_id");

    files = RealContent{};
    files.level["planet"]["surface_id"] = 777;
    level = parse_level_manifest(files.level.dump());
    NINHO_SIM_REQUIRE(level.ok());
    require_error(make_content_bundle(materials.value, archetypes.value, level.value),
        ContentErrorCode::MissingReference, "/planet/surface_id");

    files = RealContent{};
    files.level["bird_roster"][0]["bird_archetype_id"] = 777;
    level = parse_level_manifest(files.level.dump());
    NINHO_SIM_REQUIRE(level.ok());
    require_error(make_content_bundle(materials.value, archetypes.value, level.value),
        ContentErrorCode::MissingReference, "/bird_roster/0/bird_archetype_id");
}

NINHO_SIM_TEST("content public parsing boundary never throws and does not echo input")
{
    const std::string secret(4096U, 'S');
    const auto result = parse_level_manifest("{\"secret\":\"" + secret + "\"}");
    NINHO_SIM_REQUIRE(!result.ok());
    NINHO_SIM_REQUIRE(result.error.message.find(secret) == std::string::npos);
    NINHO_SIM_REQUIRE(result.error.message.size() < 256U);
}

NINHO_SIM_TEST("content enforces document collection and nesting resource budgets")
{
    RealContent files;
    constexpr std::size_t catalog_limit = 256U * 1024U;
    auto at_byte_limit = files.materials_text;
    at_byte_limit.append(catalog_limit - at_byte_limit.size(), ' ');
    NINHO_SIM_REQUIRE(at_byte_limit.size() == catalog_limit);
    NINHO_SIM_REQUIRE(parse_material_catalog(at_byte_limit).ok());
    at_byte_limit.push_back(' ');
    require_error(parse_material_catalog(at_byte_limit), ContentErrorCode::ResourceLimit, "");

    const auto prototype = files.materials["materials"][0];
    files.materials["materials"] = json::array();
    for (std::uint32_t id = 1; id <= 64; ++id) {
        auto item = prototype;
        item["id"] = id;
        item["key"] = "material_" + std::to_string(id);
        files.materials["materials"].push_back(std::move(item));
    }
    NINHO_SIM_REQUIRE(parse_material_catalog(files.materials.dump()).ok());
    auto overflow_item = prototype;
    overflow_item["id"] = 65;
    overflow_item["key"] = "material_65";
    files.materials["materials"].push_back(std::move(overflow_item));
    require_error(parse_material_catalog(files.materials.dump()),
        ContentErrorCode::ResourceLimit, "/materials");

    std::string too_deep;
    std::string deep_pointer;
    for (int depth = 0; depth < 17; ++depth) {
        too_deep += "{\"a\":";
        if (depth != 0) deep_pointer += "/a";
    }
    too_deep += '0';
    too_deep.append(17U, '}');
    require_error(parse_material_catalog(too_deep),
        ContentErrorCode::ResourceLimit, deep_pointer);
}

NINHO_SIM_TEST("content collection budget boundaries distinguish accepted caps from plus one")
{
    RealContent files;
    files.level["bodies"] = json::array();
    for (int i = 0; i < 500; ++i) files.level["bodies"].push_back(nullptr);
    require_error(parse_level_manifest(files.level.dump()),
        ContentErrorCode::InvalidType, "/bodies/0");
    files.level["bodies"].push_back(nullptr);
    require_error(parse_level_manifest(files.level.dump()),
        ContentErrorCode::ResourceLimit, "/bodies");

    files = RealContent{};
    files.level["joints"] = json::array();
    for (int i = 0; i < 250; ++i) files.level["joints"].push_back(nullptr);
    require_error(parse_level_manifest(files.level.dump()),
        ContentErrorCode::InvalidType, "/joints/0");
    files.level["joints"].push_back(nullptr);
    require_error(parse_level_manifest(files.level.dump()),
        ContentErrorCode::ResourceLimit, "/joints");

    files = RealContent{};
    json first_bodies = json::array();
    json first_joints = json::array();
    json second_joints = json::array();
    for (int id = 1; id <= 500; ++id) first_bodies.push_back(id);
    for (int id = 1; id <= 250; ++id) first_joints.push_back(id);
    for (int id = 1; id <= 249; ++id) second_joints.push_back(id);
    files.level["assemblies"] = json::array({
        {{"id", 1}, {"key", "limit_a"}, {"body_ids", first_bodies}, {"joint_ids", first_joints}},
        {{"id", 2}, {"key", "limit_b"}, {"body_ids", json::array({1})}, {"joint_ids", second_joints}}
    });
    NINHO_SIM_REQUIRE(parse_level_manifest(files.level.dump()).ok());
    files.level["assemblies"][1]["joint_ids"].push_back(250);
    require_error(parse_level_manifest(files.level.dump()),
        ContentErrorCode::ResourceLimit, "/assemblies/1");
}

NINHO_SIM_TEST("content enemy bodies obey dynamic surface archetype contract")
{
    RealContent files;
    const auto materials = parse_material_catalog(files.materials_text);
    const auto archetypes = parse_archetype_catalog(files.archetypes_text);

    files.level["bodies"][21]["body_type"] = "static";
    auto level = parse_level_manifest(files.level.dump());
    NINHO_SIM_REQUIRE(level.ok());
    require_error(make_content_bundle(materials.value, archetypes.value, level.value),
        ContentErrorCode::InvalidInvariant, "/bodies/21/body_type");

    files = RealContent{};
    files.level["bodies"][21]["material_id"] = 5;
    files.level["bodies"][21]["surface_id"] = nullptr;
    level = parse_level_manifest(files.level.dump());
    NINHO_SIM_REQUIRE(level.ok());
    require_error(make_content_bundle(materials.value, archetypes.value, level.value),
        ContentErrorCode::InvalidInvariant, "/bodies/21/material_id");

    files = RealContent{};
    files.level["bodies"][21]["surface_id"] = 1002;
    level = parse_level_manifest(files.level.dump());
    NINHO_SIM_REQUIRE(level.ok());
    require_error(make_content_bundle(materials.value, archetypes.value, level.value),
        ContentErrorCode::InvalidInvariant, "/bodies/21/surface_id");
}

NINHO_SIM_TEST("content validates every enemy entity even when objectives do not reference it")
{
    RealContent files;
    auto extra_enemy_part = files.level["bodies"][21];
    extra_enemy_part["body_id"] = 23;
    extra_enemy_part["entity_id"] = 300;
    extra_enemy_part["part_id"] = 2;
    files.level["bodies"].push_back(std::move(extra_enemy_part));
    files.level["free_body_ids"].push_back(23);
    files.level["bodies"][1]["entity_id"] = 300;
    const auto materials = parse_material_catalog(files.materials_text);
    auto archetypes = parse_archetype_catalog(files.archetypes_text);
    auto level = parse_level_manifest(files.level.dump());
    NINHO_SIM_REQUIRE(level.ok());
    require_error(make_content_bundle(materials.value, archetypes.value, level.value),
        ContentErrorCode::InvalidInvariant, "/bodies/1/enemy_archetype_id");

    files = RealContent{};
    auto second_enemy = files.archetypes["enemies"][0];
    second_enemy["id"] = 2;
    second_enemy["key"] = "anchor_variant";
    files.archetypes["enemies"].push_back(std::move(second_enemy));
    auto conflicting_part = files.level["bodies"][21];
    conflicting_part["body_id"] = 23;
    conflicting_part["part_id"] = 2;
    conflicting_part["enemy_archetype_id"] = 2;
    files.level["bodies"].push_back(std::move(conflicting_part));
    files.level["free_body_ids"].push_back(23);
    archetypes = parse_archetype_catalog(files.archetypes.dump());
    level = parse_level_manifest(files.level.dump());
    NINHO_SIM_REQUIRE(archetypes.ok() && level.ok());
    require_error(make_content_bundle(materials.value, archetypes.value, level.value),
        ContentErrorCode::InvalidInvariant, "/bodies/22/enemy_archetype_id");
}

NINHO_SIM_TEST("content cross validates material bird surface and enemy physical values")
{
    RealContent files;
    auto materials = parse_material_catalog(files.materials_text);
    auto archetypes = parse_archetype_catalog(files.archetypes_text);

    files.level["bodies"][1]["density_kg_m3"] = 521.0;
    auto level = parse_level_manifest(files.level.dump());
    require_error(make_content_bundle(materials.value, archetypes.value, level.value),
        ContentErrorCode::InvalidInvariant, "/bodies/1/density_kg_m3");

    files = RealContent{};
    files.archetypes["birds"][0]["mass_kg"] = 141.0;
    archetypes = parse_archetype_catalog(files.archetypes.dump());
    level = parse_level_manifest(files.level_text);
    require_error(make_content_bundle(materials.value, archetypes.value, level.value),
        ContentErrorCode::InvalidInvariant, "/birds/0/mass_kg");

    files = RealContent{};
    files.archetypes["birds"][0]["friction"] = 0.36;
    archetypes = parse_archetype_catalog(files.archetypes.dump());
    require_error(make_content_bundle(materials.value, archetypes.value, level.value),
        ContentErrorCode::InvalidInvariant, "/birds/0/friction");

    files = RealContent{};
    files.archetypes["enemies"][0]["mass_kg"] = 481.0;
    archetypes = parse_archetype_catalog(files.archetypes.dump());
    require_error(make_content_bundle(materials.value, archetypes.value, level.value),
        ContentErrorCode::InvalidInvariant, "/enemies/0/mass_kg");
}

NINHO_SIM_TEST("content rejects positive physical values that collapse in float")
{
    RealContent files;
    files.materials["materials"][0]["density_kg_m3"] = 1e-300;
    require_error(parse_material_catalog(files.materials.dump()),
        ContentErrorCode::OutOfRange, "/materials/0/density_kg_m3");

    files = RealContent{};
    files.level["joints"][0]["force_limit_n"] = 1e-300;
    require_error(parse_level_manifest(files.level.dump()),
        ContentErrorCode::OutOfRange, "/joints/0/force_limit_n");
}

NINHO_SIM_TEST("content rejects duplicate JSON keys at root and nested objects")
{
    RealContent files;
    auto duplicate_root = files.materials_text;
    const auto root_marker = duplicate_root.find("\"schema_version\": 1");
    NINHO_SIM_REQUIRE(root_marker != std::string::npos);
    duplicate_root.insert(root_marker, "\"schema_version\": 1, ");
    require_error(parse_material_catalog(duplicate_root),
        ContentErrorCode::DuplicateKey, "/schema_version");

    auto duplicate_nested = files.level_text;
    const auto nested_marker = duplicate_nested.find("\"radius_m\": 10.0");
    NINHO_SIM_REQUIRE(nested_marker != std::string::npos);
    duplicate_nested.insert(nested_marker, "\"radius_m\": 10.0, ");
    require_error(parse_level_manifest(duplicate_nested),
        ContentErrorCode::DuplicateKey, "/planet/radius_m");
}

}
