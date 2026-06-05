#pragma once

#include <godot_cpp/classes/node.hpp>

// Empty Godot adapter for now. Gameplay systems live in ordinary C++ and will
// be covered by C++ tests before we expose a new production UI API.
class BattleBridge : public godot::Node
{
    GDCLASS(BattleBridge, godot::Node)

protected:
    static void _bind_methods();
};
