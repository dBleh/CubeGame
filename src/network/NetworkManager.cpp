#include "NetworkManager.h"
#include <steam/steam_api.h>
#include <iostream>
#include <cstring>
#include <sstream>
#include <vector>
#include <chrono>

#define GAME_ID "SteamGame_v1"  // Define GAME_ID here; alternatively, move to Config.h

NetworkManager::NetworkManager(Game* gameInstance)
    : game(gameInstance),
      m_cbLobbyCreated(this, &NetworkManager::OnLobbyCreated),
      m_cbGameLobbyJoinRequested(this, &NetworkManager::OnGameLobbyJoinRequested),
      m_cbLobbyEnter(this, &NetworkManager::OnLobbyEnter),
      m_cbP2PSessionRequest(this, &NetworkManager::OnP2PSessionRequest),
      m_cbP2PSessionConnectFail(this, &NetworkManager::OnP2PSessionConnectFail),
      m_cbLobbyMatchList(this, &NetworkManager::OnLobbyMatchList) {
    if (!SteamAPI_Init()) {
        std::cerr << "[ERROR] Steam API initialization failed!" << std::endl;
        return;
    }
    m_networking = SteamNetworking();
    if (!m_networking) {
        std::cerr << "[ERROR] Could not get SteamNetworking interface!" << std::endl;
    }
}

NetworkManager::~NetworkManager() {
    SteamAPI_Shutdown();
}

void NetworkManager::ReceiveMessages() {
    if (!m_networking || !SteamUser()) return;

    uint32 msgSize;
    while (m_networking->IsP2PPacketAvailable(&msgSize)) {
        char buffer[1024];
        CSteamID sender;
        if (msgSize > sizeof(buffer) - 1) continue;

        if (m_networking->ReadP2PPacket(buffer, sizeof(buffer), &msgSize, &sender)) {
            buffer[msgSize] = '\0';
            std::string msg(buffer);
            if (m_connectedClients.find(sender) == m_connectedClients.end()) {
                AcceptSession(sender);
            }
            if (messageHandler) {
                messageHandler(msg, sender);
            }
        }
    }
}

bool NetworkManager::SendMessage(CSteamID target, const std::string& msg) {
    if (!m_networking || !SteamUser()) return false;
    uint32 msgSize = static_cast<uint32>(msg.size() + 1);
    return m_networking->SendP2PPacket(target, msg.c_str(), msgSize, k_EP2PSendReliable);
}

bool NetworkManager::BroadcastMessage(const std::string& msg) {
    bool success = true;
    for (const auto& client : m_connectedClients) {
        if (!SendMessage(client.first, msg)) {
            success = false;
        }
    }
    return success;
}

void NetworkManager::ProcessCallbacks() {
    SteamAPI_RunCallbacks();
}

bool NetworkManager::IsInitialized() const {
    return SteamUser() && SteamUser()->BLoggedOn();
}

void NetworkManager::AcceptSession(CSteamID remoteID) {
    if (m_networking && m_networking->AcceptP2PSessionWithUser(remoteID)) {
        m_connectedClients[remoteID] = true;
    }
}

void NetworkManager::SetMessageHandler(std::function<void(const std::string&, CSteamID)> handler) {
    messageHandler = handler;
}

void NetworkManager::JoinLobbyFromNetwork(CSteamID lobby) {
    if (game->IsInLobby()) return;
    SteamAPICall_t call = SteamMatchmaking()->JoinLobby(lobby);
    if (call == k_uAPICallInvalid) {
        std::cerr << "[LOBBY] JoinLobby call failed immediately for " 
                  << lobby.ConvertToUint64() << "\n";
        game->SetCurrentState(GameState::MainMenu);
    }
}

// Callback Implementations
void NetworkManager::OnLobbyCreated(LobbyCreated_t* pParam) {
    if (pParam->m_eResult != k_EResultOK) {
        std::cerr << "[LOBBY] Failed to create lobby. EResult=" << pParam->m_eResult << "\n";
        game->SetCurrentState(GameState::MainMenu);
        return;
    }
    CSteamID lobbyID(pParam->m_ulSteamIDLobby);
    game->SetCurrentState(GameState::Lobby);
    SteamMatchmaking()->SetLobbyData(lobbyID, "name", game->GetLobbyNameInput().c_str());
    SteamMatchmaking()->SetLobbyData(lobbyID, "game_id", GAME_ID);
    CSteamID myID = SteamUser()->GetSteamID();
    std::string hostStr = std::to_string(myID.ConvertToUint64());
    SteamMatchmaking()->SetLobbyData(lobbyID, "host_steam_id", hostStr.c_str());
    m_connectedClients[myID] = true;
}

void NetworkManager::OnLobbyEnter(LobbyEnter_t* pParam) {
    if (pParam->m_EChatRoomEnterResponse != k_EChatRoomEnterResponseSuccess) {
        std::cerr << "[ERROR] Failed to enter lobby: " << pParam->m_EChatRoomEnterResponse << std::endl;
        game->SetCurrentState(GameState::MainMenu);
        return;
    }
    std::cout << "Joined lobby" << std::endl;
    game->SetCurrentState(GameState::Lobby);
}

void NetworkManager::OnLobbyMatchList(LobbyMatchList_t* pParam) {
    lobbyList.clear();
    for (uint32 i = 0; i < pParam->m_nLobbiesMatching; ++i) {
        CSteamID lobbyID = SteamMatchmaking()->GetLobbyByIndex(i);
        const char* lobbyName = SteamMatchmaking()->GetLobbyData(lobbyID, "name");
        if (lobbyName && *lobbyName) {
            lobbyList.emplace_back(lobbyID, std::string(lobbyName));
        }
    }
    this->lobbyListUpdated = true;
    game->GetHUD().updateText("searchStatus", "Lobby Search Complete");
}

void NetworkManager::OnGameLobbyJoinRequested(GameLobbyJoinRequested_t* pParam) {
    JoinLobbyFromNetwork(pParam->m_steamIDLobby);
}

void NetworkManager::OnP2PSessionRequest(P2PSessionRequest_t* pParam) {
    if (m_networking && m_networking->AcceptP2PSessionWithUser(pParam->m_steamIDRemote)) {
        m_connectedClients[pParam->m_steamIDRemote] = true;
    }
}

void NetworkManager::OnP2PSessionConnectFail(P2PSessionConnectFail_t* pParam) {
    std::cerr << "[ERROR] P2P session failed with " << pParam->m_steamIDRemote.ConvertToUint64() << ": " << pParam->m_eP2PSessionError << std::endl;
    m_connectedClients.erase(pParam->m_steamIDRemote);
    if (m_connectedClients.empty()) {
        isConnectedToHost = false;
    }
}

void NetworkManager::ProcessNetworkMessages(const std::string& msg, CSteamID sender) {
    std::cout << "[NETWORK] Received: " << msg << " from " << sender.ConvertToUint64() << std::endl;
}

