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

    SkillView CreateSkillView(
        const Skill& definition,
        const SkillProgress& progress,
        const Competitor& user,
        const BattleStatus& userStatus,
        int cooldownRemaining) const;
    SkillUseResult UseSkill(
        BattleActor actor,
        Competitor& attacker,
        BattleStatus& attackerStatus,
        BattleActor targetActor,
        Competitor& target,
        BattleStatus& targetStatus,
        SkillProgress& progress,
        int actorPlayerIndex,
        int targetPlayerIndex,
        std::mt19937& randomEngine) const;

private:
    BattleActor Opposite(BattleActor actor) const;
    bool Chance(double probability, std::mt19937& randomEngine) const;
    SecondaryEffectResult ApplySecondaryEffect(
        BattleActor actor,
        BattleActor targetActor,
        const Skill& definition,
        const SkillEffectDefinition& effect,
        const SkillProgress& progress,
        Competitor& attacker,
        BattleStatus& attackerStatus,
        Competitor& targetCompetitor,
        BattleStatus& targetBattleStatus,
        int actorPlayerIndex,
        int targetPlayerIndex) const;

    const SimulationData& data_;
    const BattleRules& rules_;
    const ProgressionSystem& progression_;
};
