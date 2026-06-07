#pragma once

#include "BattleSession.h"
#include "Models.h"
#include "ScoutSystem.h"
#include "SimulationData.h"

#include <godot_cpp/classes/node.hpp>
#include <godot_cpp/variant/array.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/string.hpp>

#include <memory>
#include <vector>

class TrainerProfile;

// Thin Godot adapter for the first playable battle screen. Godot owns visuals
// and input; this bridge translates those calls into the C++ battle core.
class BattleBridge : public godot::Node
{
    GDCLASS(BattleBridge, godot::Node)

public:
    godot::Dictionary start_battle(const godot::Dictionary& setup);
    godot::Dictionary use_skill(const godot::String& skill_id);
    godot::Dictionary use_drill(const godot::String& result_quality);
    godot::Dictionary switch_player(int player_index);
    godot::Dictionary create_player_profile(const godot::String& player_name, const godot::String& spec) const;
    godot::Dictionary get_player_skill_summary(
        const godot::Dictionary& player_profile,
        const godot::String& skill_id,
        const godot::Dictionary& progress) const;
    godot::Dictionary get_pending_scout_offer(
        int rating,
        const godot::Array& completed_offer_ids,
        const godot::Array& declined_offer_ids) const;
    godot::Dictionary accept_scout_candidate(
        const godot::Array& roster,
        const godot::Dictionary& pending_offer,
        int candidate_index) const;
    godot::Dictionary complete_trainer_battle(
        const godot::Dictionary& trainer_state,
        const godot::Dictionary& battle_config,
        const godot::Dictionary& battle_result) const;
    godot::Dictionary get_battle_state() const;
    godot::Array get_available_skills() const;
    godot::Dictionary get_drill_action() const;
    godot::Dictionary get_last_result() const;

protected:
    static void _bind_methods();

private:
    BattleActionResult reject_action(SimulationError error_code, const std::string& error) const;
    void apply_battle_vitals(TrainerProfile& trainer, godot::Array& roster, const godot::Dictionary& battle_state) const;
    void apply_skill_progress(godot::Array& roster, int active_player_index, const godot::Dictionary& battle_result) const;
    void award_participant_xp(
        godot::Array& roster,
        const godot::Dictionary& battle_result,
        godot::Array& level_up_messages) const;
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
    Competitor competitor_from_player_profile(const godot::Dictionary& player_profile) const;
    std::vector<std::string> string_vector_from_array(const godot::Array& values) const;
    godot::Dictionary scout_offer_to_dictionary(const ScoutOfferView& offer) const;
    ScoutOfferView scout_offer_from_dictionary(const godot::Dictionary& offer_dictionary) const;
    godot::Dictionary player_profile_to_roster_dictionary(const PlayerProfileState& player_profile) const;
    godot::Dictionary competitor_to_dictionary(const CompetitorView& competitor) const;
    godot::Dictionary state_to_dictionary(const BattleState& state) const;
    godot::Dictionary skill_to_dictionary(const SkillView& skill) const;
    godot::Dictionary drill_to_dictionary(const DrillView& drill) const;
    godot::Dictionary event_to_dictionary(const BattleEvent& event) const;
    godot::Dictionary result_to_dictionary(const BattleActionResult& result) const;
    godot::Dictionary reward_to_dictionary(const BattleRewardResult& reward) const;
    godot::Dictionary passive_bonuses_to_dictionary(const PassiveBonuses& bonuses) const;
    void add_trait_fields(godot::Dictionary& dictionary, const std::string& trait_id) const;
    godot::Dictionary player_profile_to_dictionary(const PlayerProfileState& player_profile) const;
    PlayerProfileState player_profile_from_dictionary(const godot::Dictionary& player_profile) const;
    TrainerProfileState trainer_profile_from_dictionary(const godot::Dictionary& trainer_state) const;
    godot::Dictionary trainer_profile_to_dictionary(
        const TrainerProfileState& trainer_state,
        const godot::Array& roster) const;
    BattleSetup setup_from_dictionary(const godot::Dictionary& setup) const;
    Spec spec_from_string(const godot::String& value) const;
    MatchContext match_context_from_string(const godot::String& value) const;
    godot::String actor_to_string(BattleActor actor) const;
    godot::String winner_to_string(BattleWinner winner) const;
    godot::String event_type_to_string(BattleEventType type) const;
    godot::String error_to_string(SimulationError error) const;
    godot::String effect_to_string(SkillEffectType effect) const;
    DrillResultQuality drill_quality_from_string(const godot::String& value) const;
    godot::String drill_quality_to_string(DrillResultQuality quality) const;

    SimulationData data_;
    std::unique_ptr<BattleSession> session_;
    BattleActionResult last_result_;
};
