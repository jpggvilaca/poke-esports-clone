#pragma once

#include "BattleRules.h"
#include "Models.h"
#include "OpponentAI.h"
#include "ProgressionSystem.h"
#include "SkillSystem.h"

#include <cstdint>
#include <random>
#include <string>
#include <vector>

class SimulationData;

// BattleSession owns one fight. Callers submit choices and receive structured
// outcomes; they never decide combat order or edit HP directly.
class BattleSession
{
public:
    explicit BattleSession(const SimulationData& data);
    BattleSession(const SimulationData& data, std::uint32_t seed);

    BattleActionResult StartBattle(const BattleSetup& setup);
    BattleActionResult UsePlayerSkill(const std::string& skillId, int targetPlayerIndex = -1);
    BattleActionResult UsePlayerDrill(DrillResultQuality quality);
    BattleActionResult PassPlayerTurn();
    BattleActionResult SwitchPlayer(int playerIndex);

    BattleState GetState() const;
    std::vector<SkillView> GetAvailablePlayerSkills() const;
    DrillView GetPlayerDrill() const;

private:
    Competitor CreateCompetitor(
        int profileIndex,
        const std::string& name,
        GameType gameType,
        Spec spec,
        const PassiveBonuses& bonuses) const;
    Competitor CreateCompetitor(const BattleSetup::PlayerSlot& slot, GameType gameType) const;
    BattleActionResult RejectAction(SimulationError errorCode, const std::string& error) const;
    void AppendEvents(BattleActionResult& result, const std::vector<BattleEvent>& events) const;
    Competitor& ActivePlayer();
    const Competitor& ActivePlayer() const;
    BattleStatus& ActivePlayerStatus();
    const BattleStatus& ActivePlayerStatus() const;
    void MarkParticipant(int playerIndex);
    bool IsKnownPlayerIndex(int playerIndex) const;
    bool IsLivingPlayerIndex(int playerIndex) const;
    bool HasLivingPlayer() const;
    int FirstLivingPlayerIndex() const;
    int NextLivingPlayerIndex(int fromPlayerIndex) const;
    bool IsBasicAbility(const Skill& definition) const;
    AbilityRuntimeState& EnsureAbilityState(Competitor& competitor, const std::string& skillId) const;
    int GetCooldownRemaining(const Competitor& competitor, const std::string& skillId) const;
    BattleActionResult RejectSkillAction(
        SimulationError errorCode,
        const std::string& error,
        BattleActor actor,
        const std::string& skillId) const;
    DrillView CreateDrillView(const Competitor& competitor, const BattleStatus& status) const;
    DrillUseResult ResolveDrill(
        BattleActor actor,
        Competitor& competitor,
        DrillResultQuality quality,
        BattleActionResult& result) const;
    int GetDrillManaGain(const DrillDefinition& drill, DrillResultQuality quality) const;
    std::string DrillQualityToString(DrillResultQuality quality) const;
    void StartCooldown(
        BattleActor actor,
        Competitor& competitor,
        BattleStatus& status,
        const Skill& definition,
        BattleActionResult& result) const;
    void TickCooldowns(BattleActor actor, Competitor& competitor, BattleActionResult& result) const;
    void TickStatusDurations(BattleActor actor, BattleStatus& status, BattleActionResult& result) const;
    void FinishActionOpportunity(BattleActor actor, Competitor& competitor, BattleStatus& status, BattleActionResult& result) const;
    void AppendActionBlocked(
        BattleActionResult& result,
        BattleActor actor,
        const std::string& skillId,
        const std::string& reason) const;
    void ResolveOpponentTurn(BattleActionResult& result);
    void RegisterPlayerAction(BattleActionResult& result);
    void ResolveAfterPlayerAction(BattleActionResult& result);
    void AdvanceToNextPlayer(BattleActionResult& result);
    void ResolveTimedFarming(BattleActionResult& result);
    void ApplyLineupEffect(
        const Skill& definition,
        const SkillProgress& progress,
        Competitor& actor,
        BattleActionResult& result);
    CompetitorView CreateCompetitorView(const Competitor& competitor, const BattleStatus& status) const;
    void FinishBattleIfNeeded(BattleActionResult& result);
    void AttachRewardIfNeeded(BattleActionResult& result) const;

    const SimulationData& data_;
    std::mt19937 randomEngine_;
    BattleRules rules_;
    ProgressionSystem progression_;
    SkillSystem skills_;
    OpponentAI opponentAI_;
    bool started_ = false;
    bool finished_ = false;
    BattleWinner winner_ = BattleWinner::None;
    int activePlayerIndex_ = 0;
    int playerActionCount_ = 0;
    std::vector<Competitor> playerTeam_;
    std::vector<BattleStatus> playerStatuses_;
    std::vector<int> participatingPlayerIndices_;
    Competitor opponent_;
    BattleStatus opponentStatus_;
};
