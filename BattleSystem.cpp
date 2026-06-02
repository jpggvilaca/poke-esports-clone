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

AutomatedBattleResult BattleSystem::RunAutomated(Player player, Opponent opponent)
{
    // DEV BALANCE TOOL ONLY:
    // Copies are intentional. A simulated fight must not mutate a real save or
    // print thousands of combat messages. Both sides use the same simple AI.
    constexpr int MaximumTurns = 100;
    AutomatedBattleResult result;

    while (player.hp > 0 && opponent.hp > 0 && result.turns < MaximumTurns)
    {
        ++result.turns;

        SkillProgress* playerSkill = SelectAutomatedSkill(player.knownSkills, player.style, player.focus);
        const Skill* playerDefinition = playerSkill == nullptr ? nullptr : data_.FindSkill(playerSkill->skillId);
        if (playerDefinition == nullptr)
        {
            result.draw = true;
            break;
        }

        player.focus -= GetFocusCost(*playerDefinition, *playerSkill);
        if (Chance(GetAccuracy(*playerDefinition, *playerSkill)))
        {
            opponent.hp = std::max(0, opponent.hp - CalculateDamage(
                *playerDefinition,
                *playerSkill,
                player.basePower,
                player.spec,
                opponent.spec,
                false));
        }
        AwardSkillXp(*playerSkill, *playerDefinition, false);

        if (opponent.hp <= 0)
        {
            break;
        }

        SkillProgress* opponentSkill = SelectAutomatedSkill(opponent.skills, opponent.style, opponent.focus);
        const Skill* opponentDefinition = opponentSkill == nullptr ? nullptr : data_.FindSkill(opponentSkill->skillId);
        if (opponentDefinition == nullptr)
        {
            result.draw = true;
            break;
        }

        opponent.focus -= GetFocusCost(*opponentDefinition, *opponentSkill);
        if (Chance(GetAccuracy(*opponentDefinition, *opponentSkill)))
        {
            player.hp = std::max(0, player.hp - CalculateDamage(
                *opponentDefinition,
                *opponentSkill,
                opponent.basePower,
                opponent.spec,
                player.spec,
                false));
        }
        AwardSkillXp(*opponentSkill, *opponentDefinition, false);
    }

    result.playerWon = opponent.hp <= 0 && player.hp > 0;
    result.draw = result.draw || (player.hp > 0 && opponent.hp > 0);
    result.playerHpRemaining = player.hp;
    result.opponentHpRemaining = opponent.hp;
    return result;
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
        << " | Spec " << ToString(player.spec)
        << " | Style " << ToString(player.style)
        << " | Rank " << ToString(player.rankTier) << " (" << player.rankPoints << " RP)\n"
        << opponent.name
        << " | HP " << opponent.hp << "/" << opponent.maxHp
        << " | Focus " << opponent.focus << "/" << opponent.maxFocus
        << " | Level " << opponent.level
        << " | Spec " << ToString(opponent.spec)
        << " | Style " << ToString(opponent.style) << "\n";
}

SkillProgress* BattleSystem::SelectPlayerSkill(Player& player) const
{
    // SANDBOX UI ONLY:
    // Godot should eventually pass the chosen style and skill ID into the simulation.
    while (true)
    {
        std::vector<SkillProgress*> availableSkills;
        for (SkillProgress& skill : player.knownSkills)
        {
            const Skill* definition = data_.FindSkill(skill.skillId);
            if (definition != nullptr && IsAvailableForStyle(*definition, player.style))
            {
                availableSkills.push_back(&skill);
            }
        }

        std::cout << "\nChoose a skill for the " << ToString(player.style) << " loadout:\n";
        for (int index = 0; index < static_cast<int>(availableSkills.size()); ++index)
        {
            const SkillProgress& skill = *availableSkills[index];
            const Skill* definition = data_.FindSkill(skill.skillId);

            std::cout
                << index + 1 << ". " << definition->name
                << " | Power " << GetPower(*definition, skill)
                << " | Focus " << GetFocusCost(*definition, skill)
                << " | Accuracy " << static_cast<int>(GetAccuracy(*definition, skill) * 100)
                << "% | Skill Lv " << skill.level << "\n";
        }
        std::cout << availableSkills.size() + 1 << ". Change style loadout (free action)\n";

        const int choice = ReadInt("> ", 1, static_cast<int>(availableSkills.size()) + 1);
        if (choice == static_cast<int>(availableSkills.size()) + 1)
        {
            SelectPlayerStyle(player);
            continue;
        }

        SkillProgress* selected = availableSkills[choice - 1];
        const Skill* definition = data_.FindSkill(selected->skillId);
        if (definition != nullptr && GetFocusCost(*definition, *selected) <= player.focus)
        {
            return selected;
        }

        std::cout << "Not enough focus for that skill. Choose another action.\n";
    }
}

SkillProgress* BattleSystem::SelectEnemySkill(Opponent& opponent)
{
    return SelectAutomatedSkill(opponent.skills, opponent.style, opponent.focus);
}

SkillProgress* BattleSystem::SelectAutomatedSkill(std::vector<SkillProgress>& skills, Style style, int focus)
{
    std::vector<SkillProgress*> affordableSkills;
    for (SkillProgress& skill : skills)
    {
        const Skill* definition = data_.FindSkill(skill.skillId);
        if (definition != nullptr
            && IsAvailableForStyle(*definition, style)
            && GetFocusCost(*definition, skill) <= focus)
        {
            affordableSkills.push_back(&skill);
        }
    }

    // Every competitor should receive a free basic skill. Returning nullptr
    // keeps the developer simulator safe if sample data is edited incorrectly.
    if (affordableSkills.empty())
    {
        return nullptr;
    }

    std::uniform_int_distribution<int> chooseSkill(0, static_cast<int>(affordableSkills.size()) - 1);
    return affordableSkills[chooseSkill(randomEngine_)];
}

void BattleSystem::SelectPlayerStyle(Player& player) const
{
    // SANDBOX UI ONLY:
    // Godot will eventually render this as a loadout control with colors.
    std::cout
        << "\nChoose style loadout:\n"
        << "1. Aggressive\n"
        << "2. Defensive\n"
        << "3. Balanced\n";
    player.style = static_cast<Style>(ReadInt("> ", 1, 3) - 1);
    std::cout << "Changed to the " << ToString(player.style) << " loadout.\n";
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
        const int damage = CalculateDamage(*definition, skill, player.basePower, player.spec, opponent.spec);
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
        const int damage = CalculateDamage(*definition, skill, opponent.basePower, opponent.spec, player.spec);
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
    // Edit Balance::SkillLevelUpPower to change power gained per skill level.
    return definition.power + (skill.level - 1) * Balance::SkillLevelUpPower;
}

int BattleSystem::GetFocusCost(const Skill& definition, const SkillProgress& skill) const
{
    // Universal basic skills stay free forever. This guarantees that a
    // competitor cannot run out of available actions during a battle.
    if (definition.focusCost == 0)
    {
        return 0;
    }

    // A leveled skill becomes stronger and more expensive to use.
    // Edit Balance::SkillLevelUpFocusCost to tune that resource tradeoff.
    return definition.focusCost + (skill.level - 1) * Balance::SkillLevelUpFocusCost;
}

double BattleSystem::GetAccuracy(const Skill& definition, const SkillProgress& skill) const
{
    // Edit 0.98 or 0.02 to change the accuracy cap or improvement per level.
    return std::min(0.98, definition.accuracy + (skill.level - 1) * 0.02);
}

int BattleSystem::CalculateDamage(
    const Skill& definition,
    const SkillProgress& skill,
    int attackerBasePower,
    Spec attackerSpec,
    Spec defenderSpec,
    bool showFeedback) const
{
    const double modifier = GetSpecModifier(attackerSpec, defenderSpec);
    if (showFeedback && modifier > Balance::NeutralModifier)
    {
        std::cout << "Super effective! ";
    }
    else if (showFeedback && modifier < Balance::NeutralModifier)
    {
        std::cout << "Not very effective... ";
    }

    // CORE COMBAT FORMULA:
    // Edit this expression if base power and skill power should combine differently.
    // A successful hit always deals at least one damage.
    return std::max(1, static_cast<int>(std::round(
        (attackerBasePower + GetPower(definition, skill)) * modifier)));
}

void BattleSystem::AwardSkillXp(SkillProgress& skill, const Skill& definition, bool showFeedback) const
{
    skill.xp += Balance::SkillXpPerUse;
    while (skill.xp >= Balance::SkillXpPerLevel)
    {
        skill.xp -= Balance::SkillXpPerLevel;
        ++skill.level;
        if (showFeedback)
        {
            std::cout << definition.name << " reached skill level " << skill.level << "!\n";
        }
    }
}

bool BattleSystem::Chance(double probability)
{
    std::uniform_real_distribution<double> roll(0.0, 1.0);
    return roll(randomEngine_) < probability;
}

bool BattleSystem::IsAvailableForStyle(const Skill& definition, Style style) const
{
    // Universal basic skills have no required style. Special skills require the
    // active loadout style and can be presented as colored actions in Godot.
    return !definition.requiredStyle.has_value() || definition.requiredStyle.value() == style;
}

double BattleSystem::GetSpecModifier(Spec attackerSpec, Spec defenderSpec) const
{
    if (data_.GetSpec(attackerSpec).counteredSpec == defenderSpec)
    {
        return Balance::AdvantageModifier;
    }

    if (data_.GetSpec(defenderSpec).counteredSpec == attackerSpec)
    {
        return Balance::DisadvantageModifier;
    }

    return Balance::NeutralModifier;
}
