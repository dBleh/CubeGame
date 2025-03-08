#include "GameOverState.h"
#include <steam/steam_api.h> // Required for SteamFriends()
#include <iostream>

//---------------------------------------------------------
// Constructor: Set up HUD elements for Game Over
//---------------------------------------------------------
GameOverState::GameOverState(CubeGame* game) : State(game) {
    // Add and configure "Game Over" title element.
    game->GetHUD().addElement("gameOver", "Game Over", 36, 
                               sf::Vector2f(SCREEN_WIDTH / 2.f - 100.f, 150.f), 
                               GameState::GameOver, HUD::RenderMode::ViewSpace, false);
    game->GetHUD().updateBaseColor("gameOver", sf::Color::Red);

    // Add and configure leaderboard element.
    game->GetHUD().addElement("leaderboard", "Leaderboard:\n", 24, 
                               sf::Vector2f(SCREEN_WIDTH / 2.f - 150.f, 200.f), 
                               GameState::GameOver, HUD::RenderMode::ViewSpace, false);
    game->GetHUD().updateBaseColor("leaderboard", sf::Color::White);

    // Add and configure instruction element for returning to the lobby.
    game->GetHUD().addElement("return", "Press Enter to Continue or M to Return to Lobby", 20, 
                               sf::Vector2f(SCREEN_WIDTH / 2.f - 200.f, SCREEN_HEIGHT - 100.f), 
                               GameState::GameOver, HUD::RenderMode::ViewSpace, false);
    game->GetHUD().updateBaseColor("return", sf::Color::Yellow);
}

//---------------------------------------------------------
// Update: Refresh leaderboard information
//---------------------------------------------------------
void GameOverState::Update(float dt) {
    UpdateLeaderboard();
}

//---------------------------------------------------------
// Render: Draw game over screen elements
//---------------------------------------------------------
void GameOverState::Render() {
    game->GetWindow().clear(sf::Color::Black);

    // Render all player shapes (frozen state).
    for (const auto& player : game->GetPlayers()) {
        game->GetWindow().draw(player.second.shape);
    }
    // Render all enemy shapes (frozen state).
    for (const auto& enemy : game->GetEnemies()) {
        game->GetWindow().draw(enemy.second.shape);
    }

    // Render HUD elements for Game Over.
    game->GetHUD().render(game->GetWindow(), game->GetWindow().getDefaultView(), game->GetCurrentState());
    game->GetWindow().display();
}

//---------------------------------------------------------
// ProcessEvent: Handle key inputs to return to lobby
//---------------------------------------------------------
void GameOverState::ProcessEvent(const sf::Event& event) {
    if (event.type == sf::Event::KeyPressed) {
        if (event.key.code == sf::Keyboard::Return || event.key.code == sf::Keyboard::M) {
            if (game->IsHost()) {
                game->ReturnToLobby();
                char buffer[64];
                int bytes = snprintf(buffer, sizeof(buffer), "S|LOBBY");
                if (bytes > 0 && static_cast<size_t>(bytes) < sizeof(buffer)) {
                    game->GetNetworkManager()->SendGameplayMessage(std::string(buffer));
                    std::cout << "[DEBUG] Host sent S|LOBBY to return to lobby" << std::endl;
                }
            }
        }
    }
}

//---------------------------------------------------------
// UpdateLeaderboard: Refresh and update leaderboard text
//---------------------------------------------------------
void GameOverState::UpdateLeaderboard() {
    std::string leaderboard = "Leaderboard:\n";
    for (const auto& player : game->GetPlayers()) {
        const char* steamName = SteamFriends() ? SteamFriends()->GetFriendPersonaName(player.second.steamID) : "Unknown";
        if (!steamName || steamName[0] == '\0') {
            steamName = "Unknown"; // Fallback if name is empty.
        }
        leaderboard += std::string(steamName) +
                       ": Kills=" + std::to_string(player.second.kills) +
                       ", Money=" + std::to_string(player.second.money) +
                       (player.second.isAlive ? " (Alive)" : " (Dead)") + "\n";
    }
    game->GetHUD().updateText("leaderboard", leaderboard);
}
void GameOverState::Interpolate(float alpha) {
    // No interpolation needed for a static lobby search screen
}