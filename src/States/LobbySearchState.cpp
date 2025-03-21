#include "LobbySearchState.h"
#include <steam/steam_api.h>
#include <iostream>

LobbySearchState::LobbySearchState(Game* game) : State(game) {
    game->GetHUD().addElement("searchStatus", "Searching...", 18, sf::Vector2f(20.f, 20.f), GameState::LobbySearch, HUD::RenderMode::ScreenSpace, false);
    game->GetHUD().addElement("lobbyList", "Available Lobbies:\n", 20, sf::Vector2f(50.f, 100.f), GameState::LobbySearch, HUD::RenderMode::ScreenSpace, false);
    SearchLobbies();
}

void LobbySearchState::Update(float dt) {
    if (lobbyListUpdated) {
        UpdateLobbyListDisplay();
        lobbyListUpdated = false;
    }
}

void LobbySearchState::Render() {
    game->GetWindow().clear(sf::Color::White);
    game->GetHUD().render(game->GetWindow(), game->GetWindow().getDefaultView(), GameState::LobbySearch);
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
            game->SetCurrentState(GameState::MainMenu);
        }
    }
}

void LobbySearchState::SearchLobbies() {
    if (game->IsInLobby()) return;
    lobbyList.clear();
    SteamMatchmaking()->AddRequestLobbyListStringFilter("game_id", "SteamGame_v1", k_ELobbyComparisonEqual);
    SteamAPICall_t call = SteamMatchmaking()->RequestLobbyList();
    if (call == k_uAPICallInvalid) {
        std::cerr << "[ERROR] Failed to request lobby list!" << std::endl;
        game->GetHUD().updateText("searchStatus", "Failed to search lobbies");
    }
}

void LobbySearchState::UpdateLobbyListDisplay() {
    std::string lobbyText = "Available Lobbies (Press 0-9 to join, ESC to cancel):\n";
    for (size_t i = 0; i < lobbyList.size() && i < 10; ++i) {
        lobbyText += std::to_string(i) + ": " + lobbyList[i].second + "\n";
    }
    if (lobbyList.empty()) {
        lobbyText += "No lobbies available.";
    }
    game->GetHUD().updateText("lobbyList", lobbyText);
}

void LobbySearchState::JoinLobby(CSteamID lobby) {
    if (game->IsInLobby()) return;
    SteamAPICall_t call = SteamMatchmaking()->JoinLobby(lobby);
    if (call == k_uAPICallInvalid) {
        std::cerr << "[ERROR] Failed to join lobby!" << std::endl;
        game->SetCurrentState(GameState::MainMenu);
    }
}

void LobbySearchState::JoinLobbyByIndex(int index) {
    if (index >= 0 && index < static_cast<int>(lobbyList.size())) {
        JoinLobby(lobbyList[index].first);
    } else {
        game->GetHUD().updateText("searchStatus", "Invalid lobby selection");
    }
}