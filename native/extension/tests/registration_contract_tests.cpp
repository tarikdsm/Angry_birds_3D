#include <ninho/extension/register_types.hpp>

static_assert(noexcept(ninho::extension::initialize_ninho_module(
    godot::MODULE_INITIALIZATION_LEVEL_SCENE)));
static_assert(noexcept(ninho::extension::uninitialize_ninho_module(
    godot::MODULE_INITIALIZATION_LEVEL_SCENE)));
static_assert(noexcept(ninho_physics_library_init(nullptr, nullptr, nullptr)));
