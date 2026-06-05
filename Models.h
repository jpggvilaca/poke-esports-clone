#pragma once

#include <optional>
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

enum class Style
{
    Aggressive,
    Defensive,
    Balanced
};

enum class SkillEffectType
{
    None,
    Heal,
    AttackModifier,
    DefenseModifier
};

enum class SkillEffectTarget
{
    Self,
    Opponent
};

enum class SkillTone
{
    Basic,
    Aggressive,
    Defensive,
    Balanced,
    Risky,
    Utility
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

inline std::string ToString(Style style)
{
    switch (style)
    {
    case Style::Aggressive: return "Aggressive";
    case Style::Defensive: return "Defensive";
    case Style::Balanced: return "Balanced";
    }

    return "Unknown";
}

inline std::string ToString(SkillTone tone)
{
    switch (tone)
    {
    case SkillTone::Basic: return "Basic";
    case SkillTone::Aggressive: return "Aggressive";
    case SkillTone::Defensive: return "Defensive";
    case SkillTone::Balanced: return "Balanced";
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

// Skill stores rules shared by every competitor. A missing requiredStyle means
// every loadout can use the skill. SkillProgress below stores personal growth.
struct Skill
{
    std::string id;
    std::string name;
    std::string description;
    SkillTone tone = SkillTone::Basic;
    std::optional<Style> requiredStyle;
    int focusCost;
    int power;
    double accuracy;
    SkillEffectType effectType = SkillEffectType::None;
    SkillEffectTarget effectTarget = SkillEffectTarget::Self;
    int effectValue = 0;
    int effectUses = 0;
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
};

// These modifiers exist for one battle only. Remaining uses are measured in
// hits: attack modifiers affect outgoing hits and defense modifiers incoming.
struct BattleStatus
{
    int attackModifierPercent = 0;
    int attackModifierHits = 0;
    int defenseModifierPercent = 0;
    int defenseModifierHits = 0;
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
    std::string name;
    GameType gameType = GameType::LeagueOfLegends;
    Spec spec = Spec::Top;
    Style style = Style::Balanced;
    int hp = 100;
    int maxHp = 100;
    int focus = 50;
    int maxFocus = 50;
    int basePower = 5;
    int counterDamageBonusPercent = 0;
    std::vector<SkillProgress> skills;

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
};

struct BattleSetup
{
    GameType gameType = GameType::LeagueOfLegends;
    std::string playerName = "Player";
    Spec playerSpec = Spec::Top;
    Style playerStyle = Style::Balanced;
    PassiveBonuses playerPassiveBonuses;
    std::vector<SkillProgress> playerSkills;
    Spec opponentSpec = Spec::Jungle;
    Style opponentStyle = Style::Balanced;
};

// Views are read-only snapshots. A UI receives copies instead of pointers, so
// it cannot accidentally mutate simulation state behind BattleSession's back.
struct CompetitorView
{
    std::string name;
    Spec spec = Spec::Top;
    Style style = Style::Balanced;
    int hp = 0;
    int maxHp = 0;
    int focus = 0;
    int maxFocus = 0;
    int basePower = 0;
    int counterDamageBonusPercent = 0;
    BattleStatus status;
};

struct BattleState
{
    bool started = false;
    bool finished = false;
    BattleWinner winner = BattleWinner::None;
    CompetitorView player;
    CompetitorView opponent;
};

struct SkillView
{
    std::string id;
    std::string name;
    std::string description;
    SkillTone tone = SkillTone::Basic;
    int power = 0;
    int focusCost = 0;
    double accuracy = 0.0;
    int level = 1;
    int xp = 0;
    SkillEffectType effectType = SkillEffectType::None;
    SkillEffectTarget effectTarget = SkillEffectTarget::Self;
    int effectValue = 0;
    int effectUses = 0;
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
};

struct SkillUseResult
{
    bool used = false;
    BattleActor actor = BattleActor::None;
    BattleActor target = BattleActor::None;
    std::string skillId;
    bool hit = true;
    DamageResult damage;
    SecondaryEffectResult effect;
    SkillXpResult xp;
};

struct BattleActionResult
{
    bool accepted = false;
    std::string error;
    bool battleStarted = false;
    bool styleChanged = false;
    Style newStyle = Style::Balanced;
    std::vector<SkillUseResult> skillUses;
    bool battleFinished = false;
    BattleWinner winner = BattleWinner::None;
};

// Trainer state is the human/user layer: rating, money, trophies, and roster
// ownership. PlayerProfileState below is the battler layer that actually grows.
struct PlayerProfileState
{
    std::string name = "Player";
    Spec spec = Spec::Top;
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
    std::string error;
    bool won = true;
    MatchContext context = MatchContext::Normal;
    int playerLevel = 1;
    int opponentLevel = 1;
    int oldRating = 0;
    int ratingChange = 0;
    int newRating = 0;
};
