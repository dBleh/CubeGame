#include "CubeGame.h"

//==============================================================================
// State Includes
//==============================================================================
#include "../States/State.h"
#include "../States/MainMenuState.h"
#include "../States/LobbyCreationState.h"
#include "../States/LobbySearchState.h"
#include "../States/LobbyState.h"
#include "../States/GameplayState.h"
#include "../States/GameOverState.h"

//==============================================================================
// Standard Library Includes
//==============================================================================
#include <memory>
#include <cmath>
#include <cstdio>    // for snprintf
#include <iostream>

//==============================================================================
// Game Identifier
//==============================================================================
const char* CubeGame::GAME_ID = "CubeShooter_v1";

//------------------------------------------------------------------------------
// Helper Functions
//------------------------------------------------------------------------------

// Resets the provided player's state to default starting values.
void CubeGame::ResetPlayerState(Player& p) {
    p.health = 100;
    p.kills = 0;
    p.money = 0;
    p.ready = false;
    // If the player's position is invalid (NaN), reset to screen center.
    if (std::isnan(p.x)) p.x = SCREEN_WIDTH / 2.f;
    if (std::isnan(p.y)) p.y = SCREEN_HEIGHT / 2.f;
    p.renderedX = p.x;
    p.renderedY = p.y;
    p.lastX = p.x;
    p.lastY = p.y;
    p.targetX = p.x;
    p.targetY = p.y;
    p.speed = PLAYER_SPEED;
    p.interpolationTime = 0.f;
    p.shape.setPosition(SCREEN_WIDTH / 2.f, SCREEN_HEIGHT / 2.f);
}

// Formats a string update for the given player state.
// Returns an empty string if formatting fails.
std::string CubeGame::FormatPlayerUpdate(const Player& p) {
    char buffer[256];
    int bytes = snprintf(buffer, sizeof(buffer),
                         "P|%llu|%.1f|%.1f|%.1f|%.1f|%d|%d|%d|%d|%.1f|%d",
                         p.steamID.ConvertToUint64(), p.x, p.y, p.renderedX, p.renderedY,
                         p.health, p.kills, p.ready ? 1 : 0, p.money, p.speed, p.isAlive ? 1 : 0);
    if (bytes > 0 && static_cast<size_t>(bytes) < sizeof(buffer))
        return std::string(buffer);
    return "";
}

//==============================================================================
// CubeGame Member Functions
//==============================================================================

//--------------------------------------
// Constructor & Initialization
//--------------------------------------
CubeGame::CubeGame() : hud(font)
{
    networkManager = new NetworkManager(debugMode, this);
    entityManager = new EntityManager();

    // Initialize Steam API unless in debug mode.
    if (!debugMode) {
        if (!SteamAPI_Init()) {
            std::cerr << "[ERROR] Steam API initialization failed!" << std::endl;
            std::exit(1);
        }
    }

    // Create game window and set framerate.
    window.create(sf::VideoMode(SCREEN_WIDTH, SCREEN_HEIGHT), "Multiplayer Lobby System");
    if (!window.isOpen()) std::exit(1);
    window.setFramerateLimit(60);

    // Load the game font.
    if (!font.loadFromFile("Roboto-Regular.ttf")) {
        std::cerr << "[ERROR] Failed to load font!" << std::endl;
    }

    // Set local Steam ID based on debug mode.
    if (!debugMode && SteamUser() && SteamUser()->BLoggedOn()) {
        localSteamID = SteamUser()->GetSteamID();
    } else if (debugMode) {
        localSteamID = CSteamID(76561198000000000ULL + 1);
    }

    // Initialize local player in the entity manager.
    entityManager->getPlayers()[localSteamID].initialize();
    entityManager->getPlayers()[localSteamID].steamID = localSteamID;

    // Set up view and timing.
    ResetViewToDefault();
    AdjustViewToWindow();
    shootCooldown = 0.0f;
    deltaTime = 0.0f;
    // Removed SetupInitialHUD – HUD elements are now set up in the appropriate states.

    // Set callback for enemy updates.
    entityManager->setEnemyUpdateCallback([this](const std::string& msg) {
        if (m_isHost) {
            networkManager->broadcastMessage(msg);
        }
    });
}

// Returns the current GameplayState if active.
GameplayState* CubeGame::GetGameplayState() {
    return dynamic_cast<GameplayState*>(state.get());
}

//--------------------------------------
// Destructor & Cleanup
//--------------------------------------
CubeGame::~CubeGame() {
    delete networkManager;
    delete entityManager;

    if (inLobby)
        SteamMatchmaking()->LeaveLobby(m_currentLobby);
    SteamAPI_Shutdown();
}

//--------------------------------------
// Accessor Methods
//--------------------------------------
float CubeGame::GetDeltaTime() const {
    return deltaTime;
}

//--------------------------------------
// Main Game Loop
//--------------------------------------
void CubeGame::Run() {
    sf::Clock clock;
    const float fixedDt = 1.0f / 60.0f; // 60 FPS fixed timestep
    float accumulator = 0.0f;

    while (window.isOpen()) {
        networkManager->processCallbacks();
        networkManager->receiveMessages();

        float frameTime = clock.restart().asSeconds();
        deltaTime = frameTime; // For rendering or other uncapped uses
        accumulator += frameTime;

        while (accumulator >= fixedDt) {
            if (m_isHost) {
                enemySyncTimer += fixedDt;
                if (enemySyncTimer >= ENEMY_SYNC_INTERVAL) {
                    networkManager->SyncEnemies();
                    enemySyncTimer = 0.0f;
                }
            }
            if (shootCooldown > 0) shootCooldown -= fixedDt;
            if (state) state->Update(fixedDt); // Use fixedDt for game logic
            accumulator -= fixedDt;
        }

        sf::Event event;
        while (window.pollEvent(event)) {
            ProcessEvents(event);
            if (state) state->ProcessEvent(event);
        }

        // State management: create new state if current state has changed.
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
            default:
                break;
        }

        // Update and render the current state.
        if (state) {
            state->Update(fixedDt);
            if (state && currentState == GetCurrentState()) {
                state->Render();
            }
        }
    }
}

//--------------------------------------
// Event Handling & View Management
//--------------------------------------
void CubeGame::ProcessEvents(sf::Event& event) {
    if (event.type == sf::Event::Closed)
        window.close();
    if (event.type == sf::Event::Resized) {
        std::cout << "[DEBUG] Resized window\n";
        AdjustViewToWindow();
    }
}

void CubeGame::ResetViewToDefault() {
    sf::Vector2u winSize = window.getSize();
    view.setSize(static_cast<float>(winSize.x), static_cast<float>(winSize.y));
    view.setCenter(static_cast<float>(winSize.x) / 2.f, static_cast<float>(winSize.y) / 2.f);
    window.setView(view);
}

void CubeGame::AdjustViewToWindow() {
    sf::Vector2u winSize = window.getSize();
    float windowWidth = static_cast<float>(winSize.x);
    float windowHeight = static_cast<float>(winSize.y);
    float targetAspect = view.getSize().x / view.getSize().y;
    float windowAspect = windowWidth / windowHeight;

    sf::FloatRect viewport(0.f, 0.f, 1.f, 1.f);
    // Adjust viewport to maintain the aspect ratio.
    if (windowAspect > targetAspect) {
        float widthRatio = targetAspect / windowAspect;
        viewport.width = widthRatio;
        viewport.left = (1.f - widthRatio) / 2.f;
    } else {
        float heightRatio = windowAspect / targetAspect;
        viewport.height = heightRatio;
        viewport.top = (1.f - heightRatio) / 2.f;
    }
    view.setViewport(viewport);
    window.setView(view);
}

//--------------------------------------
// Lobby and Player State Management
//--------------------------------------

// Toggles the ready state of the local player and notifies the lobby.
void CubeGame::ToggleReady() {
    Player& localPlayer = entityManager->getPlayers()[localSteamID];
    localPlayer.ready = !localPlayer.ready;
    SteamMatchmaking()->SetLobbyMemberData(m_currentLobby, "ready", localPlayer.ready ? "1" : "0");
    entityManager->getPlayers()[localSteamID].ready = localPlayer.ready;
    networkManager->SendPlayerUpdate();
}

// Returns to the lobby, resetting player and game state for a new session.
void CubeGame::ReturnToLobby() {
    inLobby = true;
    gameStarted = false;
    currentState = GameState::Lobby;

    // Host resets and broadcasts player states.
    if (m_isHost) {
        for (auto& [id, player] : entityManager->getPlayers()) {
            ResetPlayerState(player);
            SteamMatchmaking()->SetLobbyMemberData(m_currentLobby, "ready", "0");

            std::string updateMsg = FormatPlayerUpdate(player);
            if (!updateMsg.empty()) {
                networkManager->broadcastMessage(updateMsg);
            }
        }
    }

    // Reset local player state.
    {
        Player& localPlayer = entityManager->getPlayers()[localSteamID];
        ResetPlayerState(localPlayer);
        entityManager->getPlayers()[localSteamID] = localPlayer;
    }

    // Clear game entities and reset level parameters.
    entityManager->getEnemies().clear();
    entityManager->getBullets().clear();
    currentLevel = 0;
    enemiesPerWave = 5;
    ResetViewToDefault();
}

// Resets the game by clearing entities and resynchronizing player states.
void CubeGame::ResetGame() {
    entityManager->getEnemies().clear();
    entityManager->getBullets().clear();

    // Reset and synchronize each player's state.
    for (auto& [id, player] : entityManager->getPlayers()) {
        ResetPlayerState(player);
        player.steamID = id;  // Preserve Steam ID
        player.isAlive = true;  // Ensure player is alive after reset

        std::string updateMsg = FormatPlayerUpdate(player);
        if (!updateMsg.empty()) {
            if (m_isHost) {
                networkManager->broadcastMessage(updateMsg);  // Host broadcasts update.
            } else if (id == localSteamID) {
                // Client sends its update to the host.
                const char* hostStr = SteamMatchmaking()->GetLobbyData(m_currentLobby, "host_steam_id");
                if (hostStr && *hostStr) {
                    CSteamID hostID(std::stoull(hostStr));
                    networkManager->sendMessage(hostID, updateMsg);
                }
            }
        }
    }

    currentLevel = 0;
    enemiesPerWave = 5;
    gameStarted = false;
    nextBulletId = 0;
    processedBulletMessages.clear();
    ResetViewToDefault();
}

// Cleans up lobby state and resets to the main menu.
void CubeGame::ReturnToMainMenu() {
    if (inLobby) {
        SteamMatchmaking()->LeaveLobby(m_currentLobby);
        inLobby = false;
        m_currentLobby = k_steamIDNil;
    }
    currentState = GameState::MainMenu;
    gameStarted = false;
    hasGameBeenPlayed = false;
    entityManager->getEnemies().clear();
    entityManager->getBullets().clear();
    entityManager->getPlayers().clear();

    // Reset the local player's state.
    Player& localPlayer = entityManager->getPlayers()[localSteamID];
    ResetPlayerState(localPlayer);
    entityManager->getPlayers()[localSteamID] = localPlayer;

    currentLevel = 0;
    enemiesPerWave = 0;
    lobbyList.clear();
    ResetViewToDefault();
}

//--------------------------------------
// Game Start and Enemy Spawning
//--------------------------------------
void CubeGame::StartGame() {
    // Only start game if in lobby and not already playing.
    if (!inLobby || currentState == GameState::Playing) {
        return;
    }
    if (hasGameBeenPlayed) {
        ResetGame();  // Only reset if a game was already played.
    }
    if (m_isHost) {
        // Check that all clients are connected.
        bool allConnected = true;
        int memberCount = SteamMatchmaking()->GetNumLobbyMembers(m_currentLobby);
        for (int i = 0; i < memberCount; i++) {
            CSteamID member = SteamMatchmaking()->GetLobbyMemberByIndex(m_currentLobby, i);
            if (member != SteamUser()->GetSteamID()) {
                auto& connectedClients = networkManager->getConnectedClients();
                if (connectedClients.count(member) == 0 || !connectedClients.at(member)) {
                    allConnected = false;
                    break;
                }
            }
        }
        if (!allConnected) {
            return;
        }

        // Reset and broadcast all player states.
        for (auto& [id, player] : entityManager->getPlayers()) {
            ResetPlayerState(player);
            SteamMatchmaking()->SetLobbyMemberData(m_currentLobby, "ready", "0");

            std::string updateMsg = FormatPlayerUpdate(player);
            if (!updateMsg.empty()) {
                networkManager->broadcastMessage(updateMsg);
            }
        }

        // Reset local player state.
        Player& localPlayer = entityManager->getPlayers()[localSteamID];
        ResetPlayerState(localPlayer);
        entityManager->getPlayers()[localSteamID] = localPlayer;
        
        // Spawn enemies and sync with clients.
        entityManager->spawnEnemies(enemiesPerWave, entityManager->getPlayers(), localSteamID.ConvertToUint64());
        networkManager->SyncEnemiesFull();

        // Send game start message.
        char startBuffer[64];
        int startBytes = snprintf(startBuffer, sizeof(startBuffer), "S|START");
        if (startBytes > 0 && static_cast<size_t>(startBytes) < sizeof(startBuffer)) {
            networkManager->SendGameplayMessage(std::string(startBuffer));
        }

        // Broadcast enemy spawn messages.
        uint64_t timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()).count();

        for (auto& [eid, enemy] : entityManager->getEnemies()) {
                enemy.spawnDelay = CubeGame::INITIAL_WAVE_DELAY;
                char buffer[128];
                int bytes = snprintf(buffer, sizeof(buffer),
                                     "E|SPAWN|%llu|%.1f|%.1f|%d|%.2f|%d|%llu",
                                     eid, enemy.x, enemy.y, enemy.health, enemy.spawnDelay,
                                     static_cast<int>(enemy.type), timestamp);
                if (bytes > 0 && static_cast<size_t>(bytes) < sizeof(buffer)) {
                    networkManager->broadcastMessage(std::string(buffer));
            }
        }
    }

    // Transition to gameplay.
    currentState = GameState::Playing;
    inLobby = false;
    gameStarted = true;
    hasGameBeenPlayed = true;
    currentLevel = 1;
    enemiesPerWave = 5;
    entityManager->getBullets().clear();
    ResetViewToDefault();
}

//--------------------------------------
// Player Loading Check
//--------------------------------------
// Returns true only if every tracked player's loaded status is true.
bool CubeGame::GetPlayersAreLoaded() {
    for (auto& [steamID, isLoaded] : playerLoadedStatus) {
        if (entityManager->getPlayers().count(steamID) && !playerLoadedStatus[steamID]) {
            return false;
        }
    }
    return true;
}
