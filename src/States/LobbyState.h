#ifndef LOBBYSTATE_H
#define LOBBYSTATE_H

#include "State.h"
#include "../Game.h"
#include "../entities/Player.h"
#include "../utils/MessageHandler.h"

#include <steam/steam_api.h>
#include <SFML/Graphics.hpp>

#include <array>
#include <unordered_map>
class Game;

struct RemotePlayer {
    Player player;
    sf::Text nameText;
};

class LobbyState : public State {
public:
    LobbyState(Game* game);
    bool IsFullyLoaded();
    void Update(float dt) override;
    void Render() override;
    void ProcessEvent(const sf::Event& event) override;
    

private:
    void UpdateRemotePlayers();
    void BroadcastPlayersList();
    void ProcessEvents(const sf::Event& event);
    void UpdateLobbyMembers(); // Kept as stub for compatibility
    bool AllPlayersReady();    // Kept as stub for compatibility
    Player localPlayer;    // The player's cube representation.
    bool playerLoaded;     // True when the local player has finished "loading".
    float loadingTimer;  
    std::unordered_map<std::string, RemotePlayer> remotePlayers;
};

#endif // LOBBYSTATE_H