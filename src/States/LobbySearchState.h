#ifndef LOBBYSEARCHSTATE_H
#define LOBBYSEARCHSTATE_H

#include "State.h"

class LobbySearchState : public State {
public:
    LobbySearchState(CubeGame* game);
    void Update(float dt) override;
    void Render() override;
    void ProcessEvent(const sf::Event& event) override;

private:
    void ProcessEvents(const sf::Event& event);
};

#endif // LOBBYSEARCHSTATE_H