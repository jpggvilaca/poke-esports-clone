#include "SimulationData.h"

#include <stdexcept>

SimulationData::SimulationData()
{
    // SAMPLE DATA ONLY:
    // Edit or replace these generic skills after the small systems feel right.
    // Each row is: ID, name, style, focus cost, power, accuracy.
    skills_ = {
        { "spec-a-basic", "Spec A Basic", Style::Balanced, 0, 16, 0.95 },
        { "spec-a-skill-1", "Spec A Skill 1", Style::Aggressive, 10, 28, 0.85 },
        { "spec-a-skill-2", "Spec A Skill 2", Style::Defensive, 17, 39, 0.79 },

        { "spec-b-basic", "Spec B Basic", Style::Balanced, 0, 16, 0.95 },
        { "spec-b-skill-1", "Spec B Skill 1", Style::Aggressive, 10, 28, 0.85 },
        { "spec-b-skill-2", "Spec B Skill 2", Style::Defensive, 17, 39, 0.79 },

        { "spec-c-basic", "Spec C Basic", Style::Balanced, 0, 16, 0.95 },
        { "spec-c-skill-1", "Spec C Skill 1", Style::Aggressive, 10, 28, 0.85 },
        { "spec-c-skill-2", "Spec C Skill 2", Style::Defensive, 17, 39, 0.79 }
    };

    // SAMPLE DATA ONLY:
    // Edit this list to change which skills belong to each spec.
    // The first skill must remain a zero-focus action.
    specs_ = {
        { Spec::SpecA, "Spec A", { "spec-a-basic", "spec-a-skill-1", "spec-a-skill-2" } },
        { Spec::SpecB, "Spec B", { "spec-b-basic", "spec-b-skill-1", "spec-b-skill-2" } },
        { Spec::SpecC, "Spec C", { "spec-c-basic", "spec-c-skill-1", "spec-c-skill-2" } }
    };

    // SAMPLE STORE DATA ONLY:
    // Edit this list to change what the temporary sandbox store sells.
    // Each row is: item ID, store label, taught skill ID, price.
    storeItems_ = {
        { "manual-spec-a-2", "Spec A Skill 2 Manual", "spec-a-skill-2", 150 },
        { "manual-spec-b-2", "Spec B Skill 2 Manual", "spec-b-skill-2", 150 },
        { "manual-spec-c-2", "Spec C Skill 2 Manual", "spec-c-skill-2", 150 }
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

const std::vector<SpecData>& SimulationData::GetSpecs() const
{
    return specs_;
}

const std::vector<StoreItem>& SimulationData::GetStoreItems() const
{
    return storeItems_;
}

