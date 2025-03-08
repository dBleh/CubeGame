#ifndef LOBBYCREATIONSTATE_H
#define LOBBYCREATIONSTATE_H

#include "State.h"

/**
 * @brief State for creating a new lobby.
 *
 * Handles user input for lobby name creation and transitions to the lobby.
 */
class LobbyCreationState : public State {
public:
    /**
     * @brief Construct a new LobbyCreationState object.
     * @param game Pointer to the main CubeGame instance.
     */
    LobbyCreationState(CubeGame* game);
    void Interpolate(float alpha) override;
    /// Update logic (if needed) for lobby creation.
    void Update(float dt) override;

    /// Render the lobby creation UI.
    void Render() override;

    /// Process SFML events.
    void ProcessEvent(const sf::Event& event) override;

private:
    /// Internal handler for processing SFML events.
    void ProcessEvents(const sf::Event& event);
    
    /// Attempt to create a lobby with the provided lobby name.
    void CreateLobby(const std::string& lobbyName);
};

#endif // LOBBYCREATIONSTATE_H
