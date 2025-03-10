#include "LobbyManager.h"
#include <iostream>

LobbyManager::LobbyManager(CubeGame* game, NetworkManager* networkManager)
    : game(game), networkManager(networkManager),
      m_cbLobbyCreated(this, &LobbyManager::OnLobbyCreated),
      m_cbGameLobbyJoinRequested(this, &LobbyManager::OnGameLobbyJoinRequested),
      m_cbLobbyEnter(this, &LobbyManager::OnLobbyEnter),
      m_cbLobbyMatchList(this, &LobbyManager::OnLobbyMatchList) {}
void LobbyManager::JoinLobbyFromNetwork(CSteamID lobby) {
    if (game->inLobby) return;
    game->m_isHost = false;
    SteamAPICall_t call = SteamMatchmaking()->JoinLobby(lobby);
    if (call == k_uAPICallInvalid) {
        std::cerr << "[LOBBY] JoinLobby call failed immediately for " 
                  << lobby.ConvertToUint64() << "\n";
        game->currentState = GameState::MainMenu;
    }
}
void LobbyManager::OnLobbyCreated(LobbyCreated_t* pParam) {
    if (pParam->m_eResult != k_EResultOK) {
        std::cerr << "[LOBBY] Failed to create lobby. EResult=" << pParam->m_eResult << "\n";
        game->currentState = GameState::MainMenu;
        return;
    }
    game->m_currentLobby = CSteamID(pParam->m_ulSteamIDLobby);
    game->inLobby = true;
    game->currentState = GameState::Lobby;
    SteamMatchmaking()->SetLobbyData(game->m_currentLobby, "name", game->lobbyNameInput.c_str());
    SteamMatchmaking()->SetLobbyData(game->m_currentLobby, "game_id", CubeGame::GAME_ID);
    CSteamID myID = SteamUser()->GetSteamID();
    std::string hostStr = std::to_string(myID.ConvertToUint64());
    SteamMatchmaking()->SetLobbyData(game->m_currentLobby, "host_steam_id", hostStr.c_str());
    m_connectedClients[myID] = true;
}
void LobbyManager::OnGameLobbyJoinRequested(GameLobbyJoinRequested_t* pParam) {
    JoinLobbyFromNetwork(pParam->m_steamIDLobby);
}
void LobbyManager::OnLobbyEnter(LobbyEnter_t* pParam) {
    if (pParam->m_EChatRoomEnterResponse != k_EChatRoomEnterResponseSuccess) {
        std::cerr << "[ERROR] Failed to enter lobby, returning to main menu." << std::endl;
        game->currentState = GameState::MainMenu;
        return;
    }

    game->m_currentLobby = CSteamID(pParam->m_ulSteamIDLobby);
    game->inLobby = true;
    game->currentState = GameState::Lobby;

    game->entityManager->getPlayers().clear();
    game->playerLoadedStatus.clear();
    m_playerStates.clear();
    m_lastEnemyUpdateTime.clear();

    int memberCount = SteamMatchmaking()->GetNumLobbyMembers(game->m_currentLobby);
    for (int i = 0; i < memberCount; i++) {
        CSteamID member = SteamMatchmaking()->GetLobbyMemberByIndex(game->m_currentLobby, i);
        Player newPlayer;
        newPlayer.initialize();
        newPlayer.steamID = member;
        game->entityManager->getPlayers()[member] = newPlayer;
        game->playerLoadedStatus[member] = false;
    }

    if (game->m_isHost) {
        game->playerLoadedStatus[game->localSteamID] = true;
        setIsConnectedToHost(false);
    } else {
        game->playerLoadedStatus[game->localSteamID] = false;
        const char* hostStr = SteamMatchmaking()->GetLobbyData(game->m_currentLobby, "host_steam_id");
        if (hostStr && *hostStr) {
            CSteamID hostID(std::stoull(hostStr));
            if (m_networking->AcceptP2PSessionWithUser(hostID)) {
                setIsConnectedToHost(true);
            }
        }
    }
}
void LobbyManager::OnLobbyMatchList(LobbyMatchList_t* pParam) {
    game->lobbyList.clear();
    for (uint32 i = 0; i < pParam->m_nLobbiesMatching; ++i) {
        CSteamID lobbyID = SteamMatchmaking()->GetLobbyByIndex(i);
        const char* lobbyName = SteamMatchmaking()->GetLobbyData(lobbyID, "name");
        if (lobbyName && *lobbyName) {
            game->lobbyList.emplace_back(lobbyID, std::string(lobbyName));
        }
    }
    game->lobbyListUpdated = true;
    game->hud.updateText("searchStatus", "Lobby Search");
}