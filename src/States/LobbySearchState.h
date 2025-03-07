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
    void UpdateLobbyListDisplay(); // New method to update HUD with lobby list
    void SearchLobbies();
    void JoinLobby(CSteamID lobby);
    void JoinLobbyByIndex(int index);

};

#endif // LOBBYSEARCHSTATE_H