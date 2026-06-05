#include "RatingSystem.h"

#include <algorithm>

RatingResult RatingSystem::CalculateChange(
    int playerRating,
    int playerLevel,
    int opponentLevel,
    MatchContext context,
    bool won) const
{
    RatingResult result;
    if (playerLevel < 1 || opponentLevel < 1)
    {
        result.errorCode = SimulationError::LevelsMustBePositive;
        result.error = "Levels must be positive.";
        return result;
    }

    result.accepted = true;
    result.won = won;
    result.context = context;
    result.playerLevel = playerLevel;
    result.opponentLevel = opponentLevel;
    result.oldRating = playerRating;

    const int levelDifference = opponentLevel - playerLevel;
    if (won)
    {
        const int rawGain = RatingBalance::BaseWinGain
            + levelDifference * RatingBalance::WinLevelAdjustment;
        result.ratingChange = std::max(RatingBalance::MinimumWinGain, rawGain);
    }
    else
    {
        const int rawLoss = -RatingBalance::BaseLossPenalty
            + levelDifference * RatingBalance::LossLevelAdjustment;
        result.ratingChange = std::min(-RatingBalance::MinimumLossPenalty, rawLoss);
    }

    result.ratingChange = ApplyContextMultiplier(result.ratingChange, context);
    result.newRating = std::max(0, playerRating + result.ratingChange);
    return result;
}

int RatingSystem::ApplyContextMultiplier(int ratingChange, MatchContext context) const
{
    switch (context)
    {
    case MatchContext::Tutorial:
        return ratingChange / 2;
    case MatchContext::Nemesis:
        return ratingChange * 3 / 2;
    case MatchContext::Major:
        return ratingChange * 2;
    case MatchContext::Normal:
        return ratingChange;
    }

    return ratingChange;
}
