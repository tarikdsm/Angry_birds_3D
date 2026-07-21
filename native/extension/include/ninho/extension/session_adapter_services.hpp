#pragma once

#include <ninho/simulation/session.hpp>

#include <array>
#include <cstdint>
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

[[nodiscard]] std::string_view domain_event_kind_name(
    simulation::DomainEventKind) noexcept;

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

struct AimEnvelope {
    double shell_radius_m{};
    double theta_min_deg{};
    double theta_max_deg{};
    double speed_min_m_s{};
    double speed_max_m_s{};
    double default_speed_m_s{};

    bool operator==(const AimEnvelope&) const = default;
};

struct LockedPlaneFrameData {
    physics::Vec3 camera_right{};
    physics::Vec3 up{};
    physics::Vec3 horizontal{};
    physics::Vec3 plane_normal{};

    bool operator==(const LockedPlaneFrameData&) const = default;
};

struct ShotFrameData {
    std::uint64_t shot_id{};
    simulation::BirdArchetypeId bird_archetype_id{};
    simulation::AbilityId ability_id{};
    simulation::TickIndex launch_tick{};
    double pull_horizontal_m{};
    double pull_vertical_m{};
    bool activation_consumed{};
    std::vector<simulation::EntityId> projectile_ids;

    bool operator==(const ShotFrameData&) const = default;
};

struct GameplayFrameFields {
    std::vector<simulation::BirdArchetypeId> bird_queue;
    std::optional<simulation::BirdArchetypeId> current_bird;
    std::optional<LockedPlaneFrameData> locked_plane;
    std::optional<ShotFrameData> shot;
    std::vector<simulation::EntitySnapshot> projectiles;
    std::uint64_t score{};
    std::uint32_t stars{};
    std::string gravity_kind;
    physics::Vec3 local_gravity_m_s2{};
};

struct SessionFrameData {
    simulation::SessionState state;
    int ticks_executed{};
    std::uint32_t birds_remaining{};
    std::vector<simulation::EntitySnapshot> snapshots;
    std::vector<simulation::DomainEvent> events;
    bool objectives_complete{};
    std::vector<simulation::ObjectiveTargetStatus> objective_targets;
    simulation::AbilityReadiness ability_readiness{
        simulation::AbilityReadiness::Unavailable};
    std::optional<simulation::TrajectoryPreview> preview;
    AimEnvelope aim_envelope;
    std::vector<simulation::BirdArchetypeId> bird_queue;
    std::optional<simulation::BirdArchetypeId> current_bird;
    std::optional<LockedPlaneFrameData> locked_plane;
    std::optional<ShotFrameData> shot;
    std::vector<simulation::EntitySnapshot> projectiles;
    std::uint64_t score{};
    std::uint32_t stars{};
    std::string gravity_kind;
    physics::Vec3 local_gravity_m_s2{};
    physics::WorldMetrics metrics;
    double discarded_time_seconds{};
};

class SessionFrameBatch {
public:
    void capture_events(std::span<const simulation::DomainEvent> events);
    void capture_latest(
        std::span<const simulation::EntitySnapshot> snapshots,
        const simulation::SessionState& state,
        std::uint32_t birds_remaining,
        bool objectives_complete,
        const physics::WorldMetrics& metrics,
        std::span<const simulation::ObjectiveTargetStatus> objective_targets = {},
        simulation::AbilityReadiness ability_readiness =
            simulation::AbilityReadiness::Unavailable);
    void set_preview(simulation::TrajectoryPreview preview);
    void clear_preview() noexcept;
    void set_aim_envelope(AimEnvelope envelope) noexcept;
    void set_gameplay_fields(GameplayFrameFields fields);
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

void capture_session_events(
    SessionFrameBatch& batch,
    const simulation::SimulationSession& session);

}
