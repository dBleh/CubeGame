#ifndef ENEMY_H
#define ENEMY_H

#include <SFML/Graphics.hpp>
#define SCREEN_WIDTH 1280
#define SCREEN_HEIGHT 720

struct Enemy {
    sf::RectangleShape shape;
    float x, y;
    float renderedX, renderedY;
    float velocityX, velocityY;
    int health;
    uint64_t id;
    float spawnDelay;

    enum Type { Default, Swarmlet, Sniper, Bomber, Brute, GravityWell } type;
    // Type-specific fields
    float attackCooldown; // For Sniper shooting
    bool exploded;        // For Bomber explosion tracking
    float pullRadius;     // For GravityWell

    void initialize(Type t = Default);
    bool move(float dt, float targetX, float targetY);
    void update(float dt);
};

#endif