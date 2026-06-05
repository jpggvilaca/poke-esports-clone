#include "PlayerProfileSystem.h"

#include "SimulationData.h"

#include <algorithm>

namespace
{
    bool Contains(const std::vector<std::string>& values, const std::string& value)
    {
        return std::find(values.begin(), values.end(), value) != values.end();
    }

    ProfileCommandResult Accept()
    {
        ProfileCommandResult result;
        result.accepted = true;
        return result;
    }

    ProfileCommandResult Reject(SimulationError errorCode, const std::string& error)
    {
        ProfileCommandResult result;
        result.errorCode = errorCode;
        result.error = error;
        return result;
    }
}

PlayerProfileSystem::PlayerProfileSystem(const SimulationData& data)
    : data_(data)
{
}

PlayerProfileState PlayerProfileSystem::CreateStarter(
    const std::string& name,
    Spec spec) const
{
    PlayerProfileState playerProfile;
    playerProfile.name = name;
    playerProfile.spec = spec;
    playerProfile.level = PlayerProfileBalance::StartingLevel;
    playerProfile.xp = PlayerProfileBalance::StartingXp;
    RefreshXpRequirement(playerProfile);
    RefreshRank(playerProfile);

    const SpecData* specData = data_.FindSpec(spec);
    if (specData == nullptr)
    {
        return playerProfile;
    }

    for (int index = 0;
        index < PlayerProfileBalance::StartingSkillCount && index < static_cast<int>(specData->skillIds.size());
        ++index)
    {
        playerProfile.learnedSkillIds.push_back(specData->skillIds[index]);
        playerProfile.activeSkillIds.push_back(specData->skillIds[index]);
    }

    return playerProfile;
}

bool PlayerProfileSystem::HasLearnedSkill(
    const PlayerProfileState& playerProfile,
    const std::string& skillId) const
{
    return Contains(playerProfile.learnedSkillIds, skillId);
}

bool PlayerProfileSystem::HasActiveSkill(
    const PlayerProfileState& playerProfile,
    const std::string& skillId) const
{
    return Contains(playerProfile.activeSkillIds, skillId);
}

ProfileCommandResult PlayerProfileSystem::LearnSkill(
    PlayerProfileState& playerProfile,
    const std::string& skillId) const
{
    if (HasLearnedSkill(playerProfile, skillId))
    {
        return Reject(SimulationError::SkillAlreadyLearned, "Skill already learned.");
    }

    playerProfile.learnedSkillIds.push_back(skillId);
    return Accept();
}

ProfileCommandResult PlayerProfileSystem::EquipSkill(
    PlayerProfileState& playerProfile,
    const std::string& skillId) const
{
    if (!HasLearnedSkill(playerProfile, skillId))
    {
        return Reject(SimulationError::SkillNotLearned, "Skill is not learned.");
    }

    if (HasActiveSkill(playerProfile, skillId))
    {
        return Reject(SimulationError::SkillAlreadyActive, "Skill is already active.");
    }

    if (static_cast<int>(playerProfile.activeSkillIds.size()) >= PlayerProfileBalance::MaxActiveSkills)
    {
        return Reject(SimulationError::ActiveSkillSlotsFull, "Active skill slots are full.");
    }

    playerProfile.activeSkillIds.push_back(skillId);
    return Accept();
}

ProfileCommandResult PlayerProfileSystem::UnequipSkill(
    PlayerProfileState& playerProfile,
    const std::string& skillId) const
{
    const auto found = std::find(playerProfile.activeSkillIds.begin(), playerProfile.activeSkillIds.end(), skillId);
    if (found == playerProfile.activeSkillIds.end())
    {
        return Reject(SimulationError::SkillNotActive, "Skill is not active.");
    }

    playerProfile.activeSkillIds.erase(found);
    return Accept();
}

ProfileCommandResult PlayerProfileSystem::AwardXp(
    PlayerProfileState& playerProfile,
    int amount) const
{
    if (amount < 0)
    {
        return Reject(SimulationError::NegativeXpAward, "XP award cannot be negative.");
    }

    ProfileCommandResult result = Accept();
    result.oldValue = playerProfile.xp;
    result.oldLevel = playerProfile.level;

    playerProfile.xp += amount;
    while (playerProfile.xp >= GetXpRequiredForLevel(playerProfile.level))
    {
        playerProfile.xp -= GetXpRequiredForLevel(playerProfile.level);
        ++playerProfile.level;
        result.leveledUp = true;
    }

    RefreshXpRequirement(playerProfile);
    RefreshRank(playerProfile);
    result.newValue = playerProfile.xp;
    result.newLevel = playerProfile.level;
    return result;
}

int PlayerProfileSystem::GetXpRequiredForLevel(int level) const
{
    const int safeLevel = std::max(1, level);
    return PlayerProfileBalance::BaseXpForNextLevel
        + (safeLevel - 1) * PlayerProfileBalance::XpGrowthPerLevel;
}

CareerRank PlayerProfileSystem::GetRankForLevel(int level) const
{
    if (level >= PlayerProfileBalance::WorldClassRankLevel)
    {
        return CareerRank::WorldClass;
    }

    if (level >= PlayerProfileBalance::EliteRankLevel)
    {
        return CareerRank::Elite;
    }

    if (level >= PlayerProfileBalance::ProRankLevel)
    {
        return CareerRank::Pro;
    }

    if (level >= PlayerProfileBalance::LadderRankLevel)
    {
        return CareerRank::Ladder;
    }

    return CareerRank::Rookie;
}

PassiveBonuses PlayerProfileSystem::GetPassiveBonusesForRank(CareerRank rank) const
{
    PassiveBonuses bonuses;
    switch (rank)
    {
    case CareerRank::Rookie:
        return bonuses;
    case CareerRank::Ladder:
        bonuses.maxHpBonus = 10;
        return bonuses;
    case CareerRank::Pro:
        bonuses.maxHpBonus = 10;
        bonuses.counterDamageBonusPercent = 5;
        return bonuses;
    case CareerRank::Elite:
        bonuses.maxHpBonus = 20;
        bonuses.counterDamageBonusPercent = 10;
        return bonuses;
    case CareerRank::WorldClass:
        bonuses.maxHpBonus = 30;
        bonuses.counterDamageBonusPercent = 15;
        return bonuses;
    }

    return bonuses;
}

void PlayerProfileSystem::RefreshXpRequirement(PlayerProfileState& playerProfile) const
{
    playerProfile.xpRequiredForNextLevel = GetXpRequiredForLevel(playerProfile.level);
}

void PlayerProfileSystem::RefreshRank(PlayerProfileState& playerProfile) const
{
    playerProfile.rank = GetRankForLevel(playerProfile.level);
    playerProfile.passiveBonuses = GetPassiveBonusesForRank(playerProfile.rank);
}
