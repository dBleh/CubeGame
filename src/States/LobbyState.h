#ifndef LOBBYSTATE_H
#define LOBBYSTATE_H

#include "State.h"
#include "../Core/CubeGame.h"
#include <SFML/Graphics.hpp>
#include <array>
/**
 * LobbyState: Manages the lobby screen.
 * Displays a modern, white, rounded-panel background and updates lobby info.
 */
// LobbyState.h

    

class LobbyState : public State {
public:
    explicit LobbyState(CubeGame* game);
    
    virtual void Update(float dt) override;
    virtual void Render() override;
    virtual void ProcessEvent(const sf::Event& event) override;
    bool IsFullyLoaded();
private:
    bool loadedMessageSent = false;

    std::array<sf::RectangleShape, 12> m_playerSlotRects;
    void ProcessEvents(const sf::Event& event);
    void UpdateLobbyMembers();
    bool AllPlayersReady();
};

#endif // LOBBYSTATE_H
