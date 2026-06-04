#include "ProgressionSystem.h"

#include "SimulationData.h"

void ProgressionSystem::AwardSkillXp(
    BattleActor actor,
    const Skill& definition,
    SkillProgress& progress,
    std::vector<BattleEvent>& events) const
{
    progress.xp += Balance::SkillXpPerUse;
    while (progress.xp >= Balance::SkillXpPerLevel)
    {
        progress.xp -= Balance::SkillXpPerLevel;
        ++progress.level;
        events.push_back({
            BattleEventType::SkillLeveledUp,
            actor,
            actor,
            definition.id,
            "",
            progress.level
        });
    }
}
