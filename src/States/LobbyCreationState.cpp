#include "LobbyCreationState.h"
#include <iostream>

//---------------------------------------------------------
// Constructor: Initialize lobby creation UI and HUD elements
//---------------------------------------------------------
LobbyCreationState::LobbyCreationState(CubeGame* game) : State(game) {
    // Clear any existing lobby name input.
    game->GetLobbyNameInput().clear();

    // Update HUD prompt for lobby name entry.
    game->GetHUD().updateText("lobbyPrompt", "Enter Lobby Name (Press Enter to Create, ESC to Cancel): ");
    std::cout << "[DEBUG] LobbyCreationState constructed\n";

    // Add HUD element for lobby name prompt.
    game->GetHUD().addElement("lobbyPrompt", 
        "Enter Lobby Name (Press Enter to Create, ESC to Cancel):", 
        18, sf::Vector2f(50.f, 200.f), 
        GameState::LobbyCreation, 
        HUD::RenderMode::ScreenSpace, false);

    // Add a "status" HUD element for lobby creation.
    game->GetHUD().addElement("status", 
        "Lobby Creation", 
        32, sf::Vector2f(SCREEN_WIDTH * 0.4f, 20.f),
        GameState::LobbyCreation, 
        HUD::RenderMode::ScreenSpace, true);
    game->GetHUD().updateBaseColor("status", sf::Color::Black);
}

//---------------------------------------------------------
// Update: No dynamic updates needed for now.
//---------------------------------------------------------
void LobbyCreationState::Update(float dt) {
    // (Placeholder) Additional dynamic updates for lobby creation can be added here.
}

//---------------------------------------------------------
// Render: Clear window, draw header bar and HUD elements.
//---------------------------------------------------------
void LobbyCreationState::Render() {
    if (game->GetCurrentState() != GameState::LobbyCreation) {
        std::cout << "[DEBUG] LobbyCreationState::Render skipped, currentState != LobbyCreation\n";
        return;
    }

    // Clear window with white background.
    game->GetWindow().clear(sf::Color::White);
    game->GetWindow().setView(game->GetWindow().getDefaultView());

    // Draw a header bar.
    sf::RectangleShape headerBar;
    headerBar.setSize(sf::Vector2f(SCREEN_WIDTH, 60.f));
    headerBar.setFillColor(sf::Color(70, 130, 180)); // Steel blue color.
    headerBar.setPosition(0.f, 0.f);
    game->GetWindow().draw(headerBar);

    // Render HUD elements (e.g., "lobbyPrompt" and "status").
    game->GetHUD().render(game->GetWindow(), game->GetWindow().getDefaultView(), game->GetCurrentState());
    game->GetWindow().display();
}

//---------------------------------------------------------
// ProcessEvent: Delegate to internal event processor.
//---------------------------------------------------------
void LobbyCreationState::ProcessEvent(const sf::Event& event) {
    ProcessEvents(event);
}

//---------------------------------------------------------
// ProcessEvents: Handle text input and key presses.
//---------------------------------------------------------
void LobbyCreationState::ProcessEvents(const sf::Event& event) {
    if (event.type == sf::Event::TextEntered) {
        // Handle Enter key: attempt to create lobby.
        if (event.text.unicode == '\r' || event.text.unicode == '\n') {
            if (!game->GetLobbyNameInput().empty()) {
                CreateLobby(game->GetLobbyNameInput());
                std::cout << "[DEBUG] Creating lobby: " << game->GetLobbyNameInput() << "\n";
            } else {
                game->GetHUD().updateText("lobbyPrompt", 
                    "Lobby name cannot be empty! (Enter to Create, ESC to Cancel): ");
            }
        }
        // Handle Backspace: remove last character.
        else if (event.text.unicode == '\b' && !game->GetLobbyNameInput().empty()) {
            game->GetLobbyNameInput().pop_back();
            game->GetHUD().updateText("lobbyPrompt", 
                "Enter Lobby Name (Press Enter to Create, ESC to Cancel): " + game->GetLobbyNameInput());
        }
        // Handle printable ASCII characters.
        else if (event.text.unicode >= 32 && event.text.unicode < 128) {
            game->GetLobbyNameInput() += static_cast<char>(event.text.unicode);
            game->GetHUD().updateText("lobbyPrompt", 
                "Enter Lobby Name (Press Enter to Create, ESC to Cancel): " + game->GetLobbyNameInput());
        }
    }
    // Handle Escape key: cancel and return to main menu.
    else if (event.type == sf::Event::KeyPressed && event.key.code == sf::Keyboard::Escape) {
        game->ReturnToMainMenu();
        std::cout << "[DEBUG] Returning to MainMenu from LobbyCreation\n";
    }
}

//---------------------------------------------------------
// CreateLobby: Initiate lobby creation via Steam API.
//---------------------------------------------------------
void LobbyCreationState::CreateLobby(const std::string& lobbyName) {
    if (game->IsInLobby()) return;

    // Set the game as host.
    game->SetIsHost(true);

    // Update the lobby name input with the provided lobby name.
    game->GetLobbyNameInput() = lobbyName;

    // Call Steam API to create a public lobby with up to 10 members.
    SteamAPICall_t call = SteamMatchmaking()->CreateLobby(k_ELobbyTypePublic, 10);
    if (call == k_uAPICallInvalid) {
        std::cerr << "[LOBBY] CreateLobby call failed immediately.\n";
        game->SetCurrentState(GameState::MainMenu);
    }
}
