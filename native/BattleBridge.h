#pragma once

#include "BattleSession.h"
#include "PlayerProfile.h"
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
    godot::Dictionary create_profile(const godot::String& player_name, int spec);
    godot::Dictionary get_profile_state() const;
    godot::Dictionary profile_award_xp(int amount);
    godot::Dictionary profile_award_rating(int amount);
    godot::Dictionary profile_award_money(int amount);
    godot::Dictionary profile_learn_skill(const godot::String& skill_id);
    godot::Dictionary profile_equip_skill(const godot::String& skill_id);
    godot::Dictionary profile_unequip_skill(const godot::String& skill_id);
    godot::Dictionary profile_add_trophy(const godot::String& trophy_id);

protected:
    static void _bind_methods();

private:
    godot::Array ToArray(const BattleActionResult& result) const;
    godot::Array Rejected(const godot::String& message) const;
    void AppendSkillUse(godot::Array& events, const SkillUseResult& skillUse) const;
    void AppendSkillXp(godot::Array& events, const SkillUseResult& skillUse) const;
    godot::Dictionary CreateEvent(
        const char* type,
        BattleActor actor,
        BattleActor target,
        const std::string& skillId = "",
        const godot::String& message = "",
        int value = 0,
        int duration = 0) const;
    godot::Dictionary ToDictionary(const CompetitorView& competitor) const;
    godot::Dictionary ToDictionary(const SkillView& skill, int available_focus) const;
    godot::Dictionary ToDictionary(const PlayerProfileState& profile) const;
    godot::Dictionary ToDictionary(const ProfileCommandResult& result) const;
    godot::Array ToSkillArray(const std::vector<std::string>& skillIds) const;
    godot::Array ToStringArray(const std::vector<std::string>& values) const;

    SimulationData data_;
    BattleSession session_;
    PlayerProfile profile_;
};
