#include "test_framework.hpp"

#include "ninho/simulation/session.hpp"

#include <array>
#include <cmath>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <limits>
#include <numbers>
#include <ranges>
#include <sstream>
#include <stdexcept>
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
    auto created = SimulationSession::create(
        materials.value, archetypes.value, level.value);
    NINHO_SIM_REQUIRE(created.ok());
    return std::move(created.value);
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

AimState miss_aim()
{
    return {{-13.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 1.0f}, 8.0};
}

AimState ring_aim(double theta_deg, double phase_deg, double speed)
{
    const double theta = theta_deg * std::numbers::pi / 180.0;
    const double phase = phase_deg * std::numbers::pi / 180.0;
    const ninho::physics::Vec3 origin{
        static_cast<float>(-13.0 * std::cos(theta)), 0.0f,
        static_cast<float>(13.0 * std::sin(theta))};
    const ninho::physics::Vec3 azimuth{
        static_cast<float>(std::sin(theta)), 0.0f,
        static_cast<float>(std::cos(theta))};
    const ninho::physics::Vec3 tangent = ninho::physics::normalized_or_zero(
        ninho::physics::Vec3{0.0f, static_cast<float>(std::cos(phase)), 0.0f}
        + azimuth * static_cast<float>(std::sin(phase)));
    return {origin, tangent, speed};
}

void launch(SimulationSession& session, Trace& trace, const AimState& value)
{
    NINHO_SIM_REQUIRE(session.state().phase == SessionPhase::Inspection);
    NINHO_SIM_REQUIRE(session.enqueue(BeginAimCommand{}).ok());
    tick(session, trace);
    NINHO_SIM_REQUIRE(session.enqueue(SetAimCommand{value}).ok());
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
        if (session.state().phase == SessionPhase::Result) return;
        tick(session, trace);
    }
    NINHO_SIM_REQUIRE(false);
}

Trace virela_sequence_script(const std::array<double, 3>& theta_by_shot,
    std::uint32_t ability_delay_ticks, bool activate_ability)
{
    auto session = make_session();
    Trace trace;
    for (int shot = 0; shot < 3 && session->state().outcome == Outcome::None; ++shot) {
        launch(*session, trace, ring_aim(theta_by_shot.at(shot), 0.0, 8.0));
        if (activate_ability) activate_after(*session, trace, ability_delay_ticks);
        resolve_shot(*session, trace);
    }
    trace.state = session->canonical_state_v2();
    trace.outcome = session->state().outcome;
    return trace;
}

Trace virela_victory_script()
{
    return virela_sequence_script({-2.0, 0.0, 0.0}, 40, true);
}

Trace virela_without_ability_script()
{
    return virela_sequence_script({-2.0, 0.0, 0.0}, 40, false);
}

Trace structural_victory_script()
{
    auto session = make_session();
    Trace trace;
    const AimState value = ring_aim(0.0, 0.0, 8.0);
    for (int shot = 0; shot < 3 && session->state().outcome == Outcome::None; ++shot) {
        launch(*session, trace, value);
        resolve_shot(*session, trace);
    }
    trace.state = session->canonical_state_v2();
    trace.outcome = session->state().outcome;
    return trace;
}

Trace defeat_script()
{
    auto session = make_session();
    Trace trace;
    for (int shot = 0; shot < 3 && session->state().phase != SessionPhase::Result; ++shot) {
        launch(*session, trace, miss_aim());
        resolve_shot(*session, trace);
    }
    trace.state = session->canonical_state_v2();
    trace.outcome = session->state().outcome;
    return trace;
}

bool has_event(const Trace& trace, DomainEventKind kind)
{
    return std::ranges::any_of(trace.events,
        [&](const DomainEvent& event) { return event.kind == kind; });
}

bool has_affected_counterweight(const Trace& trace)
{
    return std::ranges::any_of(trace.events, [](const DomainEvent& event) {
        const auto entity = event.affected_entity_id.value();
        return event.kind == DomainEventKind::AbilityAffectedBody
            && entity >= 120U && entity <= 128U;
    });
}

bool has_exact_causal_pair(
    const Trace& trace, DomainEventKind effect, DomainEventKind cause)
{
    return std::ranges::any_of(trace.events, [&](const DomainEvent& event) {
        if (event.kind != effect || event.cause_event_id == EventId{}) return false;
        return std::ranges::count_if(trace.events, [&](const DomainEvent& candidate) {
            return candidate.id == event.cause_event_id
                && candidate.id < event.id && candidate.kind == cause;
        }) == 1;
    });
}

void require_resolved_causes(const Trace& trace)
{
    for (const DomainEvent& event : trace.events) {
        if (event.cause_event_id == EventId{}) continue;
        const auto is_cause = [&](const DomainEvent& candidate) {
            return candidate.id == event.cause_event_id && candidate.id < event.id;
        };
        NINHO_SIM_REQUIRE(std::ranges::count_if(trace.events, is_cause) == 1);
        const auto cause_it = std::ranges::find_if(trace.events, is_cause);
        NINHO_SIM_REQUIRE(cause_it != trace.events.end());
        const DomainEvent& cause = *cause_it;
        switch (event.kind) {
        case DomainEventKind::JointOverloaded:
        case DomainEventKind::PieceFractureTriggered:
            NINHO_SIM_REQUIRE(cause.kind == DomainEventKind::DamageApplied);
            break;
        case DomainEventKind::JointBroken:
            NINHO_SIM_REQUIRE(cause.kind == DomainEventKind::JointOverloaded);
            break;
        case DomainEventKind::PieceFractured:
            NINHO_SIM_REQUIRE(cause.kind == DomainEventKind::PieceFractureTriggered);
            break;
        default:
            NINHO_SIM_REQUIRE(false);
        }
    }
}

std::int64_t local_canonical_quantize(double value)
{
    if (!std::isfinite(value)) {
        throw std::invalid_argument("canonical numeric value must be finite");
    }
    constexpr long double scale = 100000.0L;
    const long double scaled = static_cast<long double>(value) * scale;
    if (!std::isfinite(scaled)) {
        throw std::range_error("canonical numeric value exceeds fixed-point range");
    }
    const long double rounded = std::round(scaled);
    const long double signed_limit = std::ldexp(1.0L, 63);
    if (rounded < -signed_limit || rounded >= signed_limit) {
        throw std::range_error("canonical numeric value exceeds fixed-point range");
    }
    const auto fixed = static_cast<std::int64_t>(rounded);
    return fixed == 0 ? std::int64_t{0} : fixed;
}

std::vector<std::uint8_t> ordered_event_bytes(const Trace& trace)
{
    std::vector<std::uint8_t> bytes;
    const auto integer = [&](std::uint64_t value) {
        for (int shift = 0; shift < 64; shift += 8) {
            bytes.push_back(static_cast<std::uint8_t>(value >> shift));
        }
    };
    const auto quantized = [&](double value) {
        integer(static_cast<std::uint64_t>(local_canonical_quantize(value)));
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

std::uint64_t canonical_signature(const Trace& trace)
{
    const auto event_order_bytes = ordered_event_bytes(trace);
    std::uint64_t hash = 14695981039346656037ULL;
    const auto hash_byte = [&](std::uint8_t value) {
        hash ^= value;
        hash *= 1099511628211ULL;
    };
    for (const std::uint8_t value : trace.state) hash_byte(value);
    for (const std::uint8_t value : event_order_bytes) hash_byte(value);
    return hash;
}

std::string ordered_events_hex(const Trace& trace)
{
    const auto bytes = ordered_event_bytes(trace);
    std::ostringstream output;
    output << std::hex << std::setfill('0');
    for (const std::uint8_t value : bytes) {
        output << std::setw(2) << static_cast<unsigned>(value);
    }
    return output.str();
}

NINHO_SIM_TEST("playthrough Virela route wins through public commands and physics")
{
    const Trace trace = virela_victory_script();
    NINHO_SIM_REQUIRE(trace.outcome == Outcome::Victory);
    NINHO_SIM_REQUIRE(has_event(trace, DomainEventKind::AbilityStarted));
    NINHO_SIM_REQUIRE(has_event(trace, DomainEventKind::AbilityAffectedBody));
    NINHO_SIM_REQUIRE(has_event(trace, DomainEventKind::AbilityPulse));
    NINHO_SIM_REQUIRE(has_event(trace, DomainEventKind::AbilityEnded));
    NINHO_SIM_REQUIRE(has_affected_counterweight(trace));
    NINHO_SIM_REQUIRE(has_event(trace, DomainEventKind::DamageApplied)
        || has_event(trace, DomainEventKind::EntityNeutralized));
    NINHO_SIM_REQUIRE(has_event(trace, DomainEventKind::EntityNeutralized));
    require_resolved_causes(trace);
}

NINHO_SIM_TEST("playthrough legacy fixture publishes canonical v2 only")
{
    auto session = make_session();
    NINHO_SIM_REQUIRE(!session->canonical_state_v2().empty());
    NINHO_SIM_REQUIRE(session->canonical_hash_v2() != 0U);
    NINHO_SIM_REQUIRE(session->canonical_state_v3().empty());
    NINHO_SIM_REQUIRE(session->canonical_hash_v3() == 0U);
}

NINHO_SIM_TEST("playthrough Virela route requires its ability to neutralize the objective")
{
    const Trace active = virela_victory_script();
    const Trace trace = virela_without_ability_script();
    const auto launches = [](const Trace& value) {
        return std::ranges::count(value.events, DomainEventKind::BirdLaunched,
            &DomainEvent::kind);
    };
    NINHO_SIM_REQUIRE(active.outcome == Outcome::Victory);
    NINHO_SIM_REQUIRE(trace.outcome == Outcome::Defeat);
    NINHO_SIM_REQUIRE(launches(active) == 3);
    NINHO_SIM_REQUIRE(launches(trace) == 3);
    NINHO_SIM_REQUIRE(!has_event(trace, DomainEventKind::EntityNeutralized));
}

NINHO_SIM_TEST("playthrough structural route wins through public commands and physics")
{
    const Trace trace = structural_victory_script();
    NINHO_SIM_REQUIRE(trace.outcome == Outcome::Victory);
    NINHO_SIM_REQUIRE(has_event(trace, DomainEventKind::JointOverloaded));
    NINHO_SIM_REQUIRE(has_event(trace, DomainEventKind::JointBroken));
    NINHO_SIM_REQUIRE(has_event(trace, DomainEventKind::PieceFractureTriggered));
    NINHO_SIM_REQUIRE(has_event(trace, DomainEventKind::PieceFractured));
    NINHO_SIM_REQUIRE(has_event(trace, DomainEventKind::DamageApplied)
        || has_event(trace, DomainEventKind::EntityNeutralized));
    NINHO_SIM_REQUIRE(has_event(trace, DomainEventKind::EntityNeutralized));
    const auto joint_breaks = std::ranges::count(
        trace.events, DomainEventKind::JointBroken, &DomainEvent::kind);
    const auto overloaded_breaks = std::ranges::count_if(
        trace.events, [&](const DomainEvent& event) {
            if (event.kind != DomainEventKind::JointBroken) return false;
            return std::ranges::count_if(trace.events, [&](const DomainEvent& cause) {
                return cause.id == event.cause_event_id && cause.id < event.id
                    && cause.kind == DomainEventKind::JointOverloaded;
            }) == 1;
        });
    NINHO_SIM_REQUIRE(joint_breaks > 0 && overloaded_breaks == joint_breaks);
    NINHO_SIM_REQUIRE(has_exact_causal_pair(trace,
        DomainEventKind::PieceFractured, DomainEventKind::PieceFractureTriggered));
    require_resolved_causes(trace);
}

NINHO_SIM_TEST("playthrough three public Virela launches without objective lose")
{
    const Trace trace = defeat_script();
    NINHO_SIM_REQUIRE(trace.outcome == Outcome::Defeat);
    require_resolved_causes(trace);
}

NINHO_SIM_TEST("playthrough production tuple remains intact for 120 idle ticks")
{
    auto session = make_session();
    Trace trace;
    std::vector<EntitySnapshot> settled_snapshots;
    for (int idle_tick = 0; idle_tick < 120; ++idle_tick) {
        tick(*session, trace);
        if (idle_tick == 59) {
            settled_snapshots.assign(
                session->snapshots().begin(), session->snapshots().end());
        }
        if (idle_tick < 60) continue;
        for (const EntitySnapshot& snapshot : session->snapshots()) {
            if (snapshot.body_type != BodyType::Dynamic) continue;
            NINHO_SIM_REQUIRE(ninho::physics::length(
                snapshot.linear_velocity_m_s) < 0.15f);
            NINHO_SIM_REQUIRE(ninho::physics::length(
                snapshot.angular_velocity_rad_s) < 0.20f);
            NINHO_SIM_REQUIRE(!snapshot.ejected);
            const auto settled = std::ranges::find_if(settled_snapshots,
                [&](const EntitySnapshot& candidate) {
                    return candidate.entity_id == snapshot.entity_id
                        && candidate.part_id == snapshot.part_id;
                });
            NINHO_SIM_REQUIRE(settled != settled_snapshots.end());
            NINHO_SIM_REQUIRE(ninho::physics::length(
                snapshot.transform.position - settled->transform.position) < 0.01f);
        }
    }

    NINHO_SIM_REQUIRE(!has_event(trace, DomainEventKind::JointOverloaded));
    NINHO_SIM_REQUIRE(!has_event(trace, DomainEventKind::JointBroken));
    NINHO_SIM_REQUIRE(!has_event(trace, DomainEventKind::PieceFractureTriggered));
    NINHO_SIM_REQUIRE(!has_event(trace, DomainEventKind::PieceFractured));
    NINHO_SIM_REQUIRE(std::ranges::all_of(session->structural_joints(),
        &StructuralJointSnapshot::active));
}

NINHO_SIM_TEST("playthrough canonical contracts distinguish damage classification")
{
    Trace protected_trace;
    DomainEvent damage_event{
        .id = EventId{1},
        .tick = TickIndex{2},
        .kind = DomainEventKind::DamageApplied,
        .entity_id = EntityId{100},
        .affected_entity_id = EntityId{200},
        .energy_j = 12.0,
        .damage = 3.0,
        .damage_classification = DamageClassification::Protected,
    };
    protected_trace.events.push_back(damage_event);

    Trace vulnerable_trace = protected_trace;
    vulnerable_trace.events.front().damage_classification =
        DamageClassification::Vulnerable;

    NINHO_SIM_REQUIRE(ordered_events_hex(protected_trace)
        != ordered_events_hex(vulnerable_trace));
    NINHO_SIM_REQUIRE(canonical_signature(protected_trace)
        != canonical_signature(vulnerable_trace));
}

NINHO_SIM_TEST("playthrough local canonical quantizer enforces fixed point boundaries")
{
    NINHO_SIM_REQUIRE(local_canonical_quantize(0.0) == 0);
    NINHO_SIM_REQUIRE(local_canonical_quantize(-0.0) == 0);
    const auto ordinary = local_canonical_quantize(12.34567);
    NINHO_SIM_REQUIRE(local_canonical_quantize(12.34568) == ordinary + 1);
    NINHO_SIM_REQUIRE(local_canonical_quantize(9.223372036854774e13) > 0);

    const auto require_rejected = [](double value) {
        bool rejected = false;
        try {
            static_cast<void>(local_canonical_quantize(value));
        } catch (const std::range_error&) {
            rejected = true;
        } catch (const std::invalid_argument&) {
            rejected = true;
        }
        NINHO_SIM_REQUIRE(rejected);
    };
    require_rejected(9.223372036854776e13);
    require_rejected(-9.223372036854778e13);
    require_rejected(std::numeric_limits<double>::infinity());
    require_rejected(-std::numeric_limits<double>::infinity());
    require_rejected(std::numeric_limits<double>::quiet_NaN());
}

NINHO_SIM_TEST("vertical slice determinism compares quantized canonical signatures ten times")
{
    const Trace virela_trace = virela_victory_script();
    const Trace structural_trace = structural_victory_script();
    const Trace defeat_trace = defeat_script();
    const std::uint64_t virela = canonical_signature(virela_trace);
    const std::uint64_t structural = canonical_signature(structural_trace);
    const std::uint64_t defeat = canonical_signature(defeat_trace);
    std::uint64_t repeated_virela{};
    std::uint64_t repeated_structural{};
    std::uint64_t repeated_defeat{};
    Trace repeated_virela_trace;
    Trace repeated_structural_trace;
    Trace repeated_defeat_trace;
    for (int repetition = 1; repetition < 10; ++repetition) {
        repeated_virela_trace = virela_victory_script();
        repeated_structural_trace = structural_victory_script();
        repeated_defeat_trace = defeat_script();
        repeated_virela = canonical_signature(repeated_virela_trace);
        repeated_structural = canonical_signature(repeated_structural_trace);
        repeated_defeat = canonical_signature(repeated_defeat_trace);
        NINHO_SIM_REQUIRE(repeated_virela == virela);
        NINHO_SIM_REQUIRE(repeated_structural == structural);
        NINHO_SIM_REQUIRE(repeated_defeat == defeat);
    }
    std::cout << "[TRACE] canonical_playthrough_v4 "
              << virela << ' ' << structural << ' ' << defeat << '\n';
    std::cout << "[TRACE] canonical_playthrough_v4_repeat "
              << repeated_virela << ' ' << repeated_structural << ' ' << repeated_defeat << '\n';
    std::cout << "[TRACE] ordered_events_v2 virela_win baseline " << ordered_events_hex(virela_trace) << '\n';
    std::cout << "[TRACE] ordered_events_v2 virela_win repeat " << ordered_events_hex(repeated_virela_trace) << '\n';
    std::cout << "[TRACE] ordered_events_v2 structural_win baseline " << ordered_events_hex(structural_trace) << '\n';
    std::cout << "[TRACE] ordered_events_v2 structural_win repeat " << ordered_events_hex(repeated_structural_trace) << '\n';
    std::cout << "[TRACE] ordered_events_v2 no_ability_loss baseline " << ordered_events_hex(defeat_trace) << '\n';
    std::cout << "[TRACE] ordered_events_v2 no_ability_loss repeat " << ordered_events_hex(repeated_defeat_trace) << '\n';
}

}
