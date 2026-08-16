#include "score_system.hpp"

#include <algorithm>
#include <limits>
#include <stdexcept>
#include <tuple>

namespace ninho::simulation::detail {
namespace {

constexpr std::uint32_t chain_multiplier_step_percent = 10U;
constexpr std::uint32_t maximum_chain_multiplier_percent = 200U;

}

ScoreSystem::ScoreSystem(ScoringDefinition definition, std::uint32_t initial_queue_size)
    : definition_(definition)
    , initial_queue_size_(initial_queue_size)
    , configured_(true)
{
    if (definition_.chain_window_ticks == 0U
        || initial_queue_size_ > 32U) {
        throw std::invalid_argument("invalid scoring definition");
    }
}

ScoreSystem::CausalRecord ScoreSystem::causal_record(EventId id) const noexcept
{
    const auto found = std::lower_bound(causal_records_.begin(), causal_records_.end(), id,
        [](const CausalRecord& record, EventId value) {
            return record.event_id < value;
        });
    return found != causal_records_.end() && found->event_id == id
        ? *found : CausalRecord{};
}

ScoreSystem::EntityRoot ScoreSystem::entity_root(EntityId id) const noexcept
{
    const auto found = std::lower_bound(entity_roots_.begin(), entity_roots_.end(), id,
        [](const EntityRoot& record, EntityId value) {
            return record.entity_id < value;
        });
    return found != entity_roots_.end() && found->entity_id == id
        ? *found : EntityRoot{};
}

void ScoreSystem::upsert_causal_record(CausalRecord record)
{
    const auto position = std::lower_bound(causal_records_.begin(),
        causal_records_.end(), record.event_id,
        [](const CausalRecord& current, EventId value) {
            return current.event_id < value;
        });
    if (position != causal_records_.end()
        && position->event_id == record.event_id) {
        *position = record;
    } else {
        causal_records_.insert(position, record);
    }
}

void ScoreSystem::upsert_entity_root(EntityRoot root)
{
    if (root.entity_id == EntityId{} || root.root_event_id == EventId{}) {
        return;
    }
    const auto position = std::lower_bound(entity_roots_.begin(),
        entity_roots_.end(), root.entity_id,
        [](const EntityRoot& current, EntityId value) {
            return current.entity_id < value;
    });
    if (position != entity_roots_.end()
        && position->entity_id == root.entity_id) {
        *position = root;
    } else {
        entity_roots_.insert(position, root);
    }
}

ScoreSystem::CausalRecord ScoreSystem::resolve_event(
    const DomainEvent& event) const noexcept
{
    CausalRecord record{.event_id = event.id};
    if (event.kind == DomainEventKind::BirdLaunched) {
        return causal_record(event.id);
    }
    if (event.cause_event_id != EventId{}) {
        const CausalRecord parent = causal_record(event.cause_event_id);
        record.root_event_id = parent.root_event_id;
        record.shot_id = parent.shot_id;
        return record;
    }
    const EntityRoot source = entity_root(event.entity_id);
    if (source.root_event_id != EventId{}) {
        record.root_event_id = source.root_event_id;
        record.shot_id = source.shot_id;
        return record;
    }
    const EntityRoot target = entity_root(event.affected_entity_id);
    record.root_event_id = target.root_event_id;
    record.shot_id = target.shot_id;
    return record;
}

std::vector<const DomainEvent*> ScoreSystem::resolve_tick_provenance(
    std::span<const DomainEvent> events,
    std::span<const EntityId> dynamic_entities,
    std::span<const std::pair<EntityId, EntityId>> dynamic_physical_edges,
    std::span<const JointEntityEndpoints> joint_entity_endpoints)
{
    std::vector<const DomainEvent*> ordered;
    ordered.reserve(events.size());
    for (const DomainEvent& event : events) {
        ordered.push_back(&event);
    }
    std::ranges::sort(ordered, [](const DomainEvent* lhs, const DomainEvent* rhs) {
        return lhs->id < rhs->id;
    });

    std::vector<std::pair<EntityId, EntityId>> physical_edges;
    physical_edges.reserve(dynamic_physical_edges.size());
    for (auto [first, second] : dynamic_physical_edges) {
        if (first == EntityId{} || second == EntityId{} || first == second
            || !std::binary_search(
                dynamic_entities.begin(), dynamic_entities.end(), first)
            || !std::binary_search(
                dynamic_entities.begin(), dynamic_entities.end(), second)) {
            continue;
        }
        if (second < first) std::swap(first, second);
        physical_edges.emplace_back(first, second);
    }
    std::ranges::sort(physical_edges);
    physical_edges.erase(
        std::unique(physical_edges.begin(), physical_edges.end()),
        physical_edges.end());

    std::vector<JointEntityEndpoints> historical_joints;
    historical_joints.reserve(joint_entity_endpoints.size());
    for (JointEntityEndpoints joint : joint_entity_endpoints) {
        if (joint.joint_id == JointId{}) continue;
        if (!std::binary_search(
                dynamic_entities.begin(), dynamic_entities.end(), joint.first)) {
            joint.first = {};
        }
        if (!std::binary_search(
                dynamic_entities.begin(), dynamic_entities.end(), joint.second)) {
            joint.second = {};
        }
        if (joint.first == EntityId{} && joint.second == EntityId{}) continue;
        if (joint.first == EntityId{}
            || (joint.second != EntityId{} && joint.second < joint.first)) {
            std::swap(joint.first, joint.second);
        }
        historical_joints.push_back(joint);
    }
    std::ranges::sort(historical_joints);
    historical_joints.erase(
        std::unique(historical_joints.begin(), historical_joints.end()),
        historical_joints.end());

    std::vector<EntityId> physical_entities;
    physical_entities.reserve(physical_edges.size() * 2U);
    for (const auto& [first, second] : physical_edges) {
        physical_entities.push_back(first);
        physical_entities.push_back(second);
    }
    std::ranges::sort(physical_entities);
    physical_entities.erase(
        std::unique(physical_entities.begin(), physical_entities.end()),
        physical_entities.end());
    std::vector<std::size_t> component_parent(physical_entities.size());
    for (std::size_t index = 0; index < component_parent.size(); ++index) {
        component_parent[index] = index;
    }
    const auto find_component = [&](std::size_t index) {
        while (component_parent[index] != index) {
            index = component_parent[index];
        }
        return index;
    };
    for (const auto& [first, second] : physical_edges) {
        const std::size_t first_index = static_cast<std::size_t>(
            std::ranges::lower_bound(physical_entities, first)
            - physical_entities.begin());
        const std::size_t second_index = static_cast<std::size_t>(
            std::ranges::lower_bound(physical_entities, second)
            - physical_entities.begin());
        const std::size_t first_root = find_component(first_index);
        const std::size_t second_root = find_component(second_index);
        if (first_root != second_root) {
            component_parent[std::max(first_root, second_root)] =
                std::min(first_root, second_root);
        }
    }

    // Launch roots are immutable anchors. Register them once in canonical event
    // order before resolving any descendant or physical target provenance.
    for (const DomainEvent* event : ordered) {
        if (event->kind != DomainEventKind::BirdLaunched
            || event->id == EventId{}) {
            continue;
        }
        CausalRecord root = causal_record(event->id);
        if (root.event_id == EventId{}) {
            ++observed_launches_;
            root = {.event_id = event->id,
                .root_event_id = event->id,
                .shot_id = event->shot_id != 0U
                    ? event->shot_id : observed_launches_};
            upsert_causal_record(root);
        }
        upsert_entity_root({event->entity_id, root.root_event_id, root.shot_id});
    }

    // Events can be emitted before another same-tick event that establishes a
    // newer physical provenance for their target. Iterate the canonically
    // ordered batch to a fixpoint before any award is evaluated. Explicit
    // foreign causes remain rootless and therefore never erase a valid target
    // root.
    const std::size_t maximum_passes =
        ordered.size() + physical_entities.size()
        + historical_joints.size() * 2U + 2U;
    for (std::size_t pass = 0; pass < maximum_passes; ++pass) {
        const auto records_before = causal_records_;
        const auto roots_before = entity_roots_;
        std::vector<CausalRecord> resolved_records;
        resolved_records.reserve(ordered.size());
        std::vector<EntityRoot> root_proposals;
        root_proposals.reserve(ordered.size() * 2U);
        for (const DomainEvent* event : ordered) {
            if (event->kind == DomainEventKind::BirdLaunched
                || event->id == EventId{}) {
                continue;
            }
            const CausalRecord record = resolve_event(*event);
            resolved_records.push_back(record);
            if (record.root_event_id != EventId{}) {
                const bool joint_transition =
                    event->kind == DomainEventKind::JointOverloaded
                    || event->kind == DomainEventKind::JointBroken;
                if (joint_transition) {
                    for (const JointEntityEndpoints& joint : historical_joints) {
                        if (joint.joint_id != event->joint_id) continue;
                        for (const EntityId endpoint : {joint.first, joint.second}) {
                            if (endpoint == EntityId{}) continue;
                            const EntityRoot current = entity_root(endpoint);
                            if (current.root_event_id == EventId{}) {
                                root_proposals.push_back({endpoint,
                                    record.root_event_id, record.shot_id});
                            }
                        }
                    }
                } else {
                    const EntityRoot source_root = entity_root(event->entity_id);
                    const EntityRoot target_root =
                        entity_root(event->affected_entity_id);
                    const bool resolved_only_from_existing_target =
                        event->cause_event_id == EventId{}
                        && source_root.root_event_id == EventId{}
                        && std::pair{target_root.root_event_id,
                               target_root.shot_id}
                            == std::pair{record.root_event_id, record.shot_id};
                    if (!resolved_only_from_existing_target) {
                        root_proposals.push_back({event->affected_entity_id,
                            record.root_event_id, record.shot_id});
                    }
                }
                const bool direct_dynamic_contact =
                    event->kind == DomainEventKind::DamageApplied
                    && event->cause_event_id == EventId{}
                    && event->entity_id != EntityId{}
                    && event->affected_entity_id != EntityId{}
                    && std::binary_search(dynamic_entities.begin(),
                        dynamic_entities.end(), event->entity_id)
                    && std::binary_search(dynamic_entities.begin(),
                        dynamic_entities.end(), event->affected_entity_id);
                if (direct_dynamic_contact) {
                    root_proposals.push_back({event->entity_id,
                        record.root_event_id, record.shot_id});
                }
            }
        }

        // Resolve the pass against one immutable snapshot. A reciprocal
        // contact contributes each endpoint's established root as well as the
        // foreign proposal, so neither event ordering nor last-writer-wins can
        // merge two shots. A single new proposal may still replace stale
        // target provenance, preserving the established same-tick fixpoint
        // behavior for a later unambiguous hit.
        for (const CausalRecord& record : resolved_records) {
            upsert_causal_record(record);
        }
        std::ranges::sort(root_proposals, [](const EntityRoot& lhs,
                                              const EntityRoot& rhs) {
            return std::tuple{lhs.entity_id, lhs.root_event_id, lhs.shot_id}
                < std::tuple{rhs.entity_id, rhs.root_event_id, rhs.shot_id};
        });
        for (std::size_t first = 0U; first < root_proposals.size();) {
            std::size_t last = first + 1U;
            bool conflict = false;
            while (last < root_proposals.size()
                && root_proposals[last].entity_id
                    == root_proposals[first].entity_id) {
                if (std::pair{root_proposals[last].root_event_id,
                        root_proposals[last].shot_id}
                    != std::pair{root_proposals[first].root_event_id,
                        root_proposals[first].shot_id}) {
                    conflict = true;
                }
                ++last;
            }
            if (!conflict) {
                upsert_entity_root(root_proposals[first]);
            }
            first = last;
        }
        for (std::size_t component = 0; component < physical_entities.size();
             ++component) {
            if (find_component(component) != component) continue;
            EventId unique_root{};
            std::uint64_t unique_shot{};
            bool conflict = false;
            for (std::size_t index = 0; index < physical_entities.size(); ++index) {
                if (find_component(index) != component) continue;
                const EntityRoot root = entity_root(physical_entities[index]);
                if (root.root_event_id == EventId{}) continue;
                if (unique_root == EventId{}) {
                    unique_root = root.root_event_id;
                    unique_shot = root.shot_id;
                } else if (std::pair{unique_root, unique_shot}
                    != std::pair{root.root_event_id, root.shot_id}) {
                    conflict = true;
                    break;
                }
            }
            if (unique_root == EventId{} || conflict) continue;
            for (std::size_t index = 0; index < physical_entities.size(); ++index) {
                if (find_component(index) != component
                    || entity_root(physical_entities[index]).root_event_id
                        != EventId{}) {
                    continue;
                }
                upsert_entity_root(
                    {physical_entities[index], unique_root, unique_shot});
            }
        }
        if (causal_records_ == records_before && entity_roots_ == roots_before) {
            return ordered;
        }
    }
    throw std::runtime_error("score causal provenance did not converge");
}

void ScoreSystem::reset_chain(std::vector<ScoreTransition>& transitions)
{
    if (state_.chain_index == 0U
        && state_.current_chain_root_cause_event_id == EventId{}
        && state_.current_chain_shot_id == 0U) {
        return;
    }
    state_.chain_index = 0U;
    state_.multiplier_percent = 100U;
    state_.last_scoring_tick = {};
    state_.current_chain_root_cause_event_id = {};
    state_.current_chain_shot_id = 0U;
    transitions.push_back({
        .kind = ScoreTransitionKind::ChainChanged,
        .multiplier_percent = 100U,
        .total_score = state_.current_score,
    });
}

void ScoreSystem::award(const ScoringCandidate& candidate,
    const DomainEvent& source, std::vector<ScoreTransition>& transitions)
{
    if (candidate.base_points == 0U
        || std::binary_search(state_.scored_identities.begin(),
            state_.scored_identities.end(), candidate.identity)) {
        return;
    }
    const CausalRecord cause = causal_record(source.id);
    const bool within_window = state_.chain_index != 0U
        && source.tick.value() >= state_.last_scoring_tick.value()
        && source.tick.value() - state_.last_scoring_tick.value()
            <= definition_.chain_window_ticks;
    const bool same_valid_root = cause.root_event_id != EventId{}
        && cause.root_event_id == state_.current_chain_root_cause_event_id;
    if (within_window && same_valid_root) {
        ++state_.chain_index;
    } else {
        state_.chain_index = 1U;
        state_.current_chain_root_cause_event_id = cause.root_event_id;
        state_.current_chain_shot_id = cause.shot_id;
    }
    const std::uint64_t uncapped = 100ULL
        + static_cast<std::uint64_t>(chain_multiplier_step_percent)
            * std::min<std::uint32_t>(state_.chain_index - 1U, 10U);
    state_.multiplier_percent = static_cast<std::uint32_t>(
        std::min<std::uint64_t>(uncapped, maximum_chain_multiplier_percent));
    state_.last_scoring_tick = source.tick;
    transitions.push_back({
        .kind = ScoreTransitionKind::ChainChanged,
        .root_cause_event_id = state_.current_chain_root_cause_event_id,
        .shot_id = state_.current_chain_shot_id,
        .multiplier_percent = state_.multiplier_percent,
        .chain_index = state_.chain_index,
        .total_score = state_.current_score,
    });

    const std::uint64_t awarded =
        (static_cast<std::uint64_t>(candidate.base_points)
            * state_.multiplier_percent + 50ULL) / 100ULL;
    if (awarded > std::numeric_limits<std::uint64_t>::max() - state_.current_score) {
        throw std::overflow_error("score total overflow");
    }
    state_.current_score += awarded;
    state_.scored_identities.insert(std::lower_bound(
        state_.scored_identities.begin(), state_.scored_identities.end(),
        candidate.identity), candidate.identity);
    transitions.push_back({
        .kind = ScoreTransitionKind::Awarded,
        .identity = candidate.identity,
        .source_event_id = candidate.source_event_id,
        .root_cause_event_id = state_.current_chain_root_cause_event_id,
        .shot_id = state_.current_chain_shot_id,
        .base_points = candidate.base_points,
        .multiplier_percent = state_.multiplier_percent,
        .chain_index = state_.chain_index,
        .awarded_points = awarded,
        .total_score = state_.current_score,
    });
}

void ScoreSystem::finish_terminal(const ScoreTickInput& input,
    std::vector<ScoreTransition>& transitions)
{
    if (input.terminal == ScoreTerminalState::None
        || state_.terminal_bonus_awarded) {
        return;
    }
    state_.terminal_bonus_awarded = true;
    if (input.terminal == ScoreTerminalState::Victory) {
        const std::uint32_t remaining = std::min(
            input.remaining_birds, initial_queue_size_);
        const std::uint32_t first_unused = initial_queue_size_ - remaining;
        for (std::uint32_t slot = first_unused; slot < initial_queue_size_; ++slot) {
            const ScoringIdentity identity = ScoringIdentity::unused_bird(slot);
            if (std::binary_search(state_.scored_identities.begin(),
                    state_.scored_identities.end(), identity)) {
                continue;
            }
            const std::uint64_t awarded = definition_.unused_bird_points;
            if (awarded > std::numeric_limits<std::uint64_t>::max()
                    - state_.current_score) {
                throw std::overflow_error("score total overflow");
            }
            state_.current_score += awarded;
            state_.scored_identities.insert(std::lower_bound(
                state_.scored_identities.begin(), state_.scored_identities.end(),
                identity), identity);
            transitions.push_back({
                .kind = ScoreTransitionKind::Awarded,
                .identity = identity,
                .base_points = definition_.unused_bird_points,
                .multiplier_percent = 100U,
                .awarded_points = awarded,
                .total_score = state_.current_score,
            });
        }
        // The first threshold is the authored one-star floor. A victory always
        // has one star, while only thresholds two and three can raise it.
        state_.stars = static_cast<std::uint8_t>(std::max<unsigned>(1U,
            1U + (state_.current_score >= definition_.star_thresholds[1] ? 1U : 0U)
                + (state_.current_score >= definition_.star_thresholds[2] ? 1U : 0U)));
    } else {
        state_.stars = 0U;
    }
    transitions.push_back({
        .kind = ScoreTransitionKind::StarsAwarded,
        .total_score = state_.current_score,
        .stars = state_.stars,
    });
}

std::vector<ScoreTransition> ScoreSystem::consume(const ScoreTickInput& input)
{
    if (!configured_) {
        return {};
    }
    std::vector<ScoreTransition> transitions;
    if (state_.chain_index != 0U
        && input.tick.value() > state_.last_scoring_tick.value()
        && input.tick.value() - state_.last_scoring_tick.value()
            > definition_.chain_window_ticks) {
        reset_chain(transitions);
    }
    const auto ordered_events = resolve_tick_provenance(
        input.events, input.dynamic_entities, input.dynamic_physical_edges,
        input.joint_entity_endpoints);
    std::vector<ScoringCandidate> candidates{
        input.candidates.begin(), input.candidates.end()};
    std::ranges::sort(candidates, [](const auto& lhs, const auto& rhs) {
        return std::tie(lhs.source_event_id, lhs.identity)
            < std::tie(rhs.source_event_id, rhs.identity);
    });
    std::size_t candidate_index{};
    for (const DomainEvent* event : ordered_events) {
        if (event->kind == DomainEventKind::BirdLaunched) {
            reset_chain(transitions);
        }
        while (candidate_index < candidates.size()
            && candidates[candidate_index].source_event_id < event->id) {
            ++candidate_index;
        }
        while (candidate_index < candidates.size()
            && candidates[candidate_index].source_event_id == event->id) {
            award(candidates[candidate_index], *event, transitions);
            ++candidate_index;
        }
    }
    finish_terminal(input, transitions);
    return transitions;
}

}
