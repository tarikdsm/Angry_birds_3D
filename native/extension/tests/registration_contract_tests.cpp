#include <ninho/extension/register_types.hpp>
#include <ninho/extension/box3d_world_node.hpp>
#include <ninho/extension/orbital_session_node.hpp>

#include <godot_cpp/classes/node.hpp>
#include <godot_cpp/classes/node3d.hpp>

#include <type_traits>
#include <utility>

static_assert(noexcept(ninho::extension::initialize_ninho_module(
    godot::MODULE_INITIALIZATION_LEVEL_SCENE)));
static_assert(noexcept(ninho::extension::uninitialize_ninho_module(
    godot::MODULE_INITIALIZATION_LEVEL_SCENE)));
static_assert(noexcept(ninho_physics_library_init(nullptr, nullptr, nullptr)));
static_assert(std::is_base_of_v<godot::Node3D, ninho::extension::Box3DWorldNode>);
static_assert(std::is_base_of_v<godot::Node, ninho::extension::OrbitalSessionNode>);
static_assert(noexcept(
    std::declval<ninho::extension::OrbitalSessionNode&>()._physics_process(0.0)));
