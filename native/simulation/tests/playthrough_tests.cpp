#include "test_framework.hpp"

#include "ninho/simulation/session.hpp"
#include "session_test_facade.hpp"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string_view>
#include <vector>

namespace {

using namespace ninho::simulation;

std::string read_file(std::string_view relative)
{
    std::ifstream stream{std::filesystem::path{NINHO_SOURCE_DIR} / relative,
        std::ios::binary};
    NINHO_SIM_REQUIRE(stream.is_open());
    std::ostringstream text;
    text << stream.rdbuf();
    return text.str();
}

std::unique_ptr<SimulationSession> make_session()
{
    const auto materials = parse_material_catalog(
        read_file("game/data/materials/vertical_slice.materials.json"));
    const auto archetypes = parse_archetype_catalog(
        read_file("game/data/archetypes/vertical_slice.archetypes.json"));
    const auto level = parse_level_manifest(
        read_file("game/data/levels/first_orbit.level.json"));
    NINHO_SIM_REQUIRE(materials.ok() && archetypes.ok() && level.ok());
    auto created = SimulationSession::create(materials.value, archetypes.value, level.value);
    NINHO_SIM_REQUIRE(created.ok());
    return std::move(created.value);
}

void append_tick_events(
    const SimulationSession& session, std::vector<DomainEvent>& stream)
{
    stream.insert(stream.end(), session.events().begin(), session.events().end());
}

void launch(SimulationSession& session, std::vector<DomainEvent>& stream)
{
    const AimState aim{{-13.0f, 0.0f, 0.0f}, {0.0f, 1.0f, 0.0f}, 10.5};
    NINHO_SIM_REQUIRE(session.enqueue(BeginAimCommand{}).ok());
    NINHO_SIM_REQUIRE(session.tick().ok());
    append_tick_events(session, stream);
    NINHO_SIM_REQUIRE(session.enqueue(SetAimCommand{aim}).ok());
    NINHO_SIM_REQUIRE(session.enqueue(LaunchCommand{}).ok());
    NINHO_SIM_REQUIRE(session.tick().ok());
    append_tick_events(session, stream);
    NINHO_SIM_REQUIRE(session.state().phase == SessionPhase::FlightAbility);
}

void resolve(SimulationSession& session, std::vector<DomainEvent>& stream)
{
    detail::SessionTestFacade::finish_projectile(session);
    for (int tick = 0; tick < 160 && session.state().phase != SessionPhase::Evaluation; ++tick) {
        NINHO_SIM_REQUIRE(session.tick().ok());
        append_tick_events(session, stream);
    }
    NINHO_SIM_REQUIRE(session.state().phase == SessionPhase::Evaluation);
    NINHO_SIM_REQUIRE(session.tick().ok());
    append_tick_events(session, stream);
}

struct Trace {
    std::vector<std::uint8_t> state;
    std::vector<DomainEvent> events;
    Outcome outcome{};

    bool operator==(const Trace&) const = default;
};

Trace victory_script(bool structural)
{
    auto session = make_session();
    std::vector<DomainEvent> stream;
    launch(*session, stream);
    if (structural) {
        detail::SessionTestFacade::fracture_piece_after_solver(
            *session, EntityId{104}, PartId{1}, {-0.9f, 12.125f, -0.9f});
        NINHO_SIM_REQUIRE(session->tick().ok());
        append_tick_events(*session, stream);
    } else {
        for (int tick = 0; tick < 8; ++tick) {
            NINHO_SIM_REQUIRE(session->tick().ok());
            append_tick_events(*session, stream);
        }
        NINHO_SIM_REQUIRE(session->enqueue(ActivateAbilityCommand{}).ok());
        NINHO_SIM_REQUIRE(session->tick().ok());
        append_tick_events(*session, stream);
        NINHO_SIM_REQUIRE(detail::SessionTestFacade::ability_active(*session));
    }
    NINHO_SIM_REQUIRE(detail::SessionTestFacade::set_body_neutralized(
        *session, EntityId{200}, PartId{1}, true));
    resolve(*session, stream);
    return {session->canonical_state_v1(),
        std::move(stream),
        session->state().outcome};
}

Trace defeat_script()
{
    auto session = make_session();
    std::vector<DomainEvent> stream;
    for (int bird = 0; bird < 3; ++bird) {
        launch(*session, stream);
        resolve(*session, stream);
    }
    return {session->canonical_state_v1(),
        std::move(stream),
        session->state().outcome};
}

NINHO_SIM_TEST("playthrough Virela route wins headlessly")
{
    NINHO_SIM_REQUIRE(victory_script(false).outcome == Outcome::Victory);
}

NINHO_SIM_TEST("playthrough structural route wins headlessly")
{
    NINHO_SIM_REQUIRE(victory_script(true).outcome == Outcome::Victory);
}

NINHO_SIM_TEST("playthrough three Virelas without objective lose headlessly")
{
    NINHO_SIM_REQUIRE(defeat_script().outcome == Outcome::Defeat);
}

NINHO_SIM_TEST("vertical slice determinism repeats canonical state and event stream ten times")
{
    const Trace virela = victory_script(false);
    const Trace structural = victory_script(true);
    const Trace defeat = defeat_script();
    for (int repetition = 1; repetition < 10; ++repetition) {
        NINHO_SIM_REQUIRE(victory_script(false) == virela);
        NINHO_SIM_REQUIRE(victory_script(true) == structural);
        NINHO_SIM_REQUIRE(defeat_script() == defeat);
    }

    const auto fnv = [](const Trace& trace) {
        std::uint64_t hash = 14695981039346656037ULL;
        const auto byte = [&](std::uint8_t value) {
            hash ^= value;
            hash *= 1099511628211ULL;
        };
        const auto integer = [&](std::uint64_t value) {
            for (int shift = 0; shift < 64; shift += 8) {
                byte(static_cast<std::uint8_t>(value >> shift));
            }
        };
        for (const std::uint8_t value : trace.state) {
            byte(value);
        }
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
            const auto quantized = [&](double value) {
                integer(static_cast<std::uint64_t>(
                    detail::SessionTestFacade::quantize_canonical(value)));
            };
            const auto vector = [&](ninho::physics::Vec3 value) {
                quantized(value.x);
                quantized(value.y);
                quantized(value.z);
            };
            quantized(event.weight);
            vector(event.force_n);
            vector(event.impulse_n_s);
            integer(event.part_id.value());
            vector(event.position_m);
            vector(event.normal);
            quantized(event.energy_j);
            quantized(event.damage);
            integer(static_cast<std::uint8_t>(event.neutralization_cause));
            integer(event.cause_event_id.value());
            integer(event.joint_id.value());
            integer(event.material_id.value());
        }
        return hash;
    };
    std::cout << "[TRACE] canonical_playthrough_v1 "
              << fnv(virela) << ' ' << fnv(structural) << ' ' << fnv(defeat) << '\n';
}

}
