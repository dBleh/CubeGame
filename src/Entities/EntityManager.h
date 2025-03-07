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

// Forward declaration of CubeGame
class CubeGame;

class EntityManager {
public:
    EntityManager();
    ~EntityManager();

    std::unordered_map<CSteamID, Player, CSteamIDHash>& getPlayers();
    std::unordered_map<uint64_t, Bullet>& getBullets();
    std::unordered_map<uint64_t, Enemy>& getEnemies();

    // Updated method with forward-declared CubeGame
    Player& getLocalPlayer(CubeGame* game);

    void updateEntities(float dt);
    void spawnEnemies(int enemiesPerWave, const std::unordered_map<CSteamID, Player, CSteamIDHash>& players, uint64_t hostID);
    void checkCollisions(
        std::function<void(const Bullet&, uint64_t enemyId)> onBulletEnemyCollision,
        std::function<void(CSteamID playerId, uint64_t enemyId)> onEnemyPlayerCollision
    );

    void setEnemyUpdateCallback(std::function<void(const std::string&)> callback);
    bool areEntitiesInitialized() const;
    void interpolateEntities(float dt);
private:
float enemyUpdateInterval = 0.5f;
    std::unordered_map<CSteamID, Player, CSteamIDHash> m_players;
    std::unordered_map<uint64_t, Bullet> m_bullets;
    std::unordered_map<uint64_t, Enemy> m_enemies;
    float lastEnemyUpdateTime;
   
    std::function<void(const std::string&)> onEnemyUpdate; // Callback for sending enemy updates
};

#endif // ENTITYMANAGER_H