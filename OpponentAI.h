#pragma once

#include "BattleRules.h"
#include "Models.h"
#include "SkillSystem.h"

#include <random>

class SimulationData;

class OpponentAI
{
public:
    OpponentAI(
        const SimulationData& data,
        const BattleRules& rules,
        const SkillSystem& skills);

    SkillProgress* SelectSkill(
        Competitor& opponent,
        const BattleStatus& opponentStatus,
        const Competitor& player,
        const BattleStatus& playerStatus,
        std::mt19937& randomEngine) const;

private:
    bool IsUsefulSkill(
        const Skill& definition,
        const Competitor& opponent,
        const BattleStatus& opponentStatus,
        const Competitor& player,
        const BattleStatus& playerStatus) const;

    const SimulationData& data_;
    const BattleRules& rules_;
    const SkillSystem& skills_;
};
