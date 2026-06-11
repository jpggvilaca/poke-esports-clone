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
    return bridge_serializers::BattleResultToDictionary(last_result_, data_);
}

Dictionary BattleBridge::use_skill(const String& skill_id, int target_player_index)
{
    if (session_ == nullptr)
    {
        last_result_ = reject_action(SimulationError::BattleNotStarted, "Start a battle first.");
        return bridge_serializers::BattleResultToDictionary(last_result_, data_);
    }

    last_result_ = session_->UsePlayerSkill(to_std_string(skill_id), target_player_index);
    return bridge_serializers::BattleResultToDictionary(last_result_, data_);
}

Dictionary BattleBridge::use_drill(const String& result_quality)
{
    if (session_ == nullptr)
    {
        last_result_ = reject_action(SimulationError::BattleNotStarted, "Start a battle first.");
        return bridge_serializers::BattleResultToDictionary(last_result_, data_);
    }

    last_result_ = session_->UsePlayerDrill(drill_quality_from_string(result_quality));
    return bridge_serializers::BattleResultToDictionary(last_result_, data_);
}

Dictionary BattleBridge::pass_turn()
{
    if (session_ == nullptr)
    {
        last_result_ = reject_action(SimulationError::BattleNotStarted, "Start a battle first.");
        return bridge_serializers::BattleResultToDictionary(last_result_, data_);
    }

    last_result_ = session_->PassPlayerTurn();
    return bridge_serializers::BattleResultToDictionary(last_result_, data_);
}

Dictionary BattleBridge::get_battle_state() const
{
    if (session_ == nullptr)
    {
        return bridge_serializers::BattleStateToDictionary(BattleState{}, data_);
    }

    return bridge_serializers::BattleStateToDictionary(session_->GetState(), data_);
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
        skills.push_back(bridge_serializers::SkillToDictionary(skill));
    }

    return skills;
}

Dictionary BattleBridge::get_drill_action() const
{
    if (session_ == nullptr)
    {
        return bridge_serializers::DrillToDictionary({});
    }

    return bridge_serializers::DrillToDictionary(session_->GetPlayerDrill());
}

Dictionary BattleBridge::get_last_result() const
{
    return bridge_serializers::BattleResultToDictionary(last_result_, data_);
}

BattleActionResult BattleBridge::reject_action(SimulationError error_code, const std::string& error) const
{
    BattleActionResult result;
    result.errorCode = error_code;
    result.error = error;
    result.finalState = BattleState{};
    return result;
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
