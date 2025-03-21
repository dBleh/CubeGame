#include "Core/CubeGame.h"
#include <iostream>

int main() {
    
    std::cout << "[DEBUG] Starting CubeShooter..." << std::endl;
    CubeGame game;
    game.initialize();
    game.run();
   
    
    std::cout << "[DEBUG] Game loop exited normally." << std::endl;
    return 0;
}