#include "BattleBridge.h"

#include "BridgeDictionaryUtils.h"
#include "BridgeSerializers.h"

#include <godot_cpp/core/class_db.hpp>

#include <cstdint>
#include <string>

using godot::Array;
using godot::Dictionary;
using godot::String;
using bridge_dictionary::get_int_field;
using bridge_dictionary::get_min_int_field;
using bridge_dictionary::get_string_field;
using bridge_dictionary::to_std_string;

void BattleBridge::_bind_methods()
{
    godot::ClassDB::bind_method(godot::D_METHOD("start_battle", "setup"), &BattleBridge::start_battle);
    godot::ClassDB::bind_method(godot::D_METHOD("use_skill", "skill_id", "target_player_index"), &BattleBridge::use_skill);
    godot::ClassDB::bind_method(godot::D_METHOD("use_drill", "result_quality"), &BattleBridge::use_drill);
    godot::ClassDB::bind_method(godot::D_METHOD("pass_turn"), &BattleBridge::pass_turn);
    godot::ClassDB::bind_method(godot::D_METHOD("get_battle_state"), &BattleBridge::get_battle_state);
    godot::ClassDB::bind_method(godot::D_METHOD("get_available_skills"), &BattleBridge::get_available_skills);
    godot::ClassDB::bind_method(godot::D_METHOD("get_drill_action"), &BattleBridge::get_drill_action);
    godot::ClassDB::bind_method(godot::D_METHOD("get_last_result"), &BattleBridge::get_last_result);
}

Dictionary BattleBridge::start_battle(const Dictionary& setup_dictionary)
{
    const std::uint32_t seed = static_cast<std::uint32_t>(get_int_field(setup_dictionary, "seed", 20260606));

    session_ = std::make_unique<BattleSession>(data_, seed);
    BattleSetup setup = setup_from_dictionary(setup_dictionary);
    last_result_ = session_->StartBattle(setup);
    return result_to_dictionary(last_result_);
}

Dictionary BattleBridge::use_skill(const String& skill_id, int target_player_index)
{
    if (session_ == nullptr)
    {
        last_result_ = reject_action(SimulationError::BattleNotStarted, "Start a battle first.");
        return result_to_dictionary(last_result_);
    }

    last_result_ = session_->UsePlayerSkill(to_std_string(skill_id), target_player_index);
    return result_to_dictionary(last_result_);
}

Dictionary BattleBridge::use_drill(const String& result_quality)
{
    if (session_ == nullptr)
    {
        last_result_ = reject_action(SimulationError::BattleNotStarted, "Start a battle first.");
        return result_to_dictionary(last_result_);
    }

    last_result_ = session_->UsePlayerDrill(drill_quality_from_string(result_quality));
    return result_to_dictionary(last_result_);
}

Dictionary BattleBridge::pass_turn()
{
    if (session_ == nullptr)
    {
        last_result_ = reject_action(SimulationError::BattleNotStarted, "Start a battle first.");
        return result_to_dictionary(last_result_);
    }

    last_result_ = session_->PassPlayerTurn();
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

Dictionary BattleBridge::get_drill_action() const
{
    if (session_ == nullptr)
    {
        return drill_to_dictionary({});
    }

    return drill_to_dictionary(session_->GetPlayerDrill());
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

Dictionary BattleBridge::competitor_to_dictionary(const CompetitorView& competitor) const
{
    Dictionary dictionary;
    Dictionary status;
    status["attack_modifier_percent"] = competitor.status.attackModifierPercent;
    status["attack_modifier_turns"] = competitor.status.attackModifierTurns;
    status["defense_modifier_percent"] = competitor.status.defenseModifierPercent;
    status["defense_modifier_turns"] = competitor.status.defenseModifierTurns;
    status["attack_penetration_percent"] = competitor.status.attackPenetrationPercent;
    status["attack_penetration_turns"] = competitor.status.attackPenetrationTurns;
    status["cooldown_modifier_percent"] = competitor.status.cooldownModifierPercent;
    status["cooldown_modifier_turns"] = competitor.status.cooldownModifierTurns;
    status["healing_received_modifier_percent"] = competitor.status.healingReceivedModifierPercent;
    status["healing_received_modifier_turns"] = competitor.status.healingReceivedModifierTurns;
    status["stunned_turns"] = competitor.status.stunnedTurns;
    status["silenced_turns"] = competitor.status.silencedTurns;
    status["rooted_turns"] = competitor.status.rootedTurns;
    status["mark_turns"] = competitor.status.markTurns;
    status["mark_bonus_damage"] = competitor.status.markBonusDamage;
    status["mark_source"] = actor_to_string(competitor.status.markSource);

    dictionary["profile_index"] = competitor.profileIndex;
    dictionary["name"] = String(competitor.name.c_str());
    dictionary["spec"] = String(ToString(competitor.spec).c_str());
    add_trait_fields(dictionary, competitor.traitId);
    dictionary["hp"] = competitor.hp;
    dictionary["max_hp"] = competitor.maxHp;
    dictionary["mana"] = competitor.mana;
    dictionary["max_mana"] = competitor.maxMana;
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
    return bridge_serializers::SkillToDictionary(skill);
}

Dictionary BattleBridge::drill_to_dictionary(const DrillView& drill) const
{
    Dictionary dictionary;
    dictionary["id"] = String(drill.id.c_str());
    dictionary["display_name"] = String(drill.displayName.c_str());
    dictionary["description"] = String(drill.description.c_str());
    dictionary["miss_mana_gain"] = drill.missManaGain;
    dictionary["good_mana_gain"] = drill.goodManaGain;
    dictionary["perfect_mana_gain"] = drill.perfectManaGain;
    dictionary["can_use"] = drill.canUse;
    dictionary["disabled_reason"] = String(drill.disabledReason.c_str());
    return dictionary;
}

Dictionary BattleBridge::event_to_dictionary(const BattleEvent& event) const
{
    Dictionary dictionary;
    dictionary["type"] = event_type_to_string(event.type);
    dictionary["actor"] = actor_to_string(event.actor);
    dictionary["target"] = actor_to_string(event.target);
    dictionary["skill_id"] = String(event.skillId.c_str());
    dictionary["actor_player_index"] = event.actorPlayerIndex;
    dictionary["target_player_index"] = event.targetPlayerIndex;
    dictionary["profile_index"] = event.profileIndex;
    dictionary["target_profile_index"] = event.targetProfileIndex;
    dictionary["actor_name"] = String(event.actorName.c_str());
    dictionary["target_name"] = String(event.targetName.c_str());
    dictionary["old_player_index"] = event.oldPlayerIndex;
    dictionary["new_player_index"] = event.newPlayerIndex;
    dictionary["player_name"] = String(event.playerName.c_str());
    dictionary["old_value"] = event.oldValue;
    dictionary["new_value"] = event.newValue;
    dictionary["amount"] = event.amount;
    dictionary["reason"] = String(event.reason.c_str());
    dictionary["old_level"] = event.oldLevel;
    dictionary["new_level"] = event.newLevel;
    dictionary["winner"] = winner_to_string(event.winner);
    dictionary["damage"] = event.damage.amount;
    dictionary["mark_bonus_damage"] = event.damage.markBonusDamage > 0
        ? event.damage.markBonusDamage
        : event.effect.markBonusDamage;
    dictionary["healing"] = event.effect.healingAmount;
    dictionary["effect"] = effect_to_string(event.effect.type);
    dictionary["duration"] = event.effect.duration;
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

    Dictionary drill;
    drill["used"] = result.drillUse.used;
    drill["actor"] = actor_to_string(result.drillUse.actor);
    drill["id"] = String(result.drillUse.drillId.c_str());
    drill["quality"] = drill_quality_to_string(result.drillUse.quality);
    drill["old_mana"] = result.drillUse.oldMana;
    drill["new_mana"] = result.drillUse.newMana;
    drill["mana_gained"] = result.drillUse.manaGained;
    dictionary["drill"] = drill;

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

void BattleBridge::add_trait_fields(Dictionary& dictionary, const std::string& trait_id) const
{
    bridge_serializers::AddTraitFields(dictionary, trait_id, data_);
}

BattleSetup BattleBridge::setup_from_dictionary(const Dictionary& setup_dictionary) const
{
    BattleSetup setup;
    setup.activePlayerIndex = get_int_field(setup_dictionary, "active_player_index");

    if (setup_dictionary.has("opponent_name"))
    {
        setup.opponentName = get_string_field(setup_dictionary, "opponent_name");
    }

    if (setup_dictionary.has("opponent_spec"))
    {
        setup.opponentSpec = bridge_serializers::SpecFromString(String(setup_dictionary["opponent_spec"]));
    }
    if (setup_dictionary.has("opponent_trait_id"))
    {
        setup.opponentTraitId = get_string_field(setup_dictionary, "opponent_trait_id");
    }

    if (setup_dictionary.has("opponent_hp"))
    {
        setup.opponentMaxHp = get_min_int_field(setup_dictionary, "opponent_hp", 1, setup.opponentMaxHp);
    }

    if (setup_dictionary.has("opponent_mana"))
    {
        setup.opponentMaxMana = get_min_int_field(setup_dictionary, "opponent_mana", 1, setup.opponentMaxMana);
    }
    else if (setup_dictionary.has("opponent_focus"))
    {
        setup.opponentMaxFocus = get_min_int_field(setup_dictionary, "opponent_focus", 1, setup.opponentMaxFocus);
    }

    if (setup_dictionary.has("opponent_base_power_bonus"))
    {
        setup.opponentBasePowerBonus = get_int_field(setup_dictionary, "opponent_base_power_bonus");
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
        slot.profileIndex = get_int_field(player_dictionary, "profile_index", index);
        slot.name = get_string_field(player_dictionary, "name", "Player");
        slot.spec = player_dictionary.has("spec")
            ? bridge_serializers::SpecFromString(String(player_dictionary["spec"]))
            : Spec::Top;
        if (player_dictionary.has("trait_id"))
        {
            slot.traitId = get_string_field(player_dictionary, "trait_id");
        }
        slot.currentHp = get_int_field(player_dictionary, "current_hp", -1);
        if (player_dictionary.has("max_mana"))
        {
            slot.maxMana = get_min_int_field(player_dictionary, "max_mana", 1, slot.maxMana);
        }
        else if (player_dictionary.has("max_focus"))
        {
            slot.maxMana = get_min_int_field(player_dictionary, "max_focus", 1, slot.maxMana);
        }
        if (player_dictionary.has("current_mana"))
        {
            slot.currentMana = get_int_field(player_dictionary, "current_mana");
        }
        else if (player_dictionary.has("current_focus"))
        {
            slot.currentFocus = get_int_field(player_dictionary, "current_focus");
        }

        if (player_dictionary.has("passive_bonuses"))
        {
            Dictionary bonuses = player_dictionary["passive_bonuses"];
            slot.passiveBonuses.maxHpBonus = get_int_field(bonuses, "max_hp_bonus");
            slot.passiveBonuses.basePowerBonus = get_int_field(bonuses, "base_power_bonus");
            slot.passiveBonuses.counterDamageBonusPercent = get_int_field(bonuses, "counter_damage_bonus_percent");
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
                progress.skillId = to_std_string(skill_id);
                progress.level = get_min_int_field(skill_dictionary, "level", 1, 1);
                progress.xp = get_min_int_field(skill_dictionary, "xp", 0, 0);

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
    case BattleEventType::DrillStarted: return "drill_started";
    case BattleEventType::DrillCompleted: return "drill_completed";
    case BattleEventType::ManaChanged: return "mana_changed";
    case BattleEventType::CooldownStarted: return "cooldown_started";
    case BattleEventType::CooldownTicked: return "cooldown_ticked";
    case BattleEventType::CooldownReady: return "cooldown_ready";
    case BattleEventType::ActionBlocked: return "action_blocked";
    case BattleEventType::AttackMissed: return "attack_missed";
    case BattleEventType::DamageApplied: return "damage_applied";
    case BattleEventType::HealingApplied: return "healing_applied";
    case BattleEventType::StatusApplied: return "status_applied";
    case BattleEventType::StatusExpired: return "status_expired";
    case BattleEventType::MarkApplied: return "mark_applied";
    case BattleEventType::MarkTriggered: return "mark_triggered";
    case BattleEventType::MarkExpired: return "mark_expired";
    case BattleEventType::FarmingTriggered: return "farming_triggered";
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
    case SimulationError::UnknownDrill: return "unknown_drill";
    case SimulationError::InsufficientMana: return "insufficient_mana";
    case SimulationError::SkillOnCooldown: return "skill_on_cooldown";
    case SimulationError::ActorStunned: return "actor_stunned";
    case SimulationError::ActorSilenced: return "actor_silenced";
    case SimulationError::ActorRooted: return "actor_rooted";
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
    case SimulationError::InvalidSkillTarget: return "invalid_skill_target";
    }

    return "unknown";
}

DrillResultQuality BattleBridge::drill_quality_from_string(const String& value) const
{
    const String normalized = value.to_lower();
    if (normalized == "miss")
    {
        return DrillResultQuality::Miss;
    }
    if (normalized == "perfect")
    {
        return DrillResultQuality::Perfect;
    }

    return DrillResultQuality::Good;
}

String BattleBridge::drill_quality_to_string(DrillResultQuality quality) const
{
    switch (quality)
    {
    case DrillResultQuality::Miss: return "miss";
    case DrillResultQuality::Perfect: return "perfect";
    case DrillResultQuality::Good: return "good";
    }

    return "good";
}

String BattleBridge::effect_to_string(SkillEffectType effect) const
{
    switch (effect)
    {
    case SkillEffectType::Heal: return "heal";
    case SkillEffectType::AttackModifier: return "attack_modifier";
    case SkillEffectType::DefenseModifier: return "defense_modifier";
    case SkillEffectType::AttackPenetrationModifier: return "attack_penetration_modifier";
    case SkillEffectType::CooldownModifier: return "cooldown_modifier";
    case SkillEffectType::HealingReceivedModifier: return "healing_received_modifier";
    case SkillEffectType::Stunned: return "stunned";
    case SkillEffectType::Silenced: return "silenced";
    case SkillEffectType::Rooted: return "rooted";
    case SkillEffectType::Mark: return "mark";
    case SkillEffectType::None: return "none";
    }

    return "none";
}
