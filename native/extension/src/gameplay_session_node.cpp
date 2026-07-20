#include <ninho/extension/gameplay_session_node.hpp>

#include <ninho/extension/adapter_helpers.hpp>
#include <ninho/physics/gravity_field.hpp>

#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/core/property_info.hpp>
#include <godot_cpp/variant/array.hpp>
#include <godot_cpp/variant/typed_array.hpp>
#include <godot_cpp/variant/variant.hpp>

#include <algorithm>
#include <cmath>
#include <exception>
#include <type_traits>
#include <utility>
#include <vector>

namespace ninho::extension::detail {
namespace {

[[nodiscard]] physics::Vec3 vector_from(
    const std::array<double, 3>& value) noexcept
{
    return {
        static_cast<float>(value[0]),
        static_cast<float>(value[1]),
        static_cast<float>(value[2]),
    };
}

[[nodiscard]] LockedPlaneFrameData locked_plane_from(
    const simulation::LauncherState& launcher) noexcept
{
    return {
        .camera_right = launcher.camera_right,
        .up = launcher.up,
        .horizontal = launcher.horizontal,
        .plane_normal = launcher.plane_normal,
    };
}

[[nodiscard]] LockedPlaneFrameData locked_plane_from(
    const simulation::LockedLaunchPlaneView& plane) noexcept
{
    return {
        .camera_right = plane.camera_right,
        .up = plane.up,
        .horizontal = plane.horizontal,
        .plane_normal = plane.plane_normal,
    };
}

[[nodiscard]] std::pair<std::string, physics::Vec3> gravity_frame_from(
    const simulation::LevelManifest& level)
{
    const physics::Vec3 sample_position = vector_from(level.slingshot.rest_position_m);
    if (const auto* uniform =
            std::get_if<simulation::UniformWorldDefinition>(&level.world)) {
        const physics::Vec3 acceleration = vector_from(uniform->acceleration_m_s2);
        const physics::GravityField field{
            physics::UniformGravityConfig{acceleration}};
        return {"uniform", field.acceleration_at(sample_position)};
    }
    const auto& radial = std::get<simulation::RadialWorldDefinition>(level.world);
    const physics::GravityField field{physics::RadialGravityConfig{
        .center_m = vector_from(radial.center_m),
        .reference_radius_m = static_cast<float>(radial.reference_radius_m),
        .reference_acceleration_m_s2 =
            static_cast<float>(radial.reference_acceleration_m_s2),
    }};
    return {"radial", field.acceleration_at(sample_position)};
}

[[nodiscard]] GameplayFrameFields gameplay_fields_from(
    const simulation::SimulationSession& session,
    const simulation::ContentBundle& content,
    const std::optional<simulation::ShotStateView>& shot)
{
    GameplayFrameFields fields;
    const std::uint32_t remaining = session.birds_remaining();
    const std::size_t queue_size = content.level.bird_queue.size();
    const std::size_t first_remaining = remaining <= queue_size
        ? queue_size - static_cast<std::size_t>(remaining)
        : queue_size;
    fields.bird_queue.assign(
        content.level.bird_queue.begin() + static_cast<std::ptrdiff_t>(first_remaining),
        content.level.bird_queue.end());
    if (!fields.bird_queue.empty()) {
        fields.current_bird = fields.bird_queue.front();
    }
    if (shot) {
        fields.locked_plane = locked_plane_from(shot->locked_plane);
        fields.shot = ShotFrameData{
            .shot_id = shot->shot_id,
            .bird_archetype_id = shot->bird_archetype_id,
            .ability_id = shot->ability_id,
            .launch_tick = shot->launch_tick,
            .pull_horizontal_m = shot->pull_horizontal_m,
            .pull_vertical_m = shot->pull_vertical_m,
            .activation_consumed = shot->activation_consumed,
            .projectile_ids = shot->projectile_ids,
        };
        fields.projectiles.reserve(shot->projectile_ids.size());
        for (const simulation::EntityId projectile_id : shot->projectile_ids) {
            const auto snapshot = std::ranges::find(
                session.snapshots(), projectile_id,
                &simulation::EntitySnapshot::entity_id);
            if (snapshot != session.snapshots().end()) {
                fields.projectiles.push_back(*snapshot);
            }
        }
    } else if (session.state().phase == simulation::SessionPhase::Grabbed
        && session.state().launcher) {
        fields.locked_plane = locked_plane_from(*session.state().launcher);
    }

    auto [gravity_kind, local_gravity] = gravity_frame_from(content.level);
    fields.gravity_kind = std::move(gravity_kind);
    fields.local_gravity_m_s2 = local_gravity;
    return fields;
}

}

void GameplaySessionAdapter::latch_content_error(
    const simulation::ContentError& error) noexcept
{
    if (batch_.fault()) {
        return;
    }
    batch_.latch_fault(FaultInfo{
        simulation::content_error_code_name(error.code), error.pointer, error.message});
    ++fault_generation_;
}

void GameplaySessionAdapter::latch_exception(
    std::string_view operation, const char* message) noexcept
{
    if (batch_.fault()) {
        return;
    }
    batch_.latch_fault(FaultInfo{
        "exception", operation, message != nullptr ? message : "unknown exception"});
    ++fault_generation_;
}

void GameplaySessionAdapter::fail(
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

bool GameplaySessionAdapter::configure(
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
        const std::optional<simulation::ShotStateView> shot =
            candidate.value->shot_state();
        candidate_batch.capture_latest(
            candidate.value->snapshots(),
            candidate.value->state(),
            candidate.value->birds_remaining(),
            candidate.value->objectives_complete(),
            candidate.value->physics_metrics(),
            objective_targets,
            shot ? shot->ability_readiness : candidate.value->ability_readiness());
        candidate_batch.set_gameplay_fields(gameplay_fields_from(
            *candidate.value, candidate_content, shot));

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

bool GameplaySessionAdapter::accept_status(
    const simulation::SessionStatus& status) noexcept
{
    if (status.ok()) {
        return true;
    }
    latch_content_error(status.error);
    return false;
}

bool GameplaySessionAdapter::enqueue(simulation::PlayerCommand command) noexcept
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

bool GameplaySessionAdapter::queue_begin_grab(physics::Vec3 camera_right) noexcept
{
    if (!physics::is_finite(camera_right)) {
        return false;
    }
    return enqueue(simulation::BeginGrabCommand{camera_right});
}

bool GameplaySessionAdapter::queue_pull(
    double horizontal_m, double vertical_m) noexcept
{
    if (!std::isfinite(horizontal_m) || !std::isfinite(vertical_m)) {
        return false;
    }
    return enqueue(simulation::SetPullCommand{horizontal_m, vertical_m});
}

bool GameplaySessionAdapter::queue_release() noexcept
{
    return enqueue(simulation::ReleaseBirdCommand{});
}

bool GameplaySessionAdapter::queue_activate_ability() noexcept
{
    return enqueue(simulation::ActivateAbilityCommand{});
}

bool GameplaySessionAdapter::queue_cancel_grab() noexcept
{
    return enqueue(simulation::CancelGrabCommand{});
}

bool GameplaySessionAdapter::restart() noexcept
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
        const std::optional<simulation::ShotStateView> shot =
            candidate.value->shot_state();
        candidate_batch.capture_latest(
            candidate.value->snapshots(),
            candidate.value->state(),
            candidate.value->birds_remaining(),
            candidate.value->objectives_complete(),
            candidate.value->physics_metrics(),
            objective_targets,
            shot ? shot->ability_readiness : candidate.value->ability_readiness());
        candidate_batch.set_gameplay_fields(gameplay_fields_from(
            *candidate.value, content_, shot));

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

void GameplaySessionAdapter::capture_latest()
{
    const std::optional<simulation::ShotStateView> shot = session_->shot_state();
    const std::vector<simulation::ObjectiveTargetStatus> objective_targets =
        session_->objective_target_statuses();
    batch_.capture_latest(
        session_->snapshots(),
        session_->state(),
        session_->birds_remaining(),
        session_->objectives_complete(),
        session_->physics_metrics(),
        objective_targets,
        shot ? shot->ability_readiness : session_->ability_readiness());
    batch_.set_gameplay_fields(gameplay_fields_from(*session_, content_, shot));

    if (session_->state().phase == simulation::SessionPhase::Grabbed) {
        simulation::TrajectoryPreview preview = session_->preview();
        if (preview.status.ok()) {
            batch_.set_preview(std::move(preview));
        } else {
            batch_.clear_preview();
        }
    }
}

bool GameplaySessionAdapter::advance(double delta) noexcept
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

SessionFrameData GameplaySessionAdapter::consume_frame() noexcept
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

const SessionFrameData& GameplaySessionAdapter::peek_frame() const noexcept
{
    return batch_.peek();
}

void GameplaySessionAdapter::acknowledge_frame() noexcept
{
    batch_.acknowledge();
}

bool GameplaySessionAdapter::configured() const noexcept
{
    return session_ != nullptr;
}

const std::optional<FaultInfo>& GameplaySessionAdapter::fault() const noexcept
{
    return batch_.fault();
}

std::uint64_t GameplaySessionAdapter::fault_generation() const noexcept
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
    case simulation::SessionPhase::Grabbed: return "grabbed";
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

[[nodiscard]] const char* shape_type_name(simulation::ShapeType type) noexcept
{
    switch (type) {
    case simulation::ShapeType::Box: return "box";
    case simulation::ShapeType::Sphere: return "sphere";
    case simulation::ShapeType::Capsule: return "capsule";
    case simulation::ShapeType::ConvexHull: return "convex_hull";
    case simulation::ShapeType::Compound: return "compound";
    }
    return "unknown";
}

[[nodiscard]] const char* body_type_name(simulation::BodyType type) noexcept
{
    return type == simulation::BodyType::Static ? "static" : "dynamic";
}

[[nodiscard]] godot::Dictionary snapshot_dictionary(
    const simulation::EntitySnapshot& snapshot)
{
    godot::Dictionary shape;
    shape["type"] = shape_type_name(snapshot.shape.type);
    shape["half_extents"] = godot::Vector3{
        static_cast<godot::real_t>(snapshot.shape.half_extents_m[0]),
        static_cast<godot::real_t>(snapshot.shape.half_extents_m[1]),
        static_cast<godot::real_t>(snapshot.shape.half_extents_m[2])};
    shape["radius"] = snapshot.shape.radius_m;
    shape["half_height"] = snapshot.shape.half_height_m;

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
    result["exited_world"] = snapshot.exited_world;
    result["is_projectile"] = snapshot.is_projectile;
    return result;
}

[[nodiscard]] godot::Dictionary event_dictionary(
    const simulation::DomainEvent& event)
{
    godot::Dictionary result;
    result["id"] = static_cast<std::int64_t>(event.id.value());
    result["tick"] = static_cast<std::int64_t>(event.tick.value());
    result["kind"] = event_kind_name(event.kind);
    result["entity_id"] = static_cast<std::int64_t>(event.entity_id.value());
    result["bird_archetype_id"] =
        static_cast<std::int64_t>(event.bird_archetype_id.value());
    result["rejection_reason"] = static_cast<std::int64_t>(event.rejection_reason);
    result["rejection_reason_name"] = rejection_reason_name(event.rejection_reason);
    result["ability_id"] = static_cast<std::int64_t>(event.ability_id.value());
    result["affected_entity_id"] =
        static_cast<std::int64_t>(event.affected_entity_id.value());
    result["affected_part_id"] =
        static_cast<std::int64_t>(event.affected_part_id.value());
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
    result["neutralization_cause"] =
        static_cast<std::int64_t>(event.neutralization_cause);
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

[[nodiscard]] godot::Variant launcher_variant(
    const std::optional<simulation::LauncherState>& launcher)
{
    if (!launcher) {
        return {};
    }
    godot::Dictionary result;
    result["rest_position"] = detail::to_godot(launcher->rest_position_m);
    result["pull_horizontal_m"] = launcher->pull_horizontal_m;
    result["pull_vertical_m"] = launcher->pull_vertical_m;
    result["extension_m"] = launcher->extension_m;
    result["spring_energy_j"] = launcher->spring_energy_j;
    result["launch_energy_j"] = launcher->launch_energy_j;
    result["launch_direction"] = detail::to_godot(launcher->launch_direction);
    result["predicted_speed_m_s"] = launcher->predicted_speed_m_s;
    result["deadzone_m"] = launcher->deadzone_m;
    result["maximum_extension_m"] = launcher->maximum_extension_m;
    return result;
}

[[nodiscard]] godot::Variant locked_plane_variant(
    const std::optional<detail::LockedPlaneFrameData>& plane)
{
    if (!plane) {
        return {};
    }
    godot::Dictionary result;
    result["camera_right"] = detail::to_godot(plane->camera_right);
    result["up"] = detail::to_godot(plane->up);
    result["horizontal"] = detail::to_godot(plane->horizontal);
    result["plane_normal"] = detail::to_godot(plane->plane_normal);
    return result;
}

[[nodiscard]] godot::Variant preview_variant(
    const std::optional<simulation::TrajectoryPreview>& preview)
{
    if (!preview) {
        return {};
    }
    godot::Dictionary result;
    godot::TypedArray<godot::Vector3> samples;
    for (const physics::Vec3 sample : preview->samples) {
        samples.push_back(detail::to_godot(sample));
    }
    result["samples"] = samples;
    if (preview->first_hit) {
        godot::Dictionary hit;
        hit["entity_id"] =
            static_cast<std::int64_t>(preview->first_hit->entity_id.value());
        hit["part_id"] =
            static_cast<std::int64_t>(preview->first_hit->part_id.value());
        hit["point"] = detail::to_godot(preview->first_hit->point_m);
        hit["normal"] = detail::to_godot(preview->first_hit->normal);
        result["first_hit"] = hit;
    } else {
        result["first_hit"] = godot::Variant{};
    }
    result["canonical_hash"] = static_cast<std::int64_t>(preview->canonical_hash);
    return result;
}

[[nodiscard]] godot::Variant shot_variant(
    const std::optional<detail::ShotFrameData>& shot)
{
    if (!shot) {
        return {};
    }
    godot::Dictionary result;
    result["shot_id"] = static_cast<std::int64_t>(shot->shot_id);
    result["bird_archetype_id"] =
        static_cast<std::int64_t>(shot->bird_archetype_id.value());
    result["ability_id"] = static_cast<std::int64_t>(shot->ability_id.value());
    result["launch_tick"] = static_cast<std::int64_t>(shot->launch_tick.value());
    result["pull_horizontal_m"] = shot->pull_horizontal_m;
    result["pull_vertical_m"] = shot->pull_vertical_m;
    result["activation_consumed"] = shot->activation_consumed;
    godot::Array projectile_ids;
    for (const simulation::EntityId id : shot->projectile_ids) {
        projectile_ids.push_back(static_cast<std::int64_t>(id.value()));
    }
    result["projectile_ids"] = projectile_ids;
    return result;
}

[[nodiscard]] godot::Dictionary objectives_dictionary(
    const detail::SessionFrameData& frame)
{
    godot::TypedArray<godot::Dictionary> targets;
    for (const auto& target : frame.objective_targets) {
        godot::Dictionary item;
        item["entity_id"] = static_cast<std::int64_t>(target.entity_id.value());
        item["current_integrity"] = target.current_integrity;
        item["maximum_integrity"] = target.maximum_integrity;
        item["neutralized"] = target.neutralized;
        targets.push_back(item);
    }
    godot::Dictionary result;
    result["complete"] = frame.objectives_complete;
    result["targets"] = targets;
    return result;
}

[[nodiscard]] godot::Dictionary metrics_dictionary(
    const physics::WorldMetrics& metrics)
{
    godot::Dictionary result;
    result["body_count"] = metrics.body_count;
    result["shape_count"] = metrics.shape_count;
    result["joint_count"] = metrics.joint_count;
    result["contact_count"] = metrics.contact_count;
    result["awake_count"] = metrics.awake_count;
    result["step_ms"] = metrics.step_ms;
    return result;
}

[[nodiscard]] godot::Dictionary gameplay_frame_dictionary(const detail::SessionFrameData& frame)
{
    godot::Array bird_queue;
    for (const simulation::BirdArchetypeId id : frame.bird_queue) {
        bird_queue.push_back(static_cast<std::int64_t>(id.value()));
    }
    godot::Variant current_bird;
    if (frame.current_bird) {
        current_bird = static_cast<std::int64_t>(frame.current_bird->value());
    }
    godot::TypedArray<godot::Dictionary> projectiles;
    for (const auto& projectile : frame.projectiles) {
        projectiles.push_back(snapshot_dictionary(projectile));
    }
    godot::TypedArray<godot::Dictionary> snapshots;
    for (const auto& snapshot : frame.snapshots) {
        snapshots.push_back(snapshot_dictionary(snapshot));
    }
    godot::TypedArray<godot::Dictionary> events;
    for (const auto& event : frame.events) {
        events.push_back(event_dictionary(event));
    }

    godot::Dictionary result;
    result["frame_schema_version"] = 2;
    result["tick"] = static_cast<std::int64_t>(frame.state.tick.value());
    result["ticks_executed"] = frame.ticks_executed;
    result["phase"] = phase_name(frame.state.phase);
    result["outcome"] = outcome_name(frame.state.outcome);
    result["launcher"] = launcher_variant(frame.state.launcher);
    result["locked_plane"] = locked_plane_variant(frame.locked_plane);
    result["bird_queue"] = bird_queue;
    result["current_bird"] = current_bird;
    result["shot"] = shot_variant(frame.shot);
    result["projectiles"] = projectiles;
    result["snapshots"] = snapshots;
    result["events"] = events;
    result["objectives"] = objectives_dictionary(frame);
    result["ability_readiness"] = ability_readiness_name(frame.ability_readiness);
    result["ability_armed"] =
        frame.ability_readiness == simulation::AbilityReadiness::Armed;
    result["trajectory_preview"] = preview_variant(frame.preview);
    result["score"] = static_cast<std::int64_t>(frame.score);
    result["stars"] = static_cast<std::int64_t>(frame.stars);
    result["gravity_kind"] = godot_string(frame.gravity_kind);
    result["local_gravity"] = detail::to_godot(frame.local_gravity_m_s2);
    result["metrics"] = metrics_dictionary(frame.metrics);
    result["discarded_time_seconds"] = frame.discarded_time_seconds;
    return result;
}

}

GameplaySessionNode::GameplaySessionNode()
{
    set_process_priority(-100);
}

void GameplaySessionNode::_bind_methods()
{
    godot::ClassDB::bind_method(
        godot::D_METHOD("configure_session", "materials_json", "archetypes_json", "level_json"),
        &GameplaySessionNode::configure_session);
    godot::ClassDB::bind_method(
        godot::D_METHOD("queue_begin_grab", "camera_right"),
        &GameplaySessionNode::queue_begin_grab);
    godot::ClassDB::bind_method(
        godot::D_METHOD("queue_pull", "horizontal_m", "vertical_m"),
        &GameplaySessionNode::queue_pull);
    godot::ClassDB::bind_method(
        godot::D_METHOD("queue_release"), &GameplaySessionNode::queue_release);
    godot::ClassDB::bind_method(
        godot::D_METHOD("queue_activate_ability"),
        &GameplaySessionNode::queue_activate_ability);
    godot::ClassDB::bind_method(
        godot::D_METHOD("queue_cancel_grab"),
        &GameplaySessionNode::queue_cancel_grab);
    godot::ClassDB::bind_method(
        godot::D_METHOD("restart_level"), &GameplaySessionNode::restart_level);
    godot::ClassDB::bind_method(
        godot::D_METHOD("consume_frame"), &GameplaySessionNode::consume_frame);

    ADD_SIGNAL(godot::MethodInfo(
        "gameplay_fault",
        godot::PropertyInfo(godot::Variant::STRING, "code"),
        godot::PropertyInfo(godot::Variant::STRING, "message")));
}

void GameplaySessionNode::emit_pending_fault() noexcept
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
        // Diagnostics must never become exceptions crossing the GDExtension ABI.
    }
}

void GameplaySessionNode::emit_exception_fault(
    std::string_view operation, const char* message) noexcept
{
    (void)operation;
    adapter_.fail("exception", message != nullptr ? message : "unknown Godot adapter exception");
    emit_pending_fault();
}

bool GameplaySessionNode::configure_session(
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

bool GameplaySessionNode::queue_begin_grab(godot::Vector3 camera_right) noexcept
{
    const auto kernel_camera_right = detail::to_kernel_checked(camera_right);
    if (!kernel_camera_right) {
        return false;
    }
    const bool result = adapter_.queue_begin_grab(*kernel_camera_right);
    emit_pending_fault();
    return result;
}

bool GameplaySessionNode::queue_pull(double horizontal_m, double vertical_m) noexcept
{
    const bool result = adapter_.queue_pull(horizontal_m, vertical_m);
    emit_pending_fault();
    return result;
}

bool GameplaySessionNode::queue_release() noexcept
{
    const bool result = adapter_.queue_release();
    emit_pending_fault();
    return result;
}

bool GameplaySessionNode::queue_activate_ability() noexcept
{
    const bool result = adapter_.queue_activate_ability();
    emit_pending_fault();
    return result;
}

bool GameplaySessionNode::queue_cancel_grab() noexcept
{
    const bool result = adapter_.queue_cancel_grab();
    emit_pending_fault();
    return result;
}

bool GameplaySessionNode::restart_level() noexcept
{
    const bool result = adapter_.restart();
    emit_pending_fault();
    return result;
}

godot::Dictionary GameplaySessionNode::consume_frame() noexcept
{
    try {
        const detail::SessionFrameData& frame = adapter_.peek_frame();
        godot::Dictionary result = gameplay_frame_dictionary(frame);
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

void GameplaySessionNode::_physics_process(double delta) noexcept
{
    (void)adapter_.advance(delta);
    emit_pending_fault();
}

}
