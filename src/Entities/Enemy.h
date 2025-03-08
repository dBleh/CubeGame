#ifndef ENEMY_H
#define ENEMY_H

#include <SFML/Graphics.hpp>
#include "../Utils/Config.h"

/**
 * @brief Represents an enemy entity in the game.
 *
 * The Enemy struct holds data for position, movement, health, and special
 * behaviors such as splitting. It also stores graphical properties for rendering.
 */
struct Enemy {
    // --- Graphical and Rendering Data ---
    sf::RectangleShape shape;    // Shape used for rendering (can be replaced with color batching)
    sf::Color color;             // Color used for batching rendering
    sf::Vector2f size;           // Dimensions of the enemy

    // --- Position and Movement Data ---
    float x, y;                  // Logical position
    float renderedX, renderedY;  // Interpolated (rendered) position
    float lastSentX, lastSentY;  // Last position sent over the network
    float velocityX, velocityY;  // Current velocity components
    float lastX, lastY;          // Previous position (for interpolation)
    float interpolationTime;     // Time accumulator for interpolation

    // --- Gameplay Properties ---
    int health;                  // Health value of the enemy
    uint64_t id;                 // Unique identifier
    float spawnDelay;            // Delay before the enemy becomes active

    // --- Splitting/Shake Mechanics ---
    float splitTimer = 0.f;      // Timer for when to split or trigger shake effect
    float splitInterval = 3.0f;  // Interval between potential splits (adjustable)
    int splitCount = 0;          // Number of splits already performed
    int maxSplits = 3;           // Maximum number of splits allowed
    bool isSplitting = false;    // Flag to indicate if a splitting animation is active
    float shakeTimer = 0.f;      // Timer for shake effect duration
    float shakeDuration = 0.5f;  // Total duration for shake effect
    bool shouldStopMoving = false; // Flag to stop movement after splitting

    // --- Enemy Behavior Specifics ---
    enum Type { Swarmlet, Sniper, Bomber, Brute, GravityWell, Default, Splitter } type;
    float attackCooldown;        // Cooldown for special attacks (e.g., Sniper shooting)
    bool exploded;               // For tracking explosion state (e.g., Bomber)
    float pullRadius;            // Effective radius for gravitational pull (GravityWell)

    // --- Member Functions ---
    void initialize(Type t = Default);  // Initialize enemy properties based on type
    bool move(float dt, float targetX, float targetY); // Move enemy toward a target position
    void update(float dt);              // Update enemy state (position, interpolation, etc.)
    void updateSplitter(float dt);      // Special update routine for Splitter type enemies
    sf::FloatRect getBounds() const;    // Get the enemy's bounding box for collision detection
    sf::Vector2f calculateSeparation(const Enemy& other) const; // Calculate separation vector
};

#endif
