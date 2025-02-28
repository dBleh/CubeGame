#ifndef LOBBYCREATIONSTATE_H
#define LOBBYCREATIONSTATE_H

#include "State.h"

class LobbyCreationState : public State {
public:
    LobbyCreationState(CubeGame* game);
    void Update(float dt) override;
    void Render() override;
    void ProcessEvent(const sf::Event& event) override;

private:
    void ProcessEvents(const sf::Event& event);
};

#endif // LOBBYCREATIONSTATE_H