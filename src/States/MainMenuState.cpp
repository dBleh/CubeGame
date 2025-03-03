#include "MainMenuState.h"

MainMenuState::MainMenuState(CubeGame* game)
    : State(game)
{
    // --- Create HUD Elements ---
    // 1) "status" text at top-left (e.g., "Main Menu", or "Loading Steam...")
    game->GetHUD().addElement(
        "status",
        "",  // We'll update this in Update()
        30,  // Character size
        sf::Vector2f(10.f, 10.f),
        GameState::MainMenu,
        HUD::RenderMode::ScreenSpace,
        false // not hoverable
    );
    game->GetHUD().updateBaseColor("status", sf::Color::White);

    // 2) "leaveLobby" text (appears only if you're in a lobby)
    game->GetHUD().addElement(
        "leaveLobby",
        "",  // We'll update or clear this in Update()
        20,
        sf::Vector2f(SCREEN_WIDTH / 2.f - 150.f, 300.f),
        GameState::MainMenu,
        HUD::RenderMode::ScreenSpace,
        false
    );
    game->GetHUD().updateBaseColor("leaveLobby", sf::Color::White);

    // (Optional) 3) Add text for "Create Lobby" and "Search Lobbies"
    // if you want them to display in the Main Menu:
    game->GetHUD().addElement(
        "createLobbyText",
        "Create Lobby (Press 1)",
        20,
        sf::Vector2f(SCREEN_WIDTH / 2.f - 100.f, 200.f),
        GameState::MainMenu,
        HUD::RenderMode::ScreenSpace,
        false
    );
    game->GetHUD().updateBaseColor("createLobbyText", sf::Color::White);

    game->GetHUD().addElement(
        "searchLobbiesText",
        "Search Lobbies (Press 2)",
        20,
        sf::Vector2f(SCREEN_WIDTH / 2.f - 100.f, 250.f),
        GameState::MainMenu,
        HUD::RenderMode::ScreenSpace,
        false
    );
    game->GetHUD().updateBaseColor("searchLobbiesText", sf::Color::White);

    // --- Set up the invisible click-areas ---
    createLobbyButton.setSize(sf::Vector2f(200.f, 30.f));
    createLobbyButton.setPosition(SCREEN_WIDTH / 2.f - 100.f, 200.f);
    createLobbyButton.setFillColor(sf::Color::Transparent);

    searchLobbiesButton.setSize(sf::Vector2f(200.f, 30.f));
    searchLobbiesButton.setPosition(SCREEN_WIDTH / 2.f - 100.f, 250.f);
    searchLobbiesButton.setFillColor(sf::Color::Transparent);
}

void MainMenuState::Update(float dt)
{
    // Update "status" text
    if (!game->IsSteamInitialized()) {
        game->GetHUD().updateText("status", "Loading Steam... please wait");
    } else {
        // If Steam is initialized, show "Main Menu" (+ " (In Lobby)" if currently in a lobby)
        std::string status = "Main Menu";
        if (game->IsInLobby()) {
            status += " (In Lobby)";
        }
        game->GetHUD().updateText("status", status);
    }

    // If in a lobby, show the "Press ESC to leave" prompt. Otherwise, clear it.
    if (game->IsInLobby()) {
        game->GetHUD().updateText("leaveLobby", "Press ESC to Leave Lobby");
    } else {
        game->GetHUD().updateText("leaveLobby", "");
    }
}

void MainMenuState::Render()
{
    // Only clear/display if indeed in MainMenu state
    if (game->GetCurrentState() != GameState::MainMenu) {
        return;
    }

    game->GetWindow().clear(sf::Color::Black);

    // (Optional) Draw the transparent rectangles if you want to see them for debugging:
    // game->GetWindow().draw(createLobbyButton);
    // game->GetWindow().draw(searchLobbiesButton);

    // Draw HUD elements
    game->RenderHUDLayer();
    game->GetWindow().display();
}

void MainMenuState::ProcessEvent(const sf::Event& event)
{
    ProcessEvents(event);
}

void MainMenuState::ProcessEvents(const sf::Event& event)
{
    if (event.type == sf::Event::KeyPressed) {
        // Press 1 -> Enter Lobby Creation
        if (event.key.code == sf::Keyboard::Num1 && !game->IsInLobby()) {
            game->EnterLobbyCreation();
        }
        // Press 2 -> Enter Lobby Search
        else if (event.key.code == sf::Keyboard::Num2 && !game->IsInLobby()) {
            game->EnterLobbySearch();
        }
        // Press ESC -> Leave lobby (if in one)
        else if (event.key.code == sf::Keyboard::Escape && game->IsInLobby()) {
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
            // If user clicks the "Create Lobby" rectangle
            if (createLobbyButton.getGlobalBounds().contains(worldPos)) {
                game->EnterLobbyCreation();
                std::cout << "[DEBUG] Clicked Create Lobby" << std::endl;
            }
            // If user clicks the "Search Lobbies" rectangle
            else if (searchLobbiesButton.getGlobalBounds().contains(worldPos)) {
                game->EnterLobbySearch();
                std::cout << "[DEBUG] Clicked Search Lobbies" << std::endl;
            }
        }
    }
}