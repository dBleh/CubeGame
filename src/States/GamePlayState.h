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
 * @brief GameplayState manages game logic and rendering when the game is in play mode.
 */
class GameplayState : public State {
public:
    /**
     * @brief Construct a new GameplayState object.
     * @param game Pointer to the main CubeGame instance.
     */
    GameplayState(CubeGame* game);

    /**
     * @brief Update the game logic.
     * @param dt Delta time since the last update.
     */
    void Update(float dt) override;

    /**
     * @brief Render the current game state.
     */
    void Render() override;

    /**
     * @brief Process an SFML event.
     * @param event The event to process.
     */
    void ProcessEvent(const sf::Event& event) override;

    /**
     * @brief Spawn enemies into the game.
     */
    void SpawnEnemies();

    /// Flag indicating whether the store UI is visible.
    bool storeVisible = false;
    void StartNextLevelTimer(float duration);
    void CheckAndAdvanceLevel();
    float nextLevelTimer; // Timer for next wave
    bool timerActive = false;
    bool IsFullyLoaded();
private:
bool menuVisible = false;
struct PendingHit {
    uint64_t bulletId;
    uint64_t enemyId;
    uint64_t shooterSteamID;
    float retryTimer;
};
std::vector<PendingHit> pendingHits;
    //==========================================================================
    // Update Helpers
    //==========================================================================
    void UpdateShoppingState(float dt);
    /**
     * @brief Update game logic when in Playing state.
     * @param dt Delta time.
     */
    void UpdatePlayingState(float dt);

    /**
     * @brief Update game logic when in Shopping state.
     * @param dt Delta time.
     */

    /**
     * @brief Update game logic when in Spectating state.
     * @param dt Delta time.
     */
    void UpdateSpectatingState(float dt);

    /**
     * @brief Update the HUD text elements.
     */
    void UpdateHUD();
    void InterpolateEntities(float dt);
    //==========================================================================
    // Rendering Helpers
    //==========================================================================

    /**
     * @brief Render all player entities.
     */
    void RenderPlayers();

    /**
     * @brief Render all enemy entities.
     */
    void RenderEnemies();

    /**
     * @brief Render all bullet entities.
     */
    void RenderBullets();

    /**
     * @brief Render the store UI elements.
     */
    void RenderStoreUI();
    void ConfigureGameplayHUD();
    void ConfigureStoreHUD();
    //==========================================================================
    // Action Helpers
    //==========================================================================

    /**
     * @brief Send a player update message to the network.
     */
    void SendPlayerUpdate();

    /**
     * @brief Update the game camera based on the average player position.
     * @param dt Delta time.
     */
    void UpdateCamera(float dt);

    /**
     * @brief Handle shooting a bullet.
     */
    void ShootBullet();

    /**
     * @brief Transition to the next level.
     */
    void NextLevel();

    /**
     * @brief Process input for store purchases.
     */
    void HandleStorePurchase();

    /**
     * @brief Update the scoreboard displayed in the HUD.
     */
    void UpdateScoreboard();
    //==========================================================================
    // Initialization Helpers
    //==========================================================================

    /**
     * @brief Initialize HUD elements for gameplay.
     */
    void InitializeHUD();

    /**
     * @brief Initialize store UI elements.
     */
    void InitializeStoreUI();

    //==========================================================================
    // Placeholder Methods
    //==========================================================================
    bool AllPlayersDead();
    void SendBulletData(const Bullet& b) {}
    void SendEnemyUpdate() {}
    void SpawnSniperBullet(uint64_t enemyId, float targetX, float targetY) {}

    //==========================================================================
    // UI Elements and State Variables
    //==========================================================================
    sf::Text storeTitle;
    sf::Text speedBoostText;
    sf::RectangleShape speedBoostButton;
    sf::Font font;
    bool shopOpen; // Replaced storeVisible
    
    float lastEnemyUpdateTime = 0.0f;
    const float enemyUpdateRate = 0.2f;
   
    bool showHealthBars = false;
    sf::RectangleShape arrowShape;
    int spectatedPlayerIndex = -1; // Index of the player being spectated
    
};

#endif // GAMEPLAYSTATE_H