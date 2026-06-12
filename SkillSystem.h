#pragma once

#include "BattleRules.h"
#include "Models.h"
#include "ProgressionSystem.h"

#include <random>
#include <vector>

class SimulationData;

struct SkillActionTarget
{
    BattleActor actor = BattleActor::None;
    Competitor* competitor = nullptr;
    BattleStatus* status = nullptr;
    int playerIndex = -1;
};

struct SkillUseRequest
{
    BattleActor actor = BattleActor::None;
    Competitor* competitor = nullptr;
    BattleStatus* status = nullptr;
    SkillProgress* progress = nullptr;
    SkillActionTarget target;
    int playerIndex = -1;
    bool boosted = false;
};

class SkillSystem
{
public:
    SkillSystem(
        const SimulationData& data,
        const BattleRules& rules,
        const ProgressionSystem& progression);

    SkillView CreateSkillView(
        const Skill& definition,
        const SkillProgress& progress,
        const Competitor& user,
        const BattleStatus& userStatus,
        int cooldownRemaining) const;
    SkillUseResult UseSkill(const SkillUseRequest& request, std::mt19937& randomEngine) const;

private:
    BattleActor Opposite(BattleActor actor) const;
    bool Chance(double probability, std::mt19937& randomEngine) const;
    void ResolveSkillCost(
        const SkillUseRequest& request,
        const Skill& definition,
        SkillUseResult& result) const;
    void EmitSkillStarted(
        const SkillUseRequest& request,
        const Skill& definition,
        SkillUseResult& result) const;
    bool ResolveHitRoll(
        const SkillUseRequest& request,
        const Skill& definition,
        std::mt19937& randomEngine,
        SkillUseResult& result) const;
    void ResolveDamageStage(
        const SkillUseRequest& request,
        const Skill& definition,
        SkillUseResult& result) const;
    std::vector<SkillEffectDefinition> BuildEffectList(const Skill& definition) const;
    void ResolveSecondaryEffectsStage(
        const SkillUseRequest& request,
        const Skill& definition,
        SkillUseResult& result) const;
    void AwardSkillXpStage(
        const SkillUseRequest& request,
        const Skill& definition,
        SkillUseResult& result) const;
    SecondaryEffectResult ApplySecondaryEffect(
        BattleActor actor,
        BattleActor targetActor,
        const Skill& definition,
        const SkillEffectDefinition& effect,
        const SkillProgress& progress,
        Competitor& attacker,
        BattleStatus& attackerStatus,
        Competitor& targetCompetitor,
        BattleStatus& targetBattleStatus,
        int actorPlayerIndex,
        int targetPlayerIndex) const;

    const SimulationData& data_;
    const BattleRules& rules_;
    const ProgressionSystem& progression_;
};
