#ifndef PLAYER_H
#define PLAYER_H

#include <SFML/Graphics.hpp>
#include "../Utils/Config.h"
#include <steam/steam_api.h>
struct Player {
    sf::RectangleShape shape;
    float x, y;           // Target position from network or local movement
    float renderedX, renderedY; // Smoothed position for rendering
    int health = 100;
    CSteamID steamID;
    bool ready = false;
    bool isAlive = true;
    int kills = 0;
    void initialize();
    bool move(float dt);
};
#endif // PLAYER_H