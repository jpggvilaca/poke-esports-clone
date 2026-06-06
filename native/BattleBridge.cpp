#include "BattleBridge.h"

#include "PlayerProfileSystem.h"

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
    godot::ClassDB::bind_method(godot::D_METHOD("start_demo_battle"), &BattleBridge::start_demo_battle);
    godot::ClassDB::bind_method(godot::D_METHOD("use_skill", "skill_id"), &BattleBridge::use_skill);
    godot::ClassDB::bind_method(godot::D_METHOD("switch_player", "player_index"), &BattleBridge::switch_player);
    godot::ClassDB::bind_method(godot::D_METHOD("create_player_profile", "player_name", "spec"), &BattleBridge::create_player_profile);
    godot::ClassDB::bind_method(godot::D_METHOD("award_player_profile_xp", "player_profile", "amount"), &BattleBridge::award_player_profile_xp);
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

Dictionary BattleBridge::start_demo_battle()
{
    session_ = std::make_unique<BattleSession>(data_, 20260605);

    BattleSetup setup;
    BattleSetup::PlayerSlot player;
    player.profileIndex = 1;
    player.name = "Human Trainer";
    player.spec = Spec::Top;
    player.style = Style::Balanced;
    setup.playerTeam.push_back(player);
    setup.activePlayerIndex = 0;
    setup.opponentName = "Opponent";
    setup.opponentSpec = Spec::Jungle;
    setup.opponentStyle = Style::Balanced;

    last_result_ = session_->StartBattle(setup);
    return result_to_dictionary(last_result_);
}

Dictionary BattleBridge::use_skill(const String& skill_id)
{
    if (session_ == nullptr)
    {
        start_demo_battle();
    }

    last_result_ = session_->UsePlayerSkill(std::string(skill_id.utf8().get_data()));
    return result_to_dictionary(last_result_);
}

Dictionary BattleBridge::switch_player(int player_index)
{
    if (session_ == nullptr)
    {
        start_demo_battle();
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

Dictionary BattleBridge::award_player_profile_xp(const Dictionary& player_profile_dictionary, int amount) const
{
    PlayerProfileSystem profiles(data_);
    PlayerProfileState player_profile = player_profile_from_dictionary(player_profile_dictionary);
    const ProfileCommandResult result = profiles.AwardXp(player_profile, amount);
    return profile_result_to_dictionary(result, player_profile);
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
    dictionary["style"] = String(ToString(competitor.style).c_str());
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
    dictionary["power"] = skill.power;
    dictionary["focus_cost"] = skill.focusCost;
    dictionary["accuracy"] = skill.accuracy;
    dictionary["level"] = skill.level;
    dictionary["xp"] = skill.xp;
    dictionary["effect"] = effect_to_string(skill.effectType);
    return dictionary;
}

Dictionary BattleBridge::event_to_dictionary(const BattleEvent& event) const
{
    Dictionary dictionary;
    dictionary["type"] = event_type_to_string(event.type);
    dictionary["actor"] = actor_to_string(event.actor);
    dictionary["target"] = actor_to_string(event.target);
    dictionary["skill_id"] = String(event.skillId.c_str());
    dictionary["style"] = String(ToString(event.style).c_str());
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

Dictionary BattleBridge::player_profile_to_dictionary(const PlayerProfileState& player_profile) const
{
    Dictionary dictionary;
    dictionary["name"] = String(player_profile.name.c_str());
    dictionary["spec"] = String(ToString(player_profile.spec).c_str());
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

Dictionary BattleBridge::profile_result_to_dictionary(
    const ProfileCommandResult& result,
    const PlayerProfileState& player_profile) const
{
    Dictionary dictionary;
    dictionary["accepted"] = result.accepted;
    dictionary["error_code"] = error_to_string(result.errorCode);
    dictionary["error"] = String(result.error.c_str());
    dictionary["old_value"] = result.oldValue;
    dictionary["new_value"] = result.newValue;
    dictionary["old_level"] = result.oldLevel;
    dictionary["new_level"] = result.newLevel;
    dictionary["leveled_up"] = result.leveledUp;
    dictionary["player_profile"] = player_profile_to_dictionary(player_profile);
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
    player_profile.passiveBonuses = profiles.GetPassiveBonusesForRank(player_profile.rank);
    return player_profile;
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

    if (setup_dictionary.has("opponent_style"))
    {
        setup.opponentStyle = style_from_string(String(setup_dictionary["opponent_style"]));
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
        slot.style = player_dictionary.has("style")
            ? style_from_string(String(player_dictionary["style"]))
            : Style::Balanced;
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

Style BattleBridge::style_from_string(const String& value) const
{
    const std::string text = std::string(value.utf8().get_data());
    if (text == "Aggressive" || text == "aggressive") return Style::Aggressive;
    if (text == "Defensive" || text == "defensive") return Style::Defensive;
    return Style::Balanced;
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
    case BattleEventType::StyleChanged: return "style_changed";
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
    case SimulationError::SkillUnavailableForStyle: return "skill_unavailable_for_style";
    case SimulationError::InsufficientFocus: return "insufficient_focus";
    case SimulationError::UnknownGameType: return "unknown_game_type";
    case SimulationError::BattleNeedsPlayerProfile: return "battle_needs_player_profile";
    case SimulationError::UnknownSpec: return "unknown_spec";
    case SimulationError::UnknownPlayerProfileSpec: return "unknown_player_profile_spec";
    case SimulationError::UnknownStyle: return "unknown_style";
    case SimulationError::UnknownPlayerProfileStyle: return "unknown_player_profile_style";
    case SimulationError::UnknownActivePlayerProfile: return "unknown_active_player_profile";
    case SimulationError::StyleAlreadyActive: return "style_already_active";
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
