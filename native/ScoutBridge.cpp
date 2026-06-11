#include "ScoutBridge.h"

#include "BridgeSerializers.h"
#include "Models.h"
#include "ScoutSystem.h"
#include "TrainerProfile.h"

#include <godot_cpp/core/class_db.hpp>

using godot::Array;
using godot::Dictionary;
using godot::String;

namespace
{
String ErrorToString(SimulationError error)
{
    switch (error)
    {
    case SimulationError::None: return "none";
    case SimulationError::UnknownPlayerProfile: return "unknown_player_profile";
    case SimulationError::RosterFull: return "roster_full";
    default: return "unknown";
    }
}
}

void ScoutBridge::_bind_methods()
{
    godot::ClassDB::bind_method(godot::D_METHOD("get_pending_scout_offer", "rating", "completed_offer_ids", "declined_offer_ids"), &ScoutBridge::get_pending_scout_offer);
    godot::ClassDB::bind_method(godot::D_METHOD("accept_scout_candidate", "roster", "pending_offer", "candidate_index"), &ScoutBridge::accept_scout_candidate);
}

Dictionary ScoutBridge::get_pending_scout_offer(
    int rating,
    const Array& completed_offer_ids,
    const Array& declined_offer_ids) const
{
    ScoutSystem scouts(data_);
    return bridge_serializers::ScoutOfferToDictionary(scouts.GetNextOffer(
        rating,
        bridge_serializers::StringVectorFromArray(completed_offer_ids),
        bridge_serializers::StringVectorFromArray(declined_offer_ids)), data_);
}

Dictionary ScoutBridge::accept_scout_candidate(
    const Array& roster,
    const Dictionary& pending_offer,
    int candidate_index) const
{
    ScoutSystem scouts(data_);
    const ScoutOfferView offer = bridge_serializers::ScoutOfferFromDictionary(pending_offer, data_);
    const ProfileCommandResult result = scouts.CanRecruitCandidate(
        roster.size(),
        TrainerBalance::MaxPlayerProfiles,
        candidate_index,
        offer);

    Dictionary response;
    response["accepted"] = result.accepted;
    response["error_code"] = ErrorToString(result.errorCode);
    response["error"] = String(result.error.c_str());
    response["offer_id"] = pending_offer.get("id", "");

    Array updated_roster;
    for (int index = 0; index < roster.size(); ++index)
    {
        updated_roster.push_back(roster[index]);
    }

    if (!result.accepted)
    {
        response["roster"] = updated_roster;
        response["message"] = String(result.error.c_str());
        return response;
    }

    Array candidates = pending_offer.has("candidates")
        ? Array(pending_offer["candidates"])
        : Array();
    Dictionary candidate = candidates[candidate_index];
    updated_roster.push_back(candidate);
    response["roster"] = updated_roster;
    response["message"] = String(candidate.get("name", "Prospect")) + String(" joined your roster.");
    return response;
}
