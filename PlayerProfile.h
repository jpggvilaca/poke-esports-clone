#pragma once

#include "Models.h"

#include <string>

class SimulationData;

namespace ProfileBalance
{
    inline constexpr int StartingLevel = 1;
    inline constexpr int StartingXp = 0;
    inline constexpr int StartingRating = 1000;
    inline constexpr int StartingMoney = 0;
    inline constexpr int StartingSkillCount = 4;
    inline constexpr int MaxActiveSkills = 4;
    inline constexpr int BaseXpForNextLevel = 100;
    inline constexpr int XpGrowthPerLevel = 50;
    inline constexpr int LadderRankLevel = 5;
    inline constexpr int ProRankLevel = 10;
    inline constexpr int EliteRankLevel = 15;
    inline constexpr int WorldClassRankLevel = 20;
}

class PlayerProfile
{
public:
    static PlayerProfile CreateNew(
        const std::string& playerName,
        GameType gameType,
        Spec spec,
        const SimulationData& data);

    const PlayerProfileState& GetState() const;
    PassiveBonuses GetPassiveBonuses() const;

    bool HasLearnedSkill(const std::string& skillId) const;
    bool HasActiveSkill(const std::string& skillId) const;

    ProfileCommandResult LearnSkill(const std::string& skillId);
    ProfileCommandResult EquipSkill(const std::string& skillId);
    ProfileCommandResult UnequipSkill(const std::string& skillId);
    ProfileCommandResult AwardPlayerXp(int amount);
    ProfileCommandResult AwardRating(int amount);
    ProfileCommandResult AwardMoney(int amount);
    ProfileCommandResult AddTrophy(const std::string& trophyId);

private:
    static int GetXpRequiredForLevel(int level);
    static CareerRank GetRankForLevel(int level);
    static PassiveBonuses GetPassiveBonusesForRank(CareerRank rank);
    void RefreshXpRequirement();
    void RefreshRank();

    PlayerProfileState state_;
};
