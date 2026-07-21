#pragma once

#include "ninho/simulation/content.hpp"
#include "ninho/simulation/events.hpp"

#include <compare>
#include <cstdint>
#include <span>
#include <vector>

namespace ninho::simulation::detail {

struct ScoringIdentity {
    ScoringIdentityKind kind{ScoringIdentityKind::EnemyEntity};
    EntityId entity_id{};
    PartId part_id{};
    std::uint32_t queue_slot_id{};

    [[nodiscard]] static constexpr ScoringIdentity enemy(EntityId entity) noexcept
    {
        return {.kind = ScoringIdentityKind::EnemyEntity, .entity_id = entity};
    }

    [[nodiscard]] static constexpr ScoringIdentity material(
        EntityId entity, PartId part) noexcept
    {
        return {.kind = ScoringIdentityKind::MaterialPiece,
            .entity_id = entity, .part_id = part};
    }

    [[nodiscard]] static constexpr ScoringIdentity unused_bird(
        std::uint32_t slot) noexcept
    {
        return {.kind = ScoringIdentityKind::UnusedBirdSlot,
            .queue_slot_id = slot};
    }

    constexpr auto operator<=>(const ScoringIdentity&) const noexcept = default;
};

struct ScoringCandidate {
    ScoringIdentity identity;
    EventId source_event_id{};
    std::uint32_t base_points{};
};

enum class ScoreTerminalState : std::uint8_t { None = 0, Victory = 1, Defeat = 2 };
enum class ScoreTransitionKind : std::uint8_t {
    ChainChanged = 0,
    Awarded = 1,
    StarsAwarded = 2,
};

struct ScoreState {
    std::uint64_t current_score{};
    std::uint32_t chain_index{};
    std::uint32_t multiplier_percent{100U};
    TickIndex last_scoring_tick{};
    EventId current_chain_root_cause_event_id{};
    std::uint64_t current_chain_shot_id{};
    std::vector<ScoringIdentity> scored_identities;
    std::uint8_t stars{};
    bool terminal_bonus_awarded{};

    bool operator==(const ScoreState&) const = default;
};

struct ScoreTransition {
    ScoreTransitionKind kind{ScoreTransitionKind::ChainChanged};
    ScoringIdentity identity;
    EventId source_event_id{};
    EventId root_cause_event_id{};
    std::uint64_t shot_id{};
    std::uint32_t base_points{};
    std::uint32_t multiplier_percent{100U};
    std::uint32_t chain_index{};
    std::uint64_t awarded_points{};
    std::uint64_t total_score{};
    std::uint8_t stars{};
};

struct ScoreTickInput {
    TickIndex tick{};
    std::span<const DomainEvent> events;
    std::span<const ScoringCandidate> candidates;
    ScoreTerminalState terminal{ScoreTerminalState::None};
    std::uint32_t remaining_birds{};
};

class ScoreSystem {
public:
    struct CausalRecord {
        EventId event_id{};
        EventId root_event_id{};
        std::uint64_t shot_id{};
    };
    struct EntityRoot {
        EntityId entity_id{};
        EventId root_event_id{};
        std::uint64_t shot_id{};
    };

    ScoreSystem() = default;
    ScoreSystem(ScoringDefinition, std::uint32_t initial_queue_size);

    [[nodiscard]] std::vector<ScoreTransition> consume(const ScoreTickInput&);
    [[nodiscard]] const ScoreState& state() const noexcept { return state_; }
    [[nodiscard]] ScoreState& state_for_testing() noexcept { return state_; }
    [[nodiscard]] bool configured() const noexcept { return configured_; }
    [[nodiscard]] std::span<const CausalRecord> causal_records() const noexcept
    {
        return causal_records_;
    }
    [[nodiscard]] std::span<const EntityRoot> entity_roots() const noexcept
    {
        return entity_roots_;
    }
    [[nodiscard]] std::uint64_t observed_launches() const noexcept
    {
        return observed_launches_;
    }
    [[nodiscard]] bool has_root_for(EntityId entity) const noexcept
    {
        return entity_root(entity).root_event_id != EventId{};
    }

    [[nodiscard]] static constexpr std::uint32_t material_points(
        MaterialResponse response) noexcept
    {
        switch (response) {
        case MaterialResponse::Compressible: return 80U;
        case MaterialResponse::Fibrous: return 120U;
        case MaterialResponse::Brittle: return 160U;
        case MaterialResponse::Masonry: return 180U;
        case MaterialResponse::Ductile: return 220U;
        }
        return 0U;
    }

private:
    [[nodiscard]] CausalRecord causal_record(EventId) const noexcept;
    [[nodiscard]] EntityRoot entity_root(EntityId) const noexcept;
    void observe_event(const DomainEvent&);
    void reset_chain(std::vector<ScoreTransition>&);
    void award(const ScoringCandidate&, const DomainEvent&,
        std::vector<ScoreTransition>&);
    void finish_terminal(const ScoreTickInput&, std::vector<ScoreTransition>&);

    ScoringDefinition definition_{};
    std::uint32_t initial_queue_size_{};
    std::uint64_t observed_launches_{};
    bool configured_{};
    ScoreState state_;
    std::vector<CausalRecord> causal_records_;
    std::vector<EntityRoot> entity_roots_;
};

}
