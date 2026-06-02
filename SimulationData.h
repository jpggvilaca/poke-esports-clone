#pragma once

#include "Models.h"

#include <string>
#include <vector>

// Edit these values first when rebalancing the bare simulation.
namespace Balance
{
    inline constexpr int StartingBasePower = 5;
    inline constexpr int PlayerLevelUpHp = 15;
    inline constexpr int PlayerLevelUpFocus = 8;
    inline constexpr int PlayerLevelUpBasePower = 2;
    inline constexpr int PlayerXpPerOpponentLevel = 35;
    inline constexpr int CurrencyPerOpponentLevel = 25;
    inline constexpr int RankPointsPerWin = 35;
    inline constexpr int SkillXpPerUse = 25;
    inline constexpr int SkillXpPerLevel = 100;
    inline constexpr int SkillLevelUpPower = 4;
    inline constexpr int SkillLevelUpFocusCost = 2;
    inline constexpr double AdvantageModifier = 1.25;
    inline constexpr double DisadvantageModifier = 0.75;
    inline constexpr double NeutralModifier = 1.0;
}

class SimulationData
{
public:
    SimulationData();

    const Skill* FindSkill(const std::string& id) const;
    const SpecData& GetSpec(Spec spec) const;
    Spec GetCounterSpec(Spec spec) const;
    const GameTypeData& GetGameType(GameType gameType) const;
    const std::vector<StoreItem>& GetStoreItems() const;
    const std::vector<TournamentData>& GetTournaments() const;

private:
    std::vector<Skill> skills_;
    std::vector<SpecData> specs_;
    std::vector<GameTypeData> gameTypes_;
    std::vector<StoreItem> storeItems_;
    std::vector<TournamentData> tournaments_;
};

