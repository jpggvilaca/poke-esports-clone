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

// Events describe what happened without deciding how it should look. Godot can
// turn these into labels now and animations later without changing combat math.
enum class BattleEventType
{
    BattleStarted,
    SkillUsed,
    Missed,
    DamageDealt,
    Healed,
    SuperEffective,
    NotVeryEffective,
    AttackModified,
    DefenseModified,
    StyleChanged,
    SkillLeveledUp,
    BattleFinished,
    ActionRejected
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

// Skill stores rules shared by every competitor. A missing requiredStyle means
// every loadout can use the skill. SkillProgress below stores personal growth.
struct Skill
{
    std::string id;
    std::string name;
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
    Spec playerSpec = Spec::Top;
    Style playerStyle = Style::Balanced;
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

struct BattleEvent
{
    BattleEventType type = BattleEventType::BattleStarted;
    BattleActor actor = BattleActor::None;
    BattleActor target = BattleActor::None;
    std::string skillId;
    std::string message;
    int value = 0;
    int duration = 0;
};

struct BattleActionResult
{
    bool accepted = false;
    std::string error;
    std::vector<BattleEvent> events;
};
