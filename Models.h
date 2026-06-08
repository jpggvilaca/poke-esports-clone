#pragma once

#include <string>
#include <vector>

// This file contains small data-only types. The simulation owns ordinary C++
// structs so it can stay independent from Godot, a console, or any future UI.

enum class GameType
{
    LeagueOfLegends
};

enum class Spec
{
    Top,
    Jungle,
    Mid,
    Adc,
    Support
};

enum class SkillEffectType
{
    None,
    Heal,
    AttackModifier,
    DefenseModifier,
    AttackPenetrationModifier,
    CooldownModifier,
    HealingReceivedModifier,
    Stunned,
    Silenced,
    Rooted,
    Mark
};

enum class SkillEffectTarget
{
    Self,
    Ally,
    Enemy,
    PlayerLineup,
    Opponent
};

enum class DrillResultQuality
{
    Miss,
    Good,
    Perfect
};

enum class SkillTone
{
    Basic,
    Pressure,
    Reliable,
    Risky,
    Utility
};

enum class TraitEffectType
{
    None,
    LowHpManaDiscountPercent,
    SetupDisruptEffectBonusPercent,
    SuperEffectiveDamageBonusPercent,
    DamagingAccuracyBonusPercent,
    PositiveEffectBonusPercent
};

enum class BattleActor
{
    None,
    Player,
    Opponent
};

enum class BattleWinner
{
    None,
    Player,
    Opponent
};

enum class Effectiveness
{
    Neutral,
    SuperEffective,
    NotVeryEffective
};

enum class MatchContext
{
    Tutorial,
    Normal,
    Nemesis,
    Major
};

enum class CareerRank
{
    Rookie,
    Ladder,
    Pro,
    Elite,
    WorldClass
};

enum class SimulationError
{
    None,
    UnknownGameType,
    BattleNeedsPlayerProfile,
    UnknownSpec,
    UnknownPlayerProfileSpec,
    UnknownActivePlayerProfile,
    BattleNotStarted,
    BattleAlreadyFinished,
    UnknownSkill,
    UnknownDrill,
    InsufficientMana,
    SkillOnCooldown,
    ActorStunned,
    ActorSilenced,
    ActorRooted,
    UnknownPlayerProfile,
    PlayerProfileAlreadyActive,
    PlayerProfileCannotPlay,
    LevelsMustBePositive,
    SkillAlreadyLearned,
    SkillNotLearned,
    SkillAlreadyActive,
    ActiveSkillSlotsFull,
    SkillNotActive,
    NegativeXpAward,
    TrophyAlreadyEarned,
    RosterFull,
    InvalidSkillTarget
};

enum class BattleEventType
{
    None,
    BattleStarted,
    PlayerSwitched,
    SkillStarted,
    DrillStarted,
    DrillCompleted,
    ManaChanged,
    CooldownStarted,
    CooldownTicked,
    CooldownReady,
    ActionBlocked,
    AttackMissed,
    DamageApplied,
    HealingApplied,
    StatusApplied,
    StatusExpired,
    MarkApplied,
    MarkTriggered,
    MarkExpired,
    FarmingTriggered,
    SkillXpGained,
    SkillLeveledUp,
    BattleFinished,
    RewardGranted
};

inline std::string ToString(GameType gameType)
{
    switch (gameType)
    {
    case GameType::LeagueOfLegends: return "League of Legends";
    }

    return "Unknown";
}

inline std::string ToString(Spec spec)
{
    switch (spec)
    {
    case Spec::Top: return "Top";
    case Spec::Jungle: return "Jungle";
    case Spec::Mid: return "Mid";
    case Spec::Adc: return "ADC";
    case Spec::Support: return "Support";
    }

    return "Unknown";
}

inline std::string ToString(SkillTone tone)
{
    switch (tone)
    {
    case SkillTone::Basic: return "Basic";
    case SkillTone::Pressure: return "Pressure";
    case SkillTone::Reliable: return "Reliable";
    case SkillTone::Risky: return "Risky";
    case SkillTone::Utility: return "Utility";
    }

    return "Unknown";
}

inline std::string ToString(CareerRank rank)
{
    switch (rank)
    {
    case CareerRank::Rookie: return "Rookie";
    case CareerRank::Ladder: return "Ladder";
    case CareerRank::Pro: return "Pro";
    case CareerRank::Elite: return "Elite";
    case CareerRank::WorldClass: return "World-Class";
    }

    return "Unknown";
}

// Skill stores rules shared by every competitor. SkillProgress below stores
// personal growth.
struct Skill
{
    std::string id;
    std::string name;
    std::string description;
    SkillTone tone = SkillTone::Basic;
    int manaCost = 0;
    int manaGain = 0;
    int cooldownTurns = 0;
    int power = 0;
    double accuracy = 1.0;
    SkillEffectType effectType = SkillEffectType::None;
    SkillEffectTarget effectTarget = SkillEffectTarget::Self;
    int effectValue = 0;
    int durationTurns = 0;
    int markBonusDamage = 0;
};

struct SkillProgress
{
    std::string skillId;
    int level = 1;
    int xp = 0;
};

struct SpecData
{
    Spec spec;
    std::string name;
    std::vector<std::string> skillIds;
    Spec counteredSpec;
    std::string defaultTraitId;
};

struct TraitDefinition
{
    std::string id;
    std::string name;
    std::string description;
    TraitEffectType effectType = TraitEffectType::None;
    int effectValue = 0;
};

struct DrillDefinition
{
    GameType gameType = GameType::LeagueOfLegends;
    std::string id;
    std::string displayName;
    std::string description;
    int missManaGain = 0;
    int goodManaGain = 0;
    int perfectManaGain = 0;
};

struct AbilityRuntimeState
{
    std::string skillId;
    int cooldownRemaining = 0;
};

// These modifiers exist for one battle only. Durations tick down after the
// affected competitor's own action opportunity.
struct BattleStatus
{
    int attackModifierPercent = 0;
    int attackModifierTurns = 0;
    int defenseModifierPercent = 0;
    int defenseModifierTurns = 0;
    int attackPenetrationPercent = 0;
    int attackPenetrationTurns = 0;
    int cooldownModifierPercent = 0;
    int cooldownModifierTurns = 0;
    int healingReceivedModifierPercent = 0;
    int healingReceivedModifierTurns = 0;
    int stunnedTurns = 0;
    int silencedTurns = 0;
    int rootedTurns = 0;
    int markTurns = 0;
    int markBonusDamage = 0;
    BattleActor markSource = BattleActor::None;
};

struct PassiveBonuses
{
    int maxHpBonus = 0;
    int basePowerBonus = 0;
    int counterDamageBonusPercent = 0;
};

// Both sides follow the same combat rules, so one type replaces the earlier
// duplicated Player and Opponent structs.
struct Competitor
{
    int profileIndex = 0;
    std::string name;
    GameType gameType = GameType::LeagueOfLegends;
    Spec spec = Spec::Top;
    std::string traitId;
    int hp = 100;
    int maxHp = 100;
    int mana = 0;
    int maxMana = 100;
    int basePower = 5;
    int counterDamageBonusPercent = 0;
    std::vector<SkillProgress> skills;
    std::vector<AbilityRuntimeState> abilityStates;

    SkillProgress* FindSkill(const std::string& skillId)
    {
        for (SkillProgress& skill : skills)
        {
            if (skill.skillId == skillId)
            {
                return &skill;
            }
        }

        return nullptr;
    }

    const SkillProgress* FindSkill(const std::string& skillId) const
    {
        for (const SkillProgress& skill : skills)
        {
            if (skill.skillId == skillId)
            {
                return &skill;
            }
        }

        return nullptr;
    }

    AbilityRuntimeState* FindAbilityState(const std::string& skillId)
    {
        for (AbilityRuntimeState& state : abilityStates)
        {
            if (state.skillId == skillId)
            {
                return &state;
            }
        }

        return nullptr;
    }

    const AbilityRuntimeState* FindAbilityState(const std::string& skillId) const
    {
        for (const AbilityRuntimeState& state : abilityStates)
        {
            if (state.skillId == skillId)
            {
                return &state;
            }
        }

        return nullptr;
    }
};

struct BattleSetup
{
    struct PlayerSlot
    {
        int profileIndex = 0;
        std::string name = "Player";
        Spec spec = Spec::Top;
        std::string traitId;
        PassiveBonuses passiveBonuses;
        std::vector<SkillProgress> skills;
        int currentHp = -1;
        int currentMana = -1;
        int maxMana = -1;
        int currentFocus = -1;
    };

    GameType gameType = GameType::LeagueOfLegends;
    std::vector<PlayerSlot> playerTeam;
    int activePlayerIndex = 0;
    std::string opponentName = "Opponent";
    Spec opponentSpec = Spec::Jungle;
    std::string opponentTraitId;
    int opponentMaxHp = -1;
    int opponentMaxMana = -1;
    int opponentMaxFocus = -1;
    int opponentBasePowerBonus = 0;
};

struct BattleRewardResult
{
    bool awarded = false;
    int totalXp = 0;
    int xpPerParticipant = 0;
    std::vector<int> participantPlayerIndices;
};

// Views are read-only snapshots. A UI receives copies instead of pointers, so
// it cannot accidentally mutate simulation state behind BattleSession's back.
struct CompetitorView
{
    int profileIndex = 0;
    std::string name;
    Spec spec = Spec::Top;
    std::string traitId;
    std::string traitName;
    std::string traitDescription;
    int hp = 0;
    int maxHp = 0;
    int mana = 0;
    int maxMana = 0;
    int basePower = 0;
    int counterDamageBonusPercent = 0;
    BattleStatus status;
};

struct BattleState
{
    bool started = false;
    bool finished = false;
    BattleWinner winner = BattleWinner::None;
    int activePlayerIndex = 0;
    CompetitorView player;
    std::vector<CompetitorView> playerTeam;
    CompetitorView opponent;
};

struct SkillView
{
    std::string id;
    std::string name;
    std::string description;
    SkillTone tone = SkillTone::Basic;
    int power = 0;
    int manaCost = 0;
    int manaGain = 0;
    int cooldownTurns = 0;
    int cooldownRemaining = 0;
    bool canUse = true;
    std::string disabledReason;
    double accuracy = 0.0;
    int level = 1;
    int xp = 0;
    SkillEffectType effectType = SkillEffectType::None;
    SkillEffectTarget effectTarget = SkillEffectTarget::Self;
    int effectValue = 0;
    int durationTurns = 0;
    int markBonusDamage = 0;
};

struct DrillView
{
    std::string id;
    std::string displayName;
    std::string description;
    int missManaGain = 0;
    int goodManaGain = 0;
    int perfectManaGain = 0;
    bool canUse = true;
    std::string disabledReason;
};

struct SkillXpResult
{
    int xpGained = 0;
    int oldXp = 0;
    int newXp = 0;
    int oldLevel = 1;
    int newLevel = 1;
    bool leveledUp = false;
};

struct DamageResult
{
    bool applied = false;
    int amount = 0;
    int markBonusDamage = 0;
    double specModifier = 1.0;
    Effectiveness effectiveness = Effectiveness::Neutral;
};

struct SecondaryEffectResult
{
    bool applied = false;
    SkillEffectType type = SkillEffectType::None;
    BattleActor target = BattleActor::None;
    int value = 0;
    int duration = 0;
    int healingAmount = 0;
    int markBonusDamage = 0;
};

struct BattleEvent
{
    BattleEventType type = BattleEventType::None;
    BattleActor actor = BattleActor::None;
    BattleActor target = BattleActor::None;
    std::string skillId;
    int actorPlayerIndex = -1;
    int targetPlayerIndex = -1;
    int profileIndex = -1;
    int targetProfileIndex = -1;
    std::string actorName;
    std::string targetName;
    int oldPlayerIndex = 0;
    int newPlayerIndex = 0;
    std::string playerName;
    int oldValue = 0;
    int newValue = 0;
    int amount = 0;
    std::string reason;
    int oldLevel = 1;
    int newLevel = 1;
    DamageResult damage;
    SecondaryEffectResult effect;
    SkillXpResult xp;
    BattleWinner winner = BattleWinner::None;
    BattleRewardResult reward;
};

struct SkillUseResult
{
    bool used = false;
    BattleActor actor = BattleActor::None;
    BattleActor target = BattleActor::None;
    std::string skillId;
    bool hit = true;
    int oldMana = 0;
    int newMana = 0;
    int oldActorHp = 0;
    int newActorHp = 0;
    int oldTargetHp = 0;
    int newTargetHp = 0;
    DamageResult damage;
    SecondaryEffectResult effect;
    SkillXpResult xp;
    std::vector<BattleEvent> events;
};

struct DrillUseResult
{
    bool used = false;
    BattleActor actor = BattleActor::None;
    std::string drillId;
    DrillResultQuality quality = DrillResultQuality::Good;
    int oldMana = 0;
    int newMana = 0;
    int manaGained = 0;
};

struct BattleActionResult
{
    bool accepted = false;
    SimulationError errorCode = SimulationError::None;
    std::string error;
    bool battleStarted = false;
    bool playerSwitched = false;
    int oldPlayerIndex = 0;
    int newPlayerIndex = 0;
    std::string newPlayerName;
    std::vector<SkillUseResult> skillUses;
    DrillUseResult drillUse;
    std::vector<BattleEvent> events;
    bool battleFinished = false;
    BattleWinner winner = BattleWinner::None;
    BattleRewardResult reward;
    BattleState finalState;
};

// Trainer state is the human/user layer: rating, money, trophies, and roster
// ownership. PlayerProfileState below is the battler layer that actually grows.
struct PlayerProfileState
{
    std::string name = "Player";
    Spec spec = Spec::Top;
    std::string traitId;
    CareerRank rank = CareerRank::Rookie;
    PassiveBonuses passiveBonuses;
    int level = 1;
    int xp = 0;
    int xpRequiredForNextLevel = 100;
    std::vector<std::string> learnedSkillIds;
    std::vector<std::string> activeSkillIds;
};

struct TrainerProfileState
{
    std::string trainerName = "Trainer";
    GameType gameType = GameType::LeagueOfLegends;
    int rating = 1000;
    int money = 0;
    int activePlayerIndex = 0;
    std::vector<PlayerProfileState> roster;
    std::vector<std::string> trophyIds;
};

struct ProfileCommandResult
{
    bool accepted = false;
    SimulationError errorCode = SimulationError::None;
    std::string error;
    int oldValue = 0;
    int newValue = 0;
    int oldLevel = 1;
    int newLevel = 1;
    bool leveledUp = false;
};

struct RatingResult
{
    bool accepted = false;
    SimulationError errorCode = SimulationError::None;
    std::string error;
    bool won = true;
    MatchContext context = MatchContext::Normal;
    int playerLevel = 1;
    int opponentLevel = 1;
    int oldRating = 0;
    int ratingChange = 0;
    int newRating = 0;
};
