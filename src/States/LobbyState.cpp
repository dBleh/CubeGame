#include "LobbyState.h"
#include <iostream>
#include <Steam/steam_api.h>

//---------------------------------------------------------
// Constructor: Set up lobby HUD elements and slots.
//---------------------------------------------------------
LobbyState::LobbyState(Game* game)
    : State(game),
      playerLoaded(false),
      loadingTimer(0.f),
      localPlayer(sf::Vector2f(400.f, 300.f), sf::Color::Blue)  // starting position and color
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

    // Create a HUD element for the player loading indicator.
    game->GetHUD().addElement(
         "playerLoading",
         "Loading player...",
         24,
         sf::Vector2f(50.f, SCREEN_HEIGHT - 150.f),
         GameState::Lobby,
         HUD::RenderMode::ScreenSpace,
         false
    );

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
        "Press M to Return to Main Menu",
        24,
        sf::Vector2f(SCREEN_WIDTH * 0.5f, SCREEN_HEIGHT - 60.f),
        GameState::Lobby,
        HUD::RenderMode::ScreenSpace,
        true
    );
    game->GetHUD().updateBaseColor("returnMain", sf::Color::Black);

    // --- Set up the NetworkManager message handler ---
    // This lambda will be called whenever a network message is received.
    game->GetNetworkManager().SetMessageHandler([this, game](const std::string& msg, CSteamID sender) {
        // Parse the incoming message.
        ParsedMessage parsed = MessageHandler::ParseMessage(msg);
        // Only proceed if the message has a known type.
        if (parsed.type == MessageType::Connection) {
            // If we are the host, process connection messages.
            if (SteamUser()->GetSteamID() == SteamMatchmaking()->GetLobbyOwner(game->GetLobbyID())) {
                // Use the steamID from the parsed message as the key.
                std::string key = parsed.steamID;
                // Check if we already have this player; if so, update their info.
                if (remotePlayers.find(key) == remotePlayers.end()) {
                    // Create a new RemotePlayer.
                    RemotePlayer newPlayer;
                    // Initialize the player's cube at a default position.
                    newPlayer.player.SetPosition(sf::Vector2f(200.f, 200.f));
                    // Optionally, set the cube's color from the message.
                    newPlayer.player.GetShape().setFillColor(parsed.color);
                    // Set up the name text using the Game's font.
                    newPlayer.nameText.setFont(game->GetFont());
                    newPlayer.nameText.setString(parsed.steamName);
                    newPlayer.nameText.setCharacterSize(16);
                    newPlayer.nameText.setFillColor(sf::Color::Black);
                    // Position will be updated in UpdateRemotePlayers().
                    remotePlayers[key] = newPlayer;
                    
                    std::cout << "[HOST] New connection: " << parsed.steamID << " (" << parsed.steamName << ")" << std::endl;
                    // Broadcast the updated players list.
                    BroadcastPlayersList();
                } else {
                    // Already connected: update steam name or color if necessary.
                    remotePlayers[key].nameText.setString(parsed.steamName);
                    remotePlayers[key].player.GetShape().setFillColor(parsed.color);
                }
            }
        } else if (parsed.type == MessageType::Movement) {
            // Process movement messages.
            std::string key = parsed.steamID;
            auto it = remotePlayers.find(key);
            if (it != remotePlayers.end()) {
                // Update the remote player's position.
                it->second.player.SetPosition(parsed.position);
            }
        }
    });
}
void LobbyState::BroadcastPlayersList() {
    // Only the host should broadcast.
    if (SteamUser()->GetSteamID() != SteamMatchmaking()->GetLobbyOwner(game->GetLobbyID()))
        return;
    
    for (const auto& pair : remotePlayers) {
        const RemotePlayer& rp = pair.second;
        // Prepare the connection message for each remote player.
        // Use MessageHandler::FormatConnectionMessage
        // We'll assume rp.nameText.getString() converts to std::string properly.
        std::string msg = MessageHandler::FormatConnectionMessage(
            pair.first,
            rp.nameText.getString().toAnsiString(),
            rp.player.GetShape().getFillColor()
        );
        // Broadcast the message.
        game->GetNetworkManager().BroadcastMessage(msg);
    }
}

void LobbyState::UpdateRemotePlayers() {
    for (auto& pair : remotePlayers) {
        RemotePlayer& rp = pair.second;
        sf::Vector2f pos = rp.player.GetPosition();
        rp.nameText.setPosition(pos.x, pos.y - 20.f);
    }
    
}
//---------------------------------------------------------
// Update: Update HUD prompts.
//---------------------------------------------------------
void LobbyState::Update(float dt)
{
    // Simulate a loading delay (e.g., 2 seconds) before the local player is fully loaded.
    if (!playerLoaded) {
        loadingTimer += dt;
        if (loadingTimer >= 2.0f) {
            playerLoaded = true;
            // Remove the loading indicator.
            game->GetHUD().updateText("playerLoading", "");
        }
    } else {
        // Once loaded, update the local player's movement.
        localPlayer.Update(dt);
        
        // (For local movement, you would also send a movement message when keys are pressed or released.
            // That integration (sending messages on key state changes) would be added here.)
    }
    
    // Update the remote players' name positions.
    UpdateRemotePlayers();
    
    // Update HUD prompts based on host status.
    if (SteamUser()->GetSteamID() == SteamMatchmaking()->GetLobbyOwner(game->GetLobbyID())) {
        game->GetHUD().updateText("startGame", "Press S to Start Game (Host Only)");
    } else {
        game->GetHUD().updateText("startGame", "");
    }
}

//---------------------------------------------------------
// Render: Draw the lobby UI including header, slots, and HUD.
//---------------------------------------------------------
void LobbyState::Render()
{
    game->GetWindow().clear(sf::Color::White);

    // Draw the local player's cube if loaded.
    if (playerLoaded) {
        game->GetWindow().draw(localPlayer.GetShape());
        // Optionally, draw local player's name if desired.
    }

    // Draw each remote player's cube and their steam name.
    for (const auto& pair : remotePlayers) {
        const RemotePlayer& rp = pair.second;
        game->GetWindow().draw(rp.player.GetShape());
        game->GetWindow().draw(rp.nameText);
    }

    // Render the HUD elements.
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
        // Host pressing 'S' to start game (functionality commented out for now).
        if (event.key.code == sf::Keyboard::S &&
            SteamUser()->GetSteamID() == SteamMatchmaking()->GetLobbyOwner(game->GetLobbyID()))
        {
            std::cout <<"User wants to start game" << std::endl;
            // startGameFunction();  // (Commented out for now)
        }
    }
}


//---------------------------------------------------------
// UpdateLobbyMembers: Removed as it dealt with players
//---------------------------------------------------------
void LobbyState::UpdateLobbyMembers()
{
    // Function left empty or could be removed entirely from header
    if (!game->IsInLobby()) return;
}

//---------------------------------------------------------
// AllPlayersReady: Removed as it dealt with players
//---------------------------------------------------------
bool LobbyState::AllPlayersReady()
{
    return true; // Default to true since no player checks remain
}

//---------------------------------------------------------
// IsFullyLoaded: Verify that all game components are ready.
//---------------------------------------------------------

bool LobbyState::IsFullyLoaded() {
    return (
        game->GetHUD().isFullyLoaded() &&
        game->GetWindow().isOpen() &&
        playerLoaded
    );
}