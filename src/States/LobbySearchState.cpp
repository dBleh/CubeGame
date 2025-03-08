#include "LobbySearchState.h"
#include "../Core/CubeGame.h"
#include <iostream>

//---------------------------------------------------------
// Constructor: Initiate lobby search and set up HUD elements.
//---------------------------------------------------------
LobbySearchState::LobbySearchState(CubeGame* game) : State(game) {
    // Start searching for lobbies.
    SearchLobbies();

    // Add HUD element to show search status.
    game->GetHUD().addElement("searchStatus", "Searching...", 18, 
                               sf::Vector2f(20.f, 20.f),
                               GameState::LobbySearch, 
                               HUD::RenderMode::ScreenSpace, false);
    // Add HUD element to list available lobbies.
    game->GetHUD().addElement("lobbyList", "Available Lobbies:\n", 20, 
                               sf::Vector2f(50.f, 100.f),
                               GameState::LobbySearch, 
                               HUD::RenderMode::ScreenSpace, false);
    // Reset lobby list display text.
    game->GetHUD().updateText("lobbyList", "Available Lobbies:\n");
}

//---------------------------------------------------------
// SearchLobbies: Request available lobbies from Steam.
//---------------------------------------------------------
void LobbySearchState::SearchLobbies() {
    if (game->IsInLobby()) return; // Do not search if already in a lobby.
    game->GetLobbyList().clear();

    // Filter lobbies for this game using the game's GAME_ID.
    SteamMatchmaking()->AddRequestLobbyListStringFilter("game_id", CubeGame::GAME_ID, k_ELobbyComparisonEqual);
    SteamMatchmaking()->AddRequestLobbyListStringFilter("name", "", k_ELobbyComparisonNotEqual);

    // Request the lobby list.
    SteamAPICall_t call = SteamMatchmaking()->RequestLobbyList();
    if (call == k_uAPICallInvalid) {
        std::cerr << "[LOBBY] RequestLobbyList failed.\n";
        game->GetHUD().updateText("searchStatus", "Failed to search lobbies");
    } else {
        game->GetHUD().updateText("searchStatus", "Searching...");
    }
}

//---------------------------------------------------------
// Update: Refresh lobby list display if updated.
//---------------------------------------------------------
void LobbySearchState::Update(float dt) {
    if (game->IsLobbyListUpdated()) { 
        UpdateLobbyListDisplay();
        // Reset the update flag (ensure lobbyListUpdated is accessible)
        game->lobbyListUpdated = false;
    }
}

//---------------------------------------------------------
// Render: Draw the lobby search screen.
//---------------------------------------------------------
void LobbySearchState::Render() {
    game->GetWindow().clear(sf::Color::White);

    // Draw a header bar for visual separation.
    sf::RectangleShape headerBar;
    headerBar.setSize(sf::Vector2f(SCREEN_WIDTH, 60.f));
    headerBar.setFillColor(sf::Color(70, 130, 180)); // Steel blue
    headerBar.setPosition(0.f, 0.f);
    game->GetWindow().draw(headerBar);

    // Render HUD elements (search status and lobby list).
    game->GetHUD().render(game->GetWindow(), game->GetWindow().getDefaultView(), game->GetCurrentState());

    game->GetWindow().display();
}

//---------------------------------------------------------
// ProcessEvent: Delegate event handling.
//---------------------------------------------------------
void LobbySearchState::ProcessEvent(const sf::Event& event) {
    ProcessEvents(event);
}

//---------------------------------------------------------
// ProcessEvents: Handle key presses for lobby selection.
//---------------------------------------------------------
void LobbySearchState::ProcessEvents(const sf::Event& event) {
    if (event.type == sf::Event::KeyPressed) {
        // If number keys (0-9) are pressed, attempt to join the corresponding lobby.
        if (event.key.code >= sf::Keyboard::Num0 && event.key.code <= sf::Keyboard::Num9) {
            int index = event.key.code - sf::Keyboard::Num0;
            JoinLobbyByIndex(index);
        } 
        // Escape key returns to the main menu.
        else if (event.key.code == sf::Keyboard::Escape) {
            game->ReturnToMainMenu();
        }
    }
}

//---------------------------------------------------------
// UpdateLobbyListDisplay: Refresh the HUD with lobby list info.
//---------------------------------------------------------
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

//---------------------------------------------------------
// JoinLobby: Attempt to join a specific lobby.
//---------------------------------------------------------
void LobbySearchState::JoinLobby(CSteamID lobby) {
    if (game->IsInLobby()) return;
    // Set the game as a client.
    game->SetIsHost(false);
    SteamAPICall_t call = SteamMatchmaking()->JoinLobby(lobby);
    if (call == k_uAPICallInvalid) {
        std::cerr << "[LOBBY] JoinLobby call failed immediately for " 
                  << lobby.ConvertToUint64() << "\n";
        game->SetCurrentState(GameState::MainMenu);
    }
}

//---------------------------------------------------------
// JoinLobbyByIndex: Join a lobby by its index in the list.
//---------------------------------------------------------
void LobbySearchState::JoinLobbyByIndex(int index) {
    const auto& lobbies = game->GetLobbyList();
    if (index >= 0 && index < static_cast<int>(lobbies.size())) {
        JoinLobby(lobbies[index].first);
    } else {
        game->GetHUD().updateText("searchStatus", "Invalid lobby selection");
    }
}
