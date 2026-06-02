#include "BattleSession.h"

#include "SimulationData.h"

#include <algorithm>
#include <cmath>

namespace
{
    BattleActor Opposite(BattleActor actor)
    {
        return actor == BattleActor::Player ? BattleActor::Opponent : BattleActor::Player;
    }

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
    : data_(data), randomEngine_(std::random_device{}())
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
    result.events.push_back({ BattleEventType::BattleStarted });
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

    if (!IsAvailableForStyle(*definition, player_.style))
    {
        return RejectAction("That skill is not available for the active style.");
    }

    if (GetFocusCost(*definition, *progress) > player_.focus)
    {
        return RejectAction("Not enough focus.");
    }

    BattleActionResult result;
    result.accepted = true;
    UseSkill(
        BattleActor::Player,
        player_,
        playerStatus_,
        opponent_,
        opponentStatus_,
        *progress,
        result.events);
    FinishBattleIfNeeded(result.events);

    if (!finished_)
    {
        ResolveOpponentTurn(result.events);
        FinishBattleIfNeeded(result.events);
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
    result.events.push_back({
        BattleEventType::StyleChanged,
        BattleActor::Player,
        BattleActor::Player,
        "",
        "",
        static_cast<int>(style)
    });

    // Switching style is a tactical choice, not a free menu operation.
    ResolveOpponentTurn(result.events);
    FinishBattleIfNeeded(result.events);
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
        if (definition != nullptr && IsAvailableForStyle(*definition, player_.style))
        {
            skills.push_back(CreateSkillView(*definition, progress));
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
    result.events.push_back({
        BattleEventType::ActionRejected,
        BattleActor::Player,
        BattleActor::Player,
        "",
        error
    });
    return result;
}

void BattleSession::ResolveOpponentTurn(std::vector<BattleEvent>& events)
{
    SkillProgress* progress = SelectOpponentSkill();
    if (progress == nullptr)
    {
        return;
    }

    UseSkill(
        BattleActor::Opponent,
        opponent_,
        opponentStatus_,
        player_,
        playerStatus_,
        *progress,
        events);
}

void BattleSession::UseSkill(
    BattleActor actor,
    Competitor& attacker,
    BattleStatus& attackerStatus,
    Competitor& defender,
    BattleStatus& defenderStatus,
    SkillProgress& progress,
    std::vector<BattleEvent>& events)
{
    const Skill* definition = data_.FindSkill(progress.skillId);
    if (definition == nullptr)
    {
        return;
    }

    attacker.focus -= GetFocusCost(*definition, progress);
    events.push_back({
        BattleEventType::SkillUsed,
        actor,
        Opposite(actor),
        definition->id
    });

    if (!Chance(GetAccuracy(*definition, progress)))
    {
        events.push_back({
            BattleEventType::Missed,
            actor,
            Opposite(actor),
            definition->id
        });
        AwardSkillXp(actor, *definition, progress, events);
        return;
    }

    if (definition->power > 0)
    {
        const int damage = CalculateDamage(
            actor,
            *definition,
            progress,
            attacker,
            attackerStatus,
            defender,
            defenderStatus,
            events);
        defender.hp = std::max(0, defender.hp - damage);
        events.push_back({
            BattleEventType::DamageDealt,
            actor,
            Opposite(actor),
            definition->id,
            "",
            damage
        });
    }

    ApplySecondaryEffect(
        actor,
        Opposite(actor),
        *definition,
        progress,
        attacker,
        attackerStatus,
        defenderStatus,
        events);
    AwardSkillXp(actor, *definition, progress, events);
}

SkillProgress* BattleSession::SelectOpponentSkill()
{
    std::vector<SkillProgress*> affordableSkills;
    for (SkillProgress& progress : opponent_.skills)
    {
        const Skill* definition = data_.FindSkill(progress.skillId);
        if (definition != nullptr
            && IsAvailableForStyle(*definition, opponent_.style)
            && IsUsefulOpponentSkill(*definition)
            && GetFocusCost(*definition, progress) <= opponent_.focus)
        {
            affordableSkills.push_back(&progress);
        }
    }

    // Every competitor starts with a free basic action. Returning nullptr keeps
    // a malformed content edit from crashing the game.
    if (affordableSkills.empty())
    {
        return nullptr;
    }

    std::uniform_int_distribution<int> chooseSkill(0, static_cast<int>(affordableSkills.size()) - 1);
    return affordableSkills[chooseSkill(randomEngine_)];
}

SkillView BattleSession::CreateSkillView(const Skill& definition, const SkillProgress& progress) const
{
    SkillView view;
    view.id = definition.id;
    view.name = definition.name;
    view.power = GetPower(definition, progress);
    view.focusCost = GetFocusCost(definition, progress);
    view.accuracy = GetAccuracy(definition, progress);
    view.level = progress.level;
    view.xp = progress.xp;
    view.effectType = definition.effectType;
    view.effectTarget = definition.effectTarget;
    view.effectValue = GetEffectValue(definition, progress);
    view.effectUses = definition.effectUses;
    return view;
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

void BattleSession::FinishBattleIfNeeded(std::vector<BattleEvent>& events)
{
    if (finished_ || (player_.hp > 0 && opponent_.hp > 0))
    {
        return;
    }

    finished_ = true;
    winner_ = player_.hp > 0 ? BattleWinner::Player : BattleWinner::Opponent;
    events.push_back({
        BattleEventType::BattleFinished,
        winner_ == BattleWinner::Player ? BattleActor::Player : BattleActor::Opponent
    });
}

int BattleSession::GetPower(const Skill& definition, const SkillProgress& progress) const
{
    // Edit Balance::SkillLevelUpPower to change power gained per skill level.
    return definition.power + (progress.level - 1) * Balance::SkillLevelUpPower;
}

int BattleSession::GetFocusCost(const Skill& definition, const SkillProgress& progress) const
{
    // Free basic skills stay free forever, so neither side can run out of legal
    // actions during a long battle.
    if (definition.focusCost == 0)
    {
        return 0;
    }

    // Edit Balance::SkillLevelUpFocusCost to tune the strength-versus-cost tradeoff.
    return definition.focusCost + (progress.level - 1) * Balance::SkillLevelUpFocusCost;
}

double BattleSession::GetAccuracy(const Skill& definition, const SkillProgress& progress) const
{
    // Edit 0.98 or 0.02 to tune the attack accuracy cap or level improvement.
    // Pure utility actions may reach 100% so reliable heals and buffs do not miss.
    const double improvedAccuracy = definition.accuracy + (progress.level - 1) * 0.02;
    return definition.power == 0
        ? std::min(1.0, improvedAccuracy)
        : std::min(0.98, improvedAccuracy);
}

int BattleSession::GetEffectValue(const Skill& definition, const SkillProgress& progress) const
{
    const int growth = (progress.level - 1) * Balance::SkillLevelUpEffectValue;
    return definition.effectValue < 0
        ? definition.effectValue - growth
        : definition.effectValue + growth;
}

int BattleSession::CalculateDamage(
    BattleActor actor,
    const Skill& definition,
    const SkillProgress& progress,
    const Competitor& attacker,
    BattleStatus& attackerStatus,
    const Competitor& defender,
    BattleStatus& defenderStatus,
    std::vector<BattleEvent>& events) const
{
    const double specModifier = GetSpecModifier(attacker.spec, defender.spec);
    if (specModifier > Balance::NeutralModifier)
    {
        events.push_back({
            BattleEventType::SuperEffective,
            actor,
            Opposite(actor),
            definition.id
        });
    }
    else if (specModifier < Balance::NeutralModifier)
    {
        events.push_back({
            BattleEventType::NotVeryEffective,
            actor,
            Opposite(actor),
            definition.id
        });
    }

    double damage = (attacker.basePower + GetPower(definition, progress)) * specModifier;

    if (attackerStatus.attackModifierHits > 0)
    {
        damage *= (100.0 + attackerStatus.attackModifierPercent) / 100.0;
        --attackerStatus.attackModifierHits;
        if (attackerStatus.attackModifierHits == 0)
        {
            attackerStatus.attackModifierPercent = 0;
        }
    }

    if (defenderStatus.defenseModifierHits > 0)
    {
        damage *= (100.0 - defenderStatus.defenseModifierPercent) / 100.0;
        --defenderStatus.defenseModifierHits;
        if (defenderStatus.defenseModifierHits == 0)
        {
            defenderStatus.defenseModifierPercent = 0;
        }
    }

    // CORE COMBAT FORMULA:
    // Edit status percentages in SimulationData.cpp to tune buffs. A successful
    // damaging hit always deals at least one damage.
    return std::max(1, static_cast<int>(std::round(damage)));
}

void BattleSession::ApplySecondaryEffect(
    BattleActor actor,
    BattleActor target,
    const Skill& definition,
    const SkillProgress& progress,
    Competitor& attacker,
    BattleStatus& attackerStatus,
    BattleStatus& defenderStatus,
    std::vector<BattleEvent>& events) const
{
    if (definition.effectType == SkillEffectType::None)
    {
        return;
    }

    const int effectValue = GetEffectValue(definition, progress);
    BattleStatus& targetStatus = definition.effectTarget == SkillEffectTarget::Self
        ? attackerStatus
        : defenderStatus;
    const BattleActor effectTarget = definition.effectTarget == SkillEffectTarget::Self
        ? actor
        : target;

    if (definition.effectType == SkillEffectType::Heal)
    {
        const int oldHp = attacker.hp;
        attacker.hp = std::min(attacker.maxHp, attacker.hp + effectValue);
        events.push_back({
            BattleEventType::Healed,
            actor,
            actor,
            definition.id,
            "",
            attacker.hp - oldHp
        });
        return;
    }

    BattleEvent event;
    event.actor = actor;
    event.target = effectTarget;
    event.skillId = definition.id;
    event.value = effectValue;
    event.duration = definition.effectUses;

    if (definition.effectType == SkillEffectType::AttackModifier)
    {
        targetStatus.attackModifierPercent = effectValue;
        targetStatus.attackModifierHits = definition.effectUses;
        event.type = BattleEventType::AttackModified;
    }
    else
    {
        targetStatus.defenseModifierPercent = effectValue;
        targetStatus.defenseModifierHits = definition.effectUses;
        event.type = BattleEventType::DefenseModified;
    }

    events.push_back(event);
}

void BattleSession::AwardSkillXp(
    BattleActor actor,
    const Skill& definition,
    SkillProgress& progress,
    std::vector<BattleEvent>& events) const
{
    progress.xp += Balance::SkillXpPerUse;
    while (progress.xp >= Balance::SkillXpPerLevel)
    {
        progress.xp -= Balance::SkillXpPerLevel;
        ++progress.level;
        events.push_back({
            BattleEventType::SkillLeveledUp,
            actor,
            actor,
            definition.id,
            "",
            progress.level
        });
    }
}

bool BattleSession::IsAvailableForStyle(const Skill& definition, Style style) const
{
    return !definition.requiredStyle.has_value() || definition.requiredStyle.value() == style;
}

bool BattleSession::IsUsefulOpponentSkill(const Skill& definition) const
{
    // The initial AI is deliberately simple. It avoids pure utility actions
    // when their result is already active, but it does not switch styles.
    if (definition.power > 0 || definition.effectType == SkillEffectType::None)
    {
        return true;
    }

    if (definition.effectType == SkillEffectType::Heal)
    {
        return opponent_.hp < opponent_.maxHp;
    }

    const BattleStatus& targetStatus = definition.effectTarget == SkillEffectTarget::Self
        ? opponentStatus_
        : playerStatus_;

    if (definition.effectType == SkillEffectType::AttackModifier)
    {
        return targetStatus.attackModifierHits == 0;
    }

    return targetStatus.defenseModifierHits == 0;
}

double BattleSession::GetSpecModifier(Spec attackerSpec, Spec defenderSpec) const
{
    const SpecData* attacker = data_.FindSpec(attackerSpec);
    const SpecData* defender = data_.FindSpec(defenderSpec);
    if (attacker == nullptr || defender == nullptr)
    {
        return Balance::NeutralModifier;
    }

    if (attacker->counteredSpec == defenderSpec)
    {
        return Balance::AdvantageModifier;
    }

    if (defender->counteredSpec == attackerSpec)
    {
        return Balance::DisadvantageModifier;
    }

    return Balance::NeutralModifier;
}

bool BattleSession::Chance(double probability)
{
    std::uniform_real_distribution<double> roll(0.0, 1.0);
    return roll(randomEngine_) < probability;
}
