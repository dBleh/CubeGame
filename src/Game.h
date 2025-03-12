#ifndef GAME_H
#define GAME_H

#include <SFML/Graphics.hpp>
#include "../hud/Hud.h"
#include "../network/NetworkManager.h"
#include "GameState.h"
#include <memory>
#include <steam/steam_api.h>  // Include Steam API header

class State;
class NetworkManager;
class Game {
public:
    Game();
    ~Game();

    void Run();
    void SetCurrentState(GameState state);
    GameState GetCurrentState() const { return currentState; }
    sf::RenderWindow& GetWindow() { return window; }
    HUD& GetHUD() { return hud; }
    NetworkManager& GetNetworkManager() { return *networkManager; }
    std::string& GetLobbyNameInput() { return lobbyNameInput; }
    bool IsSteamInitialized() const { return steamInitialized; }
    bool IsInLobby() const { return inLobby; }
    CSteamID GetLobbyID() const { return currentLobby; }
    
    // --- Steam Connection Variables ---
    // Local Steam ID getter and setter.
    void SetLocalSteamID(const CSteamID& id) { localSteamID = id; }
    CSteamID GetLocalSteamID() const { return localSteamID; }

    const sf::Font& GetFont() const { return font; }
private:
    void ProcessEvents(sf::Event& event);
    void AdjustViewToWindow();

    sf::RenderWindow window;
    sf::Font font;
    HUD hud;
    std::unique_ptr<State> state;
    std::unique_ptr<NetworkManager> networkManager;

    GameState currentState{GameState::MainMenu};
    bool steamInitialized{false};
    bool inLobby{false};
    CSteamID currentLobby{k_steamIDNil};
    std::string lobbyNameInput;

    // --- New Steam Connection Variable ---
    CSteamID localSteamID{k_steamIDNil};
};

#endif // GAME_H
