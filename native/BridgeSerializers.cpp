#include "BridgeSerializers.h"

#include "BridgeDictionaryUtils.h"
#include "BattleRules.h"
#include "PlayerProfileSystem.h"
#include "ProgressionSystem.h"
#include "SkillSystem.h"

#include <algorithm>

using godot::Array;
using godot::Dictionary;
using godot::String;
using bridge_dictionary::get_clamped_int_field;
using bridge_dictionary::get_int_field;
using bridge_dictionary::get_min_int_field;
using bridge_dictionary::get_string_field;
using bridge_dictionary::to_std_string;

namespace
{
String EffectToString(SkillEffectType effect)
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

String TargetToString(SkillEffectTarget target)
{
    switch (target)
    {
    case SkillEffectTarget::Self: return "self";
    case SkillEffectTarget::Ally: return "ally";
    case SkillEffectTarget::Enemy: return "enemy";
    case SkillEffectTarget::PlayerLineup: return "player_lineup";
    case SkillEffectTarget::Opponent: return "enemy";
    }

    return "self";
}

String ActorToString(BattleActor actor)
{
    switch (actor)
    {
    case BattleActor::Player: return "player";
    case BattleActor::Opponent: return "opponent";
    case BattleActor::None: return "none";
    }

    return "none";
}

String WinnerToString(BattleWinner winner)
{
    switch (winner)
    {
    case BattleWinner::Player: return "player";
    case BattleWinner::Opponent: return "opponent";
    case BattleWinner::None: return "none";
    }

    return "none";
}

String EventTypeToString(BattleEventType type)
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

String ErrorToString(SimulationError error)
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

String DrillQualityToString(DrillResultQuality quality)
{
    switch (quality)
    {
    case DrillResultQuality::Miss: return "miss";
    case DrillResultQuality::Perfect: return "perfect";
    case DrillResultQuality::Good: return "good";
    }

    return "good";
}
}

namespace bridge_serializers
{
std::vector<std::string> StringVectorFromArray(const Array& values)
{
    std::vector<std::string> output;
    for (int index = 0; index < values.size(); ++index)
    {
        output.push_back(to_std_string(String(values[index])));
    }
    return output;
}

Spec SpecFromString(const String& value)
{
    const std::string text = to_std_string(value);
    if (text == "Jungle" || text == "jungle") return Spec::Jungle;
    if (text == "Mid" || text == "mid") return Spec::Mid;
    if (text == "ADC" || text == "Adc" || text == "adc") return Spec::Adc;
    if (text == "Support" || text == "support") return Spec::Support;
    return Spec::Top;
}

MatchContext MatchContextFromString(const String& value)
{
    const std::string text = to_std_string(value);
    if (text == "Tutorial" || text == "tutorial") return MatchContext::Tutorial;
    if (text == "Nemesis" || text == "nemesis") return MatchContext::Nemesis;
    if (text == "Major" || text == "major") return MatchContext::Major;
    return MatchContext::Normal;
}

Dictionary PassiveBonusesToDictionary(const PassiveBonuses& bonuses)
{
    Dictionary dictionary;
    dictionary["max_hp_bonus"] = bonuses.maxHpBonus;
    dictionary["base_power_bonus"] = bonuses.basePowerBonus;
    dictionary["counter_damage_bonus_percent"] = bonuses.counterDamageBonusPercent;
    return dictionary;
}

void AddTraitFields(Dictionary& dictionary, const std::string& trait_id, const SimulationData& data)
{
    const TraitDefinition* trait = data.FindTrait(trait_id);
    dictionary["trait_id"] = trait == nullptr ? String("") : String(trait->id.c_str());
    dictionary["trait_name"] = trait == nullptr ? String("") : String(trait->name.c_str());
    dictionary["trait_description"] = trait == nullptr ? String("") : String(trait->description.c_str());
}

Dictionary PlayerProfileToDictionary(const PlayerProfileState& player_profile, const SimulationData& data)
{
    Dictionary dictionary;
    dictionary["name"] = String(player_profile.name.c_str());
    dictionary["spec"] = String(ToString(player_profile.spec).c_str());
    AddTraitFields(dictionary, player_profile.traitId, data);
    dictionary["rank"] = String(ToString(player_profile.rank).c_str());
    dictionary["level"] = player_profile.level;
    dictionary["xp"] = player_profile.xp;
    dictionary["xp_required_for_next_level"] = player_profile.xpRequiredForNextLevel;
    dictionary["passive_bonuses"] = PassiveBonusesToDictionary(player_profile.passiveBonuses);
    dictionary["max_hp"] = Balance::StartingMaxHp + player_profile.passiveBonuses.maxHpBonus;
    dictionary["max_mana"] = Balance::StartingMaxMana;

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

PlayerProfileState PlayerProfileFromDictionary(const Dictionary& player_profile_dictionary, const SimulationData& data)
{
    PlayerProfileState player_profile;
    player_profile.name = get_string_field(player_profile_dictionary, "name", "Player");
    player_profile.spec = player_profile_dictionary.has("spec")
        ? SpecFromString(String(player_profile_dictionary["spec"]))
        : Spec::Top;
    if (player_profile_dictionary.has("trait_id"))
    {
        player_profile.traitId = get_string_field(player_profile_dictionary, "trait_id");
    }
    player_profile.level = get_min_int_field(player_profile_dictionary, "level", 1, 1);
    player_profile.xp = get_min_int_field(player_profile_dictionary, "xp", 0, 0);
    player_profile.xpRequiredForNextLevel = get_min_int_field(
        player_profile_dictionary,
        "xp_required_for_next_level",
        1,
        PlayerProfileBalance::BaseXpForNextLevel);

    if (player_profile_dictionary.has("learned_skill_ids"))
    {
        Array learned_skills = player_profile_dictionary["learned_skill_ids"];
        for (int index = 0; index < learned_skills.size(); ++index)
        {
            player_profile.learnedSkillIds.push_back(to_std_string(String(learned_skills[index])));
        }
    }

    if (player_profile_dictionary.has("active_skill_ids"))
    {
        Array active_skills = player_profile_dictionary["active_skill_ids"];
        for (int index = 0; index < active_skills.size(); ++index)
        {
            player_profile.activeSkillIds.push_back(to_std_string(String(active_skills[index])));
        }
    }

    PlayerProfileSystem profiles(data);
    player_profile.rank = profiles.GetRankForLevel(player_profile.level);
    player_profile.passiveBonuses = profiles.GetPassiveBonusesForLevel(player_profile.level, player_profile.spec);
    if (data.FindTrait(player_profile.traitId) == nullptr)
    {
        const SpecData* spec_data = data.FindSpec(player_profile.spec);
        player_profile.traitId = spec_data == nullptr ? "" : spec_data->defaultTraitId;
    }
    return player_profile;
}

Dictionary PlayerProfileToRosterDictionary(const PlayerProfileState& player_profile, const SimulationData& data)
{
    Dictionary dictionary = PlayerProfileToDictionary(player_profile, data);
    dictionary["current_hp"] = dictionary.get("max_hp", Balance::StartingMaxHp);
    dictionary["current_mana"] = 0;

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

Competitor CompetitorFromPlayerProfile(const Dictionary& player_profile, const SimulationData& data)
{
    Competitor competitor;
    competitor.name = get_string_field(player_profile, "name", "Player");
    competitor.spec = player_profile.has("spec")
        ? SpecFromString(String(player_profile["spec"]))
        : Spec::Top;
    competitor.traitId = get_string_field(player_profile, "trait_id");
    if (data.FindTrait(competitor.traitId) == nullptr)
    {
        const SpecData* spec_data = data.FindSpec(competitor.spec);
        competitor.traitId = spec_data == nullptr ? "" : spec_data->defaultTraitId;
    }

    PassiveBonuses bonuses;
    if (player_profile.has("passive_bonuses"))
    {
        Dictionary passive_bonuses = player_profile["passive_bonuses"];
        bonuses.maxHpBonus = get_int_field(passive_bonuses, "max_hp_bonus");
        bonuses.basePowerBonus = get_int_field(passive_bonuses, "base_power_bonus");
        bonuses.counterDamageBonusPercent = get_int_field(passive_bonuses, "counter_damage_bonus_percent");
    }

    competitor.maxHp = Balance::StartingMaxHp + bonuses.maxHpBonus;
    competitor.hp = get_clamped_int_field(player_profile, "current_hp", 0, competitor.maxHp, competitor.maxHp);
    competitor.maxMana = get_min_int_field(player_profile, "max_mana", 1, Balance::StartingMaxMana);
    if (player_profile.has("current_mana"))
    {
        competitor.mana = get_clamped_int_field(player_profile, "current_mana", 0, competitor.maxMana, 0);
    }
    else if (player_profile.has("current_focus"))
    {
        competitor.mana = get_clamped_int_field(player_profile, "current_focus", 0, competitor.maxMana, 0);
    }
    else
    {
        competitor.mana = 0;
    }
    competitor.basePower = Balance::StartingBasePower + bonuses.basePowerBonus;
    competitor.counterDamageBonusPercent = bonuses.counterDamageBonusPercent;
    return competitor;
}

Dictionary SkillToDictionary(const SkillView& skill)
{
    Dictionary dictionary;
    dictionary["id"] = String(skill.id.c_str());
    dictionary["name"] = String(skill.name.c_str());
    dictionary["description"] = String(skill.description.c_str());
    dictionary["tone"] = String(ToString(skill.tone).c_str());
    dictionary["mana_cost"] = skill.manaCost;
    dictionary["mana_gain"] = skill.manaGain;
    dictionary["cooldown_turns"] = skill.cooldownTurns;
    dictionary["cooldown_remaining"] = skill.cooldownRemaining;
    dictionary["can_use"] = skill.canUse;
    dictionary["disabled_reason"] = String(skill.disabledReason.c_str());
    dictionary["accuracy"] = skill.accuracy;
    dictionary["level"] = skill.level;
    dictionary["xp"] = skill.xp;
    dictionary["effect"] = EffectToString(skill.effectType);
    dictionary["effect_target"] = TargetToString(skill.effectTarget);
    dictionary["effect_value"] = skill.effectValue;
    dictionary["duration_turns"] = skill.durationTurns;
    dictionary["mark_bonus_damage"] = skill.markBonusDamage;

    Array effects;
    for (const SkillEffectDefinition& effect : skill.effects)
    {
        Dictionary effect_dictionary;
        effect_dictionary["effect"] = EffectToString(effect.type);
        effect_dictionary["effect_target"] = TargetToString(effect.target);
        effect_dictionary["effect_value"] = effect.value;
        effect_dictionary["duration_turns"] = effect.durationTurns;
        effect_dictionary["mark_bonus_damage"] = effect.markBonusDamage;
        effects.push_back(effect_dictionary);
    }
    dictionary["effects"] = effects;
    return dictionary;
}

Dictionary DrillToDictionary(const DrillView& drill)
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

Dictionary BattleRewardToDictionary(const BattleRewardResult& reward)
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

Dictionary CompetitorToDictionary(const CompetitorView& competitor, const SimulationData& data)
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
    status["mark_source"] = ActorToString(competitor.status.markSource);

    dictionary["profile_index"] = competitor.profileIndex;
    dictionary["name"] = String(competitor.name.c_str());
    dictionary["spec"] = String(ToString(competitor.spec).c_str());
    AddTraitFields(dictionary, competitor.traitId, data);
    dictionary["hp"] = competitor.hp;
    dictionary["max_hp"] = competitor.maxHp;
    dictionary["mana"] = competitor.mana;
    dictionary["max_mana"] = competitor.maxMana;
    dictionary["base_power"] = competitor.basePower;
    dictionary["counter_damage_bonus_percent"] = competitor.counterDamageBonusPercent;
    dictionary["status"] = status;
    return dictionary;
}

Dictionary BattleStateToDictionary(const BattleState& state, const SimulationData& data)
{
    Dictionary dictionary;
    dictionary["started"] = state.started;
    dictionary["finished"] = state.finished;
    dictionary["winner"] = WinnerToString(state.winner);
    dictionary["active_player_index"] = state.activePlayerIndex;
    dictionary["active_opponent_index"] = state.activeOpponentIndex;
    dictionary["player"] = CompetitorToDictionary(state.player, data);
    dictionary["opponent"] = CompetitorToDictionary(state.opponent, data);

    Array player_team;
    for (const CompetitorView& player : state.playerTeam)
    {
        player_team.push_back(CompetitorToDictionary(player, data));
    }
    dictionary["player_team"] = player_team;

    Array opponent_team;
    for (const CompetitorView& opponent : state.opponentTeam)
    {
        opponent_team.push_back(CompetitorToDictionary(opponent, data));
    }
    dictionary["opponent_team"] = opponent_team;
    return dictionary;
}

Dictionary BattleEventToDictionary(const BattleEvent& event)
{
    Dictionary dictionary;
    dictionary["type"] = EventTypeToString(event.type);
    dictionary["actor"] = ActorToString(event.actor);
    dictionary["target"] = ActorToString(event.target);
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
    dictionary["winner"] = WinnerToString(event.winner);
    dictionary["damage"] = event.damage.amount;
    dictionary["mark_bonus_damage"] = event.damage.markBonusDamage > 0
        ? event.damage.markBonusDamage
        : event.effect.markBonusDamage;
    dictionary["healing"] = event.effect.healingAmount;
    dictionary["effect"] = EffectToString(event.effect.type);
    dictionary["duration"] = event.effect.duration;
    dictionary["reward"] = BattleRewardToDictionary(event.reward);
    return dictionary;
}

Dictionary BattleResultToDictionary(const BattleActionResult& result, const SimulationData& data)
{
    Dictionary dictionary;
    dictionary["accepted"] = result.accepted;
    dictionary["error_code"] = ErrorToString(result.errorCode);
    dictionary["error"] = String(result.error.c_str());
    dictionary["battle_started"] = result.battleStarted;
    dictionary["battle_finished"] = result.battleFinished;
    dictionary["winner"] = WinnerToString(result.winner);
    dictionary["state"] = BattleStateToDictionary(result.finalState, data);
    dictionary["reward"] = BattleRewardToDictionary(result.reward);

    Dictionary drill;
    drill["used"] = result.drillUse.used;
    drill["actor"] = ActorToString(result.drillUse.actor);
    drill["id"] = String(result.drillUse.drillId.c_str());
    drill["quality"] = DrillQualityToString(result.drillUse.quality);
    drill["old_mana"] = result.drillUse.oldMana;
    drill["new_mana"] = result.drillUse.newMana;
    drill["mana_gained"] = result.drillUse.manaGained;
    dictionary["drill"] = drill;

    Array events;
    for (const BattleEvent& event : result.events)
    {
        events.push_back(BattleEventToDictionary(event));
    }
    dictionary["events"] = events;
    return dictionary;
}

Dictionary ScoutOfferToDictionary(const ScoutOfferView& offer, const SimulationData& data)
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
        candidates.push_back(PlayerProfileToRosterDictionary(candidate, data));
    }
    dictionary["candidates"] = candidates;
    return dictionary;
}

ScoutOfferView ScoutOfferFromDictionary(const Dictionary& offer_dictionary, const SimulationData& data)
{
    ScoutOfferView offer;
    offer.id = get_string_field(offer_dictionary, "id");
    offer.available = !offer.id.empty();
    offer.requiredRating = get_int_field(offer_dictionary, "required_rating");
    offer.message = get_string_field(offer_dictionary, "message");

    if (offer_dictionary.has("candidates"))
    {
        Array candidates = offer_dictionary["candidates"];
        for (int index = 0; index < candidates.size(); ++index)
        {
            offer.candidates.push_back(PlayerProfileFromDictionary(Dictionary(candidates[index]), data));
        }
    }
    return offer;
}
}
