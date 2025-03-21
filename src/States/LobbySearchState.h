#ifndef LOBBY_SEARCH_STATE_H
#define LOBBY_SEARCH_STATE_H

#include "State.h"
#include <vector>
#include <steam/steam_api.h>

class LobbySearchState : public State {
public:
    explicit LobbySearchState(Game* game);
    void Update(float dt) override;
    void Render() override;
    void ProcessEvent(const sf::Event& event) override;

private:
    void SearchLobbies();
    void ProcessEvents(const sf::Event& event);
    void UpdateLobbyListDisplay();
    void JoinLobby(CSteamID lobby);
    void JoinLobbyByIndex(int index);

    std::vector<std::pair<CSteamID, std::string>> lobbyList;
    bool lobbyListUpdated{false};
};

#endif // LOBBY_SEARCH_STATE_H