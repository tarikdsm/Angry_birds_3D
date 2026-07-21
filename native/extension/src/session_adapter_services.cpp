#include <ninho/extension/session_adapter_services.hpp>

#include <algorithm>
#include <cmath>
#include <utility>

namespace ninho::extension::detail {

std::string_view domain_event_kind_name(
    simulation::DomainEventKind kind) noexcept
{
    using enum simulation::DomainEventKind;
    switch (kind) {
    case BirdLaunched: return "bird_launched";
    case AbilityActivationRequested: return "ability_activation_requested";
    case CommandRejected: return "command_rejected";
    case AbilityStarted: return "ability_started";
    case AbilityAffectedBody: return "ability_affected_body";
    case AbilityPulse: return "ability_pulse";
    case AbilityEnded: return "ability_ended";
    case DamageApplied: return "damage_applied";
    case EntityNeutralized: return "entity_neutralized";
    case JointOverloaded: return "joint_overloaded";
    case PieceFractureTriggered: return "piece_fracture_triggered";
    case JointBroken: return "joint_broken";
    case PieceFractured: return "piece_fractured";
    case MassChanged: return "mass_changed";
    case SpeedChanged: return "speed_changed";
    case ProjectileSplit: return "projectile_split";
    case ProjectileSpawned: return "projectile_spawned";
    case ExplosionFuseArmed: return "explosion_fuse_armed";
    case PressureBurst: return "pressure_burst";
    case EnvironmentalTriggerArmed: return "environmental_trigger_armed";
    case EnvironmentalTriggerDetonated: return "environmental_trigger_detonated";
    }
    return "unknown";
}

SessionTickSchedule SessionFixedStepAccumulator::schedule(double delta) noexcept
{
    if (!std::isfinite(delta) || delta < 0.0) {
        reset();
        return {};
    }

    const double total = pending_seconds_ + delta;
    const double tolerance = time_step * 1.0e-9;
    const double maximum_executed_time = max_ticks_per_frame * time_step;
    if (total + tolerance >= maximum_executed_time) {
        double remainder = std::fmod(total, time_step);
        if (remainder < tolerance || time_step - remainder < tolerance) {
            remainder = 0.0;
        }
        pending_seconds_ = remainder;
        return {
            .ok = true,
            .tick_count = max_ticks_per_frame,
            .discarded_seconds = std::max(
                0.0, total - remainder - maximum_executed_time),
        };
    }
    const auto available = static_cast<std::int64_t>(
        std::floor((total + tolerance) / time_step));
    pending_seconds_ = std::max(
        0.0, total - static_cast<double>(available) * time_step);
    return {
        .ok = true,
        .tick_count = static_cast<int>(available),
        .discarded_seconds = 0.0,
    };
}

void SessionFixedStepAccumulator::reset() noexcept
{
    pending_seconds_ = 0.0;
}

namespace {

template <std::size_t Size>
void append_fixed(
    std::array<char, Size>& destination,
    std::size_t& used,
    std::string_view source) noexcept
{
    const std::size_t available = destination.size() - 1U - used;
    const std::size_t count = std::min(available, source.size());
    std::copy_n(source.data(), count, destination.data() + used);
    used += count;
    destination[used] = '\0';
}

}

FaultInfo::FaultInfo(std::string_view code, std::string_view message) noexcept
{
    append_fixed(code_storage_, code_size_, code);
    append_fixed(message_storage_, message_size_, message);
}

FaultInfo::FaultInfo(
    std::string_view code,
    std::string_view context,
    std::string_view message) noexcept
{
    append_fixed(code_storage_, code_size_, code);
    if (!context.empty()) {
        append_fixed(message_storage_, message_size_, context);
        append_fixed(message_storage_, message_size_, ": ");
    }
    append_fixed(message_storage_, message_size_, message);
}

std::string_view FaultInfo::code() const noexcept
{
    return {code_storage_.data(), code_size_};
}

std::string_view FaultInfo::message() const noexcept
{
    return {message_storage_.data(), message_size_};
}

void SessionFrameBatch::capture_events(
    std::span<const simulation::DomainEvent> events)
{
    pending_.events.insert(pending_.events.end(), events.begin(), events.end());
    ++pending_.ticks_executed;
}

void SessionFrameBatch::capture_latest(
    std::span<const simulation::EntitySnapshot> snapshots,
    const simulation::SessionState& state,
    std::uint32_t birds_remaining,
    bool objectives_complete,
    const physics::WorldMetrics& metrics,
    std::span<const simulation::ObjectiveTargetStatus> objective_targets,
    simulation::AbilityReadiness ability_readiness)
{
    pending_.snapshots.assign(snapshots.begin(), snapshots.end());
    pending_.state = state;
    pending_.birds_remaining = birds_remaining;
    pending_.objectives_complete = objectives_complete;
    pending_.objective_targets.assign(objective_targets.begin(), objective_targets.end());
    pending_.ability_readiness = ability_readiness;
    pending_.metrics = metrics;
    if (state.phase != simulation::SessionPhase::Aim
        && state.phase != simulation::SessionPhase::Grabbed) {
        pending_.preview.reset();
    }
}

void SessionFrameBatch::set_preview(simulation::TrajectoryPreview preview)
{
    pending_.preview = std::move(preview);
}

void SessionFrameBatch::clear_preview() noexcept
{
    pending_.preview.reset();
}

void SessionFrameBatch::set_aim_envelope(AimEnvelope envelope) noexcept
{
    pending_.aim_envelope = envelope;
}

void SessionFrameBatch::set_gameplay_fields(GameplayFrameFields fields)
{
    pending_.bird_queue = std::move(fields.bird_queue);
    pending_.current_bird = fields.current_bird;
    pending_.locked_plane = fields.locked_plane;
    pending_.shot = std::move(fields.shot);
    pending_.projectiles = std::move(fields.projectiles);
    pending_.score = fields.score;
    pending_.stars = fields.stars;
    pending_.gravity_kind = std::move(fields.gravity_kind);
    pending_.local_gravity_m_s2 = fields.local_gravity_m_s2;
}

void SessionFrameBatch::add_discarded_time(double seconds) noexcept
{
    if (std::isfinite(seconds) && seconds > 0.0) {
        pending_.discarded_time_seconds += seconds;
    }
}

SessionFrameData SessionFrameBatch::consume()
{
    SessionFrameData result = pending_;
    pending_.events.clear();
    pending_.ticks_executed = 0;
    pending_.discarded_time_seconds = 0.0;
    return result;
}

const SessionFrameData& SessionFrameBatch::peek() const noexcept
{
    return pending_;
}

void SessionFrameBatch::acknowledge() noexcept
{
    pending_.events.clear();
    pending_.ticks_executed = 0;
    pending_.discarded_time_seconds = 0.0;
}

void SessionFrameBatch::reset()
{
    pending_ = {};
    fault_.reset();
}

void SessionFrameBatch::latch_fault(FaultInfo fault) noexcept
{
    if (fault_) {
        return;
    }
    fault_.emplace(std::move(fault));
}

void SessionFrameBatch::clear_fault() noexcept
{
    fault_.reset();
}

const std::optional<FaultInfo>& SessionFrameBatch::fault() const noexcept
{
    return fault_;
}

void capture_session_events(
    SessionFrameBatch& batch,
    const simulation::SimulationSession& session)
{
    batch.capture_events(session.events());
}

}
