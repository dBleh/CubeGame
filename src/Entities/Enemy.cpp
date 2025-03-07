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
        case Splitter:
            shape.setSize(sf::Vector2f(30.0f, 30.0f));
            shape.setFillColor(sf::Color::Cyan);
            health = 20;
            x = static_cast<float>(rand() % SCREEN_WIDTH);
            y = static_cast<float>(rand() % SCREEN_HEIGHT);
            splitTimer = splitInterval = 10.0f;
            shakeTimer = 0.f;
            shakeDuration = 2.0f;
            break;
        case Default:
        default:
            shape.setSize(sf::Vector2f(20.0f, 20.0f));
            shape.setFillColor(sf::Color::Red);
            health = 10;
            x = static_cast<float>(rand() % SCREEN_WIDTH);
            y = static_cast<float>(rand() % SCREEN_HEIGHT);
            break;
    }
    renderedX = x;
    renderedY = y;
    lastX = x;
    lastY = y;
    interpolationTime = 0.f;
    shape.setPosition(x, y);
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
        if (type == Splitter) {
            updateSplitter(dt);
        }

        float interpFactor = std::min(30.0f * dt, 1.0f);
        renderedX += (x - renderedX) * interpFactor;
        renderedY += (y - renderedY) * interpFactor;
        shape.setPosition(renderedX, renderedY);
    } else {
        shape.setPosition(renderedX, renderedY);
    }

    if (type == Sniper && attackCooldown > 0) {
        attackCooldown -= dt;
    }
}

void Enemy::updateSplitter(float dt) {
    splitTimer -= dt;

    if (splitTimer <= shakeDuration && !isSplitting) {
        isSplitting = true;
        shakeTimer = shakeDuration;
    }

    if (isSplitting) {
        shakeTimer -= dt;
        if (shakeTimer > 0) {
            float shakeX = (rand() % 10 - 5) * (shakeTimer / shakeDuration);
            float shakeY = (rand() % 10 - 5) * (shakeTimer / shakeDuration);
            shape.setPosition(renderedX + shakeX, renderedY + shakeY);
        } else {
            isSplitting = false;
            // Color change happens in EntityManager before destruction
        }
    }
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
