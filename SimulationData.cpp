#include "SimulationData.h"

#include <stdexcept>

SimulationData::SimulationData()
{
    // SAMPLE DATA ONLY:
    // Edit or replace these generic League skills after the systems feel right.
    // Each spec has one universal basic, then three skills for each style.
    //
    // Row format:
    // ID, name, style, focus, power, accuracy, optional effect, target,
    // effect value, effect hits.
    //
    // Style identities:
    // Aggressive = damage and risk.
    // Defensive = healing and protection.
    // Balanced = reliable damage, disruption, and setup.
    skills_ = {
        { "top-basic", "Top Basic", std::nullopt, 0, 16, 0.95 },
        { "top-pressure", "Top Pressure", Style::Aggressive, 8, 26, 0.95 },
        { "top-all-in", "Top All-In", Style::Aggressive, 15, 36, 0.82 },
        { "top-reckless", "Top Reckless Play", Style::Aggressive, 18, 46, 0.72, SkillEffectType::DefenseModifier, SkillEffectTarget::Self, -25, 1 },
        { "top-recover", "Top Recover", Style::Defensive, 14, 0, 1.00, SkillEffectType::Heal, SkillEffectTarget::Self, 28, 0 },
        { "top-guard", "Top Guard", Style::Defensive, 7, 0, 1.00, SkillEffectType::DefenseModifier, SkillEffectTarget::Self, 45, 1 },
        { "top-fortify", "Top Fortify", Style::Defensive, 12, 0, 1.00, SkillEffectType::DefenseModifier, SkillEffectTarget::Self, 20, 3 },
        { "top-consistent", "Top Consistent Play", Style::Balanced, 7, 24, 0.98 },
        { "top-disrupt", "Top Disrupt", Style::Balanced, 10, 18, 0.90, SkillEffectType::AttackModifier, SkillEffectTarget::Opponent, -20, 2 },
        { "top-setup", "Top Setup", Style::Balanced, 8, 0, 1.00, SkillEffectType::AttackModifier, SkillEffectTarget::Self, 40, 1 },
        { "top-advanced", "Top Advanced", Style::Balanced, 18, 42, 0.78 },

        { "jungle-basic", "Jungle Basic", std::nullopt, 0, 16, 0.95 },
        { "jungle-pressure", "Jungle Pressure", Style::Aggressive, 8, 26, 0.95 },
        { "jungle-all-in", "Jungle All-In", Style::Aggressive, 15, 36, 0.82 },
        { "jungle-reckless", "Jungle Reckless Play", Style::Aggressive, 18, 46, 0.72, SkillEffectType::DefenseModifier, SkillEffectTarget::Self, -25, 1 },
        { "jungle-recover", "Jungle Recover", Style::Defensive, 14, 0, 1.00, SkillEffectType::Heal, SkillEffectTarget::Self, 28, 0 },
        { "jungle-guard", "Jungle Guard", Style::Defensive, 7, 0, 1.00, SkillEffectType::DefenseModifier, SkillEffectTarget::Self, 45, 1 },
        { "jungle-fortify", "Jungle Fortify", Style::Defensive, 12, 0, 1.00, SkillEffectType::DefenseModifier, SkillEffectTarget::Self, 20, 3 },
        { "jungle-consistent", "Jungle Consistent Play", Style::Balanced, 7, 24, 0.98 },
        { "jungle-disrupt", "Jungle Disrupt", Style::Balanced, 10, 18, 0.90, SkillEffectType::AttackModifier, SkillEffectTarget::Opponent, -20, 2 },
        { "jungle-setup", "Jungle Setup", Style::Balanced, 8, 0, 1.00, SkillEffectType::AttackModifier, SkillEffectTarget::Self, 40, 1 },
        { "jungle-advanced", "Jungle Advanced", Style::Balanced, 18, 42, 0.78 },

        { "mid-basic", "Mid Basic", std::nullopt, 0, 16, 0.95 },
        { "mid-pressure", "Mid Pressure", Style::Aggressive, 8, 26, 0.95 },
        { "mid-all-in", "Mid All-In", Style::Aggressive, 15, 36, 0.82 },
        { "mid-reckless", "Mid Reckless Play", Style::Aggressive, 18, 46, 0.72, SkillEffectType::DefenseModifier, SkillEffectTarget::Self, -25, 1 },
        { "mid-recover", "Mid Recover", Style::Defensive, 14, 0, 1.00, SkillEffectType::Heal, SkillEffectTarget::Self, 28, 0 },
        { "mid-guard", "Mid Guard", Style::Defensive, 7, 0, 1.00, SkillEffectType::DefenseModifier, SkillEffectTarget::Self, 45, 1 },
        { "mid-fortify", "Mid Fortify", Style::Defensive, 12, 0, 1.00, SkillEffectType::DefenseModifier, SkillEffectTarget::Self, 20, 3 },
        { "mid-consistent", "Mid Consistent Play", Style::Balanced, 7, 24, 0.98 },
        { "mid-disrupt", "Mid Disrupt", Style::Balanced, 10, 18, 0.90, SkillEffectType::AttackModifier, SkillEffectTarget::Opponent, -20, 2 },
        { "mid-setup", "Mid Setup", Style::Balanced, 8, 0, 1.00, SkillEffectType::AttackModifier, SkillEffectTarget::Self, 40, 1 },
        { "mid-advanced", "Mid Advanced", Style::Balanced, 18, 42, 0.78 },

        { "adc-basic", "ADC Basic", std::nullopt, 0, 16, 0.95 },
        { "adc-pressure", "ADC Pressure", Style::Aggressive, 8, 26, 0.95 },
        { "adc-all-in", "ADC All-In", Style::Aggressive, 15, 36, 0.82 },
        { "adc-reckless", "ADC Reckless Play", Style::Aggressive, 18, 46, 0.72, SkillEffectType::DefenseModifier, SkillEffectTarget::Self, -25, 1 },
        { "adc-recover", "ADC Recover", Style::Defensive, 14, 0, 1.00, SkillEffectType::Heal, SkillEffectTarget::Self, 28, 0 },
        { "adc-guard", "ADC Guard", Style::Defensive, 7, 0, 1.00, SkillEffectType::DefenseModifier, SkillEffectTarget::Self, 45, 1 },
        { "adc-fortify", "ADC Fortify", Style::Defensive, 12, 0, 1.00, SkillEffectType::DefenseModifier, SkillEffectTarget::Self, 20, 3 },
        { "adc-consistent", "ADC Consistent Play", Style::Balanced, 7, 24, 0.98 },
        { "adc-disrupt", "ADC Disrupt", Style::Balanced, 10, 18, 0.90, SkillEffectType::AttackModifier, SkillEffectTarget::Opponent, -20, 2 },
        { "adc-setup", "ADC Setup", Style::Balanced, 8, 0, 1.00, SkillEffectType::AttackModifier, SkillEffectTarget::Self, 40, 1 },
        { "adc-advanced", "ADC Advanced", Style::Balanced, 18, 42, 0.78 },

        { "support-basic", "Support Basic", std::nullopt, 0, 16, 0.95 },
        { "support-pressure", "Support Pressure", Style::Aggressive, 8, 26, 0.95 },
        { "support-all-in", "Support All-In", Style::Aggressive, 15, 36, 0.82 },
        { "support-reckless", "Support Reckless Play", Style::Aggressive, 18, 46, 0.72, SkillEffectType::DefenseModifier, SkillEffectTarget::Self, -25, 1 },
        { "support-recover", "Support Recover", Style::Defensive, 14, 0, 1.00, SkillEffectType::Heal, SkillEffectTarget::Self, 28, 0 },
        { "support-guard", "Support Guard", Style::Defensive, 7, 0, 1.00, SkillEffectType::DefenseModifier, SkillEffectTarget::Self, 45, 1 },
        { "support-fortify", "Support Fortify", Style::Defensive, 12, 0, 1.00, SkillEffectType::DefenseModifier, SkillEffectTarget::Self, 20, 3 },
        { "support-consistent", "Support Consistent Play", Style::Balanced, 7, 24, 0.98 },
        { "support-disrupt", "Support Disrupt", Style::Balanced, 10, 18, 0.90, SkillEffectType::AttackModifier, SkillEffectTarget::Opponent, -20, 2 },
        { "support-setup", "Support Setup", Style::Balanced, 8, 0, 1.00, SkillEffectType::AttackModifier, SkillEffectTarget::Self, 40, 1 },
        { "support-advanced", "Support Advanced", Style::Balanced, 18, 42, 0.78 }
    };

    // SAMPLE DATA ONLY:
    // Edit counteredSpec values to change the matchup cycle.
    // Current rule: Top > Jungle > Mid > ADC > Support > Top.
    specs_ = {
        { Spec::Top, "Top", { "top-basic", "top-pressure", "top-all-in", "top-reckless", "top-recover", "top-guard", "top-fortify", "top-consistent", "top-disrupt", "top-setup", "top-advanced" }, Spec::Jungle },
        { Spec::Jungle, "Jungle", { "jungle-basic", "jungle-pressure", "jungle-all-in", "jungle-reckless", "jungle-recover", "jungle-guard", "jungle-fortify", "jungle-consistent", "jungle-disrupt", "jungle-setup", "jungle-advanced" }, Spec::Mid },
        { Spec::Mid, "Mid", { "mid-basic", "mid-pressure", "mid-all-in", "mid-reckless", "mid-recover", "mid-guard", "mid-fortify", "mid-consistent", "mid-disrupt", "mid-setup", "mid-advanced" }, Spec::Adc },
        { Spec::Adc, "ADC", { "adc-basic", "adc-pressure", "adc-all-in", "adc-reckless", "adc-recover", "adc-guard", "adc-fortify", "adc-consistent", "adc-disrupt", "adc-setup", "adc-advanced" }, Spec::Support },
        { Spec::Support, "Support", { "support-basic", "support-pressure", "support-all-in", "support-reckless", "support-recover", "support-guard", "support-fortify", "support-consistent", "support-disrupt", "support-setup", "support-advanced" }, Spec::Top }
    };

    // Add another GameTypeData row when the simulation supports another genre.
    gameTypes_ = {
        { GameType::LeagueOfLegends, "League of Legends", { Spec::Top, Spec::Jungle, Spec::Mid, Spec::Adc, Spec::Support } }
    };

    // SAMPLE STORE DATA ONLY:
    // Edit this list to change what stores can eventually sell. The current
    // sandbox shop deliberately displays and purchases SkillManual rows only.
    storeItems_ = {
        { "manual-top-advanced", "Top Advanced Manual", StoreItemType::SkillManual, "top-advanced", 150 },
        { "manual-jungle-advanced", "Jungle Advanced Manual", StoreItemType::SkillManual, "jungle-advanced", 150 },
        { "manual-mid-advanced", "Mid Advanced Manual", StoreItemType::SkillManual, "mid-advanced", 150 },
        { "manual-adc-advanced", "ADC Advanced Manual", StoreItemType::SkillManual, "adc-advanced", 150 },
        { "manual-support-advanced", "Support Advanced Manual", StoreItemType::SkillManual, "support-advanced", 150 },
        { "small-hp-restore", "Small HP Restore", StoreItemType::RestoreHp, "", 40 },
        { "small-focus-restore", "Small Focus Restore", StoreItemType::RestoreFocus, "", 40 },
        { "small-attack-boost", "Small Attack Boost", StoreItemType::AttackBoost, "", 60 },
        { "small-defense-boost", "Small Defense Boost", StoreItemType::DefenseBoost, "", 60 }
    };

    // DATA FOUNDATION ONLY:
    // Requirements and entry fees are enforced in a later increment.
    tournaments_ = {
        { "sample-tournament", "Sample Tournament", 100, 50 }
    };
}

const Skill* SimulationData::FindSkill(const std::string& id) const
{
    for (const Skill& skill : skills_)
    {
        if (skill.id == id)
        {
            return &skill;
        }
    }

    return nullptr;
}

const SpecData& SimulationData::GetSpec(Spec spec) const
{
    for (const SpecData& data : specs_)
    {
        if (data.spec == spec)
        {
            return data;
        }
    }

    throw std::runtime_error("Missing spec data.");
}

Spec SimulationData::GetCounterSpec(Spec spec) const
{
    // Find the spec whose rule says that it beats the requested spec.
    for (const SpecData& data : specs_)
    {
        if (data.counteredSpec == spec)
        {
            return data.spec;
        }
    }

    throw std::runtime_error("Missing counter spec.");
}

const GameTypeData& SimulationData::GetGameType(GameType gameType) const
{
    for (const GameTypeData& data : gameTypes_)
    {
        if (data.gameType == gameType)
        {
            return data;
        }
    }

    throw std::runtime_error("Missing game type data.");
}

const std::vector<StoreItem>& SimulationData::GetStoreItems() const
{
    return storeItems_;
}

const std::vector<TournamentData>& SimulationData::GetTournaments() const
{
    return tournaments_;
}
