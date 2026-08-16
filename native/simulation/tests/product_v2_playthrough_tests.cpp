#include "test_framework.hpp"

#include "ninho/simulation/session.hpp"
#include "session_test_facade.hpp"

#include <array>
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <numbers>
#include <span>
#include <sstream>
#include <string_view>
#include <vector>

namespace {

std::string read_file(std::string_view relative)
{
    std::ifstream stream{std::filesystem::path{NINHO_SOURCE_DIR} / relative,
        std::ios::binary};
    NINHO_SIM_REQUIRE(stream.is_open());
    std::ostringstream text;
    text << stream.rdbuf();
    return text.str();
}

std::unique_ptr<ninho::simulation::SimulationSession> make_farm_session()
{
    using namespace ninho::simulation;
    const auto materials = parse_material_catalog(
        read_file("game/data/materials/product_v2.materials.json"));
    const auto archetypes = parse_archetype_catalog(
        read_file("game/data/archetypes/product_v2.archetypes.json"));
    const auto level = parse_level_manifest(
        read_file("game/data/levels/earth/farm_reaction.level.json"));
    NINHO_SIM_REQUIRE(materials.ok() && archetypes.ok() && level.ok());
    auto session = SimulationSession::create(
        materials.value, archetypes.value, level.value);
    NINHO_SIM_REQUIRE(session.ok());
    return std::move(session.value);
}

struct PublicShot {
    ninho::physics::Vec3 camera_right{};
    double pull_horizontal_m{};
    double pull_vertical_m{};
    std::uint64_t ability_tick_after_launch{};
};

struct Pig3CatchTrace {
    std::vector<std::uint64_t> catch_contact_ticks;
    std::vector<std::uint64_t> platform_contact_ticks;
};

struct GlassRackTrace {
    std::array<bool, 30> hammer_panel_contacts{};
};

void capture_tick(ninho::simulation::SimulationSession& session,
    std::vector<ninho::simulation::DomainEvent>& events,
    Pig3CatchTrace* pig3_catch = nullptr,
    GlassRackTrace* glass_rack = nullptr)
{
    NINHO_SIM_REQUIRE(session.tick().ok());
    events.insert(events.end(), session.events().begin(), session.events().end());
    if (pig3_catch != nullptr
        && ninho::simulation::detail::SessionTestFacade::physical_contact(
            session, ninho::simulation::EntityId{2003U},
            ninho::simulation::EntityId{3036U})) {
        pig3_catch->catch_contact_ticks.push_back(session.state().tick.value());
    }
    if (pig3_catch != nullptr
        && ninho::simulation::detail::SessionTestFacade::physical_contact(
            session, ninho::simulation::EntityId{2003U},
            ninho::simulation::EntityId{3042U})) {
        pig3_catch->platform_contact_ticks.push_back(session.state().tick.value());
    }
    if (glass_rack != nullptr) {
        for (std::size_t index = 0; index < glass_rack->hammer_panel_contacts.size();
             ++index) {
            glass_rack->hammer_panel_contacts[index] =
                glass_rack->hammer_panel_contacts[index]
                || ninho::simulation::detail::SessionTestFacade::physical_contact(
                    session, ninho::simulation::EntityId{3074U},
                    ninho::simulation::EntityId{
                        3077U + static_cast<std::uint32_t>(index)}).has_value();
        }
    }
}

void play_public_shot(ninho::simulation::SimulationSession& session,
    std::vector<ninho::simulation::DomainEvent>& events, const PublicShot& shot,
    Pig3CatchTrace* pig3_catch = nullptr,
    GlassRackTrace* glass_rack = nullptr)
{
    using namespace ninho::simulation;
    NINHO_SIM_REQUIRE(session.state().phase == SessionPhase::Inspection);
    NINHO_SIM_REQUIRE(session.enqueue(BeginGrabCommand{shot.camera_right}).ok());
    capture_tick(session, events, pig3_catch, glass_rack);
    NINHO_SIM_REQUIRE(session.enqueue(SetPullCommand{
        shot.pull_horizontal_m, shot.pull_vertical_m}).ok());
    NINHO_SIM_REQUIRE(session.enqueue(ReleaseBirdCommand{}).ok());
    capture_tick(session, events, pig3_catch, glass_rack);
    const auto launched = session.shot_state();
    NINHO_SIM_REQUIRE(launched.has_value());
    const std::uint64_t ability_tick = launched->launch_tick.value()
        + shot.ability_tick_after_launch;
    while (session.state().phase == SessionPhase::FlightAbility
        && session.state().tick.value() + 1U < ability_tick) {
        capture_tick(session, events, pig3_catch, glass_rack);
    }
    NINHO_SIM_REQUIRE(session.state().phase == SessionPhase::FlightAbility);
    NINHO_SIM_REQUIRE(session.enqueue(ActivateAbilityCommand{}).ok());
    capture_tick(session, events, pig3_catch, glass_rack);
    for (std::uint32_t watchdog = 0; watchdog < 2500U; ++watchdog) {
        if (session.state().phase == SessionPhase::Evaluation) {
            capture_tick(session, events, pig3_catch, glass_rack);
            return;
        }
        if (session.state().phase == SessionPhase::Inspection
            || session.state().phase == SessionPhase::Result) {
            return;
        }
        capture_tick(session, events, pig3_catch, glass_rack);
    }
    NINHO_SIM_REQUIRE(false);
}

bool is_contact_pair(const ninho::simulation::DomainEvent& event,
    std::uint32_t first, std::uint32_t second)
{
    using ninho::simulation::DomainEventKind;
    return event.kind == DomainEventKind::DamageApplied
        && ((event.entity_id.value() == first && event.affected_entity_id.value() == second)
            || (event.entity_id.value() == second && event.affected_entity_id.value() == first));
}

template <typename Predicate>
std::size_t first_event_after(const std::vector<ninho::simulation::DomainEvent>& events,
    std::size_t begin, Predicate predicate)
{
    const auto found = std::find_if(events.begin() + static_cast<std::ptrdiff_t>(begin),
        events.end(), predicate);
    NINHO_SIM_REQUIRE(found != events.end());
    return static_cast<std::size_t>(std::distance(events.begin(), found));
}

NINHO_SIM_TEST("product v2 playthrough farm counterweight releases the hay chute silo chain")
{
    using namespace ninho::simulation;
    auto session = make_farm_session();
    std::vector<DomainEvent> events;
    play_public_shot(*session, events,
        {{0.984807753F, 0.0F, -0.173648178F}, -4.0, -0.4, 10U});
    play_public_shot(*session, events,
        {{0.998629535F, 0.0F, -0.052335956F},
            -3.7099248270557466, -0.82247053296479089, 34U});

    const std::size_t counterweight_to_latch = first_event_after(events, 0U,
        [](const DomainEvent& event) { return is_contact_pair(event, 3019U, 3073U); });
    const std::size_t latch_released = first_event_after(events,
        counterweight_to_latch,
        [](const DomainEvent& event) {
            return event.kind == DomainEventKind::JointBroken
                && event.joint_id.value() == 18U;
        });
    const DomainEvent& broken = events[latch_released];
    const auto overload = std::ranges::find(
        events, broken.cause_event_id, &DomainEvent::id);
    NINHO_SIM_REQUIRE(overload != events.end());
    NINHO_SIM_REQUIRE(overload->kind == DomainEventKind::JointOverloaded);
    NINHO_SIM_REQUIRE(overload->joint_id.value() == 18U);
    const auto impact = std::ranges::find(
        events, overload->cause_event_id, &DomainEvent::id);
    NINHO_SIM_REQUIRE(impact != events.end());
    NINHO_SIM_REQUIRE(is_contact_pair(*impact, 3019U, 3073U));
    NINHO_SIM_REQUIRE(impact->id == events[counterweight_to_latch].id);
    NINHO_SIM_REQUIRE(std::ranges::none_of(events, [](const DomainEvent& event) {
        return is_contact_pair(event, 2147483649U, 3073U);
    }));

    std::array<bool, 3> released_hay{};
    const std::size_t platform_to_hay = first_event_after(events, latch_released,
        [](const DomainEvent& event) {
            return std::ranges::any_of(std::array{3021U, 3022U, 3023U},
                [&](std::uint32_t hay) { return is_contact_pair(event, 3026U, hay); });
        });
    for (const DomainEvent& event : std::span{events}.subspan(platform_to_hay)) {
        for (std::size_t hay = 0; hay < released_hay.size(); ++hay) {
            released_hay[hay] = released_hay[hay]
                || is_contact_pair(event, 3026U,
                    3021U + static_cast<std::uint32_t>(hay));
        }
    }
    NINHO_SIM_REQUIRE(std::ranges::count(released_hay, true) >= 2);

    const std::size_t hay_to_ramp = first_event_after(events, platform_to_hay,
        [](const DomainEvent& event) {
            return std::ranges::any_of(std::array{3021U, 3022U, 3023U},
                [&](std::uint32_t hay) {
                    return is_contact_pair(event, hay, 3024U)
                        || is_contact_pair(event, hay, 3025U);
                });
        });
    const std::size_t hay_to_chute = first_event_after(events, platform_to_hay,
        [](const DomainEvent& event) {
            return std::ranges::any_of(std::array{3021U, 3022U, 3023U},
                [&](std::uint32_t hay) { return is_contact_pair(event, hay, 3013U); });
        });
    NINHO_SIM_REQUIRE(hay_to_ramp < hay_to_chute);
    const std::size_t ramp_to_silo = first_event_after(events, hay_to_ramp,
        [](const DomainEvent& event) {
            for (std::uint32_t silo = 3036U; silo <= 3045U; ++silo) {
                if (is_contact_pair(event, 3024U, silo)
                    || is_contact_pair(event, 3025U, silo)) {
                    return true;
                }
            }
            return false;
        });
    const std::size_t silo_post_released = first_event_after(events, ramp_to_silo,
        [](const DomainEvent& event) {
            return event.kind == DomainEventKind::JointBroken
                && event.joint_id.value() == 34U;
        });
    const DomainEvent& post_break = events[silo_post_released];
    const auto post_overload = std::ranges::find(
        events, post_break.cause_event_id, &DomainEvent::id);
    NINHO_SIM_REQUIRE(post_overload != events.end());
    NINHO_SIM_REQUIRE(post_overload->kind == DomainEventKind::JointOverloaded);
    const auto post_impact = std::ranges::find(
        events, post_overload->cause_event_id, &DomainEvent::id);
    NINHO_SIM_REQUIRE(post_impact != events.end());
    NINHO_SIM_REQUIRE(is_contact_pair(*post_impact, 3043U, 3045U));

    const std::size_t fuel_damaged = first_event_after(events, silo_post_released,
        [](const DomainEvent& event) {
            return event.kind == DomainEventKind::DamageApplied
                && event.affected_entity_id.value() == 3058U
                && (event.entity_id.value() == 3043U
                    || event.entity_id.value() == 3045U);
        });
    const std::size_t fuel_armed = first_event_after(events, fuel_damaged,
        [](const DomainEvent& event) {
            return event.kind == DomainEventKind::EnvironmentalTriggerArmed
                && event.environmental_trigger_id == 1U;
        });
    NINHO_SIM_REQUIRE(events[fuel_armed].cause_event_id == events[fuel_damaged].id);
    const std::size_t fuel_detonated = first_event_after(events, fuel_armed,
        [](const DomainEvent& event) {
            return event.kind == DomainEventKind::EnvironmentalTriggerDetonated
                && event.environmental_trigger_id == 1U;
        });
    NINHO_SIM_REQUIRE(events[fuel_detonated].cause_event_id == events[fuel_armed].id);
    const std::size_t fuel_burst = first_event_after(events, fuel_detonated,
        [](const DomainEvent& event) {
            return event.kind == DomainEventKind::PressureBurst
                && event.environmental_trigger_id == 1U
                && event.entity_id.value() == 3058U;
        });
    NINHO_SIM_REQUIRE(events[fuel_burst].cause_event_id
        == events[fuel_detonated].id);
    const std::size_t pig4_damaged = first_event_after(events, fuel_burst,
        [](const DomainEvent& event) {
            return event.kind == DomainEventKind::DamageApplied
                && event.affected_entity_id.value() == 2004U;
        });
    NINHO_SIM_REQUIRE(events[pig4_damaged].cause_event_id
        == events[fuel_burst].id);
    first_event_after(events, pig4_damaged, [](const DomainEvent& event) {
        return event.kind == DomainEventKind::EntityNeutralized
            && event.affected_entity_id.value() == 2004U;
    });
}

NINHO_SIM_TEST("product v2 playthrough side bale striker drops the barn roof on the normative pig")
{
    using namespace ninho::simulation;
    auto session = make_farm_session();
    std::vector<DomainEvent> events;
    play_public_shot(*session, events,
        {{0.984807753F, 0.0F, -0.173648178F}, -4.0, -0.4, 10U});
    play_public_shot(*session, events,
        {{0.998629535F, 0.0F, -0.052335956F},
            -3.7099248270557466, -0.82247053296479089, 34U});

    const std::size_t latch_impact = first_event_after(events, 0U,
        [](const DomainEvent& event) {
            return is_contact_pair(event, 3019U, 3073U);
        });
    const std::size_t latch_break = first_event_after(events, latch_impact,
        [](const DomainEvent& event) {
            return event.kind == DomainEventKind::JointBroken
                && event.joint_id.value() == 18U;
        });
    const auto latch_overload = std::ranges::find(
        events, events[latch_break].cause_event_id, &DomainEvent::id);
    NINHO_SIM_REQUIRE(latch_overload != events.end());
    NINHO_SIM_REQUIRE(latch_overload->cause_event_id == events[latch_impact].id);

    const std::size_t striker_released = first_event_after(events, latch_break,
        [](const DomainEvent& event) {
            return is_contact_pair(event, 3014U, 3023U);
        });
    const std::size_t wall_hit = first_event_after(events, striker_released,
        [](const DomainEvent& event) {
            return is_contact_pair(event, 3026U, 3035U);
        });
    const std::size_t wall_released = first_event_after(events, wall_hit,
        [](const DomainEvent& event) {
            return event.kind == DomainEventKind::JointBroken
                && event.joint_id.value() == 26U;
        });
    const auto wall_overload = std::ranges::find(
        events, events[wall_released].cause_event_id, &DomainEvent::id);
    NINHO_SIM_REQUIRE(wall_overload != events.end());
    NINHO_SIM_REQUIRE(wall_overload->kind == DomainEventKind::JointOverloaded);
    NINHO_SIM_REQUIRE(wall_overload->cause_event_id == events[wall_hit].id);

    const std::size_t hay_to_ramp = first_event_after(events, wall_released,
        [](const DomainEvent& event) {
            return is_contact_pair(event, 3022U, 3025U);
        });
    const std::size_t post_hit = first_event_after(events, hay_to_ramp,
        [](const DomainEvent& event) {
            return is_contact_pair(event, 3022U, 3029U);
        });
    const std::size_t post_released = first_event_after(events, post_hit,
        [](const DomainEvent& event) {
            return event.kind == DomainEventKind::JointBroken
                && event.joint_id.value() == 20U;
        });
    const auto post_overload = std::ranges::find(
        events, events[post_released].cause_event_id, &DomainEvent::id);
    NINHO_SIM_REQUIRE(post_overload != events.end());
    NINHO_SIM_REQUIRE(post_overload->kind == DomainEventKind::JointOverloaded);
    NINHO_SIM_REQUIRE(post_overload->cause_event_id == events[post_hit].id);
    const std::size_t crushed = first_event_after(events, wall_released,
        [](const DomainEvent& event) {
            return event.kind == DomainEventKind::CrushDamageApplied
                && event.entity_id.value() == 3035U
                && event.affected_entity_id.value() == 2002U;
        });
    const std::size_t damaged = first_event_after(events, crushed,
        [&](const DomainEvent& event) {
            return event.kind == DomainEventKind::DamageApplied
                && event.affected_entity_id.value() == 2002U
                && event.cause_event_id == events[crushed].id;
        });
    NINHO_SIM_REQUIRE(events[damaged].cause_event_id == events[crushed].id);
    const std::size_t neutralized = first_event_after(events, damaged,
        [](const DomainEvent& event) {
            return event.kind == DomainEventKind::EntityNeutralized
                && event.affected_entity_id.value() == 2002U;
        });
    const auto final_cause = std::ranges::find(
        events, events[neutralized].cause_event_id, &DomainEvent::id);
    NINHO_SIM_REQUIRE(final_cause != events.end());
    NINHO_SIM_REQUIRE(final_cause->kind == DomainEventKind::CrushDamageApplied);
    NINHO_SIM_REQUIRE(final_cause->entity_id.value() == 3035U);

    std::size_t relay = hay_to_ramp;
    for (const std::uint32_t joint : {27U, 28U, 35U, 34U}) {
        relay = first_event_after(events, relay,
            [joint](const DomainEvent& event) {
                return event.kind == DomainEventKind::JointBroken
                    && event.joint_id.value() == joint;
            });
        const auto overload = std::ranges::find(
            events, events[relay].cause_event_id, &DomainEvent::id);
        NINHO_SIM_REQUIRE(overload != events.end());
        NINHO_SIM_REQUIRE(overload->kind == DomainEventKind::JointOverloaded);
        NINHO_SIM_REQUIRE(overload->joint_id.value() == joint);
        const auto physical_cause = std::ranges::find(
            events, overload->cause_event_id, &DomainEvent::id);
        NINHO_SIM_REQUIRE(physical_cause != events.end());
        NINHO_SIM_REQUIRE(physical_cause->kind == DomainEventKind::DamageApplied);
    }
    const std::size_t fuel_hit = first_event_after(events, relay,
        [](const DomainEvent& event) {
            return event.kind == DomainEventKind::DamageApplied
                && event.entity_id.value() == 3045U
                && event.affected_entity_id.value() == 3058U;
        });
    NINHO_SIM_REQUIRE(events[fuel_hit].damage > 40.0);
    const std::size_t fuel_armed = first_event_after(events, fuel_hit,
        [&](const DomainEvent& event) {
            return event.kind == DomainEventKind::EnvironmentalTriggerArmed
                && event.environmental_trigger_id == 1U
                && event.cause_event_id == events[fuel_hit].id;
        });
    const std::size_t fuel_detonated = first_event_after(events, fuel_armed,
        [&](const DomainEvent& event) {
            return event.kind == DomainEventKind::EnvironmentalTriggerDetonated
                && event.environmental_trigger_id == 1U
                && event.cause_event_id == events[fuel_armed].id;
        });
    first_event_after(events, fuel_detonated,
        [&](const DomainEvent& event) {
            return event.kind == DomainEventKind::PressureBurst
                && event.environmental_trigger_id == 1U
                && event.cause_event_id == events[fuel_detonated].id;
        });
}

NINHO_SIM_TEST("product v2 playthrough silo catch holds pig three for causal crush")
{
    using namespace ninho::simulation;
    auto session = make_farm_session();
    std::vector<DomainEvent> events;
    Pig3CatchTrace catch_trace;
    play_public_shot(*session, events,
        {{0.984807753F, 0.0F, -0.173648178F}, -4.0, -0.4, 10U},
        &catch_trace);
    play_public_shot(*session, events,
        {{0.998629535F, 0.0F, -0.052335956F},
            -3.7099248270557466, -0.82247053296479089, 34U},
        &catch_trace);

    const std::size_t burst = first_event_after(events, 0U,
        [](const DomainEvent& event) {
            return event.kind == DomainEventKind::PressureBurst
                && event.environmental_trigger_id == 1U
                && event.entity_id.value() == 3058U;
        });
    const std::uint64_t burst_tick = events[burst].tick.value();
    const auto catch_contact = std::ranges::find_if(
        catch_trace.catch_contact_ticks,
        [&](const std::uint64_t tick) { return tick >= burst_tick; });
    NINHO_SIM_REQUIRE(catch_contact != catch_trace.catch_contact_ticks.end());
    const std::size_t catch_damage = first_event_after(events, burst,
        [](const DomainEvent& event) {
            return event.kind == DomainEventKind::DamageApplied
                && event.entity_id.value() == 2003U
                && event.affected_entity_id.value() == 3036U;
        });
    NINHO_SIM_REQUIRE(events[catch_damage].tick.value() >= *catch_contact);
    const std::size_t crushed = first_event_after(events, burst,
        [](const DomainEvent& event) {
            return event.kind == DomainEventKind::CrushDamageApplied
                && event.entity_id.value() == 3042U
                && event.affected_entity_id.value() == 2003U;
        });
    NINHO_SIM_REQUIRE(events[crushed].tick.value()
        > events[catch_damage].tick.value());
    NINHO_SIM_REQUIRE(std::ranges::find(catch_trace.platform_contact_ticks,
        events[crushed].tick.value()) != catch_trace.platform_contact_ticks.end());
    const std::size_t damaged = first_event_after(events, crushed,
        [&](const DomainEvent& event) {
            return event.kind == DomainEventKind::DamageApplied
                && event.affected_entity_id.value() == 2003U
                && event.cause_event_id == events[crushed].id;
        });
    const std::size_t neutralized = first_event_after(events, damaged,
        [](const DomainEvent& event) {
            return event.kind == DomainEventKind::EntityNeutralized
                && event.affected_entity_id.value() == 2003U;
        });
    const auto cause = std::ranges::find(
        events, events[neutralized].cause_event_id, &DomainEvent::id);
    NINHO_SIM_REQUIRE(cause != events.end());
    NINHO_SIM_REQUIRE(cause->kind == DomainEventKind::CrushDamageApplied);
    NINHO_SIM_REQUIRE(cause->entity_id.value() == 3042U);
    NINHO_SIM_REQUIRE(cause->affected_entity_id.value() == 2003U);
}

NINHO_SIM_TEST("product v2 playthrough translated pressure cage neutralizes pig four")
{
    using namespace ninho::simulation;
    auto session = make_farm_session();
    std::vector<DomainEvent> events;
    Pig3CatchTrace catch_trace;
    play_public_shot(*session, events,
        {{0.984807753F, 0.0F, -0.173648178F}, -4.0, -0.4, 10U},
        &catch_trace);
    play_public_shot(*session, events,
        {{0.998629535F, 0.0F, -0.052335956F},
            -3.7099248270557466, -0.82247053296479089, 34U},
        &catch_trace);

    const std::size_t detonated = first_event_after(events, 0U,
        [](const DomainEvent& event) {
            return event.kind == DomainEventKind::EnvironmentalTriggerDetonated
                && event.environmental_trigger_id == 2U
                && event.entity_id.value() == 3059U;
        });
    const std::size_t burst = first_event_after(events, detonated,
        [&](const DomainEvent& event) {
            return event.kind == DomainEventKind::PressureBurst
                && event.environmental_trigger_id == 2U
                && event.entity_id.value() == 3059U
                && event.cause_event_id == events[detonated].id;
        });
    const std::size_t damaged = first_event_after(events, burst,
        [&](const DomainEvent& event) {
            return event.kind == DomainEventKind::DamageApplied
                && event.entity_id.value() == 3059U
                && event.affected_entity_id.value() == 2004U
                && event.cause_event_id == events[burst].id;
        });
    const std::size_t neutralized = first_event_after(events, damaged,
        [&](const DomainEvent& event) {
            return event.kind == DomainEventKind::EntityNeutralized
                && event.entity_id.value() == 3059U
                && event.affected_entity_id.value() == 2004U
                && event.cause_event_id == events[burst].id;
        });
    NINHO_SIM_REQUIRE(events[neutralized].tick.value()
        == events[damaged].tick.value());
    NINHO_SIM_REQUIRE(session->state().outcome == Outcome::Victory);
}

NINHO_SIM_TEST("product v2 playthrough pressure hammer roots the panel rack for three stars")
{
    using namespace ninho::simulation;
    auto session = make_farm_session();
    NINHO_SIM_REQUIRE(std::ranges::any_of(session->snapshots(), [](const auto& body) {
        return body.entity_id == EntityId{3076U};
    }));
    for (std::uint32_t entity = 3077U; entity <= 3106U; ++entity) {
        NINHO_SIM_REQUIRE(std::ranges::any_of(session->snapshots(),
            [&](const auto& body) { return body.entity_id == EntityId{entity}; }));
    }

    std::vector<DomainEvent> events;
    GlassRackTrace rack_trace;
    play_public_shot(*session, events,
        {{0.984807753F, 0.0F, -0.173648178F}, -4.0, -0.4, 10U},
        nullptr, &rack_trace);
    play_public_shot(*session, events,
        {{0.998629535F, 0.0F, -0.052335956F},
            -3.7099248270557466, -0.82247053296479089, 34U},
        nullptr, &rack_trace);

    NINHO_SIM_REQUIRE(session->state().outcome == Outcome::Victory);
    NINHO_SIM_REQUIRE(session->birds_remaining() == 2U);
    NINHO_SIM_REQUIRE(session->score_state().score >= 50'000U);
    NINHO_SIM_REQUIRE(session->score_state().stars == 3U);
    NINHO_SIM_REQUIRE(std::ranges::count(
        events, DomainEventKind::BirdLaunched, &DomainEvent::kind) == 2);
    const auto yellow_launch = std::ranges::find_if(events,
        [](const DomainEvent& event) {
            return event.kind == DomainEventKind::BirdLaunched
                && event.shot_id == 2U;
        });
    NINHO_SIM_REQUIRE(yellow_launch != events.end());
    for (std::uint32_t pig = 2001U; pig <= 2004U; ++pig) {
        NINHO_SIM_REQUIRE(std::ranges::any_of(events, [&](const DomainEvent& event) {
            return event.kind == DomainEventKind::EntityNeutralized
                && event.affected_entity_id == EntityId{pig};
        }));
    }
    for (const std::uint32_t joint : {18U, 27U, 28U, 35U, 34U}) {
        NINHO_SIM_REQUIRE(std::ranges::any_of(events, [&](const DomainEvent& event) {
            return event.kind == DomainEventKind::JointBroken
                && event.joint_id == JointId{joint};
        }));
    }
    for (const std::uint32_t trigger : {1U, 2U}) {
        NINHO_SIM_REQUIRE(std::ranges::any_of(events, [&](const DomainEvent& event) {
            return event.kind == DomainEventKind::PressureBurst
                && event.environmental_trigger_id == trigger;
        }));
    }

    const auto pressure_burst = std::ranges::find_if(events,
        [](const DomainEvent& event) {
            return event.kind == DomainEventKind::PressureBurst
                && event.environmental_trigger_id == 2U
                && event.entity_id == EntityId{3059U};
        });
    NINHO_SIM_REQUIRE(pressure_burst != events.end());
    NINHO_SIM_REQUIRE(std::ranges::any_of(events, [&](const DomainEvent& event) {
        return event.kind == DomainEventKind::DamageApplied
            && event.entity_id == EntityId{3059U}
            && event.affected_entity_id == EntityId{3074U}
            && event.cause_event_id == pressure_burst->id;
    }));
    NINHO_SIM_REQUIRE(std::ranges::none_of(events, [](const DomainEvent& event) {
        return event.kind == DomainEventKind::DamageApplied
            && (event.entity_id == EntityId{3058U}
                || event.entity_id == EntityId{3059U})
            && event.affected_entity_id.value() >= 3077U
            && event.affected_entity_id.value() <= 3106U;
    }));
    constexpr std::array rooted_panels{
        EntityId{3078U}, EntityId{3079U}, EntityId{3080U},
        EntityId{3083U}, EntityId{3084U}, EntityId{3085U},
        EntityId{3088U}, EntityId{3089U}, EntityId{3090U},
        EntityId{3093U}, EntityId{3094U}, EntityId{3095U},
        EntityId{3098U}, EntityId{3099U}, EntityId{3100U},
        EntityId{3103U}, EntityId{3104U}, EntityId{3105U},
    };
    NINHO_SIM_REQUIRE(static_cast<std::size_t>(std::ranges::count(
        rack_trace.hammer_panel_contacts, true)) >= rooted_panels.size());

    for (std::uint32_t panel = 3077U; panel <= 3106U; ++panel) {
        const bool expected = std::ranges::find(
            rooted_panels, EntityId{panel}) != rooted_panels.end();
        if (expected) {
            NINHO_SIM_REQUIRE(
                rack_trace.hammer_panel_contacts[panel - 3077U]);
        }
        const auto fractured = std::ranges::find_if(events,
            [&](const DomainEvent& event) {
                return event.kind == DomainEventKind::PieceFractured
                    && event.affected_entity_id == EntityId{panel};
            });
        if (!expected) {
            NINHO_SIM_REQUIRE(fractured == events.end());
            NINHO_SIM_REQUIRE(std::ranges::none_of(events,
                [&](const DomainEvent& event) {
                    return event.kind == DomainEventKind::ScoreAwarded
                        && event.scoring_identity_kind
                            == ScoringIdentityKind::MaterialPiece
                        && event.affected_entity_id == EntityId{panel};
                }));
            continue;
        }
        NINHO_SIM_REQUIRE(fractured != events.end());
        NINHO_SIM_REQUIRE(std::ranges::count_if(events,
            [&](const DomainEvent& event) {
                return event.kind == DomainEventKind::PieceFractured
                    && event.affected_entity_id == EntityId{panel};
            }) == 1);
        const auto fracture_trigger = std::ranges::find(
            events, fractured->cause_event_id, &DomainEvent::id);
        NINHO_SIM_REQUIRE(fracture_trigger != events.end());
        NINHO_SIM_REQUIRE(
            fracture_trigger->kind == DomainEventKind::PieceFractureTriggered);
        const auto contact_damage = std::ranges::find(
            events, fracture_trigger->cause_event_id, &DomainEvent::id);
        NINHO_SIM_REQUIRE(contact_damage != events.end());
        NINHO_SIM_REQUIRE(is_contact_pair(*contact_damage, 3074U, panel));
        const auto score_awarded = std::ranges::find_if(events,
            [&](const DomainEvent& event) {
                return event.kind == DomainEventKind::ScoreAwarded
                    && event.scoring_identity_kind
                        == ScoringIdentityKind::MaterialPiece
                    && event.affected_entity_id == EntityId{panel}
                    && event.affected_part_id == PartId{1U};
            });
        NINHO_SIM_REQUIRE(score_awarded != events.end());
        NINHO_SIM_REQUIRE(std::ranges::count_if(events,
            [&](const DomainEvent& event) {
                return event.kind == DomainEventKind::ScoreAwarded
                    && event.scoring_identity_kind
                        == ScoringIdentityKind::MaterialPiece
                    && event.affected_entity_id == EntityId{panel}
                    && event.affected_part_id == PartId{1U};
            }) == 1);
        NINHO_SIM_REQUIRE(score_awarded->cause_event_id == fractured->id);
        NINHO_SIM_REQUIRE(score_awarded->base_points == 160U);
        NINHO_SIM_REQUIRE(
            score_awarded->root_cause_event_id == yellow_launch->id);
        NINHO_SIM_REQUIRE(score_awarded->shot_id == 2U);
        const std::uint32_t expected_multiplier = std::min(
            200U, 100U + 10U * std::min(score_awarded->chain_index - 1U, 10U));
        NINHO_SIM_REQUIRE(
            score_awarded->multiplier_percent == expected_multiplier);
        NINHO_SIM_REQUIRE(score_awarded->awarded_points
            == (160U * expected_multiplier + 50U) / 100U);
    }
}

NINHO_SIM_TEST("product v2 playthrough impact on authored static support stays finite")
{
    using namespace ninho::simulation;
    auto session = make_farm_session();
    NINHO_SIM_REQUIRE(session->enqueue(
        BeginGrabCommand{{1.0F, 0.0F, 0.0F}}).ok());
    NINHO_SIM_REQUIRE(session->tick().ok());
    NINHO_SIM_REQUIRE(session->enqueue(SetPullCommand{-4.0, -1.0}).ok());
    NINHO_SIM_REQUIRE(session->enqueue(ReleaseBirdCommand{}).ok());
    NINHO_SIM_REQUIRE(session->tick().ok());
    const auto launch = session->shot_state();
    NINHO_SIM_REQUIRE(launch.has_value());
    while (session->state().tick.value() < launch->launch_tick.value() + 9U) {
        NINHO_SIM_REQUIRE(session->tick().ok());
    }
    NINHO_SIM_REQUIRE(session->enqueue(ActivateAbilityCommand{}).ok());
    NINHO_SIM_REQUIRE(session->tick().ok());
    for (int tick = 0; tick < 60; ++tick) {
        NINHO_SIM_REQUIRE(session->tick().ok());
    }
    NINHO_SIM_REQUIRE(session->state().phase != SessionPhase::Faulted);
    NINHO_SIM_REQUIRE(session->ability_readiness() == AbilityReadiness::Spent);
}

}
