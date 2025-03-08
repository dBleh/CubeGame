#include "MainMenuState.h"
#include "../Core/CubeGame.h"
#include <iostream>

//---------------------------------------------------------
// Constructor: Set up HUD elements and invisible button areas.
//---------------------------------------------------------
MainMenuState::MainMenuState(CubeGame* game) : State(game) {
    std::cout << "[DEBUG] MainMenuState constructor called\n";

    // Clear any existing text for title, create, and search options.
    game->GetHUD().updateText("title", "");
    game->GetHUD().updateText("createLobby", "");
    game->GetHUD().updateText("searchLobby", "");

    // Add header text ("status") with black text on a steel blue header.
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

    // Add "Leave Lobby" prompt.
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

    // Add "Create Lobby" option.
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

    // Add "Search Lobbies" option.
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

    // Add "Exit Game" option.
    game->GetHUD().addElement(
        "exitGameText", 
        "Exit Game (Press E)", 
        24, 
        sf::Vector2f(SCREEN_WIDTH * 0.4f, 340.f), 
        GameState::MainMenu, 
        HUD::RenderMode::ScreenSpace, 
        true
    );
    game->GetHUD().updateBaseColor("exitGameText", sf::Color::Black);

    // Set up invisible clickable areas.
    createLobbyButton.setSize(sf::Vector2f(300.f, 40.f));
    createLobbyButton.setPosition(SCREEN_WIDTH * 0.5f - 150.f, 200.f);
    
    searchLobbiesButton.setSize(sf::Vector2f(300.f, 40.f));
    searchLobbiesButton.setPosition(SCREEN_WIDTH * 0.5f - 150.f, 260.f);
    searchLobbiesButton.setFillColor(sf::Color::Transparent);
    
    exitGameButton.setSize(sf::Vector2f(300.f, 40.f));
    exitGameButton.setPosition(SCREEN_WIDTH * 0.5f - 150.f, 320.f);
    exitGameButton.setFillColor(sf::Color::Transparent);
}

//---------------------------------------------------------
// Update: Update header and lobby status based on Steam status.
//---------------------------------------------------------
void MainMenuState::Update(float dt) {
    // Update header status.
    if (!game->IsSteamInitialized()) {
        game->GetHUD().updateText("status", "Loading Steam... please wait");
    } else {
        std::string status = "Main Menu";
        if (game->IsInLobby()) {
            status += " (In Lobby)";
        }
        game->GetHUD().updateText("status", status);
    }

    // Show "Leave Lobby" prompt if currently in a lobby.
    if (game->IsInLobby()) {
        game->GetHUD().updateText("leaveLobby", "Press ESC to Leave Lobby");
    } else {
        game->GetHUD().updateText("leaveLobby", "");
    }
}

//---------------------------------------------------------
// Render: Draw header, HUD elements, and display the window.
//---------------------------------------------------------
void MainMenuState::Render() {
    if (game->GetCurrentState() != GameState::MainMenu) return;

    game->GetWindow().clear(sf::Color::White);
    game->GetWindow().setView(game->GetWindow().getDefaultView());

    // Draw header bar.
    sf::RectangleShape headerBar;
    headerBar.setSize(sf::Vector2f(SCREEN_WIDTH, 60.f));
    headerBar.setFillColor(sf::Color(70, 130, 180)); // Steel blue.
    headerBar.setPosition(0.f, 0.f);
    game->GetWindow().draw(headerBar);

    // Render HUD.
    game->GetHUD().render(game->GetWindow(), game->GetWindow().getDefaultView(), game->GetCurrentState());
    game->GetWindow().display();
}

//---------------------------------------------------------
// ProcessEvent: Delegate event processing.
//---------------------------------------------------------
void MainMenuState::ProcessEvent(const sf::Event& event) {
    ProcessEvents(event);
}

//---------------------------------------------------------
// ProcessEvents: Handle keyboard and mouse input for menu actions.
//---------------------------------------------------------
void MainMenuState::ProcessEvents(const sf::Event& event) {
    if (event.type == sf::Event::KeyPressed) {
        if (event.key.code == sf::Keyboard::Num1 && !game->IsInLobby()) {
            if (!game->IsSteamInitialized()) {
                game->GetHUD().updateText("status", "Waiting for Steam handshake. Please try again soon");
                return;
            }
            // Enter Lobby Creation state.
            game->SetCurrentState(GameState::LobbyCreation);
            game->GetLobbyNameInput().clear();
            game->GetHUD().updateText("lobbyPrompt", "Enter Lobby Name (Press Enter to Create, ESC to Cancel): ");
            std::cout << "[DEBUG] Entering Lobby Creation state\n";
        }
        else if (event.key.code == sf::Keyboard::Num2 && !game->IsInLobby()) {
            // Enter Lobby Search state.
            game->SetCurrentState(GameState::LobbySearch);
            std::cout << "[DEBUG] Entering Lobby Search\n";
        } 
        else if (event.key.code == sf::Keyboard::Escape && game->IsInLobby()) {
            // Leave lobby.
            SteamMatchmaking()->LeaveLobby(game->GetLobbyID());
            game->SetCurrentState(GameState::MainMenu);
            game->GetPlayers().clear();
            std::cout << "[DEBUG] Left lobby from main menu.\n";
        } 
        else if (event.key.code == sf::Keyboard::E) { // Exit game.
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
                // Enter Lobby Creation state via mouse click.
                game->SetCurrentState(GameState::LobbyCreation);
                game->GetLobbyNameInput().clear();
                game->GetHUD().updateText("lobbyPrompt", "Enter Lobby Name (Press Enter to Create, ESC to Cancel): ");
                std::cout << "[DEBUG] Clicked Create Lobby: Entering Lobby Creation state\n";
            } 
            else if (searchLobbiesButton.getGlobalBounds().contains(worldPos)) {
                // Enter Lobby Search state.
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
void MainMenuState::Interpolate(float alpha) {
    // No interpolation needed for a static lobby search screen
}