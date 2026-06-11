#pragma once

#include "BattleSession.h"
#include "Models.h"
#include "SimulationData.h"

#include <godot_cpp/classes/node.hpp>
#include <godot_cpp/variant/array.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/string.hpp>

#include <memory>

// Thin Godot adapter for the first playable battle screen. Godot owns visuals
// and input; this bridge translates those calls into the C++ battle core.
class BattleBridge : public godot::Node
{
    GDCLASS(BattleBridge, godot::Node)

public:
    godot::Dictionary start_battle(const godot::Dictionary& setup);
    godot::Dictionary use_skill(const godot::String& skill_id, int target_player_index);
    godot::Dictionary use_drill(const godot::String& result_quality);
    godot::Dictionary pass_turn();
    godot::Dictionary get_battle_state() const;
    godot::Array get_available_skills() const;
    godot::Dictionary get_drill_action() const;
    godot::Dictionary get_last_result() const;

protected:
    static void _bind_methods();

private:
    BattleActionResult reject_action(SimulationError error_code, const std::string& error) const;
    BattleSetup setup_from_dictionary(const godot::Dictionary& setup) const;
    DrillResultQuality drill_quality_from_string(const godot::String& value) const;

    SimulationData data_;
    std::unique_ptr<BattleSession> session_;
    BattleActionResult last_result_;
};
