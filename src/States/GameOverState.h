#ifndef GAMEOVERSTATE_H
#define GAMEOVERSTATE_H

#include "State.h"
#include <steam/steam_api.h> 

class GameOverState : public State {
public:
    GameOverState(CubeGame* game);
    void Update(float dt) override;
    void Render() override;
    void ProcessEvent(const sf::Event& event) override;

private:
    void UpdateLeaderboard();
};

#endif // GAMEOVERSTATE_H