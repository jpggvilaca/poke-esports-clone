#pragma once

#include "BattleEconomySystem.h"
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
    BattleActionResult UsePlayerFarm();
    BattleActionResult PassPlayerTurn();
    BattleActionResult SwitchPlayer(int playerIndex);

    BattleState GetState() const;
    std::vector<SkillView> GetAvailablePlayerSkills() const;
    DrillView GetPlayerDrill() const;

private:
    struct SkillActionValidation
    {
        bool canUse = true;
        bool consumesTurn = false;
        SimulationError errorCode = SimulationError::None;
        std::string message;
        std::string disabledReason;
    };

    struct BattleSetupValidation
    {
        bool valid = true;
        SimulationError errorCode = SimulationError::None;
        std::string message;
    };

    // Battle setup
    BattleSetupValidation ValidateBattleSetup(const BattleSetup& setup) const;
    void ResetBattleState();
    void BuildPlayerTeam(const BattleSetup& setup);
    void BuildOpponentTeam(const BattleSetup& setup);
    void SelectStartingOpponent(int requestedOpponentIndex);
    BattleActionResult CreateBattleStartedResult() const;
    Competitor CreateCompetitor(
        int profileIndex,
        const std::string& name,
        GameType gameType,
        Spec spec,
        const PassiveBonuses& bonuses) const;
    Competitor CreateCompetitor(const BattleSetup::PlayerSlot& slot, GameType gameType) const;
    Competitor CreateCompetitor(const BattleSetup::OpponentSlot& slot, GameType gameType) const;

    // Result and event helpers
    BattleActionResult RejectAction(SimulationError errorCode, const std::string& error) const;
    void AppendEvents(BattleActionResult& result, const std::vector<BattleEvent>& events) const;

    // Active combatants
    Competitor& ActivePlayer();
    const Competitor& ActivePlayer() const;
    BattleStatus& ActivePlayerStatus();
    const BattleStatus& ActivePlayerStatus() const;
    Competitor& ActiveOpponent();
    const Competitor& ActiveOpponent() const;
    BattleStatus& ActiveOpponentStatus();
    const BattleStatus& ActiveOpponentStatus() const;
    bool IsKnownPlayerIndex(int playerIndex) const;
    bool IsLivingPlayerIndex(int playerIndex) const;
    bool HasLivingPlayer() const;
    int FirstLivingPlayerIndex() const;
    int NextLivingPlayerIndex(int fromPlayerIndex) const;
    bool IsKnownOpponentIndex(int opponentIndex) const;
    bool IsLivingOpponentIndex(int opponentIndex) const;
    bool HasLivingOpponent() const;
    int FirstLivingOpponentIndex() const;
    int NextLivingOpponentIndex(int fromOpponentIndex) const;

    // Skill actions
    bool IsBasicAbility(const Skill& definition) const;
    SkillActionValidation ValidateSkillAction(
        const Competitor& actor,
        const BattleStatus& status,
        const Skill& definition,
        const SkillProgress& progress,
        int cooldownRemaining) const;
    AbilityRuntimeState& EnsureAbilityState(Competitor& competitor, const std::string& skillId) const;
    int GetCooldownRemaining(const Competitor& competitor, const std::string& skillId) const;
    BattleActionResult RejectSkillAction(
        SimulationError errorCode,
        const std::string& error,
        BattleActor actor,
        const std::string& skillId) const;
    SkillActionTarget ResolvePlayerSkillTarget(const Skill& definition, int targetPlayerIndex);
    SkillActionTarget ResolveOpponentSkillTarget(const Skill& definition);
    SkillUseResult ExecuteSkillAction(
        const SkillUseRequest& request,
        const Skill& definition,
        BattleActionResult& result);

    // Drill actions
    DrillView CreateDrillView(const Competitor& competitor, const BattleStatus& status) const;
    DrillUseResult ResolveDrill(
        BattleActor actor,
        Competitor& competitor,
        DrillResultQuality quality,
        BattleActionResult& result) const;
    int GetDrillManaGain(const DrillDefinition& drill, DrillResultQuality quality) const;
    std::string DrillQualityToString(DrillResultQuality quality) const;

    // Cooldowns and statuses
    void StartCooldown(
        BattleActor actor,
        Competitor& competitor,
        BattleStatus& status,
        const Skill& definition,
        BattleActionResult& result) const;
    void TickCooldowns(BattleActor actor, Competitor& competitor, BattleActionResult& result) const;
    void TickStatusDurations(BattleActor actor, BattleStatus& status, BattleActionResult& result) const;
    void TickActionStatusDurations(BattleActor actor, BattleStatus& status, BattleActionResult& result) const;
    void FinishNonSkillActionOpportunity(
        BattleActor actor,
        Competitor& competitor,
        BattleStatus& status,
        BattleActionResult& result,
        const std::string& blockedActionId = "",
        const std::string& blockedReason = "") const;
    void AppendActionBlocked(
        BattleActionResult& result,
        BattleActor actor,
        const std::string& skillId,
        const std::string& reason) const;

    // Turn flow and battle completion
    void MarkParticipant(int playerIndex);
    void ResolveOpponentTurn(BattleActionResult& result);
    void RegisterPlayerActionAndApplyFarming(BattleActionResult& result);
    void ResolveAfterPlayerAction(BattleActionResult& result);
    void ApplyPlayerFarm(Competitor& player, BattleStatus& status, BattleActionResult& result) const;
    void AdvanceToNextPlayer(BattleActionResult& result);
    void AdvanceToNextOpponent(BattleActionResult& result);
    void ApplyTimedFarmingIfDue(BattleActionResult& result);
    void ApplyLineupEffect(
        const Skill& definition,
        const SkillProgress& progress,
        Competitor& actor,
        BattleActionResult& result);
    void FinishBattleIfNeeded(BattleActionResult& result);
    void AttachBattleRewardIfNeeded(BattleActionResult& result) const;

    // Views
    CompetitorView CreateCompetitorView(const Competitor& competitor, const BattleStatus& status) const;

    const SimulationData& data_;
    std::mt19937 randomEngine_;
    BattleRules rules_;
    ProgressionSystem progression_;
    SkillSystem skills_;
    OpponentAI opponentAI_;
    BattleEconomySystem economy_;
    bool started_ = false;
    bool finished_ = false;
    BattleWinner winner_ = BattleWinner::None;
    int activePlayerIndex_ = 0;
    int playerActionCount_ = 0;
    std::vector<Competitor> playerTeam_;
    std::vector<BattleStatus> playerStatuses_;
    std::vector<Competitor> opponentTeam_;
    std::vector<BattleStatus> opponentStatuses_;
    std::vector<int> participatingPlayerIndices_;
    int activeOpponentIndex_ = 0;
};
