#include "BattleSession.h"

#include "BattleEventFactory.h"
#include "BattleStatusUtils.h"
#include "SimulationData.h"

#include <algorithm>
#include <random>
#include <string>

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
}

BattleSession::BattleSession(const SimulationData& data)
    : BattleSession(data, std::random_device{}())
{
}

BattleSession::BattleSession(const SimulationData& data, std::uint32_t seed)
    : data_(data),
      randomEngine_(seed),
      rules_(data),
      skills_(data, rules_, progression_),
      opponentAI_(data, rules_, skills_)
{
}

BattleActionResult BattleSession::StartBattle(const BattleSetup& setup)
{
    const BattleSetupValidation validation = ValidateBattleSetup(setup);
    if (!validation.valid)
    {
        return RejectAction(validation.errorCode, validation.message);
    }

    ResetBattleState();
    BuildPlayerTeam(setup);

    activePlayerIndex_ = setup.activePlayerIndex;
    MarkParticipant(activePlayerIndex_);

    BuildOpponentTeam(setup);
    BuildLaneObjectives();
    ResetNeutralObjectives();
    SelectStartingOpponent(setup.activeOpponentIndex);

    started_ = true;

    return CreateBattleStartedResult();
}

BattleActionResult BattleSession::UsePlayerSkill(const std::string& skillId, int targetPlayerIndex)
{
    if (!started_)
    {
        return RejectAction(SimulationError::BattleNotStarted, "Start a battle first.");
    }

    if (finished_)
    {
        return RejectAction(SimulationError::BattleAlreadyFinished, "The battle is already finished.");
    }

    Competitor& player = ActivePlayer();
    BattleStatus& playerStatus = ActivePlayerStatus();
    SkillProgress* progress = player.FindSkill(skillId);
    const Skill* definition = data_.FindSkill(skillId);
    if (progress == nullptr || definition == nullptr)
    {
        return RejectAction(SimulationError::UnknownSkill, "Unknown skill.");
    }

    AbilityRuntimeState& abilityState = EnsureAbilityState(player, skillId);
    const SkillActionValidation validation = ValidateSkillAction(
        player,
        playerStatus,
        *definition,
        *progress,
        abilityState.cooldownRemaining);
    if (!validation.canUse && validation.consumesTurn)
    {
        BattleActionResult result;
        result.accepted = true;
        FinishNonSkillActionOpportunity(
            BattleActor::Player,
            player,
            playerStatus,
            result,
            skillId,
            validation.disabledReason);
        ResolveAfterPlayerAction(result);
        result.finalState = GetState();
        return result;
    }

    if (!validation.canUse)
    {
        return RejectSkillAction(validation.errorCode, validation.message, BattleActor::Player, skillId);
    }

    SkillActionTarget target = ResolvePlayerSkillTarget(*definition, targetPlayerIndex);
    if (target.competitor == nullptr || target.status == nullptr)
    {
        return RejectSkillAction(SimulationError::InvalidSkillTarget, "Invalid target.", BattleActor::Player, skillId);
    }

    BattleActionResult result;
    result.accepted = true;
    SkillUseRequest request;
    request.actor = BattleActor::Player;
    request.competitor = &player;
    request.status = &playerStatus;
    request.progress = progress;
    request.target = target;
    request.playerIndex = activePlayerIndex_;

    ExecuteSkillAction(request, *definition, result);
    ResolveAfterPlayerAction(result);

    result.finalState = GetState();
    return result;
}

BattleActionResult BattleSession::UsePlayerSkillOnObjective(const std::string& skillId)
{
    if (!started_)
    {
        return RejectAction(SimulationError::BattleNotStarted, "Start a battle first.");
    }

    if (finished_)
    {
        return RejectAction(SimulationError::BattleAlreadyFinished, "The battle is already finished.");
    }

    Competitor& player = ActivePlayer();
    BattleStatus& playerStatus = ActivePlayerStatus();
    SkillProgress* progress = player.FindSkill(skillId);
    const Skill* definition = data_.FindSkill(skillId);
    if (progress == nullptr || definition == nullptr)
    {
        return RejectAction(SimulationError::UnknownSkill, "Unknown skill.");
    }

    AbilityRuntimeState& abilityState = EnsureAbilityState(player, skillId);
    const SkillActionValidation validation = ValidateSkillAction(
        player,
        playerStatus,
        *definition,
        *progress,
        abilityState.cooldownRemaining);
    if (!validation.canUse && validation.consumesTurn)
    {
        BattleActionResult result;
        result.accepted = true;
        FinishNonSkillActionOpportunity(
            BattleActor::Player,
            player,
            playerStatus,
            result,
            skillId,
            validation.disabledReason);
        ResolveAfterPlayerAction(result);
        result.finalState = GetState();
        return result;
    }

    if (!validation.canUse)
    {
        return RejectSkillAction(validation.errorCode, validation.message, BattleActor::Player, skillId);
    }

    if (definition->power <= 0)
    {
        return RejectSkillAction(
            SimulationError::InvalidSkillTarget,
            "Only damaging skills can target objectives.",
            BattleActor::Player,
            skillId);
    }

    if (!IsEnemyObjectiveVulnerable(BattleActor::Player))
    {
        return RejectSkillAction(
            SimulationError::InvalidSkillTarget,
            "The backdoor shield blocks the objective while an enemy champion is active.",
            BattleActor::Player,
            skillId);
    }

    BattleActionResult result;
    result.accepted = true;
    ApplyObjectiveDamage(BattleActor::Player, player, *definition, *progress, result);
    ResolveAfterPlayerAction(result);

    result.finalState = GetState();
    return result;
}

BattleActionResult BattleSession::UsePlayerPushObjective()
{
    if (!started_)
    {
        return RejectAction(SimulationError::BattleNotStarted, "Start a battle first.");
    }

    if (finished_)
    {
        return RejectAction(SimulationError::BattleAlreadyFinished, "The battle is already finished.");
    }

    Competitor& player = ActivePlayer();
    BattleStatus& playerStatus = ActivePlayerStatus();
    SkillProgress* progress = SelectBasicDamageSkill(player, playerStatus);
    if (progress == nullptr)
    {
        return RejectSkillAction(
            SimulationError::InvalidSkillTarget,
            "No basic objective attack is available.",
            BattleActor::Player,
            "push");
    }

    return UsePlayerSkillOnObjective(progress->skillId);
}

BattleActionResult BattleSession::UsePlayerAttackDragon()
{
    if (!started_)
    {
        return RejectAction(SimulationError::BattleNotStarted, "Start a battle first.");
    }

    if (finished_)
    {
        return RejectAction(SimulationError::BattleAlreadyFinished, "The battle is already finished.");
    }

    if (!dragon_.active)
    {
        return RejectSkillAction(
            SimulationError::InvalidSkillTarget,
            "Dragon is not active.",
            BattleActor::Player,
            "dragon");
    }

    Competitor& player = ActivePlayer();
    BattleStatus& playerStatus = ActivePlayerStatus();
    SkillProgress* progress = SelectBasicDamageSkill(player, playerStatus);
    if (progress == nullptr)
    {
        return RejectSkillAction(
            SimulationError::InvalidSkillTarget,
            "No basic Dragon attack is available.",
            BattleActor::Player,
            "dragon");
    }

    const Skill* definition = data_.FindSkill(progress->skillId);
    if (definition == nullptr)
    {
        return RejectAction(SimulationError::UnknownSkill, "Unknown skill.");
    }

    BattleActionResult result;
    result.accepted = true;
    ApplyDragonDamage(BattleActor::Player, player, *definition, *progress, result);
    ResolveAfterPlayerAction(result);
    result.finalState = GetState();
    return result;
}

BattleActionResult BattleSession::UsePlayerDrill(DrillResultQuality quality)
{
    if (!started_)
    {
        return RejectAction(SimulationError::BattleNotStarted, "Start a battle first.");
    }

    if (finished_)
    {
        return RejectAction(SimulationError::BattleAlreadyFinished, "The battle is already finished.");
    }

    Competitor& player = ActivePlayer();
    BattleStatus& playerStatus = ActivePlayerStatus();
    const DrillDefinition* drill = data_.FindDrill(player.gameType);
    if (drill == nullptr)
    {
        return RejectAction(SimulationError::UnknownDrill, "No drill action is configured for this genre.");
    }

    BattleActionResult result;
    result.accepted = true;

    if (playerStatus.stunnedTurns > 0)
    {
        FinishNonSkillActionOpportunity(
            BattleActor::Player,
            player,
            playerStatus,
            result,
            drill->id,
            "Stunned");
        ResolveAfterPlayerAction(result);
        result.finalState = GetState();
        return result;
    }

    result.drillUse = ResolveDrill(BattleActor::Player, player, quality, result);
    FinishNonSkillActionOpportunity(BattleActor::Player, player, playerStatus, result);
    ResolveAfterPlayerAction(result);
    result.finalState = GetState();
    return result;
}

BattleActionResult BattleSession::UsePlayerFarm()
{
    if (!started_)
    {
        return RejectAction(SimulationError::BattleNotStarted, "Start a battle first.");
    }

    if (finished_)
    {
        return RejectAction(SimulationError::BattleAlreadyFinished, "The battle is already finished.");
    }

    Competitor& player = ActivePlayer();
    BattleStatus& playerStatus = ActivePlayerStatus();

    BattleActionResult result;
    result.accepted = true;

    if (playerStatus.stunnedTurns > 0)
    {
        FinishNonSkillActionOpportunity(
            BattleActor::Player,
            player,
            playerStatus,
            result,
            "farm",
            "Stunned");
        ResolveAfterPlayerAction(result);
        result.finalState = GetState();
        return result;
    }

    ApplyPlayerFarm(player, playerStatus, result);
    FinishNonSkillActionOpportunity(BattleActor::Player, player, playerStatus, result);
    ResolveAfterPlayerAction(result);
    result.finalState = GetState();
    return result;
}

BattleActionResult BattleSession::PassPlayerTurn()
{
    if (!started_)
    {
        return RejectAction(SimulationError::BattleNotStarted, "Start a battle first.");
    }

    if (finished_)
    {
        return RejectAction(SimulationError::BattleAlreadyFinished, "The battle is already finished.");
    }

    Competitor& player = ActivePlayer();
    BattleStatus& playerStatus = ActivePlayerStatus();

    BattleActionResult result;
    result.accepted = true;
    const std::string blockedReason = playerStatus.stunnedTurns > 0 ? "Stunned" : "";
    FinishNonSkillActionOpportunity(BattleActor::Player, player, playerStatus, result, "", blockedReason);
    ResolveAfterPlayerAction(result);
    result.finalState = GetState();
    return result;
}

BattleActionResult BattleSession::SwitchPlayer(int playerIndex)
{
    (void)playerIndex;

    if (!started_)
    {
        return RejectAction(SimulationError::BattleNotStarted, "Start a battle first.");
    }

    if (finished_)
    {
        return RejectAction(SimulationError::BattleAlreadyFinished, "The battle is already finished.");
    }

    return RejectAction(SimulationError::PlayerProfileCannotPlay, "The party order is fixed during battle.");
}

BattleState BattleSession::GetState() const
{
    BattleState state;
    state.started = started_;
    state.finished = finished_;
    state.winner = winner_;
    state.activePlayerIndex = activePlayerIndex_;
    state.activeOpponentIndex = activeOpponentIndex_;
    if (!playerTeam_.empty() && IsKnownPlayerIndex(activePlayerIndex_))
    {
        state.player = CreateCompetitorView(ActivePlayer(), ActivePlayerStatus());
    }
    for (int index = 0; index < static_cast<int>(playerTeam_.size()); ++index)
    {
        state.playerTeam.push_back(CreateCompetitorView(playerTeam_[index], playerStatuses_[index]));
    }
    if (!opponentTeam_.empty() && IsKnownOpponentIndex(activeOpponentIndex_))
    {
        state.opponent = CreateCompetitorView(ActiveOpponent(), ActiveOpponentStatus());
    }
    for (int index = 0; index < static_cast<int>(opponentTeam_.size()); ++index)
    {
        state.opponentTeam.push_back(CreateCompetitorView(opponentTeam_[index], opponentStatuses_[index]));
    }
    state.playerObjectives = playerObjectives_;
    state.opponentObjectives = opponentObjectives_;
    state.activePlayerObjectiveIndex = GetActiveObjectiveIndex(playerObjectives_);
    state.activeOpponentObjectiveIndex = GetActiveObjectiveIndex(opponentObjectives_);
    state.playerObjectiveVulnerable = IsEnemyObjectiveVulnerable(BattleActor::Opponent);
    state.opponentObjectiveVulnerable = IsEnemyObjectiveVulnerable(BattleActor::Player);
    state.neutralObjective = dragon_;
    state.playerPowerBuffPercent = playerPowerBuffPercent_;
    state.opponentPowerBuffPercent = opponentPowerBuffPercent_;
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
        if (definition != nullptr)
        {
            SkillView view = skills_.CreateSkillView(
                *definition,
                progress,
                ActivePlayer(),
                ActivePlayerStatus(),
                GetCooldownRemaining(ActivePlayer(), progress.skillId));
            const SkillActionValidation validation = ValidateSkillAction(
                ActivePlayer(),
                ActivePlayerStatus(),
                *definition,
                progress,
                view.cooldownRemaining);
            view.canUse = validation.canUse;
            view.disabledReason = validation.disabledReason;
            skills.push_back(view);
        }
    }

    return skills;
}

DrillView BattleSession::GetPlayerDrill() const
{
    if (!started_ || finished_ || !IsKnownPlayerIndex(activePlayerIndex_))
    {
        return {};
    }

    return CreateDrillView(ActivePlayer(), ActivePlayerStatus());
}

BattleSession::BattleSetupValidation BattleSession::ValidateBattleSetup(const BattleSetup& setup) const
{
    if (!IsKnown(setup.gameType))
    {
        return { false, SimulationError::UnknownGameType, "Unknown game type." };
    }

    if (setup.playerTeam.empty())
    {
        return {
            false,
            SimulationError::BattleNeedsPlayerProfile,
            "A battle needs at least one player profile."
        };
    }

    if (setup.opponentTeam.empty()
        && (!IsKnown(setup.opponentSpec) || data_.FindSpec(setup.opponentSpec) == nullptr))
    {
        return { false, SimulationError::UnknownSpec, "Unknown spec." };
    }

    for (const BattleSetup::PlayerSlot& slot : setup.playerTeam)
    {
        if (!IsKnown(slot.spec) || data_.FindSpec(slot.spec) == nullptr)
        {
            return {
                false,
                SimulationError::UnknownPlayerProfileSpec,
                "Unknown player profile spec."
            };
        }
    }

    for (const BattleSetup::OpponentSlot& slot : setup.opponentTeam)
    {
        if (!IsKnown(slot.spec) || data_.FindSpec(slot.spec) == nullptr)
        {
            return { false, SimulationError::UnknownSpec, "Unknown spec." };
        }
    }

    if (setup.activePlayerIndex < 0
        || setup.activePlayerIndex >= static_cast<int>(setup.playerTeam.size()))
    {
        return {
            false,
            SimulationError::UnknownActivePlayerProfile,
            "Unknown active player profile."
        };
    }

    return {};
}

void BattleSession::ResetBattleState()
{
    playerTeam_.clear();
    playerStatuses_.clear();
    opponentTeam_.clear();
    opponentStatuses_.clear();
    participatingPlayerIndices_.clear();
    playerObjectives_.clear();
    opponentObjectives_.clear();
    ResetNeutralObjectives();
    playerPowerBuffPercent_ = 0;
    opponentPowerBuffPercent_ = 0;
    skillXpAwardedThisBattle_.clear();
    finished_ = false;
    winner_ = BattleWinner::None;
    playerActionCount_ = 0;
}

void BattleSession::BuildPlayerTeam(const BattleSetup& setup)
{
    for (const BattleSetup::PlayerSlot& slot : setup.playerTeam)
    {
        playerTeam_.push_back(CreateCompetitor(slot, setup.gameType));
        playerStatuses_.push_back({});
    }
}

void BattleSession::BuildOpponentTeam(const BattleSetup& setup)
{
    if (setup.opponentTeam.empty())
    {
        BattleSetup::OpponentSlot slot;
        slot.name = setup.opponentName;
        slot.spec = setup.opponentSpec;
        slot.traitId = setup.opponentTraitId;
        slot.maxHp = setup.opponentMaxHp;
        slot.currentHp = setup.opponentMaxHp;
        slot.maxMana = setup.opponentMaxMana > 0 ? setup.opponentMaxMana : setup.opponentMaxFocus;
        slot.basePowerBonus = setup.opponentBasePowerBonus;
        opponentTeam_.push_back(CreateCompetitor(slot, setup.gameType));
        opponentStatuses_.push_back({});
        return;
    }

    for (const BattleSetup::OpponentSlot& slot : setup.opponentTeam)
    {
        opponentTeam_.push_back(CreateCompetitor(slot, setup.gameType));
        opponentStatuses_.push_back({});
    }
}

void BattleSession::BuildLaneObjectives()
{
    playerObjectives_ = {
        { "outer_turret", "Outer turret", 75, 75, false },
        { "inhibitor", "Inhibitor", 75, 75, false },
        { "nexus", "Nexus", 75, 75, false },
    };
    opponentObjectives_ = {
        { "outer_turret", "Outer turret", 75, 75, false },
        { "inhibitor", "Inhibitor", 75, 75, false },
        { "nexus", "Nexus", 75, 75, false },
    };
}

void BattleSession::ResetNeutralObjectives()
{
    dragon_ = {};
    dragon_.id = "dragon";
    dragon_.name = "Dragon";
    dragon_.hp = 0;
    dragon_.maxHp = 80;
    dragon_.active = false;
    dragon_.respawnTimer = 6;
}

void BattleSession::SelectStartingOpponent(int requestedOpponentIndex)
{
    activeOpponentIndex_ = requestedOpponentIndex;
    if (!IsLivingOpponentIndex(activeOpponentIndex_))
    {
        activeOpponentIndex_ = FirstLivingOpponentIndex();
    }
}

BattleActionResult BattleSession::CreateBattleStartedResult() const
{
    BattleActionResult result;
    result.accepted = true;
    result.battleStarted = true;

    BattleEvent event = MakeActorEvent(
        BattleEventType::BattleStarted,
        MakeBattleEventParticipant(BattleActor::Player, activePlayerIndex_, ActivePlayer()));
    event.newPlayerIndex = activePlayerIndex_;
    event.playerName = ActivePlayer().name;
    result.events.push_back(event);
    result.finalState = GetState();
    return result;
}

Competitor BattleSession::CreateCompetitor(
    int profileIndex,
    const std::string& name,
    GameType gameType,
    Spec spec,
    const PassiveBonuses& bonuses) const
{
    Competitor competitor;
    competitor.profileIndex = profileIndex;
    competitor.name = name;
    competitor.gameType = gameType;
    competitor.spec = spec;
    const SpecData* specData = data_.FindSpec(spec);
    if (specData != nullptr)
    {
        competitor.traitId = specData->defaultTraitId;
    }
    competitor.hp = Balance::StartingMaxHp + bonuses.maxHpBonus;
    competitor.maxHp = Balance::StartingMaxHp + bonuses.maxHpBonus;
    competitor.mana = Balance::StartingMana;
    competitor.maxMana = Balance::StartingMaxMana;
    competitor.basePower = Balance::StartingBasePower + bonuses.basePowerBonus;
    competitor.counterDamageBonusPercent = bonuses.counterDamageBonusPercent;

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
        competitor.abilityStates.push_back({ specData->skillIds[index], 0 });
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
        slot.passiveBonuses);
    if (!slot.skills.empty())
    {
        competitor.skills = slot.skills;
        competitor.abilityStates.clear();
        for (const SkillProgress& progress : competitor.skills)
        {
            competitor.abilityStates.push_back({ progress.skillId, 0 });
        }
    }
    if (!slot.traitId.empty() && data_.FindTrait(slot.traitId) != nullptr)
    {
        competitor.traitId = slot.traitId;
    }
    if (slot.currentHp >= 0)
    {
        competitor.hp = std::clamp(slot.currentHp, 0, competitor.maxHp);
    }
    if (slot.maxMana > 0)
    {
        competitor.maxMana = std::max(1, slot.maxMana);
    }
    const int currentMana = slot.currentMana >= 0 ? slot.currentMana : slot.currentFocus;
    if (currentMana >= 0)
    {
        competitor.mana = std::clamp(currentMana, 0, competitor.maxMana);
    }

    return competitor;
}

Competitor BattleSession::CreateCompetitor(
    const BattleSetup::OpponentSlot& slot,
    GameType gameType) const
{
    Competitor competitor = CreateCompetitor(
        slot.profileIndex,
        slot.name,
        gameType,
        slot.spec,
        {});
    if (!slot.skills.empty())
    {
        competitor.skills = slot.skills;
        competitor.abilityStates.clear();
        for (const SkillProgress& progress : competitor.skills)
        {
            competitor.abilityStates.push_back({ progress.skillId, 0 });
        }
    }
    if (!slot.traitId.empty() && data_.FindTrait(slot.traitId) != nullptr)
    {
        competitor.traitId = slot.traitId;
    }
    if (slot.maxHp > 0)
    {
        competitor.maxHp = std::max(1, slot.maxHp);
        competitor.hp = competitor.maxHp;
    }
    if (slot.currentHp >= 0)
    {
        competitor.hp = std::clamp(slot.currentHp, 0, competitor.maxHp);
    }
    if (slot.maxMana > 0)
    {
        competitor.maxMana = std::max(1, slot.maxMana);
        competitor.mana = std::min(competitor.mana, competitor.maxMana);
    }
    const int currentMana = slot.currentMana >= 0 ? slot.currentMana : slot.currentFocus;
    if (currentMana >= 0)
    {
        competitor.mana = std::clamp(currentMana, 0, competitor.maxMana);
    }
    competitor.basePower += slot.basePowerBonus;
    return competitor;
}

BattleActionResult BattleSession::RejectAction(SimulationError errorCode, const std::string& error) const
{
    BattleActionResult result;
    result.errorCode = errorCode;
    result.error = error;
    result.finalState = GetState();
    return result;
}

void BattleSession::AppendEvents(BattleActionResult& result, const std::vector<BattleEvent>& events) const
{
    for (const BattleEvent& event : events)
    {
        result.events.push_back(event);
    }
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

Competitor& BattleSession::ActiveOpponent()
{
    return opponentTeam_[activeOpponentIndex_];
}

const Competitor& BattleSession::ActiveOpponent() const
{
    return opponentTeam_[activeOpponentIndex_];
}

BattleStatus& BattleSession::ActiveOpponentStatus()
{
    return opponentStatuses_[activeOpponentIndex_];
}

const BattleStatus& BattleSession::ActiveOpponentStatus() const
{
    return opponentStatuses_[activeOpponentIndex_];
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

bool BattleSession::IsLivingPlayerIndex(int playerIndex) const
{
    return IsKnownPlayerIndex(playerIndex) && playerTeam_[playerIndex].hp > 0;
}

bool BattleSession::HasLivingPlayer() const
{
    return FirstLivingPlayerIndex() >= 0;
}

int BattleSession::FirstLivingPlayerIndex() const
{
    for (int index = 0; index < static_cast<int>(playerTeam_.size()); ++index)
    {
        if (playerTeam_[index].hp > 0)
        {
            return index;
        }
    }

    return -1;
}

int BattleSession::NextLivingPlayerIndex(int fromPlayerIndex) const
{
    const int teamSize = static_cast<int>(playerTeam_.size());
    if (teamSize <= 0)
    {
        return -1;
    }

    const int startIndex = IsKnownPlayerIndex(fromPlayerIndex) ? fromPlayerIndex : -1;
    for (int offset = 1; offset <= teamSize; ++offset)
    {
        const int index = (startIndex + offset) % teamSize;
        if (playerTeam_[index].hp > 0)
        {
            return index;
        }
    }

    return -1;
}

bool BattleSession::IsKnownOpponentIndex(int opponentIndex) const
{
    return opponentIndex >= 0 && opponentIndex < static_cast<int>(opponentTeam_.size());
}

bool BattleSession::IsLivingOpponentIndex(int opponentIndex) const
{
    return IsKnownOpponentIndex(opponentIndex) && opponentTeam_[opponentIndex].hp > 0;
}

bool BattleSession::HasLivingOpponent() const
{
    return FirstLivingOpponentIndex() >= 0;
}

int BattleSession::FirstLivingOpponentIndex() const
{
    for (int index = 0; index < static_cast<int>(opponentTeam_.size()); ++index)
    {
        if (opponentTeam_[index].hp > 0)
        {
            return index;
        }
    }

    return -1;
}

int BattleSession::NextLivingOpponentIndex(int fromOpponentIndex) const
{
    const int teamSize = static_cast<int>(opponentTeam_.size());
    if (teamSize <= 0)
    {
        return -1;
    }

    const int startIndex = IsKnownOpponentIndex(fromOpponentIndex) ? fromOpponentIndex : -1;
    for (int offset = 1; offset <= teamSize; ++offset)
    {
        const int index = (startIndex + offset) % teamSize;
        if (opponentTeam_[index].hp > 0)
        {
            return index;
        }
    }

    return -1;
}

int BattleSession::GetActiveObjectiveIndex(const std::vector<LaneObjective>& objectives) const
{
    for (int index = 0; index < static_cast<int>(objectives.size()); ++index)
    {
        if (!objectives[index].destroyed && objectives[index].hp > 0)
        {
            return index;
        }
    }

    return objectives.empty() ? -1 : static_cast<int>(objectives.size()) - 1;
}

LaneObjective* BattleSession::GetActiveEnemyObjective(BattleActor attacker)
{
    std::vector<LaneObjective>& objectives = attacker == BattleActor::Player
        ? opponentObjectives_
        : playerObjectives_;
    const int index = GetActiveObjectiveIndex(objectives);
    if (index < 0 || index >= static_cast<int>(objectives.size()))
    {
        return nullptr;
    }

    return &objectives[index];
}

const LaneObjective* BattleSession::GetActiveEnemyObjective(BattleActor attacker) const
{
    const std::vector<LaneObjective>& objectives = attacker == BattleActor::Player
        ? opponentObjectives_
        : playerObjectives_;
    const int index = GetActiveObjectiveIndex(objectives);
    if (index < 0 || index >= static_cast<int>(objectives.size()))
    {
        return nullptr;
    }

    return &objectives[index];
}

bool BattleSession::IsEnemyObjectiveVulnerable(BattleActor attacker) const
{
    const std::vector<LaneObjective>& objectives = attacker == BattleActor::Player
        ? opponentObjectives_
        : playerObjectives_;
    const int index = GetActiveObjectiveIndex(objectives);
    if (index < 0 || index >= static_cast<int>(objectives.size()))
    {
        return false;
    }

    if (index > 0 && !objectives[index - 1].destroyed)
    {
        return false;
    }

    const bool defenderHasChampion = attacker == BattleActor::Player
        ? HasLivingOpponent()
        : HasLivingPlayer();
    return !defenderHasChampion;
}

void BattleSession::ApplyObjectiveDamage(
    BattleActor attacker,
    Competitor& competitor,
    const Skill& definition,
    SkillProgress& progress,
    BattleActionResult& result)
{
    LaneObjective* objective = GetActiveEnemyObjective(attacker);
    if (objective == nullptr)
    {
        return;
    }

    const int attackerIndex = attacker == BattleActor::Player ? activePlayerIndex_ : activeOpponentIndex_;
    BattleStatus& attackerStatus = attacker == BattleActor::Player
        ? ActivePlayerStatus()
        : ActiveOpponentStatus();
    const BattleActor targetActor = attacker == BattleActor::Player
        ? BattleActor::Opponent
        : BattleActor::Player;

    SkillUseResult skillUse;
    skillUse.used = true;
    skillUse.actor = attacker;
    skillUse.target = targetActor;
    skillUse.skillId = definition.id;
    skillUse.oldMana = competitor.mana;
    skillUse.newMana = competitor.mana;
    skillUse.oldActorHp = competitor.hp;
    skillUse.newActorHp = competitor.hp;
    skillUse.oldTargetHp = objective->hp;
    skillUse.newTargetHp = objective->hp;

    BattleEventParticipant actorParticipant = MakeBattleEventParticipant(attacker, attackerIndex, competitor);
    BattleEventParticipant targetParticipant;
    targetParticipant.actor = targetActor;
    targetParticipant.name = objective->name;

    BattleEvent started = MakeTargetedEvent(
        BattleEventType::SkillStarted,
        actorParticipant,
        targetParticipant,
        definition.id);
    result.events.push_back(started);
    skillUse.events.push_back(started);

    competitor.mana = std::clamp(
        competitor.mana - rules_.GetManaCost(definition, progress, competitor) + rules_.GetManaGain(definition, progress),
        0,
        competitor.maxMana);
    skillUse.newMana = competitor.mana;
    if (skillUse.oldMana != skillUse.newMana)
    {
        BattleEvent manaChanged = MakeActorEvent(BattleEventType::ManaChanged, actorParticipant, definition.id);
        manaChanged.skillId = definition.id;
        manaChanged.oldValue = skillUse.oldMana;
        manaChanged.newValue = skillUse.newMana;
        manaChanged.amount = skillUse.newMana - skillUse.oldMana;
        result.events.push_back(manaChanged);
        skillUse.events.push_back(manaChanged);
    }

    std::uniform_real_distribution<double> roll(0.0, 1.0);
    if (roll(randomEngine_) >= rules_.GetAccuracy(definition, progress, competitor))
    {
        skillUse.hit = false;
        BattleEvent missed = MakeTargetedEvent(
            BattleEventType::AttackMissed,
            actorParticipant,
            targetParticipant,
            definition.id);
        result.events.push_back(missed);
        skillUse.events.push_back(missed);
    }
    else
    {
        skillUse.damage = rules_.CalculateObjectiveDamage(definition, progress, competitor, attackerStatus);
        objective->hp = std::max(0, objective->hp - skillUse.damage.amount);
        objective->destroyed = objective->hp <= 0;
        skillUse.newTargetHp = objective->hp;

        BattleEvent damageApplied = MakeTargetedEvent(
            BattleEventType::DamageApplied,
            actorParticipant,
            targetParticipant,
            definition.id);
        damageApplied.oldValue = skillUse.oldTargetHp;
        damageApplied.newValue = skillUse.newTargetHp;
        damageApplied.amount = skillUse.damage.amount;
        damageApplied.damage = skillUse.damage;
        damageApplied.reason = "objective";
        result.events.push_back(damageApplied);
        skillUse.events.push_back(damageApplied);

        if (objective->destroyed)
        {
            BattleEvent destroyed = MakeTargetedEvent(
                BattleEventType::ObjectiveDestroyed,
                actorParticipant,
                targetParticipant,
                definition.id);
            destroyed.reason = objective->name;
            result.events.push_back(destroyed);
            skillUse.events.push_back(destroyed);
        }
    }

    skillUse.xp = AwardCappedSkillXp(attacker, competitor.profileIndex, progress);
    if (skillUse.xp.xpGained > 0)
    {
        BattleEvent xpGained = MakeActorEvent(BattleEventType::SkillXpGained, actorParticipant, definition.id);
        xpGained.oldValue = skillUse.xp.oldXp;
        xpGained.newValue = skillUse.xp.newXp;
        xpGained.amount = skillUse.xp.xpGained;
        xpGained.xp = skillUse.xp;
        result.events.push_back(xpGained);
        skillUse.events.push_back(xpGained);
    }
    if (skillUse.xp.leveledUp)
    {
        BattleEvent leveledUp = MakeActorEvent(BattleEventType::SkillLeveledUp, actorParticipant, definition.id);
        leveledUp.oldLevel = skillUse.xp.oldLevel;
        leveledUp.newLevel = skillUse.xp.newLevel;
        leveledUp.xp = skillUse.xp;
        result.events.push_back(leveledUp);
        skillUse.events.push_back(leveledUp);
    }

    TickCooldowns(attacker, competitor, result);
    StartCooldown(attacker, competitor, attackerStatus, definition, result);
    TickActionStatusDurations(attacker, attackerStatus, result);

    result.skillUses.push_back(skillUse);
}

void BattleSession::ApplyDragonDamage(
    BattleActor attacker,
    Competitor& competitor,
    const Skill& definition,
    SkillProgress& progress,
    BattleActionResult& result)
{
    if (!dragon_.active)
    {
        return;
    }

    const int attackerIndex = attacker == BattleActor::Player ? activePlayerIndex_ : activeOpponentIndex_;
    BattleStatus& attackerStatus = attacker == BattleActor::Player
        ? ActivePlayerStatus()
        : ActiveOpponentStatus();

    SkillUseResult skillUse;
    skillUse.used = true;
    skillUse.actor = attacker;
    skillUse.target = BattleActor::None;
    skillUse.skillId = definition.id;
    skillUse.oldMana = competitor.mana;
    skillUse.newMana = competitor.mana;
    skillUse.oldActorHp = competitor.hp;
    skillUse.newActorHp = competitor.hp;
    skillUse.oldTargetHp = dragon_.hp;
    skillUse.newTargetHp = dragon_.hp;

    BattleEventParticipant actorParticipant = MakeBattleEventParticipant(attacker, attackerIndex, competitor);
    BattleEventParticipant dragonParticipant;
    dragonParticipant.actor = BattleActor::None;
    dragonParticipant.name = dragon_.name;

    BattleEvent started = MakeTargetedEvent(
        BattleEventType::SkillStarted,
        actorParticipant,
        dragonParticipant,
        definition.id);
    result.events.push_back(started);
    skillUse.events.push_back(started);

    competitor.mana = std::clamp(
        competitor.mana - rules_.GetManaCost(definition, progress, competitor) + rules_.GetManaGain(definition, progress),
        0,
        competitor.maxMana);
    skillUse.newMana = competitor.mana;
    if (skillUse.oldMana != skillUse.newMana)
    {
        BattleEvent manaChanged = MakeActorEvent(BattleEventType::ManaChanged, actorParticipant, definition.id);
        manaChanged.skillId = definition.id;
        manaChanged.oldValue = skillUse.oldMana;
        manaChanged.newValue = skillUse.newMana;
        manaChanged.amount = skillUse.newMana - skillUse.oldMana;
        result.events.push_back(manaChanged);
        skillUse.events.push_back(manaChanged);
    }

    std::uniform_real_distribution<double> roll(0.0, 1.0);
    if (roll(randomEngine_) >= rules_.GetAccuracy(definition, progress, competitor))
    {
        skillUse.hit = false;
        BattleEvent missed = MakeTargetedEvent(
            BattleEventType::AttackMissed,
            actorParticipant,
            dragonParticipant,
            definition.id);
        result.events.push_back(missed);
        skillUse.events.push_back(missed);
    }
    else
    {
        skillUse.damage = rules_.CalculateObjectiveDamage(definition, progress, competitor, attackerStatus);
        dragon_.hp = std::max(0, dragon_.hp - skillUse.damage.amount);
        skillUse.newTargetHp = dragon_.hp;

        BattleEvent damageApplied = MakeTargetedEvent(
            BattleEventType::DamageApplied,
            actorParticipant,
            dragonParticipant,
            definition.id);
        damageApplied.oldValue = skillUse.oldTargetHp;
        damageApplied.newValue = skillUse.newTargetHp;
        damageApplied.amount = skillUse.damage.amount;
        damageApplied.damage = skillUse.damage;
        damageApplied.reason = "neutral";
        result.events.push_back(damageApplied);
        skillUse.events.push_back(damageApplied);

        if (dragon_.hp <= 0)
        {
            dragon_.active = false;
            dragon_.respawnTimer = 4;
            ApplyTeamPowerBuff(attacker, 15);

            BattleEvent slain = MakeTargetedEvent(
                BattleEventType::NeutralObjectiveSlain,
                actorParticipant,
                dragonParticipant,
                definition.id);
            slain.amount = 15;
            slain.reason = dragon_.name;
            result.events.push_back(slain);
            skillUse.events.push_back(slain);
        }
        else
        {
            const int oldHp = competitor.hp;
            competitor.hp = std::max(0, competitor.hp - 15);
            BattleEvent retaliation = MakeTargetedEvent(
                BattleEventType::DamageApplied,
                dragonParticipant,
                actorParticipant,
                "dragon-retaliation");
            retaliation.oldValue = oldHp;
            retaliation.newValue = competitor.hp;
            retaliation.amount = oldHp - competitor.hp;
            retaliation.reason = "dragon_retaliation";
            result.events.push_back(retaliation);
            skillUse.events.push_back(retaliation);
        }
    }

    skillUse.xp = AwardCappedSkillXp(attacker, competitor.profileIndex, progress);
    if (skillUse.xp.xpGained > 0)
    {
        BattleEvent xpGained = MakeActorEvent(BattleEventType::SkillXpGained, actorParticipant, definition.id);
        xpGained.oldValue = skillUse.xp.oldXp;
        xpGained.newValue = skillUse.xp.newXp;
        xpGained.amount = skillUse.xp.xpGained;
        xpGained.xp = skillUse.xp;
        result.events.push_back(xpGained);
        skillUse.events.push_back(xpGained);
    }
    if (skillUse.xp.leveledUp)
    {
        BattleEvent leveledUp = MakeActorEvent(BattleEventType::SkillLeveledUp, actorParticipant, definition.id);
        leveledUp.oldLevel = skillUse.xp.oldLevel;
        leveledUp.newLevel = skillUse.xp.newLevel;
        leveledUp.xp = skillUse.xp;
        result.events.push_back(leveledUp);
        skillUse.events.push_back(leveledUp);
    }

    TickCooldowns(attacker, competitor, result);
    StartCooldown(attacker, competitor, attackerStatus, definition, result);
    TickActionStatusDurations(attacker, attackerStatus, result);
    result.skillUses.push_back(skillUse);
}

void BattleSession::SpawnDragon(BattleActionResult& result)
{
    dragon_.id = "dragon";
    dragon_.name = "Dragon";
    dragon_.hp = dragon_.maxHp;
    dragon_.active = true;
    dragon_.respawnTimer = 0;

    BattleEvent event = MakeBattleEvent(BattleEventType::NeutralObjectiveSpawned, BattleActor::None);
    event.targetName = dragon_.name;
    event.newValue = dragon_.hp;
    event.reason = dragon_.name;
    result.events.push_back(event);
}

void BattleSession::TickNeutralObjectives(BattleActionResult& result)
{
    if (dragon_.active)
    {
        return;
    }

    for (const BattleEvent& event : result.events)
    {
        if (event.type == BattleEventType::NeutralObjectiveSlain)
        {
            return;
        }
    }

    if (dragon_.respawnTimer > 0)
    {
        --dragon_.respawnTimer;
        if (dragon_.respawnTimer > 0)
        {
            return;
        }
    }

    if (dragon_.respawnTimer == 0 && dragon_.hp <= 0)
    {
        SpawnDragon(result);
    }
}

void BattleSession::ApplyTeamPowerBuff(BattleActor actor, int percent)
{
    std::vector<Competitor>& team = actor == BattleActor::Player
        ? playerTeam_
        : opponentTeam_;
    int& teamBuff = actor == BattleActor::Player
        ? playerPowerBuffPercent_
        : opponentPowerBuffPercent_;
    teamBuff += percent;
    for (Competitor& competitor : team)
    {
        competitor.powerBuffPercent = teamBuff;
    }
}

std::string BattleSession::SkillXpAwardKey(
    BattleActor actor,
    int profileIndex,
    const std::string& skillId) const
{
    const char* actorPrefix = actor == BattleActor::Player ? "player" : "opponent";
    return std::string(actorPrefix) + ":" + std::to_string(profileIndex) + ":" + skillId;
}

int BattleSession::GetRemainingSkillXpAward(
    BattleActor actor,
    int profileIndex,
    const std::string& skillId) const
{
    const std::string key = SkillXpAwardKey(actor, profileIndex, skillId);
    const auto found = skillXpAwardedThisBattle_.find(key);
    const int awarded = found == skillXpAwardedThisBattle_.end() ? 0 : found->second;
    return std::max(0, Balance::MaxSkillXpPerBattle - awarded);
}

void BattleSession::RegisterSkillXpAward(
    BattleActor actor,
    int profileIndex,
    const std::string& skillId,
    int amount)
{
    if (amount <= 0)
    {
        return;
    }

    skillXpAwardedThisBattle_[SkillXpAwardKey(actor, profileIndex, skillId)] += amount;
}

SkillXpResult BattleSession::AwardCappedSkillXp(
    BattleActor actor,
    int profileIndex,
    SkillProgress& progress)
{
    SkillXpResult xp = progression_.AwardSkillXp(
        progress,
        GetRemainingSkillXpAward(actor, profileIndex, progress.skillId));
    RegisterSkillXpAward(actor, profileIndex, progress.skillId, xp.xpGained);
    return xp;
}

bool BattleSession::IsBasicAbility(const Skill& definition) const
{
    return definition.manaCost == 0 && definition.cooldownTurns == 0;
}

BattleSession::SkillActionValidation BattleSession::ValidateSkillAction(
    const Competitor& actor,
    const BattleStatus& status,
    const Skill& definition,
    const SkillProgress& progress,
    int cooldownRemaining) const
{
    SkillActionValidation validation;

    if (status.stunnedTurns > 0)
    {
        validation.canUse = false;
        validation.consumesTurn = true;
        validation.errorCode = SimulationError::ActorStunned;
        validation.message = "Stunned.";
        validation.disabledReason = "Stunned";
        return validation;
    }

    if (status.silencedTurns > 0 && !IsBasicAbility(definition))
    {
        validation.canUse = false;
        validation.errorCode = SimulationError::ActorSilenced;
        validation.message = "Silenced.";
        validation.disabledReason = "Silenced";
        return validation;
    }

    if (cooldownRemaining > 0)
    {
        validation.canUse = false;
        validation.errorCode = SimulationError::SkillOnCooldown;
        validation.message = "Ability is on cooldown.";
        validation.disabledReason = "Cooldown " + std::to_string(cooldownRemaining);
        return validation;
    }

    const int manaCost = rules_.GetManaCost(definition, progress, actor);
    if (manaCost > actor.mana)
    {
        validation.canUse = false;
        validation.errorCode = SimulationError::InsufficientMana;
        validation.message = "Not enough mana.";
        validation.disabledReason = "Need " + std::to_string(manaCost) + " mana";
        return validation;
    }

    return validation;
}

AbilityRuntimeState& BattleSession::EnsureAbilityState(Competitor& competitor, const std::string& skillId) const
{
    AbilityRuntimeState* state = competitor.FindAbilityState(skillId);
    if (state != nullptr)
    {
        return *state;
    }

    competitor.abilityStates.push_back({ skillId, 0 });
    return competitor.abilityStates.back();
}

int BattleSession::GetCooldownRemaining(const Competitor& competitor, const std::string& skillId) const
{
    const AbilityRuntimeState* state = competitor.FindAbilityState(skillId);
    return state == nullptr ? 0 : std::max(0, state->cooldownRemaining);
}

BattleActionResult BattleSession::RejectSkillAction(
    SimulationError errorCode,
    const std::string& error,
    BattleActor actor,
    const std::string& skillId) const
{
    BattleActionResult result = RejectAction(errorCode, error);
    AppendActionBlocked(result, actor, skillId, error);
    return result;
}

SkillActionTarget BattleSession::ResolvePlayerSkillTarget(const Skill& definition, int targetPlayerIndex)
{
    SkillActionTarget target;
    target.actor = BattleActor::Opponent;
    target.competitor = &ActiveOpponent();
    target.status = &ActiveOpponentStatus();
    target.playerIndex = -1;

    if (definition.power > 0)
    {
        if (!HasLivingOpponent())
        {
            return {};
        }
        return target;
    }

    if (definition.effectTarget == SkillEffectTarget::Self)
    {
        target.actor = BattleActor::Player;
        target.competitor = &ActivePlayer();
        target.status = &ActivePlayerStatus();
        target.playerIndex = activePlayerIndex_;
    }
    else if (definition.effectTarget == SkillEffectTarget::Ally)
    {
        const int allyIndex = targetPlayerIndex < 0 ? activePlayerIndex_ : targetPlayerIndex;
        if (!IsLivingPlayerIndex(allyIndex))
        {
            return {};
        }

        target.actor = BattleActor::Player;
        target.competitor = &playerTeam_[allyIndex];
        target.status = &playerStatuses_[allyIndex];
        target.playerIndex = allyIndex;
    }
    else if (definition.effectTarget == SkillEffectTarget::PlayerLineup)
    {
        target.actor = BattleActor::Player;
        target.competitor = &ActivePlayer();
        target.status = &ActivePlayerStatus();
        target.playerIndex = activePlayerIndex_;
    }

    return target;
}

SkillActionTarget BattleSession::ResolveOpponentSkillTarget(const Skill& definition)
{
    SkillActionTarget target;
    target.actor = BattleActor::Player;
    target.competitor = &ActivePlayer();
    target.status = &ActivePlayerStatus();
    target.playerIndex = activePlayerIndex_;

    if (definition.power <= 0
        && (definition.effectTarget == SkillEffectTarget::Self
            || definition.effectTarget == SkillEffectTarget::Ally))
    {
        target.actor = BattleActor::Opponent;
        target.competitor = &ActiveOpponent();
        target.status = &ActiveOpponentStatus();
        target.playerIndex = -1;
    }

    return target;
}

SkillUseResult BattleSession::ExecuteSkillAction(
    const SkillUseRequest& request,
    const Skill& definition,
    BattleActionResult& result)
{
    SkillUseRequest cappedRequest = request;
    if (cappedRequest.competitor != nullptr && cappedRequest.progress != nullptr)
    {
        cappedRequest.maxSkillXpAward = GetRemainingSkillXpAward(
            cappedRequest.actor,
            cappedRequest.competitor->profileIndex,
            cappedRequest.progress->skillId);
    }

    SkillUseResult skillUse = skills_.UseSkill(cappedRequest, randomEngine_);
    if (cappedRequest.competitor != nullptr && cappedRequest.progress != nullptr)
    {
        RegisterSkillXpAward(
            cappedRequest.actor,
            cappedRequest.competitor->profileIndex,
            cappedRequest.progress->skillId,
            skillUse.xp.xpGained);
    }
    AppendEvents(result, skillUse.events);
    result.skillUses.push_back(skillUse);

    if (definition.effectTarget == SkillEffectTarget::PlayerLineup
        && request.actor == BattleActor::Player
        && request.competitor != nullptr
        && request.progress != nullptr
        && skillUse.hit)
    {
        ApplyLineupEffect(definition, *request.progress, *request.competitor, result);
    }

    if (request.competitor != nullptr && request.status != nullptr)
    {
        TickCooldowns(request.actor, *request.competitor, result);
        StartCooldown(request.actor, *request.competitor, *request.status, definition, result);
        TickActionStatusDurations(request.actor, *request.status, result);
    }

    return skillUse;
}

DrillView BattleSession::CreateDrillView(const Competitor& competitor, const BattleStatus& status) const
{
    DrillView view;
    const DrillDefinition* drill = data_.FindDrill(competitor.gameType);
    if (drill == nullptr)
    {
        view.canUse = false;
        view.disabledReason = "No drill configured";
        return view;
    }

    view.id = drill->id;
    view.displayName = drill->displayName;
    view.description = drill->description;
    view.missManaGain = drill->missManaGain;
    view.goodManaGain = drill->goodManaGain;
    view.perfectManaGain = drill->perfectManaGain;
    view.canUse = status.stunnedTurns <= 0;
    view.disabledReason = view.canUse ? "" : "Stunned";
    return view;
}

DrillUseResult BattleSession::ResolveDrill(
    BattleActor actor,
    Competitor& competitor,
    DrillResultQuality quality,
    BattleActionResult& result) const
{
    DrillUseResult drillUse;
    const DrillDefinition* drill = data_.FindDrill(competitor.gameType);
    if (drill == nullptr)
    {
        return drillUse;
    }

    drillUse.used = true;
    drillUse.actor = actor;
    drillUse.drillId = drill->id;
    drillUse.quality = quality;
    drillUse.oldMana = competitor.mana;

    BattleEvent started = MakeActorEvent(BattleEventType::DrillStarted, { actor, -1, competitor.profileIndex, competitor.name }, drill->id);
    started.reason = drill->displayName;
    result.events.push_back(started);

    const int manaGain = GetDrillManaGain(*drill, quality);
    competitor.mana = std::clamp(competitor.mana + manaGain, 0, competitor.maxMana);
    drillUse.newMana = competitor.mana;
    drillUse.manaGained = std::max(0, drillUse.newMana - drillUse.oldMana);

    if (drillUse.newMana != drillUse.oldMana)
    {
        BattleEvent mana = MakeActorEvent(
            BattleEventType::ManaChanged,
            { actor, -1, competitor.profileIndex, competitor.name });
        mana.oldValue = drillUse.oldMana;
        mana.newValue = drillUse.newMana;
        mana.amount = drillUse.manaGained;
        result.events.push_back(mana);
    }

    BattleEvent completed = MakeActorEvent(
        BattleEventType::DrillCompleted,
        { actor, -1, competitor.profileIndex, competitor.name },
        drill->id);
    completed.oldValue = drillUse.oldMana;
    completed.newValue = drillUse.newMana;
    completed.amount = drillUse.manaGained;
    completed.reason = DrillQualityToString(quality);
    result.events.push_back(completed);
    return drillUse;
}

int BattleSession::GetDrillManaGain(const DrillDefinition& drill, DrillResultQuality quality) const
{
    switch (quality)
    {
    case DrillResultQuality::Miss: return drill.missManaGain;
    case DrillResultQuality::Perfect: return drill.perfectManaGain;
    case DrillResultQuality::Good: return drill.goodManaGain;
    }

    return drill.goodManaGain;
}

std::string BattleSession::DrillQualityToString(DrillResultQuality quality) const
{
    switch (quality)
    {
    case DrillResultQuality::Miss: return "miss";
    case DrillResultQuality::Perfect: return "perfect";
    case DrillResultQuality::Good: return "good";
    }

    return "good";
}

void BattleSession::StartCooldown(
    BattleActor actor,
    Competitor& competitor,
    BattleStatus& status,
    const Skill& definition,
    BattleActionResult& result) const
{
    const int cooldown = rules_.GetCooldownTurns(definition, status);
    if (cooldown <= 0)
    {
        return;
    }

    AbilityRuntimeState& abilityState = EnsureAbilityState(competitor, definition.id);
    abilityState.cooldownRemaining = cooldown;

    BattleEvent event = MakeBattleEvent(BattleEventType::CooldownStarted, actor);
    event.skillId = definition.id;
    event.newValue = cooldown;
    result.events.push_back(event);
}

void BattleSession::TickCooldowns(BattleActor actor, Competitor& competitor, BattleActionResult& result) const
{
    for (AbilityRuntimeState& abilityState : competitor.abilityStates)
    {
        if (abilityState.cooldownRemaining <= 0)
        {
            continue;
        }

        const int oldValue = abilityState.cooldownRemaining;
        --abilityState.cooldownRemaining;

        BattleEvent event = MakeBattleEvent(
            abilityState.cooldownRemaining == 0
                ? BattleEventType::CooldownReady
                : BattleEventType::CooldownTicked,
            actor);
        event.skillId = abilityState.skillId;
        event.oldValue = oldValue;
        event.newValue = abilityState.cooldownRemaining;
        result.events.push_back(event);
    }
}

void BattleSession::TickStatusDurations(BattleActor actor, BattleStatus& status, BattleActionResult& result) const
{
    for (BattleStatusEffectSlot slot : GetBattleStatusEffectSlots(status))
    {
        if (slot.turns == nullptr || *slot.turns <= 0)
        {
            continue;
        }

        --(*slot.turns);
        if (*slot.turns > 0)
        {
            continue;
        }

        if (slot.value != nullptr)
        {
            *slot.value = 0;
        }

        BattleEvent event = MakeBattleEvent(BattleEventType::StatusExpired, actor);
        event.target = actor;
        event.effect.type = slot.type;
        result.events.push_back(event);
    }

    if (status.markTurns > 0)
    {
        --status.markTurns;
        if (status.markTurns == 0)
        {
            status.markBonusDamage = 0;
            status.markSource = BattleActor::None;
            BattleEvent event = MakeBattleEvent(BattleEventType::MarkExpired, actor);
            event.target = actor;
            result.events.push_back(event);
        }
    }
}

void BattleSession::TickActionStatusDurations(
    BattleActor actor,
    BattleStatus& status,
    BattleActionResult& result) const
{
    TickStatusDurations(actor, status, result);
}

void BattleSession::FinishNonSkillActionOpportunity(
    BattleActor actor,
    Competitor& competitor,
    BattleStatus& status,
    BattleActionResult& result,
    const std::string& blockedActionId,
    const std::string& blockedReason) const
{
    if (!blockedReason.empty())
    {
        AppendActionBlocked(result, actor, blockedActionId, blockedReason);
    }

    TickCooldowns(actor, competitor, result);
    TickActionStatusDurations(actor, status, result);
}

void BattleSession::AppendActionBlocked(
    BattleActionResult& result,
    BattleActor actor,
    const std::string& skillId,
    const std::string& reason) const
{
    BattleEvent event = MakeBattleEvent(BattleEventType::ActionBlocked, actor);
    event.skillId = skillId;
    event.reason = reason;
    result.events.push_back(event);
}

int BattleSession::GetReinforcementTurns() const
{
    if (playerActionCount_ <= 10)
    {
        return 2;
    }
    if (playerActionCount_ <= 20)
    {
        return 3;
    }

    return 4;
}

void BattleSession::StartKnockoutTimers(BattleActionResult& result)
{
    StartKnockoutTimerForTeam(BattleActor::Player, playerTeam_, result);
    StartKnockoutTimerForTeam(BattleActor::Opponent, opponentTeam_, result);
}

void BattleSession::StartKnockoutTimerForTeam(
    BattleActor actor,
    std::vector<Competitor>& team,
    BattleActionResult& result)
{
    for (int index = 0; index < static_cast<int>(team.size()); ++index)
    {
        Competitor& competitor = team[index];
        if (competitor.hp > 0 || competitor.reinforcementTimer > 0)
        {
            continue;
        }

        competitor.reinforcementTimer = GetReinforcementTurns();
        BattleEvent event = MakeActorEvent(
            BattleEventType::ReinforcementStarted,
            MakeBattleEventParticipant(actor, index, competitor));
        event.newValue = competitor.reinforcementTimer;
        event.reason = "KO";
        result.events.push_back(event);
    }
}

void BattleSession::TickReinforcementTimers(BattleActionResult& result)
{
    TickReinforcementTimersForTeam(BattleActor::Player, playerTeam_, result);
    TickReinforcementTimersForTeam(BattleActor::Opponent, opponentTeam_, result);
}

void BattleSession::TickReinforcementTimersForTeam(
    BattleActor actor,
    std::vector<Competitor>& team,
    BattleActionResult& result)
{
    for (int index = 0; index < static_cast<int>(team.size()); ++index)
    {
        Competitor& competitor = team[index];
        if (competitor.hp > 0 || competitor.reinforcementTimer <= 0)
        {
            continue;
        }

        const int oldTimer = competitor.reinforcementTimer;
        --competitor.reinforcementTimer;
        if (competitor.reinforcementTimer > 0)
        {
            continue;
        }

        const int oldHp = competitor.hp;
        competitor.hp = std::max(1, competitor.maxHp);
        BattleEvent event = MakeTargetedEvent(
            BattleEventType::HealingApplied,
            MakeBattleEventParticipant(actor, index, competitor),
            MakeBattleEventParticipant(actor, index, competitor),
            "reinforcement");
        event.oldValue = oldHp;
        event.newValue = competitor.hp;
        event.amount = competitor.hp - oldHp;
        event.oldLevel = oldTimer;
        event.newValue = competitor.hp;
        event.reason = "respawn";
        result.events.push_back(event);
    }
}

void BattleSession::ResolveOpponentTurn(BattleActionResult& result)
{
    if (!HasLivingPlayer())
    {
        ResolveOpponentObjectivePushes(result);
        return;
    }

    if (!HasLivingOpponent())
    {
        return;
    }

    Competitor& opponent = ActiveOpponent();
    BattleStatus& opponentStatus = ActiveOpponentStatus();

    if (opponentStatus.stunnedTurns > 0)
    {
        FinishNonSkillActionOpportunity(
            BattleActor::Opponent,
            opponent,
            opponentStatus,
            result,
            "",
            "Stunned");
        return;
    }

    SkillProgress* progress = opponentAI_.SelectSkill(
        opponent,
        opponentStatus,
        ActivePlayer(),
        ActivePlayerStatus(),
        randomEngine_);
    if (progress == nullptr)
    {
        FinishNonSkillActionOpportunity(BattleActor::Opponent, opponent, opponentStatus, result);
        return;
    }

    const Skill* definition = data_.FindSkill(progress->skillId);
    if (definition == nullptr)
    {
        FinishNonSkillActionOpportunity(BattleActor::Opponent, opponent, opponentStatus, result);
        return;
    }

    SkillUseRequest request;
    request.actor = BattleActor::Opponent;
    request.competitor = &opponent;
    request.status = &opponentStatus;
    request.progress = progress;
    request.target = ResolveOpponentSkillTarget(*definition);
    request.playerIndex = -1;

    ExecuteSkillAction(request, *definition, result);
}

SkillProgress* BattleSession::SelectBasicDamageSkill(Competitor& competitor, BattleStatus& status) const
{
    SkillProgress* fallback = nullptr;
    for (SkillProgress& progress : competitor.skills)
    {
        const Skill* definition = data_.FindSkill(progress.skillId);
        if (definition == nullptr || definition->power <= 0 || definition->manaCost > 0)
        {
            continue;
        }

        const SkillActionValidation validation = ValidateSkillAction(
            competitor,
            status,
            *definition,
            progress,
            GetCooldownRemaining(competitor, progress.skillId));
        if (!validation.canUse)
        {
            continue;
        }

        if (fallback == nullptr)
        {
            fallback = &progress;
        }
        if (progress.skillId.find("-basic") != std::string::npos)
        {
            return &progress;
        }
    }

    return fallback;
}

SkillProgress* BattleSession::SelectObjectivePushSkill(Competitor& competitor, BattleStatus& status) const
{
    SkillProgress* selected = nullptr;
    int selectedPower = -1;
    for (SkillProgress& progress : competitor.skills)
    {
        const Skill* definition = data_.FindSkill(progress.skillId);
        if (definition == nullptr || definition->power <= 0)
        {
            continue;
        }

        const SkillActionValidation validation = ValidateSkillAction(
            competitor,
            status,
            *definition,
            progress,
            GetCooldownRemaining(competitor, progress.skillId));
        if (!validation.canUse)
        {
            continue;
        }

        const int power = rules_.GetPower(*definition, progress);
        if (power > selectedPower)
        {
            selected = &progress;
            selectedPower = power;
        }
    }

    return selected;
}

void BattleSession::ResolveOpponentObjectivePushes(BattleActionResult& result)
{
    if (finished_ || HasLivingPlayer() || !HasLivingOpponent())
    {
        return;
    }

    AdvanceToNextOpponent(result);
    Competitor& opponent = ActiveOpponent();
    BattleStatus& opponentStatus = ActiveOpponentStatus();
    SkillProgress* progress = SelectObjectivePushSkill(opponent, opponentStatus);
    if (progress == nullptr)
    {
        FinishNonSkillActionOpportunity(BattleActor::Opponent, opponent, opponentStatus, result);
        return;
    }

    const Skill* definition = data_.FindSkill(progress->skillId);
    if (definition == nullptr)
    {
        return;
    }

    ApplyObjectiveDamage(BattleActor::Opponent, opponent, *definition, *progress, result);
    FinishBattleIfNeeded(result);
}

void BattleSession::RegisterPlayerActionAndApplyFarming(BattleActionResult& result)
{
    (void)result;
    ++playerActionCount_;
}

void BattleSession::ResolveAfterPlayerAction(BattleActionResult& result)
{
    RegisterPlayerActionAndApplyFarming(result);
    FinishBattleIfNeeded(result);

    if (!finished_)
    {
        AdvanceToNextOpponent(result);
    }

    if (!finished_)
    {
        ResolveOpponentTurn(result);
        FinishBattleIfNeeded(result);
    }

    if (!finished_ && HasLivingPlayer())
    {
        AdvanceToNextPlayer(result);
        FinishBattleIfNeeded(result);
    }

    if (!finished_)
    {
        TickReinforcementTimers(result);
        StartKnockoutTimers(result);
        TickNeutralObjectives(result);
        if (!HasLivingPlayer())
        {
            AdvanceToNextPlayer(result);
        }
        if (!HasLivingOpponent())
        {
            AdvanceToNextOpponent(result);
        }
        FinishBattleIfNeeded(result);
    }
}

void BattleSession::ApplyPlayerFarm(
    Competitor& player,
    BattleStatus& status,
    BattleActionResult& result) const
{
    (void)status;

    BattleEvent farmStarted = MakeActorEvent(
        BattleEventType::FarmingTriggered,
        MakeBattleEventParticipant(BattleActor::Player, activePlayerIndex_, player),
        "farm");
    farmStarted.amount = BattleEconomySystem::FarmingManaGain;
    farmStarted.reason = "Farm";
    result.events.push_back(farmStarted);

    const int oldMana = player.mana;
    player.mana = std::clamp(
        player.mana + BattleEconomySystem::FarmingManaGain,
        0,
        player.maxMana);
    if (player.mana != oldMana)
    {
        BattleEvent manaChanged = MakeActorEvent(
            BattleEventType::ManaChanged,
            MakeBattleEventParticipant(BattleActor::Player, activePlayerIndex_, player),
            "farm");
        manaChanged.oldValue = oldMana;
        manaChanged.newValue = player.mana;
        manaChanged.amount = player.mana - oldMana;
        manaChanged.reason = "farming";
        result.events.push_back(manaChanged);
    }
}

void BattleSession::AdvanceToNextPlayer(BattleActionResult& result)
{
    const int nextPlayerIndex = NextLivingPlayerIndex(activePlayerIndex_);
    if (nextPlayerIndex < 0 || nextPlayerIndex == activePlayerIndex_)
    {
        return;
    }

    const int oldPlayerIndex = activePlayerIndex_;
    activePlayerIndex_ = nextPlayerIndex;
    MarkParticipant(activePlayerIndex_);

    result.playerSwitched = true;
    result.oldPlayerIndex = oldPlayerIndex;
    result.newPlayerIndex = activePlayerIndex_;
    result.newPlayerName = playerTeam_[activePlayerIndex_].name;

    BattleEvent event = MakeActorEvent(
        BattleEventType::PlayerSwitched,
        MakeBattleEventParticipant(BattleActor::Player, activePlayerIndex_, playerTeam_[activePlayerIndex_]));
    event.oldPlayerIndex = oldPlayerIndex;
    event.newPlayerIndex = activePlayerIndex_;
    event.playerName = playerTeam_[activePlayerIndex_].name;
    event.reason = "turn";
    result.events.push_back(event);
}

void BattleSession::AdvanceToNextOpponent(BattleActionResult& result)
{
    if (IsLivingOpponentIndex(activeOpponentIndex_))
    {
        return;
    }

    const int nextOpponentIndex = NextLivingOpponentIndex(activeOpponentIndex_);
    if (nextOpponentIndex < 0 || nextOpponentIndex == activeOpponentIndex_)
    {
        return;
    }

    const int oldOpponentIndex = activeOpponentIndex_;
    activeOpponentIndex_ = nextOpponentIndex;

    BattleEvent event = MakeActorEvent(
        BattleEventType::PlayerSwitched,
        MakeBattleEventParticipant(BattleActor::Opponent, activeOpponentIndex_, opponentTeam_[activeOpponentIndex_]));
    event.oldPlayerIndex = oldOpponentIndex;
    event.newPlayerIndex = activeOpponentIndex_;
    event.playerName = opponentTeam_[activeOpponentIndex_].name;
    event.reason = "opponent_down";
    result.events.push_back(event);
}

void BattleSession::ApplyTimedFarmingIfDue(BattleActionResult& result)
{
    economy_.ApplyTimedFarming(playerActionCount_, playerTeam_, result.events);
}

void BattleSession::ApplyLineupEffect(
    const Skill& definition,
    const SkillProgress& progress,
    Competitor& actor,
    BattleActionResult& result)
{
    if (definition.effectType == SkillEffectType::None)
    {
        return;
    }

    const int effectValue = rules_.GetEffectValue(definition, progress, actor);
    for (int index = 0; index < static_cast<int>(playerStatuses_.size()); ++index)
    {
        if (playerTeam_[index].hp <= 0)
        {
            continue;
        }

        BattleStatus& status = playerStatuses_[index];
        if (!ApplyBattleStatusEffect(status, definition.effectType, effectValue, definition.durationTurns))
        {
            continue;
        }

        BattleEvent event = MakeTargetedEvent(
            BattleEventType::StatusApplied,
            MakeBattleEventParticipant(BattleActor::Player, activePlayerIndex_, actor),
            MakeBattleEventParticipant(BattleActor::Player, index, playerTeam_[index]),
            definition.id);
        event.amount = effectValue;
        event.effect.type = definition.effectType;
        event.effect.target = BattleActor::Player;
        event.effect.value = effectValue;
        event.effect.duration = definition.durationTurns;
        result.events.push_back(event);
    }
}

CompetitorView BattleSession::CreateCompetitorView(
    const Competitor& competitor,
    const BattleStatus& status) const
{
    CompetitorView view;
    view.profileIndex = competitor.profileIndex;
    view.name = competitor.name;
    view.spec = competitor.spec;
    view.traitId = competitor.traitId;
    const TraitDefinition* trait = data_.FindTrait(competitor.traitId);
    if (trait != nullptr)
    {
        view.traitName = trait->name;
        view.traitDescription = trait->description;
    }
    view.hp = competitor.hp;
    view.maxHp = competitor.maxHp;
    view.mana = competitor.mana;
    view.maxMana = competitor.maxMana;
    view.basePower = competitor.basePower;
    view.counterDamageBonusPercent = competitor.counterDamageBonusPercent;
    view.powerBuffPercent = competitor.powerBuffPercent;
    view.reinforcementTimer = competitor.reinforcementTimer;
    view.status = status;
    return view;
}

void BattleSession::FinishBattleIfNeeded(BattleActionResult& result)
{
    if (finished_)
    {
        return;
    }

    if (!opponentObjectives_.empty()
        && opponentObjectives_.back().hp <= 0)
    {
        finished_ = true;
        winner_ = BattleWinner::Player;
    }
    else if (!playerObjectives_.empty()
        && playerObjectives_.back().hp <= 0)
    {
        finished_ = true;
        winner_ = BattleWinner::Opponent;
    }
    else
    {
        return;
    }

    result.battleFinished = true;
    result.winner = winner_;
    BattleEvent event = MakeBattleEvent(BattleEventType::BattleFinished, BattleActor::None);
    event.winner = winner_;
    result.events.push_back(event);
    AttachBattleRewardIfNeeded(result);
}

void BattleSession::AttachBattleRewardIfNeeded(BattleActionResult& result) const
{
    result.reward = economy_.CreateBattleReward(winner_, participatingPlayerIndices_, playerTeam_);

    if (!result.reward.awarded)
    {
        return;
    }

    BattleEvent event = MakeBattleEvent(BattleEventType::RewardGranted, BattleActor::Player);
    event.reward = result.reward;
    result.events.push_back(event);
}
