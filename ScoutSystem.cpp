#include "ScoutSystem.h"

#include "CollectionUtils.h"
#include "PlayerProfileSystem.h"
#include "TrainerProfile.h"

ScoutSystem::ScoutSystem(const SimulationData& data)
    : data_(data)
{
    offers_ = {
        {
            "first-scout",
            1050,
            "Scout found three prospects after your first local results.",
            {
                { "Mira", Spec::Support },
                { "Dax", Spec::Adc },
                { "Niko", Spec::Mid },
            },
        },
        {
            "second-scout",
            1125,
            "Scout sent a stronger shortlist from nearby LAN queues.",
            {
                { "Rin", Spec::Jungle },
                { "Sol", Spec::Top },
                { "Vexa", Spec::Adc },
            },
        },
        {
            "third-scout",
            1200,
            "Scout has major-ready prospects watching your team.",
            {
                { "Kade", Spec::Mid },
                { "Luma", Spec::Support },
                { "Bram", Spec::Top },
            },
        },
    };
}

ScoutOfferView ScoutSystem::GetNextOffer(
    int rating,
    const std::vector<std::string>& completedOfferIds,
    const std::vector<std::string>& declinedOfferIds) const
{
    PlayerProfileSystem profiles(data_);
    for (const ScoutOfferDefinition& offer : offers_)
    {
        if (rating < offer.requiredRating
            || ContainsValue(completedOfferIds, offer.id)
            || ContainsValue(declinedOfferIds, offer.id))
        {
            continue;
        }

        ScoutOfferView view;
        view.available = true;
        view.id = offer.id;
        view.requiredRating = offer.requiredRating;
        view.message = offer.message;
        for (const ScoutCandidateDefinition& candidate : offer.candidates)
        {
            view.candidates.push_back(profiles.CreateStarter(candidate.name, candidate.spec));
        }
        return view;
    }

    return {};
}

ProfileCommandResult ScoutSystem::CanRecruitCandidate(
    int rosterSize,
    int maxRosterSize,
    int candidateIndex,
    const ScoutOfferView& offer) const
{
    ProfileCommandResult result;
    if (!offer.available)
    {
        result.errorCode = SimulationError::UnknownPlayerProfile;
        result.error = "No scout offer is pending.";
        return result;
    }

    if (rosterSize >= maxRosterSize)
    {
        result.errorCode = SimulationError::RosterFull;
        result.error = "Roster full. Scout offer remains pending.";
        return result;
    }

    if (candidateIndex < 0 || candidateIndex >= static_cast<int>(offer.candidates.size()))
    {
        result.errorCode = SimulationError::UnknownPlayerProfile;
        result.error = "Unknown scout candidate.";
        return result;
    }

    result.accepted = true;
    return result;
}
