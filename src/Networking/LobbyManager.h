#ifndef LOBBY_MANAGER_H
#define LOBBY_MANAGER_H

#include "CubeGame.h"
#include "NetworkManager.h"
#include <steam/steam_api.h>

class LobbyManager {
public:
    LobbyManager(CubeGame* game, NetworkManager* networkManager);
    void JoinLobbyFromNetwork(CSteamID lobby);

private:
    CubeGame* game;
    NetworkManager* networkManager;

    STEAM_CALLBACK(LobbyManager, OnLobbyCreated, LobbyCreated_t, m_cbLobbyCreated);
    STEAM_CALLBACK(LobbyManager, OnGameLobbyJoinRequested, GameLobbyJoinRequested_t, m_cbGameLobbyJoinRequested);
    STEAM_CALLBACK(LobbyManager, OnLobbyEnter, LobbyEnter_t, m_cbLobbyEnter);
    STEAM_CALLBACK(LobbyManager, OnLobbyMatchList, LobbyMatchList_t, m_cbLobbyMatchList);
};

#endif