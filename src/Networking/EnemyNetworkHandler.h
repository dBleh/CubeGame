#ifndef ENEMYNETWORKHANDLER_H
#define ENEMYNETWORKHANDLER_H

#include <string>
#include <steam/steam_api.h>

class CubeGame;

class EnemyNetworkHandler {
public:
    EnemyNetworkHandler(CubeGame* gameInstance);
    
    void HandleEnemySpawn(const std::string& msg);
    void HandleEnemyUpdate(const std::string& msg);
    void HandleEnemyDeath(const std::string& msg);
    void HandleEnemyRemove(const std::string& msg);

private:
    CubeGame* game;
};

#endif // ENEMYNETWORKHANDLER_H