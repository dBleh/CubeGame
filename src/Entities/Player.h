#ifndef PLAYER_H
#define PLAYER_H

#include <SFML/Graphics.hpp>
#include "../Utils/Config.h"
#include <steam/steam_api.h>
#include <iostream>

/**
 * @brief Represents a player in the game.
 *
 * Stores graphical properties, positional data, health, and game-related stats.
 * Provides methods for initialization, movement, applying boosts, and shooting.
 */
struct Player {

    struct OrbitingCube {
        float x, y;              // Current position
        float renderedX, renderedY; // Rendered (interpolated) position
        float lastX, lastY;      // Previous position for interpolation
        float radius;            // Distance from player
        float angle;             // Current angle in radians
        float angularSpeed;      // Speed of rotation (radians per second)
        sf::RectangleShape shape;// Visual representation
        bool active;             // Whether the cube is active
        

        OrbitingCube() : x(0), y(0), renderedX(0), renderedY(0), lastX(0), lastY(0),
                         radius(50.f), angle(0.f), angularSpeed(2.f), active(false) {
            shape.setSize(sf::Vector2f(10.f, 10.f));
            shape.setFillColor(sf::Color::Green);
        }
    };

    uint64_t startTimestamp = 0; // Time when orbiting began (in milliseconds)
    uint64_t lastUpdateTimestamp = 0;

    OrbitingCube orbitingCube;
    //-------------------------------------------------------------------------
    // Graphical Data
    //-------------------------------------------------------------------------
    sf::RectangleShape shape; ///< Visual representation of the player.

    //-------------------------------------------------------------------------
    // Positional Data
    //-------------------------------------------------------------------------
    float x, y;                   ///< Logical position.
    float renderedX, renderedY;   ///< Interpolated position for rendering.
    float lastX, lastY;           ///< Previous position for interpolation.
    float targetX, targetY;       ///< Target position for interpolation.
    float interpolationTime;      ///< Elapsed time used for interpolation.
    float velocityX = 0.0f;  // Velocity in X direction
    float velocityY = 0.0f;
    static const float INTERPOLATION_TIME; // Define interpolation duration
    //-------------------------------------------------------------------------
    // Gameplay Properties
    //-------------------------------------------------------------------------
    int health = PLAYER_HEALTH;   ///< Player health.
    CSteamID steamID;             ///< Unique Steam ID.
    bool ready = false;           ///< Ready status in lobby.
    bool isAlive = true;          ///< Alive flag.
    int kills = 0;                ///< Kill count.
    int money;                    ///< In-game currency.
    float speed;                  ///< Movement speed.

    //-------------------------------------------------------------------------
    // Member Functions
    //-------------------------------------------------------------------------
    void initialize();                    ///< Set default values.
    bool move(float dt);                  ///< Process movement input.
    void applySpeedBoost(float boostAmount); ///< Apply a temporary speed boost.
    void ShootBullet(class CubeGame* game);   ///< Fire a bullet (requires CubeGame context).
    void updateOrbitingCube(float dt); // New method to update cube position
    sf::FloatRect getOrbitingCubeBounds() const; // For collision detection
};

#endif // PLAYER_H