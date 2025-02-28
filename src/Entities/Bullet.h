#ifndef BULLET_H
#define BULLET_H

#include <SFML/Graphics.hpp>
#include "../Utils/Config.h" // Ensure BULLET_SPEED is defined here
#include <iostream>
struct Bullet {
    sf::RectangleShape shape;
    float x, y;
    float renderedX, renderedY;
    float velocityX, velocityY;
    float lifetime;
    int id; // New field for unique identification

    void initialize(float startX, float startY, float targetX, float targetY);
    void update(float dt);
};

#endif // BULLET_H