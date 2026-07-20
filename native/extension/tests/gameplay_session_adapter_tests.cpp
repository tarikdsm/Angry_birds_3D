#include "test_framework.hpp"

#include <ninho/extension/gameplay_session_node.hpp>

#include "session_test_facade.hpp"

#include <nlohmann/json.hpp>

#include <array>
#include <filesystem>
#include <fstream>
#include <limits>
#include <regex>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <vector>

namespace ninho::extension::detail {

class GameplaySessionAdapterTestFacade {
public:
    [[nodiscard]] static simulation::SimulationSession& session(
        GameplaySessionAdapter& adapter) noexcept
    {
        return *adapter.session_;
    }

    static void refresh_frame(GameplaySessionAdapter& adapter)
    {
        adapter.capture_latest();
    }
};

}

namespace {

using json = nlohmann::json;
using ninho::extension::detail::GameplaySessionAdapter;
using ninho::extension::detail::GameplaySessionAdapterTestFacade;
using ninho::physics::Vec3;
using ninho::simulation::SessionPhase;

json material_catalog()
{
    return {
        {"schema_version", 2},
        {"materials", json::array({
            {{"id", 1}, {"key", "wood"}, {"response", "fibrous"},
             {"density_kg_m3", 520.0}, {"friction", 0.6},
             {"restitution", 0.1}, {"toughness", 0.5}},
        })},
        {"surfaces", json::array({
            {{"id", 1001}, {"key", "bird"}, {"density_kg_m3", 400.0},
             {"friction", 0.35}, {"restitution", 0.25}},
            {{"id", 1002}, {"key", "pig"}, {"density_kg_m3", 480.0},
             {"friction", 0.45}, {"restitution", 0.1}},
        })},
    };
}

json archetype_catalog()
{
    return {
        {"schema_version", 2},
        {"presentation_ids", json::array({"visual_bird", "icon_bird", "anim_bird"})},
        {"score_ids", json::array({"unused_bird"})},
        {"abilities", json::array({
            {{"id", 1}, {"key", "gravity_ability"}, {"kind", "gravity_field"},
             {"payload", {{"arm_ticks", 9}, {"duration_ticks", 60},
                          {"radius_m", 5.0}, {"max_body_mass_kg", 1000.0},
                          {"max_bodies", 16}, {"max_acceleration_m_s2", 20.0},
                          {"pulse_speed_m_s", 30.0}}}},
        })},
        {"birds", json::array({
            {{"id", 1}, {"key", "bird"}, {"projectile_visual_id", "visual_bird"},
             {"ability_id", 1}, {"surface_id", 1001}, {"mass_kg", 5.0},
             {"radius_m", 0.25}, {"friction", 0.35}, {"restitution", 0.25},
             {"bullet", true}, {"launch_speed_cap_m_s", 40.0},
             {"score_id", "unused_bird"}, {"icon_id", "icon_bird"},
             {"animation_id", "anim_bird"}},
        })},
        {"weakpoints", json::array({
            {{"id", 1}, {"key", "pig_front"},
             {"protected_direction", json::array({-1.0, 0.0, 0.0})},
             {"protected_cone_deg", 30.0}, {"protected_multiplier", 0.5},
             {"exposed_multiplier", 1.25}},
        })},
        {"enemies", json::array({
            {{"id", 1}, {"key", "pig"}, {"weakpoint_id", 1}, {"surface_id", 1002},
             {"mass_kg", 480.0}, {"integrity", 100.0},
             {"damage_energy_j_per_kg", 2.5}, {"max_damage", 50.0}},
        })},
    };
}

json level_manifest()
{
    return {
        {"schema_version", 2}, {"id", "uniform_minimum"}, {"world_id", "earth"},
        {"region_id", "test_region"}, {"camera_profile_id", "test_camera"},
        {"presentation_profile_id", "test_presentation"},
        {"world", {{"kind", "uniform"},
                   {"acceleration_m_s2", json::array({0.0, -9.81, 0.0})},
                   {"bounds", {{"min_m", json::array({-24.0, -12.0, -12.0})},
                               {"max_m", json::array({48.0, 32.0, 12.0})}}}}},
        {"slingshot", {{"asset_id", "slingshot"},
                       {"rest_position_m", json::array({-4.0, 2.0, 0.0})},
                       {"rest_rotation_xyzw", json::array({0.0, 0.0, 0.0, 1.0})},
                       {"spring_constant_n_m", 5200.0}, {"energy_efficiency", 0.9},
                       {"minimum_extension_m", 0.2}, {"maximum_extension_m", 4.25},
                       {"plane_policy", "gravity_vertical_camera_yaw"},
                       {"projectile_clearance_m", 0.0}, {"speed_ceiling_m_s", 40.0}}},
        {"bird_queue", json::array({1, 1})},
        {"scoring", {{"pig_points", 5000}, {"unused_bird_points", 10000},
                     {"star_thresholds", json::array({10000, 20000, 30000})},
                     {"chain_window_ticks", 45}, {"chain_multiplier_step", 0.25},
                     {"max_chain_multiplier", 3.0}}},
        {"free_body_ids", json::array({1})},
        {"bodies", json::array({
            {{"body_id", 1}, {"entity_id", 100}, {"part_id", 1},
             {"body_type", "dynamic"}, {"affected_by_world_gravity", true},
             {"material_id", nullptr}, {"surface_id", 1002},
             {"enemy_archetype_id", 1}, {"density_kg_m3", 480.0},
             {"transform", {{"position_m", json::array({8.0, 0.0, 0.0})},
                            {"rotation_xyzw", json::array({0.0, 0.0, 0.0, 1.0})}}},
             {"shape", {{"type", "box"},
                        {"half_extents_m", json::array({0.5, 0.5, 0.5})}}},
             {"visual", {{"asset_id", "pig"},
                         {"bounds_m", json::array({1.0, 1.0, 1.0})}}}},
        })},
        {"joints", json::array()}, {"assemblies", json::array()},
        {"triggers", json::array()},
        {"objectives", json::array({
            {{"id", 1}, {"kind", "neutralize_entity"}, {"target_entity_id", 100}},
        })},
        {"settle_policy", {{"linear_speed_m_s", 0.05},
                           {"angular_speed_rad_s", 0.05}, {"rest_ticks", 60}}},
        {"watchdog_ticks", 1500},
    };
}

struct JsonFixture {
    std::string materials = material_catalog().dump();
    std::string archetypes = archetype_catalog().dump();
    std::string level = level_manifest().dump();
};

void require_configured(GameplaySessionAdapter& adapter, const JsonFixture& json)
{
    if (adapter.configure(json.materials, json.archetypes, json.level)) {
        return;
    }
    if (adapter.fault()) {
        throw std::runtime_error{
            std::string{adapter.fault()->code()} + ": "
            + std::string{adapter.fault()->message()}};
    }
    throw std::runtime_error{"configuration failed without fault"};
}

std::string read_source_file(std::string_view relative_path)
{
    std::ifstream stream{
        std::filesystem::path{NINHO_SOURCE_DIR} / relative_path, std::ios::binary};
    NINHO_REQUIRE(stream.is_open());
    std::ostringstream content;
    content << stream.rdbuf();
    return content.str();
}

std::string gameplay_frame_dictionary_body(const std::string& source)
{
    constexpr std::string_view signature =
        "[[nodiscard]] godot::Dictionary gameplay_frame_dictionary("
        "const detail::SessionFrameData& frame)";
    const std::size_t signature_position = source.find(signature);
    NINHO_REQUIRE(signature_position != std::string::npos);
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

static_assert(noexcept(std::declval<GameplaySessionAdapter&>().configure(
    std::declval<std::string_view>(), std::declval<std::string_view>(),
    std::declval<std::string_view>())));
static_assert(noexcept(std::declval<GameplaySessionAdapter&>().advance(0.0)));
static_assert(noexcept(std::declval<GameplaySessionAdapter&>().consume_frame()));
static_assert(noexcept(std::declval<GameplaySessionAdapter&>().queue_begin_grab(
    std::declval<Vec3>())));
static_assert(noexcept(std::declval<GameplaySessionAdapter&>().queue_pull(0.0, 0.0)));
static_assert(noexcept(std::declval<GameplaySessionAdapter&>().queue_release()));
static_assert(noexcept(std::declval<GameplaySessionAdapter&>().queue_activate_ability()));
static_assert(noexcept(std::declval<GameplaySessionAdapter&>().queue_cancel_grab()));
static_assert(noexcept(std::declval<GameplaySessionAdapter&>().restart()));

NINHO_TEST("gameplay adapter accepts only grab pull and release launcher commands")
{
    const JsonFixture json;
    GameplaySessionAdapter adapter;
    require_configured(adapter, json);
    const auto initial = adapter.peek_frame();
    NINHO_REQUIRE(initial.bird_queue.size() == 2U);
    NINHO_REQUIRE(initial.current_bird == ninho::simulation::BirdArchetypeId{1});
    NINHO_REQUIRE(initial.gravity_kind == "uniform");
    NINHO_REQUIRE((initial.local_gravity_m_s2 == Vec3{0.0F, -9.81F, 0.0F}));
    NINHO_REQUIRE(!initial.locked_plane.has_value());
    NINHO_REQUIRE(!initial.shot.has_value());
    NINHO_REQUIRE(initial.projectiles.empty());
    NINHO_REQUIRE(initial.score == 0U);
    NINHO_REQUIRE(initial.stars == 0U);

    NINHO_REQUIRE(adapter.queue_begin_grab(Vec3{1.0F, 0.0F, 0.0F}));
    NINHO_REQUIRE(adapter.advance(1.0 / 60.0));
    NINHO_REQUIRE(adapter.peek_frame().state.phase == SessionPhase::Grabbed);
    NINHO_REQUIRE(adapter.peek_frame().state.launcher.has_value());
    NINHO_REQUIRE(adapter.peek_frame().locked_plane.has_value());

    NINHO_REQUIRE(adapter.queue_pull(-2.0, 1.0));
    NINHO_REQUIRE(adapter.advance(1.0 / 60.0));
    NINHO_REQUIRE(adapter.peek_frame().preview.has_value());
    NINHO_REQUIRE(adapter.queue_release());
    NINHO_REQUIRE(adapter.advance(1.0 / 60.0));
    NINHO_REQUIRE(adapter.peek_frame().state.phase == SessionPhase::FlightAbility);
    NINHO_REQUIRE(adapter.peek_frame().locked_plane.has_value());
    NINHO_REQUIRE(adapter.peek_frame().shot.has_value());
    NINHO_REQUIRE(adapter.peek_frame().shot->bird_archetype_id.value() == 1U);
    NINHO_REQUIRE(adapter.peek_frame().projectiles.size() == 1U);
}

NINHO_TEST("gameplay adapter rejects nonfinite pull without faulting or stopping the session")
{
    const JsonFixture json;
    GameplaySessionAdapter adapter;
    require_configured(adapter, json);
    NINHO_REQUIRE(!adapter.queue_pull(
        std::numeric_limits<double>::infinity(), 0.0));
    NINHO_REQUIRE(!adapter.fault().has_value());
    NINHO_REQUIRE(adapter.queue_begin_grab(Vec3{1.0F, 0.0F, 0.0F}));
    NINHO_REQUIRE(adapter.advance(1.0 / 60.0));
}

NINHO_TEST("gameplay adapter exposes cancel grab and activate ability without legacy aim commands")
{
    const JsonFixture json;
    GameplaySessionAdapter adapter;
    require_configured(adapter, json);

    NINHO_REQUIRE(adapter.queue_begin_grab(Vec3{1.0F, 0.0F, 0.0F}));
    NINHO_REQUIRE(adapter.advance(1.0 / 60.0));
    NINHO_REQUIRE(adapter.peek_frame().state.phase == SessionPhase::Grabbed);
    NINHO_REQUIRE(adapter.queue_cancel_grab());
    NINHO_REQUIRE(adapter.advance(1.0 / 60.0));
    NINHO_REQUIRE(adapter.peek_frame().state.phase == SessionPhase::Inspection);
    NINHO_REQUIRE(!adapter.peek_frame().locked_plane.has_value());

    NINHO_REQUIRE(adapter.queue_begin_grab(Vec3{1.0F, 0.0F, 0.0F}));
    NINHO_REQUIRE(adapter.queue_pull(-2.0, 1.0));
    NINHO_REQUIRE(adapter.queue_release());
    NINHO_REQUIRE(adapter.advance(3.0 / 60.0));
    NINHO_REQUIRE(adapter.peek_frame().state.phase == SessionPhase::FlightAbility);
    NINHO_REQUIRE(adapter.queue_activate_ability());
    NINHO_REQUIRE(adapter.advance(1.0 / 60.0));
    NINHO_REQUIRE(!adapter.fault().has_value());
}

NINHO_TEST("gameplay adapter keeps configuration atomic and restart recovers the last valid bundle")
{
    const JsonFixture json;
    GameplaySessionAdapter adapter;
    require_configured(adapter, json);
    NINHO_REQUIRE(adapter.queue_begin_grab(Vec3{1.0F, 0.0F, 0.0F}));
    NINHO_REQUIRE(adapter.queue_pull(-2.0, 1.0));
    NINHO_REQUIRE(adapter.queue_release());
    NINHO_REQUIRE(adapter.advance(3.0 / 60.0));
    const auto launched = adapter.consume_frame();
    NINHO_REQUIRE(launched.shot.has_value());

    NINHO_REQUIRE(!adapter.configure("{", json.archetypes, json.level));
    NINHO_REQUIRE(adapter.configured());
    NINHO_REQUIRE(adapter.fault().has_value());
    NINHO_REQUIRE(adapter.peek_frame().shot == launched.shot);
    NINHO_REQUIRE(adapter.peek_frame().locked_plane == launched.locked_plane);

    NINHO_REQUIRE(adapter.restart());
    NINHO_REQUIRE(!adapter.fault().has_value());
    NINHO_REQUIRE(adapter.peek_frame().state.tick.value() == 0U);
    NINHO_REQUIRE(!adapter.peek_frame().shot.has_value());
    NINHO_REQUIRE(!adapter.peek_frame().locked_plane.has_value());

    NINHO_REQUIRE(adapter.queue_begin_grab(Vec3{1.0F, 0.0F, 0.0F}));
    NINHO_REQUIRE(adapter.queue_pull(-2.0, 1.0));
    NINHO_REQUIRE(adapter.queue_release());
    NINHO_REQUIRE(adapter.advance(3.0 / 60.0));
    NINHO_REQUIRE(adapter.peek_frame().shot.has_value());
    NINHO_REQUIRE(adapter.configure(json.materials, json.archetypes, json.level));
    NINHO_REQUIRE(!adapter.peek_frame().shot.has_value());
    NINHO_REQUIRE(!adapter.peek_frame().locked_plane.has_value());
}

NINHO_TEST("gameplay frame consumption clears events once and retains latest snapshots")
{
    const JsonFixture json;
    GameplaySessionAdapter adapter;
    require_configured(adapter, json);
    NINHO_REQUIRE(adapter.queue_begin_grab(Vec3{1.0F, 0.0F, 0.0F}));
    NINHO_REQUIRE(adapter.queue_pull(-2.0, 1.0));
    NINHO_REQUIRE(adapter.queue_release());
    NINHO_REQUIRE(adapter.advance(3.0 / 60.0));

    const auto first = adapter.consume_frame();
    NINHO_REQUIRE(!first.events.empty());
    NINHO_REQUIRE(first.shot.has_value());
    NINHO_REQUIRE(first.shot->bird_archetype_id.value() == 1U);
    const auto second = adapter.consume_frame();
    NINHO_REQUIRE(second.events.empty());
    NINHO_REQUIRE(second.snapshots == first.snapshots);
    NINHO_REQUIRE(second.state.tick == first.state.tick);
}

NINHO_TEST("gameplay frame follows authoritative shot lifecycle and ordered membership")
{
    using ninho::simulation::detail::SessionTestFacade;
    const JsonFixture json;
    GameplaySessionAdapter adapter;
    require_configured(adapter, json);
    NINHO_REQUIRE(adapter.queue_begin_grab(Vec3{1.0F, 0.0F, 0.0F}));
    NINHO_REQUIRE(adapter.queue_pull(-2.0, 1.0));
    NINHO_REQUIRE(adapter.queue_release());
    NINHO_REQUIRE(adapter.advance(3.0 / 60.0));

    auto& session = GameplaySessionAdapterTestFacade::session(adapter);
    const auto authoritative = session.shot_state();
    NINHO_REQUIRE(authoritative.has_value());
    NINHO_REQUIRE(SessionTestFacade::append_projectile_body_for_testing(
        session, ninho::simulation::EntityId{0x80000003U},
        Vec3{0.0F, 10.0F, 1.0F}, Vec3{5.0F, 0.0F, 0.0F}));
    NINHO_REQUIRE(SessionTestFacade::append_projectile_body_for_testing(
        session, ninho::simulation::EntityId{0x80000002U},
        Vec3{0.0F, 10.0F, -1.0F}, Vec3{5.0F, 0.0F, 0.0F}));
    GameplaySessionAdapterTestFacade::refresh_frame(adapter);

    const auto expanded = adapter.peek_frame();
    NINHO_REQUIRE(expanded.shot.has_value());
    NINHO_REQUIRE(expanded.shot->shot_id == authoritative->shot_id);
    NINHO_REQUIRE(expanded.shot->bird_archetype_id
        == authoritative->bird_archetype_id);
    NINHO_REQUIRE((expanded.shot->projectile_ids == std::vector{
        ninho::simulation::EntityId{0x80000000U},
        ninho::simulation::EntityId{0x80000002U},
        ninho::simulation::EntityId{0x80000003U}}));
    NINHO_REQUIRE(expanded.projectiles.size() == 1U);
    NINHO_REQUIRE(expanded.locked_plane.has_value());

    SessionTestFacade::finish_projectile(session);
    NINHO_REQUIRE(adapter.advance(1.0 / 60.0));
    NINHO_REQUIRE(adapter.peek_frame().state.phase == SessionPhase::FlightAbility);
    NINHO_REQUIRE(adapter.peek_frame().shot.has_value());
    NINHO_REQUIRE(adapter.peek_frame().locked_plane.has_value());
    NINHO_REQUIRE((adapter.peek_frame().shot->projectile_ids == std::vector{
        ninho::simulation::EntityId{0x80000002U},
        ninho::simulation::EntityId{0x80000003U}}));
    NINHO_REQUIRE(adapter.peek_frame().projectiles.size() == 2U);

    SessionTestFacade::finish_projectile(
        session, ninho::simulation::EntityId{0x80000002U});
    NINHO_REQUIRE(adapter.advance(1.0 / 60.0));
    NINHO_REQUIRE(adapter.peek_frame().state.phase == SessionPhase::FlightAbility);
    NINHO_REQUIRE((adapter.peek_frame().shot->projectile_ids == std::vector{
        ninho::simulation::EntityId{0x80000003U}}));
    NINHO_REQUIRE(adapter.peek_frame().projectiles.size() == 1U);

    SessionTestFacade::finish_projectile(
        session, ninho::simulation::EntityId{0x80000003U});
    NINHO_REQUIRE(adapter.advance(1.0 / 60.0));
    NINHO_REQUIRE(adapter.peek_frame().state.phase == SessionPhase::Resolution);
    NINHO_REQUIRE((adapter.peek_frame().shot->projectile_ids == std::vector{
        ninho::simulation::EntityId{0x80000003U}}));
    NINHO_REQUIRE(adapter.peek_frame().projectiles.empty());

    for (int tick = 0;
         tick < 64 && adapter.peek_frame().state.phase != SessionPhase::Evaluation;
         ++tick) {
        NINHO_REQUIRE(adapter.advance(1.0 / 60.0));
        NINHO_REQUIRE(adapter.peek_frame().shot.has_value());
        NINHO_REQUIRE(adapter.peek_frame().locked_plane.has_value());
    }
    NINHO_REQUIRE(adapter.peek_frame().state.phase == SessionPhase::Evaluation);
    NINHO_REQUIRE(adapter.peek_frame().shot.has_value());

    NINHO_REQUIRE(adapter.advance(1.0 / 60.0));
    NINHO_REQUIRE(adapter.peek_frame().state.phase == SessionPhase::Inspection);
    NINHO_REQUIRE(!adapter.peek_frame().shot.has_value());
    NINHO_REQUIRE(!adapter.peek_frame().locked_plane.has_value());
    NINHO_REQUIRE(adapter.peek_frame().projectiles.empty());
}

NINHO_TEST("gameplay adapter has no external shot identity or event inference cache")
{
    const std::string header = read_source_file(
        "native/extension/include/ninho/extension/gameplay_session_node.hpp");
    const std::string source = read_source_file(
        "native/extension/src/gameplay_session_node.cpp");
    NINHO_REQUIRE(header.find("active_bird_") == std::string::npos);
    NINHO_REQUIRE(header.find("pending_release_bird_") == std::string::npos);
    NINHO_REQUIRE(source.find("active_bird_") == std::string::npos);
    NINHO_REQUIRE(source.find("pending_release_bird_") == std::string::npos);
    NINHO_REQUIRE(source.find("simulation::DomainEventKind::BirdLaunched")
        == std::string::npos);
    NINHO_REQUIRE(source.find("session_->shot_state()") != std::string::npos);
}

NINHO_TEST("gameplay nodes expose typed mass speed and split domain events")
{
    const std::string gameplay = read_source_file(
        "native/extension/src/gameplay_session_node.cpp");
    const std::string orbital = read_source_file(
        "native/extension/src/orbital_session_node.cpp");
    constexpr std::string_view mass_mapping =
        "case MassChanged: return \"mass_changed\";";
    constexpr std::string_view speed_mapping =
        "case SpeedChanged: return \"speed_changed\";";
    constexpr std::string_view split_mapping =
        "case ProjectileSplit: return \"projectile_split\";";
    constexpr std::string_view spawned_mapping =
        "case ProjectileSpawned: return \"projectile_spawned\";";
    constexpr std::string_view delta_mapping =
        "result[\"delta_velocity\"] = detail::to_godot(event.delta_velocity_m_s);";
    NINHO_REQUIRE(gameplay.find(mass_mapping) != std::string::npos);
    NINHO_REQUIRE(orbital.find(mass_mapping) != std::string::npos);
    NINHO_REQUIRE(gameplay.find(speed_mapping) != std::string::npos);
    NINHO_REQUIRE(orbital.find(speed_mapping) != std::string::npos);
    NINHO_REQUIRE(gameplay.find(split_mapping) != std::string::npos);
    NINHO_REQUIRE(orbital.find(split_mapping) != std::string::npos);
    NINHO_REQUIRE(gameplay.find(spawned_mapping) != std::string::npos);
    NINHO_REQUIRE(orbital.find(spawned_mapping) != std::string::npos);
    NINHO_REQUIRE(gameplay.find(delta_mapping) != std::string::npos);
    NINHO_REQUIRE(orbital.find(delta_mapping) != std::string::npos);
}

NINHO_TEST("gameplay frame schema freezes exact version two top level dictionary keys")
{
    const std::string source = read_source_file(
        "native/extension/src/gameplay_session_node.cpp");
    const std::string body = gameplay_frame_dictionary_body(source);
    const std::regex assignment{R"(result\[([^\]]+)\]\s*=)"};
    std::vector<std::string> keys;
    for (auto match = std::sregex_iterator{body.begin(), body.end(), assignment};
         match != std::sregex_iterator{}; ++match) {
        const std::string expression = (*match)[1].str();
        NINHO_REQUIRE(expression.size() >= 2U);
        NINHO_REQUIRE(expression.front() == '"' && expression.back() == '"');
        keys.push_back(expression.substr(1U, expression.size() - 2U));
    }

    const std::array expected{
        std::string_view{"frame_schema_version"}, std::string_view{"tick"},
        std::string_view{"ticks_executed"}, std::string_view{"phase"},
        std::string_view{"outcome"}, std::string_view{"launcher"},
        std::string_view{"locked_plane"}, std::string_view{"bird_queue"},
        std::string_view{"current_bird"}, std::string_view{"shot"},
        std::string_view{"projectiles"}, std::string_view{"snapshots"},
        std::string_view{"events"}, std::string_view{"objectives"},
        std::string_view{"ability_readiness"}, std::string_view{"ability_armed"},
        std::string_view{"trajectory_preview"}, std::string_view{"score"},
        std::string_view{"stars"}, std::string_view{"gravity_kind"},
        std::string_view{"local_gravity"}, std::string_view{"metrics"},
        std::string_view{"discarded_time_seconds"},
    };
    NINHO_REQUIRE(keys.size() == expected.size());
    for (std::size_t index = 0; index < expected.size(); ++index) {
        NINHO_REQUIRE(keys[index] == expected[index]);
    }
}

}
