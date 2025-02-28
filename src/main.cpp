#include "Core/CubeGame.h"
#include <iostream>

int main() {
    std::cout << "[DEBUG] Starting CubeShooter..." << std::endl;
    
    CubeGame game;
    std::cout << "[DEBUG] Game object created, calling Run()..." << std::endl;
    
    game.Run();
    
    std::cout << "[DEBUG] Game loop exited normally." << std::endl;
    return 0;
}
