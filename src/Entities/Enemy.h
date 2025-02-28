#ifndef ENEMY_H
#define ENEMY_H

#include <SFML/Graphics.hpp>
#include "../Utils/Config.h"
#include <iostream>
struct Enemy {
    sf::RectangleShape shape;
    float x, y;
    float renderedX, renderedY;
    float velocityX = 0.0f;
    float velocityY = 0.0f;
    int health = 50;
    uint64_t id;
    float spawnDelay = 1.5f; // New field: delay before moving

    void initialize();
    bool move(float dt, float targetX, float targetY);
    void update(float dt);
};

#endif // ENEMY_H