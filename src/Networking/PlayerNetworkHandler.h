#ifndef PLAYER_NETWORK_HANDLER_H
#define PLAYER_NETWORK_HANDLER_H

#include <string>

// Forward declarations to reduce dependencies
class CSteamID; // From Steam API
class CubeGame; // Main game class

class PlayerNetworkHandler {
public:
    // Constructor takes a CubeGame pointer for context
    explicit PlayerNetworkHandler(CubeGame* game);

    // Sending methods
    void SendPlayerUpdate();          // Send local player state update
    void ThrottledSendPlayerUpdate(float dt); // Add dt parameter
    
    // Receiving method
    void HandlePlayerUpdate(const std::string& msg, CSteamID sender); // Process incoming player messages

private:
    CubeGame* game;                   // Pointer to game instance for accessing players and NetworkManager
    float lastPlayerUpdateTime = 0.0f; // Timer for throttling
    static const float PLAYER_UPDATE_INTERVAL; // Throttling interval (e.g., 0.1s)
};

#endif // PLAYER_NETWORK_HANDLER_H