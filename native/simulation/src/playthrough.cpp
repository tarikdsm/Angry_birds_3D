#include "ninho/simulation/playthrough.hpp"

#include "session_internal.hpp"

#include <limits>
#include <stdexcept>
#include <string_view>
#include <utility>

namespace ninho::simulation {
namespace {

class PlaythroughWriter {
public:
    void u8(std::uint8_t value) { bytes_.push_back(value); }

    void u32(std::uint32_t value)
    {
        for (unsigned shift = 0U; shift < 32U; shift += 8U) {
            bytes_.push_back(static_cast<std::uint8_t>(value >> shift));
        }
    }

    void u64(std::uint64_t value)
    {
        for (unsigned shift = 0U; shift < 64U; shift += 8U) {
            bytes_.push_back(static_cast<std::uint8_t>(value >> shift));
        }
    }

    void i64(std::int64_t value) { u64(static_cast<std::uint64_t>(value)); }

    void quantized(double value) { i64(detail::canonical_quantize(value)); }

    void vec3(const ninho::physics::Vec3& value)
    {
        quantized(value.x);
        quantized(value.y);
        quantized(value.z);
    }

    void text(std::string_view value)
    {
        sized_length(value.size());
        bytes_.insert(bytes_.end(), value.begin(), value.end());
    }

    void blob(std::span<const std::uint8_t> value)
    {
        sized_length(value.size());
        bytes_.insert(bytes_.end(), value.begin(), value.end());
    }

    [[nodiscard]] std::vector<std::uint8_t> finish() && { return std::move(bytes_); }

private:
    void sized_length(std::size_t size)
    {
        if (size > std::numeric_limits<std::uint32_t>::max()) {
            throw std::length_error("canonical playthrough field exceeds uint32 length");
        }
        u32(static_cast<std::uint32_t>(size));
    }

    std::vector<std::uint8_t> bytes_;
};

std::uint8_t scoring_identity_tag(ScoringIdentityKind value)
{
    switch (value) {
    case ScoringIdentityKind::None: return 0U;
    case ScoringIdentityKind::EnemyEntity: return 1U;
    case ScoringIdentityKind::MaterialPiece: return 2U;
    case ScoringIdentityKind::UnusedBirdSlot: return 3U;
    }
    throw std::invalid_argument("unknown canonical scoring identity kind");
}

void write_event(PlaythroughWriter& writer, const DomainEvent& event)
{
    writer.u64(event.id.value());
    writer.u64(event.tick.value());
    writer.u8(detail::canonical_tag_of(event.kind));
    writer.u32(event.entity_id.value());
    writer.u32(event.bird_archetype_id.value());
    writer.u8(detail::canonical_tag_of(event.rejection_reason));
    writer.u32(event.ability_id.value());
    writer.u32(event.affected_entity_id.value());
    writer.u32(event.affected_part_id.value());
    writer.quantized(event.weight);
    writer.vec3(event.force_n);
    writer.vec3(event.impulse_n_s);
    writer.u32(event.part_id.value());
    writer.vec3(event.position_m);
    writer.vec3(event.normal);
    writer.quantized(event.energy_j);
    writer.quantized(event.damage);
    writer.u8(detail::canonical_tag_of(event.damage_classification));
    writer.u8(detail::canonical_tag_of(event.neutralization_cause));
    writer.u64(event.cause_event_id.value());
    writer.u32(event.joint_id.value());
    writer.u32(event.material_id.value());
    writer.quantized(event.joint_load_ratio);
    writer.quantized(event.fracture_ratio);
    writer.vec3(event.delta_velocity_m_s);
    writer.u32(event.environmental_trigger_id);
    writer.u8(scoring_identity_tag(event.scoring_identity_kind));
    writer.u32(event.queue_slot_id);
    writer.u32(event.base_points);
    writer.u32(event.multiplier_percent);
    writer.u32(event.chain_index);
    writer.u64(event.awarded_points);
    writer.u64(event.total_score);
    writer.u64(event.root_cause_event_id.value());
    writer.u64(event.shot_id);
    writer.u8(event.stars);
}

}

std::vector<std::uint8_t> ordered_events_v3(std::span<const DomainEvent> events)
{
    if (events.size() > std::numeric_limits<std::uint32_t>::max()) {
        throw std::length_error("ordered events exceed uint32 count");
    }
    PlaythroughWriter writer;
    writer.text("ordered_events_v3");
    writer.u32(3U);
    writer.u32(static_cast<std::uint32_t>(events.size()));
    for (const DomainEvent& event : events) {
        write_event(writer, event);
    }
    return std::move(writer).finish();
}

std::vector<std::uint8_t> canonical_playthrough_v5(
    const PlaythroughRouteDefinition& route,
    std::span<const std::uint8_t> ordered_events,
    std::span<const std::uint8_t> terminal_canonical_state)
{
    if (route.shots.size() > std::numeric_limits<std::uint32_t>::max()) {
        throw std::length_error("playthrough route exceeds uint32 shot count");
    }
    PlaythroughWriter writer;
    writer.text("canonical_playthrough_v5");
    writer.u32(5U);
    writer.text(route.world_id);
    writer.text(route.level_id);
    writer.u32(static_cast<std::uint32_t>(route.shots.size()));
    for (const GestureShotInput& shot : route.shots) {
        writer.vec3(shot.camera_right);
        writer.quantized(shot.pull_horizontal_m);
        writer.quantized(shot.pull_vertical_m);
        writer.u8(shot.ability_tick_after_launch.has_value() ? 1U : 0U);
        if (shot.ability_tick_after_launch) {
            writer.u64(*shot.ability_tick_after_launch);
        }
    }
    writer.blob(ordered_events);
    writer.blob(terminal_canonical_state);
    return std::move(writer).finish();
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

}
