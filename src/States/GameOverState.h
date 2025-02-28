#ifndef GAMEOVERSTATE_H
#define GAMEOVERSTATE_H

#include "State.h"

class GameOverState : public State {
public:
    GameOverState(CubeGame* game);
    void Update(float dt) override;
    void Render() override;
    void ProcessEvent(const sf::Event& event) override; // Added

private:
    void ProcessEvents(const sf::Event& event); // Renamed
    void UpdateLeaderboard();
};

#endif // GAMEOVERSTATE_H