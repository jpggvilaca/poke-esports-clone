#pragma once

#include "BattleRules.h"
#include "Models.h"
#include "OpponentAI.h"
#include "ProgressionSystem.h"
#include "SkillSystem.h"

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

    BattleActionResult StartBattle(const BattleSetup& setup);
    BattleActionResult UsePlayerSkill(const std::string& skillId);
    BattleActionResult ChangePlayerStyle(Style style);

    BattleState GetState() const;
    std::vector<SkillView> GetAvailablePlayerSkills() const;

private:
    Competitor CreateCompetitor(const std::string& name, GameType gameType, Spec spec, Style style) const;
    BattleActionResult RejectAction(const std::string& error) const;
    void ResolveOpponentTurn(std::vector<BattleEvent>& events);
    CompetitorView CreateCompetitorView(const Competitor& competitor, const BattleStatus& status) const;
    void FinishBattleIfNeeded(std::vector<BattleEvent>& events);

    const SimulationData& data_;
    std::mt19937 randomEngine_;
    BattleRules rules_;
    ProgressionSystem progression_;
    SkillSystem skills_;
    OpponentAI opponentAI_;
    bool started_ = false;
    bool finished_ = false;
    BattleWinner winner_ = BattleWinner::None;
    Competitor player_;
    Competitor opponent_;
    BattleStatus playerStatus_;
    BattleStatus opponentStatus_;
};
