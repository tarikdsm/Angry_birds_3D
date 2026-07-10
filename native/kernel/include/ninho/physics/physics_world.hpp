#pragma once

#include <ninho/physics/physics_types.hpp>

#include <cstddef>
#include <cstdint>
#include <memory>
#include <optional>
#include <span>
#include <string>
#include <variant>
#include <vector>

namespace ninho::physics {

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
    Status apply_force(BodyHandle body, Vec3 force, Vec3 point, bool wake = true);
    Status apply_impulse(BodyHandle body, Vec3 impulse, Vec3 point, bool wake = true);
    void step();
    [[nodiscard]] std::optional<BodyState> state(BodyHandle body) const;
    [[nodiscard]] std::span<const BodyState> states() const;
    [[nodiscard]] const WorldConfig& config() const;

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

}
