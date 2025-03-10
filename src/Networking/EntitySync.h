#ifndef ENTITY_SYNC_H
#define ENTITY_SYNC_H

#include "CubeGame.h"
#include "NetworkManager.h"
#include <unordered_map>
#include <chrono>

class EntitySync {
public:
    EntitySync(CubeGame* game, NetworkManager* networkManager);
    void SendPlayerUpdate();
    void ThrottledSendPlayerUpdate();
    void SpawnEnemiesAndBroadcast();
    void SyncEnemies();
    void SyncEnemiesFull();
    void BroadcastEnemyDeath(uint64_t enemyId, CSteamID killerID);
    void HandleCollisionsAndSync(float dt);

private:
    CubeGame* game;
    NetworkManager* networkManager;
    sf::Clock m_playerUpdateClock;
    std::unordered_map<uint64_t, uint64_t> m_lastEnemyUpdateTime;
    std::map<CSteamID, uint64_t> m_lastPlayerUpdateTime;
    struct PlayerState {
        float lastX = 0.f;
        float lastY = 0.f;
        float targetX = 0.f;
        float targetY = 0.f;
        float interpolationTime = 0.f;
        sf::Clock interpolationClock;
    };
    std::unordered_map<CSteamID, PlayerState, CSteamIDHash> m_playerStates;
    static constexpr float INTERPOLATION_TIME = 0.1f;

    void SendGameplayMessage(const std::string& msg);
};

#endif