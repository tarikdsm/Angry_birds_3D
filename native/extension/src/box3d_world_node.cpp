#include <ninho/extension/box3d_world_node.hpp>

#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/core/property_info.hpp>
#include <godot_cpp/variant/string_name.hpp>
#include <godot_cpp/variant/variant.hpp>

#include <cmath>
#include <exception>
#include <string>

namespace ninho::extension {
namespace {

[[nodiscard]] const char* status_code_name(physics::StatusCode code) noexcept
{
    switch (code) {
    case physics::StatusCode::Ok:
        return "ok";
    case physics::StatusCode::InvalidArgument:
        return "invalid_argument";
    case physics::StatusCode::InvalidHandle:
        return "invalid_handle";
    case physics::StatusCode::CapacityExceeded:
        return "capacity_exceeded";
    case physics::StatusCode::Unsupported:
        return "unsupported";
    case physics::StatusCode::Box3DFault:
        return "physics_backend_fault";
    }
    return "unknown_status";
}

[[nodiscard]] bool positive_finite(double value) noexcept
{
    return std::isfinite(value) && value > 0.0;
}

[[nodiscard]] bool positive_finite(const godot::Vector3& value) noexcept
{
    return detail::is_finite(value) && value.x > 0.0 && value.y > 0.0 && value.z > 0.0;
}

}

void Box3DWorldNode::_bind_methods()
{
    godot::ClassDB::bind_method(
        godot::D_METHOD("configure_planet", "radius", "surface_gravity"),
        &Box3DWorldNode::configure_planet);
    godot::ClassDB::bind_method(
        godot::D_METHOD("spawn_box", "full_size", "transform", "density"),
        &Box3DWorldNode::spawn_box);
    godot::ClassDB::bind_method(
        godot::D_METHOD("spawn_projectile", "radius", "transform", "velocity"),
        &Box3DWorldNode::spawn_projectile);
    godot::ClassDB::bind_method(
        godot::D_METHOD("apply_impulse", "handle", "impulse", "world_point"),
        &Box3DWorldNode::apply_impulse);
    godot::ClassDB::bind_method(
        godot::D_METHOD("step_fixed"), &Box3DWorldNode::step_fixed);
    godot::ClassDB::bind_method(
        godot::D_METHOD("get_body_states"), &Box3DWorldNode::get_body_states);
    godot::ClassDB::bind_method(
        godot::D_METHOD("get_metrics"), &Box3DWorldNode::get_metrics);
    godot::ClassDB::bind_method(
        godot::D_METHOD("reset_world"), &Box3DWorldNode::reset_world);

    ADD_SIGNAL(godot::MethodInfo(
        "physics_fault",
        godot::PropertyInfo(godot::Variant::STRING, "code"),
        godot::PropertyInfo(godot::Variant::STRING, "message")));
}

void Box3DWorldNode::emit_fault(std::string_view code, std::string_view message) noexcept
{
    try {
        emit_signal(
            "physics_fault",
            godot::String::utf8(code.data(), static_cast<std::int64_t>(code.size())),
            godot::String::utf8(message.data(), static_cast<std::int64_t>(message.size())));
    } catch (...) {
        // A diagnostic must never turn into an exception crossing the Godot ABI.
    }
}

void Box3DWorldNode::emit_status_fault(
    std::string_view operation, const physics::Status& status) noexcept
{
    try {
        std::string message{operation};
        message += ": ";
        message += status.message;
        emit_fault(status_code_name(status.code), message);
    } catch (...) {
        emit_fault(status_code_name(status.code), "adapter failed to format kernel status");
    }
}

void Box3DWorldNode::emit_exception_fault(
    std::string_view operation, const char* message) noexcept
{
    try {
        std::string detail{operation};
        detail += ": ";
        detail += message != nullptr ? message : "unknown exception";
        emit_fault("exception", detail);
    } catch (...) {
        emit_fault("exception", "adapter exception while formatting diagnostic");
    }
}

bool Box3DWorldNode::require_world(std::string_view operation) noexcept
{
    if (world_) {
        return true;
    }
    emit_fault("world_not_configured", operation);
    return false;
}

bool Box3DWorldNode::recreate_world(const physics::WorldConfig& config)
{
    auto replacement = std::make_unique<physics::PhysicsWorld>(config);
    const physics::Result<physics::BodyHandle> planet =
        replacement->create_body(detail::make_planet_desc(config.planet_radius));
    if (!planet) {
        emit_status_fault("create_planet", planet.status);
        return false;
    }

    world_ = std::move(replacement);
    config_ = config;
    accumulator_.reset();
    return true;
}

bool Box3DWorldNode::configure_planet(double radius, double surface_gravity)
{
    if (!positive_finite(radius) || !positive_finite(surface_gravity)) {
        emit_fault("invalid_argument", "planet radius and surface gravity must be finite and positive");
        return false;
    }

    try {
        return recreate_world(detail::make_world_config(radius, surface_gravity));
    } catch (const std::exception& error) {
        emit_exception_fault("configure_planet", error.what());
    } catch (...) {
        emit_exception_fault("configure_planet", "unknown exception");
    }
    return false;
}

std::int64_t Box3DWorldNode::spawn_box(
    godot::Vector3 full_size, godot::Transform3D transform, double density)
{
    if (!positive_finite(full_size) || !detail::is_finite(transform)
        || !positive_finite(density)) {
        emit_fault("invalid_argument", "box size, transform, and density must be finite and positive");
        return 0;
    }
    if (!require_world("spawn_box requires a configured world")) {
        return 0;
    }

    try {
        const physics::Result<physics::BodyHandle> created =
            world_->create_body(detail::make_box_desc(full_size, transform, density));
        if (!created) {
            emit_status_fault("spawn_box", created.status);
            return 0;
        }
        return detail::pack_handle(created.value);
    } catch (const std::exception& error) {
        emit_exception_fault("spawn_box", error.what());
    } catch (...) {
        emit_exception_fault("spawn_box", "unknown exception");
    }
    return 0;
}

std::int64_t Box3DWorldNode::spawn_projectile(
    double radius, godot::Transform3D transform, godot::Vector3 velocity)
{
    if (!positive_finite(radius) || !detail::is_finite(transform)
        || !detail::is_finite(velocity)) {
        emit_fault("invalid_argument", "projectile radius, transform, and velocity must be finite");
        return 0;
    }
    if (!require_world("spawn_projectile requires a configured world")) {
        return 0;
    }

    try {
        const physics::Result<physics::BodyHandle> created =
            world_->create_body(detail::make_projectile_desc(radius, transform, velocity));
        if (!created) {
            emit_status_fault("spawn_projectile", created.status);
            return 0;
        }
        return detail::pack_handle(created.value);
    } catch (const std::exception& error) {
        emit_exception_fault("spawn_projectile", error.what());
    } catch (...) {
        emit_exception_fault("spawn_projectile", "unknown exception");
    }
    return 0;
}

bool Box3DWorldNode::apply_impulse(
    std::int64_t handle, godot::Vector3 impulse, godot::Vector3 world_point)
{
    if (!require_world("apply_impulse requires a configured world")) {
        return false;
    }

    try {
        const physics::Status status =
            detail::apply_impulse_checked(*world_, handle, impulse, world_point);
        if (!status.ok()) {
            emit_status_fault("apply_impulse", status);
            return false;
        }
        return true;
    } catch (const std::exception& error) {
        emit_exception_fault("apply_impulse", error.what());
    } catch (...) {
        emit_exception_fault("apply_impulse", "unknown exception");
    }
    return false;
}

bool Box3DWorldNode::step_fixed()
{
    if (!require_world("step_fixed requires a configured world")) {
        return false;
    }
    try {
        world_->step();
        return true;
    } catch (const std::exception& error) {
        emit_exception_fault("step_fixed", error.what());
    } catch (...) {
        emit_exception_fault("step_fixed", "unknown exception");
    }
    return false;
}

godot::Array Box3DWorldNode::get_body_states()
{
    godot::Array snapshots;
    if (!require_world("get_body_states requires a configured world")) {
        return snapshots;
    }

    try {
        for (const physics::BodyState& state : world_->states()) {
            godot::Dictionary value;
            value["handle"] = detail::pack_handle(state.handle);
            value["position"] = detail::to_godot(state.transform.position);
            value["rotation"] = detail::to_godot(state.transform.rotation);
            value["linear_velocity"] = detail::to_godot(state.linear_velocity);
            value["angular_velocity"] = detail::to_godot(state.angular_velocity);
            value["mass"] = state.mass;
            value["awake"] = state.awake;
            value["ejected"] = state.ejected;
            snapshots.push_back(value);
        }
        return snapshots;
    } catch (const std::exception& error) {
        emit_exception_fault("get_body_states", error.what());
    } catch (...) {
        emit_exception_fault("get_body_states", "unknown exception");
    }
    snapshots.clear();
    return snapshots;
}

godot::Dictionary Box3DWorldNode::get_metrics()
{
    godot::Dictionary result;
    if (!require_world("get_metrics requires a configured world")) {
        return result;
    }

    try {
        const physics::WorldMetrics metrics = world_->metrics();
        result["body_count"] = metrics.body_count;
        result["shape_count"] = metrics.shape_count;
        result["joint_count"] = metrics.joint_count;
        result["contact_count"] = metrics.contact_count;
        result["awake_count"] = metrics.awake_count;
        result["step_ms"] = metrics.step_ms;
        return result;
    } catch (const std::exception& error) {
        emit_exception_fault("get_metrics", error.what());
    } catch (...) {
        emit_exception_fault("get_metrics", "unknown exception");
    }
    result.clear();
    return result;
}

bool Box3DWorldNode::reset_world()
{
    try {
        return recreate_world(config_);
    } catch (const std::exception& error) {
        accumulator_.reset();
        emit_exception_fault("reset_world", error.what());
    } catch (...) {
        accumulator_.reset();
        emit_exception_fault("reset_world", "unknown exception");
    }
    return false;
}

void Box3DWorldNode::_physics_process(double delta)
{
    if (!world_) {
        accumulator_.reset();
        return;
    }

    try {
        const detail::TickSchedule schedule = accumulator_.schedule(delta, config_.time_step);
        if (!schedule.ok) {
            emit_fault("invalid_delta", "physics delta must be finite and non-negative");
            return;
        }
        for (int tick = 0; tick < schedule.tick_count; ++tick) {
            if (!step_fixed()) {
                accumulator_.reset();
                break;
            }
        }
    } catch (const std::exception& error) {
        accumulator_.reset();
        emit_exception_fault("_physics_process", error.what());
    } catch (...) {
        accumulator_.reset();
        emit_exception_fault("_physics_process", "unknown exception");
    }
}

}
