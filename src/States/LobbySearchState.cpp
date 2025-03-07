#include "LobbySearchState.h"
#include "../Core/CubeGame.h"
#include <iostream>

LobbySearchState::LobbySearchState(CubeGame* game) : State(game) {
    // Initiate the lobby search when entering this state.
    SearchLobbies();
    game->GetHUD().addElement("searchStatus", "Searching...", 18, sf::Vector2f(20.f, 20.f),
    GameState::LobbySearch, HUD::RenderMode::ScreenSpace, false);
    game->GetHUD().addElement("lobbyList", "Available Lobbies:\n", 20, sf::Vector2f(50.f, 100.f),
    GameState::LobbySearch, HUD::RenderMode::ScreenSpace, false);
    game->GetHUD().updateText("lobbyList", "Available Lobbies:\n"); // Reset display text
}

void LobbySearchState::SearchLobbies() {
    if (game->IsInLobby()) return;
    // Clear the lobby list
    game->GetLobbyList().clear();
    // Add filters for this game using the game's GAME_ID
    SteamMatchmaking()->AddRequestLobbyListStringFilter("game_id", CubeGame::GAME_ID, k_ELobbyComparisonEqual);
    SteamMatchmaking()->AddRequestLobbyListStringFilter("name", "", k_ELobbyComparisonNotEqual);
    // Request the lobby list from Steam
    SteamAPICall_t call = SteamMatchmaking()->RequestLobbyList();
    if (call == k_uAPICallInvalid) {
        std::cerr << "[LOBBY] RequestLobbyList failed.\n";
        game->GetHUD().updateText("searchStatus", "Failed to search lobbies");
    } else {
        game->GetHUD().updateText("searchStatus", "Searching...");
    }
}

void LobbySearchState::Update(float dt) {
    if (game->IsLobbyListUpdated()) { 
        UpdateLobbyListDisplay();
        game->lobbyListUpdated = false; // Reset flag (ensure lobbyListUpdated is accessible)
    }
}

void LobbySearchState::Render() {
    game->GetWindow().clear(sf::Color::White);
    sf::RectangleShape headerBar;
    headerBar.setSize(sf::Vector2f(SCREEN_WIDTH, 60.f));
    headerBar.setFillColor(sf::Color(70, 130, 180)); // Steel blue
    headerBar.setPosition(0.f, 0.f);
    game->GetWindow().draw(headerBar);
    game->GetHUD().render(game->GetWindow(), game->GetWindow().getDefaultView(), game->GetCurrentState());

    game->GetWindow().display();
}

void LobbySearchState::ProcessEvent(const sf::Event& event) {
    ProcessEvents(event);
}

void LobbySearchState::ProcessEvents(const sf::Event& event) {
    if (event.type == sf::Event::KeyPressed) {
        if (event.key.code >= sf::Keyboard::Num0 && event.key.code <= sf::Keyboard::Num9) {
            int index = event.key.code - sf::Keyboard::Num0;
            JoinLobbyByIndex(index);
        } else if (event.key.code == sf::Keyboard::Escape) {
            game->ReturnToMainMenu();
        }
    }
}


void LobbySearchState::UpdateLobbyListDisplay() {
    std::string lobbyText = "Available Lobbies (Press 0-9 to join, ESC to cancel):\n";
    const auto& lobbies = game->GetLobbyList();
    for (size_t i = 0; i < lobbies.size() && i < 10; ++i) {
        lobbyText += std::to_string(i) + ": " + lobbies[i].second + "\n";
    }
    if (lobbies.empty()) {
        lobbyText += "No lobbies available.";
    }
    game->GetHUD().updateText("lobbyList", lobbyText);
}
void LobbySearchState::JoinLobby(CSteamID lobby) {
    if (game->IsInLobby()) return;
    // Mark as client
    game->SetIsHost(false);
    SteamAPICall_t call = SteamMatchmaking()->JoinLobby(lobby);
    if (call == k_uAPICallInvalid) {
        std::cerr << "[LOBBY] JoinLobby call failed immediately for " << lobby.ConvertToUint64() << "\n";
        game->SetCurrentState(GameState::MainMenu);
    }
}

void LobbySearchState::JoinLobbyByIndex(int index) {
    const auto& lobbies = game->GetLobbyList();
    if (index >= 0 && index < static_cast<int>(lobbies.size())) {
        JoinLobby(lobbies[index].first);
    } else {
        game->GetHUD().updateText("searchStatus", "Invalid lobby selection");
    }
}