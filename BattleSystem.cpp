#include "BattleSystem.h"

#include "SimulationData.h"

#include <algorithm>
#include <cmath>
#include <iostream>
#include <limits>
#include <vector>

// MIXED CORE AND SANDBOX UI:
// Keep the calculations in this file. Delete or replace the console input and
// formatted std::cout messages when Godot becomes responsible for presentation.
namespace
{
    // SANDBOX UI ONLY:
    // Delete this console input helper when Godot buttons choose skills.
    int ReadInt(const std::string& prompt, int minimum, int maximum)
    {
        while (true)
        {
            std::cout << prompt;

            int value = 0;
            if (std::cin >> value && value >= minimum && value <= maximum)
            {
                std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
                return value;
            }

            std::cout << "Please enter a number from " << minimum << " to " << maximum << ".\n";
            std::cin.clear();
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
        }
    }
}

BattleSystem::BattleSystem(const SimulationData& data, std::mt19937& randomEngine)
    : data_(data), randomEngine_(randomEngine)
{
}

BattleResult BattleSystem::Run(Player& player, Opponent& opponent)
{
    // SANDBOX UI ONLY:
    // Replace console text in this file with data returned to Godot later.
    std::cout << "\n=== Battle: " << player.name << " vs " << opponent.name << " ===\n";

    if (player.hp <= 0)
    {
        std::cout << "You need to recover before battling again.\n";
        return BattleResult::Defeat;
    }

    // Edit this loop if you want to change the order of a combat turn.
    while (player.hp > 0 && opponent.hp > 0)
    {
        DisplayStatus(player, opponent);
        RunPlayerTurn(player, opponent, *SelectPlayerSkill(player));

        if (opponent.hp > 0)
        {
            RunEnemyTurn(player, opponent, *SelectEnemySkill(opponent));
        }
    }

    const bool won = opponent.hp <= 0;
    std::cout << (won ? "\nVictory!\n" : "\nDefeat. Recover and try another simulation run.\n");
    return won ? BattleResult::Victory : BattleResult::Defeat;
}

void BattleSystem::DisplayStatus(const Player& player, const Opponent& opponent) const
{
    // SANDBOX UI ONLY:
    // Delete this formatted output when Godot renders battle status.
    std::cout
        << "\n" << player.name
        << " | HP " << player.hp << "/" << player.maxHp
        << " | Focus " << player.focus << "/" << player.maxFocus
        << " | Level " << player.level
        << " | Rank " << ToString(player.rankTier) << " (" << player.rankPoints << " RP)\n"
        << opponent.name
        << " | HP " << opponent.hp << "/" << opponent.maxHp
        << " | Focus " << opponent.focus << "/" << opponent.maxFocus
        << " | Level " << opponent.level
        << " | Style " << ToString(opponent.style) << "\n";
}

SkillProgress* BattleSystem::SelectPlayerSkill(Player& player) const
{
    // SANDBOX UI ONLY:
    // Godot should eventually pass the chosen skill ID into the simulation.
    while (true)
    {
        std::cout << "\nChoose a skill:\n";
        for (int index = 0; index < static_cast<int>(player.knownSkills.size()); ++index)
        {
            const SkillProgress& skill = player.knownSkills[index];
            const Skill* definition = data_.FindSkill(skill.skillId);
            if (definition == nullptr)
            {
                continue;
            }

            std::cout
                << index + 1 << ". " << definition->name
                << " | " << ToString(definition->style)
                << " | Power " << GetPower(*definition, skill)
                << " | Focus " << GetFocusCost(*definition, skill)
                << " | Accuracy " << static_cast<int>(GetAccuracy(*definition, skill) * 100)
                << "% | Skill Lv " << skill.level << "\n";
        }

        const int choice = ReadInt("> ", 1, static_cast<int>(player.knownSkills.size()));
        SkillProgress& selected = player.knownSkills[choice - 1];
        const Skill* definition = data_.FindSkill(selected.skillId);

        if (definition != nullptr && GetFocusCost(*definition, selected) <= player.focus)
        {
            return &selected;
        }

        std::cout << "Not enough focus for that skill. Choose another action.\n";
    }
}

SkillProgress* BattleSystem::SelectEnemySkill(Opponent& opponent)
{
    std::vector<SkillProgress*> affordableSkills;
    for (SkillProgress& skill : opponent.skills)
    {
        const Skill* definition = data_.FindSkill(skill.skillId);
        if (definition != nullptr && GetFocusCost(*definition, skill) <= opponent.focus)
        {
            affordableSkills.push_back(&skill);
        }
    }

    // Every opponent receives a free basic skill, so there is always an action.
    std::uniform_int_distribution<int> chooseSkill(0, static_cast<int>(affordableSkills.size()) - 1);
    return affordableSkills[chooseSkill(randomEngine_)];
}

void BattleSystem::RunPlayerTurn(Player& player, Opponent& opponent, SkillProgress& skill)
{
    const Skill* definition = data_.FindSkill(skill.skillId);
    if (definition == nullptr)
    {
        return;
    }

    player.focus -= GetFocusCost(*definition, skill);
    std::cout << player.name << " uses " << definition->name << ".\n";

    if (Chance(GetAccuracy(*definition, skill)))
    {
        const int damage = CalculateDamage(*definition, skill, opponent.style);
        opponent.hp = std::max(0, opponent.hp - damage);
        std::cout << "It deals " << damage << " damage.\n";
    }
    else
    {
        std::cout << "The skill misses.\n";
    }

    // Edit Balance::SkillXpPerUse if skills level too quickly or too slowly.
    AwardSkillXp(skill, *definition);
}

void BattleSystem::RunEnemyTurn(Player& player, Opponent& opponent, SkillProgress& skill)
{
    const Skill* definition = data_.FindSkill(skill.skillId);
    if (definition == nullptr)
    {
        return;
    }

    opponent.focus -= GetFocusCost(*definition, skill);
    std::cout << opponent.name << " uses " << definition->name << ".\n";

    if (Chance(GetAccuracy(*definition, skill)))
    {
        const int damage = CalculateDamage(*definition, skill, player.style);
        player.hp = std::max(0, player.hp - damage);
        std::cout << "It deals " << damage << " damage.\n";
    }
    else
    {
        std::cout << "The skill misses.\n";
    }

    AwardSkillXp(skill, *definition);
}

int BattleSystem::GetPower(const Skill& definition, const SkillProgress& skill) const
{
    // Edit the number 4 to change how much power each skill level adds.
    return definition.power + (skill.level - 1) * 4;
}

int BattleSystem::GetFocusCost(const Skill& definition, const SkillProgress& skill) const
{
    // Edit the number 2 to change how quickly skills become cheaper when leveled.
    return std::max(0, definition.focusCost - (skill.level - 1) * 2);
}

double BattleSystem::GetAccuracy(const Skill& definition, const SkillProgress& skill) const
{
    // Edit 0.98 or 0.02 to change the accuracy cap or improvement per level.
    return std::min(0.98, definition.accuracy + (skill.level - 1) * 0.02);
}

int BattleSystem::CalculateDamage(
    const Skill& definition,
    const SkillProgress& skill,
    Style defenderStyle) const
{
    const double modifier = GetStyleModifier(definition.style, defenderStyle);
    if (modifier > Balance::NeutralModifier)
    {
        std::cout << "Style advantage! ";
    }
    else if (modifier < Balance::NeutralModifier)
    {
        std::cout << "Style disadvantage. ";
    }

    // A successful hit always deals at least one damage.
    return std::max(1, static_cast<int>(std::round(GetPower(definition, skill) * modifier)));
}

void BattleSystem::AwardSkillXp(SkillProgress& skill, const Skill& definition) const
{
    skill.xp += Balance::SkillXpPerUse;
    while (skill.xp >= Balance::SkillXpPerLevel)
    {
        skill.xp -= Balance::SkillXpPerLevel;
        ++skill.level;
        std::cout << definition.name << " reached skill level " << skill.level << "!\n";
    }
}

bool BattleSystem::Chance(double probability)
{
    std::uniform_real_distribution<double> roll(0.0, 1.0);
    return roll(randomEngine_) < probability;
}

double BattleSystem::GetStyleModifier(Style attackerStyle, Style defenderStyle)
{
    if (attackerStyle == defenderStyle)
    {
        return Balance::NeutralModifier;
    }

    return HasAdvantage(attackerStyle, defenderStyle)
        ? Balance::AdvantageModifier
        : Balance::DisadvantageModifier;
}

bool BattleSystem::HasAdvantage(Style attackerStyle, Style defenderStyle)
{
    // Edit these comparisons if you ever want to change the style triangle.
    return
        (attackerStyle == Style::Aggressive && defenderStyle == Style::Balanced)
        || (attackerStyle == Style::Balanced && defenderStyle == Style::Defensive)
        || (attackerStyle == Style::Defensive && defenderStyle == Style::Aggressive);
}
