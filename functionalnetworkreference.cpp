#include <steam/steam_api.h>
#include <steam/isteamnetworking.h>
#include <steam/isteammatchmaking.h>
#include <iostream>
#include <string>
#include <thread>
#include <chrono>
#include <map>
#include <conio.h>

class CubeGame
{
public:
    CubeGame();
    ~CubeGame();
    void RunGame();

private:
    ISteamNetworking* m_networking = nullptr;
    CSteamID m_currentLobby;
    CSteamID m_hostID; // Track host SteamID for client
    bool m_isHost = false;
    float m_broadcastTimer = 0.0f;
    std::map<CSteamID, bool> m_connectedClients;

    void ShowMenu();
    void CreateLobby();
    void SendMessage(CSteamID target, const std::string& msg);
    void ReceiveMessages();
    void CheckUserInput();

    STEAM_CALLBACK(CubeGame, OnLobbyCreated, LobbyCreated_t, m_cbLobbyCreated);
    STEAM_CALLBACK(CubeGame, OnGameLobbyJoinRequested, GameLobbyJoinRequested_t, m_cbGameLobbyJoinRequested);
    STEAM_CALLBACK(CubeGame, OnLobbyEnter, LobbyEnter_t, m_cbLobbyEnter);
    STEAM_CALLBACK(CubeGame, OnP2PSessionRequest, P2PSessionRequest_t, m_cbP2PSessionRequest);
    STEAM_CALLBACK(CubeGame, OnP2PSessionConnectFail, P2PSessionConnectFail_t, m_cbP2PSessionConnectFail);
};

CubeGame::CubeGame()
: m_cbLobbyCreated(this, &CubeGame::OnLobbyCreated)
, m_cbGameLobbyJoinRequested(this, &CubeGame::OnGameLobbyJoinRequested)
, m_cbLobbyEnter(this, &CubeGame::OnLobbyEnter)
, m_cbP2PSessionRequest(this, &CubeGame::OnP2PSessionRequest)
, m_cbP2PSessionConnectFail(this, &CubeGame::OnP2PSessionConnectFail)
{
    if (!SteamAPI_Init()) {
        std::cerr << "[ERROR] Could not init Steam API. Is Steam running?\n";
        exit(1);
    }
    m_networking = SteamNetworking();
    if (!m_networking) {
        std::cerr << "[ERROR] Could not get SteamNetworking interface.\n";
        exit(1);
    }
    m_currentLobby = k_steamIDNil;
    m_hostID = k_steamIDNil;

    std::cout << "[INFO] Steam initialized. My SteamID: "
              << SteamUser()->GetSteamID().ConvertToUint64() << std::endl;
}

CubeGame::~CubeGame()
{
    for (auto& [steamID, _] : m_connectedClients) {
        m_networking->CloseP2PSessionWithUser(steamID);
    }
    SteamAPI_Shutdown();
    std::cout << "[INFO] Steam shut down.\n";
}

void CubeGame::RunGame()
{
    ShowMenu();
    while (true) {
        SteamAPI_RunCallbacks();
        ReceiveMessages();
        CheckUserInput();
        if (m_isHost) {
            m_broadcastTimer += 0.016f;
            if (m_broadcastTimer >= 2.0f) {
                for (auto& [steamID, _] : m_connectedClients) {
                    SendMessage(steamID, "Server tick: Hello from host");
                }
                m_broadcastTimer = 0.0f;
            }
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(16));
    }
}

void CubeGame::ShowMenu()
{
    std::cout << "\n===== STEAM TEST GAME =====\n";
    std::cout << "1) Create Game (host)\n";
    std::cout << "2) Wait for Friend Invite (client)\n";
    std::cout << "Select: ";
    int choice;
    std::cin >> choice;
    if (choice == 1) {
        m_isHost = true;
        CreateLobby();
    } else {
        m_isHost = false;
        std::cout << "[INFO] Ok, do nothing. If a friend invites you, we'll get a callback.\n";
        std::cout << "[INFO] Type any key to send to the host once connected.\n";
    }
}

void CubeGame::CreateLobby()
{
    SteamAPICall_t rc = SteamMatchmaking()->CreateLobby(k_ELobbyTypeFriendsOnly, 4);
    if (rc == k_uAPICallInvalid) {
        std::cerr << "[LOBBY] CreateLobby call failed immediately.\n";
    } else {
        std::cout << "[LOBBY] Creating lobby...\n";
        std::cout << "[INFO] Type any key to send to clients once connected.\n";
    }
}

void CubeGame::OnLobbyCreated(LobbyCreated_t* pParam)
{
    if (pParam->m_eResult != k_EResultOK) {
        std::cerr << "[LOBBY] Failed to create lobby. EResult=" << pParam->m_eResult << "\n";
        return;
    }
    m_currentLobby.SetFromUint64(pParam->m_ulSteamIDLobby);
    std::cout << "[LOBBY] Lobby " << m_currentLobby.ConvertToUint64() << " created successfully.\n";

    CSteamID myID = SteamUser()->GetSteamID();
    std::string hostStr = std::to_string(myID.ConvertToUint64());
    SteamMatchmaking()->SetLobbyData(m_currentLobby, "host_steam_id", hostStr.c_str());
}

void CubeGame::OnGameLobbyJoinRequested(GameLobbyJoinRequested_t* pParam)
{
    std::cout << "[LOBBY] OnGameLobbyJoinRequested: "
              << pParam->m_steamIDLobby.ConvertToUint64() 
              << " from friend: " << pParam->m_steamIDFriend.ConvertToUint64() << "\n";

    SteamAPICall_t rc = SteamMatchmaking()->JoinLobby(pParam->m_steamIDLobby);
    if (rc == k_uAPICallInvalid) {
        std::cerr << "[LOBBY] JoinLobby call failed.\n";
    } else {
        std::cout << "[LOBBY] Joining lobby...\n";
    }
}

void CubeGame::OnLobbyEnter(LobbyEnter_t* pParam)
{
    CSteamID lobbyID(pParam->m_ulSteamIDLobby);
    if (pParam->m_EChatRoomEnterResponse != k_EChatRoomEnterResponseSuccess) {
        std::cerr << "[LOBBY] Failed to join lobby " << lobbyID.ConvertToUint64() << "\n";
        return;
    }
    std::cout << "[LOBBY] Entered lobby " << lobbyID.ConvertToUint64() << "\n";
    m_currentLobby = lobbyID;
    if (m_isHost) {
        std::cout << "[LOBBY] Host entered own lobby.\n";
        return;
    }
    const char* hostStr = SteamMatchmaking()->GetLobbyData(lobbyID, "host_steam_id");
    if (!hostStr || !hostStr[0]) {
        std::cerr << "[LOBBY] Could not find host_steam_id in lobby data!\n";
        return;
    }
    std::cout << "[LOBBY] Found host_steam_id = " << hostStr << "\n";
    uint64 hostInt = std::stoull(hostStr);
    m_hostID.SetFromUint64(hostInt);

    SendMessage(m_hostID, "Client join request");
}

void CubeGame::OnP2PSessionRequest(P2PSessionRequest_t* pParam)
{
    std::cout << "[P2P] Session request from " << pParam->m_steamIDRemote.ConvertToUint64() << "\n";
    if (m_isHost) {
        if (m_networking->AcceptP2PSessionWithUser(pParam->m_steamIDRemote)) {
            m_connectedClients[pParam->m_steamIDRemote] = true;
            std::cout << "[P2P] Accepted session with " << pParam->m_steamIDRemote.ConvertToUint64() << "\n";
        } else {
            std::cerr << "[P2P] Failed to accept session with " << pParam->m_steamIDRemote.ConvertToUint64() << "\n";
        }
    }
}

void CubeGame::OnP2PSessionConnectFail(P2PSessionConnectFail_t* pParam)
{
    std::cout << "[P2P] Session connect fail with " << pParam->m_steamIDRemote.ConvertToUint64() 
              << " reason: " << (int)pParam->m_eP2PSessionError << "\n";
    m_connectedClients.erase(pParam->m_steamIDRemote);
}

void CubeGame::SendMessage(CSteamID target, const std::string& msg)
{
    if (!m_networking->SendP2PPacket(target, msg.c_str(), (uint32)msg.size() + 1, k_EP2PSendReliable)) {
        std::cerr << "[P2P] Failed to send message to " << target.ConvertToUint64() << ": " << msg << "\n";
    } else {
        std::cout << "[P2P] Sent message to " << target.ConvertToUint64() << ": " << msg << "\n";
    }
}

void CubeGame::ReceiveMessages()
{
    uint32 msgSize;
    while (m_networking->IsP2PPacketAvailable(&msgSize)) {
        char buffer[1024];
        CSteamID sender;
        if (msgSize > sizeof(buffer) - 1) {
            std::cerr << "[P2P] Received packet too large: " << msgSize << "\n";
            continue;
        }
        if (m_networking->ReadP2PPacket(buffer, sizeof(buffer), &msgSize, &sender)) {
            buffer[msgSize] = '\0';
            std::string msg(buffer);
            if (m_isHost && !m_connectedClients.count(sender)) {
                m_networking->AcceptP2PSessionWithUser(sender);
                m_connectedClients[sender] = true;
                std::cout << "[P2P] Accepted new client " << sender.ConvertToUint64() << "\n";
            }
            if (m_isHost) {
                std::cout << "[SERVER] Received from " << sender.ConvertToUint64() << ": " << msg << "\n";
            } else {
                std::cout << "[CLIENT] Received from " << sender.ConvertToUint64() << ": " << msg << "\n";
            }
        }
    }
}

void CubeGame::CheckUserInput()
{
    if (_kbhit()) {
        char ch = _getch();
        std::string msg(1, ch);
        if (m_isHost) {
            for (auto& [steamID, _] : m_connectedClients) {
                SendMessage(steamID, msg);
            }
        } else if (m_hostID != k_steamIDNil) {
            SendMessage(m_hostID, msg);
        }
    }
}

int main()
{
    CubeGame game;
    game.RunGame();
    return 0;
}