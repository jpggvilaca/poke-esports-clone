#include "SimulationData.h"

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
        { "top-basic", "Top Basic", std::nullopt, 0, 16, 0.95 },
        { "top-pressure", "Top Pressure", Style::Aggressive, 8, 26, 0.95 },
        { "top-all-in", "Top All-In", Style::Aggressive, 15, 36, 0.82 },
        { "top-reckless", "Top Reckless Play", Style::Aggressive, 18, 46, 0.72, SkillEffectType::DefenseModifier, SkillEffectTarget::Self, -25, 1 },
        { "top-recover", "Top Recover", Style::Defensive, 14, 0, 1.00, SkillEffectType::Heal, SkillEffectTarget::Self, 28, 0 },
        { "top-guard", "Top Guard", Style::Defensive, 7, 0, 1.00, SkillEffectType::DefenseModifier, SkillEffectTarget::Self, 45, 1 },
        { "top-fortify", "Top Fortify", Style::Defensive, 12, 0, 1.00, SkillEffectType::DefenseModifier, SkillEffectTarget::Self, 20, 3 },
        { "top-consistent", "Top Consistent Play", Style::Balanced, 7, 24, 0.98 },
        { "top-disrupt", "Top Disrupt", Style::Balanced, 10, 18, 0.90, SkillEffectType::AttackModifier, SkillEffectTarget::Opponent, -20, 2 },
        { "top-setup", "Top Setup", Style::Balanced, 8, 0, 1.00, SkillEffectType::AttackModifier, SkillEffectTarget::Self, 40, 1 },

        { "jungle-basic", "Jungle Basic", std::nullopt, 0, 16, 0.95 },
        { "jungle-pressure", "Jungle Pressure", Style::Aggressive, 8, 26, 0.95 },
        { "jungle-all-in", "Jungle All-In", Style::Aggressive, 15, 36, 0.82 },
        { "jungle-reckless", "Jungle Reckless Play", Style::Aggressive, 18, 46, 0.72, SkillEffectType::DefenseModifier, SkillEffectTarget::Self, -25, 1 },
        { "jungle-recover", "Jungle Recover", Style::Defensive, 14, 0, 1.00, SkillEffectType::Heal, SkillEffectTarget::Self, 28, 0 },
        { "jungle-guard", "Jungle Guard", Style::Defensive, 7, 0, 1.00, SkillEffectType::DefenseModifier, SkillEffectTarget::Self, 45, 1 },
        { "jungle-fortify", "Jungle Fortify", Style::Defensive, 12, 0, 1.00, SkillEffectType::DefenseModifier, SkillEffectTarget::Self, 20, 3 },
        { "jungle-consistent", "Jungle Consistent Play", Style::Balanced, 7, 24, 0.98 },
        { "jungle-disrupt", "Jungle Disrupt", Style::Balanced, 10, 18, 0.90, SkillEffectType::AttackModifier, SkillEffectTarget::Opponent, -20, 2 },
        { "jungle-setup", "Jungle Setup", Style::Balanced, 8, 0, 1.00, SkillEffectType::AttackModifier, SkillEffectTarget::Self, 40, 1 },

        { "mid-basic", "Mid Basic", std::nullopt, 0, 16, 0.95 },
        { "mid-pressure", "Mid Pressure", Style::Aggressive, 8, 26, 0.95 },
        { "mid-all-in", "Mid All-In", Style::Aggressive, 15, 36, 0.82 },
        { "mid-reckless", "Mid Reckless Play", Style::Aggressive, 18, 46, 0.72, SkillEffectType::DefenseModifier, SkillEffectTarget::Self, -25, 1 },
        { "mid-recover", "Mid Recover", Style::Defensive, 14, 0, 1.00, SkillEffectType::Heal, SkillEffectTarget::Self, 28, 0 },
        { "mid-guard", "Mid Guard", Style::Defensive, 7, 0, 1.00, SkillEffectType::DefenseModifier, SkillEffectTarget::Self, 45, 1 },
        { "mid-fortify", "Mid Fortify", Style::Defensive, 12, 0, 1.00, SkillEffectType::DefenseModifier, SkillEffectTarget::Self, 20, 3 },
        { "mid-consistent", "Mid Consistent Play", Style::Balanced, 7, 24, 0.98 },
        { "mid-disrupt", "Mid Disrupt", Style::Balanced, 10, 18, 0.90, SkillEffectType::AttackModifier, SkillEffectTarget::Opponent, -20, 2 },
        { "mid-setup", "Mid Setup", Style::Balanced, 8, 0, 1.00, SkillEffectType::AttackModifier, SkillEffectTarget::Self, 40, 1 },

        { "adc-basic", "ADC Basic", std::nullopt, 0, 16, 0.95 },
        { "adc-pressure", "ADC Pressure", Style::Aggressive, 8, 26, 0.95 },
        { "adc-all-in", "ADC All-In", Style::Aggressive, 15, 36, 0.82 },
        { "adc-reckless", "ADC Reckless Play", Style::Aggressive, 18, 46, 0.72, SkillEffectType::DefenseModifier, SkillEffectTarget::Self, -25, 1 },
        { "adc-recover", "ADC Recover", Style::Defensive, 14, 0, 1.00, SkillEffectType::Heal, SkillEffectTarget::Self, 28, 0 },
        { "adc-guard", "ADC Guard", Style::Defensive, 7, 0, 1.00, SkillEffectType::DefenseModifier, SkillEffectTarget::Self, 45, 1 },
        { "adc-fortify", "ADC Fortify", Style::Defensive, 12, 0, 1.00, SkillEffectType::DefenseModifier, SkillEffectTarget::Self, 20, 3 },
        { "adc-consistent", "ADC Consistent Play", Style::Balanced, 7, 24, 0.98 },
        { "adc-disrupt", "ADC Disrupt", Style::Balanced, 10, 18, 0.90, SkillEffectType::AttackModifier, SkillEffectTarget::Opponent, -20, 2 },
        { "adc-setup", "ADC Setup", Style::Balanced, 8, 0, 1.00, SkillEffectType::AttackModifier, SkillEffectTarget::Self, 40, 1 },

        { "support-basic", "Support Basic", std::nullopt, 0, 16, 0.95 },
        { "support-pressure", "Support Pressure", Style::Aggressive, 8, 26, 0.95 },
        { "support-all-in", "Support All-In", Style::Aggressive, 15, 36, 0.82 },
        { "support-reckless", "Support Reckless Play", Style::Aggressive, 18, 46, 0.72, SkillEffectType::DefenseModifier, SkillEffectTarget::Self, -25, 1 },
        { "support-recover", "Support Recover", Style::Defensive, 14, 0, 1.00, SkillEffectType::Heal, SkillEffectTarget::Self, 28, 0 },
        { "support-guard", "Support Guard", Style::Defensive, 7, 0, 1.00, SkillEffectType::DefenseModifier, SkillEffectTarget::Self, 45, 1 },
        { "support-fortify", "Support Fortify", Style::Defensive, 12, 0, 1.00, SkillEffectType::DefenseModifier, SkillEffectTarget::Self, 20, 3 },
        { "support-consistent", "Support Consistent Play", Style::Balanced, 7, 24, 0.98 },
        { "support-disrupt", "Support Disrupt", Style::Balanced, 10, 18, 0.90, SkillEffectType::AttackModifier, SkillEffectTarget::Opponent, -20, 2 },
        { "support-setup", "Support Setup", Style::Balanced, 8, 0, 1.00, SkillEffectType::AttackModifier, SkillEffectTarget::Self, 40, 1 }
    };

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
