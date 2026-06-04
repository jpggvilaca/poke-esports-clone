#pragma once

#include "Models.h"

#include <vector>

class ProgressionSystem
{
public:
    void AwardSkillXp(
        BattleActor actor,
        const Skill& definition,
        SkillProgress& progress,
        std::vector<BattleEvent>& events) const;
};
