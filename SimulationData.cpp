#include "SimulationData.h"

namespace
{
    Skill MakeSkill(
        const std::string& id,
        const std::string& name,
        std::optional<Style> requiredStyle,
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
        skill.requiredStyle = requiredStyle;
        skill.focusCost = focusCost;
        skill.power = power;
        skill.accuracy = accuracy;
        skill.effectType = effectType;
        skill.effectTarget = effectTarget;
        skill.effectValue = effectValue;
        skill.effectUses = effectUses;
        return skill;
    }

    SkillTone GetSkillTone(const Skill& skill)
    {
        if (skill.name.find("Reckless") != std::string::npos)
        {
            return SkillTone::Risky;
        }

        if (!skill.requiredStyle.has_value())
        {
            return SkillTone::Basic;
        }

        if (skill.effectType != SkillEffectType::None && skill.power == 0)
        {
            return SkillTone::Utility;
        }

        switch (skill.requiredStyle.value())
        {
        case Style::Aggressive: return SkillTone::Aggressive;
        case Style::Defensive: return SkillTone::Defensive;
        case Style::Balanced: return SkillTone::Balanced;
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

        if (!skill.requiredStyle.has_value())
        {
            return "A reliable basic play available in every style.";
        }

        switch (skill.requiredStyle.value())
        {
        case Style::Aggressive:
            return "Push tempo with a high-pressure offensive play.";
        case Style::Defensive:
            return "Stabilize the fight with a safer defensive play.";
        case Style::Balanced:
            return "Trade consistently without overcommitting.";
        }

        return "A competitive skill.";
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
    // universal basic, then three Aggressive, three Defensive, and three
    // Balanced skills.
    //
    // Row format:
    // ID, name, style, focus, power, accuracy, optional effect, target,
    // effect value, effect hits.
    skills_ = {
        MakeSkill("top-basic", "Top Basic", std::nullopt, 0, 16, 0.95),
        MakeSkill("top-pressure", "Top Pressure", Style::Aggressive, 8, 26, 0.95),
        MakeSkill("top-all-in", "Top All-In", Style::Aggressive, 15, 36, 0.82),
        MakeSkill("top-reckless", "Top Reckless Play", Style::Aggressive, 18, 46, 0.72, SkillEffectType::DefenseModifier, SkillEffectTarget::Self, -25, 1),
        MakeSkill("top-recover", "Top Recover", Style::Defensive, 14, 0, 1.00, SkillEffectType::Heal, SkillEffectTarget::Self, 28, 0),
        MakeSkill("top-guard", "Top Guard", Style::Defensive, 7, 0, 1.00, SkillEffectType::DefenseModifier, SkillEffectTarget::Self, 45, 1),
        MakeSkill("top-fortify", "Top Fortify", Style::Defensive, 12, 0, 1.00, SkillEffectType::DefenseModifier, SkillEffectTarget::Self, 20, 3),
        MakeSkill("top-consistent", "Top Consistent Play", Style::Balanced, 7, 24, 0.98),
        MakeSkill("top-disrupt", "Top Disrupt", Style::Balanced, 10, 18, 0.90, SkillEffectType::AttackModifier, SkillEffectTarget::Opponent, -20, 2),
        MakeSkill("top-setup", "Top Setup", Style::Balanced, 8, 0, 1.00, SkillEffectType::AttackModifier, SkillEffectTarget::Self, 40, 1),

        MakeSkill("jungle-basic", "Jungle Basic", std::nullopt, 0, 16, 0.95),
        MakeSkill("jungle-pressure", "Jungle Pressure", Style::Aggressive, 8, 26, 0.95),
        MakeSkill("jungle-all-in", "Jungle All-In", Style::Aggressive, 15, 36, 0.82),
        MakeSkill("jungle-reckless", "Jungle Reckless Play", Style::Aggressive, 18, 46, 0.72, SkillEffectType::DefenseModifier, SkillEffectTarget::Self, -25, 1),
        MakeSkill("jungle-recover", "Jungle Recover", Style::Defensive, 14, 0, 1.00, SkillEffectType::Heal, SkillEffectTarget::Self, 28, 0),
        MakeSkill("jungle-guard", "Jungle Guard", Style::Defensive, 7, 0, 1.00, SkillEffectType::DefenseModifier, SkillEffectTarget::Self, 45, 1),
        MakeSkill("jungle-fortify", "Jungle Fortify", Style::Defensive, 12, 0, 1.00, SkillEffectType::DefenseModifier, SkillEffectTarget::Self, 20, 3),
        MakeSkill("jungle-consistent", "Jungle Consistent Play", Style::Balanced, 7, 24, 0.98),
        MakeSkill("jungle-disrupt", "Jungle Disrupt", Style::Balanced, 10, 18, 0.90, SkillEffectType::AttackModifier, SkillEffectTarget::Opponent, -20, 2),
        MakeSkill("jungle-setup", "Jungle Setup", Style::Balanced, 8, 0, 1.00, SkillEffectType::AttackModifier, SkillEffectTarget::Self, 40, 1),

        MakeSkill("mid-basic", "Mid Basic", std::nullopt, 0, 16, 0.95),
        MakeSkill("mid-pressure", "Mid Pressure", Style::Aggressive, 8, 26, 0.95),
        MakeSkill("mid-all-in", "Mid All-In", Style::Aggressive, 15, 36, 0.82),
        MakeSkill("mid-reckless", "Mid Reckless Play", Style::Aggressive, 18, 46, 0.72, SkillEffectType::DefenseModifier, SkillEffectTarget::Self, -25, 1),
        MakeSkill("mid-recover", "Mid Recover", Style::Defensive, 14, 0, 1.00, SkillEffectType::Heal, SkillEffectTarget::Self, 28, 0),
        MakeSkill("mid-guard", "Mid Guard", Style::Defensive, 7, 0, 1.00, SkillEffectType::DefenseModifier, SkillEffectTarget::Self, 45, 1),
        MakeSkill("mid-fortify", "Mid Fortify", Style::Defensive, 12, 0, 1.00, SkillEffectType::DefenseModifier, SkillEffectTarget::Self, 20, 3),
        MakeSkill("mid-consistent", "Mid Consistent Play", Style::Balanced, 7, 24, 0.98),
        MakeSkill("mid-disrupt", "Mid Disrupt", Style::Balanced, 10, 18, 0.90, SkillEffectType::AttackModifier, SkillEffectTarget::Opponent, -20, 2),
        MakeSkill("mid-setup", "Mid Setup", Style::Balanced, 8, 0, 1.00, SkillEffectType::AttackModifier, SkillEffectTarget::Self, 40, 1),

        MakeSkill("adc-basic", "ADC Basic", std::nullopt, 0, 16, 0.95),
        MakeSkill("adc-pressure", "ADC Pressure", Style::Aggressive, 8, 26, 0.95),
        MakeSkill("adc-all-in", "ADC All-In", Style::Aggressive, 15, 36, 0.82),
        MakeSkill("adc-reckless", "ADC Reckless Play", Style::Aggressive, 18, 46, 0.72, SkillEffectType::DefenseModifier, SkillEffectTarget::Self, -25, 1),
        MakeSkill("adc-recover", "ADC Recover", Style::Defensive, 14, 0, 1.00, SkillEffectType::Heal, SkillEffectTarget::Self, 28, 0),
        MakeSkill("adc-guard", "ADC Guard", Style::Defensive, 7, 0, 1.00, SkillEffectType::DefenseModifier, SkillEffectTarget::Self, 45, 1),
        MakeSkill("adc-fortify", "ADC Fortify", Style::Defensive, 12, 0, 1.00, SkillEffectType::DefenseModifier, SkillEffectTarget::Self, 20, 3),
        MakeSkill("adc-consistent", "ADC Consistent Play", Style::Balanced, 7, 24, 0.98),
        MakeSkill("adc-disrupt", "ADC Disrupt", Style::Balanced, 10, 18, 0.90, SkillEffectType::AttackModifier, SkillEffectTarget::Opponent, -20, 2),
        MakeSkill("adc-setup", "ADC Setup", Style::Balanced, 8, 0, 1.00, SkillEffectType::AttackModifier, SkillEffectTarget::Self, 40, 1),

        MakeSkill("support-basic", "Support Basic", std::nullopt, 0, 16, 0.95),
        MakeSkill("support-pressure", "Support Pressure", Style::Aggressive, 8, 26, 0.95),
        MakeSkill("support-all-in", "Support All-In", Style::Aggressive, 15, 36, 0.82),
        MakeSkill("support-reckless", "Support Reckless Play", Style::Aggressive, 18, 46, 0.72, SkillEffectType::DefenseModifier, SkillEffectTarget::Self, -25, 1),
        MakeSkill("support-recover", "Support Recover", Style::Defensive, 14, 0, 1.00, SkillEffectType::Heal, SkillEffectTarget::Self, 28, 0),
        MakeSkill("support-guard", "Support Guard", Style::Defensive, 7, 0, 1.00, SkillEffectType::DefenseModifier, SkillEffectTarget::Self, 45, 1),
        MakeSkill("support-fortify", "Support Fortify", Style::Defensive, 12, 0, 1.00, SkillEffectType::DefenseModifier, SkillEffectTarget::Self, 20, 3),
        MakeSkill("support-consistent", "Support Consistent Play", Style::Balanced, 7, 24, 0.98),
        MakeSkill("support-disrupt", "Support Disrupt", Style::Balanced, 10, 18, 0.90, SkillEffectType::AttackModifier, SkillEffectTarget::Opponent, -20, 2),
        MakeSkill("support-setup", "Support Setup", Style::Balanced, 8, 0, 1.00, SkillEffectType::AttackModifier, SkillEffectTarget::Self, 40, 1)
    };
    FillSkillMetadata(skills_);

    // Edit counteredSpec values to change the matchup cycle.
    // Current rule: Top > Jungle > Mid > ADC > Support > Top.
    specs_ = {
        { Spec::Top, "Top", { "top-basic", "top-pressure", "top-all-in", "top-reckless", "top-recover", "top-guard", "top-fortify", "top-consistent", "top-disrupt", "top-setup" }, Spec::Jungle },
        { Spec::Jungle, "Jungle", { "jungle-basic", "jungle-pressure", "jungle-all-in", "jungle-reckless", "jungle-recover", "jungle-guard", "jungle-fortify", "jungle-consistent", "jungle-disrupt", "jungle-setup" }, Spec::Mid },
        { Spec::Mid, "Mid", { "mid-basic", "mid-pressure", "mid-all-in", "mid-reckless", "mid-recover", "mid-guard", "mid-fortify", "mid-consistent", "mid-disrupt", "mid-setup" }, Spec::Adc },
        { Spec::Adc, "ADC", { "adc-basic", "adc-pressure", "adc-all-in", "adc-reckless", "adc-recover", "adc-guard", "adc-fortify", "adc-consistent", "adc-disrupt", "adc-setup" }, Spec::Support },
        { Spec::Support, "Support", { "support-basic", "support-pressure", "support-all-in", "support-reckless", "support-recover", "support-guard", "support-fortify", "support-consistent", "support-disrupt", "support-setup" }, Spec::Top }
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
