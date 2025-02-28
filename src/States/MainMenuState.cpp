#include "MainMenuState.h"

MainMenuState::MainMenuState(CubeGame* game) : State(game) {
    // Set up clickable button areas (match HUD positions)
    createLobbyButton.setSize(sf::Vector2f(200, 30));
    createLobbyButton.setPosition(SCREEN_WIDTH/2 - 100, 200);
    createLobbyButton.setFillColor(sf::Color::Transparent); // Invisible, just for clicking

    searchLobbiesButton.setSize(sf::Vector2f(200, 30));
    searchLobbiesButton.setPosition(SCREEN_WIDTH/2 - 100, 250);
    searchLobbiesButton.setFillColor(sf::Color::Transparent);
}

void MainMenuState::Update(float dt) {
    std::string status = "Main Menu";
    if (game->IsInLobby()) {
        status += " (In Lobby)";
    }
    game->GetHUD().updateText("status", status);
    if (game->IsInLobby()) {
        game->GetHUD().addElement("leaveLobby", "Press ESC to Leave Lobby", 20, 
                                sf::Vector2f(SCREEN_WIDTH/2 - 150, 300), GameState::MainMenu, HUD::RenderMode::ScreenSpace);
    } else {
        game->GetHUD().updateText("leaveLobby", "");
    }
}

void MainMenuState::Render() {
    if (game->GetCurrentState() != GameState::MainMenu) return;
    game->GetWindow().clear(sf::Color::Black);
    
    // Draw clickable areas (optional: make visible for debugging)
    // game->GetWindow().draw(createLobbyButton);
    // game->GetWindow().draw(searchLobbiesButton);
    
    game->RenderHUDLayer();
    game->GetWindow().display();
}

void MainMenuState::ProcessEvent(const sf::Event& event) {
    ProcessEvents(event);
}

void MainMenuState::ProcessEvents(const sf::Event& event) {
    if (event.type == sf::Event::KeyPressed) {
        if (event.key.code == sf::Keyboard::Num1 && !game->IsInLobby()) {
            game->EnterLobbyCreation();
        } else if (event.key.code == sf::Keyboard::Num2 && !game->IsInLobby()) {
            game->EnterLobbySearch();
        } else if (event.key.code == sf::Keyboard::Escape && game->IsInLobby()) {
            SteamMatchmaking()->LeaveLobby(game->GetLobbyID());
            game->GetCurrentState() = GameState::MainMenu;
            game->GetPlayers().clear();
            std::cout << "[DEBUG] Left lobby from main menu." << std::endl;
        }
    }
    else if (event.type == sf::Event::MouseButtonPressed && event.mouseButton.button == sf::Mouse::Left) {
        sf::Vector2i mousePos = sf::Mouse::getPosition(game->GetWindow());
        sf::Vector2f worldPos = game->GetWindow().mapPixelToCoords(mousePos, game->GetView());
        
        if (!game->IsInLobby()) {
            if (createLobbyButton.getGlobalBounds().contains(worldPos)) {
                game->EnterLobbyCreation();
                std::cout << "[DEBUG] Clicked Create Lobby" << std::endl;
            } else if (searchLobbiesButton.getGlobalBounds().contains(worldPos)) {
                game->EnterLobbySearch();
                std::cout << "[DEBUG] Clicked Search Lobbies" << std::endl;
            }
        }
    }
}