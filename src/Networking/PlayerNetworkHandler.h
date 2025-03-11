#ifndef PLAYERNETWORKHANDLER_H
#define PLAYERNETWORKHANDLER_H

#include <steam/steam_api.h>
#include <string>
#include <sstream>
#include <unordered_map>
#include <SFML/Graphics.hpp>
#include "../Utils/SteamHelpers.h"
#include <chrono>

class CubeGame;
class NetworkManager;

class PlayerNetworkHandler {
public:
    PlayerNetworkHandler(CubeGame* gameInstance, NetworkManager* networkMgr);
    
    void HandlePlayerLoaded(const std::string& msg);
    void HandlePlayerUpdate(const std::string& msg);
    void SendPlayerUpdate();
    void ThrottledSendPlayerUpdate();

private:
    CubeGame* game;
    NetworkManager* networkManager;
    sf::Clock m_playerUpdateClock;
    
    struct PlayerState {
        float lastX = 0.f;
        float lastY = 0.f;
        float targetX = 0.f;
        float targetY = 0.f;
        float interpolationTime = 0.f;
        sf::Clock interpolationClock;
    };
    
    std::unordered_map<CSteamID, PlayerState, CSteamIDHash> m_playerStates;
    const float INTERPOLATION_TIME = 0.1f;
};

#endif // PLAYERNETWORKHANDLER_H