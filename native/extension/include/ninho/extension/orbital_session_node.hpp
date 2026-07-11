#pragma once

#include <ninho/simulation/session.hpp>

#include <godot_cpp/classes/node.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/string.hpp>
#include <godot_cpp/variant/vector3.hpp>

#include <cstdint>
#include <array>
#include <memory>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace ninho::extension::detail {

struct SessionTickSchedule {
    bool ok{};
    int tick_count{};
    double discarded_seconds{};
};

class SessionFixedStepAccumulator {
public:
    [[nodiscard]] SessionTickSchedule schedule(double delta) noexcept;
    void reset() noexcept;

    static constexpr double time_step = 1.0 / 60.0;
    static constexpr int max_ticks_per_frame = 4;

private:
    double pending_seconds_{};
};

struct FaultInfo {
    FaultInfo() noexcept = default;
    FaultInfo(std::string_view code, std::string_view message) noexcept;
    FaultInfo(
        std::string_view code,
        std::string_view context,
        std::string_view message) noexcept;

    [[nodiscard]] std::string_view code() const noexcept;
    [[nodiscard]] std::string_view message() const noexcept;

    bool operator==(const FaultInfo&) const = default;

private:
    static constexpr std::size_t code_capacity = 63;
    static constexpr std::size_t message_capacity = 511;
    std::array<char, code_capacity + 1> code_storage_{};
    std::array<char, message_capacity + 1> message_storage_{};
    std::size_t code_size_{};
    std::size_t message_size_{};
};

struct SessionFrameData {
    simulation::SessionState state;
    int ticks_executed{};
    std::uint32_t birds_remaining{};
    std::vector<simulation::EntitySnapshot> snapshots;
    std::vector<simulation::DomainEvent> events;
    bool objectives_complete{};
    std::optional<simulation::TrajectoryPreview> preview;
    physics::WorldMetrics metrics;
    double discarded_time_seconds{};
};

class SessionFrameBatch {
public:
    void capture_tick(
        std::span<const simulation::DomainEvent> events,
        std::span<const simulation::EntitySnapshot> snapshots,
        const simulation::SessionState& state,
        std::uint32_t birds_remaining,
        bool objectives_complete,
        const physics::WorldMetrics& metrics,
        std::optional<simulation::TrajectoryPreview> preview = std::nullopt);
    void capture_latest(
        std::span<const simulation::EntitySnapshot> snapshots,
        const simulation::SessionState& state,
        std::uint32_t birds_remaining,
        bool objectives_complete,
        const physics::WorldMetrics& metrics);
    void set_preview(simulation::TrajectoryPreview preview);
    void add_discarded_time(double seconds) noexcept;
    [[nodiscard]] SessionFrameData consume();
    [[nodiscard]] const SessionFrameData& peek() const noexcept;
    void acknowledge() noexcept;
    void reset();

    void latch_fault(FaultInfo fault) noexcept;
    void clear_fault() noexcept;
    [[nodiscard]] const std::optional<FaultInfo>& fault() const noexcept;

private:
    SessionFrameData pending_;
    std::optional<FaultInfo> fault_;
};

void capture_session_tick(
    SessionFrameBatch& batch,
    const simulation::SimulationSession& session);

class OrbitalSessionAdapter {
public:
    [[nodiscard]] bool configure(
        std::string_view materials_json,
        std::string_view archetypes_json,
        std::string_view level_json) noexcept;
    [[nodiscard]] bool queue_begin_aim() noexcept;
    [[nodiscard]] bool queue_aim(
        physics::Vec3 origin,
        physics::Vec3 tangent_direction,
        double speed) noexcept;
    [[nodiscard]] bool queue_launch() noexcept;
    [[nodiscard]] bool queue_activate_ability() noexcept;
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

class OrbitalSessionNode : public godot::Node {
    GDCLASS(OrbitalSessionNode, godot::Node)

public:
    OrbitalSessionNode();
    ~OrbitalSessionNode() override = default;

    bool configure_session(
        godot::String materials_json,
        godot::String archetypes_json,
        godot::String level_json) noexcept;
    bool queue_begin_aim() noexcept;
    bool queue_aim(
        godot::Vector3 origin,
        godot::Vector3 tangent_direction,
        double speed) noexcept;
    bool queue_launch() noexcept;
    bool queue_activate_ability() noexcept;
    bool restart_level() noexcept;
    godot::Dictionary consume_frame() noexcept;
    void _physics_process(double delta) noexcept override;

protected:
    static void _bind_methods();

private:
    void emit_pending_fault() noexcept;
    void emit_exception_fault(std::string_view operation, const char* message) noexcept;

    detail::OrbitalSessionAdapter adapter_;
    std::uint64_t reported_fault_generation_{};
};

}
