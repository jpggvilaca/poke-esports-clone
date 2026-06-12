#pragma once

#include "Models.h"

#include <string>

class SimulationData;

namespace PlayerProfileBalance
{
    inline constexpr int StartingLevel = 1;
    inline constexpr int StartingXp = 0;
    inline constexpr int StartingSkillCount = 1;
    inline constexpr int MaxActiveSkills = 4;
    inline constexpr int BaseXpForNextLevel = 100;
    inline constexpr int XpGrowthPerLevel = 50;
    inline constexpr int LadderRankLevel = 5;
    inline constexpr int ProRankLevel = 10;
    inline constexpr int EliteRankLevel = 15;
    inline constexpr int WorldClassRankLevel = 20;
    inline constexpr int MaxHpBonusPerLevel = 2;
    inline constexpr int BasePowerBonusLevelInterval = 3;
}

// PlayerProfileSystem owns rules for one controlled esports player: XP,
// rank/evolution, passive bonuses, and the four-skill active loadout.
class PlayerProfileSystem
{
public:
    explicit PlayerProfileSystem(const SimulationData& data);

    PlayerProfileState CreateStarter(
        const std::string& name,
        Spec spec) const;

    bool HasLearnedSkill(const PlayerProfileState& playerProfile, const std::string& skillId) const;
    bool HasActiveSkill(const PlayerProfileState& playerProfile, const std::string& skillId) const;

    ProfileCommandResult LearnSkill(PlayerProfileState& playerProfile, const std::string& skillId) const;
    ProfileCommandResult EquipSkill(PlayerProfileState& playerProfile, const std::string& skillId) const;
    ProfileCommandResult UnequipSkill(PlayerProfileState& playerProfile, const std::string& skillId) const;
    ProfileCommandResult AwardXp(PlayerProfileState& playerProfile, int amount) const;

    int GetXpRequiredForLevel(int level) const;
    CareerRank GetRankForLevel(int level) const;
    PassiveBonuses GetPassiveBonusesForRank(CareerRank rank) const;
    PassiveBonuses GetPassiveBonusesForLevel(int level) const;
    PassiveBonuses GetPassiveBonusesForLevel(int level, Spec spec) const;

private:
    void RefreshXpRequirement(PlayerProfileState& playerProfile) const;
    void RefreshRank(PlayerProfileState& playerProfile) const;

    const SimulationData& data_;
};
