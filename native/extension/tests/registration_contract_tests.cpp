#include "test_framework.hpp"

#include <ninho/extension/register_types.hpp>
#include <ninho/extension/box3d_world_node.hpp>
#include <ninho/extension/gameplay_session_node.hpp>
#include <ninho/extension/orbital_session_node.hpp>

#include <godot_cpp/classes/node.hpp>
#include <godot_cpp/classes/node3d.hpp>

#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>
#include <type_traits>
#include <utility>

namespace {

std::string read_registration_source()
{
    std::ifstream stream{
        std::filesystem::path{NINHO_SOURCE_DIR} / "native/extension/src/register_types.cpp",
        std::ios::binary};
    NINHO_REQUIRE(stream.is_open());
    std::ostringstream content;
    content << stream.rdbuf();
    return content.str();
}

NINHO_TEST("gameplay and orbital session classes remain registered together")
{
    const std::string source = read_registration_source();
    NINHO_REQUIRE(source.find("GDREGISTER_CLASS(GameplaySessionNode)") != std::string::npos);
    NINHO_REQUIRE(source.find("GDREGISTER_CLASS(OrbitalSessionNode)") != std::string::npos);
}

}

static_assert(noexcept(ninho::extension::initialize_ninho_module(
    godot::MODULE_INITIALIZATION_LEVEL_SCENE)));
static_assert(noexcept(ninho::extension::uninitialize_ninho_module(
    godot::MODULE_INITIALIZATION_LEVEL_SCENE)));
static_assert(noexcept(ninho_physics_library_init(nullptr, nullptr, nullptr)));
static_assert(std::is_base_of_v<godot::Node3D, ninho::extension::Box3DWorldNode>);
static_assert(std::is_base_of_v<godot::Node, ninho::extension::GameplaySessionNode>);
static_assert(std::is_base_of_v<godot::Node, ninho::extension::OrbitalSessionNode>);
static_assert(noexcept(
    std::declval<ninho::extension::GameplaySessionNode&>()._physics_process(0.0)));
static_assert(std::is_same_v<
              decltype(&ninho::extension::GameplaySessionNode::configure_session),
              bool (ninho::extension::GameplaySessionNode::*)(
                  godot::String, godot::String, godot::String) noexcept>);
static_assert(std::is_same_v<
              decltype(&ninho::extension::GameplaySessionNode::queue_begin_grab),
              bool (ninho::extension::GameplaySessionNode::*)(godot::Vector3) noexcept>);
static_assert(noexcept(std::declval<ninho::extension::GameplaySessionNode&>()
                           .queue_pull(0.0, 0.0)));
static_assert(noexcept(std::declval<ninho::extension::GameplaySessionNode&>()
                           .queue_release()));
static_assert(noexcept(std::declval<ninho::extension::GameplaySessionNode&>()
                           .queue_activate_ability()));
static_assert(noexcept(std::declval<ninho::extension::GameplaySessionNode&>()
                           .queue_cancel_grab()));
static_assert(noexcept(std::declval<ninho::extension::GameplaySessionNode&>()
                           .restart_level()));
static_assert(noexcept(std::declval<ninho::extension::GameplaySessionNode&>()
                           .consume_frame()));
static_assert(noexcept(
    std::declval<ninho::extension::OrbitalSessionNode&>()._physics_process(0.0)));
