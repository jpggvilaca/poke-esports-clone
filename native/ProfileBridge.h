#pragma once

#include "Models.h"
#include "SimulationData.h"

#include <godot_cpp/classes/node.hpp>
#include <godot_cpp/variant/array.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/string.hpp>

class TrainerProfile;

class ProfileBridge : public godot::Node
{
    GDCLASS(ProfileBridge, godot::Node)

public:
    godot::Dictionary create_player_profile(const godot::String& player_name, const godot::String& spec) const;
    godot::Dictionary get_player_skill_summary(
        const godot::Dictionary& player_profile,
        const godot::String& skill_id,
        const godot::Dictionary& progress) const;
    godot::Dictionary complete_trainer_battle(
        const godot::Dictionary& trainer_state,
        const godot::Dictionary& battle_config,
        const godot::Dictionary& battle_result) const;

protected:
    static void _bind_methods();

private:
    void apply_battle_vitals(TrainerProfile& trainer, godot::Array& roster, const godot::Dictionary& battle_state) const;
    void apply_skill_progress(
        godot::Array& roster,
        int active_player_index,
        const godot::Dictionary& battle_result,
        godot::Array& skill_progress_changes) const;
    void award_participant_xp(
        godot::Array& roster,
        const godot::Dictionary& battle_result,
        godot::Array& level_up_messages,
        godot::Array& battle_xp_awards) const;
    int get_active_player_level(const TrainerProfile& trainer, const godot::Array& roster) const;
    int apply_rating_change(
        TrainerProfile& trainer,
        const godot::Array& roster,
        const godot::Dictionary& battle_config,
        bool won) const;
    godot::String build_completion_summary(
        const godot::String& display_name,
        bool won,
        int money_reward,
        int rating_change) const;
    TrainerProfileState trainer_profile_from_dictionary(const godot::Dictionary& trainer_state) const;
    godot::Dictionary trainer_profile_to_dictionary(
        const TrainerProfileState& trainer_state,
        const godot::Array& roster) const;

    SimulationData data_;
};
