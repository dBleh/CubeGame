#ifndef MESSAGE_HANDLER_H
#define MESSAGE_HANDLER_H

#include <string>
#include "CubeGame.h"
#include "NetworkManager.h"

class MessageHandler {
public:
    MessageHandler(CubeGame* game, NetworkManager* networkManager);
    void ProcessNetworkMessages(const std::string& msg, CSteamID sender);

private:
    CubeGame* game;
    NetworkManager* networkManager;

    void HandlePlayerLoaded(const std::string& msg);
    void HandlePlayerUpdate(const std::string& msg);
    void HandleEnemySpawn(const std::string& msg);
    void HandleEnemyUpdate(const std::string& msg);
    void HandleEnemyDeath(const std::string& msg);
    void HandleBulletFire(const std::string& msg, CSteamID sender);
    void HandleHit(const std::string& msg, CSteamID sender);
    void HandleEnemyRemove(const std::string& msg);
    void HandleStart(const std::string& msg);
    void HandleNextLevel(const std::string& msg);
    void HandleTimer(const std::string& msg);
    void HandlePlay(const std::string& msg);
    void HandleGameOver(const std::string& msg);
    void HandleLobbyReturn(const std::string& msg);
};

#endif