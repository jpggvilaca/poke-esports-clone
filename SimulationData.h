#pragma once

#include "Models.h"

#include <string>
#include <vector>

// Edit these values first when rebalancing combat.
namespace Balance
{
    inline constexpr int StartingMaxHp = 100;
    inline constexpr int StartingMaxFocus = 50;
    inline constexpr int StartingBasePower = 5;
    inline constexpr int StarterSkillsPerSpec = 10;
    inline constexpr int SkillXpPerUse = 25;
    inline constexpr int SkillXpPerLevel = 100;
    inline constexpr int SkillLevelUpPower = 4;
    inline constexpr int SkillLevelUpFocusCost = 2;
    inline constexpr int SkillLevelUpEffectValue = 4;
    inline constexpr double AdvantageModifier = 1.05;
    inline constexpr double DisadvantageModifier = 0.95;
    inline constexpr double NeutralModifier = 1.0;
}

class SimulationData
{
public:
    SimulationData();

    const Skill* FindSkill(const std::string& id) const;
    const SpecData* FindSpec(Spec spec) const;
    const std::vector<SpecData>& GetSpecs() const;

private:
    std::vector<Skill> skills_;
    std::vector<SpecData> specs_;
};
