#include "SkillSystem.h"

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
    view.canUse = true;
    if (userStatus.stunnedTurns > 0)
    {
        view.canUse = false;
        view.disabledReason = "Stunned";
    }
    else if (userStatus.silencedTurns > 0 && view.manaCost > 0)
    {
        view.canUse = false;
        view.disabledReason = "Silenced";
    }
    else if (cooldownRemaining > 0)
    {
        view.canUse = false;
        view.disabledReason = "Cooldown " + std::to_string(cooldownRemaining);
    }
    else if (view.manaCost > user.mana)
    {
        view.canUse = false;
        view.disabledReason = "Need " + std::to_string(view.manaCost) + " mana";
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

    BattleEvent manaChanged;
    manaChanged.type = BattleEventType::ManaChanged;
    manaChanged.actor = request.actor;
    manaChanged.actorPlayerIndex = request.playerIndex;
    manaChanged.profileIndex = attacker.profileIndex;
    manaChanged.actorName = attacker.name;
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
    BattleEvent skillStarted;
    skillStarted.type = BattleEventType::SkillStarted;
    skillStarted.actor = request.actor;
    skillStarted.target = request.target.actor;
    skillStarted.actorPlayerIndex = request.playerIndex;
    skillStarted.targetPlayerIndex = request.target.playerIndex;
    skillStarted.profileIndex = attacker.profileIndex;
    skillStarted.targetProfileIndex = target.profileIndex;
    skillStarted.actorName = attacker.name;
    skillStarted.targetName = target.name;
    skillStarted.skillId = definition.id;
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

    BattleEvent missed;
    missed.type = BattleEventType::AttackMissed;
    missed.actor = request.actor;
    missed.target = request.target.actor;
    missed.actorPlayerIndex = request.playerIndex;
    missed.targetPlayerIndex = request.target.playerIndex;
    missed.profileIndex = attacker.profileIndex;
    missed.targetProfileIndex = target.profileIndex;
    missed.actorName = attacker.name;
    missed.targetName = target.name;
    missed.skillId = definition.id;
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

    BattleEvent damageApplied;
    damageApplied.type = BattleEventType::DamageApplied;
    damageApplied.actor = request.actor;
    damageApplied.target = request.target.actor;
    damageApplied.actorPlayerIndex = request.playerIndex;
    damageApplied.targetPlayerIndex = request.target.playerIndex;
    damageApplied.profileIndex = attacker.profileIndex;
    damageApplied.targetProfileIndex = target.profileIndex;
    damageApplied.actorName = attacker.name;
    damageApplied.targetName = target.name;
    damageApplied.skillId = definition.id;
    damageApplied.oldValue = result.oldTargetHp;
    damageApplied.newValue = result.newTargetHp;
    damageApplied.amount = result.damage.amount;
    damageApplied.damage = result.damage;
    result.events.push_back(damageApplied);

    if (result.damage.markBonusDamage <= 0)
    {
        return;
    }

    BattleEvent markTriggered;
    markTriggered.type = BattleEventType::MarkTriggered;
    markTriggered.actor = request.actor;
    markTriggered.target = request.target.actor;
    markTriggered.actorPlayerIndex = request.playerIndex;
    markTriggered.targetPlayerIndex = request.target.playerIndex;
    markTriggered.profileIndex = attacker.profileIndex;
    markTriggered.targetProfileIndex = target.profileIndex;
    markTriggered.actorName = attacker.name;
    markTriggered.targetName = target.name;
    markTriggered.skillId = definition.id;
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
            targetStatus,
            request.playerIndex,
            request.target.playerIndex);
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
        BattleEvent effectApplied;
        effectApplied.actor = request.actor;
        effectApplied.target = appliedEffect.target;
        effectApplied.actorPlayerIndex = request.playerIndex;
        effectApplied.targetPlayerIndex = effectOnActor ? request.playerIndex : request.target.playerIndex;
        effectApplied.profileIndex = attacker.profileIndex;
        effectApplied.targetProfileIndex = effectOnActor ? attacker.profileIndex : target.profileIndex;
        effectApplied.actorName = attacker.name;
        effectApplied.targetName = effectOnActor ? attacker.name : target.name;
        effectApplied.skillId = definition.id;
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
        BattleEvent xpGained;
        xpGained.type = BattleEventType::SkillXpGained;
        xpGained.actor = request.actor;
        xpGained.actorPlayerIndex = request.playerIndex;
        xpGained.profileIndex = attacker.profileIndex;
        xpGained.actorName = attacker.name;
        xpGained.skillId = definition.id;
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

    BattleEvent leveledUp;
    leveledUp.type = BattleEventType::SkillLeveledUp;
    leveledUp.actor = request.actor;
    leveledUp.actorPlayerIndex = request.playerIndex;
    leveledUp.profileIndex = attacker.profileIndex;
    leveledUp.actorName = attacker.name;
    leveledUp.skillId = definition.id;
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
    BattleStatus& targetBattleStatus,
    int actorPlayerIndex,
    int targetPlayerIndex) const
{
    (void)actorPlayerIndex;
    (void)targetPlayerIndex;
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

    result.applied = true;
    result.type = effect.type;
    result.target = effectTarget;
    result.value = effectValue;
    result.duration = effect.durationTurns;

    if (effect.type == SkillEffectType::Heal)
    {
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

    if (effect.type == SkillEffectType::AttackModifier)
    {
        effectStatus.attackModifierPercent = effectValue;
        effectStatus.attackModifierTurns = effect.durationTurns;
    }
    else if (effect.type == SkillEffectType::DefenseModifier)
    {
        effectStatus.defenseModifierPercent = effectValue;
        effectStatus.defenseModifierTurns = effect.durationTurns;
    }
    else if (effect.type == SkillEffectType::AttackPenetrationModifier)
    {
        effectStatus.attackPenetrationPercent = effectValue;
        effectStatus.attackPenetrationTurns = effect.durationTurns;
    }
    else if (effect.type == SkillEffectType::CooldownModifier)
    {
        effectStatus.cooldownModifierPercent = effectValue;
        effectStatus.cooldownModifierTurns = effect.durationTurns;
    }
    else if (effect.type == SkillEffectType::HealingReceivedModifier)
    {
        effectStatus.healingReceivedModifierPercent = effectValue;
        effectStatus.healingReceivedModifierTurns = effect.durationTurns;
    }
    else if (effect.type == SkillEffectType::Stunned)
    {
        effectStatus.stunnedTurns = effect.durationTurns;
    }
    else if (effect.type == SkillEffectType::Silenced)
    {
        effectStatus.silencedTurns = effect.durationTurns;
    }
    else if (effect.type == SkillEffectType::Rooted)
    {
        effectStatus.rootedTurns = effect.durationTurns;
    }
    else if (effect.type == SkillEffectType::Mark)
    {
        effectStatus.markTurns = effect.durationTurns;
        effectStatus.markBonusDamage = effect.markBonusDamage;
        effectStatus.markSource = actor;
        result.markBonusDamage = effect.markBonusDamage;
    }

    return result;
}
