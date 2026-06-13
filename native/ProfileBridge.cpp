#include "ProfileBridge.h"

#include "BridgeDictionaryUtils.h"
#include "BridgeSerializers.h"
#include "BattleRules.h"
#include "PlayerProfileSystem.h"
#include "ProgressionSystem.h"
#include "RatingSystem.h"
#include "SkillSystem.h"
#include "TrainerProfile.h"

#include <godot_cpp/core/class_db.hpp>

#include <algorithm>

using godot::Array;
using godot::Dictionary;
using godot::String;
using bridge_dictionary::get_int_field;
using bridge_dictionary::get_min_int_field;
using bridge_dictionary::to_std_string;

void ProfileBridge::_bind_methods()
{
    godot::ClassDB::bind_method(godot::D_METHOD("create_player_profile", "player_name", "spec"), &ProfileBridge::create_player_profile);
    godot::ClassDB::bind_method(godot::D_METHOD("get_player_skill_summary", "player_profile", "skill_id", "progress"), &ProfileBridge::get_player_skill_summary);
    godot::ClassDB::bind_method(godot::D_METHOD("complete_trainer_battle", "trainer_state", "battle_config", "battle_result"), &ProfileBridge::complete_trainer_battle);
}

Dictionary ProfileBridge::create_player_profile(const String& player_name, const String& spec) const
{
    PlayerProfileSystem profiles(data_);
    const PlayerProfileState player_profile = profiles.CreateStarter(
        to_std_string(player_name),
        bridge_serializers::SpecFromString(spec));
    return bridge_serializers::PlayerProfileToDictionary(player_profile, data_);
}

Dictionary ProfileBridge::get_player_skill_summary(
    const Dictionary& player_profile,
    const String& skill_id,
    const Dictionary& progress_dictionary) const
{
    const std::string skill_id_text = to_std_string(skill_id);
    const Skill* definition = data_.FindSkill(skill_id_text);
    if (definition == nullptr)
    {
        Dictionary summary;
        summary["id"] = skill_id;
        summary["name"] = skill_id;
        summary["level"] = 1;
        summary["xp"] = 0;
        summary["mana_cost"] = 0;
        summary["mana_gain"] = 0;
        summary["cooldown_turns"] = 0;
        summary["cooldown_remaining"] = 0;
        summary["can_use"] = true;
        summary["disabled_reason"] = "";
        return summary;
    }

    SkillProgress progress;
    progress.skillId = skill_id_text;
    progress.level = get_min_int_field(progress_dictionary, "level", 1, 1);
    progress.xp = get_min_int_field(progress_dictionary, "xp", 0, 0);

    BattleRules rules(data_);
    ProgressionSystem progression;
    SkillSystem skills(data_, rules, progression);
    const Competitor user = bridge_serializers::CompetitorFromPlayerProfile(player_profile, data_);
    return bridge_serializers::SkillToDictionary(skills.CreateSkillView(*definition, progress, user, {}, 0));
}

Dictionary ProfileBridge::complete_trainer_battle(
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

    Array skill_progress_changes;
    apply_skill_progress(roster, trainer.GetState().activePlayerIndex, battle_result, skill_progress_changes);

    Array level_up_messages;
    Array battle_xp_awards;
    int money_reward = 0;

    if (won)
    {
        award_participant_xp(roster, battle_result, level_up_messages, battle_xp_awards);
        money_reward = get_int_field(battle_config, "money_reward", 100);
        trainer.AwardMoney(money_reward);

        const String trophy_id = battle_config.has("trophy_id")
            ? String(battle_config["trophy_id"])
            : String("");
        if (!trophy_id.is_empty())
        {
            trainer.AddTrophy(to_std_string(trophy_id));
        }
    }

    const int rating_change = apply_rating_change(trainer, roster, battle_config, won);
    const String display_name = battle_config.has("display_name")
        ? String(battle_config["display_name"])
        : String("Opponent");
    restore_roster_vitals(roster);

    Dictionary progression_changes;
    progression_changes["battle_xp_awards"] = battle_xp_awards;
    progression_changes["skill_progress_changes"] = skill_progress_changes;
    progression_changes["level_up_messages"] = level_up_messages;

    Dictionary roster_changes;
    roster_changes["active_player_index"] = trainer.GetState().activePlayerIndex;
    roster_changes["roster"] = roster;

    Dictionary completion;
    completion["accepted"] = true;
    completion["won"] = won;
    completion["winner"] = winner_text;
    completion["npc_id"] = battle_config.get("id", "");
    completion["mark_npc_defeated"] = won && bool(battle_config.get("single_use", true));
    completion["level_up_messages"] = level_up_messages;
    completion["trainer_state"] = trainer_profile_to_dictionary(trainer.GetState(), roster);
    completion["progression_changes"] = progression_changes;
    completion["roster_changes"] = roster_changes;
    completion["rating_change"] = rating_change;
    completion["money_reward"] = money_reward;
    completion["battle_xp_awards"] = battle_xp_awards;
    completion["summary_text"] = build_completion_summary(display_name, won, money_reward, rating_change);
    return completion;
}

void ProfileBridge::apply_battle_vitals(
    TrainerProfile& trainer,
    Array& roster,
    const Dictionary& battle_state) const
{
    if (battle_state.has("active_player_index") && battle_state.has("player_team"))
    {
        const int battle_active_index = get_min_int_field(battle_state, "active_player_index", 0, 0);
        Array player_team = battle_state["player_team"];
        if (battle_active_index >= 0 && battle_active_index < player_team.size())
        {
            Dictionary active_player = player_team[battle_active_index];
            trainer.SetActivePlayerIndex(get_min_int_field(active_player, "profile_index", 0, battle_active_index));
        }
    }

    if (!battle_state.has("player_team"))
    {
        return;
    }

    Array player_team = battle_state["player_team"];
    for (int index = 0; index < player_team.size(); ++index)
    {
        Dictionary player_state = player_team[index];
        const int profile_index = get_int_field(player_state, "profile_index", -1);
        if (profile_index < 0 || profile_index >= roster.size())
        {
            continue;
        }

        Dictionary player = roster[profile_index];
        player["current_hp"] = player_state.get("hp", player.get("current_hp", 100));
        player["max_hp"] = player_state.get("max_hp", player.get("max_hp", 100));
        player["current_mana"] = player_state.get("mana", player.get("current_mana", player.get("current_focus", 0)));
        player["max_mana"] = player_state.get("max_mana", player.get("max_mana", player.get("max_focus", Balance::StartingMaxMana)));
        roster[profile_index] = player;
    }
}

void ProfileBridge::restore_roster_vitals(Array& roster) const
{
    for (int index = 0; index < roster.size(); ++index)
    {
        Dictionary player = roster[index];
        const int max_hp = get_min_int_field(player, "max_hp", 1, Balance::StartingMaxHp);
        const int max_mana = get_min_int_field(player, "max_mana", 1, Balance::StartingMaxMana);
        player["current_hp"] = max_hp;
        player["current_mana"] = max_mana;
        roster[index] = player;
    }
}

void ProfileBridge::apply_skill_progress(
    Array& roster,
    int active_player_index,
    const Dictionary& battle_result,
    Array& skill_progress_changes) const
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
            if (get_int_field(event, "profile_index", -1) >= 0)
            {
                current_profile_index = get_int_field(event, "profile_index");
            }
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

        const int profile_index = get_int_field(event, "profile_index", current_profile_index);
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
            Dictionary change;
            change["profile_index"] = profile_index;
            change["skill_id"] = skill_id;
            change["field"] = "xp";
            change["old_value"] = event.get("old_value", 0);
            change["new_value"] = progress["xp"];
            skill_progress_changes.push_back(change);
        }
        else if (event_type == "skill_leveled_up")
        {
            progress["level"] = event.get("new_level", progress.get("level", 1));
            Dictionary change;
            change["profile_index"] = profile_index;
            change["skill_id"] = skill_id;
            change["field"] = "level";
            change["old_value"] = event.get("old_level", 1);
            change["new_value"] = progress["level"];
            skill_progress_changes.push_back(change);
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

void ProfileBridge::award_participant_xp(
    Array& roster,
    const Dictionary& battle_result,
    Array& level_up_messages,
    Array& battle_xp_awards) const
{
    PlayerProfileSystem profiles(data_);
    Dictionary reward = battle_result.has("reward")
        ? Dictionary(battle_result["reward"])
        : Dictionary();
    const int battle_xp = get_int_field(reward, "xp_per_participant");
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
        PlayerProfileState player_profile = bridge_serializers::PlayerProfileFromDictionary(player, data_);
        const int old_level = player_profile.level;
        const int old_xp = player_profile.xp;
        const ProfileCommandResult xp_result = profiles.AwardXp(player_profile, battle_xp);
        if (!xp_result.accepted)
        {
            continue;
        }

        Dictionary profile_snapshot = bridge_serializers::PlayerProfileToDictionary(player_profile, data_);
        Array keys = profile_snapshot.keys();
        for (int key_index = 0; key_index < keys.size(); ++key_index)
        {
            player[keys[key_index]] = profile_snapshot[keys[key_index]];
        }
        const int max_hp = static_cast<int>(player.get("max_hp", 100));
        player["current_hp"] = xp_result.leveledUp
            ? max_hp
            : std::min(
                static_cast<int>(player.get("current_hp", max_hp)),
                max_hp);
        player["current_mana"] = std::min(
            static_cast<int>(player.get("current_mana", 0)),
            static_cast<int>(player.get("max_mana", Balance::StartingMaxMana)));
        roster[profile_index] = player;

        Dictionary xp_award;
        xp_award["profile_index"] = profile_index;
        xp_award["player_name"] = player_name;
        xp_award["amount"] = battle_xp;
        xp_award["old_level"] = old_level;
        xp_award["new_level"] = player_profile.level;
        xp_award["old_xp"] = old_xp;
        xp_award["new_xp"] = player_profile.xp;
        xp_award["leveled_up"] = xp_result.leveledUp;
        battle_xp_awards.push_back(xp_award);

        level_up_messages.push_back(String("XP: ") + player_name + String(" gained ") + String::num_int64(battle_xp) + String(" player XP."));
        if (xp_result.leveledUp)
        {
            level_up_messages.push_back(String("LEVEL UP: ") + player_name + String(" reached Lv") + String::num_int64(player_profile.level) + String("."));
        }
    }
}

int ProfileBridge::get_active_player_level(const TrainerProfile& trainer, const Array& roster) const
{
    const int active_index = trainer.GetState().activePlayerIndex;
    if (active_index < 0 || active_index >= roster.size())
    {
        return 1;
    }

    return bridge_serializers::PlayerProfileFromDictionary(Dictionary(roster[active_index]), data_).level;
}

int ProfileBridge::apply_rating_change(
    TrainerProfile& trainer,
    const Array& roster,
    const Dictionary& battle_config,
    bool won) const
{
    const int active_level = get_active_player_level(trainer, roster);
    const int opponent_level = get_min_int_field(battle_config, "opponent_level", 1, active_level);
    const MatchContext context = battle_config.has("match_context")
        ? bridge_serializers::MatchContextFromString(String(battle_config["match_context"]))
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

String ProfileBridge::build_completion_summary(
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

TrainerProfileState ProfileBridge::trainer_profile_from_dictionary(const Dictionary& trainer_state_dictionary) const
{
    TrainerProfileState trainer_state;
    trainer_state.trainerName = bridge_dictionary::get_string_field(trainer_state_dictionary, "trainer_name", "Trainer");
    trainer_state.rating = get_min_int_field(trainer_state_dictionary, "rating", 0, TrainerBalance::StartingRating);
    trainer_state.money = get_min_int_field(trainer_state_dictionary, "money", 0, TrainerBalance::StartingMoney);
    trainer_state.activePlayerIndex = get_min_int_field(trainer_state_dictionary, "active_player_index", 0, 0);

    if (trainer_state_dictionary.has("roster"))
    {
        Array roster = trainer_state_dictionary["roster"];
        for (int index = 0; index < roster.size(); ++index)
        {
            trainer_state.roster.push_back(bridge_serializers::PlayerProfileFromDictionary(Dictionary(roster[index]), data_));
        }
    }

    if (trainer_state_dictionary.has("trophies"))
    {
        Array trophies = trainer_state_dictionary["trophies"];
        for (int index = 0; index < trophies.size(); ++index)
        {
            trainer_state.trophyIds.push_back(to_std_string(String(trophies[index])));
        }
    }

    return trainer_state;
}

Dictionary ProfileBridge::trainer_profile_to_dictionary(
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
