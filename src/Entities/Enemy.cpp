#include "Enemy.h"
#include <cmath>
#include <iostream>

void Enemy::initialize(Type t) {
    type = t;
    spawnDelay = 1.5f;
    velocityX = 0.0f;
    velocityY = 0.0f;
    exploded = false;
    attackCooldown = 0.0f;

    switch (type) {
        case Swarmlet:
            shape.setSize(sf::Vector2f(10.0f, 10.0f));
            shape.setFillColor(sf::Color::Green);
            health = 10;
            x = static_cast<float>(rand() % SCREEN_WIDTH);
            y = static_cast<float>(rand() % SCREEN_HEIGHT);
            break;
        case Sniper:
            shape.setSize(sf::Vector2f(15.0f, 15.0f));
            shape.setFillColor(sf::Color::Magenta);
            health = 40;
            x = static_cast<float>(rand() % SCREEN_WIDTH);
            y = static_cast<float>(rand() % SCREEN_HEIGHT);
            attackCooldown = 3.0f; // Fires every 3s
            break;
        case Bomber:
            shape.setSize(sf::Vector2f(20.0f, 20.0f));
            shape.setFillColor(sf::Color::Yellow);
            health = 30;
            x = static_cast<float>(rand() % SCREEN_WIDTH);
            y = static_cast<float>(rand() % SCREEN_HEIGHT);
            break;
        case Brute:
            shape.setSize(sf::Vector2f(30.0f, 30.0f));
            shape.setFillColor(sf::Color(139, 0, 0)); // Dark red
            health = 100;
            x = static_cast<float>(rand() % SCREEN_WIDTH);
            y = static_cast<float>(rand() % SCREEN_HEIGHT);
            break;
        case GravityWell:
            shape.setSize(sf::Vector2f(30.0f, 30.0f));
            shape.setFillColor(sf::Color::Black);
            health = 80;
            x = static_cast<float>(rand() % SCREEN_WIDTH);
            y = static_cast<float>(rand() % SCREEN_HEIGHT);
            pullRadius = 100.0f;
            break;
        case Default: // Original enemy
        default:
            shape.setSize(sf::Vector2f(20.0f, 20.0f));
            shape.setFillColor(sf::Color::Red);
            health = 50;
            x = static_cast<float>(rand() % SCREEN_WIDTH);
            y = static_cast<float>(rand() % SCREEN_HEIGHT);
            break;
    }
    renderedX = x;
    renderedY = y;
    shape.setPosition(x, y);
}

bool Enemy::move(float dt, float targetX, float targetY) {
    if (spawnDelay > 0) return false;

    float dx = targetX - x;
    float dy = targetY - y;
    float dist = std::sqrt(dx * dx + dy * dy);
    float speed = 100.0f; // Default speed

    switch (type) {
        case Swarmlet:
            speed = 120.0f; // Fast
            break;
        case Sniper:
            speed = 40.0f; // Slow
            if (dist < 300.0f) return false; // Stays at range
            break;
        case Bomber:
            speed = 80.0f;
            break;
        case Brute:
            speed = 75.0f;
            break;
        case GravityWell:
            speed = 40.0f; // Slow mover
            break;
        case Default:
        default:
            speed = 100.0f;
            break;
    }

    if (dist > 0) {
        float step = speed * dt;
        if (step > dist) step = dist;
        float newX = x + (dx / dist) * step;
        float newY = y + (dy / dist) * step;

        bool directionChanged = (std::abs(newX - x) > 1.0f || std::abs(newY - y) > 1.0f);
        x = newX;
        y = newY;
        return directionChanged;
    }
    return false;
}

void Enemy::update(float dt) {
    if (spawnDelay > 0) {
        spawnDelay -= dt;
        if (spawnDelay <= 0) {
            spawnDelay = 0;
            renderedX = x;
            renderedY = y;
        }
    }

    if (health > 0) {
        float interpFactor = std::min(30.0f * dt, 1.0f);
        float oldRenderedX = renderedX, oldRenderedY = renderedY;
        renderedX += (x - renderedX) * interpFactor;
        renderedY += (y - renderedY) * interpFactor;
        shape.setPosition(renderedX, renderedY);

        float dx = x - renderedX;
        float dy = y - renderedY;
        if (std::abs(dx) > 10 || std::abs(dy) > 10) {
            std::cout << "[DEBUG] Enemy jump: (" << dx << ", " << dy << ") from (" << oldRenderedX << ", " << oldRenderedY << ") to (" << renderedX << ", " << renderedY << ")" << std::endl;
        }
    } else {
        shape.setPosition(renderedX, renderedY);
    }

    // Sniper attack cooldown
    if (type == Sniper && attackCooldown > 0) {
        attackCooldown -= dt;
    }
}