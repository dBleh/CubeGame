#include "MainMenuState.h"
#include "../Core/CubeGame.h"
#include <iostream>

MainMenuState::MainMenuState(CubeGame* game) : State(game) {
    std::cout << "[DEBUG] MainMenuState constructor called\n";

    // Clear any existing text for these keys
    game->GetHUD().updateText("title", "");
    game->GetHUD().updateText("createLobby", "");
    game->GetHUD().updateText("searchLobby", "");

    // Header text (white on steel blue header)
    game->GetHUD().addElement(
        "status", 
        "", 
        32, 
        sf::Vector2f(SCREEN_WIDTH * 0.4f, 20.f), 
        GameState::MainMenu, 
        HUD::RenderMode::ScreenSpace, 
        true
    );
    game->GetHUD().updateBaseColor("status", sf::Color::Black);

    // Leave lobby prompt (black text on white background)
    game->GetHUD().addElement(
        "leaveLobby", 
        "", 
        24, 
        sf::Vector2f(SCREEN_WIDTH * 0.4f, 20.f), 
        GameState::MainMenu, 
        HUD::RenderMode::ScreenSpace, 
        true
    );
    game->GetHUD().updateBaseColor("leaveLobby", sf::Color::Black);

    // Create/Search lobby text (black text)
    game->GetHUD().addElement(
        "createLobbyText", 
        "Create Lobby (Press 1)", 
        24, 
        sf::Vector2f(SCREEN_WIDTH * 0.4f, 220.f), 
        GameState::MainMenu, 
        HUD::RenderMode::ScreenSpace, 
        true
    );
    game->GetHUD().updateBaseColor("createLobbyText", sf::Color::Black);

    game->GetHUD().addElement(
        "searchLobbiesText", 
        "Search Lobbies (Press 2)", 
        24, 
        sf::Vector2f(SCREEN_WIDTH * 0.4f, 280.f), 
        GameState::MainMenu, 
        HUD::RenderMode::ScreenSpace, 
        true
    );
    game->GetHUD().updateBaseColor("searchLobbiesText", sf::Color::Black);

    // Add Exit Game option (black text)
    game->GetHUD().addElement(
        "exitGameText", 
        "Exit Game (Press E)", 
        24, 
        sf::Vector2f(SCREEN_WIDTH * 0.4f, 340.f), // Positioned below Search Lobbies
        GameState::MainMenu, 
        HUD::RenderMode::ScreenSpace, 
        true
    );
    game->GetHUD().updateBaseColor("exitGameText", sf::Color::Black);

    // Invisible click areas (matching size/positions)
    createLobbyButton.setSize(sf::Vector2f(300.f, 40.f));
    createLobbyButton.setPosition(SCREEN_WIDTH * 0.5f - 150.f, 200.f);
    
    searchLobbiesButton.setFillColor(sf::Color::Transparent);
    searchLobbiesButton.setSize(sf::Vector2f(300.f, 40.f));
    searchLobbiesButton.setPosition(SCREEN_WIDTH * 0.5f - 150.f, 260.f);
    searchLobbiesButton.setFillColor(sf::Color::Transparent);
    
    exitGameButton.setSize(sf::Vector2f(300.f, 40.f));
    exitGameButton.setPosition(SCREEN_WIDTH * 0.5f - 150.f, 320.f);
    exitGameButton.setFillColor(sf::Color::Transparent);
}

void MainMenuState::Update(float dt) {
    // Update header text based on Steam initialization and lobby status
    if (!game->IsSteamInitialized()) {
        game->GetHUD().updateText("status", "Loading Steam... please wait");
    } else {
        std::string status = "Main Menu";
        if (game->IsInLobby()) {
            status += " (In Lobby)";
        }
        game->GetHUD().updateText("status", status);
    }

    // Show the "Leave Lobby" prompt only when in a lobby
    if (game->IsInLobby()) {
        game->GetHUD().updateText("leaveLobby", "Press ESC to Leave Lobby");
    } else {
        game->GetHUD().updateText("leaveLobby", "");
    }
}

void MainMenuState::Render() {
    if (game->GetCurrentState() != GameState::MainMenu) {
        return;
    }

    game->GetWindow().clear(sf::Color::White);
    game->GetWindow().setView(game->GetWindow().getDefaultView());

    sf::RectangleShape headerBar;
    headerBar.setSize(sf::Vector2f(SCREEN_WIDTH, 60.f));
    headerBar.setFillColor(sf::Color(70, 130, 180)); // Steel blue
    headerBar.setPosition(0.f, 0.f);
    game->GetWindow().draw(headerBar);

    game->GetHUD().render(game->GetWindow(), game->GetWindow().getDefaultView(), game->GetCurrentState());

    game->GetWindow().display();
}

void MainMenuState::ProcessEvent(const sf::Event& event) {
    ProcessEvents(event);
}

void MainMenuState::ProcessEvents(const sf::Event& event) {
    if (event.type == sf::Event::KeyPressed) {
        if (event.key.code == sf::Keyboard::Num1 && !game->IsInLobby()) {
            if (!game->IsSteamInitialized()) {
                game->GetHUD().updateText("status", "Waiting for Steam handshake. Please try again soon");
                return;
            }
            // Switch state to LobbyCreation and initialize its HUD
            game->SetCurrentState(GameState::LobbyCreation);
            game->GetLobbyNameInput().clear();
            game->GetHUD().updateText("lobbyPrompt", "Enter Lobby Name (Press Enter to Create, ESC to Cancel): ");
            std::cout << "[DEBUG] Entering Lobby Creation state\n";
        }
        else if (event.key.code == sf::Keyboard::Num2 && !game->IsInLobby()) {
            // Switch state to LobbySearch
            game->SetCurrentState(GameState::LobbySearch);
            std::cout << "[DEBUG] Entering Lobby Search\n";
        } 
        else if (event.key.code == sf::Keyboard::Escape && game->IsInLobby()) {
            SteamMatchmaking()->LeaveLobby(game->GetLobbyID());
            game->SetCurrentState(GameState::MainMenu); // For consistency
            game->GetPlayers().clear();
            std::cout << "[DEBUG] Left lobby from main menu.\n";
        } 
        else if (event.key.code == sf::Keyboard::E) { // Exit option
            game->GetWindow().close();
            std::cout << "[DEBUG] Exiting game from main menu.\n";
        }
    }
    else if (event.type == sf::Event::MouseButtonPressed && event.mouseButton.button == sf::Mouse::Left) {
        sf::Vector2i mousePos = sf::Mouse::getPosition(game->GetWindow());
        sf::Vector2f worldPos = game->GetWindow().mapPixelToCoords(mousePos, game->GetView());
        if (!game->IsInLobby()) {
            if (createLobbyButton.getGlobalBounds().contains(worldPos)) {
                if (!game->IsSteamInitialized()) {
                    game->GetHUD().updateText("status", "Waiting for Steam handshake. Please try again soon");
                    return;
                }
                // Inline logic for entering LobbyCreation
                game->SetCurrentState(GameState::LobbyCreation);
                game->GetLobbyNameInput().clear();
                game->GetHUD().updateText("lobbyPrompt", "Enter Lobby Name (Press Enter to Create, ESC to Cancel): ");
                std::cout << "[DEBUG] Clicked Create Lobby: Entering Lobby Creation state\n";
            } 
            else if (searchLobbiesButton.getGlobalBounds().contains(worldPos)) {
                // Switch state to LobbySearch
                game->SetCurrentState(GameState::LobbySearch);
                std::cout << "[DEBUG] Clicked Search Lobbies\n";
            } 
            else if (exitGameButton.getGlobalBounds().contains(worldPos)) {
                game->GetWindow().close();
                std::cout << "[DEBUG] Clicked Exit Game\n";
            }
        }
    }
}
