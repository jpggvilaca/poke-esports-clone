#pragma once

#include "Models.h"

#include <string>
#include <vector>

class SimulationData;

struct ScoutCandidateDefinition
{
    std::string name;
    Spec spec = Spec::Top;
};

struct ScoutOfferDefinition
{
    std::string id;
    int requiredRating = 0;
    std::string message;
    std::vector<ScoutCandidateDefinition> candidates;
};

struct ScoutOfferView
{
    bool available = false;
    std::string id;
    int requiredRating = 0;
    std::string message;
    std::vector<PlayerProfileState> candidates;
};

class ScoutSystem
{
public:
    explicit ScoutSystem(const SimulationData& data);

    ScoutOfferView GetNextOffer(
        int rating,
        const std::vector<std::string>& completedOfferIds,
        const std::vector<std::string>& declinedOfferIds) const;
    ProfileCommandResult CanRecruitCandidate(
        int rosterSize,
        int maxRosterSize,
        int candidateIndex,
        const ScoutOfferView& offer) const;

private:
    bool Contains(const std::vector<std::string>& values, const std::string& value) const;

    const SimulationData& data_;
    std::vector<ScoutOfferDefinition> offers_;
};
