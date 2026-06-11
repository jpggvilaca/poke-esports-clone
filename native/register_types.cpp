#include "register_types.h"

#include "BattleBridge.h"
#include "ProfileBridge.h"
#include "ScoutBridge.h"

#include <gdextension_interface.h>
#include <godot_cpp/godot.hpp>

void initialize_poke_esports_module(godot::ModuleInitializationLevel level)
{
    if (level != godot::MODULE_INITIALIZATION_LEVEL_SCENE)
    {
        return;
    }

    GDREGISTER_CLASS(BattleBridge);
    GDREGISTER_CLASS(ProfileBridge);
    GDREGISTER_CLASS(ScoutBridge);
}

void uninitialize_poke_esports_module(godot::ModuleInitializationLevel level)
{
    if (level != godot::MODULE_INITIALIZATION_LEVEL_SCENE)
    {
        return;
    }
}

extern "C"
{
    GDExtensionBool GDE_EXPORT poke_esports_library_init(
        GDExtensionInterfaceGetProcAddress get_proc_address,
        GDExtensionClassLibraryPtr library,
        GDExtensionInitialization* initialization)
    {
        godot::GDExtensionBinding::InitObject initObject(get_proc_address, library, initialization);
        initObject.register_initializer(initialize_poke_esports_module);
        initObject.register_terminator(uninitialize_poke_esports_module);
        initObject.set_minimum_library_initialization_level(godot::MODULE_INITIALIZATION_LEVEL_SCENE);
        return initObject.init();
    }
}
