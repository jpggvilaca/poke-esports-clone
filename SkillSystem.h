#pragma once

#include "BattleRules.h"
#include "Models.h"
#include "ProgressionSystem.h"

#include <random>

class SimulationData;

class SkillSystem
{
public:
    SkillSystem(
        const SimulationData& data,
        const BattleRules& rules,
        const ProgressionSystem& progression);

    bool IsAvailableForStyle(const Skill& definition, Style style) const;
    SkillView CreateSkillView(const Skill& definition, const SkillProgress& progress) const;
    SkillUseResult UseSkill(
        BattleActor actor,
        Competitor& attacker,
        BattleStatus& attackerStatus,
        Competitor& defender,
        BattleStatus& defenderStatus,
        SkillProgress& progress,
        std::mt19937& randomEngine) const;

private:
    BattleActor Opposite(BattleActor actor) const;
    bool Chance(double probability, std::mt19937& randomEngine) const;
    SecondaryEffectResult ApplySecondaryEffect(
        BattleActor actor,
        BattleActor target,
        const Skill& definition,
        const SkillProgress& progress,
        Competitor& attacker,
        BattleStatus& attackerStatus,
        BattleStatus& defenderStatus) const;

    const SimulationData& data_;
    const BattleRules& rules_;
    const ProgressionSystem& progression_;
};
