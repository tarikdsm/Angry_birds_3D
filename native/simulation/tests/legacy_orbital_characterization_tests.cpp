#include "test_framework.hpp"

#include "session_test_facade.hpp"
#include "ninho/physics/radial_gravity.hpp"
#include "ninho/simulation/session.hpp"

#include <array>
#include <bit>
#include <cmath>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <numbers>
#include <ranges>
#include <sstream>
#include <string_view>
#include <vector>

namespace {

using namespace ninho::simulation;

std::string read_source_file(std::string_view relative_path)
{
    std::ifstream stream{std::filesystem::path{NINHO_SOURCE_DIR} / relative_path,
        std::ios::binary};
    NINHO_SIM_REQUIRE(stream.is_open());
    std::ostringstream content;
    content << stream.rdbuf();
    return content.str();
}

std::unique_ptr<SimulationSession> create_session()
{
    const auto materials = parse_material_catalog(read_source_file(
        "game/data/materials/vertical_slice.materials.json"));
    const auto archetypes = parse_archetype_catalog(read_source_file(
        "game/data/archetypes/vertical_slice.archetypes.json"));
    const auto level = parse_level_manifest(read_source_file(
        "game/data/levels/first_orbit.level.json"));
    NINHO_SIM_REQUIRE(materials.ok() && archetypes.ok() && level.ok());
    auto result = SimulationSession::create(materials.value, archetypes.value, level.value);
    NINHO_SIM_REQUIRE(result.ok());
    return std::move(result.value);
}

struct Trace {
    std::vector<std::uint8_t> state;
    std::vector<DomainEvent> events;
    Outcome outcome{};
};

void capture(const SimulationSession& session, Trace& trace)
{
    trace.events.insert(trace.events.end(), session.events().begin(), session.events().end());
}

void tick(SimulationSession& session, Trace& trace)
{
    NINHO_SIM_REQUIRE(session.tick().ok());
    capture(session, trace);
}

AimState ring_aim(double theta_deg, double speed)
{
    const double theta = theta_deg * std::numbers::pi / 180.0;
    return {{static_cast<float>(-13.0 * std::cos(theta)), 0.0F,
                static_cast<float>(13.0 * std::sin(theta))},
        {0.0F, 1.0F, 0.0F}, speed};
}

AimState miss_aim()
{
    return {{-13.0F, 0.0F, 0.0F}, {0.0F, 0.0F, 1.0F}, 8.0};
}

void launch(SimulationSession& session, Trace& trace, const AimState& aim)
{
    NINHO_SIM_REQUIRE(session.enqueue(BeginAimCommand{}).ok());
    tick(session, trace);
    NINHO_SIM_REQUIRE(session.enqueue(SetAimCommand{aim}).ok());
    NINHO_SIM_REQUIRE(session.enqueue(LaunchCommand{}).ok());
    tick(session, trace);
    NINHO_SIM_REQUIRE(session.state().phase == SessionPhase::FlightAbility);
}

void activate_after(SimulationSession& session, Trace& trace, std::uint32_t delay_ticks)
{
    for (std::uint32_t elapsed = 1;
         elapsed < delay_ticks && session.state().phase == SessionPhase::FlightAbility;
         ++elapsed) {
        tick(session, trace);
    }
    NINHO_SIM_REQUIRE(session.state().phase == SessionPhase::FlightAbility);
    NINHO_SIM_REQUIRE(session.enqueue(ActivateAbilityCommand{}).ok());
    tick(session, trace);
}

void resolve_shot(SimulationSession& session, Trace& trace)
{
    for (int watchdog = 0; watchdog < 1900; ++watchdog) {
        if (session.state().phase == SessionPhase::Evaluation) {
            tick(session, trace);
            return;
        }
        if (session.state().phase == SessionPhase::Result) {
            return;
        }
        tick(session, trace);
    }
    NINHO_SIM_REQUIRE(false);
}

Trace run_route(const std::array<double, 3>& theta_by_shot, bool activate_ability)
{
    auto session = create_session();
    Trace trace;
    for (int shot = 0; shot < 3 && session->state().outcome == Outcome::None; ++shot) {
        launch(*session, trace, ring_aim(theta_by_shot.at(shot), 8.0));
        if (activate_ability) {
            activate_after(*session, trace, 40U);
        }
        resolve_shot(*session, trace);
    }
    trace.state = session->canonical_state_v2();
    trace.outcome = session->state().outcome;
    return trace;
}

Trace run_defeat_route()
{
    auto session = create_session();
    Trace trace;
    for (int shot = 0; shot < 3 && session->state().phase != SessionPhase::Result; ++shot) {
        launch(*session, trace, miss_aim());
        resolve_shot(*session, trace);
    }
    trace.state = session->canonical_state_v2();
    trace.outcome = session->state().outcome;
    return trace;
}

std::int64_t canonical_quantize(double value)
{
    return static_cast<std::int64_t>(std::round(value * 100000.0));
}

std::vector<std::uint8_t> event_bytes(const Trace& trace)
{
    std::vector<std::uint8_t> bytes;
    const auto integer = [&](std::uint64_t value) {
        for (unsigned shift = 0; shift < 64U; shift += 8U) {
            bytes.push_back(static_cast<std::uint8_t>(value >> shift));
        }
    };
    const auto quantized = [&](double value) {
        integer(static_cast<std::uint64_t>(canonical_quantize(value)));
    };
    const auto vector = [&](ninho::physics::Vec3 value) {
        quantized(value.x);
        quantized(value.y);
        quantized(value.z);
    };
    for (const DomainEvent& event : trace.events) {
        integer(event.id.value());
        integer(event.tick.value());
        integer(static_cast<std::uint8_t>(event.kind));
        integer(event.entity_id.value());
        integer(event.bird_archetype_id.value());
        integer(static_cast<std::uint8_t>(event.rejection_reason));
        integer(event.ability_id.value());
        integer(event.affected_entity_id.value());
        integer(event.affected_part_id.value());
        quantized(event.weight);
        vector(event.force_n);
        vector(event.impulse_n_s);
        integer(event.part_id.value());
        vector(event.position_m);
        vector(event.normal);
        quantized(event.energy_j);
        quantized(event.damage);
        integer(static_cast<std::uint8_t>(event.damage_classification));
        integer(static_cast<std::uint8_t>(event.neutralization_cause));
        integer(event.cause_event_id.value());
        integer(event.joint_id.value());
        integer(event.material_id.value());
        quantized(event.joint_load_ratio);
        quantized(event.fracture_ratio);
    }
    return bytes;
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

std::uint64_t canonical_signature(const Trace& trace)
{
    std::vector<std::uint8_t> bytes = trace.state;
    const std::vector<std::uint8_t> events = event_bytes(trace);
    bytes.insert(bytes.end(), events.begin(), events.end());
    return fnv1a64(bytes);
}

NINHO_SIM_TEST("legacy orbital characterization freezes radial world ejection and removal")
{
    const ninho::physics::LegacyRadialWorldConfig legacy{};
    const ninho::physics::WorldConfig config =
        ninho::physics::make_legacy_radial_world_config(legacy);
    NINHO_SIM_REQUIRE(config.time_step == 1.0F / 60.0F);
    NINHO_SIM_REQUIRE(config.substeps == 4);
    NINHO_SIM_REQUIRE(legacy.planet_radius == 10.0F);
    NINHO_SIM_REQUIRE(legacy.surface_gravity == 9.0F);
    NINHO_SIM_REQUIRE(config.max_bodies == 500U);

    const ninho::physics::RadialGravity gravity{
        ninho::physics::LegacyRadialGravityConfig{{}, legacy.planet_radius,
            legacy.surface_gravity}};
    NINHO_SIM_REQUIRE(std::abs(gravity.acceleration({10.0F, 0.0F, 0.0F}).x + 9.0F)
        < 1.0e-6F);
    NINHO_SIM_REQUIRE(std::abs(gravity.acceleration({20.0F, 0.0F, 0.0F}).x + 2.25F)
        < 1.0e-6F);
    NINHO_SIM_REQUIRE(std::abs(gravity.acceleration({1.0F, 0.0F, 0.0F}).x + 18.0F)
        < 1.0e-6F);

    const auto arm_tick = [&](float radius, float speed) {
        ninho::physics::EjectionTracker tracker;
        for (int tick_index = 1; tick_index <= 60; ++tick_index) {
            if (tracker.update({1U, 1U}, radius, speed,
                    config.time_step, legacy.planet_radius)) {
                return tick_index;
            }
        }
        return 0;
    };
    NINHO_SIM_REQUIRE(arm_tick(4.0F * legacy.planet_radius, 2.0F) == 30);
    NINHO_SIM_REQUIRE(arm_tick(
        std::nextafter(4.0F * legacy.planet_radius, 0.0F), 2.0F) == 0);
    NINHO_SIM_REQUIRE(arm_tick(4.0F * legacy.planet_radius,
        std::nextafter(2.0F, 0.0F)) == 0);

    ninho::physics::EjectionTracker interrupted;
    for (int tick_index = 0; tick_index < 29; ++tick_index) {
        NINHO_SIM_REQUIRE(!interrupted.update({2U, 1U}, 40.0F, 2.0F,
            config.time_step, legacy.planet_radius));
    }
    NINHO_SIM_REQUIRE(!interrupted.update({2U, 1U}, 40.0F, 1.99F,
        config.time_step, legacy.planet_radius));
    for (int tick_index = 0; tick_index < 29; ++tick_index) {
        NINHO_SIM_REQUIRE(!interrupted.update({2U, 1U}, 40.0F, 2.0F,
            config.time_step, legacy.planet_radius));
    }
    NINHO_SIM_REQUIRE(interrupted.update({2U, 1U}, 40.0F, 2.0F,
        config.time_step, legacy.planet_radius));

    const auto body_lifetime_at = [](float radius) {
        ninho::physics::PhysicsWorld world(
            ninho::physics::make_legacy_radial_world_config(
                {.surface_gravity = 0.0F, .max_bodies = 1U}));
        auto body = ninho::physics::BodyDesc::dynamic_sphere(
            0.1F, {{radius, 0.0F, 0.0F}, {}}, 1.0F);
        const auto created = world.create_body(body);
        NINHO_SIM_REQUIRE(created);
        world.step();
        const bool after_removal_decision = world.state(created.value).has_value();
        world.step();
        const bool after_queued_removal = world.state(created.value).has_value();
        return std::array{after_removal_decision, after_queued_removal};
    };
    NINHO_SIM_REQUIRE((body_lifetime_at(59.999F) == std::array{true, true}));
    NINHO_SIM_REQUIRE((body_lifetime_at(60.0F) == std::array{true, false}));
}

NINHO_SIM_TEST("legacy orbital characterization freezes aim preview and snapshot order")
{
    auto session = create_session();
    const AimState source{{-13.0004F, 0.0004F, 0.0F},
        {0.0F, 1.00004F, 0.00004F}, 10.504};
    const auto quantized = session->quantize_aim(source);
    NINHO_SIM_REQUIRE(quantized.ok());
    const auto vector_bits = [](ninho::physics::Vec3 value) {
        return std::array{
            std::bit_cast<std::uint32_t>(value.x),
            std::bit_cast<std::uint32_t>(value.y),
            std::bit_cast<std::uint32_t>(value.z),
        };
    };
    NINHO_SIM_REQUIRE((vector_bits(quantized.value.origin_m)
        == std::array<std::uint32_t, 3>{3243245569U, 0U, 0U}));
    NINHO_SIM_REQUIRE((vector_bits(quantized.value.tangent_direction)
        == std::array<std::uint32_t, 3>{0U, 1065353216U, 0U}));
    NINHO_SIM_REQUIRE(quantized.value.speed_m_s == 10.5);

    constexpr std::array expected_order{
        std::pair{1U, 0U}, std::pair{10U, 1U}, std::pair{100U, 1U},
        std::pair{101U, 1U}, std::pair{102U, 1U}, std::pair{103U, 1U},
        std::pair{104U, 1U}, std::pair{105U, 1U}, std::pair{106U, 1U},
        std::pair{107U, 1U}, std::pair{110U, 1U}, std::pair{111U, 1U},
        std::pair{112U, 1U}, std::pair{120U, 1U}, std::pair{121U, 1U},
        std::pair{122U, 1U}, std::pair{123U, 1U}, std::pair{124U, 1U},
        std::pair{125U, 1U}, std::pair{126U, 1U}, std::pair{127U, 1U},
        std::pair{128U, 1U}, std::pair{200U, 1U},
    };
    NINHO_SIM_REQUIRE(session->snapshots().size() == expected_order.size());
    for (std::size_t index = 0; index < expected_order.size(); ++index) {
        NINHO_SIM_REQUIRE(session->snapshots()[index].entity_id.value()
            == expected_order[index].first);
        NINHO_SIM_REQUIRE(session->snapshots()[index].part_id.value()
            == expected_order[index].second);
    }

    NINHO_SIM_REQUIRE(detail::SessionTestFacade::add_static_sphere(
        *session, EntityId{9000}, {-12.7F, 4.0F, 0.0F}, 0.5));
    NINHO_SIM_REQUIRE(session->tick().ok());
    const TrajectoryPreview preview = session->preview(source);
    NINHO_SIM_REQUIRE(preview.status.ok() && preview.first_hit.has_value());
    NINHO_SIM_REQUIRE(preview.canonical_hash == 15211795551651615890ULL);
    NINHO_SIM_REQUIRE(preview.samples.size() == 19U);
    NINHO_SIM_REQUIRE(preview.first_hit->entity_id == EntityId{9000});
    NINHO_SIM_REQUIRE(preview.first_hit->part_id == PartId{1});
    NINHO_SIM_REQUIRE(std::abs(preview.first_hit->point_m.x + 12.7318F) < 1.0e-4F);
    NINHO_SIM_REQUIRE(std::abs(preview.first_hit->point_m.y - 3.50102F) < 1.0e-4F);
    NINHO_SIM_REQUIRE(std::abs(preview.first_hit->point_m.z) < 1.0e-6F);
    NINHO_SIM_REQUIRE(std::abs(preview.first_hit->normal.x + 0.0636893F) < 1.0e-5F);
    NINHO_SIM_REQUIRE(std::abs(preview.first_hit->normal.y + 0.99797F) < 1.0e-5F);
    NINHO_SIM_REQUIRE(std::abs(preview.first_hit->normal.z) < 1.0e-6F);
}

NINHO_SIM_TEST("legacy orbital characterization freezes numeric tags")
{
    const std::array phase_tags{
        static_cast<std::uint8_t>(SessionPhase::Inspection),
        static_cast<std::uint8_t>(SessionPhase::Aim),
        static_cast<std::uint8_t>(SessionPhase::FlightAbility),
        static_cast<std::uint8_t>(SessionPhase::Resolution),
        static_cast<std::uint8_t>(SessionPhase::Evaluation),
        static_cast<std::uint8_t>(SessionPhase::Result),
        static_cast<std::uint8_t>(SessionPhase::Faulted),
    };
    NINHO_SIM_REQUIRE((phase_tags == std::array<std::uint8_t, 7>{0, 1, 2, 3, 4, 5, 6}));

    const std::array event_tags{
        static_cast<std::uint8_t>(DomainEventKind::BirdLaunched),
        static_cast<std::uint8_t>(DomainEventKind::AbilityActivationRequested),
        static_cast<std::uint8_t>(DomainEventKind::CommandRejected),
        static_cast<std::uint8_t>(DomainEventKind::AbilityStarted),
        static_cast<std::uint8_t>(DomainEventKind::AbilityAffectedBody),
        static_cast<std::uint8_t>(DomainEventKind::AbilityPulse),
        static_cast<std::uint8_t>(DomainEventKind::AbilityEnded),
        static_cast<std::uint8_t>(DomainEventKind::DamageApplied),
        static_cast<std::uint8_t>(DomainEventKind::EntityNeutralized),
        static_cast<std::uint8_t>(DomainEventKind::JointOverloaded),
        static_cast<std::uint8_t>(DomainEventKind::PieceFractureTriggered),
        static_cast<std::uint8_t>(DomainEventKind::JointBroken),
        static_cast<std::uint8_t>(DomainEventKind::PieceFractured),
    };
    NINHO_SIM_REQUIRE((event_tags
        == std::array<std::uint8_t, 13>{0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12}));

    const std::array rejection_tags{
        static_cast<std::uint8_t>(CommandRejectionReason::None),
        static_cast<std::uint8_t>(CommandRejectionReason::InvalidPhase),
        static_cast<std::uint8_t>(CommandRejectionReason::InvalidAim),
        static_cast<std::uint8_t>(CommandRejectionReason::NotArmed),
        static_cast<std::uint8_t>(CommandRejectionReason::NoBirdAvailable),
    };
    NINHO_SIM_REQUIRE((rejection_tags == std::array<std::uint8_t, 5>{0, 1, 2, 3, 4}));

    const std::array neutralization_tags{
        static_cast<std::uint8_t>(NeutralizationCause::None),
        static_cast<std::uint8_t>(NeutralizationCause::IntegrityDepleted),
        static_cast<std::uint8_t>(NeutralizationCause::Ejection),
    };
    NINHO_SIM_REQUIRE((neutralization_tags == std::array<std::uint8_t, 3>{0, 1, 2}));

    const std::array classification_tags{
        static_cast<std::uint8_t>(DamageClassification::None),
        static_cast<std::uint8_t>(DamageClassification::Protected),
        static_cast<std::uint8_t>(DamageClassification::Vulnerable),
    };
    NINHO_SIM_REQUIRE((classification_tags == std::array<std::uint8_t, 3>{0, 1, 2}));

    const std::array command_tags{
        PlayerCommand{BeginAimCommand{}}.index(),
        PlayerCommand{SetAimCommand{}}.index(),
        PlayerCommand{LaunchCommand{}}.index(),
        PlayerCommand{ActivateAbilityCommand{}}.index(),
        PlayerCommand{CancelAimCommand{}}.index(),
    };
    NINHO_SIM_REQUIRE((command_tags == std::array<std::size_t, 5>{0, 1, 2, 3, 4}));
}

NINHO_SIM_TEST("legacy orbital characterization freezes canonical state v2")
{
    auto canonical = create_session();
    NINHO_SIM_REQUIRE(canonical->enqueue(BeginAimCommand{}).ok());
    NINHO_SIM_REQUIRE(canonical->tick().ok());
    NINHO_SIM_REQUIRE(canonical->enqueue(SetAimCommand{AimState{
        {-13.0F, 0.0F, 0.0F}, {0.0F, 1.0F, 0.0F}, 10.5}}).ok());
    NINHO_SIM_REQUIRE(canonical->enqueue(LaunchCommand{}).ok());
    NINHO_SIM_REQUIRE(canonical->tick().ok());
    NINHO_SIM_REQUIRE(canonical->canonical_hash_v2() == 16778877272428821006ULL);
    NINHO_SIM_REQUIRE(fnv1a64(canonical->canonical_state_v2())
        == 16778877272428821006ULL);
}

NINHO_SIM_TEST("legacy orbital characterization freezes three route events and playthrough v4")
{
    const Trace virela = run_route({-2.0, 0.0, 0.0}, true);
    const Trace structural = run_route({0.0, 0.0, 0.0}, false);
    const Trace defeat = run_defeat_route();
    struct ExpectedRoute {
        const Trace* trace;
        Outcome outcome;
        std::uint64_t playthrough_v4;
        std::uint64_t state_v2;
        std::uint64_t events;
        std::size_t event_count;
    };
    const std::array routes{
        ExpectedRoute{&virela, Outcome::Victory, 16950529107460336353ULL,
            17614499550350473939ULL, 1419519005172768707ULL, 205U},
        ExpectedRoute{&structural, Outcome::Victory, 9286739750595435608ULL,
            4069305773712158776ULL, 11981083583776292473ULL, 102U},
        ExpectedRoute{&defeat, Outcome::Defeat, 9146724923232777924ULL,
            16998920908514221234ULL, 1498846658507908711ULL, 3U},
    };
    for (const ExpectedRoute& route : routes) {
        NINHO_SIM_REQUIRE(route.trace->outcome == route.outcome);
        NINHO_SIM_REQUIRE(canonical_signature(*route.trace) == route.playthrough_v4);
        NINHO_SIM_REQUIRE(fnv1a64(route.trace->state) == route.state_v2);
        NINHO_SIM_REQUIRE(fnv1a64(event_bytes(*route.trace)) == route.events);
        NINHO_SIM_REQUIRE(route.trace->events.size() == route.event_count);
    }
}

}
