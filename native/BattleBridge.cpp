#include "BattleBridge.h"

#include <godot_cpp/core/class_db.hpp>

using godot::Array;
using godot::Dictionary;
using godot::String;

void BattleBridge::_bind_methods()
{
    godot::ClassDB::bind_method(godot::D_METHOD("start_demo_battle"), &BattleBridge::start_demo_battle);
    godot::ClassDB::bind_method(godot::D_METHOD("use_skill", "skill_id"), &BattleBridge::use_skill);
    godot::ClassDB::bind_method(godot::D_METHOD("get_battle_state"), &BattleBridge::get_battle_state);
    godot::ClassDB::bind_method(godot::D_METHOD("get_available_skills"), &BattleBridge::get_available_skills);
    godot::ClassDB::bind_method(godot::D_METHOD("get_last_result"), &BattleBridge::get_last_result);
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

    Array events;
    for (const BattleEvent& event : result.events)
    {
        events.push_back(event_to_dictionary(event));
    }
    dictionary["events"] = events;
    return dictionary;
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
