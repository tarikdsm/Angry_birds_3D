#include "test_framework.hpp"

#include "box3d_allocator_probe.hpp"
#include "material_mapping.hpp"
#include "physics_world_test_facade.hpp"
#include "session_test_facade.hpp"
#include "ninho/physics/physics_world.hpp"
#include "ninho/simulation/session.hpp"

#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <limits>
#include <numbers>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <type_traits>
#include <vector>

namespace {

using namespace ninho::simulation;

template <typename T>
concept ExposesHandle = requires(T value) { value.handle; };

std::string read_source_file(std::string_view relative_path)
{
    const auto path = std::filesystem::path{NINHO_SOURCE_DIR} / relative_path;
    std::ifstream stream{path, std::ios::binary};
    NINHO_SIM_REQUIRE(stream.is_open());
    std::ostringstream content;
    content << stream.rdbuf();
    return content.str();
}

ContentBundle load_real_bundle()
{
    const auto materials = parse_material_catalog(
        read_source_file("game/data/materials/vertical_slice.materials.json"));
    const auto archetypes = parse_archetype_catalog(
        read_source_file("game/data/archetypes/vertical_slice.archetypes.json"));
    const auto level = parse_level_manifest(
        read_source_file("game/data/levels/first_orbit.level.json"));
    NINHO_SIM_REQUIRE(materials.ok() && archetypes.ok() && level.ok());
    const auto bundle = make_content_bundle(materials.value, archetypes.value, level.value);
    NINHO_SIM_REQUIRE(bundle.ok());
    return bundle.value;
}

std::unique_ptr<SimulationSession> create_real_session()
{
    const ContentBundle bundle = load_real_bundle();
    auto result = SimulationSession::create(bundle.materials, bundle.archetypes, bundle.level);
    NINHO_SIM_REQUIRE(result.ok());
    NINHO_SIM_REQUIRE(result.value != nullptr);
    return std::move(result.value);
}

std::uint64_t fnv1a64(std::span<const std::uint8_t> bytes) noexcept
{
    std::uint64_t hash = 14695981039346656037ULL;
    for (const std::uint8_t byte : bytes) {
        hash ^= byte;
        hash *= 1099511628211ULL;
    }
    return hash;
}

double position_distance(
    const ninho::physics::Transform& lhs, const ninho::physics::Transform& rhs)
{
    const auto delta = lhs.position - rhs.position;
    return static_cast<double>(ninho::physics::length(delta));
}

double orientation_distance(
    const ninho::physics::Quat& lhs, const ninho::physics::Quat& rhs)
{
    const double cosine = std::clamp(std::abs(
        static_cast<double>(lhs.x) * rhs.x + static_cast<double>(lhs.y) * rhs.y
        + static_cast<double>(lhs.z) * rhs.z + static_cast<double>(lhs.w) * rhs.w),
        0.0, 1.0);
    return 2.0 * std::acos(cosine);
}

const EntitySnapshot& find_snapshot(
    const SimulationSession& session, JointEndpoint endpoint)
{
    const auto found = std::ranges::find_if(session.snapshots(), [&](const auto& snapshot) {
        return snapshot.entity_id == endpoint.entity_id && snapshot.part_id == endpoint.part_id;
    });
    NINHO_SIM_REQUIRE(found != session.snapshots().end());
    return *found;
}

void require_weld_does_not_snap(
    const SimulationSession& session, const StructuralJointSnapshot& joint)
{
    const auto& body_a = find_snapshot(session, joint.a);
    const auto& body_b = find_snapshot(session, joint.b);
    using namespace ninho::physics;
    PhysicsWorld isolated({.surface_gravity = 0.0f});
    auto description_a = BodyDesc::dynamic_sphere(0.05f, body_a.transform, 10.0f);
    auto description_b = BodyDesc::dynamic_sphere(0.05f, body_b.transform, 10.0f);
    description_a.radial_gravity = false;
    description_b.radial_gravity = false;
    const auto handle_a = isolated.create_body(description_a);
    const auto handle_b = isolated.create_body(description_b);
    NINHO_SIM_REQUIRE(handle_a && handle_b);
    NINHO_SIM_REQUIRE(isolated.create_joint(WeldJointDesc{
        .a = handle_a.value,
        .b = handle_b.value,
        .frame_a = joint.frame_a,
        .frame_b = joint.frame_b,
        .hertz = 8.0f,
        .damping_ratio = 1.0f,
        .collide_connected = false,
    }));
    NINHO_SIM_REQUIRE(isolated.commit_pending_initial_state().ok());
    const auto before_a = isolated.state(handle_a.value);
    const auto before_b = isolated.state(handle_b.value);
    NINHO_SIM_REQUIRE(before_a && before_b);
    isolated.step();
    const auto after_a = isolated.state(handle_a.value);
    const auto after_b = isolated.state(handle_b.value);
    NINHO_SIM_REQUIRE(after_a && after_b);
    NINHO_SIM_REQUIRE(position_distance(before_a->transform, after_a->transform) <= 1.0e-5);
    NINHO_SIM_REQUIRE(position_distance(before_b->transform, after_b->transform) <= 1.0e-5);
    NINHO_SIM_REQUIRE(
        orientation_distance(before_a->transform.rotation, after_a->transform.rotation) <= 1.0e-5);
    NINHO_SIM_REQUIRE(
        orientation_distance(before_b->transform.rotation, after_b->transform.rotation) <= 1.0e-5);
}

class CanonicalReader {
public:
    explicit CanonicalReader(const std::vector<std::uint8_t>& source) : bytes(source) {}

    std::uint8_t u8() { return integer<std::uint8_t>(); }
    std::uint32_t u32() { return integer<std::uint32_t>(); }
    std::uint64_t u64() { return integer<std::uint64_t>(); }
    std::int64_t i64() { return static_cast<std::int64_t>(integer<std::uint64_t>()); }

    std::string text()
    {
        const auto size = u32();
        NINHO_SIM_REQUIRE(offset + size <= bytes.size());
        std::string value{reinterpret_cast<const char*>(bytes.data() + offset), size};
        offset += size;
        return value;
    }

    std::vector<std::uint8_t> blob()
    {
        const auto size = u32();
        NINHO_SIM_REQUIRE(offset + size <= bytes.size());
        std::vector<std::uint8_t> value(
            bytes.begin() + static_cast<std::ptrdiff_t>(offset),
            bytes.begin() + static_cast<std::ptrdiff_t>(offset + size));
        offset += size;
        return value;
    }

private:
    template <typename Integer>
    Integer integer()
    {
        NINHO_SIM_REQUIRE(offset + sizeof(Integer) <= bytes.size());
        Integer value{};
        for (std::size_t index = 0; index < sizeof(Integer); ++index) {
            value |= static_cast<Integer>(bytes[offset++]) << (index * 8U);
        }
        return value;
    }

    const std::vector<std::uint8_t>& bytes;
    std::size_t offset{};
};

NINHO_SIM_TEST("session bootstrap commits only initial creations without stepping")
{
    using namespace ninho::physics;
    PhysicsWorld world({.surface_gravity = 0.0f});
    const auto body = world.create_body(
        BodyDesc::dynamic_box({0.5f, 0.5f, 0.5f}, {{0.0f, 5.0f, 0.0f}, {}}, 10.0f));
    NINHO_SIM_REQUIRE(body);
    NINHO_SIM_REQUIRE(world.states().empty());
    NINHO_SIM_REQUIRE(world.contact_hits().empty());

    const Status committed = world.commit_pending_initial_state();
    NINHO_SIM_REQUIRE(committed.ok());
    NINHO_SIM_REQUIRE(world.states().size() == 1U);
    NINHO_SIM_REQUIRE(world.contact_hits().empty());
    NINHO_SIM_REQUIRE(world.joint_reactions().empty());
    NINHO_SIM_REQUIRE(position_distance(
        world.states().front().transform, {{0.0f, 5.0f, 0.0f}, {}}) <= 1.0e-7);
    NINHO_SIM_REQUIRE(!world.commit_pending_initial_state().ok());
}

NINHO_SIM_TEST("session bootstrap initial commit rejects non-create commands atomically")
{
    using namespace ninho::physics;
    PhysicsWorld world({.surface_gravity = 0.0f});
    const auto body = world.create_body(
        BodyDesc::dynamic_box({0.5f, 0.5f, 0.5f}, {{0.0f, 5.0f, 0.0f}, {}}, 10.0f));
    NINHO_SIM_REQUIRE(body);
    NINHO_SIM_REQUIRE(world.apply_force(body.value, {1.0f, 0.0f, 0.0f}, {}, true).ok());
    const Status status = world.commit_pending_initial_state();
    NINHO_SIM_REQUIRE(!status.ok());
    NINHO_SIM_REQUIRE(status.code == StatusCode::InvalidArgument);
    NINHO_SIM_REQUIRE(world.states().empty());
    const Status retry = world.commit_pending_initial_state();
    NINHO_SIM_REQUIRE(!retry.ok());
    NINHO_SIM_REQUIRE(retry.code == StatusCode::InvalidArgument);
    world.step();
    NINHO_SIM_REQUIRE(world.states().size() == 1U);
}

NINHO_SIM_TEST("session bootstrap initial commit is unavailable after any step")
{
    using namespace ninho::physics;
    PhysicsWorld world({.surface_gravity = 0.0f});
    world.step();
    const Status status = world.commit_pending_initial_state();
    NINHO_SIM_REQUIRE(!status.ok());
    NINHO_SIM_REQUIRE(status.code == StatusCode::InvalidArgument);
}

NINHO_SIM_TEST("session bootstrap latches a partial initial commit failure permanently")
{
    using namespace ninho::physics;
    PhysicsWorld world({.surface_gravity = 0.0f});
    NINHO_SIM_REQUIRE(world.create_body(
        BodyDesc::dynamic_sphere(0.2f, {{0.0f, 5.0f, 0.0f}, {}}, 10.0f)));
    NINHO_SIM_REQUIRE(world.create_body(
        BodyDesc::dynamic_sphere(0.2f, {{1.0f, 5.0f, 0.0f}, {}}, 10.0f)));
    ninho::physics::detail::PhysicsWorldTestFacade::fail_initial_commit_after(world, 1U);

    const Status first = world.commit_pending_initial_state();
    NINHO_SIM_REQUIRE(!first.ok());
    NINHO_SIM_REQUIRE(first.code == StatusCode::Box3DFault);
    const Status retry = world.commit_pending_initial_state();
    NINHO_SIM_REQUIRE(!retry.ok());
    NINHO_SIM_REQUIRE(retry.code == StatusCode::Box3DFault);

    bool step_threw = false;
    try {
        world.step();
    } catch (const std::logic_error&) {
        step_threw = true;
    }
    NINHO_SIM_REQUIRE(step_threw);
}

NINHO_SIM_TEST("session bootstrap creates ordered domain snapshots and weld registry")
{
    static_assert(!ExposesHandle<EntitySnapshot>);
    static_assert(!ExposesHandle<StructuralJointSnapshot>);

    const auto session = create_real_session();
    NINHO_SIM_REQUIRE(session->state().tick == TickIndex{0});
    NINHO_SIM_REQUIRE(session->state().phase == SessionPhase::Inspection);
    NINHO_SIM_REQUIRE(session->state().outcome == Outcome::None);
    NINHO_SIM_REQUIRE(session->birds_remaining() == 3U);
    NINHO_SIM_REQUIRE(session->events().empty());
    NINHO_SIM_REQUIRE(session->physics_metrics().body_count == 23);
    NINHO_SIM_REQUIRE(session->physics_metrics().joint_count == 18);
    NINHO_SIM_REQUIRE(session->physics_metrics().shape_count == 23);
    NINHO_SIM_REQUIRE(session->snapshots().size() == 23U);
    NINHO_SIM_REQUIRE(session->structural_joints().size() == 18U);

    NINHO_SIM_REQUIRE(std::ranges::is_sorted(session->snapshots(), {}, [](const auto& value) {
        return std::pair{value.entity_id, value.part_id};
    }));
    NINHO_SIM_REQUIRE(std::ranges::is_sorted(session->structural_joints(), {}, [](const auto& value) {
        return value.id;
    }));

    const auto& planet = session->snapshots().front();
    NINHO_SIM_REQUIRE(planet.entity_id == EntityId{1});
    NINHO_SIM_REQUIRE(planet.part_id == PartId{0});
    NINHO_SIM_REQUIRE(planet.surface_id == SurfaceId{1001});
    NINHO_SIM_REQUIRE(!planet.material_id.has_value());
    NINHO_SIM_REQUIRE(planet.body_type == BodyType::Static);
    NINHO_SIM_REQUIRE(planet.shape.type == ShapeType::Sphere);
    NINHO_SIM_REQUIRE(std::abs(planet.shape.radius_m - 10.0) <= 1.0e-9);
    NINHO_SIM_REQUIRE(planet.visual_id == "AST_AsterPlanet");
    NINHO_SIM_REQUIRE(!planet.is_projectile);

    const auto platform = std::ranges::find(
        session->snapshots(), EntityId{10}, &EntitySnapshot::entity_id);
    NINHO_SIM_REQUIRE(platform != session->snapshots().end());
    NINHO_SIM_REQUIRE(platform->surface_id == SurfaceId{1002});
    NINHO_SIM_REQUIRE(platform->shape.type == ShapeType::Box);
    NINHO_SIM_REQUIRE(platform->visual_id == "AST_Platform");

    const auto anchor = std::ranges::find(session->snapshots(), EntityId{200}, &EntitySnapshot::entity_id);
    NINHO_SIM_REQUIRE(anchor != session->snapshots().end());
    NINHO_SIM_REQUIRE(std::abs(anchor->mass_kg - 480.0) <= 0.01);
    NINHO_SIM_REQUIRE(anchor->surface_id == SurfaceId{1004});
    NINHO_SIM_REQUIRE(!anchor->material_id.has_value());

    NINHO_SIM_REQUIRE(std::ranges::count(session->snapshots(), MaterialId{1},
                          [](const auto& value) { return value.material_id.value_or(MaterialId{}); })
        == 8);
    NINHO_SIM_REQUIRE(std::ranges::count(session->snapshots(), MaterialId{5},
                          [](const auto& value) { return value.material_id.value_or(MaterialId{}); })
        == 9);
    NINHO_SIM_REQUIRE(std::ranges::count(session->snapshots(), MaterialId{9},
                          [](const auto& value) { return value.material_id.value_or(MaterialId{}); })
        == 3);

    const ContentBundle bundle = load_real_bundle();
    for (const BodyDefinition& definition : bundle.level.bodies) {
        const auto found = std::ranges::find_if(session->snapshots(), [&](const auto& snapshot) {
            return snapshot.entity_id == definition.entity_id
                && snapshot.part_id == definition.part_id;
        });
        NINHO_SIM_REQUIRE(found != session->snapshots().end());
        NINHO_SIM_REQUIRE(found->body_type == definition.body_type);
        NINHO_SIM_REQUIRE(found->material_id == definition.material_id);
        NINHO_SIM_REQUIRE(found->surface_id == definition.surface_id);
        NINHO_SIM_REQUIRE(found->enemy_archetype_id == definition.enemy_archetype_id);
        NINHO_SIM_REQUIRE(found->shape.type == definition.shape.type);
        NINHO_SIM_REQUIRE(found->shape.half_extents_m == definition.shape.half_extents_m);
        NINHO_SIM_REQUIRE(found->shape.radius_m == definition.shape.radius_m);
        NINHO_SIM_REQUIRE(found->visual_id == definition.visual.asset_id);
        const double expected_mass = definition.body_type == BodyType::Static
            ? 0.0
            : definition.density_kg_m3 * 8.0 * definition.shape.half_extents_m[0]
                * definition.shape.half_extents_m[1] * definition.shape.half_extents_m[2];
        NINHO_SIM_REQUIRE(std::abs(found->mass_kg - expected_mass) <= 0.02);
    }

    NINHO_SIM_REQUIRE(
        ninho::simulation::detail::physics_material_tag(MaterialId{1}) == 1U);
    NINHO_SIM_REQUIRE(
        ninho::simulation::detail::physics_surface_tag(SurfaceId{1001})
        == ((std::uint64_t{1} << 63U) | 1001U));
    NINHO_SIM_REQUIRE(ninho::simulation::detail::physics_material_tag(
                          MaterialId{std::numeric_limits<std::uint32_t>::max()})
        != ninho::simulation::detail::physics_surface_tag(SurfaceId{0}));
}

NINHO_SIM_TEST("session bootstrap globally orders planet and level bodies by domain identity")
{
    for (const EntityId planet_id : {EntityId{1}, EntityId{500}}) {
        ContentBundle bundle = load_real_bundle();
        bundle.level.planet.entity_id = planet_id;
        auto first = SimulationSession::create(
            bundle.materials, bundle.archetypes, bundle.level);
        auto second = SimulationSession::create(
            bundle.materials, bundle.archetypes, bundle.level);
        NINHO_SIM_REQUIRE(first.ok() && second.ok());
        NINHO_SIM_REQUIRE(first.value->canonical_hash_v2() == second.value->canonical_hash_v2());
        NINHO_SIM_REQUIRE(std::ranges::equal(
            first.value->snapshots(), second.value->snapshots()));
        NINHO_SIM_REQUIRE(std::ranges::is_sorted(
            first.value->snapshots(), {}, [](const auto& snapshot) {
                return std::pair{snapshot.entity_id, snapshot.part_id};
            }));
        const auto planet = std::ranges::find_if(first.value->snapshots(), [&](const auto& snapshot) {
            return snapshot.entity_id == planet_id && snapshot.part_id == PartId{0};
        });
        NINHO_SIM_REQUIRE(planet != first.value->snapshots().end());
        NINHO_SIM_REQUIRE(planet->shape.type == ShapeType::Sphere);
    }
}

NINHO_SIM_TEST("session bootstrap rejects exact planet and level domain identity collision")
{
    ContentBundle bundle = load_real_bundle();
    bundle.level.bodies.front().entity_id = bundle.level.planet.entity_id;
    bundle.level.bodies.front().part_id = PartId{0};
    const auto created = SimulationSession::create(
        bundle.materials, bundle.archetypes, bundle.level);
    NINHO_SIM_REQUIRE(!created.ok());
    NINHO_SIM_REQUIRE(created.error.code == ContentErrorCode::InvalidInvariant);
}

NINHO_SIM_TEST("session bootstrap rejects typed duplicate bird roster archetypes atomically")
{
    for (const auto counts : {std::pair{1U, 2U}, std::pair{2U, 1U}}) {
        ContentBundle bundle = load_real_bundle();
        bundle.level.bird_roster.front().count = counts.first;
        bundle.level.bird_roster.push_back(
            {bundle.level.bird_roster.front().bird_archetype_id, counts.second});
        const auto created = SimulationSession::create(
            bundle.materials, bundle.archetypes, bundle.level);
        NINHO_SIM_REQUIRE(!created.ok());
        NINHO_SIM_REQUIRE(created.error.code == ContentErrorCode::DuplicateId);
        NINHO_SIM_REQUIRE(created.error.pointer == "/bird_roster/1/bird_archetype_id");
    }
}

NINHO_SIM_TEST("session bootstrap weld frames preserve manifest poses on first isolated solver step")
{
    const auto session = create_real_session();
    NINHO_SIM_REQUIRE(session->structural_joints().size() == 18U);
    for (const StructuralJointSnapshot& joint : session->structural_joints()) {
        require_weld_does_not_snap(*session, joint);
    }

    ContentBundle rotated = load_real_bundle();
    const double sine = std::sin(std::numbers::pi / 8.0);
    const double cosine = std::cos(std::numbers::pi / 8.0);
    rotated.level.bodies.at(1).transform.rotation_xyzw = {0.0, sine, 0.0, cosine};
    rotated.level.bodies.at(5).transform.rotation_xyzw = {sine, 0.0, 0.0, cosine};
    const auto rotated_session = SimulationSession::create(
        rotated.materials, rotated.archetypes, rotated.level);
    NINHO_SIM_REQUIRE(rotated_session.ok());
    require_weld_does_not_snap(
        *rotated_session.value, rotated_session.value->structural_joints().front());
}

NINHO_SIM_TEST("session reconfigure failure preserves canonical state byte for byte")
{
    auto session = create_real_session();
    const auto bytes_before = session->canonical_state_v2();
    const auto hash_before = session->canonical_hash_v2();
    const auto snapshots_before = std::vector<EntitySnapshot>{
        session->snapshots().begin(), session->snapshots().end()};

    ContentBundle invalid = load_real_bundle();
    invalid.level.bodies.front().surface_id = SurfaceId{999999};
    const SessionStatus status = session->reconfigure(
        invalid.materials, invalid.archetypes, invalid.level);
    NINHO_SIM_REQUIRE(!status.ok());
    NINHO_SIM_REQUIRE(session->canonical_state_v2() == bytes_before);
    NINHO_SIM_REQUIRE(session->canonical_hash_v2() == hash_before);
    NINHO_SIM_REQUIRE(std::ranges::equal(session->snapshots(), snapshots_before));
    NINHO_SIM_REQUIRE(session->events().empty());
}

NINHO_SIM_TEST("session reconfigure valid content swaps the complete immutable configuration")
{
    auto session = create_real_session();
    const auto original_hash = session->canonical_hash_v2();
    ContentBundle replacement = load_real_bundle();
    replacement.level.id = "first_orbit_reconfigured";
    replacement.level.planet.surface_gravity_m_s2 = 8.75;
    NINHO_SIM_REQUIRE(session->reconfigure(
        replacement.materials, replacement.archetypes, replacement.level).ok());
    NINHO_SIM_REQUIRE(session->canonical_hash_v2() != original_hash);
    const auto configured_hash = session->canonical_hash_v2();
    NINHO_SIM_REQUIRE(session->tick().ok());
    NINHO_SIM_REQUIRE(session->restart().ok());
    NINHO_SIM_REQUIRE(session->canonical_hash_v2() == configured_hash);
}

NINHO_SIM_TEST("session fault latches exact canonical diagnostic without throwing and restart recovers")
{
    static_assert(!noexcept(std::declval<SimulationSession&>().tick()));
    static_assert(!noexcept(std::declval<SimulationSession&>().restart()));
    static_assert(!noexcept(std::declval<SimulationSession&>().reconfigure(
        std::declval<const MaterialCatalog&>(),
        std::declval<const ArchetypeCatalog&>(),
        std::declval<const LevelManifest&>())));
    static_assert(!noexcept(SimulationSession::create(
        std::declval<const MaterialCatalog&>(),
        std::declval<const ArchetypeCatalog&>(),
        std::declval<const LevelManifest&>())));

    auto session = create_real_session();
    const auto canonical_before_fault = session->canonical_state_v2();
    ninho::simulation::detail::SessionTestFacade::fail_next_canonical_refresh(
        *session, "injected canonical tick failure");
    const SessionStatus first = session->tick();
    NINHO_SIM_REQUIRE(!first.ok());
    NINHO_SIM_REQUIRE(first.error.code == ContentErrorCode::InternalError);
    NINHO_SIM_REQUIRE(first.error.message == "injected canonical tick failure");
    NINHO_SIM_REQUIRE(session->state().phase == SessionPhase::Faulted);
    NINHO_SIM_REQUIRE(session->state().outcome == Outcome::None);
    NINHO_SIM_REQUIRE(session->canonical_state_v2() == canonical_before_fault);

    const SessionStatus repeated = session->tick();
    NINHO_SIM_REQUIRE(!repeated.ok());
    NINHO_SIM_REQUIRE(repeated.error.code == first.error.code);
    NINHO_SIM_REQUIRE(repeated.error.pointer == first.error.pointer);
    NINHO_SIM_REQUIRE(repeated.error.message == first.error.message);

    NINHO_SIM_REQUIRE(session->restart().ok());
    NINHO_SIM_REQUIRE(session->state().phase == SessionPhase::Inspection);
    NINHO_SIM_REQUIRE(session->state().outcome == Outcome::None);
    NINHO_SIM_REQUIRE(session->tick().ok());

    ninho::simulation::detail::SessionTestFacade::fail_next_canonical_refresh(
        *session, "second injected failure");
    NINHO_SIM_REQUIRE(!session->tick().ok());
    ContentBundle replacement = load_real_bundle();
    replacement.level.id += "_recovered";
    NINHO_SIM_REQUIRE(session->reconfigure(
        replacement.materials, replacement.archetypes, replacement.level).ok());
    NINHO_SIM_REQUIRE(session->state().phase == SessionPhase::Inspection);
    NINHO_SIM_REQUIRE(session->state().tick == TickIndex{0});
    NINHO_SIM_REQUIRE(session->tick().ok());
}

NINHO_SIM_TEST("session canonical state v2 has explicit little endian fixed point and UTF8 layout")
{
    ContentBundle bundle = load_real_bundle();
    bundle.level.id = std::string{"\xC3\xB3rbita_\xCE\xB2"};
    bundle.level.bodies.front().transform.position_m = {1.234567, -0.0, -2.345678};
    const auto created = SimulationSession::create(
        bundle.materials, bundle.archetypes, bundle.level);
    NINHO_SIM_REQUIRE(created.ok());
    const auto& bytes = created.value->canonical_state_v2();
    NINHO_SIM_REQUIRE(bytes.size() > 32U);
    NINHO_SIM_REQUIRE(bytes[0] == 18U);
    NINHO_SIM_REQUIRE(bytes[1] == 0U && bytes[2] == 0U && bytes[3] == 0U);

    CanonicalReader reader{bytes};
    NINHO_SIM_REQUIRE(reader.text() == "canonical_state_v2");
    NINHO_SIM_REQUIRE(reader.u64() == 0U);
    NINHO_SIM_REQUIRE(reader.u8() == static_cast<std::uint8_t>(SessionPhase::Inspection));
    NINHO_SIM_REQUIRE(reader.u8() == static_cast<std::uint8_t>(Outcome::None));
    NINHO_SIM_REQUIRE(reader.u32() == 3U);
    NINHO_SIM_REQUIRE(reader.u64() == 1U);
    NINHO_SIM_REQUIRE(reader.u64() == 1U);
    NINHO_SIM_REQUIRE(reader.text() == bundle.level.id);
    const auto material_blob = reader.blob();
    const auto archetype_blob = reader.blob();
    const auto level_blob = reader.blob();
    NINHO_SIM_REQUIRE(!material_blob.empty());
    NINHO_SIM_REQUIRE(!archetype_blob.empty());
    NINHO_SIM_REQUIRE(!level_blob.empty());
    const std::string material_text{
        reinterpret_cast<const char*>(material_blob.data()), material_blob.size()};
    NINHO_SIM_REQUIRE(material_text != to_canonical_json(bundle.materials));

    const auto body_count = reader.u32();
    NINHO_SIM_REQUIRE(body_count == 23U);
    bool found_platform = false;
    for (std::uint32_t body_index = 0; body_index < body_count; ++body_index) {
        const auto entity = reader.u32();
        static_cast<void>(reader.u32());
        static_cast<void>(reader.u8());
        for (int optional_id = 0; optional_id < 3; ++optional_id) {
            if (reader.u8() != 0U) {
                static_cast<void>(reader.u32());
            }
        }
        static_cast<void>(reader.u8());
        for (int shape_value = 0; shape_value < 4; ++shape_value) {
            static_cast<void>(reader.i64());
        }
        static_cast<void>(reader.text());
        const auto x = reader.i64();
        const auto y = reader.i64();
        const auto z = reader.i64();
        for (int component = 0; component < 4 + 3 + 3; ++component) {
            static_cast<void>(reader.i64());
        }
        static_cast<void>(reader.i64());
        static_cast<void>(reader.u8());
        static_cast<void>(reader.u8());
        NINHO_SIM_REQUIRE(reader.u8() == 0U);
        if (entity == 10U) {
            found_platform = true;
            NINHO_SIM_REQUIRE(x == std::llround(
                static_cast<double>(static_cast<float>(1.234567)) * 100000.0));
            NINHO_SIM_REQUIRE(y == 0);
            NINHO_SIM_REQUIRE(z == std::llround(
                static_cast<double>(static_cast<float>(-2.345678)) * 100000.0));
        }
    }
    NINHO_SIM_REQUIRE(found_platform);
}

NINHO_SIM_TEST("session canonical state contract is explicitly versioned as v2")
{
    auto session = create_real_session();
    NINHO_SIM_REQUIRE(session->enqueue(BeginAimCommand{}).ok());
    NINHO_SIM_REQUIRE(session->tick().ok());
    NINHO_SIM_REQUIRE(session->enqueue(SetAimCommand{AimState{
        {-13.0F, 0.0F, 0.0F}, {0.0F, 1.0F, 0.0F}, 10.5}}).ok());
    NINHO_SIM_REQUIRE(session->enqueue(LaunchCommand{}).ok());
    NINHO_SIM_REQUIRE(session->tick().ok());
    const auto& bytes = session->canonical_state_v2();
    CanonicalReader reader{bytes};
    NINHO_SIM_REQUIRE(reader.text() == "canonical_state_v2");
    NINHO_SIM_REQUIRE(fnv1a64(bytes) == session->canonical_hash_v2());
    constexpr std::uint64_t canonical_state_v2_golden = 16778877272428821006ULL;
    if (session->canonical_hash_v2() != canonical_state_v2_golden) {
        test::fail(__FILE__, __LINE__,
            "canonical_state_v2 golden mismatch: actual="
                + std::to_string(session->canonical_hash_v2()));
    }
}

NINHO_SIM_TEST("session canonical cache preserves v2 bytes and reuses immutable bundle blobs")
{
    using detail::SessionTestFacade;
    auto session = create_real_session();
    const auto reference = SessionTestFacade::canonical_state_uncached(*session);
    NINHO_SIM_REQUIRE(reference == session->canonical_state_v2());
    NINHO_SIM_REQUIRE(fnv1a64(reference) == session->canonical_hash_v2());
    NINHO_SIM_REQUIRE(
        SessionTestFacade::canonical_static_content_build_count(*session) == 1U);

    for (std::size_t refresh = 0; refresh < 64U; ++refresh) {
        SessionTestFacade::refresh_canonical_state(*session);
        NINHO_SIM_REQUIRE(reference == session->canonical_state_v2());
        NINHO_SIM_REQUIRE(
            SessionTestFacade::canonical_static_content_build_count(*session) == 1U);
    }

    ContentBundle replacement = load_real_bundle();
    replacement.level.id += "_canonical_cache_reconfigured";
    replacement.level.planet.surface_gravity_m_s2 = 8.75;
    NINHO_SIM_REQUIRE(session->reconfigure(
        replacement.materials, replacement.archetypes, replacement.level).ok());
    const auto reconfigured_reference =
        SessionTestFacade::canonical_state_uncached(*session);
    NINHO_SIM_REQUIRE(reconfigured_reference == session->canonical_state_v2());
    NINHO_SIM_REQUIRE(fnv1a64(reconfigured_reference) == session->canonical_hash_v2());
    NINHO_SIM_REQUIRE(
        SessionTestFacade::canonical_static_content_build_count(*session) == 1U);
    NINHO_SIM_REQUIRE(reconfigured_reference != reference);
}

NINHO_SIM_TEST("session canonical cache focal benchmark avoids static reserialization")
{
    using detail::SessionTestFacade;
    using Clock = std::chrono::steady_clock;
    constexpr std::size_t iterations = 256U;
    auto session = create_real_session();
    SessionTestFacade::refresh_canonical_state(*session);
    static_cast<void>(SessionTestFacade::canonical_state_uncached(*session));

    std::uint64_t cached_checksum{};
    const auto cached_start = Clock::now();
    for (std::size_t iteration = 0; iteration < iterations; ++iteration) {
        SessionTestFacade::refresh_canonical_state(*session);
        cached_checksum += session->canonical_hash_v2();
    }
    const auto cached_elapsed = Clock::now() - cached_start;

    std::uint64_t uncached_checksum{};
    const auto uncached_start = Clock::now();
    for (std::size_t iteration = 0; iteration < iterations; ++iteration) {
        const auto bytes = SessionTestFacade::canonical_state_uncached(*session);
        uncached_checksum += fnv1a64(bytes);
    }
    const auto uncached_elapsed = Clock::now() - uncached_start;

    const auto cached_us = std::chrono::duration_cast<std::chrono::microseconds>(
        cached_elapsed).count();
    const auto uncached_us = std::chrono::duration_cast<std::chrono::microseconds>(
        uncached_elapsed).count();
    std::cout << "[BENCH] canonical_cache iterations=" << iterations
              << " cached_us=" << cached_us
              << " uncached_us=" << uncached_us << '\n';
    NINHO_SIM_REQUIRE(cached_checksum == uncached_checksum);
    NINHO_SIM_REQUIRE(cached_us > 0 && uncached_us > 0);
    NINHO_SIM_REQUIRE(
        SessionTestFacade::canonical_static_content_build_count(*session) == 1U);
}

NINHO_SIM_TEST("session publishes snapshots once per tick with linear metadata reuse")
{
    using detail::SessionTestFacade;
    auto session = create_real_session();
    const auto initial_hash = session->canonical_hash_v2();
    const auto rebuilds_before = SessionTestFacade::snapshot_rebuild_count(*session);
    const auto visual_copies_before =
        SessionTestFacade::snapshot_visual_copy_count(*session);

    NINHO_SIM_REQUIRE(session->tick().ok());
    NINHO_SIM_REQUIRE(
        SessionTestFacade::snapshot_rebuild_count(*session) == rebuilds_before + 1U);
    NINHO_SIM_REQUIRE(
        SessionTestFacade::snapshot_visual_copy_count(*session) == visual_copies_before);
    NINHO_SIM_REQUIRE(std::ranges::equal(
        session->snapshots(), SessionTestFacade::snapshots_uncached(*session)));
    NINHO_SIM_REQUIRE(session->canonical_hash_v2() != initial_hash);
    NINHO_SIM_REQUIRE(fnv1a64(session->canonical_state_v2()) == session->canonical_hash_v2());
}

NINHO_SIM_TEST("session snapshot rebuild focal benchmark compares uncached quadratic reference")
{
    using detail::SessionTestFacade;
    using Clock = std::chrono::steady_clock;
    constexpr std::size_t iterations = 256U;
    auto session = create_real_session();

    const auto cached_start = Clock::now();
    for (std::size_t iteration = 0; iteration < iterations; ++iteration) {
        SessionTestFacade::rebuild_snapshots(*session);
    }
    const auto cached_elapsed = Clock::now() - cached_start;

    std::size_t uncached_checksum{};
    const auto uncached_start = Clock::now();
    for (std::size_t iteration = 0; iteration < iterations; ++iteration) {
        uncached_checksum += SessionTestFacade::snapshots_uncached(*session).size();
    }
    const auto uncached_elapsed = Clock::now() - uncached_start;

    const auto cached_us = std::chrono::duration_cast<std::chrono::microseconds>(
        cached_elapsed).count();
    const auto uncached_us = std::chrono::duration_cast<std::chrono::microseconds>(
        uncached_elapsed).count();
    std::cout << "[BENCH] snapshot_rebuild iterations=" << iterations
              << " cached_us=" << cached_us
              << " uncached_us=" << uncached_us << '\n';
    NINHO_SIM_REQUIRE(uncached_checksum == iterations * session->snapshots().size());
    NINHO_SIM_REQUIRE(std::ranges::equal(
        session->snapshots(), SessionTestFacade::snapshots_uncached(*session)));
    NINHO_SIM_REQUIRE(cached_us > 0 && uncached_us > 0);
}

NINHO_SIM_TEST("session canonical hash includes every immutable physical content document")
{
    const ContentBundle base = load_real_bundle();
    const auto base_session = SimulationSession::create(
        base.materials, base.archetypes, base.level);
    NINHO_SIM_REQUIRE(base_session.ok());
    const auto base_hash = base_session.value->canonical_hash_v2();

    auto require_distinct = [&](ContentBundle changed) {
        const auto session = SimulationSession::create(
            changed.materials, changed.archetypes, changed.level);
        NINHO_SIM_REQUIRE(session.ok());
        NINHO_SIM_REQUIRE(session.value->canonical_hash_v2() != base_hash);
    };

    ContentBundle friction = base;
    friction.materials.materials.front().friction += 0.01;
    require_distinct(std::move(friction));
    ContentBundle archetype = base;
    archetype.archetypes.abilities.front().max_acceleration_m_s2 += 0.5;
    require_distinct(std::move(archetype));
    ContentBundle gravity = base;
    gravity.level.planet.surface_gravity_m_s2 += 0.1;
    require_distinct(std::move(gravity));
    ContentBundle joint = base;
    joint.level.joints.front().force_limit_n += 1.0;
    require_distinct(std::move(joint));
    ContentBundle identity = base;
    identity.level.id += "_variant";
    require_distinct(std::move(identity));
}

NINHO_SIM_TEST("session canonical configuration is structural ordered and quantum stable")
{
    ContentBundle base = load_real_bundle();
    const auto base_session = SimulationSession::create(
        base.materials, base.archetypes, base.level);
    NINHO_SIM_REQUIRE(base_session.ok());
    const auto base_bytes = base_session.value->canonical_state_v2();
    const auto base_hash = base_session.value->canonical_hash_v2();

    ContentBundle shuffled = base;
    std::ranges::reverse(shuffled.materials.materials);
    std::ranges::reverse(shuffled.materials.surfaces);
    std::ranges::reverse(shuffled.archetypes.abilities);
    std::ranges::reverse(shuffled.archetypes.birds);
    std::ranges::reverse(shuffled.archetypes.weakpoints);
    std::ranges::reverse(shuffled.archetypes.enemies);
    std::ranges::reverse(shuffled.level.bird_roster);
    std::ranges::reverse(shuffled.level.free_body_ids);
    std::ranges::reverse(shuffled.level.bodies);
    std::ranges::reverse(shuffled.level.joints);
    std::ranges::reverse(shuffled.level.assemblies);
    std::ranges::reverse(shuffled.level.objectives);
    for (AssemblyDefinition& assembly : shuffled.level.assemblies) {
        std::ranges::reverse(assembly.body_ids);
        std::ranges::reverse(assembly.joint_ids);
    }
    const auto shuffled_session = SimulationSession::create(
        shuffled.materials, shuffled.archetypes, shuffled.level);
    NINHO_SIM_REQUIRE(shuffled_session.ok());
    NINHO_SIM_REQUIRE(shuffled_session.value->canonical_state_v2() == base_bytes);
    NINHO_SIM_REQUIRE(shuffled_session.value->canonical_hash_v2() == base_hash);

    ContentBundle below_quantum = base;
    below_quantum.materials.materials.front().toughness += 0.000004;
    const auto below = SimulationSession::create(
        below_quantum.materials, below_quantum.archetypes, below_quantum.level);
    NINHO_SIM_REQUIRE(below.ok());
    NINHO_SIM_REQUIRE(below.value->canonical_state_v2() == base_bytes);

    ContentBundle above_quantum = base;
    above_quantum.materials.materials.front().toughness += 0.000006;
    const auto above = SimulationSession::create(
        above_quantum.materials, above_quantum.archetypes, above_quantum.level);
    NINHO_SIM_REQUIRE(above.ok());
    NINHO_SIM_REQUIRE(above.value->canonical_hash_v2() != base_hash);
}

NINHO_SIM_TEST("session canonical quantizer validates finite range boundary and one quantum")
{
    using ninho::simulation::detail::SessionTestFacade;
    NINHO_SIM_REQUIRE(SessionTestFacade::quantize_canonical(0.0) == 0);
    NINHO_SIM_REQUIRE(SessionTestFacade::quantize_canonical(-0.0) == 0);
    const auto ordinary = SessionTestFacade::quantize_canonical(12.34567);
    NINHO_SIM_REQUIRE(
        SessionTestFacade::quantize_canonical(12.34568) == ordinary + 1);
    NINHO_SIM_REQUIRE(
        SessionTestFacade::quantize_canonical(9.223372036854774e13) > 0);

    const auto require_rejected = [](double value) {
        bool rejected = false;
        try {
            static_cast<void>(SessionTestFacade::quantize_canonical(value));
        } catch (const std::range_error&) {
            rejected = true;
        } catch (const std::invalid_argument&) {
            rejected = true;
        }
        NINHO_SIM_REQUIRE(rejected);
    };
    require_rejected(9.223372036854776e13);
    require_rejected(std::numeric_limits<double>::infinity());
    require_rejected(std::numeric_limits<double>::quiet_NaN());

    ContentBundle unrepresentable = load_real_bundle();
    unrepresentable.materials.materials.front().density_kg_m3 = 1.0e14;
    for (BodyDefinition& body : unrepresentable.level.bodies) {
        if (body.material_id == MaterialId{1}) {
            body.density_kg_m3 = 1.0e14;
        }
    }
    const auto rejected_session = SimulationSession::create(
        unrepresentable.materials, unrepresentable.archetypes, unrepresentable.level);
    NINHO_SIM_REQUIRE(!rejected_session.ok());

    auto invalid_state = create_real_session();
    SessionTestFacade::override_next_snapshot_mass(*invalid_state, 1.0e14);
    const SessionStatus state_status = invalid_state->tick();
    NINHO_SIM_REQUIRE(!state_status.ok());
    NINHO_SIM_REQUIRE(state_status.error.code == ContentErrorCode::InternalError);
    NINHO_SIM_REQUIRE(state_status.error.message.find("fixed-point range") != std::string::npos);
    NINHO_SIM_REQUIRE(invalid_state->state().phase == SessionPhase::Faulted);
    NINHO_SIM_REQUIRE(invalid_state->state().outcome == Outcome::None);
}

NINHO_SIM_TEST("session restart reproduces canonical state and allocator usage twenty times")
{
    auto session = create_real_session();
    const auto initial_bytes = session->canonical_state_v2();
    const auto initial_hash = session->canonical_hash_v2();
    const auto initial_snapshots = std::vector<EntitySnapshot>{
        session->snapshots().begin(), session->snapshots().end()};
    const auto initial_joints = std::vector<StructuralJointSnapshot>{
        session->structural_joints().begin(), session->structural_joints().end()};
    const auto initial_allocator_bytes = ninho::physics::detail::box3d_allocator_byte_count();

    for (int iteration = 0; iteration < 20; ++iteration) {
        NINHO_SIM_REQUIRE(session->tick().ok());
        NINHO_SIM_REQUIRE(session->restart().ok());
        NINHO_SIM_REQUIRE(session->state().tick == TickIndex{0});
        NINHO_SIM_REQUIRE(session->events().empty());
        NINHO_SIM_REQUIRE(session->canonical_state_v2() == initial_bytes);
        NINHO_SIM_REQUIRE(session->canonical_hash_v2() == initial_hash);
        NINHO_SIM_REQUIRE(std::ranges::equal(session->snapshots(), initial_snapshots));
        NINHO_SIM_REQUIRE(std::ranges::equal(session->structural_joints(), initial_joints));
        NINHO_SIM_REQUIRE(session->physics_metrics().body_count == 23);
        NINHO_SIM_REQUIRE(session->physics_metrics().joint_count == 18);
        NINHO_SIM_REQUIRE(
            ninho::physics::detail::box3d_allocator_byte_count() == initial_allocator_bytes);
    }
}

NINHO_SIM_TEST("session exposes authoritative objective integrity and ability readiness")
{
    auto session = create_real_session();
    const auto initial_targets = session->objective_target_statuses();
    NINHO_SIM_REQUIRE(initial_targets.size() == 1U);
    NINHO_SIM_REQUIRE(initial_targets.front().entity_id == EntityId{200});
    NINHO_SIM_REQUIRE(initial_targets.front().current_integrity == 100.0);
    NINHO_SIM_REQUIRE(initial_targets.front().maximum_integrity == 100.0);
    NINHO_SIM_REQUIRE(!initial_targets.front().neutralized);
    NINHO_SIM_REQUIRE(session->ability_readiness() == AbilityReadiness::Unavailable);

    NINHO_SIM_REQUIRE(session->enqueue(BeginAimCommand{}).ok());
    NINHO_SIM_REQUIRE(session->tick().ok());
    NINHO_SIM_REQUIRE(session->enqueue(LaunchCommand{}).ok());
    NINHO_SIM_REQUIRE(session->tick().ok());
    NINHO_SIM_REQUIRE(std::ranges::count(
                          session->snapshots(), true, &EntitySnapshot::is_projectile)
        == 1);
    const auto projectile = std::ranges::find(
        session->snapshots(), true, &EntitySnapshot::is_projectile);
    NINHO_SIM_REQUIRE(projectile != session->snapshots().end());
    NINHO_SIM_REQUIRE(projectile->entity_id.value() >= 0x80000000U);
    const TickIndex launched = detail::SessionTestFacade::projectile_launch_tick(*session);
    NINHO_SIM_REQUIRE(session->ability_readiness() == AbilityReadiness::Arming);
    while (session->state().tick < TickIndex{launched.value() + 9U}) {
        NINHO_SIM_REQUIRE(session->tick().ok());
    }
    NINHO_SIM_REQUIRE(session->ability_readiness() == AbilityReadiness::Armed);

    NINHO_SIM_REQUIRE(session->enqueue(ActivateAbilityCommand{}).ok());
    NINHO_SIM_REQUIRE(session->tick().ok());
    NINHO_SIM_REQUIRE(session->ability_readiness() == AbilityReadiness::Active);
    const TickIndex ability_end = detail::SessionTestFacade::ability_end_tick(*session);
    while (session->state().tick < ability_end) {
        NINHO_SIM_REQUIRE(session->tick().ok());
    }
    NINHO_SIM_REQUIRE(session->ability_readiness() == AbilityReadiness::Spent);
}

}
