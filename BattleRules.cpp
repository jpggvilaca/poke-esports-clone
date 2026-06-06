#include "BattleRules.h"

#include "SimulationData.h"

#include <algorithm>
#include <cmath>

BattleRules::BattleRules(const SimulationData& data)
    : data_(data)
{
}

int BattleRules::GetPower(const Skill& definition, const SkillProgress& progress) const
{
    // Edit Balance::SkillLevelUpPower to change power gained per skill level.
    return definition.power + (progress.level - 1) * Balance::SkillLevelUpPower;
}

int BattleRules::GetFocusCost(
    const Skill& definition,
    const SkillProgress& progress,
    const Competitor& user) const
{
    // Free basic skills stay free forever, so neither side can run out of legal
    // actions during a long battle.
    if (definition.focusCost == 0)
    {
        return 0;
    }

    // Edit Balance::SkillLevelUpFocusCost to tune the strength-versus-cost tradeoff.
    int focusCost = definition.focusCost + (progress.level - 1) * Balance::SkillLevelUpFocusCost;
    if (HasTraitEffect(user, TraitEffectType::LowHpFocusDiscountPercent)
        && user.maxHp > 0
        && user.hp * 100 < user.maxHp * Balance::ClutchLowHpThresholdPercent)
    {
        const int discount = GetTraitEffectValue(user, TraitEffectType::LowHpFocusDiscountPercent);
        focusCost = focusCost * (100 - discount) / 100;
    }

    return std::max(0, focusCost);
}

double BattleRules::GetAccuracy(
    const Skill& definition,
    const SkillProgress& progress,
    const Competitor& user) const
{
    // Edit 0.98 or 0.02 to tune the attack accuracy cap or level improvement.
    // Pure utility actions may reach 100% so reliable heals and buffs do not miss.
    double improvedAccuracy = definition.accuracy + (progress.level - 1) * 0.02;
    if (definition.power > 0 && HasTraitEffect(user, TraitEffectType::DamagingAccuracyBonusPercent))
    {
        improvedAccuracy += GetTraitEffectValue(user, TraitEffectType::DamagingAccuracyBonusPercent) / 100.0;
    }

    return definition.power == 0
        ? std::min(1.0, improvedAccuracy)
        : std::min(0.98, improvedAccuracy);
}

int BattleRules::GetEffectValue(
    const Skill& definition,
    const SkillProgress& progress,
    const Competitor& user) const
{
    const int growth = (progress.level - 1) * Balance::SkillLevelUpEffectValue;
    int effectValue = definition.effectValue < 0
        ? definition.effectValue - growth
        : definition.effectValue + growth;

    if (definition.effectType == SkillEffectType::AttackModifier
        && HasTraitEffect(user, TraitEffectType::SetupDisruptEffectBonusPercent))
    {
        effectValue = ApplyPercentBonus(
            effectValue,
            GetTraitEffectValue(user, TraitEffectType::SetupDisruptEffectBonusPercent));
    }

    if ((definition.effectType == SkillEffectType::Heal
            || (definition.effectType == SkillEffectType::DefenseModifier && effectValue > 0))
        && HasTraitEffect(user, TraitEffectType::PositiveEffectBonusPercent))
    {
        effectValue = ApplyPercentBonus(
            effectValue,
            GetTraitEffectValue(user, TraitEffectType::PositiveEffectBonusPercent));
    }

    return effectValue;
}

double BattleRules::GetSpecModifier(Spec attackerSpec, Spec defenderSpec) const
{
    const SpecData* attacker = data_.FindSpec(attackerSpec);
    const SpecData* defender = data_.FindSpec(defenderSpec);
    if (attacker == nullptr || defender == nullptr)
    {
        return Balance::NeutralModifier;
    }

    if (attacker->counteredSpec == defenderSpec)
    {
        return Balance::AdvantageModifier;
    }

    if (defender->counteredSpec == attackerSpec)
    {
        return Balance::DisadvantageModifier;
    }

    return Balance::NeutralModifier;
}

DamageResult BattleRules::CalculateDamage(
    const Skill& definition,
    const SkillProgress& progress,
    const Competitor& attacker,
    BattleStatus& attackerStatus,
    const Competitor& defender,
    BattleStatus& defenderStatus) const
{
    DamageResult result;
    result.applied = true;
    const double specModifier = GetSpecModifier(attacker.spec, defender.spec);
    result.specModifier = specModifier;
    if (specModifier > Balance::NeutralModifier)
    {
        result.effectiveness = Effectiveness::SuperEffective;
    }
    else if (specModifier < Balance::NeutralModifier)
    {
        result.effectiveness = Effectiveness::NotVeryEffective;
    }

    double damage = (attacker.basePower + GetPower(definition, progress)) * specModifier;
    if (specModifier > Balance::NeutralModifier && attacker.counterDamageBonusPercent > 0)
    {
        damage *= (100.0 + attacker.counterDamageBonusPercent) / 100.0;
    }
    if (specModifier > Balance::NeutralModifier
        && HasTraitEffect(attacker, TraitEffectType::SuperEffectiveDamageBonusPercent))
    {
        damage *= (100.0 + GetTraitEffectValue(attacker, TraitEffectType::SuperEffectiveDamageBonusPercent)) / 100.0;
    }

    if (attackerStatus.attackModifierHits > 0)
    {
        damage *= (100.0 + attackerStatus.attackModifierPercent) / 100.0;
        --attackerStatus.attackModifierHits;
        if (attackerStatus.attackModifierHits == 0)
        {
            attackerStatus.attackModifierPercent = 0;
        }
    }

    if (defenderStatus.defenseModifierHits > 0)
    {
        damage *= (100.0 - defenderStatus.defenseModifierPercent) / 100.0;
        --defenderStatus.defenseModifierHits;
        if (defenderStatus.defenseModifierHits == 0)
        {
            defenderStatus.defenseModifierPercent = 0;
        }
    }

    // CORE COMBAT FORMULA:
    // Edit status percentages in SimulationData.cpp to tune buffs. A successful
    // damaging hit always deals at least one damage.
    result.amount = std::max(1, static_cast<int>(std::round(damage)));
    return result;
}

const TraitDefinition* BattleRules::FindTrait(const Competitor& competitor) const
{
    return data_.FindTrait(competitor.traitId);
}

bool BattleRules::HasTraitEffect(const Competitor& competitor, TraitEffectType effectType) const
{
    const TraitDefinition* trait = FindTrait(competitor);
    return trait != nullptr && trait->effectType == effectType;
}

int BattleRules::GetTraitEffectValue(const Competitor& competitor, TraitEffectType effectType) const
{
    const TraitDefinition* trait = FindTrait(competitor);
    return trait != nullptr && trait->effectType == effectType ? trait->effectValue : 0;
}

int BattleRules::ApplyPercentBonus(int value, int percent) const
{
    return static_cast<int>(std::round(value * (100.0 + percent) / 100.0));
}
