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

class NetworkManager {
public:
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
    
    // Message handlers
    void HandlePlayerLoaded(const std::string& msg);
    void HandlePlayerUpdate(const std::string& msg);
    void HandleEnemySpawn(const std::string& msg);
    void HandleEnemyUpdate(const std::string& msg);
    void HandleEnemyDeath(const std::string& msg);  // Handler for explicit death
    void HandleEnemySync(const std::string& msg);   // New handler for full enemy sync
    void HandleBulletFire(const std::string& msg, CSteamID sender);
    void HandleHit(const std::string& msg, CSteamID sender);
    void HandleStart(const std::string& msg);
    void HandleNextLevel(const std::string& msg);
    void HandleTimer(const std::string& msg);
    void HandlePlay(const std::string& msg);
    void HandleGameOver(const std::string& msg);
    void HandleLobbyReturn(const std::string& msg);
    
    void ReportNetworkUsage() const;
    void ResetNetworkUsage();
    
    // Network/game functions
    void ProcessNetworkMessages(const std::string& msg, CSteamID sender);
    void SendGameplayMessage(const std::string& msg);
    void SendPlayerUpdate();
    void SyncEnemies();
    void SyncEnemiesFull();                         // New function for full enemy sync
    void BroadcastEnemySpawns();
    void SpawnEnemiesAndBroadcast();
    void ThrottledSendPlayerUpdate();
    void BroadcastEnemyDeath(uint64_t enemyId, CSteamID killerID);
    void HandleEnemyRemove(const std::string& msg);
private:
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