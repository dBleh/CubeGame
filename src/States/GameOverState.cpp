#include "GameOverState.h"

GameOverState::GameOverState(CubeGame* game) : State(game) {
    game->GetHUD().addElement("gameOver", "Game Over", 24, sf::Vector2f(SCREEN_WIDTH/2 - 100, 200), GameState::GameOver, HUD::RenderMode::ViewSpace);
    game->GetHUD().addElement("leaderboard", "Leaderboard:\n", 24, sf::Vector2f(SCREEN_WIDTH/2 - 100, 250), GameState::GameOver, HUD::RenderMode::ViewSpace);
    game->GetHUD().addElement("return", "Press M to Return to Lobby", 20, sf::Vector2f(SCREEN_WIDTH/2 - 150, 350), GameState::GameOver, HUD::RenderMode::ViewSpace);
}

void GameOverState::Update(float dt) {
    std::string leaderboard = "Leaderboard:\n";
    for (const auto& player : game->GetPlayers()) {
        leaderboard += "Player " + std::to_string(player.second.steamID.ConvertToUint64() % 10000) +
                      ": Kills=" + std::to_string(player.second.kills) + "\n";
    }
    game->GetHUD().updateText("leaderboard", leaderboard);
}

void GameOverState::Render() {
    game->GetWindow().clear(sf::Color::Black);
    
    for (const auto& player : game->GetPlayers()) {
        game->GetWindow().draw(player.second.shape);
    }
    for (const auto& enemy : game->GetEnemies()) {
        game->GetWindow().draw(enemy.second.shape);
    }
    
    game->RenderHUDLayer();
    game->GetWindow().display();
}

void GameOverState::ProcessEvent(const sf::Event& event) {
    if (event.type == sf::Event::KeyPressed) {
        if (event.key.code == sf::Keyboard::M) {
            game->ReturnToLobby();
        }
    }
}