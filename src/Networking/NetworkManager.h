#ifndef NETWORKMANAGER_H
#define NETWORKMANAGER_H

#include <steam/steam_api.h>
#include <steam/isteamnetworking.h>
#include <SFML/Graphics.hpp>
#include <string>
#include <sstream>
#include <unordered_map>
#include <functional>
#include "../Utils/SteamHelpers.h"
#include <vector>
#include <chrono>

class CubeGame;
class EntityManager;
class PlayerNetworkHandler;
class EnemyNetworkHandler;

class NetworkManager {
public:
    friend class EnemyNetworkHandler;
    NetworkManager(bool debugMode = false, CubeGame* gameInstance = nullptr);
    ~NetworkManager();
    void JoinLobbyFromNetwork(CSteamID lobby);
    bool isInitialized() const;
    bool isLoaded();
    bool sendMessage(CSteamID target, const std::string &msg);
    bool broadcastMessage(const std::string &msg);
    void processCallbacks();
    void receiveMessages();
    void setMessageHandler(std::function<void(const std::string&, CSteamID)> handler);
    void acceptSession(CSteamID remoteID);
    const std::unordered_map<CSteamID, bool, CSteamIDHash>& getConnectedClients() const;
    bool AcceptP2PSessionWithUser(CSteamID user);
    void setIsConnectedToHost(bool b);

    void HandleBulletFire(const std::string& msg, CSteamID sender);
    void HandleHit(const std::string& msg, CSteamID sender);

    void ReportNetworkUsage() const;
    void ResetNetworkUsage();

    // Network/game functions
    void ProcessNetworkMessages(const std::string& msg, CSteamID sender);
    void SendGameplayMessage(const std::string& msg);
    void SendPlayerUpdate();
    void SyncEnemies();
    void SyncEnemiesFull();
    void SpawnEnemiesAndBroadcast();

    void BroadcastEnemyDeath(uint64_t enemyId, CSteamID killerID);

    void HandleCollisionsAndSync(float dt, CubeGame* game);
    void syncTimer(float timerValue);           // Sync next level timer
    void broadcastGameOver();                   // Signal game over
    void syncLevelTransition(float duration);
    void syncEntities(EntityManager* em); // Sync flagged entities
    
    PlayerNetworkHandler* playerHandler;
    EnemyNetworkHandler* enemyHandler;

private:
    std::map<CSteamID, uint64_t> m_lastPlayerUpdateTime;
    struct NetworkStats {
        size_t bytesSent = 0;
        size_t bytesReceived = 0;
        size_t messageCountSent = 0;
        size_t messageCountReceived = 0;
    };

    struct PlayerState {
        float lastX = 0.f;
        float lastY = 0.f;
        float targetX = 0.f;
        float targetY = 0.f;
        float interpolationTime = 0.f;
        sf::Clock interpolationClock;
    };

    sf::Clock m_playerUpdateClock;
    bool isConnectedToHost;
    ISteamNetworking* m_networking;
    bool debugMode;
    std::unordered_map<CSteamID, bool, CSteamIDHash> m_connectedClients;
    std::unordered_map<CSteamID, PlayerState, CSteamIDHash> m_playerStates;
    std::unordered_map<uint64_t, uint64_t> m_lastEnemyUpdateTime;  // enemyID -> timestamp
    uint64_t m_lastEnemySyncTime = 0;                              // New member for last sync timestamp
    std::function<void(const std::string&, CSteamID)> messageHandler;
    CubeGame* game;
    std::map<std::string, NetworkStats> networkUsage;
    sf::Clock usageClock;
    float usageReportInterval = 10.0f;
    const float INTERPOLATION_TIME = 0.1f;

    STEAM_CALLBACK(NetworkManager, OnLobbyCreated, LobbyCreated_t, m_cbLobbyCreated);
    STEAM_CALLBACK(NetworkManager, OnGameLobbyJoinRequested, GameLobbyJoinRequested_t, m_cbGameLobbyJoinRequested);
    STEAM_CALLBACK(NetworkManager, OnLobbyEnter, LobbyEnter_t, m_cbLobbyEnter);
    STEAM_CALLBACK(NetworkManager, OnP2PSessionRequest, P2PSessionRequest_t, m_cbP2PSessionRequest);
    STEAM_CALLBACK(NetworkManager, OnP2PSessionConnectFail, P2PSessionConnectFail_t, m_cbP2PSessionConnectFail);
    STEAM_CALLBACK(NetworkManager, OnLobbyMatchList, LobbyMatchList_t, m_cbLobbyMatchList);
};

#endif // NETWORKMANAGER_H