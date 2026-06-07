#pragma once

#include "Models.h"

#include <string>

class PlayerProfileSystem;

namespace TrainerBalance
{
    inline constexpr int StartingRating = 1000;
    inline constexpr int StartingMoney = 0;
    inline constexpr int MaxPlayerProfiles = 6;
}

// TrainerProfile is not the fighter. It owns the save-profile style state,
// while each PlayerProfileState owns combat growth like spec, XP, and skills.
class TrainerProfile
{
public:
    static TrainerProfile CreateNew(
        const std::string& trainerName,
        GameType gameType,
        Spec starterSpec,
        const PlayerProfileSystem& playerProfiles);
    static TrainerProfile FromState(const TrainerProfileState& state);

    const TrainerProfileState& GetState() const;
    const PlayerProfileState* GetActivePlayerProfile() const;
    PlayerProfileState* GetMutableActivePlayerProfile();
    PlayerProfileState* GetMutablePlayerProfile(int playerIndex);
    PassiveBonuses GetActivePlayerPassiveBonuses() const;
    int GetActivePlayerLevel() const;

    ProfileCommandResult AwardRating(int amount);
    ProfileCommandResult AwardMoney(int amount);
    ProfileCommandResult SetActivePlayerIndex(int playerIndex);
    ProfileCommandResult AddTrophy(const std::string& trophyId);
    ProfileCommandResult AddPlayerProfile(const PlayerProfileState& playerProfile);

private:
    TrainerProfileState state_;
};
