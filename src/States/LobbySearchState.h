#ifndef LOBBYSEARCHSTATE_H
#define LOBBYSEARCHSTATE_H

#include "State.h"

/**
 * @brief State for searching and joining lobbies.
 *
 * Displays a list of available lobbies and allows the user to select one
 * using number keys. Also handles returning to the main menu.
 */
class LobbySearchState : public State {
public:
    /**
     * @brief Construct a new LobbySearchState object.
     * @param game Pointer to the main CubeGame instance.
     */
    LobbySearchState(CubeGame* game);

    /// Update logic (e.g., refresh lobby list display).
    void Update(float dt) override;

    /// Render the lobby search UI.
    void Render() override;

    /// Process SFML events.
    void ProcessEvent(const sf::Event& event) override;

private:
    /// Internal method to process events.
    void ProcessEvents(const sf::Event& event);

    /// Update the HUD element displaying the lobby list.
    void UpdateLobbyListDisplay();

    /// Initiate a lobby search via the Steam API.
    void SearchLobbies();

    /// Attempt to join a specific lobby.
    void JoinLobby(CSteamID lobby);

    /// Join a lobby by its index in the displayed list.
    void JoinLobbyByIndex(int index);
};

#endif // LOBBYSEARCHSTATE_H
