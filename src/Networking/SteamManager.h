#ifndef STEAMMANAGER_H
#define STEAMMANAGER_H

#include <steam/steam_api.h>
#include "../Core/CubeGame.h"

/**
 * @brief Provides Steam API initialization and update messaging.
 */
class SteamManager {
public:
    // Initialize the Steam API.
    static void Initialize();

    // Send a player update message via Steam matchmaking chat.
    static void SendPlayerUpdate(CubeGame* game);
};

#endif // STEAMMANAGER_H
