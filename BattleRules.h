#pragma once

#include "Models.h"

class SimulationData;

class BattleRules
{
public:
    explicit BattleRules(const SimulationData& data);

    int GetPower(const Skill& definition, const SkillProgress& progress) const;
    int GetManaCost(const Skill& definition, const SkillProgress& progress, const Competitor& user) const;
    int GetManaGain(const Skill& definition, const SkillProgress& progress) const;
    int GetCooldownTurns(const Skill& definition, const BattleStatus& userStatus) const;
    double GetAccuracy(const Skill& definition, const SkillProgress& progress, const Competitor& user) const;
    int GetEffectValue(const Skill& definition, const SkillProgress& progress, const Competitor& user) const;
    double GetSpecModifier(Spec attackerSpec, Spec defenderSpec) const;
    DamageResult CalculateDamage(
        const Skill& definition,
        const SkillProgress& progress,
        const Competitor& attacker,
        BattleStatus& attackerStatus,
        const Competitor& defender,
        BattleStatus& defenderStatus) const;
    DamageResult CalculateObjectiveDamage(
        const Skill& definition,
        const SkillProgress& progress,
        const Competitor& attacker,
        const BattleStatus& attackerStatus) const;

private:
    const TraitDefinition* FindTrait(const Competitor& competitor) const;
    bool HasTraitEffect(const Competitor& competitor, TraitEffectType effectType) const;
    int GetTraitEffectValue(const Competitor& competitor, TraitEffectType effectType) const;
    int ApplyPercentBonus(int value, int percent) const;

    const SimulationData& data_;
};
