#include "SteamManager.h"

void SteamManager::Initialize() {
    if (!SteamAPI_Init()) {
        std::cerr << "[ERROR] Steam API initialization failed!" << std::endl;
    }
}

void SteamManager::SendPlayerUpdate(CubeGame* game) {
    if (!game->IsInLobby()) return;
    char buffer[256];
    snprintf(buffer, sizeof(buffer), "P|%llu|%.2f|%.2f|%d",
             game->GetLocalPlayer().steamID.ConvertToUint64(), 
             game->GetLocalPlayer().x, 
             game->GetLocalPlayer().y, 
             game->GetLocalPlayer().health);
    SteamMatchmaking()->SendLobbyChatMsg(game->GetLobbyID(), buffer, strlen(buffer) + 1);
}

