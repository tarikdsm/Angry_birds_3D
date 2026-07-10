#include <ninho/extension/box3d_world_node.hpp>

#include <gdextension_interface.h>

#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/core/defs.hpp>
#include <godot_cpp/godot.hpp>

namespace {

void initialize_ninho_physics(godot::ModuleInitializationLevel level)
{
    if (level != godot::MODULE_INITIALIZATION_LEVEL_SCENE) {
        return;
    }
    GDREGISTER_CLASS(ninho::extension::Box3DWorldNode);
}

void uninitialize_ninho_physics(godot::ModuleInitializationLevel level)
{
    if (level != godot::MODULE_INITIALIZATION_LEVEL_SCENE) {
        return;
    }
}

}

extern "C" {

GDExtensionBool GDE_EXPORT ninho_physics_library_init(
    GDExtensionInterfaceGetProcAddress get_proc_address,
    GDExtensionClassLibraryPtr library,
    GDExtensionInitialization* initialization)
{
    godot::GDExtensionBinding::InitObject init{
        get_proc_address, library, initialization};
    init.register_initializer(initialize_ninho_physics);
    init.register_terminator(uninitialize_ninho_physics);
    init.set_minimum_library_initialization_level(
        godot::MODULE_INITIALIZATION_LEVEL_SCENE);
    return init.init();
}

}
