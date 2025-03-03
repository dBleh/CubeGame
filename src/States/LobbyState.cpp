#include "LobbyState.h"


LobbyState::LobbyState(CubeGame* game)
    : State(game)
{
    // Create the HUD elements for the Lobby
    // (Using white text so they show against the black background in Render().)
    game->GetHUD().addElement(
        "lobbyStatus",
        "",                           // Content will be set dynamically
        24,                           // Character size
        sf::Vector2f(10.f, 10.f),     // Position (ScreenSpace)
        GameState::Lobby,            // Visible in the Lobby state
        HUD::RenderMode::ScreenSpace, // prefix with HUD::
        false                         // Not hoverable
    );
    game->GetHUD().updateBaseColor("lobbyStatus", sf::Color::White);

    game->GetHUD().addElement(
        "startGame",
        "",                           // Content will be set dynamically
        24,
        sf::Vector2f(10.f, 40.f),
        GameState::Lobby,
        HUD::RenderMode::ScreenSpace, // prefix with HUD::
        false
    );
    game->GetHUD().updateBaseColor("startGame", sf::Color::White);

    game->GetHUD().addElement(
        "returnMain",
        "",                           // Content will be set dynamically
        24,
        sf::Vector2f(10.f, 70.f),
        GameState::Lobby,
        HUD::RenderMode::ScreenSpace, // prefix with HUD::
        false
    );
    game->GetHUD().updateBaseColor("returnMain", sf::Color::White);
}

void LobbyState::Update(float dt)
{
    UpdateLobbyMembers();

    size_t readyCount = std::count_if(
        game->GetPlayers().begin(),
        game->GetPlayers().end(),
        [](const auto& p) {
            return p.second.ready;
        }
    );

    // Update the lobby status text
    std::string lobbyInfo = "Lobby - Players: " + std::to_string(game->GetPlayers().size()) + 
                            "\nReady: " + std::to_string(readyCount);
    game->GetHUD().updateText("lobbyStatus", lobbyInfo);

    // Explicitly control "startGame" and "returnMain" visibility via text
    if (game->GetLocalPlayer().steamID == SteamMatchmaking()->GetLobbyOwner(game->GetLobbyID()))
    {
        // Host gets the "Start Game" prompt
        game->GetHUD().updateText("startGame", "Press S to Start Game (Host Only)");
        // You could also choose to always show "returnMain" if you wish
        game->GetHUD().updateText("returnMain", "");
    }
    else
    {
        // Non-host sees a blank for "startGame" and a prompt to go back
        game->GetHUD().updateText("startGame", "");
        game->GetHUD().updateText("returnMain", "Press M to Return to Main Menu");
    }
}

void LobbyState::Render()
{
    // Clear the screen with black so white text is visible
    game->GetWindow().clear(sf::Color::Black);

    // Draw each player's shape (as before)
    for (const auto& player : game->GetPlayers()) {
        game->GetWindow().draw(player.second.shape);
    }

    // Example: draw a simple rectangle in the lobby
    sf::RectangleShape lobbyIndicator(sf::Vector2f(200.f, 50.f));
    lobbyIndicator.setFillColor(sf::Color::Yellow);
    lobbyIndicator.setPosition(SCREEN_WIDTH / 2.f - 100.f, 100.f);
    game->GetWindow().draw(lobbyIndicator);

    // Render the HUD on top
    game->RenderHUDLayer();

    game->GetWindow().display();
}

void LobbyState::ProcessEvent(const sf::Event& event)
{
    ProcessEvents(event);
}

void LobbyState::ProcessEvents(const sf::Event& event)
{
    if (event.type == sf::Event::KeyPressed)
    {
        if (event.key.code == sf::Keyboard::R)
        {
            game->ToggleReady();
        }
        else if (event.key.code == sf::Keyboard::S &&
                 game->GetLocalPlayer().steamID == SteamMatchmaking()->GetLobbyOwner(game->GetLobbyID()))
        {
            std::cout << "[DEBUG] Host pressed S. Players: " << game->GetPlayers().size() 
                      << ", All ready: " << (AllPlayersReady() ? "Yes" : "No") << std::endl;

            // Allow start if single-player OR (multiplayer and everyone is ready OR game was already played)
            if (game->GetPlayers().size() <= 1 ||
               (game->GetPlayers().size() > 1 && (AllPlayersReady() || game->HasGameBeenPlayed())))
            {
                game->StartGame();
            }
            else
            {
                std::cout << "[DEBUG] Cannot start: Multiple players not all ready" << std::endl;
            }
        }
        else if (event.key.code == sf::Keyboard::M)
        {
            game->ReturnToMainMenu();
            std::cout << "[DEBUG] Returning to main menu from lobby" << std::endl;
        }
    }
}

void LobbyState::UpdateLobbyMembers()
{
    if (!game->IsInLobby()) return;

    int memberCount = SteamMatchmaking()->GetNumLobbyMembers(game->GetLobbyID());

    // Add any new players
    for (int i = 0; i < memberCount; ++i)
    {
        CSteamID member = SteamMatchmaking()->GetLobbyMemberByIndex(game->GetLobbyID(), i);
        if (!game->GetPlayers().count(member))
        {
            Player p;
            p.initialize();
            p.steamID = member;
            game->GetPlayers()[member] = p;
            std::cout << "[DEBUG] Added player: " << member.ConvertToUint64() << std::endl;
        }

        // Update ready state from lobby data
        const char* readyState = SteamMatchmaking()->GetLobbyMemberData(
                                     game->GetLobbyID(), member, "ready");
        game->GetPlayers()[member].ready = (readyState && std::string(readyState) == "1");
    }

    // Remove players who left
    for (auto it = game->GetPlayers().begin(); it != game->GetPlayers().end();)
    {
        bool found = false;
        for (int i = 0; i < memberCount; ++i)
        {
            if (SteamMatchmaking()->GetLobbyMemberByIndex(game->GetLobbyID(), i) == it->first)
            {
                found = true;
                break;
            }
        }
        if (!found)
        {
            std::cout << "[DEBUG] Removed stale player: " 
                      << it->first.ConvertToUint64() << std::endl;
            it = game->GetPlayers().erase(it);
        }
        else
        {
            ++it;
        }
    }
}

bool LobbyState::AllPlayersReady()
{
    return std::all_of(
        game->GetPlayers().begin(),
        game->GetPlayers().end(),
        [](const auto& p) {
            return p.second.ready;
        }
    );
}
