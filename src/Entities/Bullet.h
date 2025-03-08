#ifndef BULLET_H
#define BULLET_H

#include <SFML/Graphics.hpp>
#include "../Utils/Config.h" // Ensure BULLET_SPEED is defined here
#include <iostream>

/**
 * @brief Represents a bullet projectile.
 *
 * The Bullet struct holds position, movement, and lifetime data for a bullet,
 * along with its graphical representation.
 */
struct Bullet {
    // --- Graphical Data ---
    sf::RectangleShape shape;  // Visual representation of the bullet

    // --- Position and Movement Data ---
    float x, y;                // Logical position
    float renderedX, renderedY;// Interpolated (rendered) position
    float velocityX, velocityY;// Velocity components
    float lastX, lastY;        // Last position (useful for interpolation)
    float targetX, targetY;    // Target position for interpolation

    // --- Lifetime and Identification ---
    float lifetime;            // Time remaining before the bullet expires
    uint64_t id;               // Unique identifier for the bullet

    // --- Member Functions ---
    void initialize(float startX, float startY, float targetX, float targetY);
    void update(float dt);
};

#endif // BULLET_H
