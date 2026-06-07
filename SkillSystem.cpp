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
    Competitor& defender,
    BattleStatus& defenderStatus,
    SkillProgress& progress,
    std::mt19937& randomEngine) const
{
    SkillUseResult result;
    result.actor = actor;
    result.target = Opposite(actor);
    result.skillId = progress.skillId;
    result.oldMana = attacker.mana;
    result.newMana = attacker.mana;
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
    attacker.mana = std::clamp(
        attacker.mana - rules_.GetManaCost(*definition, progress, attacker) + rules_.GetManaGain(*definition, progress),
        0,
        attacker.maxMana);
    result.newMana = attacker.mana;

    BattleEvent skillStarted;
    skillStarted.type = BattleEventType::SkillStarted;
    skillStarted.actor = actor;
    skillStarted.target = result.target;
    skillStarted.skillId = definition->id;
    result.events.push_back(skillStarted);

    if (result.oldMana != result.newMana)
    {
        BattleEvent manaChanged;
        manaChanged.type = BattleEventType::ManaChanged;
        manaChanged.actor = actor;
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

        if (result.damage.markBonusDamage > 0)
        {
            BattleEvent markTriggered;
            markTriggered.type = BattleEventType::MarkTriggered;
            markTriggered.actor = actor;
            markTriggered.target = result.target;
            markTriggered.skillId = definition->id;
            markTriggered.amount = result.damage.markBonusDamage;
            result.events.push_back(markTriggered);
            defenderStatus.markTurns = 0;
            defenderStatus.markBonusDamage = 0;
            defenderStatus.markSource = BattleActor::None;
        }
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
        else if (result.effect.type == SkillEffectType::Mark)
        {
            effectApplied.type = BattleEventType::MarkApplied;
            effectApplied.amount = result.effect.markBonusDamage;
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

    const int effectValue = rules_.GetEffectValue(definition, progress, attacker);
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
    result.duration = definition.durationTurns;

    if (definition.effectType == SkillEffectType::Heal)
    {
        const int oldHp = attacker.hp;
        int healing = effectValue;
        if (attackerStatus.healingReceivedModifierTurns > 0)
        {
            healing = healing * (100 + attackerStatus.healingReceivedModifierPercent) / 100;
        }
        attacker.hp = std::min(attacker.maxHp, attacker.hp + healing);
        result.healingAmount = attacker.hp - oldHp;
        return result;
    }

    if (definition.effectType == SkillEffectType::AttackModifier)
    {
        targetStatus.attackModifierPercent = effectValue;
        targetStatus.attackModifierTurns = definition.durationTurns;
    }
    else if (definition.effectType == SkillEffectType::DefenseModifier)
    {
        targetStatus.defenseModifierPercent = effectValue;
        targetStatus.defenseModifierTurns = definition.durationTurns;
    }
    else if (definition.effectType == SkillEffectType::AttackPenetrationModifier)
    {
        targetStatus.attackPenetrationPercent = effectValue;
        targetStatus.attackPenetrationTurns = definition.durationTurns;
    }
    else if (definition.effectType == SkillEffectType::CooldownModifier)
    {
        targetStatus.cooldownModifierPercent = effectValue;
        targetStatus.cooldownModifierTurns = definition.durationTurns;
    }
    else if (definition.effectType == SkillEffectType::HealingReceivedModifier)
    {
        targetStatus.healingReceivedModifierPercent = effectValue;
        targetStatus.healingReceivedModifierTurns = definition.durationTurns;
    }
    else if (definition.effectType == SkillEffectType::Stunned)
    {
        targetStatus.stunnedTurns = definition.durationTurns;
    }
    else if (definition.effectType == SkillEffectType::Silenced)
    {
        targetStatus.silencedTurns = definition.durationTurns;
    }
    else if (definition.effectType == SkillEffectType::Rooted)
    {
        targetStatus.rootedTurns = definition.durationTurns;
    }
    else if (definition.effectType == SkillEffectType::Mark)
    {
        targetStatus.markTurns = definition.durationTurns;
        targetStatus.markBonusDamage = definition.markBonusDamage;
        targetStatus.markSource = actor;
        result.markBonusDamage = definition.markBonusDamage;
    }

    return result;
}
