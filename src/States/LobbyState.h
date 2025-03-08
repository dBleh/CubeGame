#ifndef LOBBYSTATE_H
#define LOBBYSTATE_H

#include "State.h"
#include "../Core/CubeGame.h"
#include <SFML/Graphics.hpp>
#include <array>

/**
 * @brief Manages the lobby screen.
 *
 * Displays a header, player slots, and prompts for starting the game or returning to the main menu.
 */
class LobbyState : public State {
public:
    /**
     * @brief Constructs a new LobbyState object.
     * @param game Pointer to the main CubeGame instance.
     */
    explicit LobbyState(CubeGame* game);

    /// Update lobby state (e.g., update lobby members and HUD).
    void Update(float dt) override;
    /// Render the lobby screen.
    void Render() override;
    /// Process user input events.
    void ProcessEvent(const sf::Event& event) override;

    /// Check if the game and HUD are fully loaded.
    bool IsFullyLoaded();

private:
    bool loadedMessageSent = false; ///< Flag to ensure PLAYER_LOADED message is sent once.

    std::array<sf::RectangleShape, 12> m_playerSlotRects; ///< Rectangles for up to 12 player slots.
    
    /// Internal event processing.
    void ProcessEvents(const sf::Event& event);
    /// Update the HUD with current lobby member information.
    void UpdateLobbyMembers();
    /// Check if all players in the lobby are marked as ready.
    bool AllPlayersReady();
};

#endif // LOBBYSTATE_H
