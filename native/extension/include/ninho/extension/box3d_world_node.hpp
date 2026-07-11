#pragma once

#include <ninho/extension/adapter_helpers.hpp>
#include <ninho/physics/physics_world.hpp>

#include <godot_cpp/classes/node3d.hpp>
#include <godot_cpp/variant/array.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/string.hpp>
#include <godot_cpp/variant/transform3d.hpp>
#include <godot_cpp/variant/vector3.hpp>

#include <cstdint>
#include <memory>
#include <string_view>

namespace ninho::extension {

class Box3DWorldNode : public godot::Node3D {
    GDCLASS(Box3DWorldNode, godot::Node3D)

public:
    Box3DWorldNode() = default;
    ~Box3DWorldNode() override = default;

    bool configure_planet(double radius, double surface_gravity);
    std::int64_t spawn_box(
        godot::Vector3 full_size, godot::Transform3D transform, double density);
    std::int64_t spawn_projectile(
        double radius, godot::Transform3D transform, godot::Vector3 velocity);
    bool apply_impulse(
        std::int64_t handle, godot::Vector3 impulse, godot::Vector3 world_point);
    bool step_fixed();
    godot::Array get_body_states();
    godot::Dictionary get_metrics();
    bool reset_world();
    void _physics_process(double delta) override;

protected:
    static void _bind_methods();

private:
    void emit_fault(std::string_view code, std::string_view message) noexcept;
    void emit_status_fault(std::string_view operation, const physics::Status& status) noexcept;
    void emit_exception_fault(std::string_view operation, const char* message) noexcept;
    [[nodiscard]] bool require_world(std::string_view operation) noexcept;
    [[nodiscard]] bool recreate_world(const physics::WorldConfig& config);

    std::unique_ptr<physics::PhysicsWorld> world_;
    physics::WorldConfig config_{};
    detail::FixedStepAccumulator accumulator_{};
};

}
