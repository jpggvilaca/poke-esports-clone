#include "OpponentAI.h"

#include "SimulationData.h"

#include <vector>

OpponentAI::OpponentAI(
    const SimulationData& data,
    const BattleRules& rules,
    const SkillSystem& skills)
    : data_(data), rules_(rules), skills_(skills)
{
}

SkillProgress* OpponentAI::SelectSkill(
    Competitor& opponent,
    const BattleStatus& opponentStatus,
    const Competitor& player,
    const BattleStatus& playerStatus,
    std::mt19937& randomEngine) const
{
    std::vector<SkillProgress*> affordableSkills;
    if (opponentStatus.stunnedTurns > 0)
    {
        return nullptr;
    }

    for (SkillProgress& progress : opponent.skills)
    {
        const Skill* definition = data_.FindSkill(progress.skillId);
        const AbilityRuntimeState* abilityState = opponent.FindAbilityState(progress.skillId);
        const int cooldownRemaining = abilityState == nullptr ? 0 : abilityState->cooldownRemaining;
        if (definition != nullptr
            && IsUsefulSkill(*definition, opponent, opponentStatus, player, playerStatus)
            && cooldownRemaining <= 0
            && rules_.GetManaCost(*definition, progress, opponent) <= opponent.mana
            && !(opponentStatus.silencedTurns > 0 && definition->manaCost > 0))
        {
            affordableSkills.push_back(&progress);
        }
    }

    // Every competitor starts with a free basic action. Returning nullptr keeps
    // a malformed content edit from crashing the game.
    if (affordableSkills.empty())
    {
        return nullptr;
    }

    std::uniform_int_distribution<int> chooseSkill(0, static_cast<int>(affordableSkills.size()) - 1);
    return affordableSkills[chooseSkill(randomEngine)];
}

bool OpponentAI::IsUsefulSkill(
    const Skill& definition,
    const Competitor& opponent,
    const BattleStatus& opponentStatus,
    const Competitor& player,
    const BattleStatus& playerStatus) const
{
    (void)player;

    // The initial AI is deliberately simple. It avoids pure utility actions
    // when their result is already active.
    if (definition.power > 0 || definition.effectType == SkillEffectType::None)
    {
        return true;
    }

    if (definition.effectTarget == SkillEffectTarget::PlayerLineup)
    {
        return false;
    }

    if (definition.effectType == SkillEffectType::Heal)
    {
        return opponent.hp < opponent.maxHp;
    }

    const BattleStatus& targetStatus = (definition.effectTarget == SkillEffectTarget::Self
            || definition.effectTarget == SkillEffectTarget::Ally)
        ? opponentStatus
        : playerStatus;

    if (definition.effectType == SkillEffectType::AttackModifier)
    {
        return targetStatus.attackModifierTurns == 0;
    }

    if (definition.effectType == SkillEffectType::AttackPenetrationModifier)
    {
        return targetStatus.attackPenetrationTurns == 0;
    }

    if (definition.effectType == SkillEffectType::CooldownModifier)
    {
        return targetStatus.cooldownModifierTurns == 0;
    }

    if (definition.effectType == SkillEffectType::HealingReceivedModifier)
    {
        return targetStatus.healingReceivedModifierTurns == 0;
    }

    if (definition.effectType == SkillEffectType::Stunned)
    {
        return targetStatus.stunnedTurns == 0;
    }

    if (definition.effectType == SkillEffectType::Silenced)
    {
        return targetStatus.silencedTurns == 0;
    }

    if (definition.effectType == SkillEffectType::Rooted)
    {
        return targetStatus.rootedTurns == 0;
    }

    if (definition.effectType == SkillEffectType::Mark)
    {
        return targetStatus.markTurns == 0;
    }

    return targetStatus.defenseModifierTurns == 0;
}
