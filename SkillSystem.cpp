#include "SkillSystem.h"

#include "BattleEventFactory.h"
#include "BattleStatusUtils.h"
#include "SimulationData.h"

#include <algorithm>
#include <vector>

SkillSystem::SkillSystem(
    const SimulationData& data,
    const BattleRules& rules,
    const ProgressionSystem& progression)
    : data_(data), rules_(rules), progression_(progression)
{
}

SkillView SkillSystem::CreateSkillView(
    const Skill& definition,
    const SkillProgress& progress,
    const Competitor& user,
    const BattleStatus& userStatus,
    int cooldownRemaining) const
{
    SkillView view;
    view.id = definition.id;
    view.name = definition.name;
    view.description = definition.description;
    view.sourceSpec = definition.sourceSpec;
    view.colorId = definition.colorId;
    view.tone = definition.tone;
    view.power = rules_.GetPower(definition, progress);
    view.manaCost = rules_.GetManaCost(definition, progress, user);
    view.manaGain = rules_.GetManaGain(definition, progress);
    view.cooldownTurns = rules_.GetCooldownTurns(definition, userStatus);
    view.cooldownRemaining = cooldownRemaining;
    view.accuracy = rules_.GetAccuracy(definition, progress, user);
    view.level = progress.level;
    view.xp = progress.xp;
    view.effectType = definition.effectType;
    view.effectTarget = definition.effectTarget;
    view.effectValue = rules_.GetEffectValue(definition, progress, user);
    view.durationTurns = definition.durationTurns;
    view.markBonusDamage = definition.markBonusDamage;
    view.effects = definition.effects;
    if (view.effects.empty() && definition.effectType != SkillEffectType::None)
    {
        view.effects.push_back({
            definition.effectType,
            definition.effectTarget,
            definition.effectValue,
            definition.durationTurns,
            definition.markBonusDamage,
        });
    }
    for (SkillEffectDefinition& effect : view.effects)
    {
        Skill effectDefinition = definition;
        effectDefinition.effectType = effect.type;
        effectDefinition.effectTarget = effect.target;
        effectDefinition.effectValue = effect.value;
        effectDefinition.durationTurns = effect.durationTurns;
        effectDefinition.markBonusDamage = effect.markBonusDamage;
        effect.value = rules_.GetEffectValue(effectDefinition, progress, user);
    }
    return view;
}

SkillUseResult SkillSystem::UseSkill(const SkillUseRequest& request, std::mt19937& randomEngine) const
{
    SkillUseResult result;
    if (request.competitor == nullptr
        || request.status == nullptr
        || request.progress == nullptr
        || request.target.competitor == nullptr
        || request.target.status == nullptr)
    {
        return result;
    }

    result.actor = request.actor;
    result.target = request.target.actor;
    result.skillId = request.progress->skillId;
    result.oldMana = request.competitor->mana;
    result.newMana = request.competitor->mana;
    result.oldActorHp = request.competitor->hp;
    result.newActorHp = request.competitor->hp;
    result.oldTargetHp = request.target.competitor->hp;
    result.newTargetHp = request.target.competitor->hp;

    const Skill* definition = data_.FindSkill(request.progress->skillId);
    if (definition == nullptr)
    {
        return result;
    }

    result.used = true;
    result.skillId = definition->id;
    EmitSkillStarted(request, *definition, result);
    ResolveSkillCost(request, *definition, result);

    if (!ResolveHitRoll(request, *definition, randomEngine, result))
    {
        AwardSkillXpStage(request, *definition, result);
        return result;
    }

    ResolveDamageStage(request, *definition, result);
    ResolvePostSkillReactionsStage(request, *definition, result);
    ResolveSecondaryEffectsStage(request, *definition, result);
    AwardSkillXpStage(request, *definition, result);
    return result;
}

void SkillSystem::ResolveSkillCost(
    const SkillUseRequest& request,
    const Skill& definition,
    SkillUseResult& result) const
{
    Competitor& attacker = *request.competitor;
    attacker.mana = std::clamp(
        attacker.mana - rules_.GetManaCost(definition, *request.progress, attacker) + rules_.GetManaGain(definition, *request.progress),
        0,
        attacker.maxMana);
    result.newMana = attacker.mana;

    if (result.oldMana == result.newMana)
    {
        return;
    }

    BattleEvent manaChanged = MakeActorEvent(
        BattleEventType::ManaChanged,
        MakeBattleEventParticipant(request.actor, request.playerIndex, attacker),
        definition.id);
    manaChanged.skillId = definition.id;
    manaChanged.oldValue = result.oldMana;
    manaChanged.newValue = result.newMana;
    manaChanged.amount = result.newMana - result.oldMana;
    result.events.push_back(manaChanged);
}

void SkillSystem::EmitSkillStarted(
    const SkillUseRequest& request,
    const Skill& definition,
    SkillUseResult& result) const
{
    const Competitor& attacker = *request.competitor;
    const Competitor& target = *request.target.competitor;
    BattleEvent skillStarted = MakeTargetedEvent(
        BattleEventType::SkillStarted,
        MakeBattleEventParticipant(request.actor, request.playerIndex, attacker),
        MakeBattleEventParticipant(request.target.actor, request.target.playerIndex, target),
        definition.id);
    result.events.push_back(skillStarted);
}

bool SkillSystem::ResolveHitRoll(
    const SkillUseRequest& request,
    const Skill& definition,
    std::mt19937& randomEngine,
    SkillUseResult& result) const
{
    const Competitor& attacker = *request.competitor;
    const Competitor& target = *request.target.competitor;
    if (Chance(rules_.GetAccuracy(definition, *request.progress, attacker), randomEngine))
    {
        return true;
    }

    result.hit = false;

    BattleEvent missed = MakeTargetedEvent(
        BattleEventType::AttackMissed,
        MakeBattleEventParticipant(request.actor, request.playerIndex, attacker),
        MakeBattleEventParticipant(request.target.actor, request.target.playerIndex, target),
        definition.id);
    result.events.push_back(missed);
    return false;
}

void SkillSystem::ResolveDamageStage(
    const SkillUseRequest& request,
    const Skill& definition,
    SkillUseResult& result) const
{
    if (definition.power <= 0)
    {
        return;
    }

    Competitor& attacker = *request.competitor;
    BattleStatus& attackerStatus = *request.status;
    Competitor& target = *request.target.competitor;
    BattleStatus& targetStatus = *request.target.status;
    result.damage = rules_.CalculateDamage(
        definition,
        *request.progress,
        attacker,
        attackerStatus,
        target,
        targetStatus);
    target.hp = std::max(0, target.hp - result.damage.amount);
    result.newTargetHp = target.hp;

    BattleEvent damageApplied = MakeTargetedEvent(
        BattleEventType::DamageApplied,
        MakeBattleEventParticipant(request.actor, request.playerIndex, attacker),
        MakeBattleEventParticipant(request.target.actor, request.target.playerIndex, target),
        definition.id);
    damageApplied.oldValue = result.oldTargetHp;
    damageApplied.newValue = result.newTargetHp;
    damageApplied.amount = result.damage.amount;
    damageApplied.damage = result.damage;
    result.events.push_back(damageApplied);
}

void SkillSystem::ResolvePostSkillReactionsStage(
    const SkillUseRequest& request,
    const Skill& definition,
    SkillUseResult& result) const
{
    (void)definition;
    if (!result.damage.applied || result.damage.markBonusDamage <= 0)
    {
        return;
    }

    BattleStatus& targetStatus = *request.target.status;
    if (targetStatus.markTurns <= 0)
    {
        return;
    }

    const Competitor& attacker = *request.competitor;
    const Competitor& target = *request.target.competitor;
    BattleEvent markTriggered = MakeTargetedEvent(
        BattleEventType::MarkTriggered,
        MakeBattleEventParticipant(request.actor, request.playerIndex, attacker),
        MakeBattleEventParticipant(request.target.actor, request.target.playerIndex, target),
        definition.id);
    markTriggered.amount = result.damage.markBonusDamage;
    result.events.push_back(markTriggered);
    targetStatus.markTurns = 0;
    targetStatus.markBonusDamage = 0;
    targetStatus.markSource = BattleActor::None;
}

std::vector<SkillEffectDefinition> SkillSystem::BuildEffectList(const Skill& definition) const
{
    std::vector<SkillEffectDefinition> effects = definition.effects;
    if (effects.empty() && definition.effectType != SkillEffectType::None)
    {
        effects.push_back({
            definition.effectType,
            definition.effectTarget,
            definition.effectValue,
            definition.durationTurns,
            definition.markBonusDamage,
        });
    }
    return effects;
}

void SkillSystem::ResolveSecondaryEffectsStage(
    const SkillUseRequest& request,
    const Skill& definition,
    SkillUseResult& result) const
{
    Competitor& attacker = *request.competitor;
    BattleStatus& attackerStatus = *request.status;
    Competitor& target = *request.target.competitor;
    BattleStatus& targetStatus = *request.target.status;
    for (const SkillEffectDefinition& effect : BuildEffectList(definition))
    {
        const int oldActorHp = attacker.hp;
        const int oldTargetHp = target.hp;
        SecondaryEffectResult appliedEffect = ApplySecondaryEffect(
            request.actor,
            request.target.actor,
            definition,
            effect,
            *request.progress,
            attacker,
            attackerStatus,
            target,
            targetStatus);
        result.newActorHp = attacker.hp;
        result.newTargetHp = target.hp;
        if (!appliedEffect.applied)
        {
            continue;
        }

        if (!result.effect.applied)
        {
            result.effect = appliedEffect;
        }
        result.effects.push_back(appliedEffect);

        const bool effectOnActor = effect.target == SkillEffectTarget::Self;
        BattleEvent effectApplied = MakeTargetedEvent(
            BattleEventType::StatusApplied,
            MakeBattleEventParticipant(request.actor, request.playerIndex, attacker),
            effectOnActor
                ? MakeBattleEventParticipant(request.actor, request.playerIndex, attacker)
                : MakeBattleEventParticipant(request.target.actor, request.target.playerIndex, target),
            definition.id);
        effectApplied.target = appliedEffect.target;
        effectApplied.effect = appliedEffect;

        if (appliedEffect.type == SkillEffectType::Heal)
        {
            effectApplied.type = BattleEventType::HealingApplied;
            effectApplied.oldValue = effectOnActor ? oldActorHp : oldTargetHp;
            effectApplied.newValue = effectOnActor ? result.newActorHp : result.newTargetHp;
            effectApplied.amount = appliedEffect.healingAmount;
        }
        else if (appliedEffect.type == SkillEffectType::Mark)
        {
            effectApplied.type = BattleEventType::MarkApplied;
            effectApplied.amount = appliedEffect.markBonusDamage;
        }
        else
        {
            effectApplied.type = BattleEventType::StatusApplied;
            effectApplied.amount = appliedEffect.value;
        }

        result.events.push_back(effectApplied);
    }
}

void SkillSystem::AwardSkillXpStage(
    const SkillUseRequest& request,
    const Skill& definition,
    SkillUseResult& result) const
{
    Competitor& attacker = *request.competitor;
    result.xp = progression_.AwardSkillXp(*request.progress);
    if (result.xp.xpGained > 0)
    {
        BattleEvent xpGained = MakeActorEvent(
            BattleEventType::SkillXpGained,
            MakeBattleEventParticipant(request.actor, request.playerIndex, attacker),
            definition.id);
        xpGained.oldValue = result.xp.oldXp;
        xpGained.newValue = result.xp.newXp;
        xpGained.amount = result.xp.xpGained;
        xpGained.xp = result.xp;
        result.events.push_back(xpGained);
    }

    if (!result.xp.leveledUp)
    {
        return;
    }

    BattleEvent leveledUp = MakeActorEvent(
        BattleEventType::SkillLeveledUp,
        MakeBattleEventParticipant(request.actor, request.playerIndex, attacker),
        definition.id);
    leveledUp.oldLevel = result.xp.oldLevel;
    leveledUp.newLevel = result.xp.newLevel;
    leveledUp.xp = result.xp;
    result.events.push_back(leveledUp);
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
    BattleActor targetActor,
    const Skill& definition,
    const SkillEffectDefinition& effect,
    const SkillProgress& progress,
    Competitor& attacker,
    BattleStatus& attackerStatus,
    Competitor& targetCompetitor,
    BattleStatus& targetBattleStatus) const
{
    SecondaryEffectResult result;
    if (effect.type == SkillEffectType::None
        || effect.target == SkillEffectTarget::PlayerLineup)
    {
        return result;
    }

    Skill effectDefinition = definition;
    effectDefinition.effectType = effect.type;
    effectDefinition.effectTarget = effect.target;
    effectDefinition.effectValue = effect.value;
    effectDefinition.durationTurns = effect.durationTurns;
    effectDefinition.markBonusDamage = effect.markBonusDamage;

    const int effectValue = rules_.GetEffectValue(effectDefinition, progress, attacker);
    BattleStatus& effectStatus = effect.target == SkillEffectTarget::Self
        ? attackerStatus
        : targetBattleStatus;
    Competitor& effectTargetCompetitor = effect.target == SkillEffectTarget::Self
        ? attacker
        : targetCompetitor;
    const BattleActor effectTarget = effect.target == SkillEffectTarget::Self
        ? actor
        : targetActor;

    result.type = effect.type;
    result.target = effectTarget;
    result.value = effectValue;
    result.duration = effect.durationTurns;

    if (effect.type == SkillEffectType::Heal)
    {
        result.applied = true;
        const int oldHp = effectTargetCompetitor.hp;
        int healing = effectValue;
        if (effectStatus.healingReceivedModifierTurns > 0)
        {
            healing = healing * (100 + effectStatus.healingReceivedModifierPercent) / 100;
        }
        effectTargetCompetitor.hp = std::min(effectTargetCompetitor.maxHp, effectTargetCompetitor.hp + healing);
        result.healingAmount = effectTargetCompetitor.hp - oldHp;
        return result;
    }

    result.applied = ApplyBattleStatusEffect(
        effectStatus,
        effect.type,
        effectValue,
        effect.durationTurns,
        actor,
        effect.markBonusDamage);
    if (effect.type == SkillEffectType::Mark && result.applied)
    {
        result.markBonusDamage = effect.markBonusDamage;
    }

    return result;
}
