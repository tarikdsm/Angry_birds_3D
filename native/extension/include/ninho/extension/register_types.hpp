#pragma once

#include <gdextension_interface.h>

#include <godot_cpp/core/defs.hpp>
#include <godot_cpp/godot.hpp>

namespace ninho::extension {

void initialize_ninho_module(godot::ModuleInitializationLevel level) noexcept;
void uninitialize_ninho_module(godot::ModuleInitializationLevel level) noexcept;

}

extern "C" GDExtensionBool GDE_EXPORT ninho_physics_library_init(
    GDExtensionInterfaceGetProcAddress get_proc_address,
    GDExtensionClassLibraryPtr library,
    GDExtensionInitialization* initialization) noexcept;
