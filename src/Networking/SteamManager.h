#ifndef STEAMMANAGER_H
#define STEAMMANAGER_H

#include <steam/steam_api.h>
#include "../Core/CubeGame.h"

class SteamManager {
public:
    static void Initialize();
    static void SendPlayerUpdate(CubeGame* game); // Simplified to only handle sending updates
};

#endif // STEAMMANAGER_H