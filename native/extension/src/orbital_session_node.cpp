#include <ninho/extension/orbital_session_node.hpp>

#include <ninho/extension/adapter_helpers.hpp>

#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/core/property_info.hpp>
#include <godot_cpp/variant/string_name.hpp>
#include <godot_cpp/variant/typed_array.hpp>
#include <godot_cpp/variant/variant.hpp>

#include <algorithm>
#include <cmath>
#include <exception>
#include <limits>
#include <string>
#include <utility>
#include <type_traits>

namespace ninho::extension::detail {

namespace {

[[nodiscard]] AimEnvelope aim_envelope_from(
    const simulation::LevelManifest& level) noexcept
{
    return {
        .shell_radius_m = level.planet.radius_m + level.launch_ring.shell_offset_m,
        .theta_min_deg = level.launch_ring.theta_min_deg,
        .theta_max_deg = level.launch_ring.theta_max_deg,
        .speed_min_m_s = level.launch_ring.phase_speed_min_m_s,
        .speed_max_m_s = level.launch_ring.phase_speed_max_m_s,
        .default_speed_m_s = level.launch_ring.default_speed_m_s,
    };
}

}

void OrbitalSessionAdapter::latch_content_error(
    const simulation::ContentError& error) noexcept
{
    if (batch_.fault()) {
        return;
    }
    batch_.latch_fault(FaultInfo{
        simulation::content_error_code_name(error.code), error.pointer, error.message});
    ++fault_generation_;
}

void OrbitalSessionAdapter::latch_exception(
    std::string_view operation, const char* message) noexcept
{
    if (batch_.fault()) {
        return;
    }
    batch_.latch_fault(FaultInfo{
        "exception", operation, message != nullptr ? message : "unknown exception"});
    ++fault_generation_;
}

void OrbitalSessionAdapter::fail(
    std::string_view code, std::string_view message) noexcept
{
    if (batch_.fault()) {
        return;
    }
    batch_.latch_fault(FaultInfo{code, message});
    ++fault_generation_;
}

static_assert(std::is_nothrow_move_assignable_v<SessionFrameBatch>);
static_assert(std::is_nothrow_move_assignable_v<simulation::ContentBundle>);

bool OrbitalSessionAdapter::configure(
    std::string_view materials_json,
    std::string_view archetypes_json,
    std::string_view level_json) noexcept
{
    try {
        auto materials = simulation::parse_material_catalog(materials_json);
        if (!materials) {
            batch_.clear_fault();
            latch_content_error(materials.error);
            return false;
        }
        auto archetypes = simulation::parse_archetype_catalog(archetypes_json);
        if (!archetypes) {
            batch_.clear_fault();
            latch_content_error(archetypes.error);
            return false;
        }
        auto level = simulation::parse_level_manifest(level_json);
        if (!level) {
            batch_.clear_fault();
            latch_content_error(level.error);
            return false;
        }
        simulation::ContentBundle candidate_content{
            std::move(materials.value),
            std::move(archetypes.value),
            std::move(level.value),
        };
        auto candidate = simulation::SimulationSession::create(
            candidate_content.materials,
            candidate_content.archetypes,
            candidate_content.level);
        if (!candidate) {
            batch_.clear_fault();
            latch_content_error(candidate.error);
            return false;
        }

        SessionFrameBatch candidate_batch;
        const std::vector<simulation::ObjectiveTargetStatus> objective_targets =
            candidate.value->objective_target_statuses();
        candidate_batch.capture_latest(
            candidate.value->snapshots(),
            candidate.value->state(),
            candidate.value->birds_remaining(),
            candidate.value->objectives_complete(),
            candidate.value->physics_metrics(),
            objective_targets,
            candidate.value->ability_readiness());
        candidate_batch.set_aim_envelope(aim_envelope_from(candidate_content.level));

        session_ = std::move(candidate.value);
        content_ = std::move(candidate_content);
        batch_ = std::move(candidate_batch);
        accumulator_.reset();
        return true;
    } catch (const std::exception& error) {
        batch_.clear_fault();
        latch_exception("configure_session", error.what());
    } catch (...) {
        batch_.clear_fault();
        latch_exception("configure_session", "unknown exception");
    }
    return false;
}

bool OrbitalSessionAdapter::accept_status(
    const simulation::SessionStatus& status) noexcept
{
    if (status.ok()) {
        return true;
    }
    latch_content_error(status.error);
    return false;
}

bool OrbitalSessionAdapter::enqueue(simulation::PlayerCommand command) noexcept
{
    if (batch_.fault()) {
        return false;
    }
    if (!session_) {
        fail("session_not_configured", "configure_session must succeed first");
        return false;
    }
    try {
        return accept_status(session_->enqueue(std::move(command)));
    } catch (const std::exception& error) {
        latch_exception("queue_command", error.what());
    } catch (...) {
        latch_exception("queue_command", "unknown exception");
    }
    return false;
}

bool OrbitalSessionAdapter::queue_begin_aim() noexcept
{
    return enqueue(simulation::BeginAimCommand{});
}

bool OrbitalSessionAdapter::queue_aim(
    physics::Vec3 origin,
    physics::Vec3 tangent_direction,
    double speed) noexcept
{
    if (batch_.fault()) {
        return false;
    }
    if (!session_) {
        fail("session_not_configured", "configure_session must succeed first");
        return false;
    }
    if (!physics::is_finite(origin) || !physics::is_finite(tangent_direction)
        || !std::isfinite(speed)) {
        return false;
    }
    try {
        simulation::AimState aim{origin, tangent_direction, speed};
        simulation::TrajectoryPreview preview = session_->preview(aim);
        if (!accept_status(preview.status)) {
            return false;
        }
        if (!accept_status(session_->enqueue(simulation::SetAimCommand{aim}))) {
            return false;
        }
        batch_.set_preview(std::move(preview));
        return true;
    } catch (const std::exception& error) {
        latch_exception("queue_aim", error.what());
    } catch (...) {
        latch_exception("queue_aim", "unknown exception");
    }
    return false;
}

bool OrbitalSessionAdapter::queue_launch() noexcept
{
    return enqueue(simulation::LaunchCommand{});
}

bool OrbitalSessionAdapter::queue_activate_ability() noexcept
{
    return enqueue(simulation::ActivateAbilityCommand{});
}

bool OrbitalSessionAdapter::queue_cancel_aim() noexcept
{
    return enqueue(simulation::CancelAimCommand{});
}

bool OrbitalSessionAdapter::restart() noexcept
{
    if (!session_) {
        fail("session_not_configured", "configure_session must succeed before restart");
        return false;
    }
    try {
        auto candidate = simulation::SimulationSession::create(
            content_.materials, content_.archetypes, content_.level);
        if (!candidate) {
            batch_.clear_fault();
            latch_content_error(candidate.error);
            return false;
        }
        SessionFrameBatch candidate_batch;
        const std::vector<simulation::ObjectiveTargetStatus> objective_targets =
            candidate.value->objective_target_statuses();
        candidate_batch.capture_latest(
            candidate.value->snapshots(),
            candidate.value->state(),
            candidate.value->birds_remaining(),
            candidate.value->objectives_complete(),
            candidate.value->physics_metrics(),
            objective_targets,
            candidate.value->ability_readiness());
        candidate_batch.set_aim_envelope(aim_envelope_from(content_.level));

        session_ = std::move(candidate.value);
        batch_ = std::move(candidate_batch);
        accumulator_.reset();
        return true;
    } catch (const std::exception& error) {
        batch_.clear_fault();
        latch_exception("restart_level", error.what());
    } catch (...) {
        batch_.clear_fault();
        latch_exception("restart_level", "unknown exception");
    }
    return false;
}

void OrbitalSessionAdapter::capture_latest()
{
    const std::vector<simulation::ObjectiveTargetStatus> objective_targets =
        session_->objective_target_statuses();
    batch_.capture_latest(
        session_->snapshots(),
        session_->state(),
        session_->birds_remaining(),
        session_->objectives_complete(),
        session_->physics_metrics(),
        objective_targets,
        session_->ability_readiness());
}

bool OrbitalSessionAdapter::advance(double delta) noexcept
{
    if (batch_.fault()) {
        return false;
    }
    if (!session_) {
        fail("session_not_configured", "configure_session must succeed before processing");
        return false;
    }
    const SessionTickSchedule schedule = accumulator_.schedule(delta);
    if (!schedule.ok) {
        fail("invalid_delta", "physics delta must be finite and non-negative");
        return false;
    }
    batch_.add_discarded_time(schedule.discarded_seconds);

    try {
        for (int index = 0; index < schedule.tick_count; ++index) {
            const simulation::SessionStatus status = session_->tick();
            capture_session_events(batch_, *session_);
            if (!status.ok()) {
                capture_latest();
                return accept_status(status);
            }
        }
        if (schedule.tick_count > 0) {
            capture_latest();
        }
        return true;
    } catch (const std::exception& error) {
        latch_exception("physics_process", error.what());
    } catch (...) {
        latch_exception("physics_process", "unknown exception");
    }
    return false;
}

SessionFrameData OrbitalSessionAdapter::consume_frame() noexcept
{
    try {
        return batch_.consume();
    } catch (const std::exception& error) {
        latch_exception("consume_frame", error.what());
    } catch (...) {
        latch_exception("consume_frame", "unknown exception");
    }
    return {};
}

const SessionFrameData& OrbitalSessionAdapter::peek_frame() const noexcept
{
    return batch_.peek();
}

void OrbitalSessionAdapter::acknowledge_frame() noexcept
{
    batch_.acknowledge();
}

bool OrbitalSessionAdapter::configured() const noexcept
{
    return session_ != nullptr;
}

const std::optional<FaultInfo>& OrbitalSessionAdapter::fault() const noexcept
{
    return batch_.fault();
}

std::uint64_t OrbitalSessionAdapter::fault_generation() const noexcept
{
    return fault_generation_;
}

}

namespace ninho::extension {
namespace {

[[nodiscard]] godot::String godot_string(std::string_view text)
{
    return godot::String::utf8(text.data(), static_cast<std::int64_t>(text.size()));
}

[[nodiscard]] const char* phase_name(simulation::SessionPhase phase) noexcept
{
    switch (phase) {
    case simulation::SessionPhase::Inspection: return "inspection";
    case simulation::SessionPhase::Aim: return "aim";
    case simulation::SessionPhase::FlightAbility: return "flight_ability";
    case simulation::SessionPhase::Resolution: return "resolution";
    case simulation::SessionPhase::Evaluation: return "evaluation";
    case simulation::SessionPhase::Result: return "result";
    case simulation::SessionPhase::Faulted: return "faulted";
    }
    return "unknown";
}

[[nodiscard]] const char* outcome_name(simulation::Outcome outcome) noexcept
{
    switch (outcome) {
    case simulation::Outcome::None: return "none";
    case simulation::Outcome::Victory: return "victory";
    case simulation::Outcome::Defeat: return "defeat";
    }
    return "unknown";
}

[[nodiscard]] const char* ability_readiness_name(
    simulation::AbilityReadiness readiness) noexcept
{
    switch (readiness) {
    case simulation::AbilityReadiness::Unavailable: return "unavailable";
    case simulation::AbilityReadiness::Arming: return "arming";
    case simulation::AbilityReadiness::Armed: return "armed";
    case simulation::AbilityReadiness::Active: return "active";
    case simulation::AbilityReadiness::Spent: return "spent";
    }
    return "unavailable";
}

[[nodiscard]] const char* rejection_reason_name(
    simulation::CommandRejectionReason reason) noexcept
{
    switch (reason) {
    case simulation::CommandRejectionReason::None: return "none";
    case simulation::CommandRejectionReason::InvalidPhase: return "invalid_phase";
    case simulation::CommandRejectionReason::InvalidAim: return "invalid_aim";
    case simulation::CommandRejectionReason::NotArmed: return "not_armed";
    case simulation::CommandRejectionReason::NoBirdAvailable: return "no_bird_available";
    case simulation::CommandRejectionReason::AbilityUnavailable: return "ability_unavailable";
    }
    return "unknown";
}

[[nodiscard]] const char* damage_classification_name(
    simulation::DamageClassification classification) noexcept
{
    switch (classification) {
    case simulation::DamageClassification::None: return "none";
    case simulation::DamageClassification::Protected: return "protected";
    case simulation::DamageClassification::Vulnerable: return "vulnerable";
    }
    return "unknown";
}

[[nodiscard]] const char* event_kind_name(simulation::DomainEventKind kind) noexcept
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
    }
    return "unknown";
}

[[nodiscard]] const char* body_type_name(simulation::BodyType type) noexcept
{
    return type == simulation::BodyType::Static ? "static" : "dynamic";
}

[[nodiscard]] godot::Dictionary aim_dictionary(const simulation::AimState& aim)
{
    godot::Dictionary result;
    result["origin"] = detail::to_godot(aim.origin_m);
    result["tangent_direction"] = detail::to_godot(aim.tangent_direction);
    result["speed"] = aim.speed_m_s;
    return result;
}

[[nodiscard]] godot::Dictionary snapshot_dictionary(
    const simulation::EntitySnapshot& snapshot)
{
    godot::Dictionary shape;
    shape["type"] = snapshot.shape.type == simulation::ShapeType::Box ? "box" : "sphere";
    shape["half_extents"] = godot::Vector3{
        static_cast<godot::real_t>(snapshot.shape.half_extents_m[0]),
        static_cast<godot::real_t>(snapshot.shape.half_extents_m[1]),
        static_cast<godot::real_t>(snapshot.shape.half_extents_m[2])};
    shape["radius"] = snapshot.shape.radius_m;

    godot::Dictionary result;
    result["entity_id"] = static_cast<std::int64_t>(snapshot.entity_id.value());
    result["part_id"] = static_cast<std::int64_t>(snapshot.part_id.value());
    result["body_type"] = body_type_name(snapshot.body_type);
    if (snapshot.material_id) {
        result["material_id"] = static_cast<std::int64_t>(snapshot.material_id->value());
    }
    if (snapshot.surface_id) {
        result["surface_id"] = static_cast<std::int64_t>(snapshot.surface_id->value());
    }
    if (snapshot.enemy_archetype_id) {
        result["enemy_archetype_id"] =
            static_cast<std::int64_t>(snapshot.enemy_archetype_id->value());
    }
    result["shape"] = shape;
    result["visual_id"] = godot_string(snapshot.visual_id);
    result["transform"] = detail::to_godot(snapshot.transform);
    result["linear_velocity"] = detail::to_godot(snapshot.linear_velocity_m_s);
    result["angular_velocity"] = detail::to_godot(snapshot.angular_velocity_rad_s);
    result["mass_kg"] = snapshot.mass_kg;
    result["awake"] = snapshot.awake;
    result["ejected"] = snapshot.ejected;
    result["is_projectile"] = snapshot.is_projectile;
    return result;
}

[[nodiscard]] godot::Dictionary event_dictionary(const simulation::DomainEvent& event)
{
    godot::Dictionary result;
    result["id"] = static_cast<std::int64_t>(event.id.value());
    result["tick"] = static_cast<std::int64_t>(event.tick.value());
    result["kind"] = event_kind_name(event.kind);
    result["entity_id"] = static_cast<std::int64_t>(event.entity_id.value());
    result["bird_archetype_id"] = static_cast<std::int64_t>(event.bird_archetype_id.value());
    result["rejection_reason"] = static_cast<std::int64_t>(event.rejection_reason);
    result["rejection_reason_name"] = rejection_reason_name(event.rejection_reason);
    result["ability_id"] = static_cast<std::int64_t>(event.ability_id.value());
    result["affected_entity_id"] = static_cast<std::int64_t>(event.affected_entity_id.value());
    result["affected_part_id"] = static_cast<std::int64_t>(event.affected_part_id.value());
    result["weight"] = event.weight;
    result["force"] = detail::to_godot(event.force_n);
    result["impulse"] = detail::to_godot(event.impulse_n_s);
    result["part_id"] = static_cast<std::int64_t>(event.part_id.value());
    result["position"] = detail::to_godot(event.position_m);
    result["normal"] = detail::to_godot(event.normal);
    result["energy_j"] = event.energy_j;
    result["damage"] = event.damage;
    result["damage_classification"] =
        damage_classification_name(event.damage_classification);
    result["neutralization_cause"] = static_cast<std::int64_t>(event.neutralization_cause);
    result["cause_event_id"] = static_cast<std::int64_t>(event.cause_event_id.value());
    result["joint_id"] = static_cast<std::int64_t>(event.joint_id.value());
    result["material_id"] = static_cast<std::int64_t>(event.material_id.value());
    result["joint_load_ratio"] = event.joint_load_ratio;
    result["fracture_ratio"] = event.fracture_ratio;
    if (event.kind == simulation::DomainEventKind::SpeedChanged) {
        result["delta_velocity"] = detail::to_godot(event.delta_velocity_m_s);
    }
    return result;
}

[[nodiscard]] godot::Variant preview_variant(
    const std::optional<simulation::TrajectoryPreview>& preview)
{
    if (!preview) {
        return {};
    }
    godot::Dictionary result;
    result["aim"] = aim_dictionary(preview->quantized_aim);
    godot::TypedArray<godot::Vector3> samples;
    for (const physics::Vec3 sample : preview->samples) {
        samples.push_back(detail::to_godot(sample));
    }
    result["samples"] = samples;
    if (preview->first_hit) {
        godot::Dictionary hit;
        hit["entity_id"] = static_cast<std::int64_t>(preview->first_hit->entity_id.value());
        hit["part_id"] = static_cast<std::int64_t>(preview->first_hit->part_id.value());
        hit["point"] = detail::to_godot(preview->first_hit->point_m);
        hit["normal"] = detail::to_godot(preview->first_hit->normal);
        result["first_hit"] = hit;
    } else {
        result["first_hit"] = godot::Variant{};
    }
    result["canonical_hash"] = static_cast<std::int64_t>(preview->canonical_hash);
    return result;
}

[[nodiscard]] godot::Dictionary frame_dictionary(const detail::SessionFrameData& frame)
{
    godot::TypedArray<godot::Dictionary> snapshots;
    for (const auto& snapshot : frame.snapshots) {
        snapshots.push_back(snapshot_dictionary(snapshot));
    }
    godot::TypedArray<godot::Dictionary> events;
    for (const auto& event : frame.events) {
        events.push_back(event_dictionary(event));
    }
    godot::TypedArray<godot::Dictionary> objective_targets;
    for (const auto& target : frame.objective_targets) {
        godot::Dictionary item;
        item["entity_id"] = static_cast<std::int64_t>(target.entity_id.value());
        item["current_integrity"] = target.current_integrity;
        item["maximum_integrity"] = target.maximum_integrity;
        item["neutralized"] = target.neutralized;
        objective_targets.push_back(item);
    }
    godot::Dictionary metrics;
    metrics["body_count"] = frame.metrics.body_count;
    metrics["shape_count"] = frame.metrics.shape_count;
    metrics["joint_count"] = frame.metrics.joint_count;
    metrics["contact_count"] = frame.metrics.contact_count;
    metrics["awake_count"] = frame.metrics.awake_count;
    metrics["step_ms"] = frame.metrics.step_ms;

    godot::Dictionary aim_envelope;
    aim_envelope["shell_radius_m"] = frame.aim_envelope.shell_radius_m;
    aim_envelope["theta_min_deg"] = frame.aim_envelope.theta_min_deg;
    aim_envelope["theta_max_deg"] = frame.aim_envelope.theta_max_deg;
    aim_envelope["speed_min_m_s"] = frame.aim_envelope.speed_min_m_s;
    aim_envelope["speed_max_m_s"] = frame.aim_envelope.speed_max_m_s;
    aim_envelope["default_speed_m_s"] = frame.aim_envelope.default_speed_m_s;

    godot::Dictionary result;
    result["tick"] = static_cast<std::int64_t>(frame.state.tick.value());
    result["ticks_executed"] = frame.ticks_executed;
    result["phase"] = phase_name(frame.state.phase);
    result["outcome"] = outcome_name(frame.state.outcome);
    result["birds_remaining"] = static_cast<std::int64_t>(frame.birds_remaining);
    result["snapshots"] = snapshots;
    result["events"] = events;
    result["objectives_complete"] = frame.objectives_complete;
    result["objective_targets"] = objective_targets;
    result["ability_readiness"] = ability_readiness_name(frame.ability_readiness);
    result["ability_armed"] =
        frame.ability_readiness == simulation::AbilityReadiness::Armed;
    result["trajectory_preview"] = preview_variant(frame.preview);
    result["aim_envelope"] = aim_envelope;
    result["metrics"] = metrics;
    result["discarded_time_seconds"] = frame.discarded_time_seconds;
    return result;
}

}

OrbitalSessionNode::OrbitalSessionNode()
{
    set_process_priority(-100);
}

void OrbitalSessionNode::_bind_methods()
{
    godot::ClassDB::bind_method(
        godot::D_METHOD("configure_session", "materials_json", "archetypes_json", "level_json"),
        &OrbitalSessionNode::configure_session);
    godot::ClassDB::bind_method(
        godot::D_METHOD("queue_begin_aim"), &OrbitalSessionNode::queue_begin_aim);
    godot::ClassDB::bind_method(
        godot::D_METHOD("queue_aim", "origin", "tangent_direction", "speed"),
        &OrbitalSessionNode::queue_aim);
    godot::ClassDB::bind_method(
        godot::D_METHOD("queue_launch"), &OrbitalSessionNode::queue_launch);
    godot::ClassDB::bind_method(
        godot::D_METHOD("queue_activate_ability"),
        &OrbitalSessionNode::queue_activate_ability);
    godot::ClassDB::bind_method(
        godot::D_METHOD("queue_cancel_aim"), &OrbitalSessionNode::queue_cancel_aim);
    godot::ClassDB::bind_method(
        godot::D_METHOD("restart_level"), &OrbitalSessionNode::restart_level);
    godot::ClassDB::bind_method(
        godot::D_METHOD("consume_frame"), &OrbitalSessionNode::consume_frame);

    ADD_SIGNAL(godot::MethodInfo(
        "gameplay_fault",
        godot::PropertyInfo(godot::Variant::STRING, "code"),
        godot::PropertyInfo(godot::Variant::STRING, "message")));
}

void OrbitalSessionNode::emit_pending_fault() noexcept
{
    if (adapter_.fault_generation() == reported_fault_generation_) {
        return;
    }
    try {
        const auto& fault = adapter_.fault();
        if (fault) {
            emit_signal(
                "gameplay_fault", godot_string(fault->code()), godot_string(fault->message()));
            reported_fault_generation_ = adapter_.fault_generation();
        }
    } catch (...) {
        // Diagnostics must not become exceptions crossing the GDExtension ABI.
    }
}

void OrbitalSessionNode::emit_exception_fault(
    std::string_view operation, const char* message) noexcept
{
    (void)operation;
    adapter_.fail("exception", message != nullptr ? message : "unknown Godot adapter exception");
    emit_pending_fault();
}

bool OrbitalSessionNode::configure_session(
    godot::String materials_json,
    godot::String archetypes_json,
    godot::String level_json) noexcept
{
    try {
        const godot::CharString materials = materials_json.utf8();
        const godot::CharString archetypes = archetypes_json.utf8();
        const godot::CharString level = level_json.utf8();
        const bool result = adapter_.configure(
            std::string_view{materials.get_data(), static_cast<std::size_t>(materials.length())},
            std::string_view{archetypes.get_data(), static_cast<std::size_t>(archetypes.length())},
            std::string_view{level.get_data(), static_cast<std::size_t>(level.length())});
        emit_pending_fault();
        return result;
    } catch (const std::exception& error) {
        emit_exception_fault("configure_session", error.what());
    } catch (...) {
        emit_exception_fault("configure_session", "unknown exception");
    }
    return false;
}

bool OrbitalSessionNode::queue_begin_aim() noexcept
{
    const bool result = adapter_.queue_begin_aim();
    emit_pending_fault();
    return result;
}

bool OrbitalSessionNode::queue_aim(
    godot::Vector3 origin,
    godot::Vector3 tangent_direction,
    double speed) noexcept
{
    const auto kernel_origin = detail::to_kernel_checked(origin);
    const auto kernel_direction = detail::to_kernel_checked(tangent_direction);
    if (!kernel_origin || !kernel_direction || !std::isfinite(speed)) {
        return false;
    }
    const bool result = adapter_.queue_aim(*kernel_origin, *kernel_direction, speed);
    emit_pending_fault();
    return result;
}

bool OrbitalSessionNode::queue_launch() noexcept
{
    const bool result = adapter_.queue_launch();
    emit_pending_fault();
    return result;
}

bool OrbitalSessionNode::queue_activate_ability() noexcept
{
    const bool result = adapter_.queue_activate_ability();
    emit_pending_fault();
    return result;
}

bool OrbitalSessionNode::queue_cancel_aim() noexcept
{
    const bool result = adapter_.queue_cancel_aim();
    emit_pending_fault();
    return result;
}

bool OrbitalSessionNode::restart_level() noexcept
{
    const bool result = adapter_.restart();
    emit_pending_fault();
    return result;
}

godot::Dictionary OrbitalSessionNode::consume_frame() noexcept
{
    try {
        const detail::SessionFrameData& frame = adapter_.peek_frame();
        godot::Dictionary result = frame_dictionary(frame);
        adapter_.acknowledge_frame();
        emit_pending_fault();
        return result;
    } catch (const std::exception& error) {
        emit_exception_fault("consume_frame", error.what());
    } catch (...) {
        emit_exception_fault("consume_frame", "unknown exception");
    }
    return {};
}

void OrbitalSessionNode::_physics_process(double delta) noexcept
{
    (void)adapter_.advance(delta);
    emit_pending_fault();
}

}
