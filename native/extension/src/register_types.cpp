#include <ninho/extension/box3d_world_node.hpp>
#include <ninho/extension/gameplay_session_node.hpp>
#include <ninho/extension/orbital_session_node.hpp>
#include <ninho/extension/register_types.hpp>

#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/core/defs.hpp>
#include <godot_cpp/godot.hpp>

#include <cstdio>
#include <exception>

namespace {

void log_registration_failure(const char* stage, const char* message) noexcept
{
    std::fputs("ninho_physics: ", stderr);
    std::fputs(stage, stderr);
    std::fputs(": ", stderr);
    std::fputs(message, stderr);
    std::fputc('\n', stderr);
}

}

namespace ninho::extension {

void initialize_ninho_module(godot::ModuleInitializationLevel level) noexcept
{
    try {
        if (level != godot::MODULE_INITIALIZATION_LEVEL_SCENE) {
            return;
        }
        GDREGISTER_CLASS(Box3DWorldNode);
        GDREGISTER_CLASS(GameplaySessionNode);
        GDREGISTER_CLASS(OrbitalSessionNode);
    } catch (const std::exception& error) {
        log_registration_failure("initializer", error.what());
    } catch (...) {
        log_registration_failure("initializer", "unknown exception");
    }
}

void uninitialize_ninho_module(godot::ModuleInitializationLevel level) noexcept
{
    try {
        if (level != godot::MODULE_INITIALIZATION_LEVEL_SCENE) {
            return;
        }
    } catch (const std::exception& error) {
        log_registration_failure("terminator", error.what());
    } catch (...) {
        log_registration_failure("terminator", "unknown exception");
    }
}

}

extern "C" {

GDExtensionBool GDE_EXPORT ninho_physics_library_init(
    GDExtensionInterfaceGetProcAddress get_proc_address,
    GDExtensionClassLibraryPtr library,
    GDExtensionInitialization* initialization) noexcept
{
    try {
        godot::GDExtensionBinding::InitObject init{
            get_proc_address, library, initialization};
        init.register_initializer(ninho::extension::initialize_ninho_module);
        init.register_terminator(ninho::extension::uninitialize_ninho_module);
        init.set_minimum_library_initialization_level(
            godot::MODULE_INITIALIZATION_LEVEL_SCENE);
        return init.init();
    } catch (const std::exception& error) {
        log_registration_failure("entry", error.what());
    } catch (...) {
        log_registration_failure("entry", "unknown exception");
    }
    return false;
}

}
