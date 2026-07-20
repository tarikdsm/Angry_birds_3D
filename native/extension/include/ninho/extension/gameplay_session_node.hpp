#pragma once

#include <ninho/extension/session_adapter_services.hpp>

#include <godot_cpp/classes/node.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/string.hpp>
#include <godot_cpp/variant/vector3.hpp>

#include <memory>
#include <string_view>

namespace ninho::extension::detail {

#if defined(NINHO_ENABLE_TEST_FACADES)
class GameplaySessionAdapterTestFacade;
#endif

class GameplaySessionAdapter {
public:
    [[nodiscard]] bool configure(
        std::string_view materials_json,
        std::string_view archetypes_json,
        std::string_view level_json) noexcept;
    [[nodiscard]] bool queue_begin_grab(physics::Vec3 camera_right) noexcept;
    [[nodiscard]] bool queue_pull(double horizontal_m, double vertical_m) noexcept;
    [[nodiscard]] bool queue_release() noexcept;
    [[nodiscard]] bool queue_activate_ability() noexcept;
    [[nodiscard]] bool queue_cancel_grab() noexcept;
    [[nodiscard]] bool restart() noexcept;
    [[nodiscard]] bool advance(double delta) noexcept;
    [[nodiscard]] SessionFrameData consume_frame() noexcept;
    [[nodiscard]] const SessionFrameData& peek_frame() const noexcept;
    void acknowledge_frame() noexcept;

    [[nodiscard]] bool configured() const noexcept;
    [[nodiscard]] const std::optional<FaultInfo>& fault() const noexcept;
    [[nodiscard]] std::uint64_t fault_generation() const noexcept;
    void fail(std::string_view code, std::string_view message) noexcept;

private:
#if defined(NINHO_ENABLE_TEST_FACADES)
    friend class GameplaySessionAdapterTestFacade;
#endif

    [[nodiscard]] bool enqueue(simulation::PlayerCommand command) noexcept;
    [[nodiscard]] bool accept_status(const simulation::SessionStatus& status) noexcept;
    void capture_latest();
    void latch_content_error(const simulation::ContentError& error) noexcept;
    void latch_exception(std::string_view operation, const char* message) noexcept;

    std::unique_ptr<simulation::SimulationSession> session_;
    simulation::ContentBundle content_;
    SessionFixedStepAccumulator accumulator_;
    SessionFrameBatch batch_;
    std::uint64_t fault_generation_{};
};

}

namespace ninho::extension {

class GameplaySessionNode : public godot::Node {
    GDCLASS(GameplaySessionNode, godot::Node)

public:
    GameplaySessionNode();
    ~GameplaySessionNode() override = default;

    bool configure_session(
        godot::String materials_json,
        godot::String archetypes_json,
        godot::String level_json) noexcept;
    bool queue_begin_grab(godot::Vector3 camera_right) noexcept;
    bool queue_pull(double horizontal_m, double vertical_m) noexcept;
    bool queue_release() noexcept;
    bool queue_activate_ability() noexcept;
    bool queue_cancel_grab() noexcept;
    bool restart_level() noexcept;
    godot::Dictionary consume_frame() noexcept;
    void _physics_process(double delta) noexcept override;

protected:
    static void _bind_methods();

private:
    void emit_pending_fault() noexcept;
    void emit_exception_fault(std::string_view operation, const char* message) noexcept;

    detail::GameplaySessionAdapter adapter_;
    std::uint64_t reported_fault_generation_{};
};

}
