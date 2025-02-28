#ifndef MAINMENUSTATE_H
#define MAINMENUSTATE_H

#include "State.h"

class MainMenuState : public State {
public:
    MainMenuState(CubeGame* game);
    void Update(float dt) override;
    void Render() override;
    void ProcessEvent(const sf::Event& event) override;

private:
    void ProcessEvents(const sf::Event& event);
    sf::RectangleShape createLobbyButton;  // Clickable area for "Create Lobby"
    sf::RectangleShape searchLobbiesButton; // Clickable area for "Search Lobbies"
};

#endif // MAINMENUSTATE_H