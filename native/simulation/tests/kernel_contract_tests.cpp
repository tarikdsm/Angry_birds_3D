#include "test_framework.hpp"

#include "ninho/simulation/commands.hpp"
#include "ninho/simulation/content.hpp"
#include "ninho/simulation/events.hpp"
#include "ninho/simulation/session.hpp"

#include <concepts>
#include <cstdint>
#include <type_traits>
#include <variant>

namespace {

using ninho::simulation::EntityId;
using ninho::simulation::EventId;
using ninho::simulation::JointId;
using ninho::simulation::MaterialId;
using ninho::simulation::Outcome;
using ninho::simulation::PartId;
using ninho::simulation::SessionPhase;
using ninho::simulation::SessionState;
using ninho::simulation::TickIndex;

static_assert(!std::same_as<EntityId, PartId>);
static_assert(!std::same_as<EntityId, MaterialId>);
static_assert(!std::same_as<EntityId, JointId>);
static_assert(!std::same_as<EntityId, EventId>);
static_assert(!std::same_as<EntityId, TickIndex>);
static_assert(!std::same_as<PartId, MaterialId>);
static_assert(!std::same_as<PartId, JointId>);
static_assert(!std::same_as<PartId, EventId>);
static_assert(!std::same_as<PartId, TickIndex>);
static_assert(!std::same_as<MaterialId, JointId>);
static_assert(!std::same_as<MaterialId, EventId>);
static_assert(!std::same_as<MaterialId, TickIndex>);
static_assert(!std::same_as<JointId, EventId>);
static_assert(!std::same_as<JointId, TickIndex>);
static_assert(!std::same_as<EventId, TickIndex>);

template <typename Id, typename Representation>
inline constexpr bool has_strong_id_conversion_contract =
    std::is_constructible_v<Id, Representation>
    && !std::is_convertible_v<Representation, Id>
    && !std::is_convertible_v<Id, Representation>;

static_assert(has_strong_id_conversion_contract<EntityId, std::uint32_t>);
static_assert(has_strong_id_conversion_contract<PartId, std::uint32_t>);
static_assert(has_strong_id_conversion_contract<MaterialId, std::uint32_t>);
static_assert(has_strong_id_conversion_contract<JointId, std::uint32_t>);
static_assert(has_strong_id_conversion_contract<EventId, std::uint64_t>);
static_assert(has_strong_id_conversion_contract<TickIndex, std::uint64_t>);
static_assert(std::is_trivially_copyable_v<EntityId>);
static_assert(std::is_trivially_copyable_v<PartId>);
static_assert(std::is_trivially_copyable_v<MaterialId>);
static_assert(std::is_trivially_copyable_v<JointId>);
static_assert(std::is_trivially_copyable_v<EventId>);
static_assert(std::is_trivially_copyable_v<TickIndex>);
static_assert(std::is_enum_v<SessionPhase>);
static_assert(std::is_enum_v<Outcome>);
static_assert(std::variant_size_v<ninho::simulation::WorldDefinition> == 2U);
static_assert(std::variant_size_v<ninho::simulation::AbilityArchetype::Payload> == 5U);
static_assert(!std::same_as<ninho::simulation::UniformWorldDefinition,
    ninho::simulation::RadialWorldDefinition>);

NINHO_SIM_TEST("simulation contracts start in inspection without an outcome")
{
    const SessionState state{};

    NINHO_SIM_REQUIRE(state.phase == SessionPhase::Inspection);
    NINHO_SIM_REQUIRE(state.outcome == Outcome::None);
    NINHO_SIM_REQUIRE(state.tick == TickIndex{});
}

}
