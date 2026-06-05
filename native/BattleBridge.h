#pragma once

#include "BattleSession.h"
#include "PlayerProfileSystem.h"
#include "RatingSystem.h"
#include "SimulationData.h"
#include "TrainerProfile.h"

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
    godot::Array start_battle(int player_style, int opponent_spec, int opponent_style);
    godot::Array use_skill(const godot::String& skill_id);
    godot::Array switch_player(int player_index);
    godot::Array change_style(int style);
    godot::Dictionary get_battle_state() const;
    godot::Array get_available_skills() const;
    godot::Dictionary create_trainer(const godot::String& trainer_name, int starter_spec);
    godot::Dictionary get_trainer_state() const;
    godot::Dictionary active_player_award_xp(int amount);
    godot::Dictionary active_player_learn_skill(const godot::String& skill_id);
    godot::Dictionary active_player_equip_skill(const godot::String& skill_id);
    godot::Dictionary active_player_unequip_skill(const godot::String& skill_id);
    godot::Dictionary trainer_award_rating(int amount);
    godot::Dictionary trainer_award_money(int amount);
    godot::Dictionary trainer_add_trophy(const godot::String& trophy_id);
    godot::Dictionary trainer_add_player(const godot::String& player_name, int spec);
    godot::Dictionary trainer_apply_match_result(int opponent_level, int context, bool won);

protected:
    static void _bind_methods();

private:
    godot::Array ToArray(const BattleActionResult& result) const;
    godot::Array ToArrayAndApplyRewards(const BattleActionResult& result);
    godot::Array Rejected(const godot::String& message) const;
    void AppendSkillUse(godot::Array& events, const SkillUseResult& skillUse) const;
    void AppendSkillXp(godot::Array& events, const SkillUseResult& skillUse) const;
    void AppendBattleReward(godot::Array& events, const BattleRewardResult& reward);
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
    godot::Dictionary ToDictionary(const PlayerProfileState& playerProfile) const;
    godot::Dictionary ToDictionary(const TrainerProfileState& profile) const;
    godot::Dictionary ToDictionary(const ProfileCommandResult& result) const;
    godot::Dictionary ToDictionary(const RatingResult& result) const;
    godot::Dictionary ToDictionary(const BattleRewardResult& result) const;
    godot::Array ToSkillArray(const std::vector<std::string>& skillIds) const;
    godot::Array ToStringArray(const std::vector<std::string>& values) const;
    bool IsValidMatchContext(int value) const;

    SimulationData data_;
    PlayerProfileSystem playerProfiles_;
    BattleSession session_;
    TrainerProfile profile_;
    RatingSystem rating_;
};
