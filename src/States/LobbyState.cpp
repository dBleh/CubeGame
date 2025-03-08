#include "LobbyState.h"
#include "../Utils/RoundedRectangleShape.hpp" // Assuming usage for rounded rectangles.
#include <iostream>
#include <algorithm>
#include <Steam/steam_api.h>

//---------------------------------------------------------
// Constructor: Set up lobby HUD elements and player slots.
//---------------------------------------------------------
LobbyState::LobbyState(CubeGame* game)
    : State(game)
{
    // Retrieve lobby name from Steam; default to "Lobby" if empty.
    std::string lobbyName = SteamMatchmaking()->GetLobbyData(game->GetLobbyID(), "name");
    if (lobbyName.empty()) {
        lobbyName = "Lobby";
    }

    // Add header element with lobby name.
    game->GetHUD().addElement(
         "lobbyHeader",
         lobbyName,
         32, 
         sf::Vector2f(SCREEN_WIDTH * 0.5f, 20.f),
         GameState::Lobby,
         HUD::RenderMode::ScreenSpace,
         true
    );
    game->GetHUD().updateBaseColor("lobbyHeader", sf::Color::White);

    // Create 12 player slot placeholders arranged in a 3×4 grid.
    const int rows = 4;
    const int cols = 3;
    const float slotWidth  = 220.f;
    const float slotHeight = 60.f;
    const float spacing    = 40.f;

    float totalWidth  = cols * slotWidth + (cols - 1) * spacing;
    float totalHeight = rows * slotHeight + (rows - 1) * spacing;

    float xStart = (SCREEN_WIDTH  - totalWidth ) * 0.5f;
    float yStart = 120.f; // Gap below header

    int slotIndex = 0;
    for (int r = 0; r < rows; ++r) {
        for (int c = 0; c < cols; ++c) {
            float xPos = xStart + c * (slotWidth + spacing);
            float yPos = yStart + r * (slotHeight + spacing);

            // Configure slot background.
            sf::RectangleShape& rect = m_playerSlotRects[slotIndex];
            rect.setSize(sf::Vector2f(slotWidth, slotHeight));
            rect.setFillColor(sf::Color(100, 149, 237)); // CornflowerBlue.
            rect.setOutlineColor(sf::Color::Black);
            rect.setOutlineThickness(2.f);
            rect.setPosition(xPos, yPos);

            // Add HUD element for player's name in this slot.
            {
                std::string nameId = "playerNameSlot" + std::to_string(slotIndex);
                float nameX = xPos + slotWidth * 0.3f;
                float nameY = yPos + slotHeight * 0.3f;
                game->GetHUD().addElement(
                    nameId,
                    "Empty", 
                    22,
                    sf::Vector2f(nameX, nameY),
                    GameState::Lobby,
                    HUD::RenderMode::ScreenSpace,
                    true
                );
                game->GetHUD().updateBaseColor(nameId, sf::Color::Black);
            }

            // Add HUD element for player's ready status in this slot.
            {
                std::string readyId = "playerReadySlot" + std::to_string(slotIndex);
                float readyX = xPos + slotWidth * 0.3f;
                float readyY = yPos + slotHeight * 0.6f;
                game->GetHUD().addElement(
                    readyId,
                    "",  // Will be set to "Ready"/"Not Ready"
                    20,
                    sf::Vector2f(readyX, readyY),
                    GameState::Lobby,
                    HUD::RenderMode::ScreenSpace,
                    true
                );
                game->GetHUD().updateBaseColor(readyId, sf::Color::Black);
            }

            ++slotIndex;
        }
    }

    // Add HUD prompts near the bottom.
    game->GetHUD().addElement(
        "startGame",
        "",
        24,
        sf::Vector2f(SCREEN_WIDTH * 0.5f, SCREEN_HEIGHT - 100.f),
        GameState::Lobby,
        HUD::RenderMode::ScreenSpace,
        true
    );
    game->GetHUD().updateBaseColor("startGame", sf::Color::Black);

    game->GetHUD().addElement(
        "returnMain",
        "",
        24,
        sf::Vector2f(SCREEN_WIDTH * 0.5f, SCREEN_HEIGHT - 60.f),
        GameState::Lobby,
        HUD::RenderMode::ScreenSpace,
        true
    );
    game->GetHUD().updateBaseColor("returnMain", sf::Color::Black);
}

//---------------------------------------------------------
// IsFullyLoaded: Verify that all game components are ready.
//---------------------------------------------------------
bool LobbyState::IsFullyLoaded() {
    return (
        game->GetEntityManager()->areEntitiesInitialized() &&
        game->GetNetworkManager()->isInitialized() &&
        game->GetNetworkManager()->isLoaded() &&
        game->GetHUD().isFullyLoaded() &&
        game->GetWindow().isOpen()
    );
}

//---------------------------------------------------------
// Update: Update lobby members and HUD prompts.
//---------------------------------------------------------
void LobbyState::Update(float dt)
{
    UpdateLobbyMembers();

    // For non-host clients, send PLAYER_LOADED message once when fully loaded.
    if (!game->IsHost() && IsFullyLoaded() && !loadedMessageSent) {
        loadedMessageSent = true;
        std::string loadedMsg = "PLAYER_LOADED|" + std::to_string(game->GetLocalPlayer().steamID.ConvertToUint64());
        const char* hostStr = SteamMatchmaking()->GetLobbyData(game->GetLobbyID(), "host_steam_id");
        if (hostStr && *hostStr) {
            CSteamID hostID(std::stoull(hostStr));
            game->GetNetworkManager()->sendMessage(hostID, loadedMsg);
            std::cout << "[DEBUG] Client sent PLAYER_LOADED message to host: " << loadedMsg << "\n";
        }
    }

    // Assign up to 12 players to the 12 slots.
    int i = 0;
    for (auto& playerPair : game->GetPlayers()) {
        if (i >= 12) break;

        const auto& player = playerPair.second;
        const char* steamName = (SteamFriends()) ? SteamFriends()->GetFriendPersonaName(player.steamID) : "Unknown";

        // Update HUD for player's name.
        std::string nameId = "playerNameSlot" + std::to_string(i);
        game->GetHUD().updateText(nameId, steamName);

        // Update HUD for player's ready status.
        std::string readyId = "playerReadySlot" + std::to_string(i);
        game->GetHUD().updateText(readyId, player.ready ? "Ready" : "Not Ready");

        ++i;
    }

    // Clear remaining slots.
    for (; i < 12; ++i)
    {
        std::string nameId  = "playerNameSlot" + std::to_string(i);
        std::string readyId = "playerReadySlot" + std::to_string(i);
        game->GetHUD().updateText(nameId, "Empty");
        game->GetHUD().updateText(readyId, "");
    }

    // Set prompts based on host status.
    if (game->GetLocalPlayer().steamID == SteamMatchmaking()->GetLobbyOwner(game->GetLobbyID()))
    {
        game->GetHUD().updateText("startGame", "Press S to Start Game (Host Only)");
        game->GetHUD().updateText("returnMain", "Press M to Return to Main Menu");
    }
    else
    {
        game->GetHUD().updateText("startGame", "");
        game->GetHUD().updateText("returnMain", "Press M to Return to Main Menu");
    }
}

//---------------------------------------------------------
// Render: Draw the lobby UI including header, player slots, and HUD.
//---------------------------------------------------------
void LobbyState::Render()
{
    // Clear window.
    game->GetWindow().clear(sf::Color::White);

    // Optionally draw in-world player shapes.
    for (const auto& player : game->GetPlayers()) {
        game->GetWindow().draw(player.second.shape);
    }

    // Set view to default for UI rendering.
    game->GetWindow().setView(game->GetWindow().getDefaultView());

    // Draw header bar.
    sf::RectangleShape headerBar;
    headerBar.setSize(sf::Vector2f(SCREEN_WIDTH, 60.f));
    headerBar.setFillColor(sf::Color(70, 130, 180)); // Steel blue.
    headerBar.setPosition(0.f, 0.f);
    game->GetWindow().draw(headerBar);

    // Draw all player slot rectangles.
    for (auto& rect : m_playerSlotRects) {
        game->GetWindow().draw(rect);
    }

    // Render HUD elements.
    game->GetHUD().render(game->GetWindow(), game->GetWindow().getDefaultView(), game->GetCurrentState());

    game->GetWindow().display();
}

//---------------------------------------------------------
// ProcessEvent: Delegate to internal event processor.
//---------------------------------------------------------
void LobbyState::ProcessEvent(const sf::Event& event)
{
    ProcessEvents(event);
}

//---------------------------------------------------------
// ProcessEvents: Handle key input for lobby actions.
//---------------------------------------------------------
void LobbyState::ProcessEvents(const sf::Event& event)
{
    if (event.type == sf::Event::KeyPressed)
    {
        // Toggle ready state when "R" is pressed.
        if (event.key.code == sf::Keyboard::R)
        {
            game->ToggleReady();
        }
        // Start game (host only) when "S" is pressed.
        else if (event.key.code == sf::Keyboard::S &&
                 game->GetLocalPlayer().steamID == SteamMatchmaking()->GetLobbyOwner(game->GetLobbyID()))
        {
            if(game->GetPlayersAreLoaded() == false){
                std::cout << "[DEBUG] Not all players are loaded \n";
            }
            else if (game->GetPlayers().size() <= 1 ||
                     (game->GetPlayers().size() > 1 && (AllPlayersReady() || game->HasGameBeenPlayed())))
            {
                game->StartGame();
            }
            else
            {
                std::cout << "[DEBUG] Cannot start: multiple players not all ready\n";
            }
        }
        // Return to main menu when "M" is pressed.
        else if (event.key.code == sf::Keyboard::M)
        {
            game->ReturnToMainMenu();
        }
    }
}

//---------------------------------------------------------
// UpdateLobbyMembers: Synchronize lobby member list with Steam data.
//---------------------------------------------------------
void LobbyState::UpdateLobbyMembers()
{
    if (!game->IsInLobby()) return;

    int memberCount = SteamMatchmaking()->GetNumLobbyMembers(game->GetLobbyID());

    // Add new players not yet in the list.
    for (int i = 0; i < memberCount; ++i)
    {
        CSteamID member = SteamMatchmaking()->GetLobbyMemberByIndex(game->GetLobbyID(), i);
        if (!game->GetPlayers().count(member))
        {
            Player p;
            p.initialize();
            p.steamID = member;
            game->GetPlayers()[member] = p;
        }

        // Update ready status from lobby data.
        const char* readyState = SteamMatchmaking()->GetLobbyMemberData(game->GetLobbyID(), member, "ready");
        game->GetPlayers()[member].ready = (readyState && std::string(readyState) == "1");
    }

    // Remove players who have left the lobby.
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
            it = game->GetPlayers().erase(it);
        else
            ++it;
    }
}

//---------------------------------------------------------
// AllPlayersReady: Check if every player is marked ready.
//---------------------------------------------------------
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
void LobbyState::Interpolate(float alpha) {
    // No interpolation needed for a static lobby search screen
}