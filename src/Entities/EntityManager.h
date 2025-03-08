#ifndef ENTITYMANAGER_H
#define ENTITYMANAGER_H

#include <unordered_map>
#include <functional>
#include "Player.h"
#include "Bullet.h"
#include "Enemy.h"
#include <steam/steam_api.h>
#include "../Utils/SteamHelpers.h"
#include "../Utils/Config.h"
#include <chrono>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// Forward declaration of CubeGame.
class CubeGame;

/**
 * @brief Manages game entities including players, bullets, and enemies.
 *
 * Maintains entity containers and a spatial collision grid for efficient
 * collision detection and updates. Provides functions for updating, spawning,
 * and interpolating entities.
 */
class EntityManager {
public:
    //-------------------------------------------------------------------------
    // Constructors & Destructor
    //-------------------------------------------------------------------------
    EntityManager();
    ~EntityManager();

    //-------------------------------------------------------------------------
    // Accessor Methods
    //-------------------------------------------------------------------------
    std::unordered_map<CSteamID, Player, CSteamIDHash>& getPlayers(); ///< Returns reference to the players map.
    std::unordered_map<uint64_t, Bullet>& getBullets();                 ///< Returns reference to the bullets map.
    std::unordered_map<uint64_t, Enemy>& getEnemies();                    ///< Returns reference to the enemies map.
    Player& getLocalPlayer(CubeGame* game);                               ///< Returns the local player.

    //-------------------------------------------------------------------------
    // Update & Spawn Methods
    //-------------------------------------------------------------------------
    void updateEntities(float dt); ///< Updates players, bullets, and enemies.
    void spawnEnemies(int enemiesPerWave, const std::unordered_map<CSteamID, Player, CSteamIDHash>& players, uint64_t hostID); ///< Spawns a new wave of enemies.

    //-------------------------------------------------------------------------
    // Collision Detection
    //-------------------------------------------------------------------------
    void checkCollisions(
        std::function<void(const Bullet&, uint64_t enemyId)> onBulletEnemyCollision, ///< Callback for bullet-enemy collision.
        std::function<void(CSteamID playerId, uint64_t enemyId)> onEnemyPlayerCollision  ///< Callback for enemy-player collision.
    );

    //-------------------------------------------------------------------------
    // Callback & Interpolation Methods
    //-------------------------------------------------------------------------
    void setEnemyUpdateCallback(std::function<void(const std::string&)> callback); ///< Sets the enemy update callback.
    bool areEntitiesInitialized() const; ///< Returns true if there is at least one player.
    void interpolateEntities(float dt);  ///< Interpolates positions for smooth rendering.

private:
    //-------------------------------------------------------------------------
    // Spatial Collision Grid
    //-------------------------------------------------------------------------
    struct GridCell {
        std::vector<uint64_t> enemyIds;   ///< IDs of enemies in this cell.
        std::vector<uint64_t> bulletIds;  ///< IDs of bullets in this cell.
    };

    void updateCollisionGrid(); ///< Updates the collision grid based on current entity positions.
    
    // The collision grid is keyed by a computed value: (x/100)*1000 + (y/100)
    std::unordered_map<int, GridCell> collisionGrid;

    //-------------------------------------------------------------------------
    // Private Data Members
    //-------------------------------------------------------------------------
    float enemyUpdateInterval = 0.5f; ///< Interval for enemy network updates.
    std::unordered_map<CSteamID, Player, CSteamIDHash> m_players; ///< Container for players.
    std::unordered_map<uint64_t, Bullet> m_bullets;                 ///< Container for bullets.
    std::unordered_map<uint64_t, Enemy> m_enemies;                    ///< Container for enemies.
    float lastEnemyUpdateTime;                                      ///< Accumulator for enemy updates.
    std::function<void(const std::string&)> onEnemyUpdate;          ///< Callback for enemy update messages.
};

#endif // ENTITYMANAGER_H
