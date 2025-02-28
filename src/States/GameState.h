#ifndef GAMESTATE_H
#define GAMESTATE_H

enum class GameState {
    MainMenu,
    LobbyCreation,
    LobbySearch,  // New state for searching lobbies
    Lobby,
    Playing,
    Spectating,
    Leaderboard,
    GameOver
};

#endif // GAMESTATE_H