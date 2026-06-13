#include "ProgressionSystem.h"

#include "SimulationData.h"

#include <algorithm>

SkillXpResult ProgressionSystem::AwardSkillXp(SkillProgress& progress, int maxAward) const
{
    SkillXpResult result;
    const int xpAward = maxAward < 0
        ? Balance::SkillXpPerUse
        : std::min(Balance::SkillXpPerUse, std::max(0, maxAward));
    result.xpGained = xpAward;
    result.oldXp = progress.xp;
    result.oldLevel = progress.level;

    progress.xp += xpAward;
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
