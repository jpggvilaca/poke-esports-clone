#pragma once

#include "Models.h"

#include <string>
#include <vector>

// Edit these values first when rebalancing combat.
//
// C++ note:
// - namespace groups related names so we can write Balance::StartingMaxHp
//   and avoid colliding with other constants called StartingMaxHp later.
// - constexpr means the value is known at compile time, so the compiler can
//   treat it like a true constant.
// - inline lets this header define the constant safely in every .cpp file that
//   includes it. Without inline, headers with variable definitions can cause
//   duplicate-definition linker errors.
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
