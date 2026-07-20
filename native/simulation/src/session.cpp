#include "ninho/simulation/session.hpp"

#include "session_internal.hpp"
#if defined(NINHO_ENABLE_TEST_FACADES)
#include "session_test_facade.hpp"
#include "../../kernel/src/physics_world_test_facade.hpp"
#endif

#include <algorithm>
#include <exception>
#include <ranges>
#include <utility>

namespace ninho::simulation {

SimulationSession::SimulationSession(std::unique_ptr<Impl> implementation) noexcept
    : impl_(std::move(implementation))
{
}

SimulationSession::~SimulationSession() = default;
SimulationSession::SimulationSession(SimulationSession&&) noexcept = default;
SimulationSession& SimulationSession::operator=(SimulationSession&&) noexcept = default;

SessionStatus SimulationSession::reconfigure(
    const MaterialCatalog& materials,
    const ArchetypeCatalog& archetypes,
    const LevelManifest& level)
{
    auto candidate = create(materials, archetypes, level);
    if (!candidate.ok()) {
        return {candidate.error};
    }
    impl_.swap(candidate.value->impl_);
    return {};
}

SessionStatus SimulationSession::restart()
{
    return reconfigure(impl_->bundle.materials, impl_->bundle.archetypes, impl_->bundle.level);
}

SessionStatus SimulationSession::tick()
{
    if (impl_->latched_fault) {
        return *impl_->latched_fault;
    }
    try {
        impl_->domain_events.clear();
        impl_->session_state.tick = TickIndex{impl_->session_state.tick.value() + 1U};
        impl_->apply_pending_fractures_before_step();
        const SessionStatus command_status = impl_->process_commands();
        if (!command_status.ok()) {
            impl_->session_state.phase = SessionPhase::Faulted;
            impl_->session_state.outcome = Outcome::None;
            impl_->latched_fault = command_status;
            return command_status;
        }
        impl_->update_fsm_before_step();
        const SessionStatus ability_status = impl_->apply_ability_before_step();
        if (!ability_status.ok()) {
            impl_->session_state.phase = SessionPhase::Faulted;
            impl_->session_state.outcome = Outcome::None;
            impl_->latched_fault = ability_status;
            return ability_status;
        }
        impl_->physics.step();
        const SessionStatus contact_status = impl_->process_ability_after_step();
        if (!contact_status.ok()) {
            impl_->session_state.phase = SessionPhase::Faulted;
            impl_->session_state.outcome = Outcome::None;
            impl_->latched_fault = contact_status;
            return contact_status;
        }
        impl_->process_damage_after_step();
        impl_->evaluate_fractures_after_step();
        impl_->evaluate_objectives_after_step();
        const SessionStatus finish_status = impl_->finish_ability_after_step();
        if (!finish_status.ok()) {
            impl_->session_state.phase = SessionPhase::Faulted;
            impl_->session_state.outcome = Outcome::None;
            impl_->latched_fault = finish_status;
            return finish_status;
        }
        impl_->remove_confirmed_runtime_body_records();
        impl_->update_fsm_after_step();
        impl_->rebuild_snapshots();
#if defined(NINHO_ENABLE_TEST_FACADES)
        if (impl_->snapshot_mass_override_for_testing && !impl_->entity_snapshots.empty()) {
            impl_->entity_snapshots.front().mass_kg =
                *impl_->snapshot_mass_override_for_testing;
            impl_->snapshot_mass_override_for_testing.reset();
        }
#endif
        impl_->refresh_canonical_state();
        return {};
    } catch (const std::exception& error) {
        impl_->session_state.phase = SessionPhase::Faulted;
        impl_->session_state.outcome = Outcome::None;
        try {
            impl_->latched_fault = SessionStatus{{
                ContentErrorCode::InternalError, "", error.what()}};
        } catch (...) {
            impl_->latched_fault.swap(impl_->emergency_fault);
        }
        return *impl_->latched_fault;
    } catch (...) {
        impl_->session_state.phase = SessionPhase::Faulted;
        impl_->session_state.outcome = Outcome::None;
        impl_->latched_fault.swap(impl_->emergency_fault);
        return *impl_->latched_fault;
    }
}

std::span<const EntitySnapshot> SimulationSession::snapshots() const noexcept
{
    return impl_->entity_snapshots;
}

std::span<const StructuralJointSnapshot> SimulationSession::structural_joints() const noexcept
{
    return impl_->joint_snapshots;
}

std::span<const DomainEvent> SimulationSession::events() const noexcept
{
    return impl_->domain_events;
}

const SessionState& SimulationSession::state() const noexcept
{
    return impl_->session_state;
}

std::uint32_t SimulationSession::birds_remaining() const noexcept
{
    return impl_->remaining_birds;
}

bool SimulationSession::objectives_complete() const noexcept
{
    return impl_->objective_complete;
}

std::vector<ObjectiveTargetStatus> SimulationSession::objective_target_statuses() const
{
    std::vector<ObjectiveTargetStatus> result;
    result.reserve(impl_->bundle.level.objectives.size());
    for (const ObjectiveDefinition& objective : impl_->bundle.level.objectives) {
        if (objective.kind != ObjectiveKind::NeutralizeEntity) {
            continue;
        }
        const auto body = std::ranges::find_if(impl_->body_records, [&](const auto& record) {
            return record.entity_id == objective.target_entity_id
                && record.enemy_archetype_id.has_value();
        });
        if (body == impl_->body_records.end()) {
            continue;
        }
        const auto enemy = std::ranges::find(impl_->bundle.archetypes.enemies,
            *body->enemy_archetype_id, &EnemyArchetype::id);
        if (enemy == impl_->bundle.archetypes.enemies.end()) {
            continue;
        }
        const auto damage = impl_->damage_system.state(body->entity_id, body->part_id);
        const bool neutralized = body->neutralized
            || (damage.has_value() && damage->neutralized);
        result.push_back({
            .entity_id = objective.target_entity_id,
            .current_integrity = neutralized ? 0.0
                : damage.has_value() ? damage->remaining_integrity : enemy->integrity,
            .maximum_integrity = enemy->integrity,
            .neutralized = neutralized,
        });
    }
    std::ranges::sort(result, {}, &ObjectiveTargetStatus::entity_id);
    return result;
}

AbilityReadiness SimulationSession::ability_readiness() const noexcept
{
    if (!impl_->shot || impl_->shot->primary_projectile() == nullptr) {
        return AbilityReadiness::Unavailable;
    }
    if (ability_runtime_active(impl_->shot->runtime)) {
        return AbilityReadiness::Active;
    }
    if (impl_->shot->activation_consumed
        || impl_->shot->primary_projectile()->finished
        || impl_->session_state.phase != SessionPhase::FlightAbility) {
        return AbilityReadiness::Spent;
    }
    const AbilityArchetype* ability = impl_->ability_archetype(
        impl_->shot->ability_id);
    if (ability == nullptr) {
        return AbilityReadiness::Unavailable;
    }
    const auto armed_tick = impl_->shot->launch_tick.value()
        + static_cast<std::uint64_t>(
            detail::AbilitySystem::activation_arm_ticks(*ability));
    return impl_->session_state.tick.value() >= armed_tick
        ? AbilityReadiness::Armed : AbilityReadiness::Arming;
}

std::optional<ShotStateView> SimulationSession::shot_state() const
{
    if (!impl_->shot) {
        return std::nullopt;
    }
    const ShotState& shot = *impl_->shot;
    ShotStateView result{
        .shot_id = shot.shot_id,
        .bird_archetype_id = shot.bird_archetype_id,
        .ability_id = shot.ability_id,
        .launch_tick = shot.launch_tick,
        .locked_plane = {
            .camera_right = shot.locked_plane.camera_right,
            .up = shot.locked_plane.up,
            .horizontal = shot.locked_plane.horizontal,
            .plane_normal = shot.locked_plane.plane_normal,
        },
        .pull_horizontal_m = shot.pull_horizontal_m,
        .pull_vertical_m = shot.pull_vertical_m,
        .activation_consumed = shot.activation_consumed,
        .ability_readiness = ability_readiness(),
    };
    result.projectile_ids.reserve(shot.projectiles().size());
    for (const ProjectileState& projectile : shot.projectiles()) {
        result.projectile_ids.push_back(projectile.entity_id());
    }
    return result;
}

ninho::physics::WorldMetrics SimulationSession::physics_metrics() const noexcept
{
    return impl_->physics.metrics();
}

const std::vector<std::uint8_t>& SimulationSession::canonical_state_v2() const noexcept
{
    return impl_->canonical_bytes;
}

std::uint64_t SimulationSession::canonical_hash_v2() const noexcept
{
    return impl_->canonical_hash;
}

const std::vector<std::uint8_t>& SimulationSession::canonical_state_v3() const noexcept
{
    return impl_->canonical_v3_bytes;
}

std::uint64_t SimulationSession::canonical_hash_v3() const noexcept
{
    return impl_->canonical_v3_hash;
}

#if defined(NINHO_ENABLE_TEST_FACADES)
void detail::SessionTestFacade::fail_next_canonical_refresh(
    SimulationSession& session, std::string message)
{
    session.impl_->canonical_refresh_failure_for_testing = std::move(message);
}

void detail::SessionTestFacade::override_next_snapshot_mass(
    SimulationSession& session, double mass_kg)
{
    session.impl_->snapshot_mass_override_for_testing = mass_kg;
}

std::uint64_t detail::SessionTestFacade::last_processed_command_sequence(
    const SimulationSession& session)
{
    return session.impl_->last_processed_command_sequence;
}

bool detail::SessionTestFacade::projectile_is_bullet(const SimulationSession& session)
{
    return session.impl_->shot && session.impl_->shot->primary_projectile()
        && session.impl_->shot->primary_projectile()->bullet;
}

void detail::SessionTestFacade::finish_projectile(SimulationSession& session)
{
    if (session.impl_->shot && session.impl_->shot->primary_projectile()) {
        finish_projectile(
            session, session.impl_->shot->primary_projectile()->entity_id());
    }
}

void detail::SessionTestFacade::finish_projectile(
    SimulationSession& session, EntityId entity)
{
    if (!session.impl_->shot) {
        return;
    }
    const auto found = std::ranges::find(
        session.impl_->shot->projectiles(), entity, &ProjectileState::entity_id);
    if (found == session.impl_->shot->projectiles().end()) {
        return;
    }
    ProjectileState replacement = *found;
    replacement.finished = true;
    static_cast<void>(session.impl_->shot->replace_projectile(std::move(replacement)));
    session.impl_->force_settled_for_testing = true;
}

bool detail::SessionTestFacade::append_projectile_body_for_testing(
    SimulationSession& session, EntityId entity,
    ninho::physics::Vec3 position, ninho::physics::Vec3 linear_velocity)
{
    if (!session.impl_->shot
        || !add_dynamic_sphere(session, entity, PartId{1}, position,
            6.0, linear_velocity, 0.25)) {
        return false;
    }
    const auto record = std::ranges::find(
        session.impl_->body_records, entity,
        &SimulationSession::Impl::BodyRecord::entity_id);
    if (record == session.impl_->body_records.end()) {
        return false;
    }
    record->is_projectile = true;
    if (session.impl_->shot->insert_projectile(
            ProjectileState{entity, record->physics_handle, true})) {
        return true;
    }
    static_cast<void>(session.impl_->physics.destroy_body(record->physics_handle));
    session.impl_->body_records.erase(record);
    return false;
}

std::uint32_t detail::SessionTestFacade::projectile_age(
    const SimulationSession& session, EntityId entity)
{
    if (!session.impl_->shot) {
        return 0U;
    }
    const auto found = std::ranges::find(
        session.impl_->shot->projectiles(), entity, &ProjectileState::entity_id);
    return found == session.impl_->shot->projectiles().end()
        ? 0U : found->age_ticks;
}

bool detail::SessionTestFacade::has_body_record(
    const SimulationSession& session, EntityId entity)
{
    return std::ranges::any_of(session.impl_->body_records,
        [&](const auto& record) { return record.entity_id == entity; });
}

void detail::SessionTestFacade::complete_objective(SimulationSession& session)
{
    for (const ObjectiveDefinition& objective : session.impl_->bundle.level.objectives) {
        for (SimulationSession::Impl::BodyRecord& record : session.impl_->body_records) {
            if (record.entity_id == objective.target_entity_id) {
                record.neutralized = true;
            }
        }
    }
}

void detail::SessionTestFacade::set_ability_active(SimulationSession& session, bool active)
{
    if (session.impl_->shot) {
        set_ability_runtime_active(session.impl_->shot->runtime, active);
    }
}

void detail::SessionTestFacade::age_projectile(
    SimulationSession& session, std::uint32_t age_ticks)
{
    if (session.impl_->shot && session.impl_->shot->primary_projectile()) {
        age_projectile(session,
            session.impl_->shot->primary_projectile()->entity_id(), age_ticks);
    }
}

void detail::SessionTestFacade::age_projectile(
    SimulationSession& session, EntityId entity, std::uint32_t age_ticks)
{
    if (!session.impl_->shot) {
        return;
    }
    const auto found = std::ranges::find(
        session.impl_->shot->projectiles(), entity, &ProjectileState::entity_id);
    if (found != session.impl_->shot->projectiles().end()) {
        ProjectileState replacement = *found;
        replacement.age_ticks = age_ticks;
        static_cast<void>(
            session.impl_->shot->replace_projectile(std::move(replacement)));
    }
}

bool detail::SessionTestFacade::ability_requested(const SimulationSession& session)
{
    return session.impl_->shot && session.impl_->shot->activation_consumed;
}

bool detail::SessionTestFacade::ability_active(const SimulationSession& session)
{
    return session.impl_->shot
        && ability_runtime_active(session.impl_->shot->runtime);
}

void detail::SessionTestFacade::clear_speed_boost_direction_for_testing(
    SimulationSession& session)
{
    if (!session.impl_->shot) {
        return;
    }
    if (auto* runtime = std::get_if<SpeedBoostAbilityRuntime>(
            &session.impl_->shot->runtime)) {
        runtime->last_valid_flight_direction.reset();
    }
}

void detail::SessionTestFacade::set_speed_boost_direction_for_testing(
    SimulationSession& session, ninho::physics::Vec3 direction)
{
    if (!session.impl_->shot) {
        return;
    }
    if (auto* runtime = std::get_if<SpeedBoostAbilityRuntime>(
            &session.impl_->shot->runtime)) {
        runtime->last_valid_flight_direction = direction;
    }
}

bool detail::SessionTestFacade::set_last_speed_changed_delta_for_testing(
    SimulationSession& session, ninho::physics::Vec3 delta_velocity)
{
    const auto event = std::ranges::find(
        session.impl_->domain_events.rbegin(), session.impl_->domain_events.rend(),
        DomainEventKind::SpeedChanged, &DomainEvent::kind);
    if (event == session.impl_->domain_events.rend()) {
        return false;
    }
    event->delta_velocity_m_s = delta_velocity;
    return true;
}

int detail::SessionTestFacade::collision_group(
    const SimulationSession& session, EntityId entity)
{
    const auto record = std::ranges::find(
        session.impl_->body_records, entity,
        &SimulationSession::Impl::BodyRecord::entity_id);
    if (record == session.impl_->body_records.end()) {
        return 0;
    }
    const auto groups = ninho::physics::detail::PhysicsWorldTestFacade::
        shape_collision_groups(session.impl_->physics, record->physics_handle);
    return groups.empty() ? 0 : groups.front();
}

bool detail::SessionTestFacade::set_split_child_id_for_testing(
    SimulationSession& session, std::size_t index, EntityId entity)
{
    if (!session.impl_->shot || index >= 3U) {
        return false;
    }
    auto* runtime = std::get_if<SplitAbilityRuntime>(&session.impl_->shot->runtime);
    if (runtime == nullptr || !runtime->applied) {
        return false;
    }
    runtime->child_ids[index] = entity;
    return true;
}

bool detail::SessionTestFacade::set_split_grace_end_for_testing(
    SimulationSession& session, TickIndex tick)
{
    if (!session.impl_->shot) {
        return false;
    }
    auto* runtime = std::get_if<SplitAbilityRuntime>(&session.impl_->shot->runtime);
    if (runtime == nullptr || !runtime->applied) {
        return false;
    }
    runtime->grace_end_tick = tick;
    return true;
}

void detail::SessionTestFacade::invalidate_locked_plane_for_testing(
    SimulationSession& session)
{
    if (session.impl_->shot) {
        session.impl_->shot->locked_plane.plane_normal = {};
    }
}

bool detail::SessionTestFacade::impulse_entity(
    SimulationSession& session, EntityId entity, ninho::physics::Vec3 impulse)
{
    const auto found = std::ranges::find(
        session.impl_->body_records, entity, &SimulationSession::Impl::BodyRecord::entity_id);
    if (found == session.impl_->body_records.end()) {
        return false;
    }
    const auto state = session.impl_->physics.state(found->physics_handle);
    return state && session.impl_->physics.apply_impulse(
        found->physics_handle, impulse, state->transform.position).ok();
}

bool detail::SessionTestFacade::add_static_sphere(SimulationSession& session,
    EntityId entity, ninho::physics::Vec3 position, double radius_m)
{
    auto description = ninho::physics::BodyDesc::static_sphere(
        static_cast<float>(radius_m), {position, {}});
    description.name = "TEST_StaticPreviewTarget";
    const auto created = session.impl_->physics.create_body(description);
    if (!created) {
        return false;
    }
    session.impl_->body_records.push_back({0U, entity, PartId{1}, BodyType::Static,
        std::nullopt, SurfaceId{1002}, std::nullopt,
        {.type = ShapeType::Sphere, .radius_m = radius_m},
        "TEST_StaticPreviewTarget", created.value, false, false, false});
    return true;
}

bool detail::SessionTestFacade::add_dynamic_sphere(SimulationSession& session,
    EntityId entity, PartId part, ninho::physics::Vec3 position, double mass_kg,
    ninho::physics::Vec3 linear_velocity, double radius_m)
{
    const double sphere_volume_m3 = 4.0 / 3.0 * 3.14159265358979323846
        * radius_m * radius_m * radius_m;
    auto description = ninho::physics::BodyDesc::dynamic_sphere(
        static_cast<float>(radius_m), {position, {}},
        static_cast<float>(mass_kg / sphere_volume_m3));
    description.linear_velocity = linear_velocity;
    description.name = "TEST_AbilityCandidate";
    const auto created = session.impl_->physics.create_body(description);
    if (!created) {
        return false;
    }
    session.impl_->body_records.push_back({0U, entity, part, BodyType::Dynamic,
        std::nullopt, std::nullopt, std::nullopt,
        {.type = ShapeType::Sphere, .radius_m = radius_m},
        "TEST_AbilityCandidate", created.value, false, false, true});
    return true;
}

bool detail::SessionTestFacade::affected_by_world_gravity(
    const SimulationSession& session, EntityId entity, PartId part)
{
    const auto record = std::ranges::find_if(session.impl_->body_records,
        [&](const auto& value) {
            return value.entity_id == entity && value.part_id == part;
        });
    return record != session.impl_->body_records.end()
        && record->affected_by_world_gravity;
}

ninho::physics::Vec3 detail::SessionTestFacade::gravity_at(
    const SimulationSession& session, ninho::physics::Vec3 position)
{
    return session.impl_->physics.gravity_at(position);
}

std::optional<ninho::physics::Aabb> detail::SessionTestFacade::body_bounds(
    const SimulationSession& session, EntityId entity, PartId part)
{
    const auto record = std::ranges::find_if(session.impl_->body_records,
        [&](const auto& value) {
            return value.entity_id == entity && value.part_id == part;
        });
    if (record == session.impl_->body_records.end()) {
        return std::nullopt;
    }
    return session.impl_->physics.body_bounds(record->physics_handle);
}

bool detail::SessionTestFacade::set_body_neutralized(SimulationSession& session,
    EntityId entity, PartId part, bool neutralized)
{
    const auto record = std::ranges::find_if(session.impl_->body_records,
        [&](const auto& value) {
            return value.entity_id == entity && value.part_id == part;
        });
    if (record == session.impl_->body_records.end()) {
        return false;
    }
    record->neutralized = neutralized;
    return true;
}

void detail::SessionTestFacade::override_joint_ratio_after_solver(
    SimulationSession& session, JointId joint, double ratio)
{
    session.impl_->joint_ratio_overrides_for_testing[joint.value()] = {ratio};
}

void detail::SessionTestFacade::override_joint_ratio_without_new_cause_after_solver(
    SimulationSession& session, JointId joint, double ratio)
{
    session.impl_->joint_ratio_overrides_for_testing[joint.value()] = {ratio};
}

void detail::SessionTestFacade::fracture_piece_after_solver(
    SimulationSession& session, EntityId entity, PartId part,
    ninho::physics::Vec3 position)
{
    session.impl_->piece_fracture_requests_for_testing.push_back(
        {entity, part, position, false});
}

void detail::SessionTestFacade::fracture_piece_at_incident_tie_after_solver(
    SimulationSession& session, EntityId entity, PartId part)
{
    session.impl_->piece_fracture_requests_for_testing.push_back(
        {entity, part, {}, true});
}

TickIndex detail::SessionTestFacade::projectile_launch_tick(const SimulationSession& session)
{
    return session.impl_->shot ? session.impl_->shot->launch_tick : TickIndex{};
}

TickIndex detail::SessionTestFacade::ability_end_tick(const SimulationSession& session)
{
    if (!session.impl_->shot) {
        return {};
    }
    const auto end_tick = ability_runtime_end_tick(session.impl_->shot->runtime);
    return end_tick.value_or(TickIndex{});
}

BirdArchetypeId detail::SessionTestFacade::next_bird_archetype_id(
    const SimulationSession& session)
{
    const BirdArchetype* bird = session.impl_->next_bird_archetype();
    return bird == nullptr ? BirdArchetypeId{} : bird->id;
}

AbilityId detail::SessionTestFacade::ability_id(const SimulationSession& session)
{
    return session.impl_->shot ? session.impl_->shot->ability_id : AbilityId{};
}

void detail::SessionTestFacade::set_launch_ordinal(
    SimulationSession& session, std::uint32_t ordinal)
{
    session.impl_->launch_count = ordinal;
}

std::uint32_t detail::SessionTestFacade::launch_ordinal(
    const SimulationSession& session)
{
    return session.impl_->launch_count;
}

std::size_t detail::SessionTestFacade::remaining_body_capacity(
    const SimulationSession& session)
{
    return session.impl_->physics.remaining_body_capacity();
}

std::size_t detail::SessionTestFacade::body_record_count(const SimulationSession& session)
{
    return session.impl_->body_records.size();
}

std::size_t detail::SessionTestFacade::canonical_static_content_build_count(
    const SimulationSession& session)
{
    return session.impl_->canonical_static_content_builds;
}

std::vector<std::uint8_t> detail::SessionTestFacade::canonical_state_uncached(
    const SimulationSession& session)
{
    return session.impl_->canonical_state_uncached_for_testing();
}

void detail::SessionTestFacade::refresh_canonical_state(SimulationSession& session)
{
    session.impl_->refresh_canonical_state();
}

std::size_t detail::SessionTestFacade::snapshot_rebuild_count(
    const SimulationSession& session)
{
    return session.impl_->snapshot_rebuilds;
}

std::size_t detail::SessionTestFacade::snapshot_visual_copy_count(
    const SimulationSession& session)
{
    return session.impl_->snapshot_visual_id_copies;
}

std::vector<EntitySnapshot> detail::SessionTestFacade::snapshots_uncached(
    const SimulationSession& session)
{
    std::vector<EntitySnapshot> result;
    result.reserve(session.impl_->body_records.size());
    for (const SimulationSession::Impl::BodyRecord& record : session.impl_->body_records) {
        const auto state = session.impl_->physics.state(record.physics_handle);
        if (!state) {
            continue;
        }
        result.push_back({record.entity_id,
            record.part_id,
            record.body_type,
            record.material_id,
            record.surface_id,
            record.enemy_archetype_id,
            record.shape,
            record.visual_id,
            state->transform,
            state->linear_velocity,
            state->angular_velocity,
            state->mass,
            state->awake,
            state->ejected,
            state->exited_world,
            record.is_projectile});
    }
    std::ranges::sort(result, [](const auto& lhs, const auto& rhs) {
        return std::pair{lhs.entity_id, lhs.part_id}
            < std::pair{rhs.entity_id, rhs.part_id};
    });
    return result;
}

void detail::SessionTestFacade::rebuild_snapshots(SimulationSession& session)
{
    session.impl_->rebuild_snapshots();
}

bool detail::SessionTestFacade::set_snapshot_exited_world(
    SimulationSession& session, EntityId entity, PartId part, bool exited_world)
{
    const auto snapshot = std::ranges::find_if(session.impl_->entity_snapshots,
        [&](const EntitySnapshot& value) {
            return value.entity_id == entity && value.part_id == part;
        });
    if (snapshot == session.impl_->entity_snapshots.end()) {
        return false;
    }
    snapshot->exited_world = exited_world;
    return true;
}

bool detail::SessionTestFacade::append_projectile_for_testing(
    SimulationSession& session, EntityId entity)
{
    if (!session.impl_->shot) {
        return false;
    }
    const auto* source = session.impl_->shot->primary_projectile();
    if (source == nullptr) {
        return false;
    }
    return session.impl_->shot->insert_projectile(
        source->copy_with_entity_id(entity));
}

void detail::SessionTestFacade::set_shot_runtime_for_testing(
    SimulationSession& session, bool consumed, bool active,
    std::optional<TickIndex> start_tick, std::optional<TickIndex> end_tick)
{
    if (!session.impl_->shot) {
        return;
    }
    session.impl_->shot->activation_consumed = consumed;
    std::visit([&](auto& runtime) {
        runtime.start_tick = start_tick;
        runtime.end_tick = end_tick;
        runtime.active = active;
    }, session.impl_->shot->runtime);
}

std::int64_t detail::SessionTestFacade::quantize_canonical(double value)
{
    return canonical_quantize(value);
}
#endif

}
