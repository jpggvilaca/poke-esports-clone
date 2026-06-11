#include "SimulationData.h"

namespace
{
    Skill MakeSkill(
        const std::string& id,
        const std::string& name,
        int manaCost,
        int manaGain,
        int cooldownTurns,
        int power,
        double accuracy,
        SkillEffectType effectType = SkillEffectType::None,
        SkillEffectTarget effectTarget = SkillEffectTarget::Self,
        int effectValue = 0,
        int durationTurns = 0,
        int markBonusDamage = 0)
    {
        Skill skill;
        skill.id = id;
        skill.name = name;
        skill.manaCost = manaCost;
        skill.manaGain = manaGain;
        skill.cooldownTurns = cooldownTurns;
        skill.power = power;
        skill.accuracy = accuracy;
        skill.effectType = effectType;
        skill.effectTarget = effectTarget;
        skill.effectValue = effectValue;
        skill.durationTurns = durationTurns;
        skill.markBonusDamage = markBonusDamage;
        if (effectType != SkillEffectType::None)
        {
            skill.effects.push_back({ effectType, effectTarget, effectValue, durationTurns, markBonusDamage });
        }
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

    DrillDefinition MakeDrill(
        GameType gameType,
        const std::string& id,
        const std::string& displayName,
        const std::string& description,
        int missManaGain,
        int goodManaGain,
        int perfectManaGain)
    {
        DrillDefinition drill;
        drill.gameType = gameType;
        drill.id = id;
        drill.displayName = displayName;
        drill.description = description;
        drill.missManaGain = missManaGain;
        drill.goodManaGain = goodManaGain;
        drill.perfectManaGain = perfectManaGain;
        return drill;
    }

    SkillTone GetSkillTone(const Skill& skill)
    {
        if (skill.cooldownTurns >= 5)
        {
            return SkillTone::Risky;
        }

        if (skill.effectType == SkillEffectType::Heal
            || skill.effectType == SkillEffectType::AttackModifier
            || skill.effectType == SkillEffectType::AttackPenetrationModifier
            || skill.effectType == SkillEffectType::CooldownModifier
            || skill.effectType == SkillEffectType::HealingReceivedModifier
            || skill.effectType == SkillEffectType::Stunned
            || skill.effectType == SkillEffectType::Silenced
            || skill.effectType == SkillEffectType::Rooted
            || skill.effectType == SkillEffectType::Mark
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
            if (skill.effectValue >= 0)
            {
                if (skill.effectTarget == SkillEffectTarget::PlayerLineup)
                {
                    return "Boost the lineup's attacks for a short window.";
                }
                if (skill.effectTarget == SkillEffectTarget::Ally)
                {
                    return "Boost an ally's attacks for a short window.";
                }
                return "Set up a stronger follow-up play.";
            }
            return "Disrupt the opponent's next attacks.";
        }

        if (skill.effectType == SkillEffectType::AttackPenetrationModifier)
        {
            return "Prepare attacks that cut through enemy defenses.";
        }

        if (skill.effectType == SkillEffectType::CooldownModifier)
        {
            return skill.effectTarget == SkillEffectTarget::Self
                ? "Speed up your next ability cycle."
                : "Slow the opponent's next ability cycle.";
        }

        if (skill.effectType == SkillEffectType::HealingReceivedModifier)
        {
            return "Reduce the opponent's recovery window.";
        }

        if (skill.effectType == SkillEffectType::Stunned)
        {
            return "Stun the opponent and deny their next action.";
        }

        if (skill.effectType == SkillEffectType::Silenced)
        {
            return "Silence the opponent's non-basic abilities.";
        }

        if (skill.effectType == SkillEffectType::Rooted)
        {
            return "Root the opponent and prevent switching.";
        }

        if (skill.effectType == SkillEffectType::Mark)
        {
            return "Mark the opponent for a high-damage follow-up hit.";
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
    // Row format:
    // ID, name, mana cost, mana gain, cooldown, hidden power, accuracy,
    // optional effect, target, effect value, duration, mark bonus.
    skills_ = {
        MakeSkill("top-basic", "Basic attack", 0, Balance::BasicManaGain, 0, 16, 0.95),
        MakeSkill("top-hold-line", "Stomp", 35, 0, 2, 0, 1.00, SkillEffectType::Rooted, SkillEffectTarget::Enemy, 0, 1),
        MakeSkill("top-sunder", "Purge", 45, 0, 2, 28, 0.95, SkillEffectType::AttackPenetrationModifier, SkillEffectTarget::Self, 35, 2),
        MakeSkill("top-gamebreaker", "Unbreakable", 100, 0, 5, 28, 0.90, SkillEffectType::DefenseModifier, SkillEffectTarget::Self, 30, 2),

        MakeSkill("jungle-basic", "Basic attack", 0, Balance::BasicManaGain, 0, 16, 0.95),
        MakeSkill("jungle-gank", "Gank", 35, 0, 2, 14, 0.95, SkillEffectType::Stunned, SkillEffectTarget::Enemy, 0, 1),
        MakeSkill("jungle-invade", "Invade", 45, 0, 2, 30, 0.95, SkillEffectType::CooldownModifier, SkillEffectTarget::Enemy, 50, 2),
        MakeSkill("jungle-smite-fight", "Decimate", 100, 0, 5, 62, 0.90, SkillEffectType::Mark, SkillEffectTarget::Enemy, 0, 2, 28),

        MakeSkill("mid-basic", "Basic attack", 0, Balance::BasicManaGain, 0, 16, 0.95),
        MakeSkill("mid-silence", "Light binding", 35, 0, 2, 12, 0.95, SkillEffectType::Silenced, SkillEffectTarget::Enemy, 0, 1),
        MakeSkill("mid-burst", "Spark", 45, 0, 2, 34, 0.92, SkillEffectType::AttackModifier, SkillEffectTarget::Enemy, -30, 2),
        MakeSkill("mid-ultimate", "Sonic wave", 100, 0, 5, 65, 0.88, SkillEffectType::CooldownModifier, SkillEffectTarget::Self, -30, 2),

        MakeSkill("adc-basic", "Basic Attack", 0, Balance::BasicManaGain, 0, 16, 0.95),
        MakeSkill("adc-trap", "Place Trap", 35, 0, 2, 8, 1.00, SkillEffectType::Stunned, SkillEffectTarget::Enemy, 0, 2),
        MakeSkill("adc-multi-strike", "Multi Strike", 45, 0, 2, 36, 0.94),
        MakeSkill("adc-bullet-time", "Bullet Time", 100, 0, 5, 72, 0.88),

        MakeSkill("support-basic", "Basic attack", 0, Balance::BasicManaGain, 0, 14, 0.95),
        MakeSkill("support-peel", "Peel", 35, 0, 2, 0, 1.00, SkillEffectType::Heal, SkillEffectTarget::Ally, 40, 0),
        MakeSkill("support-shotcall", "Shotcall", 45, 0, 2, 0, 1.00, SkillEffectType::AttackModifier, SkillEffectTarget::Ally, 25, 2),
        MakeSkill("support-teamfight", "Teamfight plan", 100, 0, 5, 0, 1.00, SkillEffectType::AttackModifier, SkillEffectTarget::PlayerLineup, 10, 2)
    };
    FillSkillMetadata(skills_);

    drills_ = {
        MakeDrill(
            GameType::LeagueOfLegends,
            "moba-farm",
            "Farm",
            "Take a timing challenge to generate mana instead of attacking.",
            5,
            30,
            50)
    };

    traits_ = {
        MakeTrait(
            "clutch-player",
            "Clutch Player",
            "Below 35% HP, paid abilities cost 25% less mana.",
            TraitEffectType::LowHpManaDiscountPercent,
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
        { Spec::Top, "Top", { "top-basic", "top-hold-line", "top-sunder", "top-gamebreaker" }, Spec::Jungle, "clutch-player", 4, 0.20 },
        { Spec::Jungle, "Jungle", { "jungle-basic", "jungle-gank", "jungle-invade", "jungle-smite-fight" }, Spec::Mid, "shotcaller", 2, 0.35 },
        { Spec::Mid, "Mid", { "mid-basic", "mid-silence", "mid-burst", "mid-ultimate" }, Spec::Adc, "lane-bully", 1, 0.45 },
        { Spec::Adc, "ADC", { "adc-basic", "adc-trap", "adc-multi-strike", "adc-bullet-time" }, Spec::Support, "precision-carry", 1, 0.50 },
        { Spec::Support, "Support", { "support-basic", "support-peel", "support-shotcall", "support-teamfight" }, Spec::Top, "stabilizer", 3, 0.25 }
    };
    BuildIndexes();
}

const Skill* SimulationData::FindSkill(const std::string& id) const
{
    const auto found = skillIndexById_.find(id);
    if (found == skillIndexById_.end())
    {
        return nullptr;
    }

    return &skills_[found->second];
}

const DrillDefinition* SimulationData::FindDrill(GameType gameType) const
{
    const auto found = drillIndexByGameType_.find(static_cast<int>(gameType));
    if (found == drillIndexByGameType_.end())
    {
        return nullptr;
    }

    return &drills_[found->second];
}

const TraitDefinition* SimulationData::FindTrait(const std::string& id) const
{
    const auto found = traitIndexById_.find(id);
    if (found == traitIndexById_.end())
    {
        return nullptr;
    }

    return &traits_[found->second];
}

const SpecData* SimulationData::FindSpec(Spec spec) const
{
    const auto found = specIndexBySpec_.find(static_cast<int>(spec));
    if (found == specIndexBySpec_.end())
    {
        return nullptr;
    }

    return &specs_[found->second];
}

const std::vector<SpecData>& SimulationData::GetSpecs() const
{
    return specs_;
}

void SimulationData::BuildIndexes()
{
    skillIndexById_.clear();
    for (std::size_t index = 0; index < skills_.size(); ++index)
    {
        skillIndexById_[skills_[index].id] = index;
    }

    drillIndexByGameType_.clear();
    for (std::size_t index = 0; index < drills_.size(); ++index)
    {
        drillIndexByGameType_[static_cast<int>(drills_[index].gameType)] = index;
    }

    traitIndexById_.clear();
    for (std::size_t index = 0; index < traits_.size(); ++index)
    {
        traitIndexById_[traits_[index].id] = index;
    }

    specIndexBySpec_.clear();
    for (std::size_t index = 0; index < specs_.size(); ++index)
    {
        specIndexBySpec_[static_cast<int>(specs_[index].spec)] = index;
    }
}
