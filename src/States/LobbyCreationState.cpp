#include "LobbyCreationState.h"

LobbyCreationState::LobbyCreationState(CubeGame* game) : State(game) {
    game->GetLobbyNameInput().clear();
    game->GetHUD().updateText("lobbyPrompt", "Enter Lobby Name (Press Enter to Create, ESC to Cancel): ");
    std::cout << "[DEBUG] LobbyCreationState constructed\n";
    game->GetHUD().addElement("lobbyPrompt", 
        "Enter Lobby Name (Press Enter to Create, ESC to Cancel):", 
        18, sf::Vector2f(50.f, 200.f), 
        GameState::LobbyCreation, HUD::RenderMode::ScreenSpace, false);
    // Add "status" HUD element for LobbyCreation state
    game->GetHUD().addElement(
        "status", 
        "Lobby Creation", // Initial text
        32, 
        sf::Vector2f(SCREEN_WIDTH * 0.4f, 20.f), // Centered in header
        GameState::LobbyCreation, // Visible only in LobbyCreation state
        HUD::RenderMode::ScreenSpace, 
        true // Center horizontally
    );
    game->GetHUD().updateBaseColor("status", sf::Color::Black);
}

void LobbyCreationState::Update(float dt) {
    // No need to update "status" here since it's set in constructor
    // If dynamic updates are needed later, add them here
}

void LobbyCreationState::Render() {
    if (game->GetCurrentState() != GameState::LobbyCreation) {
        std::cout << "[DEBUG] LobbyCreationState::Render skipped, currentState != LobbyCreation\n";
        return;
    }

    game->GetWindow().clear(sf::Color::White); // Clear to white first
    game->GetWindow().setView(game->GetWindow().getDefaultView()); // Set view immediately after clear

    // Draw header bar
    sf::RectangleShape headerBar;
    headerBar.setSize(sf::Vector2f(SCREEN_WIDTH, 60.f));
    headerBar.setFillColor(sf::Color(70, 130, 180)); // Steel blue
    headerBar.setPosition(0.f, 0.f);
    game->GetWindow().draw(headerBar);

    // Render HUD layer (includes "status" and "lobbyPrompt")
    game->GetHUD().render(game->GetWindow(), game->GetWindow().getDefaultView(), game->GetCurrentState());


    game->GetWindow().display();
}

void LobbyCreationState::ProcessEvent(const sf::Event& event) {
    ProcessEvents(event);
}

void LobbyCreationState::ProcessEvents(const sf::Event& event) {
    if (event.type == sf::Event::TextEntered) {
        if (event.text.unicode == '\r' || event.text.unicode == '\n') { // Enter key
            if (!game->GetLobbyNameInput().empty()) {
                // Instead of calling CubeGame::CreateLobby, call our new function.
                CreateLobby(game->GetLobbyNameInput());
                std::cout << "[DEBUG] Creating lobby: " << game->GetLobbyNameInput() << "\n";
            } else {
                game->GetHUD().updateText("lobbyPrompt", "Lobby name cannot be empty! (Enter to Create, ESC to Cancel): ");
            }
        } else if (event.text.unicode == '\b' && !game->GetLobbyNameInput().empty()) { // Backspace
            game->GetLobbyNameInput().pop_back();
            game->GetHUD().updateText("lobbyPrompt", "Enter Lobby Name (Press Enter to Create, ESC to Cancel): " + game->GetLobbyNameInput());
        } else if (event.text.unicode >= 32 && event.text.unicode < 128) { // Printable ASCII
            game->GetLobbyNameInput() += static_cast<char>(event.text.unicode);
            game->GetHUD().updateText("lobbyPrompt", "Enter Lobby Name (Press Enter to Create, ESC to Cancel): " + game->GetLobbyNameInput());
        }
    } else if (event.type == sf::Event::KeyPressed && event.key.code == sf::Keyboard::Escape) {
        game->ReturnToMainMenu();
        std::cout << "[DEBUG] Returning to MainMenu from LobbyCreation\n";
    }
}

void LobbyCreationState::CreateLobby(const std::string& lobbyName) {
    if (game->IsInLobby()) return;
    // Set host flag – you can either add a setter in CubeGame or use friend access.
    game->SetIsHost(true);
    // Update the lobby name in CubeGame.
    game->GetLobbyNameInput() = lobbyName;
    // Attempt to create the lobby via Steam API.
    SteamAPICall_t call = SteamMatchmaking()->CreateLobby(k_ELobbyTypePublic, 10);
    if (call == k_uAPICallInvalid) {
        std::cerr << "[LOBBY] CreateLobby call failed immediately.\n";
        game->SetCurrentState(GameState::MainMenu);
    }
}