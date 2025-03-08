#ifndef STATE_H
#define STATE_H

#include "../Core/CubeGame.h"

/**
 * @brief Base class for game states.
 *
 * Provides virtual methods for updating, rendering, and processing events.
 */
class State {
public:
    /**
     * @brief Construct a new State object.
     * @param game Pointer to the main CubeGame instance.
     */
    State(CubeGame* game) : game(game) {}
    virtual ~State() = default;

    /// Update the state logic.
    virtual void Update(float dt) = 0;
    /// Render the state.
    virtual void Render() = 0;
    /// Process input events.
    virtual void ProcessEvent(const sf::Event& event) = 0;

protected:
    CubeGame* game; ///< Pointer to the main game instance.
};

#endif // STATE_H
