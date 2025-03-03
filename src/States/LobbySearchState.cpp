#include "LobbySearchState.h"

LobbySearchState::LobbySearchState(CubeGame* game) : State(game) {
    game->SearchLobbies();
}

void LobbySearchState::Update(float dt) {
    // No continuous updates needed
}

void LobbySearchState::Render() {
    game->GetWindow().clear(sf::Color::White);
    game->RenderHUDLayer();
    game->GetWindow().display();
}

void LobbySearchState::ProcessEvent(const sf::Event& event) {
    ProcessEvents(event);
}

void LobbySearchState::ProcessEvents(const sf::Event& event) {
    if (event.type == sf::Event::KeyPressed) {
        if (event.key.code >= sf::Keyboard::Num0 && event.key.code <= sf::Keyboard::Num9) {
            int index = event.key.code - sf::Keyboard::Num0;
            game->JoinLobbyByIndex(index);
        } else if (event.key.code == sf::Keyboard::Escape) {
            game->ReturnToMainMenu();
        }
    }
}