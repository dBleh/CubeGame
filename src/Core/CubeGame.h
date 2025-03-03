#ifndef CUBEGAME_H
#define CUBEGAME_H

#include <SFML/Graphics.hpp>
#include <vector>
#include <unordered_map>
#include <steam/steam_api.h>
#include <iostream>
#include "../Utils/Config.h"
#include "../Entities/Bullet.h"
#include "../Entities/Enemy.h"
#include "../States/GameState.h"
#include "../Entities/Player.h"
#include "../Hud/Hud.h"



struct CSteamIDHash {
    std::size_t operator()(const CSteamID& id) const {
        return std::hash<uint64_t>{}(id.ConvertToUint64());
    }
};

class State;

class CubeGame {
public:
    CubeGame();
    ~CubeGame();
    void Run();
    void ProcessEvents(sf::Event& event);
    void RenderHUDLayer();
    void EnterLobbyCreation();
    void CreateLobby(const std::string& lobbyName);
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

    HUD& GetHUD() { return hud; }
    sf::RenderWindow& GetWindow() { return window; }
    sf::Font& GetFont() { return font; }
    sf::View& GetView() { return view; }
    Player& GetLocalPlayer() { return localPlayer; }
    std::unordered_map<CSteamID, Player, CSteamIDHash>& GetPlayers() { return players; }
    std::unordered_map<uint64_t, Enemy>& GetEnemies() { return enemies; }
    std::unordered_map<int, Bullet>& GetBullets() { return bullets; }
    int& GetNextBulletId() { return nextBulletId; }
    CSteamID GetLobbyID() const { return lobbyID; }
    GameState& GetCurrentState() { return currentState; }
    bool IsInLobby() const { return inLobby; }
    bool IsGameStarted() const { return gameStarted; }
    bool HasGameBeenPlayed() const { return hasGameBeenPlayed; }
    int& GetCurrentLevel() { return currentLevel; }
    int& GetEnemiesPerWave() { return enemiesPerWave; }
    float& GetShootCooldown() { return shootCooldown; }
    std::vector<std::pair<CSteamID, std::string>>& GetLobbyList() { return lobbyList; }
    std::string& GetLobbyNameInput() { return lobbyNameInput; }
    ISteamNetworkingSockets* GetNetworkingSockets() { return m_pNetworkingSockets; }
    std::unordered_map<CSteamID, HSteamNetConnection, CSteamIDHash>& GetConnections() { return m_connections; }
    bool IsDebugMode() const { return debugMode; } // Added
    void SetDebugMode(bool debug) { debugMode = debug; } // Added
    void ConnectToPlayer(CSteamID player);
    void AddDebugPlayer(CSteamID id);
    float GetShoppingTimer() const { return shoppingTimer; } // New
    void SetShoppingTimer(float t) { shoppingTimer = t; }
    bool IsSteamInitialized() const {
        // For debug mode, we might skip the check. Otherwise, confirm
        // that SteamUser is valid and the user is actually logged on.
        return debugMode || (SteamUser() && SteamUser()->BLoggedOn());
    }
private:
float shoppingTimer; // New: Tracks shopping phase duration
    HUD hud;
    sf::View view;
    float shootCooldown;
    sf::RenderWindow window;
    GameState currentState = GameState::MainMenu;
    bool inLobby = false;
    bool gameStarted = false;
    CSteamID lobbyID;
    Player localPlayer;
    std::unordered_map<CSteamID, Player, CSteamIDHash> players;
    std::unordered_map<uint64_t, Enemy> enemies;
    std::unordered_map<int, Bullet> bullets;
    bool hasGameBeenPlayed = false;
    int currentLevel = 0;
    int enemiesPerWave = 5;
    std::vector<std::pair<CSteamID, std::string>> lobbyList;
    std::string lobbyNameInput;
    int nextBulletId = 0;
    sf::Font font;

    ISteamNetworkingSockets* m_pNetworkingSockets;
    std::unordered_map<CSteamID, HSteamNetConnection, CSteamIDHash> m_connections;
    bool debugMode = false; // Added
    STEAM_CALLBACK(CubeGame, OnSteamNetConnectionStatusChanged, SteamNetConnectionStatusChangedCallback_t);

    CCallResult<CubeGame, LobbyCreated_t> lobbyCreatedCallResult;
    CCallResult<CubeGame, LobbyEnter_t> lobbyEnterCallResult;
    CCallResult<CubeGame, LobbyMatchList_t> lobbyListCallResult;
    CCallback<CubeGame, LobbyChatUpdate_t> lobbyChatUpdateCallback{this, &CubeGame::OnLobbyChatUpdate};
    CCallback<CubeGame, LobbyChatMsg_t> lobbyChatMsgCallback{this, &CubeGame::OnLobbyChatMsg};

    void SetupInitialHUD();
    void ResetViewToDefault();
    void ProcessNetworkMessages();
    void ProcessMessage(const std::string& msg, CSteamID sender);
    void OnLobbyCreated(LobbyCreated_t* pResult, bool bIOFailure);
    void OnLobbyEntered(LobbyEnter_t* pResult, bool bIOFailure);
    void OnLobbyListReceived(LobbyMatchList_t* pResult, bool bIOFailure);
    void OnLobbyChatUpdate(LobbyChatUpdate_t* pParam);
    void OnLobbyChatMsg(LobbyChatMsg_t* pParam);

    static const char* GAME_ID;
};

#endif // CUBEGAME_H