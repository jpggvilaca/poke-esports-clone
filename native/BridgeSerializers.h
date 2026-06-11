#pragma once

#include "Models.h"
#include "ScoutSystem.h"
#include "SimulationData.h"

#include <godot_cpp/variant/array.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/string.hpp>

#include <string>
#include <vector>

namespace bridge_serializers
{
std::vector<std::string> StringVectorFromArray(const godot::Array& values);
Spec SpecFromString(const godot::String& value);
MatchContext MatchContextFromString(const godot::String& value);
godot::Dictionary PassiveBonusesToDictionary(const PassiveBonuses& bonuses);
void AddTraitFields(godot::Dictionary& dictionary, const std::string& trait_id, const SimulationData& data);
godot::Dictionary PlayerProfileToDictionary(const PlayerProfileState& player_profile, const SimulationData& data);
PlayerProfileState PlayerProfileFromDictionary(const godot::Dictionary& player_profile, const SimulationData& data);
godot::Dictionary PlayerProfileToRosterDictionary(const PlayerProfileState& player_profile, const SimulationData& data);
Competitor CompetitorFromPlayerProfile(const godot::Dictionary& player_profile, const SimulationData& data);
godot::Dictionary SkillToDictionary(const SkillView& skill);
godot::Dictionary DrillToDictionary(const DrillView& drill);
godot::Dictionary BattleStateToDictionary(const BattleState& state, const SimulationData& data);
godot::Dictionary BattleResultToDictionary(const BattleActionResult& result, const SimulationData& data);
godot::Dictionary ScoutOfferToDictionary(const ScoutOfferView& offer, const SimulationData& data);
ScoutOfferView ScoutOfferFromDictionary(const godot::Dictionary& offer_dictionary, const SimulationData& data);
}
