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

void SkillSystem::UseSkill(
    BattleActor actor,
    Competitor& attacker,
    BattleStatus& attackerStatus,
    Competitor& defender,
    BattleStatus& defenderStatus,
    SkillProgress& progress,
    std::mt19937& randomEngine,
    std::vector<BattleEvent>& events) const
{
    const Skill* definition = data_.FindSkill(progress.skillId);
    if (definition == nullptr)
    {
        return;
    }

    const BattleActor target = Opposite(actor);
    attacker.focus -= rules_.GetFocusCost(*definition, progress);
    events.push_back({
        BattleEventType::SkillUsed,
        actor,
        target,
        definition->id
    });

    if (!Chance(rules_.GetAccuracy(*definition, progress), randomEngine))
    {
        events.push_back({
            BattleEventType::Missed,
            actor,
            target,
            definition->id
        });
        progression_.AwardSkillXp(actor, *definition, progress, events);
        return;
    }

    if (definition->power > 0)
    {
        const int damage = rules_.CalculateDamage(
            actor,
            target,
            *definition,
            progress,
            attacker,
            attackerStatus,
            defender,
            defenderStatus,
            events);
        defender.hp = std::max(0, defender.hp - damage);
        events.push_back({
            BattleEventType::DamageDealt,
            actor,
            target,
            definition->id,
            "",
            damage
        });
    }

    ApplySecondaryEffect(
        actor,
        target,
        *definition,
        progress,
        attacker,
        attackerStatus,
        defenderStatus,
        events);
    progression_.AwardSkillXp(actor, *definition, progress, events);
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

void SkillSystem::ApplySecondaryEffect(
    BattleActor actor,
    BattleActor target,
    const Skill& definition,
    const SkillProgress& progress,
    Competitor& attacker,
    BattleStatus& attackerStatus,
    BattleStatus& defenderStatus,
    std::vector<BattleEvent>& events) const
{
    if (definition.effectType == SkillEffectType::None)
    {
        return;
    }

    const int effectValue = rules_.GetEffectValue(definition, progress);
    BattleStatus& targetStatus = definition.effectTarget == SkillEffectTarget::Self
        ? attackerStatus
        : defenderStatus;
    const BattleActor effectTarget = definition.effectTarget == SkillEffectTarget::Self
        ? actor
        : target;

    if (definition.effectType == SkillEffectType::Heal)
    {
        const int oldHp = attacker.hp;
        attacker.hp = std::min(attacker.maxHp, attacker.hp + effectValue);
        events.push_back({
            BattleEventType::Healed,
            actor,
            actor,
            definition.id,
            "",
            attacker.hp - oldHp
        });
        return;
    }

    BattleEvent event;
    event.actor = actor;
    event.target = effectTarget;
    event.skillId = definition.id;
    event.value = effectValue;
    event.duration = definition.effectUses;

    if (definition.effectType == SkillEffectType::AttackModifier)
    {
        targetStatus.attackModifierPercent = effectValue;
        targetStatus.attackModifierHits = definition.effectUses;
        event.type = BattleEventType::AttackModified;
    }
    else
    {
        targetStatus.defenseModifierPercent = effectValue;
        targetStatus.defenseModifierHits = definition.effectUses;
        event.type = BattleEventType::DefenseModified;
    }

    events.push_back(event);
}
