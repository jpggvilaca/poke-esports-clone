#pragma once

#include "BattleSession.h"
#include "SimulationData.h"

#include <godot_cpp/classes/node.hpp>
#include <godot_cpp/variant/array.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/string.hpp>

// This is the narrow adapter between Godot and ordinary C++ combat code.
// Keep Godot-specific Array and Dictionary types here instead of spreading them
// through BattleSession.
class BattleBridge : public godot::Node
{
    GDCLASS(BattleBridge, godot::Node)

public:
    BattleBridge();

    godot::Array get_specs() const;
    godot::Array get_styles() const;
    godot::Array start_battle(int player_spec, int player_style, int opponent_spec, int opponent_style);
    godot::Array use_skill(const godot::String& skill_id);
    godot::Array change_style(int style);
    godot::Dictionary get_battle_state() const;
    godot::Array get_available_skills() const;

protected:
    static void _bind_methods();

private:
    godot::Array ToArray(const BattleActionResult& result) const;
    godot::Array Rejected(const godot::String& message) const;
    godot::Dictionary ToDictionary(const BattleEvent& event) const;
    godot::Dictionary ToDictionary(const CompetitorView& competitor) const;
    godot::Dictionary ToDictionary(const SkillView& skill, int available_focus) const;

    SimulationData data_;
    BattleSession session_;
};
