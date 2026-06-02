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

    BattleStatus playerStatus;
    BattleStatus opponentStatus;

    // Edit this loop if you want to change the order of a combat turn.
    while (player.hp > 0 && opponent.hp > 0)
    {
        DisplayStatus(player, opponent);
        SkillProgress* playerSkill = SelectPlayerSkill(player);
        if (playerSkill != nullptr)
        {
            RunPlayerTurn(player, opponent, playerStatus, opponentStatus, *playerSkill);
        }
        else
        {
            // Changing loadout style is deliberately a meaningful action.
            std::cout << player.name << " spends the turn changing style.\n";
        }

        if (opponent.hp > 0)
        {
            SkillProgress* opponentSkill = SelectAutomatedSkill(
                opponent.skills,
                opponent.style,
                opponent.hp,
                opponent.maxHp,
                opponent.focus,
                opponentStatus,
                playerStatus);
            RunEnemyTurn(player, opponent, playerStatus, opponentStatus, *opponentSkill);
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
    BattleStatus playerStatus;
    BattleStatus opponentStatus;

    while (player.hp > 0 && opponent.hp > 0 && result.turns < MaximumTurns)
    {
        ++result.turns;

        SkillProgress* playerSkill = SelectAutomatedSkill(
            player.knownSkills,
            player.style,
            player.hp,
            player.maxHp,
            player.focus,
            playerStatus,
            opponentStatus);
        const Skill* playerDefinition = playerSkill == nullptr ? nullptr : data_.FindSkill(playerSkill->skillId);
        if (playerDefinition == nullptr)
        {
            result.draw = true;
            break;
        }

        UseSkill(
            player.name,
            player.hp,
            player.maxHp,
            player.focus,
            player.basePower,
            player.spec,
            playerStatus,
            opponent.hp,
            opponent.spec,
            opponentStatus,
            *playerSkill,
            false);

        if (opponent.hp <= 0)
        {
            break;
        }

        SkillProgress* opponentSkill = SelectAutomatedSkill(
            opponent.skills,
            opponent.style,
            opponent.hp,
            opponent.maxHp,
            opponent.focus,
            opponentStatus,
            playerStatus);
        const Skill* opponentDefinition = opponentSkill == nullptr ? nullptr : data_.FindSkill(opponentSkill->skillId);
        if (opponentDefinition == nullptr)
        {
            result.draw = true;
            break;
        }

        UseSkill(
            opponent.name,
            opponent.hp,
            opponent.maxHp,
            opponent.focus,
            opponent.basePower,
            opponent.spec,
            opponentStatus,
            player.hp,
            player.spec,
            playerStatus,
            *opponentSkill,
            false);
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
                << "% | Skill Lv " << skill.level
                << DescribeEffect(*definition, skill) << "\n";
        }
        std::cout << availableSkills.size() + 1 << ". Change style loadout (uses turn)\n";

        const int choice = ReadInt("> ", 1, static_cast<int>(availableSkills.size()) + 1);
        if (choice == static_cast<int>(availableSkills.size()) + 1)
        {
            SelectPlayerStyle(player);
            return nullptr;
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

SkillProgress* BattleSystem::SelectAutomatedSkill(
    std::vector<SkillProgress>& skills,
    Style style,
    int hp,
    int maxHp,
    int focus,
    const BattleStatus& selfStatus,
    const BattleStatus& opponentStatus)
{
    std::vector<SkillProgress*> affordableSkills;
    for (SkillProgress& skill : skills)
    {
        const Skill* definition = data_.FindSkill(skill.skillId);
        if (definition != nullptr
            && IsAvailableForStyle(*definition, style)
            && IsUsefulAutomatedSkill(*definition, hp, maxHp, selfStatus, opponentStatus)
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

void BattleSystem::RunPlayerTurn(
    Player& player,
    Opponent& opponent,
    BattleStatus& playerStatus,
    BattleStatus& opponentStatus,
    SkillProgress& skill)
{
    UseSkill(
        player.name,
        player.hp,
        player.maxHp,
        player.focus,
        player.basePower,
        player.spec,
        playerStatus,
        opponent.hp,
        opponent.spec,
        opponentStatus,
        skill,
        true);
}

void BattleSystem::RunEnemyTurn(
    Player& player,
    Opponent& opponent,
    BattleStatus& playerStatus,
    BattleStatus& opponentStatus,
    SkillProgress& skill)
{
    UseSkill(
        opponent.name,
        opponent.hp,
        opponent.maxHp,
        opponent.focus,
        opponent.basePower,
        opponent.spec,
        opponentStatus,
        player.hp,
        player.spec,
        playerStatus,
        skill,
        true);
}

void BattleSystem::UseSkill(
    const std::string& attackerName,
    int& attackerHp,
    int attackerMaxHp,
    int& attackerFocus,
    int attackerBasePower,
    Spec attackerSpec,
    BattleStatus& attackerStatus,
    int& defenderHp,
    Spec defenderSpec,
    BattleStatus& defenderStatus,
    SkillProgress& skill,
    bool showFeedback)
{
    const Skill* definition = data_.FindSkill(skill.skillId);
    if (definition == nullptr)
    {
        return;
    }

    attackerFocus -= GetFocusCost(*definition, skill);
    if (showFeedback)
    {
        std::cout << attackerName << " uses " << definition->name << ".\n";
    }

    if (Chance(GetAccuracy(*definition, skill)))
    {
        if (definition->power > 0)
        {
            const int damage = CalculateDamage(
                *definition,
                skill,
                attackerBasePower,
                attackerSpec,
                defenderSpec,
                attackerStatus,
                defenderStatus,
                showFeedback);
            defenderHp = std::max(0, defenderHp - damage);
            if (showFeedback)
            {
                std::cout << "It deals " << damage << " damage.\n";
            }
        }

        ApplySecondaryEffect(
            *definition,
            skill,
            attackerHp,
            attackerMaxHp,
            attackerStatus,
            defenderStatus,
            showFeedback);
    }
    else if (showFeedback)
    {
        std::cout << "The skill misses.\n";
    }

    // Edit Balance::SkillXpPerUse if skills level too quickly or too slowly.
    AwardSkillXp(skill, *definition, showFeedback);
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
    // Edit 0.98 or 0.02 to change the attack accuracy cap or improvement per
    // level. Pure utility skills may reach 100% so reliable heals and buffs do
    // not unexpectedly miss. Damage skills always retain a small miss chance.
    const double improvedAccuracy = definition.accuracy + (skill.level - 1) * 0.02;
    return definition.power == 0
        ? std::min(1.0, improvedAccuracy)
        : std::min(0.98, improvedAccuracy);
}

int BattleSystem::GetEffectValue(const Skill& definition, const SkillProgress& skill) const
{
    // Utility skills improve as they level too. Positive values grow upward;
    // negative values grow downward so debuffs become more potent.
    const int growth = (skill.level - 1) * Balance::SkillLevelUpEffectValue;
    return definition.effectValue < 0
        ? definition.effectValue - growth
        : definition.effectValue + growth;
}

std::string BattleSystem::DescribeEffect(const Skill& definition, const SkillProgress& skill) const
{
    // SANDBOX UI ONLY:
    // Delete this text formatting when Godot renders skill details visually.
    if (definition.effectType == SkillEffectType::None)
    {
        return "";
    }

    const int effectValue = GetEffectValue(definition, skill);
    if (definition.effectType == SkillEffectType::Heal)
    {
        return " | Heal " + std::to_string(effectValue) + " HP";
    }

    const std::string target = definition.effectTarget == SkillEffectTarget::Opponent
        ? "Enemy "
        : "";
    const std::string stat = definition.effectType == SkillEffectType::AttackModifier
        ? "attack "
        : "defense ";
    const std::string sign = effectValue >= 0 ? "+" : "";

    return " | " + target + stat + sign + std::to_string(effectValue)
        + "% for " + std::to_string(definition.effectUses) + " hit(s)";
}

int BattleSystem::CalculateDamage(
    const Skill& definition,
    const SkillProgress& skill,
    int attackerBasePower,
    Spec attackerSpec,
    Spec defenderSpec,
    BattleStatus& attackerStatus,
    BattleStatus& defenderStatus,
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

    double damage = (attackerBasePower + GetPower(definition, skill)) * modifier;

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
    // Edit the status percentages in SimulationData.cpp to tune buffs. A
    // successful hit always deals at least one damage.
    return std::max(1, static_cast<int>(std::round(damage)));
}

void BattleSystem::ApplySecondaryEffect(
    const Skill& definition,
    const SkillProgress& skill,
    int& attackerHp,
    int attackerMaxHp,
    BattleStatus& attackerStatus,
    BattleStatus& defenderStatus,
    bool showFeedback) const
{
    if (definition.effectType == SkillEffectType::None)
    {
        return;
    }

    const int effectValue = GetEffectValue(definition, skill);
    BattleStatus& targetStatus = definition.effectTarget == SkillEffectTarget::Self
        ? attackerStatus
        : defenderStatus;

    if (definition.effectType == SkillEffectType::Heal)
    {
        const int oldHp = attackerHp;
        attackerHp = std::min(attackerMaxHp, attackerHp + effectValue);
        if (showFeedback)
        {
            std::cout << "Recovered " << attackerHp - oldHp << " HP.\n";
        }
        return;
    }

    if (definition.effectType == SkillEffectType::AttackModifier)
    {
        targetStatus.attackModifierPercent = effectValue;
        targetStatus.attackModifierHits = definition.effectUses;
        if (showFeedback)
        {
            std::cout
                << (effectValue >= 0 ? "Attack increased" : "Attack reduced")
                << " for " << definition.effectUses << " hit(s).\n";
        }
        return;
    }

    targetStatus.defenseModifierPercent = effectValue;
    targetStatus.defenseModifierHits = definition.effectUses;
    if (showFeedback)
    {
        std::cout
            << (effectValue >= 0 ? "Defense increased" : "Defense reduced")
            << " for " << definition.effectUses << " hit(s).\n";
    }
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

bool BattleSystem::IsUsefulAutomatedSkill(
    const Skill& definition,
    int hp,
    int maxHp,
    const BattleStatus& selfStatus,
    const BattleStatus& opponentStatus) const
{
    // DEV BALANCE TOOL ONLY:
    // Keep AI intentionally simple, but do not waste pure utility actions when
    // their effect cannot help. Damage skills remain valid even with a drawback.
    if (definition.power > 0 || definition.effectType == SkillEffectType::None)
    {
        return true;
    }

    if (definition.effectType == SkillEffectType::Heal)
    {
        return hp < maxHp;
    }

    const BattleStatus& targetStatus = definition.effectTarget == SkillEffectTarget::Self
        ? selfStatus
        : opponentStatus;

    if (definition.effectType == SkillEffectType::AttackModifier)
    {
        return targetStatus.attackModifierHits == 0;
    }

    return targetStatus.defenseModifierHits == 0;
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
