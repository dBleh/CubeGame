#ifndef CUBEGAME_H
#define CUBEGAME_H

//==============================================================================
// Library Includes
//==============================================================================
#include <SFML/Graphics.hpp>
#include <steam/steam_api.h>
#include <steam/isteamnetworking.h>
#include <steam/isteammatchmaking.h>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <iostream>
#include <conio.h>
#include <chrono>

//==============================================================================
// Project Includes
//==============================================================================
#include "../Networking/NetworkManager.h"
#include "../Entities/EntityManager.h"
#include "../Utils/Config.h"
#include "../Entities/Bullet.h"
#include "../Entities/Enemy.h"
#include "../States/GameState.h"
#include "../Entities/Player.h"
#include "../Hud/Hud.h"

// Forward declaration of State classes.
class State;
class GameplayState;

/**
 * @brief The CubeGame class is the central class for the CubeShooter game.
 *
 * It manages game states, networking, entity updates, and rendering.
 */
class CubeGame {
public:
    //--------------------------------------------------------------------------
    // Public Helper Functions
    //--------------------------------------------------------------------------
    std::string FormatPlayerUpdate(const Player& p);
    void ResetPlayerState(Player& p);

    // Allow NetworkManager direct access to private members.
    friend class NetworkManager;

    // Returns the current GameplayState (if active).
    GameplayState* GetGameplayState();

    //--------------------------------------------------------------------------
    // Constructors & Destructors
    //--------------------------------------------------------------------------
    CubeGame();
    ~CubeGame();

    //--------------------------------------------------------------------------
    // Main Game Loop
    //--------------------------------------------------------------------------
    void Run();
    void SetIsHost(bool isHost) { m_isHost = isHost; }

    /**
     * @brief Checks if the local instance is the host.
     * @return true if host, false otherwise.
     */
    bool IsHost() const { return m_isHost; }

    //--------------------------------------------------------------------------
    // Public Networking and Rendering Methods
    //--------------------------------------------------------------------------
    /**
     * @brief Sends a message to a specified target Steam user.
     * @param target The target Steam user.
     * @param msg The message string.
     */
    void SendMessage(CSteamID target, const std::string& msg);

    /**
     * @brief Renders the HUD layer on the window.
     */
    void RenderHUDLayer();

    //--------------------------------------------------------------------------
    // Accessor Methods
    //--------------------------------------------------------------------------
    HUD& GetHUD() { return hud; }
    sf::RenderWindow& GetWindow() { return window; }
    sf::Font& GetFont() { return font; }
    sf::View& GetView() { return view; }
    const std::vector<std::pair<CSteamID, std::string>>& GetLobbyList() const { return lobbyList; }
    bool IsLobbyListUpdated() const { return lobbyListUpdated; }
    std::unordered_map<CSteamID, Player, CSteamIDHash>& GetPlayers() { return entityManager->getPlayers(); }
    std::unordered_map<uint64_t, Enemy>& GetEnemies() { return entityManager->getEnemies(); }
    std::unordered_map<uint64_t, Bullet>& GetBullets() { return entityManager->getBullets(); }
    int& GetNextBulletId() { return nextBulletId; }
    CSteamID GetLobbyID() const { return m_currentLobby; }
    GameState& GetCurrentState() { return currentState; }
    std::unordered_map<CSteamID, bool, CSteamIDHash> playerLoadedStatus;
    static constexpr float INITIAL_WAVE_DELAY = 3.0f;

    /**
     * @brief Sets the current game state.
     * @param state The new game state.
     */
    void SetCurrentState(GameState state) { currentState = state; }

    bool IsInLobby() const { return inLobby; }
    bool IsGameStarted() const { return gameStarted; }
    bool HasGameBeenPlayed() const { return hasGameBeenPlayed; }
    int& GetCurrentLevel() { return currentLevel; }
    int& GetEnemiesPerWave() { return enemiesPerWave; }
    float& GetShootCooldown() { return shootCooldown; }
    std::vector<std::pair<CSteamID, std::string>>& GetLobbyList() { return lobbyList; }
    std::string& GetLobbyNameInput() { return lobbyNameInput; }
    bool IsDebugMode() const { return debugMode; }
    void SetDebugMode(bool debug) { debugMode = debug; }
    float GetShoppingTimer() const { return shoppingTimer; }
    void SetShoppingTimer(float t) { shoppingTimer = t; }

    /**
     * @brief Checks if Steam is properly initialized.
     * @return true if Steam is initialized or in debug mode, false otherwise.
     */
    bool IsSteamInitialized() const { return debugMode || (SteamUser() && SteamUser()->BLoggedOn()); }

    //--------------------------------------------------------------------------
    // Game State Transition Methods
    //--------------------------------------------------------------------------
    Player& GetLocalPlayer() { return entityManager->getPlayers()[localSteamID]; }
    void EnterLobbyCreation();
    void EnterLobbySearch();
    void SearchLobbies();
    void JoinLobby(CSteamID lobby);
    void JoinLobbyByIndex(int index);
    void ToggleReady();
    void SendPlayerUpdate();
    void ResetLobby();
    void ResetGame();
    void ReturnToLobby();
    void ReturnToMainMenu();
    void StartGame();
    bool GetPlayersAreLoaded();

    //--------------------------------------------------------------------------
    // Manager Accessors
    //--------------------------------------------------------------------------
    void SyncEnemies();
    NetworkManager* GetNetworkManager() { return networkManager; }
    EntityManager* GetEntityManager() { return entityManager; }
    bool AllPlayersReady();
    bool lobbyListUpdated = false; // Flag for lobby list update.
    CSteamID GetCurrentLobby() const { return m_currentLobby; }
    float GetDeltaTime() const;

    //--------------------------------------------------------------------------
    // Rendering Helpers
    //--------------------------------------------------------------------------
    void AdjustViewToWindow();

    //--------------------------------------------------------------------------
    // Game Identifier Constant
    //--------------------------------------------------------------------------
    static const char* GAME_ID;
    
private:
    //--------------------------------------------------------------------------
    // Game Timing & State Variables
    //--------------------------------------------------------------------------
    float deltaTime;
    bool m_isHost = false;
    float shoppingTimer = 0.0f;
    GameState currentState = GameState::MainMenu;
    bool inLobby = false;
    bool gameStarted = false;
    bool hasGameBeenPlayed = false;
    int currentLevel = 0;
    int enemiesPerWave = 5;
    int nextBulletId = 0;
    bool debugMode = false;
    float shootCooldown = 0.0f;
    float enemySyncTimer = 0.0f;
    const float ENEMY_SYNC_INTERVAL = 3.0f;

    //--------------------------------------------------------------------------
    // Steam & Lobby Related Variables
    //--------------------------------------------------------------------------
    CSteamID m_currentLobby = k_steamIDNil;
    CSteamID m_hostID = k_steamIDNil;
    int m_joinAttempts = 0;
    std::vector<std::pair<CSteamID, std::string>> lobbyList;
    std::string lobbyNameInput;
    std::unique_ptr<State> state;
   
    //--------------------------------------------------------------------------
    // HUD, Rendering and View Management
    //--------------------------------------------------------------------------
    HUD hud;
    sf::View view;
    sf::RenderWindow window;
    sf::Font font;

    //--------------------------------------------------------------------------
    // Local Player & Processed Messages
    //--------------------------------------------------------------------------
    CSteamID localSteamID;
    std::unordered_set<uint32_t> processedBulletMessages; // Tracks processed bullet messages.

    //--------------------------------------------------------------------------
    // Manager Pointers
    //--------------------------------------------------------------------------
    NetworkManager* networkManager = nullptr;
    EntityManager* entityManager = nullptr;

    //--------------------------------------------------------------------------
    // Private Helper Methods
    //--------------------------------------------------------------------------
    void ProcessEvents(sf::Event& event);
    void SetupInitialHUD();  // Legacy method (no longer used as HUD setup is state-based).
    void ResetViewToDefault();
    void ProcessNetworkMessages(const std::string& msg, CSteamID sender);
    void CheckUserInput();
};

#endif // CUBEGAME_H
