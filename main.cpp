#include "Game.h"

#include <iostream>
#include <string>

int main(int argc, char* argv[])
{
    // SANDBOX UI ONLY:
    // Delete this console entry point when Godot becomes the executable frontend.
    Game game;

    // DEV BALANCE TOOL ONLY:
    // Run hundreds of quiet automated fights with:
    // Poke-clone-esports.exe --balance 250
    if (argc == 3 && std::string(argv[1]) == "--balance")
    {
        try
        {
            game.RunBalanceSimulation(std::stoi(argv[2]));
            return 0;
        }
        catch (const std::exception&)
        {
            std::cout << "Use a numeric battle-pair count, for example: --balance 250\n";
            return 1;
        }
    }

    game.Run();
    return 0;
}
