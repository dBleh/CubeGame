#include "Enemy.h"
#include <cmath>

void Enemy::initialize() {
    shape.setSize(sf::Vector2f(20.0f, 20.0f));
    shape.setFillColor(sf::Color::Red);
    x = static_cast<float>(rand() % SCREEN_WIDTH); // Will be overridden in SpawnEnemies
    y = static_cast<float>(rand() % SCREEN_HEIGHT);
    renderedX = x;
    renderedY = y;
    velocityX = 0.0f;
    velocityY = 0.0f;
    spawnDelay = 1.5f; // Initial delay
}

bool Enemy::move(float dt, float targetX, float targetY) {
    if (spawnDelay > 0) return false; // Don’t move until delay expires

    float dx = targetX - x;
    float dy = targetY - y;
    float dist = std::sqrt(dx * dx + dy * dy);

    if (dist > 0) {
        float speed = 100.0f;
        float step = speed * dt;

        if (step > dist) {
            step = dist;
        }

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
        spawnDelay -= dt; // Decrease delay
        if (spawnDelay < 0) spawnDelay = 0;
    } else {
        x += velocityX * dt;
        y += velocityY * dt;
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
}