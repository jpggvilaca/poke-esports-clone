#include "TrainerProfile.h"

#include "CollectionUtils.h"
#include "PlayerProfileSystem.h"

#include <algorithm>

namespace
{
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

TrainerProfile TrainerProfile::CreateNew(
    const std::string& trainerName,
    GameType gameType,
    Spec starterSpec,
    const PlayerProfileSystem& playerProfiles)
{
    TrainerProfile profile;
    profile.state_.trainerName = trainerName;
    profile.state_.gameType = gameType;
    profile.state_.rating = TrainerBalance::StartingRating;
    profile.state_.money = TrainerBalance::StartingMoney;
    profile.state_.activePlayerIndex = 0;
    profile.state_.roster.push_back(playerProfiles.CreateStarter("Starter", starterSpec));

    return profile;
}

TrainerProfile TrainerProfile::FromState(const TrainerProfileState& state)
{
    TrainerProfile profile;
    profile.state_ = state;
    return profile;
}

const TrainerProfileState& TrainerProfile::GetState() const
{
    return state_;
}

const PlayerProfileState* TrainerProfile::GetActivePlayerProfile() const
{
    if (state_.activePlayerIndex < 0
        || state_.activePlayerIndex >= static_cast<int>(state_.roster.size()))
    {
        return nullptr;
    }

    return &state_.roster[state_.activePlayerIndex];
}

PlayerProfileState* TrainerProfile::GetMutableActivePlayerProfile()
{
    return GetMutablePlayerProfile(state_.activePlayerIndex);
}

PlayerProfileState* TrainerProfile::GetMutablePlayerProfile(int playerIndex)
{
    if (playerIndex < 0 || playerIndex >= static_cast<int>(state_.roster.size()))
    {
        return nullptr;
    }

    return &state_.roster[playerIndex];
}

PassiveBonuses TrainerProfile::GetActivePlayerPassiveBonuses() const
{
    const PlayerProfileState* active = GetActivePlayerProfile();
    return active == nullptr ? PassiveBonuses{} : active->passiveBonuses;
}

int TrainerProfile::GetActivePlayerLevel() const
{
    const PlayerProfileState* active = GetActivePlayerProfile();
    return active == nullptr ? 1 : active->level;
}

ProfileCommandResult TrainerProfile::AwardRating(int amount)
{
    ProfileCommandResult result = Accept();
    result.oldValue = state_.rating;
    state_.rating = std::max(0, state_.rating + amount);
    result.newValue = state_.rating;
    return result;
}

ProfileCommandResult TrainerProfile::AwardMoney(int amount)
{
    ProfileCommandResult result = Accept();
    result.oldValue = state_.money;
    state_.money = std::max(0, state_.money + amount);
    result.newValue = state_.money;
    return result;
}

ProfileCommandResult TrainerProfile::SetActivePlayerIndex(int playerIndex)
{
    if (playerIndex < 0 || playerIndex >= static_cast<int>(state_.roster.size()))
    {
        return Reject(SimulationError::UnknownPlayerProfile, "Unknown player profile.");
    }

    ProfileCommandResult result = Accept();
    result.oldValue = state_.activePlayerIndex;
    state_.activePlayerIndex = playerIndex;
    result.newValue = state_.activePlayerIndex;
    return result;
}

ProfileCommandResult TrainerProfile::AddTrophy(const std::string& trophyId)
{
    if (ContainsValue(state_.trophyIds, trophyId))
    {
        return Reject(SimulationError::TrophyAlreadyEarned, "Trophy already earned.");
    }

    state_.trophyIds.push_back(trophyId);
    return Accept();
}

ProfileCommandResult TrainerProfile::AddPlayerProfile(const PlayerProfileState& playerProfile)
{
    if (static_cast<int>(state_.roster.size()) >= TrainerBalance::MaxPlayerProfiles)
    {
        return Reject(SimulationError::RosterFull, "Roster is full.");
    }

    state_.roster.push_back(playerProfile);
    return Accept();
}
