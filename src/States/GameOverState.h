#ifndef GAMEOVERSTATE_H
#define GAMEOVERSTATE_H

#include "State.h"
#include <steam/steam_api.h>

/**
 * @brief Represents the game state when the game is over.
 *
 * Displays the "Game Over" screen, leaderboard, and instructions to return to the lobby.
 */
class GameOverState : public State {
public:
    /**
     * @brief Construct a new GameOverState object.
     * @param game Pointer to the main CubeGame instance.
     */
    GameOverState(CubeGame* game);

    /// Update game over logic (e.g., refresh leaderboard).
    void Update(float dt) override;

    /// Render the game over screen.
    void Render() override;

    /// Process user input in the game over state.
    void ProcessEvent(const sf::Event& event) override;

private:
    /// Update the leaderboard displayed on the HUD.
    void UpdateLeaderboard();
};

#endif // GAMEOVERSTATE_H
