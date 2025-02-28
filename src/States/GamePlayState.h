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
    float lastEnemyUpdateTime = 0.0f;
    const float enemyUpdateRate = 0.2f;
    bool menuVisible; // Flag to toggle menu visibility without pausing
};

#endif // GAMEPLAYSTATE_H