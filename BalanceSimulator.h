#pragma once

#include "Models.h"

#include <random>
#include <vector>

class BattleSystem;
class SimulationData;

// DEV BALANCE TOOL ONLY:
// Delete this class when a more complete analytics workflow replaces the
// console sandbox. It exists to reveal balance problems before hand-tuning.
class BalanceSimulator
{
public:
    BalanceSimulator(const SimulationData& data, BattleSystem& battleSystem, std::mt19937& randomEngine);

    void Run(int battlePairsPerMatchup);

private:
    struct MatchupStats
    {
        int wins = 0;
        int losses = 0;
        int draws = 0;
        int totalTurns = 0;
    };

    Player CreatePlayer(Spec spec, Style style) const;
    Opponent CreateOpponent(Spec spec, Style style) const;
    std::vector<SkillProgress> CreateStarterSkills(Spec spec) const;
    Style RandomStyle();
    void RecordResult(MatchupStats& stats, bool rowSpecWasFirst, bool firstCompetitorWon, bool draw, int turns) const;

    const SimulationData& data_;
    BattleSystem& battleSystem_;
    std::mt19937& randomEngine_;
};

