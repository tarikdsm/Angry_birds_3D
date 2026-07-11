#include "test_framework.hpp"

#include <ninho/extension/adapter_helpers.hpp>

#include <godot_cpp/variant/basis.hpp>

#include <bit>
#include <cfloat>
#include <cstdint>
#include <limits>
#include <variant>

using namespace ninho::extension::detail;
using ninho::physics::BodyHandle;
using ninho::physics::Quat;
using ninho::physics::Transform;
using ninho::physics::Vec3;

NINHO_TEST("adapter converts vectors component for component")
{
    const godot::Vector3 godot_value{1.25, -2.5, 3.75};
    const auto converted = to_kernel_checked(godot_value);
    NINHO_REQUIRE(converted.has_value());
    const Vec3 kernel_value = *converted;
    NINHO_REQUIRE_NEAR(kernel_value.x, 1.25, 1e-6);
    NINHO_REQUIRE_NEAR(kernel_value.y, -2.5, 1e-6);
    NINHO_REQUIRE_NEAR(kernel_value.z, 3.75, 1e-6);

    const godot::Vector3 round_trip = to_godot(kernel_value);
    NINHO_REQUIRE_NEAR(round_trip.x, godot_value.x, 1e-6);
    NINHO_REQUIRE_NEAR(round_trip.y, godot_value.y, 1e-6);
    NINHO_REQUIRE_NEAR(round_trip.z, godot_value.z, 1e-6);
}

NINHO_TEST("adapter converts quaternions in x y z w order")
{
    const godot::Quaternion godot_value{0.1F, 0.2F, 0.3F, 0.9F};
    const auto converted = to_kernel_checked(godot_value);
    NINHO_REQUIRE(converted.has_value());
    const Quat kernel_value = *converted;
    NINHO_REQUIRE_NEAR(kernel_value.x, 0.1, 1e-6);
    NINHO_REQUIRE_NEAR(kernel_value.y, 0.2, 1e-6);
    NINHO_REQUIRE_NEAR(kernel_value.z, 0.3, 1e-6);
    NINHO_REQUIRE_NEAR(kernel_value.w, 0.9, 1e-6);

    const godot::Quaternion round_trip = to_godot(kernel_value);
    NINHO_REQUIRE_NEAR(round_trip.x, godot_value.x, 1e-6);
    NINHO_REQUIRE_NEAR(round_trip.y, godot_value.y, 1e-6);
    NINHO_REQUIRE_NEAR(round_trip.z, godot_value.z, 1e-6);
    NINHO_REQUIRE_NEAR(round_trip.w, godot_value.w, 1e-6);
}

NINHO_TEST("adapter converts transforms through quaternion basis")
{
    const godot::Quaternion rotation{0.0, 0.0, 0.0, 1.0};
    const godot::Transform3D godot_value{godot::Basis{rotation}, {4.0, 5.0, 6.0}};
    const auto converted = to_kernel_checked(godot_value);
    NINHO_REQUIRE(converted.has_value());
    const Transform kernel_value = *converted;
    NINHO_REQUIRE(
        (kernel_value == Transform{{4.0F, 5.0F, 6.0F}, {0.0F, 0.0F, 0.0F, 1.0F}}));

    const godot::Transform3D round_trip = to_godot(kernel_value);
    NINHO_REQUIRE_NEAR(round_trip.origin.x, 4.0, 1e-6);
    NINHO_REQUIRE_NEAR(round_trip.origin.y, 5.0, 1e-6);
    NINHO_REQUIRE_NEAR(round_trip.origin.z, 6.0, 1e-6);
    NINHO_REQUIRE_NEAR(round_trip.basis.get_quaternion().w, 1.0, 1e-6);
}

NINHO_TEST("adapter packs generation before index and rejects invalid handles")
{
    NINHO_REQUIRE(pack_handle({0, 1}) == 0);
    NINHO_REQUIRE(pack_handle({1, 0}) == 0);
    NINHO_REQUIRE(!unpack_handle(0).valid());

    const BodyHandle handle{0x89ABCDEFu, 0xFEDCBA98u};
    const std::int64_t packed = pack_handle(handle);
    NINHO_REQUIRE(std::bit_cast<std::uint64_t>(packed) == 0xFEDCBA9889ABCDEFull);
    NINHO_REQUIRE(unpack_handle(packed) == handle);
}

NINHO_TEST("fixed scheduler clamps backlog and caps each frame at four ticks")
{
    FixedStepAccumulator scheduler;
    const TickSchedule first = scheduler.schedule(1.0, 1.0 / 60.0);
    NINHO_REQUIRE(first.ok);
    NINHO_REQUIRE(first.tick_count == 4);

    const TickSchedule second = scheduler.schedule(0.0, 1.0 / 60.0);
    NINHO_REQUIRE(second.ok);
    NINHO_REQUIRE(second.tick_count == 2);
}

NINHO_TEST("fixed scheduler rejects invalid delta and reset clears backlog")
{
    FixedStepAccumulator scheduler;
    NINHO_REQUIRE(!scheduler.schedule(-0.01, 1.0 / 60.0).ok);
    NINHO_REQUIRE(!scheduler.schedule(std::numeric_limits<double>::infinity(), 1.0 / 60.0).ok);
    NINHO_REQUIRE(!scheduler.schedule(0.01, 0.0).ok);

    NINHO_REQUIRE(scheduler.schedule(0.1, 1.0 / 60.0).tick_count == 4);
    scheduler.reset();
    NINHO_REQUIRE(scheduler.schedule(0.0, 1.0 / 60.0).tick_count == 0);
}

NINHO_TEST("adapter builds planet configuration and static sphere")
{
    const ninho::physics::WorldConfig config = make_world_config(25.0F, 12.0F);
    NINHO_REQUIRE_NEAR(config.planet_radius, 25.0, 1e-6);
    NINHO_REQUIRE_NEAR(config.surface_gravity, 12.0, 1e-6);

    const ninho::physics::BodyDesc planet = make_planet_desc(config.planet_radius);
    NINHO_REQUIRE(planet.type == ninho::physics::BodyType::Static);
    const auto& sphere = std::get<ninho::physics::SphereShape>(planet.shapes.front().geometry);
    NINHO_REQUIRE_NEAR(sphere.radius, 25.0, 1e-6);
}

NINHO_TEST("adapter converts full box size to half extents")
{
    const godot::Transform3D transform{godot::Basis{}, {1.0, 2.0, 3.0}};
    const auto kernel_transform = to_kernel_checked(transform);
    const auto kernel_size = to_kernel_checked(godot::Vector3{2.0, 4.0, 6.0});
    NINHO_REQUIRE(kernel_transform.has_value());
    NINHO_REQUIRE(kernel_size.has_value());
    const ninho::physics::BodyDesc box =
        make_box_desc(*kernel_size, *kernel_transform, 7.5F);
    NINHO_REQUIRE(box.type == ninho::physics::BodyType::Dynamic);
    const auto& shape = std::get<ninho::physics::BoxShape>(box.shapes.front().geometry);
    NINHO_REQUIRE((shape.half_extents == Vec3{1.0F, 2.0F, 3.0F}));
    NINHO_REQUIRE_NEAR(box.shapes.front().density, 7.5, 1e-6);
}

NINHO_TEST("adapter marks projectiles as bullets and preserves velocity")
{
    const auto kernel_transform = to_kernel_checked(godot::Transform3D{});
    const auto kernel_velocity =
        to_kernel_checked(godot::Vector3{10.0F, -2.0F, 3.0F});
    NINHO_REQUIRE(kernel_transform.has_value());
    NINHO_REQUIRE(kernel_velocity.has_value());
    const ninho::physics::BodyDesc projectile = make_projectile_desc(
        0.75F, *kernel_transform, *kernel_velocity);
    NINHO_REQUIRE(projectile.bullet);
    NINHO_REQUIRE((projectile.linear_velocity == Vec3{10.0F, -2.0F, 3.0F}));
    const auto& sphere =
        std::get<ninho::physics::SphereShape>(projectile.shapes.front().geometry);
    NINHO_REQUIRE_NEAR(sphere.radius, 0.75, 1e-6);
    NINHO_REQUIRE_NEAR(projectile.shapes.front().density, 1.0, 1e-6);
}

NINHO_TEST("adapter rejects zero and stale packed handles without losing generation")
{
    ninho::physics::PhysicsWorld world(ninho::physics::WorldConfig{});
    const ninho::physics::BodyDesc body = ninho::physics::BodyDesc::dynamic_box(
        {0.5F, 0.5F, 0.5F}, {{0.0F, 15.0F, 0.0F}, {}}, 10.0F);
    const BodyHandle old = world.create_body(body).value;
    world.step();
    NINHO_REQUIRE(world.destroy_body(old).ok());
    world.step();
    const BodyHandle replacement = world.create_body(body).value;
    NINHO_REQUIRE(old.index == replacement.index);
    NINHO_REQUIRE(old.generation != replacement.generation);

    const ninho::physics::Status zero = apply_impulse_checked(
        world, 0, godot::Vector3{1.0F, 0.0F, 0.0F}, godot::Vector3{});
    NINHO_REQUIRE(zero.code == ninho::physics::StatusCode::InvalidHandle);
    const ninho::physics::Status stale = apply_impulse_checked(
        world, pack_handle(old), godot::Vector3{1.0F, 0.0F, 0.0F}, godot::Vector3{});
    NINHO_REQUIRE(stale.code == ninho::physics::StatusCode::InvalidHandle);
}

NINHO_TEST("adapter accepts only positive doubles representable as positive finite floats")
{
    NINHO_REQUIRE(!checked_positive_float(0.0).has_value());
    NINHO_REQUIRE(!checked_positive_float(-1.0).has_value());
    NINHO_REQUIRE(!checked_positive_float(std::numeric_limits<double>::quiet_NaN()).has_value());
    NINHO_REQUIRE(!checked_positive_float(std::numeric_limits<double>::infinity()).has_value());
    NINHO_REQUIRE(!checked_positive_float(DBL_MAX).has_value());
    NINHO_REQUIRE(!checked_positive_float(std::numeric_limits<double>::denorm_min()).has_value());

    const auto minimum = checked_positive_float(std::numeric_limits<float>::min());
    NINHO_REQUIRE(minimum.has_value());
    NINHO_REQUIRE(*minimum == std::numeric_limits<float>::min());
    const auto subnormal = checked_positive_float(std::numeric_limits<float>::denorm_min());
    NINHO_REQUIRE(subnormal.has_value());
    NINHO_REQUIRE(*subnormal == std::numeric_limits<float>::denorm_min());
    const auto maximum = checked_positive_float(std::numeric_limits<float>::max());
    NINHO_REQUIRE(maximum.has_value());
    NINHO_REQUIRE(*maximum == std::numeric_limits<float>::max());
}

NINHO_TEST("adapter accepts rigid identity and quarter turn transforms")
{
    const auto identity = to_kernel_checked(godot::Transform3D{});
    NINHO_REQUIRE(identity.has_value());
    NINHO_REQUIRE(identity->rotation == Quat{});

    constexpr godot::real_t half_sqrt_two = static_cast<godot::real_t>(0.7071067811865476);
    const godot::Quaternion quarter_turn{0.0F, 0.0F, half_sqrt_two, half_sqrt_two};
    const godot::Transform3D rotated{godot::Basis{quarter_turn}, {1.0F, 2.0F, 3.0F}};
    const auto kernel = to_kernel_checked(rotated);
    NINHO_REQUIRE(kernel.has_value());
    const godot::Transform3D round_trip = to_godot(*kernel);
    NINHO_REQUIRE_NEAR(round_trip.basis.get_column(0).length(), 1.0, 1e-5);
    NINHO_REQUIRE_NEAR(round_trip.basis.get_column(1).length(), 1.0, 1e-5);
    NINHO_REQUIRE_NEAR(round_trip.basis.determinant(), 1.0, 1e-5);
}

NINHO_TEST("adapter rejects scale shear reflection degenerate and nonfinite bases")
{
    const godot::Vector3 x{1.0F, 0.0F, 0.0F};
    const godot::Vector3 y{0.0F, 1.0F, 0.0F};
    const godot::Vector3 z{0.0F, 0.0F, 1.0F};
    NINHO_REQUIRE(!to_kernel_checked(godot::Transform3D{
        godot::Basis{{2.0F, 0.0F, 0.0F}, y, z}, {}}).has_value());
    NINHO_REQUIRE(!to_kernel_checked(godot::Transform3D{
        godot::Basis{x, {0.0F, 0.5F, 0.0F}, z}, {}}).has_value());
    NINHO_REQUIRE(!to_kernel_checked(godot::Transform3D{
        godot::Basis{x, {0.5F, 1.0F, 0.0F}, z}, {}}).has_value());
    NINHO_REQUIRE(!to_kernel_checked(godot::Transform3D{
        godot::Basis{{-1.0F, 0.0F, 0.0F}, y, z}, {}}).has_value());
    NINHO_REQUIRE(!to_kernel_checked(godot::Transform3D{
        godot::Basis{{0.0F, 0.0F, 0.0F}, y, z}, {}}).has_value());
    NINHO_REQUIRE(!to_kernel_checked(godot::Transform3D{
        godot::Basis{{std::numeric_limits<godot::real_t>::infinity(), 0.0F, 0.0F}, y, z},
        {}}).has_value());
}
