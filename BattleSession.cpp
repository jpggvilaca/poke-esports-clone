#include "BattleSession.h"

#include "SimulationData.h"

namespace
{
    inline constexpr int PlayerWinXpReward = 120;

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

    if (setup.playerTeam.empty())
    {
        return RejectAction("A battle needs at least one player profile.");
    }

    if (!IsKnown(setup.opponentSpec) || data_.FindSpec(setup.opponentSpec) == nullptr)
    {
        return RejectAction("Unknown spec.");
    }

    for (const BattleSetup::PlayerSlot& slot : setup.playerTeam)
    {
        if (!IsKnown(slot.spec) || data_.FindSpec(slot.spec) == nullptr)
        {
            return RejectAction("Unknown player profile spec.");
        }

        if (!IsKnown(slot.style))
        {
            return RejectAction("Unknown player profile style.");
        }
    }

    if (!IsKnown(setup.opponentStyle))
    {
        return RejectAction("Unknown style.");
    }

    if (setup.activePlayerIndex < 0
        || setup.activePlayerIndex >= static_cast<int>(setup.playerTeam.size()))
    {
        return RejectAction("Unknown active player profile.");
    }

    playerTeam_.clear();
    playerStatuses_.clear();
    participatingPlayerIndices_.clear();
    for (const BattleSetup::PlayerSlot& slot : setup.playerTeam)
    {
        playerTeam_.push_back(CreateCompetitor(slot, setup.gameType));
        playerStatuses_.push_back({});
    }

    activePlayerIndex_ = setup.activePlayerIndex;
    MarkParticipant(activePlayerIndex_);

    opponent_ = CreateCompetitor(
        0,
        "Opponent",
        setup.gameType,
        setup.opponentSpec,
        setup.opponentStyle,
        {});
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

    Competitor& player = ActivePlayer();
    BattleStatus& playerStatus = ActivePlayerStatus();
    SkillProgress* progress = player.FindSkill(skillId);
    const Skill* definition = data_.FindSkill(skillId);
    if (progress == nullptr || definition == nullptr)
    {
        return RejectAction("Unknown skill.");
    }

    if (!skills_.IsAvailableForStyle(*definition, player.style))
    {
        return RejectAction("That skill is not available for the active style.");
    }

    if (rules_.GetFocusCost(*definition, *progress) > player.focus)
    {
        return RejectAction("Not enough focus.");
    }

    BattleActionResult result;
    result.accepted = true;
    result.skillUses.push_back(skills_.UseSkill(
        BattleActor::Player,
        player,
        playerStatus,
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

    if (style == ActivePlayer().style)
    {
        return RejectAction("That style is already active.");
    }

    ActivePlayer().style = style;

    BattleActionResult result;
    result.accepted = true;
    result.styleChanged = true;
    result.newStyle = style;

    // Switching style is a tactical choice, not a free menu operation.
    ResolveOpponentTurn(result);
    FinishBattleIfNeeded(result);
    return result;
}

BattleActionResult BattleSession::SwitchPlayer(int playerIndex)
{
    if (!started_)
    {
        return RejectAction("Start a battle first.");
    }

    if (finished_)
    {
        return RejectAction("The battle is already finished.");
    }

    if (!IsKnownPlayerIndex(playerIndex))
    {
        return RejectAction("Unknown player profile.");
    }

    if (playerIndex == activePlayerIndex_)
    {
        return RejectAction("That player profile is already active.");
    }

    if (playerTeam_[playerIndex].hp <= 0)
    {
        return RejectAction("That player profile cannot play.");
    }

    BattleActionResult result;
    result.accepted = true;
    result.playerSwitched = true;
    result.oldPlayerIndex = activePlayerIndex_;
    result.newPlayerIndex = playerIndex;
    result.newPlayerName = playerTeam_[playerIndex].name;

    activePlayerIndex_ = playerIndex;
    MarkParticipant(activePlayerIndex_);

    // Switching player profiles spends the turn, matching classic party-battle pacing.
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
    state.activePlayerIndex = activePlayerIndex_;
    if (!playerTeam_.empty() && IsKnownPlayerIndex(activePlayerIndex_))
    {
        state.player = CreateCompetitorView(ActivePlayer(), ActivePlayerStatus());
    }
    for (int index = 0; index < static_cast<int>(playerTeam_.size()); ++index)
    {
        state.playerTeam.push_back(CreateCompetitorView(playerTeam_[index], playerStatuses_[index]));
    }
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

    for (const SkillProgress& progress : ActivePlayer().skills)
    {
        const Skill* definition = data_.FindSkill(progress.skillId);
        if (definition != nullptr && skills_.IsAvailableForStyle(*definition, ActivePlayer().style))
        {
            skills.push_back(skills_.CreateSkillView(*definition, progress));
        }
    }

    return skills;
}

Competitor BattleSession::CreateCompetitor(
    int profileIndex,
    const std::string& name,
    GameType gameType,
    Spec spec,
    Style style,
    const PassiveBonuses& bonuses) const
{
    Competitor competitor;
    competitor.profileIndex = profileIndex;
    competitor.name = name;
    competitor.gameType = gameType;
    competitor.spec = spec;
    competitor.style = style;
    competitor.hp = Balance::StartingMaxHp + bonuses.maxHpBonus;
    competitor.maxHp = Balance::StartingMaxHp + bonuses.maxHpBonus;
    competitor.focus = Balance::StartingMaxFocus;
    competitor.maxFocus = Balance::StartingMaxFocus;
    competitor.basePower = Balance::StartingBasePower + bonuses.basePowerBonus;
    competitor.counterDamageBonusPercent = bonuses.counterDamageBonusPercent;

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

Competitor BattleSession::CreateCompetitor(
    const BattleSetup::PlayerSlot& slot,
    GameType gameType) const
{
    Competitor competitor = CreateCompetitor(
        slot.profileIndex,
        slot.name,
        gameType,
        slot.spec,
        slot.style,
        slot.passiveBonuses);
    if (!slot.skills.empty())
    {
        competitor.skills = slot.skills;
    }

    return competitor;
}

BattleActionResult BattleSession::RejectAction(const std::string& error) const
{
    BattleActionResult result;
    result.error = error;
    return result;
}

Competitor& BattleSession::ActivePlayer()
{
    return playerTeam_[activePlayerIndex_];
}

const Competitor& BattleSession::ActivePlayer() const
{
    return playerTeam_[activePlayerIndex_];
}

BattleStatus& BattleSession::ActivePlayerStatus()
{
    return playerStatuses_[activePlayerIndex_];
}

const BattleStatus& BattleSession::ActivePlayerStatus() const
{
    return playerStatuses_[activePlayerIndex_];
}

void BattleSession::MarkParticipant(int playerIndex)
{
    for (int participant : participatingPlayerIndices_)
    {
        if (participant == playerIndex)
        {
            return;
        }
    }

    participatingPlayerIndices_.push_back(playerIndex);
}

bool BattleSession::IsKnownPlayerIndex(int playerIndex) const
{
    return playerIndex >= 0 && playerIndex < static_cast<int>(playerTeam_.size());
}

void BattleSession::ResolveOpponentTurn(BattleActionResult& result)
{
    SkillProgress* progress = opponentAI_.SelectSkill(
        opponent_,
        opponentStatus_,
        ActivePlayer(),
        ActivePlayerStatus(),
        randomEngine_);
    if (progress == nullptr)
    {
        return;
    }

    result.skillUses.push_back(skills_.UseSkill(
        BattleActor::Opponent,
        opponent_,
        opponentStatus_,
        ActivePlayer(),
        ActivePlayerStatus(),
        *progress,
        randomEngine_));
}

CompetitorView BattleSession::CreateCompetitorView(
    const Competitor& competitor,
    const BattleStatus& status) const
{
    CompetitorView view;
    view.profileIndex = competitor.profileIndex;
    view.name = competitor.name;
    view.spec = competitor.spec;
    view.style = competitor.style;
    view.hp = competitor.hp;
    view.maxHp = competitor.maxHp;
    view.focus = competitor.focus;
    view.maxFocus = competitor.maxFocus;
    view.basePower = competitor.basePower;
    view.counterDamageBonusPercent = competitor.counterDamageBonusPercent;
    view.status = status;
    return view;
}

void BattleSession::FinishBattleIfNeeded(BattleActionResult& result)
{
    if (finished_ || (ActivePlayer().hp > 0 && opponent_.hp > 0))
    {
        return;
    }

    finished_ = true;
    winner_ = ActivePlayer().hp > 0 ? BattleWinner::Player : BattleWinner::Opponent;
    result.battleFinished = true;
    result.winner = winner_;
    AttachRewardIfNeeded(result);
}

void BattleSession::AttachRewardIfNeeded(BattleActionResult& result) const
{
    if (winner_ != BattleWinner::Player || participatingPlayerIndices_.empty())
    {
        return;
    }

    result.reward.awarded = true;
    result.reward.totalXp = PlayerWinXpReward;
    result.reward.xpPerParticipant = PlayerWinXpReward
        / static_cast<int>(participatingPlayerIndices_.size());

    for (int playerIndex : participatingPlayerIndices_)
    {
        result.reward.participantPlayerIndices.push_back(playerTeam_[playerIndex].profileIndex);
    }
}
