#pragma once

#include "BattleSystem.h"
#include "SimulationData.h"

#include <optional>
#include <random>
#include <string>
#include <vector>

class Game
{
public:
    Game();
    void Run();
    void RunBalanceSimulation(int battlePairsPerMatchup);

private:
    void DisplayMenu() const;
    void CreateCompetitor();
    void ViewProfile() const;
    void RunGeneratedBattle();
    void VisitShop();
    void ChangeStyleLoadout();
    void RestoreCompetitor();
    void RunSampleTournament();
    void RunManualRivalBattle();

    Opponent GenerateOpponent(bool isRival = false);
    std::vector<SkillProgress> BuildOpponentSkills(Spec spec, bool includeAdvanced) const;
    void AwardVictory(const Opponent& opponent);
    void AwardPlayerXp(int xp);
    void UpdateRankTier();

    int RandomInt(int minimum, int maximum);
    static RankTier CalculateRankTier(int rankPoints);
    bool RequirePlayer() const;

    SimulationData data_;
    std::mt19937 randomEngine_;
    BattleSystem battleSystem_;

    // std::optional means the Player may not exist yet. The value appears after
    // menu option 1 creates a competitor.
    std::optional<Player> player_;
};
