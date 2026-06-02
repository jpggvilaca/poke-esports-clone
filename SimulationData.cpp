#include "SimulationData.h"

#include <stdexcept>

SimulationData::SimulationData()
{
    // SAMPLE DATA ONLY:
    // Edit or replace these generic League skills after the systems feel right.
    // Each spec has: universal basic, three styled starters, Balanced advanced.
    skills_ = {
        { "top-basic", "Top Basic", std::nullopt, 0, 16, 0.95 },
        { "top-aggressive", "Top Aggressive", Style::Aggressive, 10, 28, 0.85 },
        { "top-defensive", "Top Defensive", Style::Defensive, 10, 28, 0.85 },
        { "top-balanced", "Top Balanced", Style::Balanced, 10, 28, 0.85 },
        { "top-advanced", "Top Advanced", Style::Balanced, 18, 42, 0.78 },

        { "jungle-basic", "Jungle Basic", std::nullopt, 0, 16, 0.95 },
        { "jungle-aggressive", "Jungle Aggressive", Style::Aggressive, 10, 28, 0.85 },
        { "jungle-defensive", "Jungle Defensive", Style::Defensive, 10, 28, 0.85 },
        { "jungle-balanced", "Jungle Balanced", Style::Balanced, 10, 28, 0.85 },
        { "jungle-advanced", "Jungle Advanced", Style::Balanced, 18, 42, 0.78 },

        { "mid-basic", "Mid Basic", std::nullopt, 0, 16, 0.95 },
        { "mid-aggressive", "Mid Aggressive", Style::Aggressive, 10, 28, 0.85 },
        { "mid-defensive", "Mid Defensive", Style::Defensive, 10, 28, 0.85 },
        { "mid-balanced", "Mid Balanced", Style::Balanced, 10, 28, 0.85 },
        { "mid-advanced", "Mid Advanced", Style::Balanced, 18, 42, 0.78 },

        { "adc-basic", "ADC Basic", std::nullopt, 0, 16, 0.95 },
        { "adc-aggressive", "ADC Aggressive", Style::Aggressive, 10, 28, 0.85 },
        { "adc-defensive", "ADC Defensive", Style::Defensive, 10, 28, 0.85 },
        { "adc-balanced", "ADC Balanced", Style::Balanced, 10, 28, 0.85 },
        { "adc-advanced", "ADC Advanced", Style::Balanced, 18, 42, 0.78 },

        { "support-basic", "Support Basic", std::nullopt, 0, 16, 0.95 },
        { "support-aggressive", "Support Aggressive", Style::Aggressive, 10, 28, 0.85 },
        { "support-defensive", "Support Defensive", Style::Defensive, 10, 28, 0.85 },
        { "support-balanced", "Support Balanced", Style::Balanced, 10, 28, 0.85 },
        { "support-advanced", "Support Advanced", Style::Balanced, 18, 42, 0.78 }
    };

    // SAMPLE DATA ONLY:
    // Edit counteredSpec values to change the matchup cycle.
    // Current rule: Top > Jungle > Mid > ADC > Support > Top.
    specs_ = {
        { Spec::Top, "Top", { "top-basic", "top-aggressive", "top-defensive", "top-balanced", "top-advanced" }, Spec::Jungle },
        { Spec::Jungle, "Jungle", { "jungle-basic", "jungle-aggressive", "jungle-defensive", "jungle-balanced", "jungle-advanced" }, Spec::Mid },
        { Spec::Mid, "Mid", { "mid-basic", "mid-aggressive", "mid-defensive", "mid-balanced", "mid-advanced" }, Spec::Adc },
        { Spec::Adc, "ADC", { "adc-basic", "adc-aggressive", "adc-defensive", "adc-balanced", "adc-advanced" }, Spec::Support },
        { Spec::Support, "Support", { "support-basic", "support-aggressive", "support-defensive", "support-balanced", "support-advanced" }, Spec::Top }
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

