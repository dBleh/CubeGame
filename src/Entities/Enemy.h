#ifndef ENEMY_H
#define ENEMY_H

#include <SFML/Graphics.hpp>
#include "../Utils/Config.h"

struct Enemy {
    
    sf::RectangleShape shape;
    float x, y;
    float renderedX, renderedY;
    float lastSentX, lastSentY; // Last position sent over network
    float velocityX, velocityY;
    float lastX;
    float lastY;
    float interpolationTime;
    int health;
    uint64_t id;
    float spawnDelay;

    float splitTimer = 0.f;
    float splitInterval = 3.0f;    // No longer const
    int splitCount = 0;
    int maxSplits = 3;            // No longer const
    bool isSplitting = false;
    float shakeTimer = 0.f;
    float shakeDuration = 0.5f;

    enum Type { Swarmlet, Sniper, Bomber, Brute, GravityWell, Default, Splitter } type;
    float attackCooldown; // For Sniper shooting
    bool exploded;        // For Bomber explosion tracking
    float pullRadius;     // For GravityWell

    void initialize(Type t = Default);
    bool move(float dt, float targetX, float targetY);
    void update(float dt);
    void updateSplitter(float dt);
};

#endif