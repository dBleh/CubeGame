#include "LobbyCreationState.h"
#include <steam/steam_api.h>
#include <iostream>

LobbyCreationState::LobbyCreationState(Game* game) : State(game) {
    game->GetHUD().addElement("prompt", "Enter Lobby Name (Enter to Create, ESC to Cancel): ", 18, sf::Vector2f(50.f, 200.f), GameState::LobbyCreation, HUD::RenderMode::ScreenSpace, false);
}

void LobbyCreationState::Update(float dt) {}

void LobbyCreationState::Render() {
    game->GetWindow().clear(sf::Color::White);
    game->GetHUD().render(game->GetWindow(), game->GetWindow().getDefaultView(), GameState::LobbyCreation);
    game->GetWindow().display();
}

void LobbyCreationState::ProcessEvent(const sf::Event& event) {
    ProcessEvents(event);
}

void LobbyCreationState::ProcessEvents(const sf::Event& event) {
    if (event.type == sf::Event::TextEntered) {
        if (event.text.unicode == '\r' || event.text.unicode == '\n') {
            if (!game->GetLobbyNameInput().empty()) {
                CreateLobby(game->GetLobbyNameInput());
            }
        } else if (event.text.unicode == '\b' && !game->GetLobbyNameInput().empty()) {
            game->GetLobbyNameInput().pop_back();
            game->GetHUD().updateText("prompt", "Enter Lobby Name (Enter to Create, ESC to Cancel): " + game->GetLobbyNameInput());
        } else if (event.text.unicode >= 32 && event.text.unicode < 128) {
            game->GetLobbyNameInput() += static_cast<char>(event.text.unicode);
            game->GetHUD().updateText("prompt", "Enter Lobby Name (Enter to Create, ESC to Cancel): " + game->GetLobbyNameInput());
        }
    } else if (event.type == sf::Event::KeyPressed && event.key.code == sf::Keyboard::Escape) {
        game->SetCurrentState(GameState::MainMenu);
    }
}

void LobbyCreationState::CreateLobby(const std::string& lobbyName) {
    std::cout << "[LOBBY] Lobby creation requested.\n";


    // Update the lobby name input with the provided lobby name
    game->GetLobbyNameInput() = lobbyName;

    // Call Steam API to create a public lobby with up to 10 members
    SteamAPICall_t call = SteamMatchmaking()->CreateLobby(k_ELobbyTypePublic, 10);
    if (call == k_uAPICallInvalid) {
        std::cout << "[LOBBY] CreateLobby call failed immediately.\n";
        game->SetCurrentState(GameState::MainMenu);
    } else {
        std::cout << "[LOBBY] Lobby creation requested.\n";
        // Note: onLobbyEnter will be called from the OnLobbyCreated callback
    }
}

//---------------------------------------------------------
// onLobbyEnter: Handle actions when entering a newly created lobby
//---------------------------------------------------------
void LobbyCreationState::onLobbyEnter(CSteamID lobbyID) {
    if (!SteamMatchmaking()) {
        std::cerr << "[ERROR] SteamMatchmaking interface not available!" << std::endl;
        game->SetCurrentState(GameState::MainMenu);
        return;
    }

    // Transition to the Lobby state
    game->SetCurrentState(GameState::Lobby);
    std::cout << "on lobby enter" << std::endl;



    // Set lobby data
    SteamMatchmaking()->SetLobbyData(lobbyID, "name", game->GetLobbyNameInput().c_str());
    SteamMatchmaking()->SetLobbyData(lobbyID, "game_id", GAME_ID);
    CSteamID myID = SteamUser()->GetSteamID();
    std::string hostStr = std::to_string(myID.ConvertToUint64());
    SteamMatchmaking()->SetLobbyData(lobbyID, "host_steam_id", hostStr.c_str());


    // Add the local player to the NetworkManager's connected clients
    game->GetNetworkManager().AcceptSession(myID);

    // Log entry for debugging
    std::cout << "[LOBBY] Created and entered lobby " << lobbyID.ConvertToUint64() 
              << " as host" << std::endl;
}