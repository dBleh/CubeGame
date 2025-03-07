#ifndef PLAYER_H
#define PLAYER_H

#include <SFML/Graphics.hpp>
#include "../Utils/Config.h"
#include <steam/steam_api.h>
#include <iostream>

struct Player {
    sf::RectangleShape shape;
    float x, y;           // Target position from network or local movement
    float renderedX, renderedY; // Smoothed position for rendering
    float lastX, lastY;   // Previous position for interpolation
    float targetX, targetY; // Target position for interpolation
    float interpolationTime; // Time elapsed for interpolation
    int health = 100;
    CSteamID steamID;
    bool ready = false;
    bool isAlive = true;
    int kills = 0;
    int money;    // Currency for store
    float speed;

    void initialize();
    bool move(float dt);
    void applySpeedBoost(float boostAmount);
};

#endif // PLAYER_H