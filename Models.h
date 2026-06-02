#pragma once

#include <string>
#include <vector>

// CORE MODEL FILE
// These structs are the small amount of game state worth keeping when the
// temporary console sandbox is replaced by Godot.

enum class Spec
{
    SpecA,
    SpecB,
    SpecC
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

// SANDBOX UI ONLY:
// Godot will eventually decide how enum values appear on screen.
inline std::string ToString(Spec spec)
{
    switch (spec)
    {
    case Spec::SpecA: return "Spec A";
    case Spec::SpecB: return "Spec B";
    case Spec::SpecC: return "Spec C";
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

// Skill stores shared rules: name, style, focus cost, power, and accuracy.
// It does NOT contain a store price because a skill is not a store item.
struct Skill
{
    std::string id;
    std::string name;
    Style style;
    int focusCost;
    int power;
    double accuracy;
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

// SpecData groups the skills available to one spec.
// Edit the actual spec contents in SimulationData.cpp.
struct SpecData
{
    Spec spec;
    std::string name;
    std::vector<std::string> skillIds;
};

// A store item can teach a skill, restore focus, or do something else later.
// Price belongs here because stores sell items, not skills directly.
struct StoreItem
{
    std::string id;
    std::string name;
    std::string taughtSkillId;
    int price;
};

struct Player
{
    std::string name;
    Spec spec = Spec::SpecA;
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
    Spec spec = Spec::SpecA;
    Style style = Style::Balanced;
    int level = 1;
    int hp = 100;
    int maxHp = 100;
    int focus = 50;
    int maxFocus = 50;
    std::vector<SkillProgress> skills;
};

