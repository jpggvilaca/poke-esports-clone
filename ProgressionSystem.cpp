#include "ProgressionSystem.h"

#include "SimulationData.h"

SkillXpResult ProgressionSystem::AwardSkillXp(SkillProgress& progress) const
{
    SkillXpResult result;
    result.xpGained = Balance::SkillXpPerUse;
    result.oldXp = progress.xp;
    result.oldLevel = progress.level;

    progress.xp += Balance::SkillXpPerUse;
    while (progress.xp >= Balance::SkillXpPerLevel)
    {
        progress.xp -= Balance::SkillXpPerLevel;
        ++progress.level;
        result.leveledUp = true;
    }

    result.newXp = progress.xp;
    result.newLevel = progress.level;
    return result;
}
