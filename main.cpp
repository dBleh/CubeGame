#include "CubeGame.h"
#include <iostream>

int main(int argc, char* argv[]) {
    CubeGame game;
    if (argc > 1 && std::string(argv[1]) == "--debug") {
        game.SetDebugMode(true);
    }
    game.Run();
    return 0;
}