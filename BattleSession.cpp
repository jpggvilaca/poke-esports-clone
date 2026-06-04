#include "BattleSession.h"

#include "SimulationData.h"

namespace
{
    bool IsKnown(GameType gameType)
    {
        return gameType == GameType::LeagueOfLegends;
    }

    bool IsKnown(Spec spec)
    {
        return spec == Spec::Top
            || spec == Spec::Jungle
            || spec == Spec::Mid
            || spec == Spec::Adc
            || spec == Spec::Support;
    }

    bool IsKnown(Style style)
    {
        return style == Style::Aggressive
            || style == Style::Defensive
            || style == Style::Balanced;
    }
}

BattleSession::BattleSession(const SimulationData& data)
    : data_(data),
      randomEngine_(std::random_device{}()),
      rules_(data),
      skills_(data, rules_, progression_),
      opponentAI_(data, rules_, skills_)
{
}

BattleActionResult BattleSession::StartBattle(const BattleSetup& setup)
{
    if (!IsKnown(setup.gameType))
    {
        return RejectAction("Unknown game type.");
    }

    if (!IsKnown(setup.playerSpec)
        || !IsKnown(setup.opponentSpec)
        || data_.FindSpec(setup.playerSpec) == nullptr
        || data_.FindSpec(setup.opponentSpec) == nullptr)
    {
        return RejectAction("Unknown spec.");
    }

    if (!IsKnown(setup.playerStyle) || !IsKnown(setup.opponentStyle))
    {
        return RejectAction("Unknown style.");
    }

    player_ = CreateCompetitor("Player", setup.gameType, setup.playerSpec, setup.playerStyle);
    opponent_ = CreateCompetitor("Opponent", setup.gameType, setup.opponentSpec, setup.opponentStyle);
    playerStatus_ = {};
    opponentStatus_ = {};
    started_ = true;
    finished_ = false;
    winner_ = BattleWinner::None;

    BattleActionResult result;
    result.accepted = true;
    result.battleStarted = true;
    return result;
}

BattleActionResult BattleSession::UsePlayerSkill(const std::string& skillId)
{
    if (!started_)
    {
        return RejectAction("Start a battle first.");
    }

    if (finished_)
    {
        return RejectAction("The battle is already finished.");
    }

    SkillProgress* progress = player_.FindSkill(skillId);
    const Skill* definition = data_.FindSkill(skillId);
    if (progress == nullptr || definition == nullptr)
    {
        return RejectAction("Unknown skill.");
    }

    if (!skills_.IsAvailableForStyle(*definition, player_.style))
    {
        return RejectAction("That skill is not available for the active style.");
    }

    if (rules_.GetFocusCost(*definition, *progress) > player_.focus)
    {
        return RejectAction("Not enough focus.");
    }

    BattleActionResult result;
    result.accepted = true;
    result.skillUses.push_back(skills_.UseSkill(
        BattleActor::Player,
        player_,
        playerStatus_,
        opponent_,
        opponentStatus_,
        *progress,
        randomEngine_));
    FinishBattleIfNeeded(result);

    if (!finished_)
    {
        ResolveOpponentTurn(result);
        FinishBattleIfNeeded(result);
    }

    return result;
}

BattleActionResult BattleSession::ChangePlayerStyle(Style style)
{
    if (!started_)
    {
        return RejectAction("Start a battle first.");
    }

    if (finished_)
    {
        return RejectAction("The battle is already finished.");
    }

    if (!IsKnown(style))
    {
        return RejectAction("Unknown style.");
    }

    if (style == player_.style)
    {
        return RejectAction("That style is already active.");
    }

    player_.style = style;

    BattleActionResult result;
    result.accepted = true;
    result.styleChanged = true;
    result.newStyle = style;

    // Switching style is a tactical choice, not a free menu operation.
    ResolveOpponentTurn(result);
    FinishBattleIfNeeded(result);
    return result;
}

BattleState BattleSession::GetState() const
{
    BattleState state;
    state.started = started_;
    state.finished = finished_;
    state.winner = winner_;
    state.player = CreateCompetitorView(player_, playerStatus_);
    state.opponent = CreateCompetitorView(opponent_, opponentStatus_);
    return state;
}

std::vector<SkillView> BattleSession::GetAvailablePlayerSkills() const
{
    std::vector<SkillView> skills;
    if (!started_ || finished_)
    {
        return skills;
    }

    for (const SkillProgress& progress : player_.skills)
    {
        const Skill* definition = data_.FindSkill(progress.skillId);
        if (definition != nullptr && skills_.IsAvailableForStyle(*definition, player_.style))
        {
            skills.push_back(skills_.CreateSkillView(*definition, progress));
        }
    }

    return skills;
}

Competitor BattleSession::CreateCompetitor(
    const std::string& name,
    GameType gameType,
    Spec spec,
    Style style) const
{
    Competitor competitor;
    competitor.name = name;
    competitor.gameType = gameType;
    competitor.spec = spec;
    competitor.style = style;
    competitor.hp = Balance::StartingMaxHp;
    competitor.maxHp = Balance::StartingMaxHp;
    competitor.focus = Balance::StartingMaxFocus;
    competitor.maxFocus = Balance::StartingMaxFocus;
    competitor.basePower = Balance::StartingBasePower;

    const SpecData* specData = data_.FindSpec(spec);
    if (specData == nullptr)
    {
        return competitor;
    }

    // Edit Balance::StarterSkillsPerSpec when changing the starting loadout.
    for (int index = 0;
        index < Balance::StarterSkillsPerSpec && index < static_cast<int>(specData->skillIds.size());
        ++index)
    {
        competitor.skills.push_back({ specData->skillIds[index] });
    }

    return competitor;
}

BattleActionResult BattleSession::RejectAction(const std::string& error) const
{
    BattleActionResult result;
    result.error = error;
    return result;
}

void BattleSession::ResolveOpponentTurn(BattleActionResult& result)
{
    SkillProgress* progress = opponentAI_.SelectSkill(
        opponent_,
        opponentStatus_,
        player_,
        playerStatus_,
        randomEngine_);
    if (progress == nullptr)
    {
        return;
    }

    result.skillUses.push_back(skills_.UseSkill(
        BattleActor::Opponent,
        opponent_,
        opponentStatus_,
        player_,
        playerStatus_,
        *progress,
        randomEngine_));
}

CompetitorView BattleSession::CreateCompetitorView(
    const Competitor& competitor,
    const BattleStatus& status) const
{
    CompetitorView view;
    view.name = competitor.name;
    view.spec = competitor.spec;
    view.style = competitor.style;
    view.hp = competitor.hp;
    view.maxHp = competitor.maxHp;
    view.focus = competitor.focus;
    view.maxFocus = competitor.maxFocus;
    view.basePower = competitor.basePower;
    view.status = status;
    return view;
}

void BattleSession::FinishBattleIfNeeded(BattleActionResult& result)
{
    if (finished_ || (player_.hp > 0 && opponent_.hp > 0))
    {
        return;
    }

    finished_ = true;
    winner_ = player_.hp > 0 ? BattleWinner::Player : BattleWinner::Opponent;
    result.battleFinished = true;
    result.winner = winner_;
}
