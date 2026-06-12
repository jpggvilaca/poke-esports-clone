#pragma once

#include "Models.h"

#include <string>

struct BattleEventParticipant
{
    BattleActor actor = BattleActor::None;
    int playerIndex = -1;
    int profileIndex = -1;
    std::string name;
};

inline BattleEventParticipant MakeBattleEventParticipant(
    BattleActor actor,
    int playerIndex,
    const Competitor& competitor)
{
    BattleEventParticipant participant;
    participant.actor = actor;
    participant.playerIndex = playerIndex;
    participant.profileIndex = competitor.profileIndex;
    participant.name = competitor.name;
    return participant;
}

inline void SetBattleEventActor(BattleEvent& event, const BattleEventParticipant& actor)
{
    event.actor = actor.actor;
    event.actorPlayerIndex = actor.playerIndex;
    event.profileIndex = actor.profileIndex;
    event.actorName = actor.name;
}

inline void SetBattleEventTarget(BattleEvent& event, const BattleEventParticipant& target)
{
    event.target = target.actor;
    event.targetPlayerIndex = target.playerIndex;
    event.targetProfileIndex = target.profileIndex;
    event.targetName = target.name;
}

inline BattleEvent MakeActorEvent(
    BattleEventType type,
    const BattleEventParticipant& actor,
    const std::string& skillId = "")
{
    BattleEvent event;
    event.type = type;
    SetBattleEventActor(event, actor);
    event.skillId = skillId;
    return event;
}

inline BattleEvent MakeTargetedEvent(
    BattleEventType type,
    const BattleEventParticipant& actor,
    const BattleEventParticipant& target,
    const std::string& skillId = "")
{
    BattleEvent event = MakeActorEvent(type, actor, skillId);
    SetBattleEventTarget(event, target);
    return event;
}

inline BattleEvent MakeBattleEvent(BattleEventType type, BattleActor actor)
{
    BattleEvent event;
    event.type = type;
    event.actor = actor;
    return event;
}
