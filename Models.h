#pragma once

#include <optional>
#include <string>
#include <vector>

// CORE MODEL FILE
// These structs are the small amount of game state worth keeping when the
// temporary console sandbox is replaced by Godot.

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

enum class RankTier
{
    Bronze,
    Silver,
    Gold,
    Platinum,
    Diamond,
    Master,
    Challenger
};

enum class StoreItemType
{
    SkillManual,
    RestoreHp,
    RestoreFocus,
    AttackBoost,
    DefenseBoost
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

// SANDBOX UI ONLY:
// Godot will eventually decide how enum values appear on screen.
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

inline std::string ToString(RankTier tier)
{
    switch (tier)
    {
    case RankTier::Bronze: return "Bronze";
    case RankTier::Silver: return "Silver";
    case RankTier::Gold: return "Gold";
    case RankTier::Platinum: return "Platinum";
    case RankTier::Diamond: return "Diamond";
    case RankTier::Master: return "Master";
    case RankTier::Challenger: return "Challenger";
    }

    return "Unknown";
}

// Skill stores shared rules. A missing requiredStyle means that every loadout
// can use the skill. Power above zero deals damage. A skill can also apply one
// small secondary effect, which is enough for the first style experiments.
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

// SkillProgress stores one competitor's changing relationship with a skill.
// Example: Skill says Basic Attack starts at 16 power. SkillProgress says the
// player's copy is level 2 with 25 XP. Both are needed because shared rules and
// personal progress change for different reasons.
struct SkillProgress
{
    std::string skillId;
    int level = 1;
    int xp = 0;
};

// SpecData groups skills and records one future-proof matchup rule.
// counteredSpec means "this spec deals bonus damage to that spec."
struct SpecData
{
    Spec spec;
    std::string name;
    std::vector<std::string> skillIds;
    Spec counteredSpec;
};

// GameTypeData lets one game genre own its available specs.
// Only League exists in the MVP, so the console skips a genre selection menu.
struct GameTypeData
{
    GameType gameType;
    std::string name;
    std::vector<Spec> specs;
};

// A store item can teach a skill, restore a resource, or add a bonus later.
// Price belongs here because stores sell items, not skills directly.
struct StoreItem
{
    std::string id;
    std::string name;
    StoreItemType type;
    std::string taughtSkillId;
    int price;
};

// DATA FOUNDATION ONLY:
// Tournament requirements are stored now but enforced in a later increment.
struct TournamentData
{
    std::string id;
    std::string name;
    int minimumRankPoints;
    int entryFee;
};

// CORE BATTLE STATE:
// These modifiers last only for the current fight. New effects replace old
// effects instead of stacking forever. Remaining uses are measured in hits:
// attack modifiers affect outgoing damage hits; defense modifiers affect
// incoming damage hits.
struct BattleStatus
{
    int attackModifierPercent = 0;
    int attackModifierHits = 0;
    int defenseModifierPercent = 0;
    int defenseModifierHits = 0;
};

struct Player
{
    std::string name;
    GameType gameType = GameType::LeagueOfLegends;
    Spec spec = Spec::Top;
    Style style = Style::Balanced;
    int level = 1;
    int xp = 0;
    int xpToNextLevel = 100;
    int rankPoints = 0;
    RankTier rankTier = RankTier::Bronze;
    int hp = 100;
    int maxHp = 100;
    int focus = 50;
    int maxFocus = 50;
    int basePower = 5;
    int currency = 100;
    std::vector<SkillProgress> knownSkills;

    SkillProgress* FindSkill(const std::string& skillId)
    {
        for (SkillProgress& skill : knownSkills)
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
        for (const SkillProgress& skill : knownSkills)
        {
            if (skill.skillId == skillId)
            {
                return &skill;
            }
        }

        return nullptr;
    }

    bool KnowsSkill(const std::string& skillId) const
    {
        return FindSkill(skillId) != nullptr;
    }

    void LearnSkill(const std::string& skillId)
    {
        if (!KnowsSkill(skillId))
        {
            knownSkills.push_back({ skillId });
        }
    }
};

struct Opponent
{
    std::string name;
    GameType gameType = GameType::LeagueOfLegends;
    Spec spec = Spec::Top;
    Style style = Style::Balanced;
    int level = 1;
    int hp = 100;
    int maxHp = 100;
    int focus = 50;
    int maxFocus = 50;
    int basePower = 5;
    std::vector<SkillProgress> skills;
};
