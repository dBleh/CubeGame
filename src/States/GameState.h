#ifndef GAMESTATE_H
#define GAMESTATE_H

enum class GameState {
    MainMenu,
    LobbyCreation,
    LobbySearch,
    Lobby,
    Playing,
    Spectating,
    Leaderboard,
    Shopping,
    GameOver
};

#endif // GAMESTATE_H