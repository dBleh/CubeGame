#ifndef STATE_H
#define STATE_H

#include "../Core/CubeGame.h"

class State {
public:
    State(CubeGame* game) : game(game) {}
    virtual ~State() = default;
    virtual void Update(float dt) = 0;
    virtual void Render() = 0;
    virtual void ProcessEvent(const sf::Event& event) = 0; // Added

protected:
    CubeGame* game;
};

#endif // STATE_H