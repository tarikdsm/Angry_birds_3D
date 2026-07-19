#pragma once

#include <ninho/physics/physics_types.hpp>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <optional>
#include <span>
#include <string>
#include <variant>
#include <vector>

namespace ninho::physics {

namespace detail {
#if defined(NINHO_ENABLE_TEST_FACADES)
class PhysicsWorldTestFacade;
#endif
}

enum class StatusCode { Ok, InvalidArgument, InvalidHandle, CapacityExceeded, Unsupported, Box3DFault };

struct Status {
    StatusCode code{StatusCode::Ok};
    std::string message{};

    [[nodiscard]] bool ok() const { return code == StatusCode::Ok; }
};

template<class T>
struct Result {
    T value{};
    Status status{};

    [[nodiscard]] explicit operator bool() const { return status.ok(); }
};

enum class BodyType { Static, Kinematic, Dynamic };

struct SphereShape {
    float radius{};
    Transform local{};
};

struct BoxShape {
    Vec3 half_extents{};
    Transform local{};
};

struct CapsuleShape {
    float half_height{};
    float radius{};
    Transform local{};
};

struct HullShape {
    std::vector<Vec3> vertices;
    Transform local{};
};

using PrimitiveShape = std::variant<SphereShape, BoxShape, CapsuleShape, HullShape>;
using QueryShape = PrimitiveShape;

struct CompoundShape {
    std::vector<PrimitiveShape> children;
};

using ShapeGeometry = std::variant<SphereShape, BoxShape, CapsuleShape, HullShape, CompoundShape>;

struct ShapeDesc {
    ShapeGeometry geometry;
    float density{1.0f};
    float friction{0.5f};
    float restitution{};
    std::uint64_t material_id{};
    bool hit_events{true};
};

struct BodyDesc {
    BodyType type{BodyType::Static};
    Transform transform{};
    Vec3 linear_velocity{};
    Vec3 angular_velocity{};
    std::vector<ShapeDesc> shapes;
    bool bullet{};
    bool enable_sleep{true};
    bool radial_gravity{true};
    bool remove_beyond_six_r{true};
    std::string name;

    static BodyDesc static_sphere(float radius, Transform transform);
    static BodyDesc static_box(Vec3 half_extents, Transform transform);
    static BodyDesc dynamic_sphere(float radius, Transform transform, float density);
    static BodyDesc dynamic_box(Vec3 half_extents, Transform transform, float density);
};

struct WorldConfig {
    float time_step{1.0f / 60.0f};
    int substeps{4};
    float planet_radius{10.0f};
    float surface_gravity{9.0f};
    std::size_t max_bodies{500};
};

struct BodyState {
    BodyHandle handle{};
    Transform transform{};
    Vec3 linear_velocity{};
    Vec3 angular_velocity{};
    float mass{};
    bool awake{};
    bool ejected{};
};

struct DistanceJointDesc {
    BodyHandle a{};
    BodyHandle b{};
    Transform frame_a{};
    Transform frame_b{};
    float length{1.0f};
    float hertz{4.0f};
    float damping_ratio{1.0f};
    bool collide_connected{};
};

struct WeldJointDesc {
    BodyHandle a{};
    BodyHandle b{};
    Transform frame_a{};
    Transform frame_b{};
    float hertz{8.0f};
    float damping_ratio{1.0f};
    bool collide_connected{};
};

using JointDesc = std::variant<DistanceJointDesc, WeldJointDesc>;

struct QueryHit {
    BodyHandle body{};
    Vec3 point{};
    Vec3 normal{};
    float fraction{};
    std::uint64_t material_id{};
};

struct ContactHit {
    BodyHandle a{};
    BodyHandle b{};
    Vec3 point{};
    Vec3 normal{};
    float approach_speed{};
    float effective_mass{};
    float derived_energy{};
    std::uint64_t material_a{};
    std::uint64_t material_b{};
};

[[nodiscard]] inline float derived_contact_energy(
    float effective_mass, float approach_speed) noexcept
{
    const float damaging_speed = std::max(0.0f, approach_speed - 1.0f);
    return 0.5f * effective_mass * damaging_speed * damaging_speed;
}

[[nodiscard]] inline bool stronger_contact_for_pair(
    const ContactHit& candidate, const ContactHit& current) noexcept
{
    return candidate.a == current.a && candidate.b == current.b
        && candidate.approach_speed > current.approach_speed;
}

struct JointReaction {
    JointHandle joint{};
    Vec3 force{};
    Vec3 torque{};
    float linear_separation{};
    float angular_separation{};
};

struct Aabb {
    Vec3 lower{};
    Vec3 upper{};
};

struct WorldMetrics {
    int body_count{};
    int shape_count{};
    int joint_count{};
    int contact_count{};
    int awake_count{};
    double step_ms{};
};

class PhysicsWorld {
public:
    explicit PhysicsWorld(WorldConfig config);
    ~PhysicsWorld();

    PhysicsWorld(PhysicsWorld&&) noexcept;
    PhysicsWorld& operator=(PhysicsWorld&&) noexcept;
    PhysicsWorld(const PhysicsWorld&) = delete;
    PhysicsWorld& operator=(const PhysicsWorld&) = delete;

    Result<BodyHandle> create_body(const BodyDesc& desc);
    Status destroy_body(BodyHandle body);
    Result<JointHandle> create_joint(const JointDesc& desc);
    Status destroy_joint(JointHandle joint);
    Status apply_force(BodyHandle body, Vec3 force, Vec3 point, bool wake = true);
    Status apply_impulse(BodyHandle body, Vec3 impulse, Vec3 point, bool wake = true);
    Status commit_pending_initial_state();
    void step();
    [[nodiscard]] std::optional<BodyState> state(BodyHandle body) const;
    // Non-owning views of buffers published by this world. Any non-const
    // operation on the world, as well as moving or destroying it, may
    // invalidate a previously returned view. Copy elements that must outlive
    // that boundary.
    [[nodiscard]] std::span<const BodyState> states() const;
    [[nodiscard]] std::vector<QueryHit> overlap_shape(
        const QueryShape& shape, Transform transform) const;
    [[nodiscard]] std::optional<QueryHit> cast_shape(
        const QueryShape& shape, Transform transform, Vec3 translation) const;
    [[nodiscard]] std::vector<QueryHit> overlap_sphere(Vec3 center, float radius) const;
    [[nodiscard]] std::optional<QueryHit> cast_sphere(
        Vec3 center, float radius, Vec3 translation) const;
    [[nodiscard]] std::optional<Aabb> body_bounds(BodyHandle body) const;
    // Borrowed published view; it follows the invalidation rules above.
    [[nodiscard]] std::span<const ContactHit> contact_hits() const;
    // Borrowed published view; it follows the invalidation rules above.
    [[nodiscard]] std::span<const JointReaction> joint_reactions() const;
    [[nodiscard]] std::optional<JointReaction> joint_reaction(JointHandle joint) const;
    [[nodiscard]] WorldMetrics metrics() const;
    [[nodiscard]] const WorldConfig& config() const;

private:
#if defined(NINHO_ENABLE_TEST_FACADES)
    friend class detail::PhysicsWorldTestFacade;
#endif
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

}
