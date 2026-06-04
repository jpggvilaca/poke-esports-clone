#pragma once

#include "Models.h"

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
    DamageResult CalculateDamage(
        const Skill& definition,
        const SkillProgress& progress,
        const Competitor& attacker,
        BattleStatus& attackerStatus,
        const Competitor& defender,
        BattleStatus& defenderStatus) const;

private:
    const SimulationData& data_;
};
