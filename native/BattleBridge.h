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
    godot::Dictionary start_demo_battle();
    godot::Dictionary use_skill(const godot::String& skill_id);
    godot::Dictionary switch_player(int player_index);
    godot::Dictionary create_player_profile(const godot::String& player_name, const godot::String& spec) const;
    godot::Dictionary award_player_profile_xp(const godot::Dictionary& player_profile, int amount) const;
    godot::Dictionary get_battle_state() const;
    godot::Array get_available_skills() const;
    godot::Dictionary get_last_result() const;

protected:
    static void _bind_methods();

private:
    godot::Dictionary competitor_to_dictionary(const CompetitorView& competitor) const;
    godot::Dictionary state_to_dictionary(const BattleState& state) const;
    godot::Dictionary skill_to_dictionary(const SkillView& skill) const;
    godot::Dictionary event_to_dictionary(const BattleEvent& event) const;
    godot::Dictionary result_to_dictionary(const BattleActionResult& result) const;
    godot::Dictionary reward_to_dictionary(const BattleRewardResult& reward) const;
    godot::Dictionary passive_bonuses_to_dictionary(const PassiveBonuses& bonuses) const;
    void add_trait_fields(godot::Dictionary& dictionary, const std::string& trait_id) const;
    godot::Dictionary player_profile_to_dictionary(const PlayerProfileState& player_profile) const;
    godot::Dictionary profile_result_to_dictionary(
        const ProfileCommandResult& result,
        const PlayerProfileState& player_profile) const;
    PlayerProfileState player_profile_from_dictionary(const godot::Dictionary& player_profile) const;
    BattleSetup setup_from_dictionary(const godot::Dictionary& setup) const;
    Spec spec_from_string(const godot::String& value) const;
    godot::String actor_to_string(BattleActor actor) const;
    godot::String winner_to_string(BattleWinner winner) const;
    godot::String event_type_to_string(BattleEventType type) const;
    godot::String error_to_string(SimulationError error) const;
    godot::String effect_to_string(SkillEffectType effect) const;

    SimulationData data_;
    std::unique_ptr<BattleSession> session_;
    BattleActionResult last_result_;
};
