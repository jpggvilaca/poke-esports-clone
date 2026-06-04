#pragma once

#include "Models.h"

#include <vector>

class SimulationData;

class BattleRules
{
public:
    explicit BattleRules(const SimulationData& data);

    int GetPower(const Skill& definition, const SkillProgress& progress) const;
    int GetFocusCost(const Skill& definition, const SkillProgress& progress) const;
    double GetAccuracy(const Skill& definition, const SkillProgress& progress) const;
    int GetEffectValue(const Skill& definition, const SkillProgress& progress) const;
    double GetSpecModifier(Spec attackerSpec, Spec defenderSpec) const;
    int CalculateDamage(
        BattleActor actor,
        BattleActor target,
        const Skill& definition,
        const SkillProgress& progress,
        const Competitor& attacker,
        BattleStatus& attackerStatus,
        const Competitor& defender,
        BattleStatus& defenderStatus,
        std::vector<BattleEvent>& events) const;

private:
    const SimulationData& data_;
};
