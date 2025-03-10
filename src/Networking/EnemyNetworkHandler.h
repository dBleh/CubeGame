#ifndef ENEMYNETWORKHANDLER_H
#define ENEMYNETWORKHANDLER_H

#include "CubeGame.h"
#include "NetworkManager.h"
#include "Enemy.h"
#include "steam/steam_api.h"
#include <string>

class EnemyNetworkHandler {
public:
    // Define static constant for enemy update interval
    static const float ENEMY_UPDATE_INTERVAL; // 0.5s for periodic updates

    // Constructor
    EnemyNetworkHandler(CubeGame* game);

    // Send an enemy spawn message
    void SendEnemySpawn(const Enemy& enemy);

    // Send an enemy update message (position, health, etc.)
    void SendEnemyUpdate(const Enemy& enemy);

    // Send an enemy death message
    void SendEnemyDeath(uint64_t enemyId, CSteamID killerID);

    // Send an enemy removal message
    void SendEnemyRemove(uint64_t enemyId);

    // Rate-limited enemy update (for periodic sync)
    void ThrottledSendEnemyUpdates(float dt);

    // Process incoming enemy-related messages
    void HandleEnemyMessage(const std::string& msg, CSteamID sender);

private:
    CubeGame* game;
    float lastEnemyUpdateTime = 0.0f; // Tracks time since last throttled update
};

#endif // ENEMYNETWORKHANDLER_H