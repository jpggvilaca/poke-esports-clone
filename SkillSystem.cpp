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
    view.description = definition.description;
    view.tone = definition.tone;
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
    result.oldFocus = attacker.focus;
    result.newFocus = attacker.focus;
    result.oldActorHp = attacker.hp;
    result.newActorHp = attacker.hp;
    result.oldTargetHp = defender.hp;
    result.newTargetHp = defender.hp;

    const Skill* definition = data_.FindSkill(progress.skillId);
    if (definition == nullptr)
    {
        return result;
    }

    result.used = true;
    result.skillId = definition->id;
    attacker.focus -= rules_.GetFocusCost(*definition, progress);
    result.newFocus = attacker.focus;

    BattleEvent skillStarted;
    skillStarted.type = BattleEventType::SkillStarted;
    skillStarted.actor = actor;
    skillStarted.target = result.target;
    skillStarted.skillId = definition->id;
    result.events.push_back(skillStarted);

    if (result.oldFocus != result.newFocus)
    {
        BattleEvent focusChanged;
        focusChanged.type = BattleEventType::FocusChanged;
        focusChanged.actor = actor;
        focusChanged.skillId = definition->id;
        focusChanged.oldValue = result.oldFocus;
        focusChanged.newValue = result.newFocus;
        focusChanged.amount = result.oldFocus - result.newFocus;
        result.events.push_back(focusChanged);
    }

    if (!Chance(rules_.GetAccuracy(*definition, progress), randomEngine))
    {
        result.hit = false;
        BattleEvent missed;
        missed.type = BattleEventType::AttackMissed;
        missed.actor = actor;
        missed.target = result.target;
        missed.skillId = definition->id;
        result.events.push_back(missed);

        result.xp = progression_.AwardSkillXp(progress);
        if (result.xp.xpGained > 0)
        {
            BattleEvent xpGained;
            xpGained.type = BattleEventType::SkillXpGained;
            xpGained.actor = actor;
            xpGained.skillId = definition->id;
            xpGained.oldValue = result.xp.oldXp;
            xpGained.newValue = result.xp.newXp;
            xpGained.amount = result.xp.xpGained;
            xpGained.xp = result.xp;
            result.events.push_back(xpGained);
        }

        if (result.xp.leveledUp)
        {
            BattleEvent leveledUp;
            leveledUp.type = BattleEventType::SkillLeveledUp;
            leveledUp.actor = actor;
            leveledUp.skillId = definition->id;
            leveledUp.oldLevel = result.xp.oldLevel;
            leveledUp.newLevel = result.xp.newLevel;
            leveledUp.xp = result.xp;
            result.events.push_back(leveledUp);
        }
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
        result.newTargetHp = defender.hp;

        BattleEvent damageApplied;
        damageApplied.type = BattleEventType::DamageApplied;
        damageApplied.actor = actor;
        damageApplied.target = result.target;
        damageApplied.skillId = definition->id;
        damageApplied.oldValue = result.oldTargetHp;
        damageApplied.newValue = result.newTargetHp;
        damageApplied.amount = result.damage.amount;
        damageApplied.damage = result.damage;
        result.events.push_back(damageApplied);
    }

    result.effect = ApplySecondaryEffect(
        actor,
        result.target,
        *definition,
        progress,
        attacker,
        attackerStatus,
        defenderStatus);
    result.newActorHp = attacker.hp;
    result.newTargetHp = defender.hp;
    if (result.effect.applied)
    {
        BattleEvent effectApplied;
        effectApplied.actor = actor;
        effectApplied.target = result.effect.target;
        effectApplied.skillId = definition->id;
        effectApplied.effect = result.effect;

        if (result.effect.type == SkillEffectType::Heal)
        {
            effectApplied.type = BattleEventType::HealingApplied;
            effectApplied.oldValue = result.oldActorHp;
            effectApplied.newValue = result.newActorHp;
            effectApplied.amount = result.effect.healingAmount;
        }
        else
        {
            effectApplied.type = BattleEventType::StatusApplied;
            effectApplied.amount = result.effect.value;
        }

        result.events.push_back(effectApplied);
    }

    result.xp = progression_.AwardSkillXp(progress);
    if (result.xp.xpGained > 0)
    {
        BattleEvent xpGained;
        xpGained.type = BattleEventType::SkillXpGained;
        xpGained.actor = actor;
        xpGained.skillId = definition->id;
        xpGained.oldValue = result.xp.oldXp;
        xpGained.newValue = result.xp.newXp;
        xpGained.amount = result.xp.xpGained;
        xpGained.xp = result.xp;
        result.events.push_back(xpGained);
    }

    if (result.xp.leveledUp)
    {
        BattleEvent leveledUp;
        leveledUp.type = BattleEventType::SkillLeveledUp;
        leveledUp.actor = actor;
        leveledUp.skillId = definition->id;
        leveledUp.oldLevel = result.xp.oldLevel;
        leveledUp.newLevel = result.xp.newLevel;
        leveledUp.xp = result.xp;
        result.events.push_back(leveledUp);
    }
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
