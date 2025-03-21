#include "MainMenuState.h"
#include <iostream>

MainMenuState::MainMenuState(Game* game) : State(game) {
    game->GetHUD().addElement("title", "SteamGame", 32, sf::Vector2f(580.f, 20.f), GameState::MainMenu, HUD::RenderMode::ScreenSpace, false);
    game->GetHUD().addElement("createLobby", "Create Lobby (Press 1)", 24, sf::Vector2f(540.f, 220.f), GameState::MainMenu, HUD::RenderMode::ScreenSpace, false);
    game->GetHUD().addElement("searchLobby", "Search Lobbies (Press 2)", 24, sf::Vector2f(540.f, 280.f), GameState::MainMenu, HUD::RenderMode::ScreenSpace, false);
}

void MainMenuState::Update(float dt) {
    if (!game->IsSteamInitialized()) {
        game->GetHUD().updateText("title", "Loading Steam...");
    } else {
        game->GetHUD().updateText("title", "SteamGame");
    }
}

void MainMenuState::Render() {
    game->GetWindow().clear(sf::Color::White);
    game->GetHUD().render(game->GetWindow(), game->GetWindow().getDefaultView(), GameState::MainMenu);
    game->GetWindow().display();
}

void MainMenuState::ProcessEvent(const sf::Event& event) {
    ProcessEvents(event);
}

void MainMenuState::ProcessEvents(const sf::Event& event) {
    if (event.type == sf::Event::KeyPressed) {
        if (event.key.code == sf::Keyboard::Num1 && game->IsSteamInitialized()) {
            game->SetCurrentState(GameState::LobbyCreation);
            game->GetLobbyNameInput().clear();
        } else if (event.key.code == sf::Keyboard::Num2 && game->IsSteamInitialized()) {
            game->SetCurrentState(GameState::LobbySearch);
        }
    }
}