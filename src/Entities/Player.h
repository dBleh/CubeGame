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
};

#endif // PLAYER_H
