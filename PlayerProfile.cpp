#include "PlayerProfile.h"

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

    ProfileCommandResult Reject(const std::string& error)
    {
        ProfileCommandResult result;
        result.error = error;
        return result;
    }
}

PlayerProfile PlayerProfile::CreateNew(
    const std::string& playerName,
    GameType gameType,
    Spec spec,
    const SimulationData& data)
{
    PlayerProfile profile;
    profile.state_.playerName = playerName;
    profile.state_.gameType = gameType;
    profile.state_.spec = spec;
    profile.state_.level = ProfileBalance::StartingLevel;
    profile.state_.xp = ProfileBalance::StartingXp;
    profile.RefreshXpRequirement();
    profile.RefreshRank();
    profile.state_.rating = ProfileBalance::StartingRating;
    profile.state_.money = ProfileBalance::StartingMoney;

    const SpecData* specData = data.FindSpec(spec);
    if (specData == nullptr)
    {
        return profile;
    }

    for (int index = 0;
        index < ProfileBalance::StartingSkillCount && index < static_cast<int>(specData->skillIds.size());
        ++index)
    {
        profile.state_.learnedSkillIds.push_back(specData->skillIds[index]);
        profile.state_.activeSkillIds.push_back(specData->skillIds[index]);
    }

    return profile;
}

const PlayerProfileState& PlayerProfile::GetState() const
{
    return state_;
}

PassiveBonuses PlayerProfile::GetPassiveBonuses() const
{
    return state_.passiveBonuses;
}

bool PlayerProfile::HasLearnedSkill(const std::string& skillId) const
{
    return Contains(state_.learnedSkillIds, skillId);
}

bool PlayerProfile::HasActiveSkill(const std::string& skillId) const
{
    return Contains(state_.activeSkillIds, skillId);
}

ProfileCommandResult PlayerProfile::LearnSkill(const std::string& skillId)
{
    if (HasLearnedSkill(skillId))
    {
        return Reject("Skill already learned.");
    }

    state_.learnedSkillIds.push_back(skillId);
    return Accept();
}

ProfileCommandResult PlayerProfile::EquipSkill(const std::string& skillId)
{
    if (!HasLearnedSkill(skillId))
    {
        return Reject("Skill is not learned.");
    }

    if (HasActiveSkill(skillId))
    {
        return Reject("Skill is already active.");
    }

    if (static_cast<int>(state_.activeSkillIds.size()) >= ProfileBalance::MaxActiveSkills)
    {
        return Reject("Active skill slots are full.");
    }

    state_.activeSkillIds.push_back(skillId);
    return Accept();
}

ProfileCommandResult PlayerProfile::UnequipSkill(const std::string& skillId)
{
    const auto found = std::find(state_.activeSkillIds.begin(), state_.activeSkillIds.end(), skillId);
    if (found == state_.activeSkillIds.end())
    {
        return Reject("Skill is not active.");
    }

    state_.activeSkillIds.erase(found);
    return Accept();
}

ProfileCommandResult PlayerProfile::AwardPlayerXp(int amount)
{
    if (amount < 0)
    {
        return Reject("XP award cannot be negative.");
    }

    ProfileCommandResult result = Accept();
    result.oldValue = state_.xp;
    result.oldLevel = state_.level;

    state_.xp += amount;
    while (state_.xp >= GetXpRequiredForLevel(state_.level))
    {
        state_.xp -= GetXpRequiredForLevel(state_.level);
        ++state_.level;
        result.leveledUp = true;
    }

    RefreshXpRequirement();
    RefreshRank();
    result.newValue = state_.xp;
    result.newLevel = state_.level;
    return result;
}

ProfileCommandResult PlayerProfile::AwardRating(int amount)
{
    ProfileCommandResult result = Accept();
    result.oldValue = state_.rating;
    state_.rating = std::max(0, state_.rating + amount);
    result.newValue = state_.rating;
    return result;
}

ProfileCommandResult PlayerProfile::AwardMoney(int amount)
{
    ProfileCommandResult result = Accept();
    result.oldValue = state_.money;
    state_.money = std::max(0, state_.money + amount);
    result.newValue = state_.money;
    return result;
}

ProfileCommandResult PlayerProfile::AddTrophy(const std::string& trophyId)
{
    if (Contains(state_.trophyIds, trophyId))
    {
        return Reject("Trophy already earned.");
    }

    state_.trophyIds.push_back(trophyId);
    return Accept();
}

int PlayerProfile::GetXpRequiredForLevel(int level)
{
    const int safeLevel = std::max(1, level);
    return ProfileBalance::BaseXpForNextLevel
        + (safeLevel - 1) * ProfileBalance::XpGrowthPerLevel;
}

CareerRank PlayerProfile::GetRankForLevel(int level)
{
    if (level >= ProfileBalance::WorldClassRankLevel)
    {
        return CareerRank::WorldClass;
    }

    if (level >= ProfileBalance::EliteRankLevel)
    {
        return CareerRank::Elite;
    }

    if (level >= ProfileBalance::ProRankLevel)
    {
        return CareerRank::Pro;
    }

    if (level >= ProfileBalance::LadderRankLevel)
    {
        return CareerRank::Ladder;
    }

    return CareerRank::Rookie;
}

PassiveBonuses PlayerProfile::GetPassiveBonusesForRank(CareerRank rank)
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

void PlayerProfile::RefreshXpRequirement()
{
    state_.xpRequiredForNextLevel = GetXpRequiredForLevel(state_.level);
}

void PlayerProfile::RefreshRank()
{
    state_.rank = GetRankForLevel(state_.level);
    state_.passiveBonuses = GetPassiveBonusesForRank(state_.rank);
}
