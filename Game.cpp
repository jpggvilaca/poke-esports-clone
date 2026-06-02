#include "Game.h"

#include <algorithm>
#include <iostream>
#include <limits>

// SANDBOX UI ONLY:
// Most of this file exists to manually exercise the core systems before Godot.
// Read SANDBOX_NOTES.md before moving to the Godot frontend.
namespace
{
    // SANDBOX UI ONLY:
    // Delete these console input helpers when Godot owns input and menus.
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

    std::string ReadName()
    {
        while (true)
        {
            std::cout << "Competitor name: ";

            std::string name;
            std::getline(std::cin, name);
            if (!name.empty())
            {
                return name;
            }

            std::cout << "Please enter at least one character.\n";
        }
    }
}

Game::Game()
    : randomEngine_(std::random_device{}()),
      battleSystem_(data_, randomEngine_)
{
}

void Game::Run()
{
    // SANDBOX UI ONLY:
    // Delete this menu loop when Godot calls the simulation through UI actions.
    std::cout << "Esports RPG systems sandbox\n";
    std::cout << "This build exercises simulation systems without campaign scripting.\n";

    bool running = true;
    while (running)
    {
        DisplayMenu();
        switch (ReadInt("> ", 1, 8))
        {
        case 1: CreateCompetitor(); break;
        case 2: ViewProfile(); break;
        case 3: RunGeneratedBattle(); break;
        case 4: VisitShop(); break;
        case 5: RestoreCompetitor(); break;
        case 6: RunSampleTournament(); break;
        case 7: RunManualRivalBattle(); break;
        case 8: running = false; break;
        }
    }

    std::cout << "Sandbox session ended. Progress was intentionally not saved.\n";
}

void Game::DisplayMenu() const
{
    // Edit this menu and the switch in Run() together when adding sandbox tools.
    std::cout
        << "\n=== Systems Sandbox ===\n"
        << "1. Create or reset competitor\n"
        << "2. View profile and skills\n"
        << "3. Generate opponent battle\n"
        << "4. Visit skill manual shop\n"
        << "5. Restore HP and focus\n"
        << "6. Run configurable sample tournament\n"
        << "7. Generate rival battle manually\n"
        << "8. Exit\n";
}

void Game::CreateCompetitor()
{
    const std::string name = ReadName();

    std::cout << "\nChoose a spec:\n";
    const std::vector<SpecData>& specs = data_.GetSpecs();
    for (int index = 0; index < static_cast<int>(specs.size()); ++index)
    {
        std::cout << index + 1 << ". " << specs[index].name << "\n";
    }
    const int specChoice = ReadInt("> ", 1, static_cast<int>(specs.size()));

    std::cout
        << "\nChoose a personal style:\n"
        << "1. Aggressive\n"
        << "2. Defensive\n"
        << "3. Balanced\n";
    const int styleChoice = ReadInt("> ", 1, 3);

    const SpecData& spec = specs[specChoice - 1];
    player_ = Player{};
    player_->name = name;
    player_->spec = spec.spec;
    player_->style = static_cast<Style>(styleChoice - 1);

    // Edit these two lines if you want players to start with more or fewer skills.
    player_->LearnSkill(spec.skillIds[0]);
    player_->LearnSkill(spec.skillIds[1]);

    std::cout
        << "\nCreated " << player_->name
        << " | Spec " << ToString(player_->spec)
        << " | Style " << ToString(player_->style) << "\n"
        << "Learned a free basic skill and one starter skill.\n";
}

void Game::ViewProfile() const
{
    if (!RequirePlayer())
    {
        return;
    }

    std::cout
        << "\n=== Competitor Profile ===\n"
        << "Name: " << player_->name << "\n"
        << "Spec: " << ToString(player_->spec) << "\n"
        << "Style: " << ToString(player_->style) << "\n"
        << "Level: " << player_->level << " | XP: " << player_->xp << "/" << player_->xpToNextLevel << "\n"
        << "Rank: " << ToString(player_->rankTier) << " | RP: " << player_->rankPoints << "\n"
        << "HP: " << player_->hp << "/" << player_->maxHp << "\n"
        << "Focus: " << player_->focus << "/" << player_->maxFocus << "\n"
        << "Currency: " << player_->currency << "\n"
        << "\nKnown skills:\n";

    for (const SkillProgress& skill : player_->knownSkills)
    {
        const Skill* definition = data_.FindSkill(skill.skillId);
        if (definition != nullptr)
        {
            std::cout
                << "- " << definition->name
                << " | " << ToString(definition->style)
                << " | Skill Lv " << skill.level
                << " | XP " << skill.xp << "/" << Balance::SkillXpPerLevel
                << " | Base power " << definition->power
                << " | Base focus " << definition->focusCost
                << "\n";
        }
    }
}

void Game::RunGeneratedBattle()
{
    if (!RequirePlayer())
    {
        return;
    }

    Opponent opponent = GenerateOpponent();
    if (battleSystem_.Run(*player_, opponent) == BattleResult::Victory)
    {
        AwardVictory(opponent);
    }
}

void Game::VisitShop()
{
    if (!RequirePlayer())
    {
        return;
    }

    std::vector<const StoreItem*> availableItems;
    for (const StoreItem& item : data_.GetStoreItems())
    {
        const std::vector<std::string>& specSkills = data_.GetSpec(player_->spec).skillIds;
        const bool belongsToSpec = std::find(specSkills.begin(), specSkills.end(), item.taughtSkillId) != specSkills.end();
        if (belongsToSpec && !player_->KnowsSkill(item.taughtSkillId))
        {
            availableItems.push_back(&item);
        }
    }

    if (availableItems.empty())
    {
        std::cout << "No manuals are currently available for your spec.\n";
        return;
    }

    std::cout << "\n=== Skill Manual Shop ===\n";
    for (int index = 0; index < static_cast<int>(availableItems.size()); ++index)
    {
        std::cout
            << index + 1 << ". " << availableItems[index]->name
            << " | Price " << availableItems[index]->price << "\n";
    }
    std::cout << availableItems.size() + 1 << ". Leave shop\n";

    const int choice = ReadInt("> ", 1, static_cast<int>(availableItems.size()) + 1);
    if (choice == static_cast<int>(availableItems.size()) + 1)
    {
        return;
    }

    const StoreItem& item = *availableItems[choice - 1];
    if (item.price > player_->currency)
    {
        std::cout << "Not enough currency for that manual.\n";
        return;
    }

    player_->currency -= item.price;
    player_->LearnSkill(item.taughtSkillId);
    std::cout << "Purchased " << item.name << ".\n";
}

void Game::RestoreCompetitor()
{
    if (!RequirePlayer())
    {
        return;
    }

    // Later, this can become a hotel or item action. For now it is a sandbox tool.
    player_->hp = player_->maxHp;
    player_->focus = player_->maxFocus;
    std::cout << "HP and focus restored.\n";
}

void Game::RunSampleTournament()
{
    if (!RequirePlayer())
    {
        return;
    }

    const int rounds = ReadInt("How many generated rounds? (1-5): ", 1, 5);
    std::cout << "\n=== Configurable Sandbox Tournament ===\n";
    std::cout << "HP and focus carry between rounds. Recover before entering if needed.\n";

    // This loop is deliberately not a story event. A future campaign layer can
    // decide tournament names, requirements, rewards, and when events appear.
    for (int round = 1; round <= rounds; ++round)
    {
        std::cout << "\n--- Round " << round << " of " << rounds << " ---\n";
        Opponent opponent = GenerateOpponent();
        if (battleSystem_.Run(*player_, opponent) == BattleResult::Defeat)
        {
            std::cout << "Tournament simulation ended.\n";
            return;
        }

        AwardVictory(opponent);
    }

    std::cout << "Tournament simulation complete. Campaign trophies are deferred.\n";
}

void Game::RunManualRivalBattle()
{
    if (!RequirePlayer())
    {
        return;
    }

    // The sandbox generates a rival only when you choose this menu option.
    // A future campaign layer will decide when recurring rival battles happen.
    Opponent rival = GenerateOpponent(true);
    std::cout
        << "\nGenerated rival manually: " << rival.name
        << " | Spec " << ToString(rival.spec)
        << " | Counter style " << ToString(rival.style)
        << " | Level " << rival.level << "\n";

    if (battleSystem_.Run(*player_, rival) == BattleResult::Victory)
    {
        AwardVictory(rival);
    }
}

Opponent Game::GenerateOpponent(bool isRival)
{
    static const std::vector<std::string> names = {
        "Opponent A", "Opponent B", "Opponent C", "Opponent D",
        "Opponent E", "Opponent F", "Opponent G", "Opponent H"
    };

    Opponent opponent;
    opponent.name = isRival ? "Old Friend" : names[RandomInt(0, static_cast<int>(names.size()) - 1)];
    opponent.spec = isRival ? player_->spec : static_cast<Spec>(RandomInt(0, 2));
    opponent.style = isRival ? CounterStyle(player_->style) : static_cast<Style>(RandomInt(0, 2));
    opponent.level = isRival ? player_->level + 1 : std::max(1, player_->level + RandomInt(-1, 1));

    // Edit these formulas to change generated opponent durability and focus.
    opponent.maxHp = 85 + opponent.level * 15;
    opponent.hp = opponent.maxHp;
    opponent.maxFocus = 42 + opponent.level * 8;
    opponent.focus = opponent.maxFocus;
    opponent.skills = BuildOpponentSkills(opponent.spec, opponent.level >= 3 ? 2 : 1);
    return opponent;
}

std::vector<SkillProgress> Game::BuildOpponentSkills(Spec spec, int extraSkills) const
{
    const std::vector<std::string>& skillIds = data_.GetSpec(spec).skillIds;
    const int count = std::min(static_cast<int>(skillIds.size()), extraSkills + 1);

    std::vector<SkillProgress> skills;
    for (int index = 0; index < count; ++index)
    {
        skills.push_back({ skillIds[index] });
    }

    return skills;
}

void Game::AwardVictory(const Opponent& opponent)
{
    const int xp = Balance::PlayerXpPerOpponentLevel * opponent.level;
    const int currency = Balance::CurrencyPerOpponentLevel * opponent.level;

    AwardPlayerXp(xp);
    player_->currency += currency;
    player_->rankPoints += Balance::RankPointsPerWin;
    UpdateRankTier();

    std::cout
        << "Rewards: +" << xp << " XP, +" << currency
        << " currency, +" << Balance::RankPointsPerWin << " RP.\n";
}

void Game::AwardPlayerXp(int xp)
{
    player_->xp += xp;
    while (player_->xp >= player_->xpToNextLevel)
    {
        player_->xp -= player_->xpToNextLevel;
        ++player_->level;

        // Edit this formula to change how quickly later levels require more XP.
        player_->xpToNextLevel = 100 + (player_->level - 1) * 50;
        player_->maxHp += Balance::PlayerLevelUpHp;
        player_->maxFocus += Balance::PlayerLevelUpFocus;
        player_->hp += Balance::PlayerLevelUpHp;
        player_->focus += Balance::PlayerLevelUpFocus;

        std::cout << "Level up! You are now level " << player_->level << ".\n";
    }
}

void Game::UpdateRankTier()
{
    player_->rankTier = CalculateRankTier(player_->rankPoints);
}

int Game::RandomInt(int minimum, int maximum)
{
    std::uniform_int_distribution<int> roll(minimum, maximum);
    return roll(randomEngine_);
}

Style Game::CounterStyle(Style style)
{
    // Edit this switch if you want to change which style a rival chooses.
    switch (style)
    {
    case Style::Aggressive: return Style::Defensive;
    case Style::Defensive: return Style::Balanced;
    case Style::Balanced: return Style::Aggressive;
    }

    return Style::Balanced;
}

RankTier Game::CalculateRankTier(int rankPoints)
{
    // Edit these thresholds to rebalance the ranked ladder.
    if (rankPoints >= 1800) return RankTier::Challenger;
    if (rankPoints >= 1200) return RankTier::Master;
    if (rankPoints >= 800) return RankTier::Diamond;
    if (rankPoints >= 500) return RankTier::Platinum;
    if (rankPoints >= 250) return RankTier::Gold;
    if (rankPoints >= 100) return RankTier::Silver;
    return RankTier::Bronze;
}

bool Game::RequirePlayer() const
{
    if (player_.has_value())
    {
        return true;
    }

    std::cout << "Create a competitor first.\n";
    return false;
}
