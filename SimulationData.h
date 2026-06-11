#pragma once

#include "Models.h"

#include <cstddef>
#include <string>
#include <unordered_map>
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
    inline constexpr int StartingMaxMana = 100;
    inline constexpr int StartingMana = 25;
    inline constexpr int BasicManaGain = 25;
    inline constexpr int StartingBasePower = 5;
    inline constexpr int StarterSkillsPerSpec = 4;
    inline constexpr int SkillXpPerUse = 25;
    inline constexpr int SkillXpPerLevel = 100;
    inline constexpr int SkillLevelUpPower = 4;
    inline constexpr int SkillLevelUpEffectValue = 4;
    inline constexpr int ClutchLowHpThresholdPercent = 35;
    inline constexpr double AdvantageModifier = 1.05;
    inline constexpr double DisadvantageModifier = 0.95;
    inline constexpr double NeutralModifier = 1.0;
}

class SimulationData
{
public:
    SimulationData();

    const Skill* FindSkill(const std::string& id) const;
    const DrillDefinition* FindDrill(GameType gameType) const;
    const TraitDefinition* FindTrait(const std::string& id) const;
    const SpecData* FindSpec(Spec spec) const;
    const std::vector<SpecData>& GetSpecs() const;
    bool LoadedExternalSkills() const;
    bool LoadedExternalTraits() const;
    bool LoadedExternalSpecs() const;
    bool LoadedExternalDrills() const;

private:
    void BuildIndexes();
    void LoadExternalDataIfAvailable();
    void LoadSkillsFromCsv(const std::string& path);
    void LoadTraitsFromCsv(const std::string& path);
    void LoadSpecsFromCsv(const std::string& path);
    void LoadDrillsFromCsv(const std::string& path);
    void ValidateReferences() const;

    std::vector<Skill> skills_;
    std::vector<DrillDefinition> drills_;
    std::vector<TraitDefinition> traits_;
    std::vector<SpecData> specs_;
    bool loadedExternalSkills_ = false;
    bool loadedExternalTraits_ = false;
    bool loadedExternalSpecs_ = false;
    bool loadedExternalDrills_ = false;
    std::unordered_map<std::string, std::size_t> skillIndexById_;
    std::unordered_map<int, std::size_t> drillIndexByGameType_;
    std::unordered_map<std::string, std::size_t> traitIndexById_;
    std::unordered_map<int, std::size_t> specIndexBySpec_;
};
