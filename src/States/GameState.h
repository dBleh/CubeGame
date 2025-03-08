#ifndef GAMESTATE_H
#define GAMESTATE_H

/**
 * @brief Enumeration of the various game states.
 */
enum class GameState {
    MainMenu,       ///< Main menu of the game.
    LobbyCreation,  ///< Lobby creation screen.
    LobbySearch,    ///< Lobby search screen.
    Lobby,          ///< In-lobby state.
    Playing,        ///< Active gameplay.
    Spectating,     ///< Spectator mode.
    Leaderboard,    ///< Leaderboard display.
    Shopping,       ///< In-game store.
    GameOver        ///< Game over screen.
};

#endif // GAMESTATE_H
