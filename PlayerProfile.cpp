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
    while (state_.xp >= ProfileBalance::PlayerXpPerLevel)
    {
        state_.xp -= ProfileBalance::PlayerXpPerLevel;
        ++state_.level;
        result.leveledUp = true;
    }

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
