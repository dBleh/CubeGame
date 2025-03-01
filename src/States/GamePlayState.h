#ifndef GAMEPLAYSTATE_H
#define GAMEPLAYSTATE_H

#include "State.h"
#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif
class GameplayState : public State {
public:
    GameplayState(CubeGame* game);
    void Update(float dt) override;
    void Render() override;
    void ProcessEvent(const sf::Event& event) override;
    void initialize(float startX, float startY, float targetX, float targetY);
private:
    void ProcessEvents(const sf::Event& event);
    void UpdateCamera(float dt);
    void MoveEnemies(float dt);
    void UpdateBullets(float dt);
    void CheckCollisions();
    void SpawnEnemies();
    void ShootBullet();
    void NextLevel();
    void UpdateScoreboard();
    void ShowInGameMenu();
    bool AllPlayersDead();
    void SendBulletData(const Bullet& b);
    void SendEnemyUpdate(); 
    void SendGameStateUpdate();
    void SendPlayerUpdate();
    void SpawnSniperBullet(uint64_t enemyId, float targetX, float targetY);
    float lastEnemyUpdateTime = 0.0f;
    const float enemyUpdateRate = 0.2f;
    bool menuVisible; // Flag to toggle menu visibility without pausing
    bool showHealthBars; // New: Toggle for health bars
    sf::RectangleShape arrowShape;
    void RenderStoreUI();        // New: Draw store UI
    void HandleStorePurchase();
    bool storeVisible;
    sf::Text storeTitle;
    sf::Text speedBoostText;
    sf::RectangleShape speedBoostButton;
    sf::Font font;
};

#endif // GAMEPLAYSTATE_H