#include "CubeGame.h"
#include "../States/State.h"
#include "../States/MainMenuState.h"
#include "../States/LobbyCreationState.h"
#include "../States/LobbySearchState.h"
#include "../States/LobbyState.h"
#include "../States/GameplayState.h"
#include "../States/GameOverState.h"
#include "../Networking/SteamManager.h"
#include <memory>
#include <cmath>

const char* CubeGame::GAME_ID = "CubeShooter_v1";




// CubeGame Implementation
CubeGame::CubeGame() : hud(font) {
    if (!debugMode) {
        SteamManager::Initialize();
        m_pNetworkingSockets = SteamNetworkingSockets();
    } else {
        std::cout << "[DEBUG] Running in debug mode, Steam not initialized" << std::endl;
    }
    window.create(sf::VideoMode(SCREEN_WIDTH, SCREEN_HEIGHT), "Multiplayer Lobby System");
    if (!window.isOpen()) std::exit(1);
    window.setFramerateLimit(60);

    if (!font.loadFromFile("Roboto-Regular.ttf")) {
        std::cerr << "[ERROR] Failed to load font!" << std::endl;
    }

    localPlayer.initialize();
    if (!debugMode && SteamUser() && SteamUser()->BLoggedOn()) {
        localPlayer.steamID = SteamUser()->GetSteamID();
    } else if (debugMode) {
        localPlayer.steamID = CSteamID(76561198000000000ULL + 1); // Fake SteamID for testing
    }

    ResetViewToDefault();
    shootCooldown = 0.0f;
    SetupInitialHUD();
}

CubeGame::~CubeGame() {
    for (auto& [steamID, conn] : m_connections) {
        m_pNetworkingSockets->CloseConnection(conn, 0, nullptr, false);
    }
    if (inLobby) SteamMatchmaking()->LeaveLobby(lobbyID);
    SteamAPI_Shutdown();
}

void CubeGame::Run() {
    sf::Clock clock;
    std::unique_ptr<State> state;

    // If we were already in a lobby, connect to players:
    if (inLobby) {
        int memberCount = SteamMatchmaking()->GetNumLobbyMembers(lobbyID);
        for (int i = 0; i < memberCount; ++i) {
            CSteamID member = SteamMatchmaking()->GetLobbyMemberByIndex(lobbyID, i);
            ConnectToPlayer(member);
        }
    }

    while (window.isOpen()) {
        // Existing Steam callbacks for matchmaking/lobby:
        SteamAPI_RunCallbacks();

        // New line: let SteamNetworkingSockets invoke our OnSteamNetConnectionStatusChanged
        m_pNetworkingSockets->RunCallbacks();  // <--- ADD THIS

        float dt = clock.restart().asSeconds();
        dt = std::min(dt, 0.033f);

        sf::Event event;
        while (window.pollEvent(event)) {
            ProcessEvents(event);
            if (state) state->ProcessEvent(event);
        }

        // Existing function that handles incoming data messages (ProcessMessage, etc.)
        ProcessNetworkMessages();

        // Typical finite-state-machine for your states:
        switch (currentState) {
            case GameState::MainMenu:
                if (!state || !dynamic_cast<MainMenuState*>(state.get()))
                    state = std::make_unique<MainMenuState>(this);
                break;
            case GameState::LobbyCreation:
                if (!state || !dynamic_cast<LobbyCreationState*>(state.get()))
                    state = std::make_unique<LobbyCreationState>(this);
                break;
            case GameState::LobbySearch:
                if (!state || !dynamic_cast<LobbySearchState*>(state.get()))
                    state = std::make_unique<LobbySearchState>(this);
                break;
            case GameState::Lobby:
                if (!state || !dynamic_cast<LobbyState*>(state.get()))
                    state = std::make_unique<LobbyState>(this);
                break;
            case GameState::Playing:
                if (!state || !dynamic_cast<GameplayState*>(state.get()))
                    state = std::make_unique<GameplayState>(this);
                break;
            case GameState::GameOver:
                if (!state || !dynamic_cast<GameOverState*>(state.get()))
                    state = std::make_unique<GameOverState>(this);
                break;
            case GameState::Spectating:
            case GameState::Leaderboard:
                break;
        }

        if (state) {
            state->Update(dt);
            if (state && currentState == GetCurrentState()) {
                state->Render();
            }
        }
    }
    std::cout << "[DEBUG] Game loop exited normally" << std::endl;
}


void CubeGame::ProcessEvents(sf::Event& event) {
    if (event.type == sf::Event::Closed) {
        window.close();
    }
}

void CubeGame::SetupInitialHUD()
{
    //
    // MAIN MENU
    //
    hud.addElement(
        /* id          */ "title",
        /* content     */ "MAIN MENU",
        /* size        */ 30,
        /* position    */ sf::Vector2f(50.f, 50.f),
        /* visibleState*/ GameState::MainMenu,
        /* renderMode  */ HUD::RenderMode::ScreenSpace,
        /* hoverable   */ false // no hover effect on the title
    );

    hud.addElement(
        "createLobby",
        "Create Lobby",
        24,
        sf::Vector2f(100.f, 150.f),
        GameState::MainMenu,
        HUD::RenderMode::ScreenSpace,
        true // hoverable, so text color changes slightly on hover
    );

    hud.addElement(
        "searchLobby",
        "Search Lobbies",
        24,
        sf::Vector2f(100.f, 200.f),
        GameState::MainMenu,
        HUD::RenderMode::ScreenSpace,
        true // hoverable
    );


    //
    // LOBBY CREATION
    //
    hud.addElement(
        "lobbyPrompt",
        "Enter Lobby Name (Press Enter to Create, ESC to Cancel):",
        18,
        sf::Vector2f(50.f, 200.f),
        GameState::LobbyCreation,
        HUD::RenderMode::ScreenSpace,
        false
    );


    //
    // LOBBY SEARCH
    //
    hud.addElement(
        "searchStatus",
        "Searching...",
        18,
        sf::Vector2f(20.f, 20.f),
        GameState::LobbySearch,
        HUD::RenderMode::ScreenSpace,
        false
    );

    hud.addElement(
        "lobbyList",
        "Available Lobbies:\n",
        20,
        sf::Vector2f(50.f, 100.f),
        GameState::LobbySearch,
        HUD::RenderMode::ScreenSpace,
        false
    );


    //
    // LOBBY
    //
    hud.addElement(
        "lobbyStatus",
        "Lobby",
        22,
        sf::Vector2f(20.f, 20.f),
        GameState::Lobby,
        HUD::RenderMode::ScreenSpace,
        false
    );

    hud.addElement(
        "ready",
        "Press R to Toggle Ready",
        20,
        sf::Vector2f(SCREEN_WIDTH / 2.f - 100.f, 300.f),
        GameState::Lobby,
        HUD::RenderMode::ScreenSpace,
        false
    );

    hud.addElement(
        "startGame",
        "Press S to Start Game (Host Only)",
        20,
        sf::Vector2f(SCREEN_WIDTH / 2.f - 100.f, 350.f),
        GameState::Lobby,
        HUD::RenderMode::ScreenSpace,
        false
    );

    hud.addElement(
        "returnMain",
        "Press M to Return to Main Menu",
        20,
        sf::Vector2f(SCREEN_WIDTH / 2.f - 100.f, 400.f),
        GameState::Lobby,
        HUD::RenderMode::ScreenSpace,
        false
    );

    hud.updateBaseColor("level", sf::Color::White);
    //
    // GAMEPLAY
    //
    hud.addElement(
        "gameStatus",
        "Playing",
        16,
        sf::Vector2f(10.f, 10.f),
        GameState::Playing,
        HUD::RenderMode::ViewSpace,
        false
    );
    hud.updateBaseColor("scoreboard", sf::Color::White);
    hud.addElement(
        "level",
        "Level: 0\nEnemies: 0\nHP: 100",
        16,
        sf::Vector2f(10.f, 50.f),
        GameState::Playing,
        HUD::RenderMode::ViewSpace,
        false
    );
    hud.updateBaseColor("scoreboard", sf::Color::White);
    hud.addElement(
        "scoreboard",
        "Scoreboard:\n",
        16,
        sf::Vector2f(SCREEN_WIDTH - 200.f, 10.f),
        GameState::Playing,
        HUD::RenderMode::ViewSpace,
        false
    );
    hud.updateBaseColor("scoreboard", sf::Color::White);
    hud.addElement(
        "menu",
        "",
        20,
        sf::Vector2f(10.f, 300.f),
        GameState::Playing,
        HUD::RenderMode::ViewSpace,
        false
    );
}


void CubeGame::RenderHUDLayer() {
    hud.render(window, view, currentState);
}

void CubeGame::ResetViewToDefault() {
    view.setCenter(sf::Vector2f(SCREEN_WIDTH / 2.f, SCREEN_HEIGHT / 2.f));
    view.setSize(sf::Vector2f(SCREEN_WIDTH, SCREEN_HEIGHT));
    window.setView(view);
}

void CubeGame::EnterLobbyCreation() {
    // If we’re already in a lobby, do nothing.
    if (inLobby) return;

    // NEW: If Steam isn’t ready, show a warning and block creation.
    if (!IsSteamInitialized()) {
        hud.updateText("status", "Waiting for Steam handshake. Please try again soon...");
        std::cout << "[DEBUG] Attempted to create lobby before Steam was ready." << std::endl;
        return;
    }

    // Otherwise proceed as normal
    currentState = GameState::LobbyCreation;
    lobbyNameInput.clear();
    hud.updateText("lobbyPrompt", "Enter Lobby Name (Press Enter to Create, ESC to Cancel): ");
    std::cout << "[DEBUG] Entered Lobby Creation state" << std::endl;
}

void CubeGame::CreateLobby(const std::string& lobbyName) {
    if (inLobby) return;
    SteamAPICall_t call = SteamMatchmaking()->CreateLobby(k_ELobbyTypePublic, 10);
    lobbyCreatedCallResult.Set(call, this, &CubeGame::OnLobbyCreated);
    lobbyNameInput = lobbyName;
    std::cout << "[DEBUG] Creating Steam lobby with name: " << lobbyName << std::endl;
}

void CubeGame::EnterLobbySearch() {
    if (inLobby) return;
    currentState = GameState::LobbySearch;
    SearchLobbies();
    std::cout << "[DEBUG] Entered Lobby Search state" << std::endl;
}

void CubeGame::SearchLobbies() {
    if (inLobby) return;
    lobbyList.clear();
    SteamMatchmaking()->AddRequestLobbyListStringFilter("game_id", GAME_ID, k_ELobbyComparisonEqual);
    SteamMatchmaking()->AddRequestLobbyListStringFilter("name", "", k_ELobbyComparisonNotEqual);
    SteamAPICall_t call = SteamMatchmaking()->RequestLobbyList();
    lobbyListCallResult.Set(call, this, &CubeGame::OnLobbyListReceived);
    hud.updateText("searchStatus", "Searching...");
    std::cout << "[DEBUG] Searching for Steam lobbies..." << std::endl;
}

void CubeGame::JoinLobby(CSteamID lobby) {
    SteamAPICall_t call = SteamMatchmaking()->JoinLobby(lobby);
    lobbyEnterCallResult.Set(call, this, &CubeGame::OnLobbyEntered);
    std::cout << "[DEBUG] Joining Steam lobby: " << lobby.ConvertToUint64() << std::endl;
}

void CubeGame::JoinLobbyByIndex(int index) {
    if (index >= 0 && index < static_cast<int>(lobbyList.size())) {
        JoinLobby(lobbyList[index].first);
    } else {
        std::cout << "[DEBUG] Invalid lobby index: " << index << std::endl;
        hud.updateText("searchStatus", "Invalid lobby selection");
    }
}

void CubeGame::ToggleReady() {
    localPlayer.ready = !localPlayer.ready;
    SteamMatchmaking()->SetLobbyMemberData(lobbyID, "ready", localPlayer.ready ? "1" : "0");
    players[localPlayer.steamID].ready = localPlayer.ready;
    SendPlayerUpdate();
}

void CubeGame::SendPlayerUpdate() {
    char buffer[256];
    Player& p = localPlayer;
    if (std::isnan(p.x) || std::isnan(p.y)) {
        p.x = p.y = 0.0f;
        std::cout << "[ERROR] Player position was NaN, reset to (0,0)" << std::endl;
    }
    int bytes = snprintf(buffer, sizeof(buffer), "P|%llu|%.1f|%.1f|%d|%d|%d",
                         p.steamID.ConvertToUint64(), p.x, p.y, p.health, p.kills, p.ready ? 1 : 0);
    if (bytes > 0 && static_cast<size_t>(bytes) < sizeof(buffer)) {
        for (const auto& [steamID, conn] : m_connections) {
            m_pNetworkingSockets->SendMessageToConnection(conn, buffer, bytes + 1, k_nSteamNetworkingSend_Reliable, nullptr);
        }
    }
}

void CubeGame::ResetLobby() {
    inLobby = true;
    gameStarted = false;
    currentState = GameState::Lobby;
    players.clear();
    localPlayer.ready = false;
    players[localPlayer.steamID] = localPlayer;
    SteamMatchmaking()->SetLobbyMemberData(lobbyID, "ready", "0");
    ResetViewToDefault();
}

void CubeGame::ResetGame() {
    enemies.clear();
    bullets.clear();
    players.clear();
    localPlayer.health = 100;
    localPlayer.isAlive = true;
    localPlayer.kills = 0;
    localPlayer.ready = false;
    localPlayer.initialize();
    players[localPlayer.steamID] = localPlayer;
    currentLevel = 0;
    enemiesPerWave = 5;
    gameStarted = false;
    ResetViewToDefault();
}

void CubeGame::ReturnToLobby() {
    ResetLobby();
    if (!inLobby) return;
    currentState = GameState::Lobby;
    gameStarted = false;
    enemies.clear();
    bullets.clear();
    currentLevel = 0;
    enemiesPerWave = 0;
    ResetViewToDefault();
}

void CubeGame::ReturnToMainMenu() {
    if (inLobby) {
        SteamMatchmaking()->LeaveLobby(lobbyID);
        inLobby = false;
        lobbyID = CSteamID();
    }
    currentState = GameState::MainMenu;
    gameStarted = false;
    hasGameBeenPlayed = false;
    enemies.clear();
    bullets.clear();
    players.clear();
    localPlayer.health = 100;
    localPlayer.isAlive = true;
    localPlayer.kills = 0;
    localPlayer.ready = false;
    localPlayer.initialize();
    currentLevel = 0;
    enemiesPerWave = 0;
    lobbyList.clear();
    ResetViewToDefault();
}

void CubeGame::StartGame() {
    if (!inLobby || currentState == GameState::Playing) return;
    currentState = GameState::Playing;
    gameStarted = true;
    hasGameBeenPlayed = true;
    currentLevel = 1;
    enemiesPerWave = 5;
    enemies.clear();
    bullets.clear();
    for (auto& player : players) {
        player.second.health = 100;
        player.second.isAlive = true;
        player.second.kills = 0;
        player.second.ready = false;
        player.second.x = std::isnan(player.second.x) ? SCREEN_WIDTH / 2.f : player.second.x;
        player.second.y = std::isnan(player.second.y) ? SCREEN_HEIGHT / 2.f : player.second.y;
        player.second.shape.setPosition(player.second.x, player.second.y);
        SteamMatchmaking()->SetLobbyMemberData(lobbyID, "ready", "0");
    }
    localPlayer.health = 100;
    localPlayer.isAlive = true;
    localPlayer.kills = 0;
    localPlayer.ready = false;
    players[localPlayer.steamID] = localPlayer;
    ResetViewToDefault();
    std::cout << "[DEBUG] Starting game locally" << std::endl;

    char startBuffer[16];
    int startBytes = snprintf(startBuffer, sizeof(startBuffer), "S|START");
    if (startBytes > 0 && static_cast<size_t>(startBytes) < sizeof(startBuffer)) {
        for (const auto& [steamID, conn] : m_connections) {
            m_pNetworkingSockets->SendMessageToConnection(conn, startBuffer, startBytes + 1, k_nSteamNetworkingSend_Reliable, nullptr);
        }
        std::cout << "[DEBUG] Sent game start signal via P2P" << std::endl;
    }
}

void CubeGame::ConnectToPlayer(CSteamID player) {
    if (m_connections.find(player) != m_connections.end() || player == localPlayer.steamID) return;
    SteamNetworkingIdentity identity;
    identity.SetSteamID(player);
    HSteamNetConnection conn = m_pNetworkingSockets->ConnectP2P(identity, 0, 0, nullptr);
    if (conn != k_HSteamNetConnection_Invalid) {
        m_connections[player] = conn;
        std::cout << "[DEBUG] Connected to player " << player.ConvertToUint64() << std::endl;
    } else {
        std::cerr << "[ERROR] Failed to connect to player " << player.ConvertToUint64() << std::endl;
    }
}

// This replaces (or renames) your old OnNetConnectionStatusChanged.
// It must match the signature given by STEAM_CALLBACK.
void CubeGame::OnSteamNetConnectionStatusChanged(SteamNetConnectionStatusChangedCallback_t* pInfo)
{
    CSteamID remoteID = pInfo->m_info.m_identityRemote.GetSteamID();

    if (pInfo->m_info.m_eState == k_ESteamNetworkingConnectionState_Connected)
    {
        // The connection just transitioned into the connected state.
        m_connections[remoteID] = pInfo->m_hConn;
        std::cout << "[DEBUG] Connection established with " 
                  << remoteID.ConvertToUint64() << std::endl;
    }
    else if (pInfo->m_info.m_eState == k_ESteamNetworkingConnectionState_ClosedByPeer ||
             pInfo->m_info.m_eState == k_ESteamNetworkingConnectionState_ProblemDetectedLocally)
    {
        // The connection either timed out or was closed.
        m_connections.erase(remoteID);
        m_pNetworkingSockets->CloseConnection(pInfo->m_hConn, 0, nullptr, false);
        std::cout << "[DEBUG] Connection closed with " 
                  << remoteID.ConvertToUint64() << std::endl;
    }
}


void CubeGame::ProcessNetworkMessages() {
    for (auto& [steamID, conn] : m_connections) {
        SteamNetworkingMessage_t* pMessages[16];
        int numMsgs = m_pNetworkingSockets->ReceiveMessagesOnConnection(conn, pMessages, 16);
        for (int i = 0; i < numMsgs; ++i) {
            std::string msg(static_cast<const char*>(pMessages[i]->m_pData), pMessages[i]->m_cbSize);
            ProcessMessage(msg, steamID);
            pMessages[i]->Release();
        }
    }
}

void CubeGame::ProcessMessage(const std::string& msg, CSteamID sender) {
    if (msg[0] == 'P') {
        uint64_t steamID;
        float x, y;
        int health, kills, ready;
        if (sscanf(msg.c_str(), "P|%llu|%f|%f|%d|%d|%d", &steamID, &x, &y, &health, &kills, &ready) == 6) {
            CSteamID id(steamID);
            if (id != localPlayer.steamID) {
                if (x < 0 || x > 2000 || y < 0 || y > 2000 || health < 0 || health > 100) {
                    std::cerr << "[WARNING] Invalid player data from " << steamID << std::endl;
                    return;
                }
                Player& p = players[id];
                if (!players.count(id)) p.initialize();
                p.x = x;
                p.y = y;
                p.health = health;
                p.kills = kills;
                p.ready = ready;
            }
        }
    } else if (msg[0] == 'E' && sender == SteamMatchmaking()->GetLobbyOwner(lobbyID)) {
        uint64_t enemyID;
        float x, y, spawnDelay;
        int health;
        if (sscanf(msg.c_str(), "E|%llu|%f|%f|%d|%f", &enemyID, &x, &y, &health, &spawnDelay) == 5) {
            if (enemies.count(enemyID)) {
                Enemy& e = enemies[enemyID];
                e.x = x;
                e.y = y;
                e.health = health;
                e.spawnDelay = spawnDelay;
            } else {
                Enemy e;
                e.initialize();
                e.id = enemyID;
                e.x = x;
                e.y = y;
                e.health = health;
                e.spawnDelay = spawnDelay;
                enemies[enemyID] = e;
            }
        }
    } else if (msg.substr(0, 7) == "B|fire") {
        int shooterID, bulletIdx;
        float startX, startY, targetX, targetY;
        if (sscanf(msg.c_str(), "B|fire|%d|%d|%f|%f|%f|%f", &shooterID, &bulletIdx, &startX, &startY, &targetX, &targetY) == 6) {
            int uniqueBulletId = (shooterID << 16) | bulletIdx;
            if (!bullets.count(uniqueBulletId)) {
                Bullet b;
                b.initialize(startX, startY, targetX, targetY);
                b.id = uniqueBulletId;
                bullets[uniqueBulletId] = b;
            }
        }
    } else if (msg[0] == 'H' && sender == SteamMatchmaking()->GetLobbyOwner(lobbyID)) {
        int bulletID;
        uint64_t enemyID;
        if (sscanf(msg.c_str(), "H|%d|%llu", &bulletID, &enemyID) == 2) {
            if (bullets.count(bulletID)) bullets.erase(bulletID);
            if (enemies.count(enemyID)) {
                enemies[enemyID].health -= 25;
                if (enemies[enemyID].health <= 0) enemies.erase(enemyID);
            }
        }
    } else if (msg[0] == 'K' && sender == SteamMatchmaking()->GetLobbyOwner(lobbyID)) {
        uint64_t playerID;
        if (sscanf(msg.c_str(), "K|%llu", &playerID) == 1) {
            if (CSteamID(playerID) == localPlayer.steamID) {
                localPlayer.kills++;
                players[localPlayer.steamID] = localPlayer;
            }
        }
    } else if (msg == "S|START" && sender == SteamMatchmaking()->GetLobbyOwner(lobbyID)) {
        if (currentState != GameState::Playing) {
            StartGame();
            std::cout << "[DEBUG] Received S|START signal via P2P" << std::endl;
        }
    }
}

void CubeGame::OnLobbyCreated(LobbyCreated_t* result, bool bIOFailure) {
    if (result->m_eResult == k_EResultOK) {
        lobbyID = CSteamID(result->m_ulSteamIDLobby);
        inLobby = true;
        currentState = GameState::Lobby;
        SteamMatchmaking()->SetLobbyData(lobbyID, "name", lobbyNameInput.c_str());
        SteamMatchmaking()->SetLobbyData(lobbyID, "game_id", GAME_ID);
        JoinLobby(lobbyID);
        hud.updateText("status", "Lobby Created! ID: " + std::to_string(lobbyID.ConvertToUint64()));
        std::cout << "[DEBUG] Steam lobby created successfully. ID: " << lobbyID.ConvertToUint64() << std::endl;
    } else {
        hud.updateText("status", "Failed to create lobby: " + std::to_string(result->m_eResult));
        std::cerr << "[ERROR] Failed to create Steam lobby! Result: " << result->m_eResult << std::endl;
        currentState = GameState::MainMenu;
    }
}

void CubeGame::OnLobbyEntered(LobbyEnter_t* result, bool bIOFailure) {
    if (result->m_EChatRoomEnterResponse == k_EChatRoomEnterResponseSuccess) {
        lobbyID = CSteamID(result->m_ulSteamIDLobby);
        inLobby = true;
        currentState = GameState::Lobby;
        std::cout << "[DEBUG] Successfully entered Steam lobby." << std::endl;
    } else {
        hud.updateText("searchStatus", "Failed to join lobby");
        std::cerr << "[ERROR] Failed to enter Steam lobby!" << std::endl;
        currentState = GameState::LobbySearch;
    }
}

void CubeGame::OnLobbyListReceived(LobbyMatchList_t* result, bool bIOFailure) {
    if (!bIOFailure && result->m_nLobbiesMatching > 0) {
        lobbyList.clear();
        std::string lobbyListText = "Available Lobbies:\n";
        for (uint32 i = 0; i < result->m_nLobbiesMatching; i++) {
            CSteamID lobbyId = SteamMatchmaking()->GetLobbyByIndex(i);
            const char* name = SteamMatchmaking()->GetLobbyData(lobbyId, "name");
            const char* gameId = SteamMatchmaking()->GetLobbyData(lobbyId, "game_id");
            if (name && *name && gameId && strcmp(gameId, GAME_ID) == 0) {
                lobbyList.emplace_back(lobbyId, name);
                lobbyListText += std::to_string(i) + ". " + name + " (ID: " + std::to_string(lobbyId.ConvertToUint64()) + ")\n";
            }
        }
        hud.updateText("lobbyList", lobbyListText);
        hud.updateText("searchStatus", "Select lobby number (0-" + std::to_string(lobbyList.size() - 1) + ") or ESC to cancel");
        std::cout << "[DEBUG] Found " << lobbyList.size() << " lobbies" << std::endl;
    } else {
        hud.updateText("lobbyList", "No lobbies found");
        hud.updateText("searchStatus", "No lobbies available (ESC to cancel)");
        std::cerr << "[ERROR] No Steam lobbies found." << std::endl;
    }
}

void CubeGame::OnLobbyChatUpdate(LobbyChatUpdate_t* pParam) {
    if (pParam->m_ulSteamIDLobby == lobbyID.ConvertToUint64()) {
        if (pParam->m_rgfChatMemberStateChange & k_EChatMemberStateChangeEntered) {
            CSteamID newMember(pParam->m_ulSteamIDMakingChange);
            ConnectToPlayer(newMember);
        } else if (pParam->m_rgfChatMemberStateChange & (k_EChatMemberStateChangeLeft | k_EChatMemberStateChangeDisconnected)) {
            CSteamID leavingMember(pParam->m_ulSteamIDMakingChange);
            if (m_connections.count(leavingMember)) {
                m_pNetworkingSockets->CloseConnection(m_connections[leavingMember], 0, nullptr, false);
                m_connections.erase(leavingMember);
                std::cout << "[DEBUG] Player " << leavingMember.ConvertToUint64() << " disconnected" << std::endl;
            }
        }
    }
}

void CubeGame::OnLobbyChatMsg(LobbyChatMsg_t* pParam) {
    if (pParam->m_ulSteamIDLobby != lobbyID.ConvertToUint64()) return;

    char buffer[1024];
    int bytes = SteamMatchmaking()->GetLobbyChatEntry(lobbyID, pParam->m_iChatID, nullptr, buffer, sizeof(buffer), nullptr);
    if (bytes <= 0) return;

    std::string msg(buffer);
    // Minimal use of lobby chat; most messages handled via P2P
}