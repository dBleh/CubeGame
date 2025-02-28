#include "LobbyCreationState.h"

LobbyCreationState::LobbyCreationState(CubeGame* game) : State(game) {
    game->GetLobbyNameInput().clear();
    game->GetHUD().updateText("lobbyPrompt", "Enter Lobby Name (Press Enter to Create, ESC to Cancel): ");
}

void LobbyCreationState::Update(float dt) {
    // No continuous updates needed
}

void LobbyCreationState::Render() {
    game->GetWindow().clear(sf::Color::Black);
    game->RenderHUDLayer();
    game->GetWindow().display();
}

void LobbyCreationState::ProcessEvent(const sf::Event& event) {
    ProcessEvents(event);
}

void LobbyCreationState::ProcessEvents(const sf::Event& event) {
    if (event.type == sf::Event::TextEntered) {
        if (event.text.unicode == '\r' || event.text.unicode == '\n') { // Enter key
            if (!game->GetLobbyNameInput().empty()) {
                game->CreateLobby(game->GetLobbyNameInput());
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
    }
}