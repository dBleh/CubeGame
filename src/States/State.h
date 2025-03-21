#ifndef STATE_H
#define STATE_H

#include <SFML/Graphics.hpp>
#include "../Game.h"

class State {
public:
    explicit State(Game* game) : game(game) {}
    virtual ~State() = default;

    virtual void Update(float dt) = 0;
    virtual void Render() = 0;
    virtual void ProcessEvent(const sf::Event& event) = 0;

protected:
    Game* game;
};

#endif // STATE_H