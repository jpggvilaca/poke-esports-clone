#pragma once

#include "Models.h"

#include <array>

struct BattleStatusEffectSlot
{
    SkillEffectType type = SkillEffectType::None;
    int* value = nullptr;
    int* turns = nullptr;
};

inline std::array<BattleStatusEffectSlot, 8> GetBattleStatusEffectSlots(BattleStatus& status)
{
    return {{
        { SkillEffectType::AttackModifier, &status.attackModifierPercent, &status.attackModifierTurns },
        { SkillEffectType::DefenseModifier, &status.defenseModifierPercent, &status.defenseModifierTurns },
        { SkillEffectType::AttackPenetrationModifier, &status.attackPenetrationPercent, &status.attackPenetrationTurns },
        { SkillEffectType::CooldownModifier, &status.cooldownModifierPercent, &status.cooldownModifierTurns },
        { SkillEffectType::HealingReceivedModifier, &status.healingReceivedModifierPercent, &status.healingReceivedModifierTurns },
        { SkillEffectType::Stunned, nullptr, &status.stunnedTurns },
        { SkillEffectType::Silenced, nullptr, &status.silencedTurns },
        { SkillEffectType::Rooted, nullptr, &status.rootedTurns },
    }};
}

inline bool ApplyBattleStatusEffect(
    BattleStatus& status,
    SkillEffectType type,
    int value,
    int durationTurns,
    BattleActor markSource = BattleActor::None,
    int markBonusDamage = 0)
{
    if (type == SkillEffectType::Mark)
    {
        status.markTurns = durationTurns;
        status.markBonusDamage = markBonusDamage;
        status.markSource = markSource;
        return true;
    }

    for (BattleStatusEffectSlot slot : GetBattleStatusEffectSlots(status))
    {
        if (slot.type != type || slot.turns == nullptr)
        {
            continue;
        }

        if (slot.value != nullptr)
        {
            *slot.value = value;
        }
        *slot.turns = durationTurns;
        return true;
    }

    return false;
}
