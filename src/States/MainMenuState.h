#ifndef MAINMENUSTATE_H
#define MAINMENUSTATE_H

#include "State.h"
#include "../Hud/Hud.h"

/**
 * @brief Manages the Main Menu screen.
 *
 * Displays options for creating a lobby, searching for lobbies, and exiting the game.
 */
class MainMenuState : public State {
public:
    /**
     * @brief Constructs a new MainMenuState object.
     * @param game Pointer to the main CubeGame instance.
     */
    MainMenuState(CubeGame* game);
    void Interpolate(float alpha) override;
    /// Update menu logic (e.g., update header/status text).
    void Update(float dt) override;
    /// Render the main menu screen.
    void Render() override;
    /// Process user input events.
    void ProcessEvent(const sf::Event& event) override;

private:
    /// Internal method for processing events.
    void ProcessEvents(const sf::Event& event);

    // Invisible buttons for clickable areas.
    sf::RectangleShape createLobbyButton;  ///< Clickable area for "Create Lobby"
    sf::RectangleShape searchLobbiesButton; ///< Clickable area for "Search Lobbies"
    sf::RectangleShape exitGameButton;       ///< Clickable area for "Exit Game"
};

#endif // MAINMENUSTATE_H
