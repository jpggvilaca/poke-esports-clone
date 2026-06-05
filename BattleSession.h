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
    BattleActionResult UsePlayerSkill(const std::string& skillId);
    BattleActionResult ChangePlayerStyle(Style style);
    BattleActionResult SwitchPlayer(int playerIndex);

    BattleState GetState() const;
    std::vector<SkillView> GetAvailablePlayerSkills() const;

private:
    Competitor CreateCompetitor(
        int profileIndex,
        const std::string& name,
        GameType gameType,
        Spec spec,
        Style style,
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
    void ResolveOpponentTurn(BattleActionResult& result);
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
    std::vector<Competitor> playerTeam_;
    std::vector<BattleStatus> playerStatuses_;
    std::vector<int> participatingPlayerIndices_;
    Competitor opponent_;
    BattleStatus opponentStatus_;
};
