#ifndef LOBBYCREATIONSTATE_H
#define LOBBYCREATIONSTATE_H

#include "../Game.h"  // Assuming Game.h includes necessary dependencies
#include <steam/steam_api.h>
#include "State.h"
class Game;
class LobbyCreationState : public State {
public:
    LobbyCreationState(Game* game);
    void Update(float dt) override;
    void Render() override;
    void ProcessEvent(const sf::Event& event) override;

    // Function to handle lobby entry logic after creation, without player references
    void onLobbyEnter(CSteamID lobbyID);

private:
    void ProcessEvents(const sf::Event& event);
    void CreateLobby(const std::string& lobbyName);
};

#endif // LOBBYCREATIONSTATE_H