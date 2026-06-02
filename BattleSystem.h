#pragma once

#include "Models.h"

#include <random>

class SimulationData;

enum class BattleResult
{
    Victory,
    Defeat
};

// DEV BALANCE TOOL ONLY:
// The batch simulator uses this quiet result instead of parsing console text.
struct AutomatedBattleResult
{
    bool playerWon = false;
    bool draw = false;
    int turns = 0;
    int playerHpRemaining = 0;
    int opponentHpRemaining = 0;
};

class BattleSystem
{
public:
    BattleSystem(const SimulationData& data, std::mt19937& randomEngine);

    BattleResult Run(Player& player, Opponent& opponent);
    AutomatedBattleResult RunAutomated(Player player, Opponent opponent);

private:
    void DisplayStatus(const Player& player, const Opponent& opponent) const;
    SkillProgress* SelectPlayerSkill(Player& player) const;
    SkillProgress* SelectAutomatedSkill(
        std::vector<SkillProgress>& skills,
        Style style,
        int hp,
        int maxHp,
        int focus,
        const BattleStatus& selfStatus,
        const BattleStatus& opponentStatus);
    void SelectPlayerStyle(Player& player) const;
    void RunPlayerTurn(
        Player& player,
        Opponent& opponent,
        BattleStatus& playerStatus,
        BattleStatus& opponentStatus,
        SkillProgress& skill);
    void RunEnemyTurn(
        Player& player,
        Opponent& opponent,
        BattleStatus& playerStatus,
        BattleStatus& opponentStatus,
        SkillProgress& skill);
    void UseSkill(
        const std::string& attackerName,
        int& attackerHp,
        int attackerMaxHp,
        int& attackerFocus,
        int attackerBasePower,
        Spec attackerSpec,
        BattleStatus& attackerStatus,
        int& defenderHp,
        Spec defenderSpec,
        BattleStatus& defenderStatus,
        SkillProgress& skill,
        bool showFeedback);

    int GetPower(const Skill& definition, const SkillProgress& skill) const;
    int GetFocusCost(const Skill& definition, const SkillProgress& skill) const;
    double GetAccuracy(const Skill& definition, const SkillProgress& skill) const;
    int GetEffectValue(const Skill& definition, const SkillProgress& skill) const;
    std::string DescribeEffect(const Skill& definition, const SkillProgress& skill) const;
    int CalculateDamage(
        const Skill& definition,
        const SkillProgress& skill,
        int attackerBasePower,
        Spec attackerSpec,
        Spec defenderSpec,
        BattleStatus& attackerStatus,
        BattleStatus& defenderStatus,
        bool showFeedback = true) const;
    void ApplySecondaryEffect(
        const Skill& definition,
        const SkillProgress& skill,
        int& attackerHp,
        int attackerMaxHp,
        BattleStatus& attackerStatus,
        BattleStatus& defenderStatus,
        bool showFeedback) const;
    void AwardSkillXp(SkillProgress& skill, const Skill& definition, bool showFeedback = true) const;
    bool Chance(double probability);

    bool IsAvailableForStyle(const Skill& definition, Style style) const;
    bool IsUsefulAutomatedSkill(
        const Skill& definition,
        int hp,
        int maxHp,
        const BattleStatus& selfStatus,
        const BattleStatus& opponentStatus) const;
    double GetSpecModifier(Spec attackerSpec, Spec defenderSpec) const;

    const SimulationData& data_;
    std::mt19937& randomEngine_;
};
