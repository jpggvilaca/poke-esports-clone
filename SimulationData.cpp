#include "SimulationData.h"

namespace
{
    Skill MakeSkill(
        const std::string& id,
        const std::string& name,
        int focusCost,
        int power,
        double accuracy,
        SkillEffectType effectType = SkillEffectType::None,
        SkillEffectTarget effectTarget = SkillEffectTarget::Self,
        int effectValue = 0,
        int effectUses = 0)
    {
        Skill skill;
        skill.id = id;
        skill.name = name;
        skill.focusCost = focusCost;
        skill.power = power;
        skill.accuracy = accuracy;
        skill.effectType = effectType;
        skill.effectTarget = effectTarget;
        skill.effectValue = effectValue;
        skill.effectUses = effectUses;
        return skill;
    }

    TraitDefinition MakeTrait(
        const std::string& id,
        const std::string& name,
        const std::string& description,
        TraitEffectType effectType,
        int effectValue)
    {
        TraitDefinition trait;
        trait.id = id;
        trait.name = name;
        trait.description = description;
        trait.effectType = effectType;
        trait.effectValue = effectValue;
        return trait;
    }

    SkillTone GetSkillTone(const Skill& skill)
    {
        if (skill.name.find("Reckless") != std::string::npos)
        {
            return SkillTone::Risky;
        }

        if (skill.effectType == SkillEffectType::Heal
            || skill.effectType == SkillEffectType::AttackModifier
            || (skill.effectType == SkillEffectType::DefenseModifier && skill.effectValue > 0))
        {
            return SkillTone::Utility;
        }

        if (skill.power >= 30)
        {
            return SkillTone::Pressure;
        }

        if (skill.power > 0)
        {
            return SkillTone::Reliable;
        }

        return SkillTone::Basic;
    }

    std::string CreateSkillDescription(const Skill& skill)
    {
        if (!skill.description.empty())
        {
            return skill.description;
        }

        if (skill.effectType == SkillEffectType::Heal)
        {
            return "Recover HP and stay in the set longer.";
        }

        if (skill.effectType == SkillEffectType::AttackModifier)
        {
            return skill.effectTarget == SkillEffectTarget::Self
                ? "Set up a stronger follow-up play."
                : "Disrupt the opponent's next attacks.";
        }

        if (skill.effectType == SkillEffectType::DefenseModifier)
        {
            return skill.effectValue < 0
                ? "A risky play that hits hard but leaves you exposed."
                : "Reduce incoming pressure for a few hits.";
        }

        if (skill.power >= 30)
        {
            return "Push tempo with a high-pressure offensive play.";
        }

        if (skill.power > 0)
        {
            return "Trade consistently without overcommitting.";
        }

        return "A reliable basic play.";
    }

    void FillSkillMetadata(std::vector<Skill>& skills)
    {
        for (Skill& skill : skills)
        {
            skill.tone = GetSkillTone(skill);
            skill.description = CreateSkillDescription(skill);
        }
    }
}

SimulationData::SimulationData()
{
    // SAMPLE COMBAT DATA:
    // Edit these rows to tune the first League loadouts. Each spec has one
    // basic action, pressure plays, recovery/defense options, and setup tools.
    //
    // Row format:
    // ID, name, focus, power, accuracy, optional effect, target,
    // effect value, effect hits.
    skills_ = {
        MakeSkill("top-basic", "Top Basic", 0, 16, 0.95),
        MakeSkill("top-pressure", "Top Pressure", 8, 26, 0.95),
        MakeSkill("top-all-in", "Top All-In", 15, 36, 0.82),
        MakeSkill("top-reckless", "Top Reckless Play", 18, 46, 0.72, SkillEffectType::DefenseModifier, SkillEffectTarget::Self, -25, 1),
        MakeSkill("top-recover", "Top Recover", 14, 0, 1.00, SkillEffectType::Heal, SkillEffectTarget::Self, 28, 0),
        MakeSkill("top-guard", "Top Guard", 7, 0, 1.00, SkillEffectType::DefenseModifier, SkillEffectTarget::Self, 45, 1),
        MakeSkill("top-fortify", "Top Fortify", 12, 0, 1.00, SkillEffectType::DefenseModifier, SkillEffectTarget::Self, 20, 3),
        MakeSkill("top-consistent", "Top Consistent Play", 7, 24, 0.98),
        MakeSkill("top-disrupt", "Top Disrupt", 10, 18, 0.90, SkillEffectType::AttackModifier, SkillEffectTarget::Opponent, -20, 2),
        MakeSkill("top-setup", "Top Setup", 8, 0, 1.00, SkillEffectType::AttackModifier, SkillEffectTarget::Self, 40, 1),

        MakeSkill("jungle-basic", "Jungle Basic", 0, 16, 0.95),
        MakeSkill("jungle-pressure", "Jungle Pressure", 8, 26, 0.95),
        MakeSkill("jungle-all-in", "Jungle All-In", 15, 36, 0.82),
        MakeSkill("jungle-reckless", "Jungle Reckless Play", 18, 46, 0.72, SkillEffectType::DefenseModifier, SkillEffectTarget::Self, -25, 1),
        MakeSkill("jungle-recover", "Jungle Recover", 14, 0, 1.00, SkillEffectType::Heal, SkillEffectTarget::Self, 28, 0),
        MakeSkill("jungle-guard", "Jungle Guard", 7, 0, 1.00, SkillEffectType::DefenseModifier, SkillEffectTarget::Self, 45, 1),
        MakeSkill("jungle-fortify", "Jungle Fortify", 12, 0, 1.00, SkillEffectType::DefenseModifier, SkillEffectTarget::Self, 20, 3),
        MakeSkill("jungle-consistent", "Jungle Consistent Play", 7, 24, 0.98),
        MakeSkill("jungle-disrupt", "Jungle Disrupt", 10, 18, 0.90, SkillEffectType::AttackModifier, SkillEffectTarget::Opponent, -20, 2),
        MakeSkill("jungle-setup", "Jungle Setup", 8, 0, 1.00, SkillEffectType::AttackModifier, SkillEffectTarget::Self, 40, 1),

        MakeSkill("mid-basic", "Mid Basic", 0, 16, 0.95),
        MakeSkill("mid-pressure", "Mid Pressure", 8, 26, 0.95),
        MakeSkill("mid-all-in", "Mid All-In", 15, 36, 0.82),
        MakeSkill("mid-reckless", "Mid Reckless Play", 18, 46, 0.72, SkillEffectType::DefenseModifier, SkillEffectTarget::Self, -25, 1),
        MakeSkill("mid-recover", "Mid Recover", 14, 0, 1.00, SkillEffectType::Heal, SkillEffectTarget::Self, 28, 0),
        MakeSkill("mid-guard", "Mid Guard", 7, 0, 1.00, SkillEffectType::DefenseModifier, SkillEffectTarget::Self, 45, 1),
        MakeSkill("mid-fortify", "Mid Fortify", 12, 0, 1.00, SkillEffectType::DefenseModifier, SkillEffectTarget::Self, 20, 3),
        MakeSkill("mid-consistent", "Mid Consistent Play", 7, 24, 0.98),
        MakeSkill("mid-disrupt", "Mid Disrupt", 10, 18, 0.90, SkillEffectType::AttackModifier, SkillEffectTarget::Opponent, -20, 2),
        MakeSkill("mid-setup", "Mid Setup", 8, 0, 1.00, SkillEffectType::AttackModifier, SkillEffectTarget::Self, 40, 1),

        MakeSkill("adc-basic", "ADC Basic", 0, 16, 0.95),
        MakeSkill("adc-pressure", "ADC Pressure", 8, 26, 0.95),
        MakeSkill("adc-all-in", "ADC All-In", 15, 36, 0.82),
        MakeSkill("adc-reckless", "ADC Reckless Play", 18, 46, 0.72, SkillEffectType::DefenseModifier, SkillEffectTarget::Self, -25, 1),
        MakeSkill("adc-recover", "ADC Recover", 14, 0, 1.00, SkillEffectType::Heal, SkillEffectTarget::Self, 28, 0),
        MakeSkill("adc-guard", "ADC Guard", 7, 0, 1.00, SkillEffectType::DefenseModifier, SkillEffectTarget::Self, 45, 1),
        MakeSkill("adc-fortify", "ADC Fortify", 12, 0, 1.00, SkillEffectType::DefenseModifier, SkillEffectTarget::Self, 20, 3),
        MakeSkill("adc-consistent", "ADC Consistent Play", 7, 24, 0.98),
        MakeSkill("adc-disrupt", "ADC Disrupt", 10, 18, 0.90, SkillEffectType::AttackModifier, SkillEffectTarget::Opponent, -20, 2),
        MakeSkill("adc-setup", "ADC Setup", 8, 0, 1.00, SkillEffectType::AttackModifier, SkillEffectTarget::Self, 40, 1),

        MakeSkill("support-basic", "Support Basic", 0, 16, 0.95),
        MakeSkill("support-pressure", "Support Pressure", 8, 26, 0.95),
        MakeSkill("support-all-in", "Support All-In", 15, 36, 0.82),
        MakeSkill("support-reckless", "Support Reckless Play", 18, 46, 0.72, SkillEffectType::DefenseModifier, SkillEffectTarget::Self, -25, 1),
        MakeSkill("support-recover", "Support Recover", 14, 0, 1.00, SkillEffectType::Heal, SkillEffectTarget::Self, 28, 0),
        MakeSkill("support-guard", "Support Guard", 7, 0, 1.00, SkillEffectType::DefenseModifier, SkillEffectTarget::Self, 45, 1),
        MakeSkill("support-fortify", "Support Fortify", 12, 0, 1.00, SkillEffectType::DefenseModifier, SkillEffectTarget::Self, 20, 3),
        MakeSkill("support-consistent", "Support Consistent Play", 7, 24, 0.98),
        MakeSkill("support-disrupt", "Support Disrupt", 10, 18, 0.90, SkillEffectType::AttackModifier, SkillEffectTarget::Opponent, -20, 2),
        MakeSkill("support-setup", "Support Setup", 8, 0, 1.00, SkillEffectType::AttackModifier, SkillEffectTarget::Self, 40, 1)
    };
    FillSkillMetadata(skills_);

    traits_ = {
        MakeTrait(
            "clutch-player",
            "Clutch Player",
            "Below 35% HP, paid skills cost 25% less focus.",
            TraitEffectType::LowHpFocusDiscountPercent,
            25),
        MakeTrait(
            "shotcaller",
            "Shotcaller",
            "Setup and disruption effects are 20% stronger.",
            TraitEffectType::SetupDisruptEffectBonusPercent,
            20),
        MakeTrait(
            "lane-bully",
            "Lane Bully",
            "Super-effective hits deal 15% more damage.",
            TraitEffectType::SuperEffectiveDamageBonusPercent,
            15),
        MakeTrait(
            "precision-carry",
            "Precision Carry",
            "Damaging skills gain 5% accuracy.",
            TraitEffectType::DamagingAccuracyBonusPercent,
            5),
        MakeTrait(
            "stabilizer",
            "Stabilizer",
            "Healing and defensive effects are 20% stronger.",
            TraitEffectType::PositiveEffectBonusPercent,
            20)
    };

    // Edit counteredSpec values to change the matchup cycle.
    // Current rule: Top > Jungle > Mid > ADC > Support > Top.
    specs_ = {
        { Spec::Top, "Top", { "top-basic", "top-pressure", "top-all-in", "top-reckless", "top-recover", "top-guard", "top-fortify", "top-consistent", "top-disrupt", "top-setup" }, Spec::Jungle, "clutch-player" },
        { Spec::Jungle, "Jungle", { "jungle-basic", "jungle-pressure", "jungle-all-in", "jungle-reckless", "jungle-recover", "jungle-guard", "jungle-fortify", "jungle-consistent", "jungle-disrupt", "jungle-setup" }, Spec::Mid, "shotcaller" },
        { Spec::Mid, "Mid", { "mid-basic", "mid-pressure", "mid-all-in", "mid-reckless", "mid-recover", "mid-guard", "mid-fortify", "mid-consistent", "mid-disrupt", "mid-setup" }, Spec::Adc, "lane-bully" },
        { Spec::Adc, "ADC", { "adc-basic", "adc-pressure", "adc-all-in", "adc-reckless", "adc-recover", "adc-guard", "adc-fortify", "adc-consistent", "adc-disrupt", "adc-setup" }, Spec::Support, "precision-carry" },
        { Spec::Support, "Support", { "support-basic", "support-pressure", "support-all-in", "support-reckless", "support-recover", "support-guard", "support-fortify", "support-consistent", "support-disrupt", "support-setup" }, Spec::Top, "stabilizer" }
    };
}

const Skill* SimulationData::FindSkill(const std::string& id) const
{
    for (const Skill& skill : skills_)
    {
        if (skill.id == id)
        {
            return &skill;
        }
    }

    return nullptr;
}

const TraitDefinition* SimulationData::FindTrait(const std::string& id) const
{
    for (const TraitDefinition& trait : traits_)
    {
        if (trait.id == id)
        {
            return &trait;
        }
    }

    return nullptr;
}

const SpecData* SimulationData::FindSpec(Spec spec) const
{
    for (const SpecData& data : specs_)
    {
        if (data.spec == spec)
        {
            return &data;
        }
    }

    return nullptr;
}

const std::vector<SpecData>& SimulationData::GetSpecs() const
{
    return specs_;
}
