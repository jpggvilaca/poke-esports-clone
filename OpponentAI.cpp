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
    for (SkillProgress& progress : opponent.skills)
    {
        const Skill* definition = data_.FindSkill(progress.skillId);
        if (definition != nullptr
            && IsUsefulSkill(*definition, opponent, opponentStatus, player, playerStatus)
            && rules_.GetFocusCost(*definition, progress, opponent) <= opponent.focus)
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

    if (definition.effectType == SkillEffectType::Heal)
    {
        return opponent.hp < opponent.maxHp;
    }

    const BattleStatus& targetStatus = definition.effectTarget == SkillEffectTarget::Self
        ? opponentStatus
        : playerStatus;

    if (definition.effectType == SkillEffectType::AttackModifier)
    {
        return targetStatus.attackModifierHits == 0;
    }

    return targetStatus.defenseModifierHits == 0;
}
