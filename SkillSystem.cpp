#include "SkillSystem.h"

#include "SimulationData.h"

#include <algorithm>

SkillSystem::SkillSystem(
    const SimulationData& data,
    const BattleRules& rules,
    const ProgressionSystem& progression)
    : data_(data), rules_(rules), progression_(progression)
{
}

bool SkillSystem::IsAvailableForStyle(const Skill& definition, Style style) const
{
    return !definition.requiredStyle.has_value() || definition.requiredStyle.value() == style;
}

SkillView SkillSystem::CreateSkillView(const Skill& definition, const SkillProgress& progress) const
{
    SkillView view;
    view.id = definition.id;
    view.name = definition.name;
    view.power = rules_.GetPower(definition, progress);
    view.focusCost = rules_.GetFocusCost(definition, progress);
    view.accuracy = rules_.GetAccuracy(definition, progress);
    view.level = progress.level;
    view.xp = progress.xp;
    view.effectType = definition.effectType;
    view.effectTarget = definition.effectTarget;
    view.effectValue = rules_.GetEffectValue(definition, progress);
    view.effectUses = definition.effectUses;
    return view;
}

SkillUseResult SkillSystem::UseSkill(
    BattleActor actor,
    Competitor& attacker,
    BattleStatus& attackerStatus,
    Competitor& defender,
    BattleStatus& defenderStatus,
    SkillProgress& progress,
    std::mt19937& randomEngine) const
{
    SkillUseResult result;
    result.actor = actor;
    result.target = Opposite(actor);
    result.skillId = progress.skillId;

    const Skill* definition = data_.FindSkill(progress.skillId);
    if (definition == nullptr)
    {
        return result;
    }

    result.used = true;
    result.skillId = definition->id;
    attacker.focus -= rules_.GetFocusCost(*definition, progress);

    if (!Chance(rules_.GetAccuracy(*definition, progress), randomEngine))
    {
        result.hit = false;
        result.xp = progression_.AwardSkillXp(progress);
        return result;
    }

    if (definition->power > 0)
    {
        result.damage = rules_.CalculateDamage(
            *definition,
            progress,
            attacker,
            attackerStatus,
            defender,
            defenderStatus);
        defender.hp = std::max(0, defender.hp - result.damage.amount);
    }

    result.effect = ApplySecondaryEffect(
        actor,
        result.target,
        *definition,
        progress,
        attacker,
        attackerStatus,
        defenderStatus);
    result.xp = progression_.AwardSkillXp(progress);
    return result;
}

BattleActor SkillSystem::Opposite(BattleActor actor) const
{
    return actor == BattleActor::Player ? BattleActor::Opponent : BattleActor::Player;
}

bool SkillSystem::Chance(double probability, std::mt19937& randomEngine) const
{
    std::uniform_real_distribution<double> roll(0.0, 1.0);
    return roll(randomEngine) < probability;
}

SecondaryEffectResult SkillSystem::ApplySecondaryEffect(
    BattleActor actor,
    BattleActor target,
    const Skill& definition,
    const SkillProgress& progress,
    Competitor& attacker,
    BattleStatus& attackerStatus,
    BattleStatus& defenderStatus) const
{
    SecondaryEffectResult result;
    if (definition.effectType == SkillEffectType::None)
    {
        return result;
    }

    const int effectValue = rules_.GetEffectValue(definition, progress);
    BattleStatus& targetStatus = definition.effectTarget == SkillEffectTarget::Self
        ? attackerStatus
        : defenderStatus;
    const BattleActor effectTarget = definition.effectTarget == SkillEffectTarget::Self
        ? actor
        : target;

    result.applied = true;
    result.type = definition.effectType;
    result.target = effectTarget;
    result.value = effectValue;
    result.duration = definition.effectUses;

    if (definition.effectType == SkillEffectType::Heal)
    {
        const int oldHp = attacker.hp;
        attacker.hp = std::min(attacker.maxHp, attacker.hp + effectValue);
        result.healingAmount = attacker.hp - oldHp;
        return result;
    }

    if (definition.effectType == SkillEffectType::AttackModifier)
    {
        targetStatus.attackModifierPercent = effectValue;
        targetStatus.attackModifierHits = definition.effectUses;
    }
    else
    {
        targetStatus.defenseModifierPercent = effectValue;
        targetStatus.defenseModifierHits = definition.effectUses;
    }

    return result;
}
