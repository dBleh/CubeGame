#ifndef LOBBYSTATE_H
#define LOBBYSTATE_H

#include "State.h"

class LobbyState : public State {
public:
    LobbyState(CubeGame* game);
    void Update(float dt) override;
    void Render() override;
    void ProcessEvent(const sf::Event& event) override;

private:
    void ProcessEvents(const sf::Event& event);
    void UpdateLobbyMembers();
    bool AllPlayersReady();
};

#endif // LOBBYSTATE_H