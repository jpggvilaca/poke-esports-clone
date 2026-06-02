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
    void SelectPlayerStyle(Player& player) const;
    void RunPlayerTurn(Player& player, Opponent& opponent, SkillProgress& skill);
    void RunEnemyTurn(Player& player, Opponent& opponent, SkillProgress& skill);

    int GetPower(const Skill& definition, const SkillProgress& skill) const;
    int GetFocusCost(const Skill& definition, const SkillProgress& skill) const;
    double GetAccuracy(const Skill& definition, const SkillProgress& skill) const;
    int CalculateDamage(
        const Skill& definition,
        const SkillProgress& skill,
        int attackerBasePower,
        Spec attackerSpec,
        Spec defenderSpec) const;
    void AwardSkillXp(SkillProgress& skill, const Skill& definition) const;
    bool Chance(double probability);

    bool IsAvailableForStyle(const Skill& definition, Style style) const;
    double GetSpecModifier(Spec attackerSpec, Spec defenderSpec) const;

    const SimulationData& data_;
    std::mt19937& randomEngine_;
};
