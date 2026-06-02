#include "BalanceSimulator.h"

#include "BattleSystem.h"
#include "SimulationData.h"

#include <iomanip>
#include <iostream>

BalanceSimulator::BalanceSimulator(
    const SimulationData& data,
    BattleSystem& battleSystem,
    std::mt19937& randomEngine)
    : data_(data), battleSystem_(battleSystem), randomEngine_(randomEngine)
{
}

void BalanceSimulator::Run(int battlePairsPerMatchup)
{
    // Each pair swaps turn order. That prevents the first attacker from making
    // one spec look stronger merely because it always acts first.
    const std::vector<Spec>& specs = data_.GetGameType(GameType::LeagueOfLegends).specs;
    std::vector<std::vector<MatchupStats>> matrix(specs.size(), std::vector<MatchupStats>(specs.size()));

    for (int row = 0; row < static_cast<int>(specs.size()); ++row)
    {
        for (int column = 0; column < static_cast<int>(specs.size()); ++column)
        {
            MatchupStats& stats = matrix[row][column];
            for (int pair = 0; pair < battlePairsPerMatchup; ++pair)
            {
                AutomatedBattleResult rowFirst = battleSystem_.RunAutomated(
                    CreatePlayer(specs[row], RandomStyle()),
                    CreateOpponent(specs[column], RandomStyle()));
                RecordResult(stats, true, rowFirst.playerWon, rowFirst.draw, rowFirst.turns);

                AutomatedBattleResult columnFirst = battleSystem_.RunAutomated(
                    CreatePlayer(specs[column], RandomStyle()),
                    CreateOpponent(specs[row], RandomStyle()));
                RecordResult(stats, false, columnFirst.playerWon, columnFirst.draw, columnFirst.turns);
            }
        }
    }

    std::cout
        << "\n=== Developer Balance Report ===\n"
        << "Each cell shows row-spec win rate across " << battlePairsPerMatchup * 2
        << " mirrored battles. Styles are randomized.\n"
        << "Expected: mirror cells near 50%; counter-spec cells above 50%.\n\n"
        << std::setw(10) << "Attacker";

    for (Spec spec : specs)
    {
        std::cout << std::setw(10) << ToString(spec);
    }
    std::cout << "\n";

    for (int row = 0; row < static_cast<int>(specs.size()); ++row)
    {
        std::cout << std::setw(10) << ToString(specs[row]);
        for (int column = 0; column < static_cast<int>(specs.size()); ++column)
        {
            const MatchupStats& stats = matrix[row][column];
            const int completedBattles = stats.wins + stats.losses;
            const double winRate = completedBattles == 0
                ? 0.0
                : 100.0 * stats.wins / completedBattles;
            std::cout << std::setw(9) << std::fixed << std::setprecision(1) << winRate << "%";
        }
        std::cout << "\n";
    }

    std::cout << "\nCounter-spec audit:\n";
    for (int row = 0; row < static_cast<int>(specs.size()); ++row)
    {
        const Spec target = data_.GetSpec(specs[row]).counteredSpec;
        int targetColumn = 0;
        while (specs[targetColumn] != target)
        {
            ++targetColumn;
        }

        const MatchupStats& stats = matrix[row][targetColumn];
        const int completedBattles = stats.wins + stats.losses;
        const double winRate = completedBattles == 0
            ? 0.0
            : 100.0 * stats.wins / completedBattles;
        const double averageTurns = (stats.wins + stats.losses + stats.draws) == 0
            ? 0.0
            : static_cast<double>(stats.totalTurns) / (stats.wins + stats.losses + stats.draws);

        std::cout
            << "- " << ToString(specs[row]) << " > " << ToString(target)
            << ": " << std::fixed << std::setprecision(1) << winRate
            << "% wins, " << averageTurns << " average turns, "
            << stats.draws << " draws\n";
    }

    // Styles now have different skills, so measure fixed loadouts independently
    // in neutral Top-vs-Top fights. Automated fighters do not switch styles.
    // This makes overtuned healing or burst visible, but it does not measure a
    // human player's ability to switch loadouts at a useful moment.
    const std::vector<Style> styles = {
        Style::Aggressive,
        Style::Defensive,
        Style::Balanced
    };
    std::vector<std::vector<MatchupStats>> styleMatrix(styles.size(), std::vector<MatchupStats>(styles.size()));

    for (int row = 0; row < static_cast<int>(styles.size()); ++row)
    {
        for (int column = 0; column < static_cast<int>(styles.size()); ++column)
        {
            MatchupStats& stats = styleMatrix[row][column];
            for (int pair = 0; pair < battlePairsPerMatchup; ++pair)
            {
                AutomatedBattleResult rowFirst = battleSystem_.RunAutomated(
                    CreatePlayer(Spec::Top, styles[row]),
                    CreateOpponent(Spec::Top, styles[column]));
                RecordResult(stats, true, rowFirst.playerWon, rowFirst.draw, rowFirst.turns);

                AutomatedBattleResult columnFirst = battleSystem_.RunAutomated(
                    CreatePlayer(Spec::Top, styles[column]),
                    CreateOpponent(Spec::Top, styles[row]));
                RecordResult(stats, false, columnFirst.playerWon, columnFirst.draw, columnFirst.turns);
            }
        }
    }

    std::cout
        << "\nStyle audit (neutral Top vs Top fights, fixed loadouts):\n"
        << std::setw(12) << "Style";
    for (Style style : styles)
    {
        std::cout << std::setw(12) << ToString(style);
    }
    std::cout << "\n";

    for (int row = 0; row < static_cast<int>(styles.size()); ++row)
    {
        std::cout << std::setw(12) << ToString(styles[row]);
        for (int column = 0; column < static_cast<int>(styles.size()); ++column)
        {
            const MatchupStats& stats = styleMatrix[row][column];
            const int completedBattles = stats.wins + stats.losses;
            const double winRate = completedBattles == 0
                ? 0.0
                : 100.0 * stats.wins / completedBattles;
            std::cout << std::setw(11) << std::fixed << std::setprecision(1) << winRate << "%";
        }
        std::cout << "\n";
    }
}

Player BalanceSimulator::CreatePlayer(Spec spec, Style style) const
{
    Player player;
    player.name = "Automated Player";
    player.spec = spec;
    player.style = style;
    player.basePower = Balance::StartingBasePower;
    player.knownSkills = CreateStarterSkills(spec);
    return player;
}

Opponent BalanceSimulator::CreateOpponent(Spec spec, Style style) const
{
    Opponent opponent;
    opponent.name = "Automated Opponent";
    opponent.spec = spec;
    opponent.style = style;
    opponent.basePower = Balance::StartingBasePower;
    opponent.skills = CreateStarterSkills(spec);
    return opponent;
}

std::vector<SkillProgress> BalanceSimulator::CreateStarterSkills(Spec spec) const
{
    const std::vector<std::string>& skillIds = data_.GetSpec(spec).skillIds;
    std::vector<SkillProgress> skills;
    for (int index = 0; index < 10; ++index)
    {
        skills.push_back({ skillIds[index] });
    }
    return skills;
}

Style BalanceSimulator::RandomStyle()
{
    std::uniform_int_distribution<int> roll(0, 2);
    return static_cast<Style>(roll(randomEngine_));
}

void BalanceSimulator::RecordResult(
    MatchupStats& stats,
    bool rowSpecWasFirst,
    bool firstCompetitorWon,
    bool draw,
    int turns) const
{
    stats.totalTurns += turns;
    if (draw)
    {
        ++stats.draws;
    }
    else if (firstCompetitorWon == rowSpecWasFirst)
    {
        ++stats.wins;
    }
    else
    {
        ++stats.losses;
    }
}
