#pragma once

#include "Models.h"

class ProgressionSystem
{
public:
    SkillXpResult AwardSkillXp(SkillProgress& progress, int maxAward = -1) const;
};
