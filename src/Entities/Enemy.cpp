#include "Enemy.h"
#include <cmath>
#include <iostream>

/**
 * @brief Initializes enemy properties based on its type.
 *
 * Depending on the enemy type, set size, color, health, initial position, and
 * specific timers for behaviors such as splitting or shaking.
 *
 * @param t The type of enemy to initialize.
 */
void Enemy::initialize(Type t) {
    type = t;
    spawnDelay = 1.5f;      // Delay before activation
    velocityX = 0.0f;
    velocityY = 0.0f;
    exploded = false;
    attackCooldown = 0.0f;
    shouldStopMoving = false;

    // Set properties based on enemy type.
    switch (type) {
        case Splitter:
            size = sf::Vector2f(30.0f, 30.0f);
            color = sf::Color::Cyan;
            health = 20;
            // Spawn at a random location within the screen.
            x = static_cast<float>(rand() % SCREEN_WIDTH);
            y = static_cast<float>(rand() % SCREEN_HEIGHT);
            splitTimer = splitInterval = 10.0f; // Time until splitting starts.
            shakeTimer = 0.f;
            shakeDuration = 2.0f;
            break;
        case Default:
        default:
            size = sf::Vector2f(20.0f, 20.0f);
            color = sf::Color::Red;
            health = 10;
            // Spawn at a random location within the screen.
            x = static_cast<float>(rand() % SCREEN_WIDTH);
            y = static_cast<float>(rand() % SCREEN_HEIGHT);
            break;
    }
    // Set initial rendered and last positions.
    renderedX = x;
    renderedY = y;
    lastX = x;
    lastY = y;
    interpolationTime = 0.f;
}

/**
 * @brief Updates the enemy state each frame.
 *
 * Handles the spawn delay, interpolation of position, and if applicable,
 * calls the splitter-specific update.
 *
 * @param dt Delta time (time elapsed since the last update).
 */
void Enemy::update(float dt) {
    // Process spawn delay.
    if (spawnDelay > 0) {
        spawnDelay -= dt;
        if (spawnDelay <= 0) {
            spawnDelay = 0;
            // Reset rendered position once the enemy spawns.
            renderedX = x;
            renderedY = y;
        }
    }
    // If enemy is alive, update movement interpolation.
    if (health > 0) {
        if (type == Splitter) {
            updateSplitter(dt);
        }
        float interpFactor = std::min(30.0f * dt, 1.0f);
        renderedX += (x - renderedX) * interpFactor;
        renderedY += (y - renderedY) * interpFactor;
    }

}

/**
 * @brief Special update routine for Splitter enemies.
 *
 * Handles the shake effect prior to splitting by randomly offsetting the shape's position.
 *
 * @param dt Delta time (time elapsed since the last update).
 */
void Enemy::updateSplitter(float dt) {
    splitTimer -= dt;

    // Start shaking when the split timer nears the shake duration.
    if (splitTimer <= shakeDuration && !isSplitting) {
        isSplitting = true;
        shakeTimer = shakeDuration;
        shouldStopMoving = true; // Stop movement during shaking
    }

    // Process shaking effect.
    if (isSplitting) {
        shakeTimer -= dt;
        if (shakeTimer > 0) {
            float shakeX = (rand() % 10 - 5) * (shakeTimer / shakeDuration);
            float shakeY = (rand() % 10 - 5) * (shakeTimer / shakeDuration);
            shape.setPosition(renderedX + shakeX, renderedY + shakeY);
        } else {
            isSplitting = false;
            shouldStopMoving = false; // Reset after splitting (though this enemy will be removed)
            splitTimer = 0.f; // Force split condition in EntityManager
        }
    }
}

/**
 * @brief Moves the enemy towards a target position.
 *
 * Calculates movement based on enemy type, and only moves if the spawn delay has elapsed.
 *
 * @param dt Delta time (time elapsed since the last update).
 * @param targetX X-coordinate of the target.
 * @param targetY Y-coordinate of the target.
 * @return True if the enemy changed direction significantly, false otherwise.
 */
bool Enemy::move(float dt, float targetX, float targetY) {
    if (spawnDelay > 0 || shouldStopMoving) return false; // Do not move if still in spawn delay.

    float dx = targetX - x;
    float dy = targetY - y;
    float dist = std::sqrt(dx * dx + dy * dy);
    float speed = 100.0f; // Default speed

    // Adjust speed based on enemy type.
    switch (type) {
        case Swarmlet:
            speed = 120.0f; // Fast mover
            break;
        case Sniper:
            speed = 40.0f;  // Slow mover; maintains range.
            if (dist < 300.0f) return false;
            break;
        case Bomber:
            speed = 80.0f;
            break;
        case Brute:
            speed = 75.0f;
            break;
        case GravityWell:
            speed = 40.0f;  // Slow mover due to gravity effects.
            break;
        case Default:
        default:
            speed = 50.0f;
            break;
    }

    if (dist > 0) {
        float step = speed * dt;
        if (step > dist) step = dist;
        float newX = x + (dx / dist) * step;
        float newY = y + (dy / dist) * step;

        // Determine if direction change is significant.
        bool directionChanged = (std::abs(newX - x) > 1.0f || std::abs(newY - y) > 1.0f);
        x = newX;
        y = newY;
        return directionChanged;
    }
    return false;
}

/**
 * @brief Retrieves the bounding rectangle of the enemy.
 *
 * Used for collision detection.
 *
 * @return sf::FloatRect representing the enemy's bounds.
 */
sf::FloatRect Enemy::getBounds() const {
    return sf::FloatRect(renderedX, renderedY, size.x, size.y);
}

/**
 * @brief Calculates a separation vector to push this enemy away from another.
 *
 * @param other The other enemy to separate from.
 * @return sf::Vector2f The separation vector.
 */
sf::Vector2f Enemy::calculateSeparation(const Enemy& other) const {
    float dx = renderedX - other.renderedX;
    float dy = renderedY - other.renderedY;
    float distance = std::sqrt(dx * dx + dy * dy);
    float minDistance = (size.x + other.size.x) * 0.5f; // Minimum distance to avoid overlap

    if (distance < minDistance && distance > 0) {
        float strength = (minDistance - distance) / minDistance; // Stronger push when closer
        return sf::Vector2f(dx / distance * strength, dy / distance * strength);
    }
    return sf::Vector2f(0.f, 0.f); // No separation needed
}
