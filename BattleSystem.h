#pragma once

#include "Models.h"

#include <random>

class SimulationData;

enum class BattleResult
{
    Victory,
    Defeat
};

class BattleSystem
{
public:
    BattleSystem(const SimulationData& data, std::mt19937& randomEngine);

    BattleResult Run(Player& player, Opponent& opponent);

private:
    void DisplayStatus(const Player& player, const Opponent& opponent) const;
    SkillProgress* SelectPlayerSkill(Player& player) const;
    SkillProgress* SelectEnemySkill(Opponent& opponent);
    void RunPlayerTurn(Player& player, Opponent& opponent, SkillProgress& skill);
    void RunEnemyTurn(Player& player, Opponent& opponent, SkillProgress& skill);

    int GetPower(const Skill& definition, const SkillProgress& skill) const;
    int GetFocusCost(const Skill& definition, const SkillProgress& skill) const;
    double GetAccuracy(const Skill& definition, const SkillProgress& skill) const;
    int CalculateDamage(const Skill& definition, const SkillProgress& skill, Style defenderStyle) const;
    void AwardSkillXp(SkillProgress& skill, const Skill& definition) const;
    bool Chance(double probability);

    static double GetStyleModifier(Style attackerStyle, Style defenderStyle);
    static bool HasAdvantage(Style attackerStyle, Style defenderStyle);

    const SimulationData& data_;
    std::mt19937& randomEngine_;
};
