#pragma once

#include "Models.h"

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
    void UseSkill(
        BattleActor actor,
        Competitor& attacker,
        BattleStatus& attackerStatus,
        Competitor& defender,
        BattleStatus& defenderStatus,
        SkillProgress& skill,
        std::vector<BattleEvent>& events);
    SkillProgress* SelectOpponentSkill();
    SkillView CreateSkillView(const Skill& definition, const SkillProgress& progress) const;
    CompetitorView CreateCompetitorView(const Competitor& competitor, const BattleStatus& status) const;
    void FinishBattleIfNeeded(std::vector<BattleEvent>& events);

    int GetPower(const Skill& definition, const SkillProgress& progress) const;
    int GetFocusCost(const Skill& definition, const SkillProgress& progress) const;
    double GetAccuracy(const Skill& definition, const SkillProgress& progress) const;
    int GetEffectValue(const Skill& definition, const SkillProgress& progress) const;
    int CalculateDamage(
        BattleActor actor,
        const Skill& definition,
        const SkillProgress& progress,
        const Competitor& attacker,
        BattleStatus& attackerStatus,
        const Competitor& defender,
        BattleStatus& defenderStatus,
        std::vector<BattleEvent>& events) const;
    void ApplySecondaryEffect(
        BattleActor actor,
        BattleActor target,
        const Skill& definition,
        const SkillProgress& progress,
        Competitor& attacker,
        BattleStatus& attackerStatus,
        BattleStatus& defenderStatus,
        std::vector<BattleEvent>& events) const;
    void AwardSkillXp(
        BattleActor actor,
        const Skill& definition,
        SkillProgress& progress,
        std::vector<BattleEvent>& events) const;
    bool IsAvailableForStyle(const Skill& definition, Style style) const;
    bool IsUsefulOpponentSkill(const Skill& definition) const;
    double GetSpecModifier(Spec attackerSpec, Spec defenderSpec) const;
    bool Chance(double probability);

    const SimulationData& data_;
    std::mt19937 randomEngine_;
    bool started_ = false;
    bool finished_ = false;
    BattleWinner winner_ = BattleWinner::None;
    Competitor player_;
    Competitor opponent_;
    BattleStatus playerStatus_;
    BattleStatus opponentStatus_;
};
