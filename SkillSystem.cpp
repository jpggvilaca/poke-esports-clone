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

SkillUseResult SkillSystem::UseSkill(
    BattleActor actor,
    Competitor& attacker,
    BattleStatus& attackerStatus,
    BattleActor targetActor,
    Competitor& target,
    BattleStatus& targetStatus,
    SkillProgress& progress,
    int actorPlayerIndex,
    int targetPlayerIndex,
    std::mt19937& randomEngine) const
{
    SkillUseResult result;
    result.actor = actor;
    result.target = targetActor;
    result.skillId = progress.skillId;
    result.oldMana = attacker.mana;
    result.newMana = attacker.mana;
    result.oldActorHp = attacker.hp;
    result.newActorHp = attacker.hp;
    result.oldTargetHp = target.hp;
    result.newTargetHp = target.hp;

    const Skill* definition = data_.FindSkill(progress.skillId);
    if (definition == nullptr)
    {
        return result;
    }

    result.used = true;
    result.skillId = definition->id;
    attacker.mana = std::clamp(
        attacker.mana - rules_.GetManaCost(*definition, progress, attacker) + rules_.GetManaGain(*definition, progress),
        0,
        attacker.maxMana);
    result.newMana = attacker.mana;

    BattleEvent skillStarted;
    skillStarted.type = BattleEventType::SkillStarted;
    skillStarted.actor = actor;
    skillStarted.target = result.target;
    skillStarted.actorPlayerIndex = actorPlayerIndex;
    skillStarted.targetPlayerIndex = targetPlayerIndex;
    skillStarted.profileIndex = attacker.profileIndex;
    skillStarted.targetProfileIndex = target.profileIndex;
    skillStarted.actorName = attacker.name;
    skillStarted.targetName = target.name;
    skillStarted.skillId = definition->id;
    result.events.push_back(skillStarted);

    if (result.oldMana != result.newMana)
    {
        BattleEvent manaChanged;
        manaChanged.type = BattleEventType::ManaChanged;
        manaChanged.actor = actor;
        manaChanged.actorPlayerIndex = actorPlayerIndex;
        manaChanged.profileIndex = attacker.profileIndex;
        manaChanged.actorName = attacker.name;
        manaChanged.skillId = definition->id;
        manaChanged.oldValue = result.oldMana;
        manaChanged.newValue = result.newMana;
        manaChanged.amount = result.newMana - result.oldMana;
        result.events.push_back(manaChanged);
    }

    if (!Chance(rules_.GetAccuracy(*definition, progress, attacker), randomEngine))
    {
        result.hit = false;
        BattleEvent missed;
        missed.type = BattleEventType::AttackMissed;
        missed.actor = actor;
        missed.target = result.target;
        missed.actorPlayerIndex = actorPlayerIndex;
        missed.targetPlayerIndex = targetPlayerIndex;
        missed.profileIndex = attacker.profileIndex;
        missed.targetProfileIndex = target.profileIndex;
        missed.actorName = attacker.name;
        missed.targetName = target.name;
        missed.skillId = definition->id;
        result.events.push_back(missed);

        result.xp = progression_.AwardSkillXp(progress);
        if (result.xp.xpGained > 0)
        {
            BattleEvent xpGained;
            xpGained.type = BattleEventType::SkillXpGained;
            xpGained.actor = actor;
            xpGained.actorPlayerIndex = actorPlayerIndex;
            xpGained.profileIndex = attacker.profileIndex;
            xpGained.actorName = attacker.name;
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
            leveledUp.actorPlayerIndex = actorPlayerIndex;
            leveledUp.profileIndex = attacker.profileIndex;
            leveledUp.actorName = attacker.name;
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
            target,
            targetStatus);
        target.hp = std::max(0, target.hp - result.damage.amount);
        result.newTargetHp = target.hp;

        BattleEvent damageApplied;
        damageApplied.type = BattleEventType::DamageApplied;
        damageApplied.actor = actor;
        damageApplied.target = result.target;
        damageApplied.actorPlayerIndex = actorPlayerIndex;
        damageApplied.targetPlayerIndex = targetPlayerIndex;
        damageApplied.profileIndex = attacker.profileIndex;
        damageApplied.targetProfileIndex = target.profileIndex;
        damageApplied.actorName = attacker.name;
        damageApplied.targetName = target.name;
        damageApplied.skillId = definition->id;
        damageApplied.oldValue = result.oldTargetHp;
        damageApplied.newValue = result.newTargetHp;
        damageApplied.amount = result.damage.amount;
        damageApplied.damage = result.damage;
        result.events.push_back(damageApplied);

        if (result.damage.markBonusDamage > 0)
        {
            BattleEvent markTriggered;
            markTriggered.type = BattleEventType::MarkTriggered;
            markTriggered.actor = actor;
            markTriggered.target = result.target;
            markTriggered.actorPlayerIndex = actorPlayerIndex;
            markTriggered.targetPlayerIndex = targetPlayerIndex;
            markTriggered.profileIndex = attacker.profileIndex;
            markTriggered.targetProfileIndex = target.profileIndex;
            markTriggered.actorName = attacker.name;
            markTriggered.targetName = target.name;
            markTriggered.skillId = definition->id;
            markTriggered.amount = result.damage.markBonusDamage;
            result.events.push_back(markTriggered);
            targetStatus.markTurns = 0;
            targetStatus.markBonusDamage = 0;
            targetStatus.markSource = BattleActor::None;
        }
    }

    std::vector<SkillEffectDefinition> effects = definition->effects;
    if (effects.empty() && definition->effectType != SkillEffectType::None)
    {
        effects.push_back({
            definition->effectType,
            definition->effectTarget,
            definition->effectValue,
            definition->durationTurns,
            definition->markBonusDamage,
        });
    }

    for (const SkillEffectDefinition& effect : effects)
    {
        const int oldActorHp = attacker.hp;
        const int oldTargetHp = target.hp;
        SecondaryEffectResult appliedEffect = ApplySecondaryEffect(
            actor,
            result.target,
            *definition,
            effect,
            progress,
            attacker,
            attackerStatus,
            target,
            targetStatus,
            actorPlayerIndex,
            targetPlayerIndex);
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
        effectApplied.actor = actor;
        effectApplied.target = appliedEffect.target;
        effectApplied.actorPlayerIndex = actorPlayerIndex;
        effectApplied.targetPlayerIndex = effectOnActor ? actorPlayerIndex : targetPlayerIndex;
        effectApplied.profileIndex = attacker.profileIndex;
        effectApplied.targetProfileIndex = effectOnActor ? attacker.profileIndex : target.profileIndex;
        effectApplied.actorName = attacker.name;
        effectApplied.targetName = effectOnActor ? attacker.name : target.name;
        effectApplied.skillId = definition->id;
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

    result.xp = progression_.AwardSkillXp(progress);
    if (result.xp.xpGained > 0)
    {
        BattleEvent xpGained;
        xpGained.type = BattleEventType::SkillXpGained;
        xpGained.actor = actor;
        xpGained.actorPlayerIndex = actorPlayerIndex;
        xpGained.profileIndex = attacker.profileIndex;
        xpGained.actorName = attacker.name;
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
        leveledUp.actorPlayerIndex = actorPlayerIndex;
        leveledUp.profileIndex = attacker.profileIndex;
        leveledUp.actorName = attacker.name;
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
