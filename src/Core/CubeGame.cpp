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

// HUD Implementation
HUD::HUD(sf::Font& font) : font(font) {}

void HUD::addElement(const std::string& id, const std::string& content, unsigned int size, 
                     sf::Vector2f pos, GameState visibleState, RenderMode mode) {
    sf::Text text;
    text.setFont(font);
    text.setString(content);
    text.setCharacterSize(size);
    text.setFillColor(sf::Color::White);
    elements[id] = {text, pos, visibleState, mode};
}

void HUD::updateText(const std::string& id, const std::string& content) {
    if (elements.find(id) != elements.end()) {
        elements[id].text.setString(content);
    }
}

void HUD::render(sf::RenderWindow& window, const sf::View& view, GameState currentState) {
    sf::Vector2f viewTopLeft = view.getCenter() - view.getSize() / 2.f;
    for (auto& [id, element] : elements) {
        if (element.visibleState == currentState) {
            if (element.mode == RenderMode::ScreenSpace) {
                element.text.setPosition(element.pos);
            } else {
                element.text.setPosition(viewTopLeft + element.pos);
            }
            window.draw(element.text);
        }
    }
}

// CubeGame Implementation
CubeGame::CubeGame() : hud(font) {
    SteamManager::Initialize();
    window.create(sf::VideoMode(SCREEN_WIDTH, SCREEN_HEIGHT), "Multiplayer Lobby System");
    if (!window.isOpen()) {
        std::exit(1);
    }
    window.setFramerateLimit(60);

    if (!font.loadFromFile("Roboto-Regular.ttf")) {
        std::cerr << "[ERROR] Failed to load font!" << std::endl;
    }

    localPlayer.initialize();
    if (SteamUser() && SteamUser()->BLoggedOn()) {
        localPlayer.steamID = SteamUser()->GetSteamID();
    }

    ResetViewToDefault();
    shootCooldown = 0.0f;
    SetupInitialHUD();
}

CubeGame::~CubeGame() {
    if (inLobby) {
        SteamMatchmaking()->LeaveLobby(lobbyID);
    }
    SteamAPI_Shutdown();
}

void CubeGame::Run() {
    sf::Clock clock;
    std::unique_ptr<State> state;

    while (window.isOpen()) {
        SteamAPI_RunCallbacks();
        float dt = clock.restart().asSeconds();
        dt = std::min(dt, 0.033f);

        sf::Event event;
        while (window.pollEvent(event)) {
            ProcessEvents(event);
            if (state) {
                state->ProcessEvent(event);
            }
        }

        switch (currentState) {
            case GameState::MainMenu:
                if (!state || dynamic_cast<MainMenuState*>(state.get()) == nullptr) {
                    state = std::make_unique<MainMenuState>(this);
                }
                break;
            case GameState::LobbyCreation:
                if (!state || dynamic_cast<LobbyCreationState*>(state.get()) == nullptr) {
                    state = std::make_unique<LobbyCreationState>(this);
                }
                break;
            case GameState::LobbySearch:
                if (!state || dynamic_cast<LobbySearchState*>(state.get()) == nullptr) {
                    state = std::make_unique<LobbySearchState>(this);
                }
                break;
            case GameState::Lobby:
                if (!state || dynamic_cast<LobbyState*>(state.get()) == nullptr) {
                    state = std::make_unique<LobbyState>(this);
                }
                break;
            case GameState::Playing:
                if (!state || dynamic_cast<GameplayState*>(state.get()) == nullptr) {
                    state = std::make_unique<GameplayState>(this);
                }
                break;
            case GameState::GameOver:
                if (!state || dynamic_cast<GameOverState*>(state.get()) == nullptr) {
                    state = std::make_unique<GameOverState>(this);
                }
                break;
            case GameState::Spectating:
            case GameState::Leaderboard:
                break;
        }

        if (state) {
            state->Update(dt);
            if (state && this->GetCurrentState() == currentState) state->Render();
            if (currentState == GameState::Playing && localPlayer.steamID == SteamMatchmaking()->GetLobbyOwner(lobbyID)) {
                SendGameStateUpdate();
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

void CubeGame::SetupInitialHUD() {
    hud.addElement("status", "Main Menu", 16, sf::Vector2f(10, 10), GameState::MainMenu, HUD::RenderMode::ScreenSpace);
    hud.addElement("createLobby", "1. Create Lobby", 24, sf::Vector2f(SCREEN_WIDTH/2 - 100, 200), GameState::MainMenu, HUD::RenderMode::ScreenSpace);
    hud.addElement("joinLobby", "2. Search Lobbies", 24, sf::Vector2f(SCREEN_WIDTH/2 - 100, 250), GameState::MainMenu, HUD::RenderMode::ScreenSpace);
    hud.addElement("lobbyPrompt", "Enter Lobby Name (Press Enter to Create, ESC to Cancel): ", 20, sf::Vector2f(SCREEN_WIDTH/2 - 200, 200), GameState::LobbyCreation, HUD::RenderMode::ScreenSpace);
    hud.addElement("searchStatus", "Searching...", 16, sf::Vector2f(10, 10), GameState::LobbySearch, HUD::RenderMode::ScreenSpace);
    hud.addElement("lobbyList", "Available Lobbies:\n", 20, sf::Vector2f(SCREEN_WIDTH/2 - 150, 100), GameState::LobbySearch, HUD::RenderMode::ScreenSpace);
    
    hud.addElement("lobbyStatus", "Lobby", 16, sf::Vector2f(10, 10), GameState::Lobby, HUD::RenderMode::ScreenSpace);
    hud.addElement("ready", "Press R to Toggle Ready", 20, sf::Vector2f(SCREEN_WIDTH/2 - 100, 300), GameState::Lobby, HUD::RenderMode::ScreenSpace);
    hud.addElement("startGame", "Press S to Start Game (Host Only)", 20, sf::Vector2f(SCREEN_WIDTH/2 - 150, 350), GameState::Lobby, HUD::RenderMode::ScreenSpace);
    hud.addElement("returnMain", "Press M to Return to Main Menu", 20, sf::Vector2f(SCREEN_WIDTH/2 - 150, 400), GameState::Lobby, HUD::RenderMode::ScreenSpace);
    
    hud.addElement("gameStatus", "Playing", 16, sf::Vector2f(10, 10), GameState::Playing, HUD::RenderMode::ViewSpace);
    hud.addElement("level", "Level: 0\nEnemies: 0\nHP: 100", 16, sf::Vector2f(10, 50), GameState::Playing, HUD::RenderMode::ViewSpace);
    hud.addElement("scoreboard", "Scoreboard:\n", 16, sf::Vector2f(SCREEN_WIDTH - 200, 10), GameState::Playing, HUD::RenderMode::ViewSpace);
    hud.addElement("menu", "", 20, sf::Vector2f(10, 300), GameState::Playing, HUD::RenderMode::ViewSpace);
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
    if (inLobby) return;
    currentState = GameState::LobbyCreation;
    lobbyNameInput.clear();
    hud.updateText("lobbyPrompt", "Enter Lobby Name (Press Enter to Create, ESC to Cancel): ");
    std::cout << "[DEBUG] Entered Lobby Creation state" << std::endl;
}

void CubeGame::CreateLobby(const std::string& lobbyName) {
    if (inLobby) return;
    SteamAPICall_t call = SteamMatchmaking()->CreateLobby(k_ELobbyTypePublic, 4);
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
        SteamMatchmaking()->SendLobbyChatMsg(lobbyID, buffer, bytes + 1);
    }
}

void CubeGame::SendGameStateUpdate() {
    if (!inLobby || currentState != GameState::Playing) return;

    std::string stateMsg = "G|";
    for (const auto& [id, p] : players) {
        float x = std::isnan(p.x) ? 0.0f : p.x;
        float y = std::isnan(p.y) ? 0.0f : p.y;
        stateMsg += std::to_string(id.ConvertToUint64()) + "," + 
                    std::to_string(static_cast<int>(x)) + "," + std::to_string(static_cast<int>(y)) + "," + 
                    std::to_string(p.health) + "," + std::to_string(p.kills) + "," + 
                    (p.isAlive ? "1" : "0") + ";";
    }
    stateMsg += "|";
    for (const auto& [id, e] : enemies) {
        float x = std::isnan(e.x) ? 0.0f : e.x;
        float y = std::isnan(e.y) ? 0.0f : e.y;
        stateMsg += std::to_string(id) + "," + std::to_string(static_cast<int>(x)) + "," + 
                    std::to_string(static_cast<int>(y)) + "," + std::to_string(e.health) + "," +
                    std::to_string(static_cast<int>(e.spawnDelay * 1000)) + ";";
    }
    stateMsg += "|";
    for (const auto& [id, b] : bullets) {
        float x = std::isnan(b.x) ? 0.0f : b.x;
        float y = std::isnan(b.y) ? 0.0f : b.y;
        float vx = std::isnan(b.velocityX) ? 0.0f : b.velocityX;
        float vy = std::isnan(b.velocityY) ? 0.0f : b.velocityY;
        int shooterID = (b.id >> 16) & 0xFFFF;
        int bulletIdx = b.id & 0xFFFF;
        stateMsg += std::to_string(shooterID) + "," + std::to_string(bulletIdx) + "," +
                    std::to_string(static_cast<int>(x)) + "," + std::to_string(static_cast<int>(y)) + "," +
                    std::to_string(static_cast<int>(vx)) + "," + std::to_string(static_cast<int>(vy)) + ";";
    }

    char buffer[4096];
    int bytes = snprintf(buffer, sizeof(buffer), "%s", stateMsg.c_str());
    if (bytes > 0 && static_cast<size_t>(bytes) < sizeof(buffer)) {
        SteamMatchmaking()->SendLobbyChatMsg(lobbyID, buffer, bytes + 1);
        std::cout << "[DEBUG] Sent game state message, size: " << bytes << " bytes" << std::endl;
    } else {
        std::cout << "[ERROR] Game state message too large or failed: " << bytes << std::endl;
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

    // Send S|START once here
    char startBuffer[16];
    int startBytes = snprintf(startBuffer, sizeof(startBuffer), "S|START");
    if (startBytes > 0 && static_cast<size_t>(startBytes) < sizeof(startBuffer)) {
        SteamMatchmaking()->SendLobbyChatMsg(lobbyID, startBuffer, startBytes + 1);
        std::cout << "[DEBUG] Sent game start signal" << std::endl;
    }
    SendGameStateUpdate();
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
        // Handled by LobbyState
    }
}

void CubeGame::OnLobbyChatMsg(LobbyChatMsg_t* pParam) {
    if (pParam->m_ulSteamIDLobby != lobbyID.ConvertToUint64()) return;

    char buffer[1024];
    int bytes = SteamMatchmaking()->GetLobbyChatEntry(lobbyID, pParam->m_iChatID, nullptr, buffer, sizeof(buffer), nullptr);
    if (bytes <= 0) {
        std::cerr << "[ERROR] Failed to receive lobby chat message" << std::endl;
        return;
    }

    std::string msg(buffer);

    if (msg[0] == 'P') {
        uint64_t steamID;
        float x, y;
        int health, kills, ready;
        if (sscanf(msg.c_str(), "P|%llu|%f|%f|%d|%d|%d", &steamID, &x, &y, &health, &kills, &ready) == 6) {
            CSteamID id(steamID);
            if (id != localPlayer.steamID) {
                Player& p = players[id];
                if (!players.count(id)) p.initialize();
                p.x = std::isnan(x) ? (players.count(id) ? players[id].x : 0.0f) : x;
                p.y = std::isnan(y) ? (players.count(id) ? players[id].y : 0.0f) : y;
                p.health = health;
                p.kills = kills;
                p.ready = ready;
                std::cout << "[DEBUG] Updated player " << steamID << " target position: (" << p.x << ", " << p.y << ")" << std::endl;
            }
        }
    }
    else if (msg == "S|START") {
        if (currentState != GameState::Playing) {
            StartGame();
            std::cout << "[DEBUG] Received S|START signal, transitioning to Playing state" << std::endl;
        } else {
            std::cout << "[DEBUG] Ignored S|START, already in Playing state" << std::endl;
        }
    }
    else if (msg[0] == 'B') {
        uint64_t shooterID;
        int bulletIdx;
        float x, y, vx, vy;
        if (sscanf(msg.c_str(), "B|%llu|%d|%f|%f|%f|%f", &shooterID, &bulletIdx, &x, &y, &vx, &vy) == 6) {
            int uniqueBulletId = static_cast<int>((shooterID & 0xFFFF) << 16 | (bulletIdx & 0xFFFF));
            if (!bullets.count(uniqueBulletId)) {
                Bullet b;
                b.initialize(x, y, x + vx, y + vy);
                b.x = std::isnan(x) ? 0.0f : x;
                b.y = std::isnan(y) ? 0.0f : y;
                b.velocityX = std::isnan(vx) ? 0.0f : vx;
                b.velocityY = std::isnan(vy) ? 0.0f : vy;
                b.lifetime = 2.0f;
                b.id = uniqueBulletId;
                bullets[uniqueBulletId] = b;
                std::cout << "[DEBUG] Added client bullet " << uniqueBulletId << " at (" << b.x << ", " << b.y << ")" << std::endl;
            }
        }
    }
    else if (msg[0] == 'G') {
        if (currentState != GameState::Playing) {
            StartGame();
            std::cout << "[DEBUG] Received G| message, forcing transition to Playing state" << std::endl;
        }

        std::vector<std::string> parts;
        size_t pos = 0;
        std::string tempMsg = msg;
        while ((pos = tempMsg.find('|')) != std::string::npos) {
            parts.push_back(tempMsg.substr(0, pos));
            tempMsg.erase(0, pos + 1);
        }
        parts.push_back(tempMsg);

        if (parts.size() < 4) {
            std::cerr << "[ERROR] Invalid game state message format" << std::endl;
            return;
        }

        std::string playerData = parts[1];
        std::unordered_map<CSteamID, Player, CSteamIDHash> newPlayers;
        pos = 0;
        while ((pos = playerData.find(';')) != std::string::npos) {
            std::string pData = playerData.substr(0, pos);
            uint64_t steamID;
            float x = 0.0f, y = 0.0f;
            int health = 0, kills = 0, alive = 0;
            if (sscanf(pData.c_str(), "%llu,%f,%f,%d,%d,%d", &steamID, &x, &y, &health, &kills, &alive) == 6) {
                CSteamID id(steamID);
                Player p = players.count(id) ? players[id] : Player();
                if (!players.count(id)) p.initialize();
                p.steamID = id;
                p.x = std::isnan(x) ? (players.count(id) ? players[id].x : 0.0f) : x;
                p.y = std::isnan(y) ? (players.count(id) ? players[id].y : 0.0f) : y;
                p.health = health;
                p.kills = kills;
                p.isAlive = alive;
                newPlayers[id] = p;
            }
            playerData.erase(0, pos + 1);
        }
        players = newPlayers;
        if (players.count(localPlayer.steamID)) {
            localPlayer.health = players[localPlayer.steamID].health;
            localPlayer.kills = players[localPlayer.steamID].kills;
            localPlayer.isAlive = players[localPlayer.steamID].isAlive;
        }
        players[localPlayer.steamID] = localPlayer;

        std::string enemyData = parts[2];
        pos = 0;
        while ((pos = enemyData.find(';')) != std::string::npos) {
            std::string eData = enemyData.substr(0, pos);
            uint64_t id;
            float x = 0.0f, y = 0.0f;
            int health, spawnDelayMs;
            if (sscanf(eData.c_str(), "%llu,%f,%f,%d,%d", &id, &x, &y, &health, &spawnDelayMs) == 5) {
                if (enemies.count(id)) {
                    Enemy& e = enemies[id];
                    float oldX = e.x, oldY = e.y;
                    e.x = std::isnan(x) ? e.x : e.x + (x - e.x) * 0.5f;
                    e.y = std::isnan(y) ? e.y : e.y + (y - e.y) * 0.5f;
                    e.health = health;
                    e.spawnDelay = spawnDelayMs / 1000.0f;
                    std::cout << "[DEBUG] Updated enemy " << id << " from (" << oldX << ", " << oldY << ") to (" << e.x << ", " << e.y << "), spawnDelay: " << e.spawnDelay << std::endl;
                } else {
                    Enemy e;
                    e.initialize();
                    e.x = std::isnan(x) ? 0.0f : x;
                    e.y = std::isnan(y) ? 0.0f : y;
                    e.renderedX = e.x;
                    e.renderedY = e.y;
                    e.health = health;
                    e.spawnDelay = spawnDelayMs / 1000.0f;
                    e.id = id;
                    enemies[id] = e;
                    std::cout << "[DEBUG] Added new enemy " << id << " at (" << e.x << ", " << e.y << "), spawnDelay: " << e.spawnDelay << std::endl;
                }
            }
            enemyData.erase(0, pos + 1);
        }
        for (auto it = enemies.begin(); it != enemies.end();) {
            if (enemyData.find(std::to_string(it->first)) == std::string::npos && !enemyData.empty()) {
                std::cout << "[DEBUG] Removed enemy " << it->first << " at (" << it->second.x << ", " << it->second.y << ")" << std::endl;
                it = enemies.erase(it);
            } else {
                ++it;
            }
        }

        std::string bulletData = parts[3];
        pos = 0;
        while ((pos = bulletData.find(';')) != std::string::npos) {
            std::string bData = bulletData.substr(0, pos);
            uint64_t shooterID;
            int bulletIdx;
            float x = 0.0f, y = 0.0f, vx = 0.0f, vy = 0.0f;
            if (sscanf(bData.c_str(), "%llu,%d,%f,%f,%f,%f", &shooterID, &bulletIdx, &x, &y, &vx, &vy) == 6) {
                int uniqueBulletId = static_cast<int>((shooterID & 0xFFFF) << 16 | (bulletIdx & 0xFFFF));
                if (bullets.count(uniqueBulletId)) {
                    Bullet& b = bullets[uniqueBulletId];
                    float oldX = b.x, oldY = b.y;
                    b.x = std::isnan(x) ? b.x : b.x + (x - b.x) * 0.2f;
                    b.y = std::isnan(y) ? b.y : b.y + (y - b.y) * 0.2f;
                    b.velocityX = std::isnan(vx) ? b.velocityX : vx;
                    b.velocityY = std::isnan(vy) ? b.velocityY : vy;
                    std::cout << "[DEBUG] Updated bullet " << uniqueBulletId << " from (" << oldX << ", " << oldY << ") to (" << b.x << ", " << b.y << ")" << std::endl;
                } else {
                    Bullet b;
                    b.initialize(x, y, x + vx, y + vy);
                    b.x = std::isnan(x) ? 0.0f : x;
                    b.y = std::isnan(y) ? 0.0f : y;
                    b.velocityX = std::isnan(vx) ? 0.0f : vx;
                    b.velocityY = std::isnan(vy) ? 0.0f : vy;
                    b.lifetime = 2.0f;
                    b.id = uniqueBulletId;
                    bullets[uniqueBulletId] = b;
                    std::cout << "[DEBUG] Added new bullet " << uniqueBulletId << " at (" << b.x << ", " << b.y << ")" << std::endl;
                }
            }
            bulletData.erase(0, pos + 1);
        }
        for (auto it = bullets.begin(); it != bullets.end();) {
            bool found = false;
            pos = 0;
            std::string tempBulletData = parts[3];
            while ((pos = tempBulletData.find(';')) != std::string::npos) {
                std::string bData = tempBulletData.substr(0, pos);
                uint64_t shooterID;
                int bulletIdx;
                float x, y, vx, vy;
                if (sscanf(bData.c_str(), "%llu,%d,%f,%f,%f,%f", &shooterID, &bulletIdx, &x, &y, &vx, &vy) == 6) {
                    int uniqueBulletId = static_cast<int>((shooterID & 0xFFFF) << 16 | (bulletIdx & 0xFFFF));
                    if (uniqueBulletId == it->first) {
                        found = true;
                        break;
                    }
                }
                tempBulletData.erase(0, pos + 1);
            }
            if (!found && !bulletData.empty()) {
                std::cout << "[DEBUG] Removed bullet " << it->first << " at (" << it->second.x << ", " << it->second.y << ")" << std::endl;
                it = bullets.erase(it);
            } else {
                ++it;
            }
        }
    }
}