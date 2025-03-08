#ifndef GAMEPLAYSTATE_H
#define GAMEPLAYSTATE_H

#include "State.h"
#include <SFML/Graphics.hpp>
#include <cmath>
#include <steam/steam_api.h>
#include "../Utils/Config.h"

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

/**
 * @brief GameplayState manages game logic and rendering while in play mode.
 *
 * This state handles player movement, enemy spawning and updates, camera control,
 * HUD updates, collision checks, and transitioning between levels.
 */
class GameplayState : public State {
public:
    // Structure to store pending hit information for retrying if enemy not found yet.
    struct PendingHit {
        uint64_t bulletId;
        uint64_t enemyId;
        uint64_t shooterSteamID;
        float retryTimer;
    };

    /**
     * @brief Construct a new GameplayState object.
     * @param game Pointer to the main CubeGame instance.
     */
    GameplayState(CubeGame* game);

    /// Update game logic.
    void Update(float dt) override;

    /// Render the current game state.
    void Render() override;

    /// Process input events.
    void ProcessEvent(const sf::Event& event) override;

    /// Spawn enemies (wrapper function, if needed).
    void SpawnEnemies();

    /// Update enemy vertex data for batch rendering.
    void updateEnemyVertices();
    void Interpolate(float alpha) override; // Add interpolation method

    /// Start the next level timer.
    void StartNextLevelTimer(float duration);

    /// Check for level completion and advance to the next level.
    void CheckAndAdvanceLevel();

    // Public state variables.
    sf::VertexArray enemyVertices; ///< Vertex array for enemy rendering.
    bool storeVisible = false;     ///< Flag indicating whether the store UI is visible.
    float nextLevelTimer;          ///< Timer for the next wave.
    bool timerActive = false;      ///< Indicates if the next-level timer is active.
    std::vector<PendingHit> pendingHits; ///< Pending hit events (for client-side prediction).

private:
    bool menuVisible = false;      ///< Flag for pause menu visibility.

    //===============================================================
    // Update Helper Methods
    //===============================================================
    void UpdatePlayingState(float dt);  ///< Update logic when game is in Playing state.
    void UpdateShoppingState(float dt); ///< Update logic for shopping state (if applicable).
    void UpdateSpectatingState(float dt); ///< Update logic when spectating (if applicable).
    void UpdateHUD();                   ///< Update HUD text elements.
    void InterpolateEntities(float dt); ///< Interpolate positions for smooth movement.

    //===============================================================
    // Rendering Helper Methods
    //===============================================================
    void RenderPlayers();   ///< Draw all player entities.
    void RenderEnemies();   ///< Draw all enemy entities.
    void RenderBullets();   ///< Draw all bullet entities.
    void RenderStoreUI();   ///< Draw store UI elements.
    void RenderGrid(sf::RenderWindow& window, const sf::View& camera); ///< Draw grid overlay.

    //===============================================================
    // Action Helper Methods
    //===============================================================
    void UpdateCamera(float dt);   ///< Update camera based on player position.
    void NextLevel();              ///< Transition to the next level.
    void HandleStorePurchase();    ///< Process store purchase input.
    void UpdateScoreboard();       ///< Update scoreboard in the HUD.

    //===============================================================
    // HUD & UI Initialization Helpers
    //===============================================================
    void InitializeHUD();          ///< Initialize HUD elements for gameplay.
    void InitializeStoreUI();      ///< Initialize store UI elements.
    
    //===============================================================
    // Placeholder and Utility Methods
    //===============================================================
    bool AllPlayersDead();         ///< Check if all players are dead.
    void SendBulletData(const Bullet& b) {} ///< (Placeholder) Send bullet data.
    void SendEnemyUpdate() {}              ///< (Placeholder) Send enemy update.
    void SpawnSniperBullet(uint64_t enemyId, float targetX, float targetY) {} ///< (Placeholder)

    //===============================================================
    // UI Elements and Additional State Variables
    //===============================================================
    sf::Text storeTitle;
    sf::Text speedBoostText;
    sf::RectangleShape speedBoostButton;
    sf::Font font;               ///< Local font for UI elements.
    bool shopOpen;               ///< True if the store UI is open (replaces storeVisible).

    float lastEnemyUpdateTime = 0.0f; ///< Accumulator for enemy update timing.
    const float enemyUpdateRate = 0.2f; ///< Interval for enemy update sync.
    float gridSize = 50.f;        ///< Grid square size for rendering grid overlay.
    bool showHealthBars = false;  ///< Option to display enemy health bars.
    sf::RectangleShape arrowShape; ///< Optional shape for directional indicators.
    int spectatedPlayerIndex = -1; ///< Index of player being spectated (if applicable).
};

#endif // GAMEPLAYSTATE_H
