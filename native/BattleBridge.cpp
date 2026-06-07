#include "BattleBridge.h"

#include "PlayerProfileSystem.h"
#include "RatingSystem.h"
#include "TrainerProfile.h"

#include <godot_cpp/core/class_db.hpp>

#include <algorithm>
#include <cstdint>
#include <string>

using godot::Array;
using godot::Dictionary;
using godot::String;

void BattleBridge::_bind_methods()
{
    godot::ClassDB::bind_method(godot::D_METHOD("start_battle", "setup"), &BattleBridge::start_battle);
    godot::ClassDB::bind_method(godot::D_METHOD("use_skill", "skill_id"), &BattleBridge::use_skill);
    godot::ClassDB::bind_method(godot::D_METHOD("switch_player", "player_index"), &BattleBridge::switch_player);
    godot::ClassDB::bind_method(godot::D_METHOD("create_player_profile", "player_name", "spec"), &BattleBridge::create_player_profile);
    godot::ClassDB::bind_method(godot::D_METHOD("get_player_skill_summary", "player_profile", "skill_id", "progress"), &BattleBridge::get_player_skill_summary);
    godot::ClassDB::bind_method(godot::D_METHOD("get_pending_scout_offer", "rating", "completed_offer_ids", "declined_offer_ids"), &BattleBridge::get_pending_scout_offer);
    godot::ClassDB::bind_method(godot::D_METHOD("accept_scout_candidate", "roster", "pending_offer", "candidate_index"), &BattleBridge::accept_scout_candidate);
    godot::ClassDB::bind_method(godot::D_METHOD("complete_trainer_battle", "trainer_state", "battle_config", "battle_result"), &BattleBridge::complete_trainer_battle);
    godot::ClassDB::bind_method(godot::D_METHOD("get_battle_state"), &BattleBridge::get_battle_state);
    godot::ClassDB::bind_method(godot::D_METHOD("get_available_skills"), &BattleBridge::get_available_skills);
    godot::ClassDB::bind_method(godot::D_METHOD("get_last_result"), &BattleBridge::get_last_result);
}

Dictionary BattleBridge::start_battle(const Dictionary& setup_dictionary)
{
    const std::uint32_t seed = setup_dictionary.has("seed")
        ? static_cast<std::uint32_t>(static_cast<int>(setup_dictionary["seed"]))
        : 20260606;

    session_ = std::make_unique<BattleSession>(data_, seed);
    BattleSetup setup = setup_from_dictionary(setup_dictionary);
    last_result_ = session_->StartBattle(setup);
    return result_to_dictionary(last_result_);
}

Dictionary BattleBridge::use_skill(const String& skill_id)
{
    if (session_ == nullptr)
    {
        last_result_ = reject_action(SimulationError::BattleNotStarted, "Start a battle first.");
        return result_to_dictionary(last_result_);
    }

    last_result_ = session_->UsePlayerSkill(std::string(skill_id.utf8().get_data()));
    return result_to_dictionary(last_result_);
}

Dictionary BattleBridge::switch_player(int player_index)
{
    if (session_ == nullptr)
    {
        last_result_ = reject_action(SimulationError::BattleNotStarted, "Start a battle first.");
        return result_to_dictionary(last_result_);
    }

    last_result_ = session_->SwitchPlayer(player_index);
    return result_to_dictionary(last_result_);
}

Dictionary BattleBridge::create_player_profile(const String& player_name, const String& spec) const
{
    PlayerProfileSystem profiles(data_);
    const PlayerProfileState player_profile = profiles.CreateStarter(
        std::string(player_name.utf8().get_data()),
        spec_from_string(spec));
    return player_profile_to_dictionary(player_profile);
}

Dictionary BattleBridge::get_player_skill_summary(
    const Dictionary& player_profile,
    const String& skill_id,
    const Dictionary& progress_dictionary) const
{
    const std::string skill_id_text = std::string(skill_id.utf8().get_data());
    const Skill* definition = data_.FindSkill(skill_id_text);
    if (definition == nullptr)
    {
        Dictionary summary;
        summary["id"] = skill_id;
        summary["name"] = skill_id;
        summary["level"] = 1;
        summary["xp"] = 0;
        summary["power"] = 0;
        summary["focus_cost"] = 0;
        return summary;
    }

    SkillProgress progress;
    progress.skillId = skill_id_text;
    progress.level = progress_dictionary.has("level")
        ? std::max(1, static_cast<int>(progress_dictionary["level"]))
        : 1;
    progress.xp = progress_dictionary.has("xp")
        ? std::max(0, static_cast<int>(progress_dictionary["xp"]))
        : 0;

    BattleRules rules(data_);
    ProgressionSystem progression;
    SkillSystem skills(data_, rules, progression);
    const Competitor user = competitor_from_player_profile(player_profile);
    return skill_to_dictionary(skills.CreateSkillView(*definition, progress, user));
}

Dictionary BattleBridge::get_pending_scout_offer(
    int rating,
    const Array& completed_offer_ids,
    const Array& declined_offer_ids) const
{
    ScoutSystem scouts(data_);
    return scout_offer_to_dictionary(scouts.GetNextOffer(
        rating,
        string_vector_from_array(completed_offer_ids),
        string_vector_from_array(declined_offer_ids)));
}

Dictionary BattleBridge::accept_scout_candidate(
    const Array& roster,
    const Dictionary& pending_offer,
    int candidate_index) const
{
    ScoutSystem scouts(data_);
    const ScoutOfferView offer = scout_offer_from_dictionary(pending_offer);
    const ProfileCommandResult result = scouts.CanRecruitCandidate(
        roster.size(),
        TrainerBalance::MaxPlayerProfiles,
        candidate_index,
        offer);

    Dictionary response;
    response["accepted"] = result.accepted;
    response["error_code"] = error_to_string(result.errorCode);
    response["error"] = String(result.error.c_str());
    response["offer_id"] = pending_offer.get("id", "");

    Array updated_roster;
    for (int index = 0; index < roster.size(); ++index)
    {
        updated_roster.push_back(roster[index]);
    }

    if (!result.accepted)
    {
        response["roster"] = updated_roster;
        response["message"] = String(result.error.c_str());
        return response;
    }

    Array candidates = pending_offer.has("candidates")
        ? Array(pending_offer["candidates"])
        : Array();
    Dictionary candidate = candidates[candidate_index];
    updated_roster.push_back(candidate);
    response["roster"] = updated_roster;
    response["message"] = String(candidate.get("name", "Prospect")) + String(" joined your roster.");
    return response;
}

Dictionary BattleBridge::complete_trainer_battle(
    const Dictionary& trainer_state_dictionary,
    const Dictionary& battle_config,
    const Dictionary& battle_result) const
{
    TrainerProfile trainer = TrainerProfile::FromState(trainer_profile_from_dictionary(trainer_state_dictionary));

    Array roster = trainer_state_dictionary.has("roster")
        ? Array(trainer_state_dictionary["roster"])
        : Array();
    Dictionary state = battle_result.has("state")
        ? Dictionary(battle_result["state"])
        : Dictionary();
    const String winner_text = battle_result.has("winner")
        ? String(battle_result["winner"])
        : String("none");
    const bool won = winner_text == "player";

    apply_battle_vitals(trainer, roster, state);
    apply_skill_progress(roster, trainer.GetState().activePlayerIndex, battle_result);

    Array level_up_messages;
    int money_reward = 0;

    if (won)
    {
        award_participant_xp(roster, battle_result, level_up_messages);
        money_reward = battle_config.has("money_reward")
            ? static_cast<int>(battle_config["money_reward"])
            : 100;
        trainer.AwardMoney(money_reward);

        const String trophy_id = battle_config.has("trophy_id")
            ? String(battle_config["trophy_id"])
            : String("");
        if (!trophy_id.is_empty())
        {
            trainer.AddTrophy(std::string(trophy_id.utf8().get_data()));
        }
    }

    const int rating_change = apply_rating_change(trainer, roster, battle_config, won);
    const String display_name = battle_config.has("display_name")
        ? String(battle_config["display_name"])
        : String("Opponent");

    Dictionary completion;
    completion["accepted"] = true;
    completion["won"] = won;
    completion["winner"] = winner_text;
    completion["npc_id"] = battle_config.get("id", "");
    completion["mark_npc_defeated"] = won && bool(battle_config.get("single_use", true));
    completion["level_up_messages"] = level_up_messages;
    completion["trainer_state"] = trainer_profile_to_dictionary(trainer.GetState(), roster);
    completion["summary_text"] = build_completion_summary(display_name, won, money_reward, rating_change);
    return completion;
}

Dictionary BattleBridge::get_battle_state() const
{
    if (session_ == nullptr)
    {
        return state_to_dictionary(BattleState{});
    }

    return state_to_dictionary(session_->GetState());
}

Array BattleBridge::get_available_skills() const
{
    Array skills;
    if (session_ == nullptr)
    {
        return skills;
    }

    for (const SkillView& skill : session_->GetAvailablePlayerSkills())
    {
        skills.push_back(skill_to_dictionary(skill));
    }

    return skills;
}

Dictionary BattleBridge::get_last_result() const
{
    return result_to_dictionary(last_result_);
}

BattleActionResult BattleBridge::reject_action(SimulationError error_code, const std::string& error) const
{
    BattleActionResult result;
    result.errorCode = error_code;
    result.error = error;
    result.finalState = BattleState{};
    return result;
}

void BattleBridge::apply_battle_vitals(
    TrainerProfile& trainer,
    Array& roster,
    const Dictionary& battle_state) const
{
    if (battle_state.has("active_player_index"))
    {
        trainer.SetActivePlayerIndex(std::max(0, static_cast<int>(battle_state["active_player_index"])));
    }

    if (!battle_state.has("player_team"))
    {
        return;
    }

    Array player_team = battle_state["player_team"];
    for (int index = 0; index < player_team.size(); ++index)
    {
        Dictionary player_state = player_team[index];
        const int profile_index = player_state.has("profile_index")
            ? static_cast<int>(player_state["profile_index"])
            : -1;
        if (profile_index < 0 || profile_index >= roster.size())
        {
            continue;
        }

        Dictionary player = roster[profile_index];
        player["current_hp"] = player_state.get("hp", player.get("current_hp", 100));
        player["max_hp"] = player_state.get("max_hp", player.get("max_hp", 100));
        player["current_focus"] = player_state.get("focus", player.get("current_focus", 50));
        player["max_focus"] = player_state.get("max_focus", player.get("max_focus", 50));
        roster[profile_index] = player;
    }
}

void BattleBridge::apply_skill_progress(
    Array& roster,
    int active_player_index,
    const Dictionary& battle_result) const
{
    int current_profile_index = active_player_index;
    if (!battle_result.has("events"))
    {
        return;
    }

    Array events = battle_result["events"];
    for (int index = 0; index < events.size(); ++index)
    {
        Dictionary event = events[index];
        const String event_type = event.has("type") ? String(event["type"]) : String("none");
        if (event_type == "battle_started" || event_type == "player_switched")
        {
            current_profile_index = event.has("new_player_index")
                ? static_cast<int>(event["new_player_index"])
                : current_profile_index;
        }

        if (!event.has("actor") || String(event["actor"]) != "player")
        {
            continue;
        }

        const String skill_id = event.has("skill_id") ? String(event["skill_id"]) : String("");
        if (skill_id.is_empty())
        {
            continue;
        }

        const int profile_index = event.has("profile_index")
            ? static_cast<int>(event["profile_index"])
            : current_profile_index;
        if (profile_index < 0 || profile_index >= roster.size())
        {
            continue;
        }

        Dictionary player = roster[profile_index];
        Dictionary progress_by_skill = player.has("skill_progress")
            ? Dictionary(player["skill_progress"])
            : Dictionary();
        Dictionary progress = progress_by_skill.has(skill_id)
            ? Dictionary(progress_by_skill[skill_id])
            : Dictionary();
        progress["skill_id"] = skill_id;

        if (event_type == "skill_xp_gained")
        {
            progress["xp"] = event.get("new_value", progress.get("xp", 0));
        }
        else if (event_type == "skill_leveled_up")
        {
            progress["level"] = event.get("new_level", progress.get("level", 1));
        }
        else
        {
            continue;
        }

        progress_by_skill[skill_id] = progress;
        player["skill_progress"] = progress_by_skill;
        roster[profile_index] = player;
    }
}

void BattleBridge::award_participant_xp(
    Array& roster,
    const Dictionary& battle_result,
    Array& level_up_messages) const
{
    PlayerProfileSystem profiles(data_);
    Dictionary reward = battle_result.has("reward")
        ? Dictionary(battle_result["reward"])
        : Dictionary();
    const int battle_xp = reward.has("xp_per_participant")
        ? static_cast<int>(reward["xp_per_participant"])
        : 0;
    if (battle_xp <= 0 || !reward.has("participant_player_indices"))
    {
        return;
    }

    Array participant_indices = reward["participant_player_indices"];
    for (int index = 0; index < participant_indices.size(); ++index)
    {
        const int profile_index = static_cast<int>(participant_indices[index]);
        if (profile_index < 0 || profile_index >= roster.size())
        {
            continue;
        }

        Dictionary player = roster[profile_index];
        const String player_name = player.has("name") ? String(player["name"]) : String("Player");
        PlayerProfileState player_profile = player_profile_from_dictionary(player);
        const ProfileCommandResult xp_result = profiles.AwardXp(player_profile, battle_xp);
        if (!xp_result.accepted)
        {
            continue;
        }

        Dictionary profile_snapshot = player_profile_to_dictionary(player_profile);
        Array keys = profile_snapshot.keys();
        for (int key_index = 0; key_index < keys.size(); ++key_index)
        {
            player[keys[key_index]] = profile_snapshot[keys[key_index]];
        }
        player["current_hp"] = std::min(
            static_cast<int>(player.get("current_hp", player.get("max_hp", 100))),
            static_cast<int>(player.get("max_hp", 100)));
        player["current_focus"] = std::min(
            static_cast<int>(player.get("current_focus", player.get("max_focus", 50))),
            static_cast<int>(player.get("max_focus", 50)));
        roster[profile_index] = player;

        level_up_messages.push_back(String("XP: ") + player_name + String(" gained ") + String::num_int64(battle_xp) + String(" player XP."));
        if (xp_result.leveledUp)
        {
            level_up_messages.push_back(String("LEVEL UP: ") + player_name + String(" reached Lv") + String::num_int64(player_profile.level) + String("."));
        }
    }
}

int BattleBridge::get_active_player_level(const TrainerProfile& trainer, const Array& roster) const
{
    const int active_index = trainer.GetState().activePlayerIndex;
    if (active_index < 0 || active_index >= roster.size())
    {
        return 1;
    }

    return player_profile_from_dictionary(Dictionary(roster[active_index])).level;
}

int BattleBridge::apply_rating_change(
    TrainerProfile& trainer,
    const Array& roster,
    const Dictionary& battle_config,
    bool won) const
{
    const int active_level = get_active_player_level(trainer, roster);
    const int opponent_level = battle_config.has("opponent_level")
        ? std::max(1, static_cast<int>(battle_config["opponent_level"]))
        : active_level;
    const MatchContext context = battle_config.has("match_context")
        ? match_context_from_string(String(battle_config["match_context"]))
        : MatchContext::Normal;

    RatingSystem ratings;
    const RatingResult rating_result = ratings.CalculateChange(
        trainer.GetState().rating,
        active_level,
        opponent_level,
        context,
        won);
    if (!rating_result.accepted)
    {
        return 0;
    }

    trainer.AwardRating(rating_result.ratingChange);
    return rating_result.ratingChange;
}

String BattleBridge::build_completion_summary(
    const String& display_name,
    bool won,
    int money_reward,
    int rating_change) const
{
    if (won)
    {
        return String("Won against ") + display_name + String(" (+")
            + String::num_int64(money_reward) + String(" money, +")
            + String::num_int64(rating_change) + String(" rating).");
    }

    return String("Lost against ") + display_name + String(" (")
        + String::num_int64(rating_change) + String(" rating).");
}

Competitor BattleBridge::competitor_from_player_profile(const Dictionary& player_profile) const
{
    Competitor competitor;
    competitor.name = player_profile.has("name")
        ? std::string(String(player_profile["name"]).utf8().get_data())
        : "Player";
    competitor.spec = player_profile.has("spec")
        ? spec_from_string(String(player_profile["spec"]))
        : Spec::Top;
    competitor.traitId = player_profile.has("trait_id")
        ? std::string(String(player_profile["trait_id"]).utf8().get_data())
        : "";
    if (data_.FindTrait(competitor.traitId) == nullptr)
    {
        const SpecData* spec_data = data_.FindSpec(competitor.spec);
        competitor.traitId = spec_data == nullptr ? "" : spec_data->defaultTraitId;
    }

    PassiveBonuses bonuses;
    if (player_profile.has("passive_bonuses"))
    {
        Dictionary passive_bonuses = player_profile["passive_bonuses"];
        bonuses.maxHpBonus = passive_bonuses.has("max_hp_bonus")
            ? static_cast<int>(passive_bonuses["max_hp_bonus"])
            : 0;
        bonuses.basePowerBonus = passive_bonuses.has("base_power_bonus")
            ? static_cast<int>(passive_bonuses["base_power_bonus"])
            : 0;
        bonuses.counterDamageBonusPercent = passive_bonuses.has("counter_damage_bonus_percent")
            ? static_cast<int>(passive_bonuses["counter_damage_bonus_percent"])
            : 0;
    }

    competitor.maxHp = Balance::StartingMaxHp + bonuses.maxHpBonus;
    competitor.hp = player_profile.has("current_hp")
        ? std::clamp(static_cast<int>(player_profile["current_hp"]), 0, competitor.maxHp)
        : competitor.maxHp;
    competitor.maxFocus = Balance::StartingMaxFocus;
    competitor.focus = player_profile.has("current_focus")
        ? std::clamp(static_cast<int>(player_profile["current_focus"]), 0, competitor.maxFocus)
        : competitor.maxFocus;
    competitor.basePower = Balance::StartingBasePower + bonuses.basePowerBonus;
    competitor.counterDamageBonusPercent = bonuses.counterDamageBonusPercent;
    return competitor;
}

std::vector<std::string> BattleBridge::string_vector_from_array(const Array& values) const
{
    std::vector<std::string> output;
    for (int index = 0; index < values.size(); ++index)
    {
        output.push_back(std::string(String(values[index]).utf8().get_data()));
    }
    return output;
}

Dictionary BattleBridge::scout_offer_to_dictionary(const ScoutOfferView& offer) const
{
    Dictionary dictionary;
    if (!offer.available)
    {
        return dictionary;
    }

    dictionary["id"] = String(offer.id.c_str());
    dictionary["required_rating"] = offer.requiredRating;
    dictionary["message"] = String(offer.message.c_str());

    Array candidates;
    for (const PlayerProfileState& candidate : offer.candidates)
    {
        candidates.push_back(player_profile_to_roster_dictionary(candidate));
    }
    dictionary["candidates"] = candidates;
    return dictionary;
}

ScoutOfferView BattleBridge::scout_offer_from_dictionary(const Dictionary& offer_dictionary) const
{
    ScoutOfferView offer;
    offer.id = offer_dictionary.has("id")
        ? std::string(String(offer_dictionary["id"]).utf8().get_data())
        : "";
    offer.available = !offer.id.empty();
    offer.requiredRating = offer_dictionary.has("required_rating")
        ? static_cast<int>(offer_dictionary["required_rating"])
        : 0;
    offer.message = offer_dictionary.has("message")
        ? std::string(String(offer_dictionary["message"]).utf8().get_data())
        : "";

    if (offer_dictionary.has("candidates"))
    {
        Array candidates = offer_dictionary["candidates"];
        for (int index = 0; index < candidates.size(); ++index)
        {
            offer.candidates.push_back(player_profile_from_dictionary(Dictionary(candidates[index])));
        }
    }
    return offer;
}

Dictionary BattleBridge::player_profile_to_roster_dictionary(const PlayerProfileState& player_profile) const
{
    Dictionary dictionary = player_profile_to_dictionary(player_profile);
    dictionary["current_hp"] = dictionary.get("max_hp", Balance::StartingMaxHp);
    dictionary["current_focus"] = dictionary.get("max_focus", Balance::StartingMaxFocus);

    Dictionary progress_by_skill;
    Array active_skills = dictionary["active_skill_ids"];
    for (int index = 0; index < active_skills.size(); ++index)
    {
        const String skill_id = String(active_skills[index]);
        Dictionary progress;
        progress["skill_id"] = skill_id;
        progress["level"] = 1;
        progress["xp"] = 0;
        progress_by_skill[skill_id] = progress;
    }
    dictionary["skill_progress"] = progress_by_skill;
    return dictionary;
}

Dictionary BattleBridge::competitor_to_dictionary(const CompetitorView& competitor) const
{
    Dictionary dictionary;
    Dictionary status;
    status["attack_modifier_percent"] = competitor.status.attackModifierPercent;
    status["attack_modifier_hits"] = competitor.status.attackModifierHits;
    status["defense_modifier_percent"] = competitor.status.defenseModifierPercent;
    status["defense_modifier_hits"] = competitor.status.defenseModifierHits;

    dictionary["profile_index"] = competitor.profileIndex;
    dictionary["name"] = String(competitor.name.c_str());
    dictionary["spec"] = String(ToString(competitor.spec).c_str());
    add_trait_fields(dictionary, competitor.traitId);
    dictionary["hp"] = competitor.hp;
    dictionary["max_hp"] = competitor.maxHp;
    dictionary["focus"] = competitor.focus;
    dictionary["max_focus"] = competitor.maxFocus;
    dictionary["base_power"] = competitor.basePower;
    dictionary["counter_damage_bonus_percent"] = competitor.counterDamageBonusPercent;
    dictionary["status"] = status;
    return dictionary;
}

Dictionary BattleBridge::state_to_dictionary(const BattleState& state) const
{
    Dictionary dictionary;
    dictionary["started"] = state.started;
    dictionary["finished"] = state.finished;
    dictionary["winner"] = winner_to_string(state.winner);
    dictionary["active_player_index"] = state.activePlayerIndex;
    dictionary["player"] = competitor_to_dictionary(state.player);
    dictionary["opponent"] = competitor_to_dictionary(state.opponent);

    Array player_team;
    for (const CompetitorView& player : state.playerTeam)
    {
        player_team.push_back(competitor_to_dictionary(player));
    }
    dictionary["player_team"] = player_team;
    return dictionary;
}

Dictionary BattleBridge::skill_to_dictionary(const SkillView& skill) const
{
    Dictionary dictionary;
    dictionary["id"] = String(skill.id.c_str());
    dictionary["name"] = String(skill.name.c_str());
    dictionary["description"] = String(skill.description.c_str());
    dictionary["tone"] = String(ToString(skill.tone).c_str());
    dictionary["power"] = skill.power;
    dictionary["focus_cost"] = skill.focusCost;
    dictionary["accuracy"] = skill.accuracy;
    dictionary["level"] = skill.level;
    dictionary["xp"] = skill.xp;
    dictionary["effect"] = effect_to_string(skill.effectType);
    dictionary["effect_value"] = skill.effectValue;
    dictionary["effect_uses"] = skill.effectUses;
    return dictionary;
}

Dictionary BattleBridge::event_to_dictionary(const BattleEvent& event) const
{
    Dictionary dictionary;
    dictionary["type"] = event_type_to_string(event.type);
    dictionary["actor"] = actor_to_string(event.actor);
    dictionary["target"] = actor_to_string(event.target);
    dictionary["skill_id"] = String(event.skillId.c_str());
    dictionary["old_player_index"] = event.oldPlayerIndex;
    dictionary["new_player_index"] = event.newPlayerIndex;
    dictionary["player_name"] = String(event.playerName.c_str());
    dictionary["old_value"] = event.oldValue;
    dictionary["new_value"] = event.newValue;
    dictionary["amount"] = event.amount;
    dictionary["old_level"] = event.oldLevel;
    dictionary["new_level"] = event.newLevel;
    dictionary["winner"] = winner_to_string(event.winner);
    dictionary["damage"] = event.damage.amount;
    dictionary["healing"] = event.effect.healingAmount;
    dictionary["effect"] = effect_to_string(event.effect.type);
    dictionary["reward"] = reward_to_dictionary(event.reward);
    return dictionary;
}

Dictionary BattleBridge::result_to_dictionary(const BattleActionResult& result) const
{
    Dictionary dictionary;
    dictionary["accepted"] = result.accepted;
    dictionary["error_code"] = error_to_string(result.errorCode);
    dictionary["error"] = String(result.error.c_str());
    dictionary["battle_started"] = result.battleStarted;
    dictionary["battle_finished"] = result.battleFinished;
    dictionary["winner"] = winner_to_string(result.winner);
    dictionary["state"] = state_to_dictionary(result.finalState);
    dictionary["reward"] = reward_to_dictionary(result.reward);

    Array events;
    for (const BattleEvent& event : result.events)
    {
        events.push_back(event_to_dictionary(event));
    }
    dictionary["events"] = events;
    return dictionary;
}

Dictionary BattleBridge::reward_to_dictionary(const BattleRewardResult& reward) const
{
    Dictionary dictionary;
    dictionary["awarded"] = reward.awarded;
    dictionary["total_xp"] = reward.totalXp;
    dictionary["xp_per_participant"] = reward.xpPerParticipant;

    Array participants;
    for (int index : reward.participantPlayerIndices)
    {
        participants.push_back(index);
    }
    dictionary["participant_player_indices"] = participants;
    return dictionary;
}

Dictionary BattleBridge::passive_bonuses_to_dictionary(const PassiveBonuses& bonuses) const
{
    Dictionary dictionary;
    dictionary["max_hp_bonus"] = bonuses.maxHpBonus;
    dictionary["base_power_bonus"] = bonuses.basePowerBonus;
    dictionary["counter_damage_bonus_percent"] = bonuses.counterDamageBonusPercent;
    return dictionary;
}

void BattleBridge::add_trait_fields(Dictionary& dictionary, const std::string& trait_id) const
{
    const TraitDefinition* trait = data_.FindTrait(trait_id);
    dictionary["trait_id"] = trait == nullptr ? String("") : String(trait->id.c_str());
    dictionary["trait_name"] = trait == nullptr ? String("") : String(trait->name.c_str());
    dictionary["trait_description"] = trait == nullptr ? String("") : String(trait->description.c_str());
}

Dictionary BattleBridge::player_profile_to_dictionary(const PlayerProfileState& player_profile) const
{
    Dictionary dictionary;
    dictionary["name"] = String(player_profile.name.c_str());
    dictionary["spec"] = String(ToString(player_profile.spec).c_str());
    add_trait_fields(dictionary, player_profile.traitId);
    dictionary["rank"] = String(ToString(player_profile.rank).c_str());
    dictionary["level"] = player_profile.level;
    dictionary["xp"] = player_profile.xp;
    dictionary["xp_required_for_next_level"] = player_profile.xpRequiredForNextLevel;
    dictionary["passive_bonuses"] = passive_bonuses_to_dictionary(player_profile.passiveBonuses);
    dictionary["max_hp"] = Balance::StartingMaxHp + player_profile.passiveBonuses.maxHpBonus;
    dictionary["max_focus"] = Balance::StartingMaxFocus;

    Array learned_skills;
    for (const std::string& skill_id : player_profile.learnedSkillIds)
    {
        learned_skills.push_back(String(skill_id.c_str()));
    }
    dictionary["learned_skill_ids"] = learned_skills;

    Array active_skills;
    for (const std::string& skill_id : player_profile.activeSkillIds)
    {
        active_skills.push_back(String(skill_id.c_str()));
    }
    dictionary["active_skill_ids"] = active_skills;
    return dictionary;
}

PlayerProfileState BattleBridge::player_profile_from_dictionary(const Dictionary& player_profile_dictionary) const
{
    PlayerProfileState player_profile;
    player_profile.name = player_profile_dictionary.has("name")
        ? std::string(String(player_profile_dictionary["name"]).utf8().get_data())
        : "Player";
    player_profile.spec = player_profile_dictionary.has("spec")
        ? spec_from_string(String(player_profile_dictionary["spec"]))
        : Spec::Top;
    if (player_profile_dictionary.has("trait_id"))
    {
        player_profile.traitId = std::string(String(player_profile_dictionary["trait_id"]).utf8().get_data());
    }
    player_profile.level = player_profile_dictionary.has("level")
        ? std::max(1, static_cast<int>(player_profile_dictionary["level"]))
        : 1;
    player_profile.xp = player_profile_dictionary.has("xp")
        ? std::max(0, static_cast<int>(player_profile_dictionary["xp"]))
        : 0;
    player_profile.xpRequiredForNextLevel = player_profile_dictionary.has("xp_required_for_next_level")
        ? std::max(1, static_cast<int>(player_profile_dictionary["xp_required_for_next_level"]))
        : PlayerProfileBalance::BaseXpForNextLevel;

    if (player_profile_dictionary.has("learned_skill_ids"))
    {
        Array learned_skills = player_profile_dictionary["learned_skill_ids"];
        for (int index = 0; index < learned_skills.size(); ++index)
        {
            player_profile.learnedSkillIds.push_back(std::string(String(learned_skills[index]).utf8().get_data()));
        }
    }

    if (player_profile_dictionary.has("active_skill_ids"))
    {
        Array active_skills = player_profile_dictionary["active_skill_ids"];
        for (int index = 0; index < active_skills.size(); ++index)
        {
            player_profile.activeSkillIds.push_back(std::string(String(active_skills[index]).utf8().get_data()));
        }
    }

    PlayerProfileSystem profiles(data_);
    player_profile.rank = profiles.GetRankForLevel(player_profile.level);
    player_profile.passiveBonuses = profiles.GetPassiveBonusesForLevel(player_profile.level);
    if (data_.FindTrait(player_profile.traitId) == nullptr)
    {
        const SpecData* spec_data = data_.FindSpec(player_profile.spec);
        player_profile.traitId = spec_data == nullptr ? "" : spec_data->defaultTraitId;
    }
    return player_profile;
}

TrainerProfileState BattleBridge::trainer_profile_from_dictionary(const Dictionary& trainer_state_dictionary) const
{
    TrainerProfileState trainer_state;
    trainer_state.trainerName = trainer_state_dictionary.has("trainer_name")
        ? std::string(String(trainer_state_dictionary["trainer_name"]).utf8().get_data())
        : "Trainer";
    trainer_state.rating = trainer_state_dictionary.has("rating")
        ? std::max(0, static_cast<int>(trainer_state_dictionary["rating"]))
        : TrainerBalance::StartingRating;
    trainer_state.money = trainer_state_dictionary.has("money")
        ? std::max(0, static_cast<int>(trainer_state_dictionary["money"]))
        : TrainerBalance::StartingMoney;
    trainer_state.activePlayerIndex = trainer_state_dictionary.has("active_player_index")
        ? std::max(0, static_cast<int>(trainer_state_dictionary["active_player_index"]))
        : 0;

    if (trainer_state_dictionary.has("roster"))
    {
        Array roster = trainer_state_dictionary["roster"];
        for (int index = 0; index < roster.size(); ++index)
        {
            trainer_state.roster.push_back(player_profile_from_dictionary(Dictionary(roster[index])));
        }
    }

    if (trainer_state_dictionary.has("trophies"))
    {
        Array trophies = trainer_state_dictionary["trophies"];
        for (int index = 0; index < trophies.size(); ++index)
        {
            trainer_state.trophyIds.push_back(std::string(String(trophies[index]).utf8().get_data()));
        }
    }

    return trainer_state;
}

Dictionary BattleBridge::trainer_profile_to_dictionary(
    const TrainerProfileState& trainer_state,
    const Array& roster) const
{
    Dictionary dictionary;
    dictionary["trainer_name"] = String(trainer_state.trainerName.c_str());
    dictionary["rating"] = trainer_state.rating;
    dictionary["money"] = trainer_state.money;
    dictionary["active_player_index"] = trainer_state.activePlayerIndex;
    dictionary["roster"] = roster;

    Array trophies;
    for (const std::string& trophy_id : trainer_state.trophyIds)
    {
        trophies.push_back(String(trophy_id.c_str()));
    }
    dictionary["trophies"] = trophies;
    return dictionary;
}

BattleSetup BattleBridge::setup_from_dictionary(const Dictionary& setup_dictionary) const
{
    BattleSetup setup;
    setup.activePlayerIndex = setup_dictionary.has("active_player_index")
        ? static_cast<int>(setup_dictionary["active_player_index"])
        : 0;

    if (setup_dictionary.has("opponent_name"))
    {
        setup.opponentName = std::string(String(setup_dictionary["opponent_name"]).utf8().get_data());
    }

    if (setup_dictionary.has("opponent_spec"))
    {
        setup.opponentSpec = spec_from_string(String(setup_dictionary["opponent_spec"]));
    }
    if (setup_dictionary.has("opponent_trait_id"))
    {
        setup.opponentTraitId = std::string(String(setup_dictionary["opponent_trait_id"]).utf8().get_data());
    }

    if (setup_dictionary.has("opponent_hp"))
    {
        setup.opponentMaxHp = std::max(1, static_cast<int>(setup_dictionary["opponent_hp"]));
    }

    if (setup_dictionary.has("opponent_focus"))
    {
        setup.opponentMaxFocus = std::max(1, static_cast<int>(setup_dictionary["opponent_focus"]));
    }

    if (setup_dictionary.has("opponent_base_power_bonus"))
    {
        setup.opponentBasePowerBonus = static_cast<int>(setup_dictionary["opponent_base_power_bonus"]);
    }

    if (!setup_dictionary.has("player_team"))
    {
        return setup;
    }

    Array player_team = setup_dictionary["player_team"];
    for (int index = 0; index < player_team.size(); ++index)
    {
        Dictionary player_dictionary = player_team[index];
        BattleSetup::PlayerSlot slot;
        slot.profileIndex = player_dictionary.has("profile_index")
            ? static_cast<int>(player_dictionary["profile_index"])
            : index;
        slot.name = player_dictionary.has("name")
            ? std::string(String(player_dictionary["name"]).utf8().get_data())
            : "Player";
        slot.spec = player_dictionary.has("spec")
            ? spec_from_string(String(player_dictionary["spec"]))
            : Spec::Top;
        if (player_dictionary.has("trait_id"))
        {
            slot.traitId = std::string(String(player_dictionary["trait_id"]).utf8().get_data());
        }
        slot.currentHp = player_dictionary.has("current_hp")
            ? static_cast<int>(player_dictionary["current_hp"])
            : -1;
        slot.currentFocus = player_dictionary.has("current_focus")
            ? static_cast<int>(player_dictionary["current_focus"])
            : -1;

        if (player_dictionary.has("passive_bonuses"))
        {
            Dictionary bonuses = player_dictionary["passive_bonuses"];
            slot.passiveBonuses.maxHpBonus = bonuses.has("max_hp_bonus")
                ? static_cast<int>(bonuses["max_hp_bonus"])
                : 0;
            slot.passiveBonuses.basePowerBonus = bonuses.has("base_power_bonus")
                ? static_cast<int>(bonuses["base_power_bonus"])
                : 0;
            slot.passiveBonuses.counterDamageBonusPercent = bonuses.has("counter_damage_bonus_percent")
                ? static_cast<int>(bonuses["counter_damage_bonus_percent"])
                : 0;
        }

        if (player_dictionary.has("skills"))
        {
            Array skills = player_dictionary["skills"];
            for (int skill_index = 0; skill_index < skills.size(); ++skill_index)
            {
                Dictionary skill_dictionary = skills[skill_index];
                SkillProgress progress;
                const String skill_id = skill_dictionary.has("id")
                    ? String(skill_dictionary["id"])
                    : String(skill_dictionary.get("skill_id", ""));
                progress.skillId = std::string(skill_id.utf8().get_data());
                progress.level = skill_dictionary.has("level")
                    ? std::max(1, static_cast<int>(skill_dictionary["level"]))
                    : 1;
                progress.xp = skill_dictionary.has("xp")
                    ? std::max(0, static_cast<int>(skill_dictionary["xp"]))
                    : 0;

                if (!progress.skillId.empty())
                {
                    slot.skills.push_back(progress);
                }
            }
        }

        setup.playerTeam.push_back(slot);
    }

    return setup;
}

Spec BattleBridge::spec_from_string(const String& value) const
{
    const std::string text = std::string(value.utf8().get_data());
    if (text == "Jungle" || text == "jungle") return Spec::Jungle;
    if (text == "Mid" || text == "mid") return Spec::Mid;
    if (text == "ADC" || text == "Adc" || text == "adc") return Spec::Adc;
    if (text == "Support" || text == "support") return Spec::Support;
    return Spec::Top;
}

MatchContext BattleBridge::match_context_from_string(const String& value) const
{
    const std::string text = std::string(value.utf8().get_data());
    if (text == "Tutorial" || text == "tutorial") return MatchContext::Tutorial;
    if (text == "Nemesis" || text == "nemesis") return MatchContext::Nemesis;
    if (text == "Major" || text == "major") return MatchContext::Major;
    return MatchContext::Normal;
}

String BattleBridge::actor_to_string(BattleActor actor) const
{
    switch (actor)
    {
    case BattleActor::Player: return "player";
    case BattleActor::Opponent: return "opponent";
    case BattleActor::None: return "none";
    }

    return "none";
}

String BattleBridge::winner_to_string(BattleWinner winner) const
{
    switch (winner)
    {
    case BattleWinner::Player: return "player";
    case BattleWinner::Opponent: return "opponent";
    case BattleWinner::None: return "none";
    }

    return "none";
}

String BattleBridge::event_type_to_string(BattleEventType type) const
{
    switch (type)
    {
    case BattleEventType::BattleStarted: return "battle_started";
    case BattleEventType::PlayerSwitched: return "player_switched";
    case BattleEventType::SkillStarted: return "skill_started";
    case BattleEventType::FocusChanged: return "focus_changed";
    case BattleEventType::AttackMissed: return "attack_missed";
    case BattleEventType::DamageApplied: return "damage_applied";
    case BattleEventType::HealingApplied: return "healing_applied";
    case BattleEventType::StatusApplied: return "status_applied";
    case BattleEventType::SkillXpGained: return "skill_xp_gained";
    case BattleEventType::SkillLeveledUp: return "skill_leveled_up";
    case BattleEventType::BattleFinished: return "battle_finished";
    case BattleEventType::RewardGranted: return "reward_granted";
    case BattleEventType::None: return "none";
    }

    return "none";
}

String BattleBridge::error_to_string(SimulationError error) const
{
    switch (error)
    {
    case SimulationError::None: return "none";
    case SimulationError::BattleNotStarted: return "battle_not_started";
    case SimulationError::BattleAlreadyFinished: return "battle_already_finished";
    case SimulationError::UnknownSkill: return "unknown_skill";
    case SimulationError::InsufficientFocus: return "insufficient_focus";
    case SimulationError::UnknownGameType: return "unknown_game_type";
    case SimulationError::BattleNeedsPlayerProfile: return "battle_needs_player_profile";
    case SimulationError::UnknownSpec: return "unknown_spec";
    case SimulationError::UnknownPlayerProfileSpec: return "unknown_player_profile_spec";
    case SimulationError::UnknownActivePlayerProfile: return "unknown_active_player_profile";
    case SimulationError::UnknownPlayerProfile: return "unknown_player_profile";
    case SimulationError::PlayerProfileAlreadyActive: return "player_profile_already_active";
    case SimulationError::PlayerProfileCannotPlay: return "player_profile_cannot_play";
    case SimulationError::LevelsMustBePositive: return "levels_must_be_positive";
    case SimulationError::SkillAlreadyLearned: return "skill_already_learned";
    case SimulationError::SkillNotLearned: return "skill_not_learned";
    case SimulationError::SkillAlreadyActive: return "skill_already_active";
    case SimulationError::ActiveSkillSlotsFull: return "active_skill_slots_full";
    case SimulationError::SkillNotActive: return "skill_not_active";
    case SimulationError::NegativeXpAward: return "negative_xp_award";
    case SimulationError::TrophyAlreadyEarned: return "trophy_already_earned";
    case SimulationError::RosterFull: return "roster_full";
    }

    return "unknown";
}

String BattleBridge::effect_to_string(SkillEffectType effect) const
{
    switch (effect)
    {
    case SkillEffectType::Heal: return "heal";
    case SkillEffectType::AttackModifier: return "attack_modifier";
    case SkillEffectType::DefenseModifier: return "defense_modifier";
    case SkillEffectType::None: return "none";
    }

    return "none";
}
