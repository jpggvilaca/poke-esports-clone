#pragma once

#include "Models.h"

namespace RatingBalance
{
    inline constexpr int BaseWinGain = 20;
    inline constexpr int BaseLossPenalty = 14;
    inline constexpr int WinLevelAdjustment = 5;
    inline constexpr int LossLevelAdjustment = 3;
    inline constexpr int MinimumWinGain = 2;
    inline constexpr int MinimumLossPenalty = 2;
}

class RatingSystem
{
public:
    RatingResult CalculateChange(
        int playerRating,
        int playerLevel,
        int opponentLevel,
        MatchContext context,
        bool won) const;

private:
    int ApplyContextMultiplier(int ratingChange, MatchContext context) const;
};
