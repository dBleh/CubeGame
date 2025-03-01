#include "GameplayState.h"
#include <cmath>
#include <random>

GameplayState::GameplayState(CubeGame* game) : State(game), menuVisible(false), showHealthBars(false), storeVisible(false) {
    if (game->GetLocalPlayer().steamID == SteamMatchmaking()->GetLobbyOwner(game->GetLobbyID())) {
        SpawnEnemies();
    }
    std::cout << "[DEBUG] GameplayState initialized for " << (game->GetLocalPlayer().steamID == SteamMatchmaking()->GetLobbyOwner(game->GetLobbyID()) ? "host" : "client") << std::endl;

    arrowShape.setSize(sf::Vector2f(20.f, 10.f));
    arrowShape.setFillColor(sf::Color::White);
    arrowShape.setOrigin(10.f, 5.f);

    if (!font.loadFromFile("Roboto-Regular.ttf")) {
        std::cerr << "[ERROR] Failed to load font for store UI" << std::endl;
    }
    storeTitle.setFont(font);
    storeTitle.setString("Store (5s)");
    storeTitle.setCharacterSize(24);
    storeTitle.setFillColor(sf::Color::White);

    speedBoostText.setFont(font);
    speedBoostText.setString("Speed Boost (+50) - 50");
    speedBoostText.setCharacterSize(20);
    speedBoostText.setFillColor(sf::Color::White);

    speedBoostButton.setSize(sf::Vector2f(160.f, 40.f));
    speedBoostButton.setFillColor(sf::Color(50, 150, 50));

    game->SetShoppingTimer(0.f);
}

void GameplayState::SendPlayerUpdate() {
    static sf::Clock playerUpdateClock;
    const float playerUpdateRate = 0.016f;

    if (playerUpdateClock.getElapsedTime().asSeconds() < playerUpdateRate) return;
    playerUpdateClock.restart();

    static float lastSentX = -1.0f, lastSentY = -1.0f;
    Player& p = game->GetLocalPlayer();

    if (std::abs(p.x - lastSentX) < 1.5f && std::abs(p.y - lastSentY) < 1.5f && p.speed == game->GetPlayers()[p.steamID].speed) return;

    lastSentX = p.x;
    lastSentY = p.y;
    game->SendPlayerUpdate();
}

void GameplayState::Update(float dt) {
    ShowInGameMenu();

    if (game->GetCurrentState() == GameState::Playing) {
        bool playerMoved = game->GetLocalPlayer().move(dt);
        if (playerMoved) {
            SendPlayerUpdate();
        }

        if (sf::Mouse::isButtonPressed(sf::Mouse::Left)) {
            ShootBullet();
        }

        if (game->GetLocalPlayer().steamID != SteamMatchmaking()->GetLobbyOwner(game->GetLobbyID())) {
            for (auto it = game->GetBullets().begin(); it != game->GetBullets().end();) {
                it->second.update(dt);
                if (it->second.lifetime <= 0) {
                    it = game->GetBullets().erase(it);
                } else {
                    ++it;
                }
            }
            for (auto& [id, enemy] : game->GetEnemies()) {
                float interpFactor = std::min(20.0f * dt, 1.0f);
                enemy.renderedX += (enemy.x - enemy.renderedX) * interpFactor;
                enemy.renderedY += (enemy.y - enemy.renderedY) * interpFactor;
                enemy.shape.setPosition(enemy.renderedX, enemy.renderedY);
            }
            for (auto& [id, player] : game->GetPlayers()) {
                if (id != game->GetLocalPlayer().steamID) {
                    float interpFactor = std::min(20.0f * dt, 1.0f);
                    player.renderedX += (player.x - player.renderedX) * interpFactor;
                    player.renderedY += (player.y - player.renderedY) * interpFactor;
                    player.shape.setPosition(player.renderedX, player.renderedY);
                }
            }
        } else {
            MoveEnemies(dt);
            UpdateBullets(dt);
            CheckCollisions();
            if (game->GetEnemies().empty()) NextLevel();
            if (AllPlayersDead()) {
                game->GetCurrentState() = GameState::GameOver;
            }
        }
        Player& p = game->GetPlayers()[game->GetLocalPlayer().steamID];
        p.x = game->GetLocalPlayer().x;
        p.y = game->GetLocalPlayer().y;
        p.renderedX = game->GetLocalPlayer().renderedX;
        p.renderedY = game->GetLocalPlayer().renderedY;
        p.isAlive = game->GetLocalPlayer().isAlive;
        p.speed = game->GetLocalPlayer().speed; // Sync speed
        p.money = game->GetLocalPlayer().money; // Sync money
        UpdateCamera(dt);
    } else if (game->GetCurrentState() == GameState::Shopping) {
        game->SetShoppingTimer(game->GetShoppingTimer() - dt);
        storeVisible = true;
        if (game->GetShoppingTimer() <= 0) {
            game->GetCurrentState() = GameState::Playing;
            storeVisible = false;
            if (game->GetLocalPlayer().steamID == SteamMatchmaking()->GetLobbyOwner(game->GetLobbyID())) {
                SpawnEnemies();
            }
        }
        storeTitle.setString("Store (" + std::to_string(static_cast<int>(game->GetShoppingTimer() + 0.5f)) + "s)");
    }

    if (game->GetShootCooldown() > 0) game->GetShootCooldown() -= dt;

    game->GetHUD().updateText("level", "Level: " + std::to_string(game->GetCurrentLevel()) +
                             "\nEnemies: " + std::to_string(game->GetEnemies().size()) +
                             "\nHP: " + std::to_string(game->GetPlayers()[game->GetLocalPlayer().steamID].health) +
                             "\nMoney: " + std::to_string(game->GetPlayers()[game->GetLocalPlayer().steamID].money));
    game->GetHUD().updateText("gameStatus", game->GetCurrentState() == GameState::Shopping ? "Shopping" : "Playing");
    UpdateScoreboard();
}

void GameplayState::Render() {
    game->GetWindow().clear(sf::Color::Black);

    sf::RectangleShape gridLine;
    gridLine.setFillColor(sf::Color(50, 50, 50, 100));
    const float gridSize = 50.f;

    for (float x = 0; x < 2000; x += gridSize) {
        gridLine.setSize(sf::Vector2f(1, 2000));
        gridLine.setPosition(x, 0);
        game->GetWindow().draw(gridLine);
    }
    for (float y = 0; y < 2000; y += gridSize) {
        gridLine.setSize(sf::Vector2f(2000, 1));
        gridLine.setPosition(0, y);
        game->GetWindow().draw(gridLine);
    }

    for (auto& player : game->GetPlayers()) {
        if (player.second.isAlive && !std::isnan(player.second.renderedX) && !std::isnan(player.second.renderedY)) {
            player.second.shape.setPosition(player.second.renderedX, player.second.renderedY);
            game->GetWindow().draw(player.second.shape);
        }
    }

    if (game->GetCurrentState() == GameState::Playing || game->GetCurrentState() == GameState::Shopping) {
        sf::View view = game->GetView();
        sf::Vector2f viewCenter = view.getCenter();
        sf::Vector2f viewSize = view.getSize();
        float left = viewCenter.x - viewSize.x / 2;
        float right = viewCenter.x + viewSize.x / 2;
        float top = viewCenter.y - viewSize.y / 2;
        float bottom = viewCenter.y + viewSize.y / 2;

        sf::Vector2f playerScreenPos = static_cast<sf::Vector2f>(game->GetWindow().mapCoordsToPixel(
            sf::Vector2f(game->GetLocalPlayer().renderedX, game->GetLocalPlayer().renderedY), view));
        float arrowRadius = std::min(viewSize.x, viewSize.y) * 0.25f;

        for (auto& [id, enemy] : game->GetEnemies()) {
            if (enemy.health > 0 && !std::isnan(enemy.renderedX) && !std::isnan(enemy.renderedY)) {
                enemy.shape.setPosition(enemy.renderedX, enemy.renderedY);
                game->GetWindow().draw(enemy.shape);

                if (showHealthBars) {
                    float maxHealth = (enemy.type == Enemy::Brute) ? 100.f : 
                                      (enemy.type == Enemy::GravityWell) ? 80.f : 
                                      (enemy.type == Enemy::Sniper) ? 40.f : 
                                      (enemy.type == Enemy::Bomber) ? 30.f : 
                                      (enemy.type == Enemy::Swarmlet) ? 10.f : 50.f;
                    float healthRatio = enemy.health / maxHealth;
                    sf::RectangleShape healthBar;
                    healthBar.setSize(sf::Vector2f(enemy.shape.getSize().x * healthRatio, 5.f));
                    healthBar.setFillColor(sf::Color(0, 255, 0));
                    if (healthRatio < 0.3f) healthBar.setFillColor(sf::Color::Red);
                    else if (healthRatio < 0.6f) healthBar.setFillColor(sf::Color::Yellow);
                    healthBar.setPosition(enemy.renderedX, enemy.renderedY + enemy.shape.getSize().y + 2.f);
                    game->GetWindow().draw(healthBar);
                }

                if (enemy.renderedX < left || enemy.renderedX > right || enemy.renderedY < top || enemy.renderedY > bottom) {
                    float dx = enemy.renderedX - game->GetLocalPlayer().renderedX;
                    float dy = enemy.renderedY - game->GetLocalPlayer().renderedY;
                    float angle = std::atan2(dy, dx) * 180.f / M_PI;

                    sf::Vector2f arrowPos(
                        playerScreenPos.x + arrowRadius * std::cos(angle * M_PI / 180.f),
                        playerScreenPos.y + arrowRadius * std::sin(angle * M_PI / 180.f)
                    );
                    arrowPos.x = std::max(10.f, std::min(viewSize.x - 10.f, arrowPos.x));
                    arrowPos.y = std::max(10.f, std::min(viewSize.y - 10.f, arrowPos.y));

                    sf::Vector2i arrowPixelPos(static_cast<int>(arrowPos.x), static_cast<int>(arrowPos.y));
                    arrowShape.setPosition(game->GetWindow().mapPixelToCoords(arrowPixelPos, view));
                    arrowShape.setRotation(angle);
                    game->GetWindow().draw(arrowShape);
                }
            }
        }

        for (auto& [id, bullet] : game->GetBullets()) {
            if (!std::isnan(bullet.renderedX) && !std::isnan(bullet.renderedY)) {
                bullet.shape.setPosition(bullet.renderedX, bullet.renderedY);
                game->GetWindow().draw(bullet.shape);
            }
        }

        if (storeVisible) {
            sf::Vector2f viewCenter = game->GetView().getCenter();
            storeTitle.setPosition(viewCenter.x - 50, viewCenter.y - 100);
            speedBoostText.setPosition(viewCenter.x - 80, viewCenter.y - 50);
            speedBoostButton.setPosition(viewCenter.x - 80, viewCenter.y - 20);
            RenderStoreUI();
        }
    }

    game->RenderHUDLayer();
    game->GetWindow().display();
}

void GameplayState::ProcessEvent(const sf::Event& event) {
    ProcessEvents(event);
}

void GameplayState::ProcessEvents(const sf::Event& event) {
    if (event.type == sf::Event::KeyPressed) {
        if (game->GetCurrentState() == GameState::Playing || game->GetCurrentState() == GameState::Shopping) {
            if (event.key.code == sf::Keyboard::Escape) {
                menuVisible = !menuVisible;
            } else if (event.key.code == sf::Keyboard::M) {
                if (game->GetLocalPlayer().steamID == SteamMatchmaking()->GetLobbyOwner(game->GetLobbyID())) {
                    int memberCount = SteamMatchmaking()->GetNumLobbyMembers(game->GetLobbyID());
                    if (memberCount > 1) {
                        for (int i = 0; i < memberCount; ++i) {
                            CSteamID newHost = SteamMatchmaking()->GetLobbyMemberByIndex(game->GetLobbyID(), i);
                            if (newHost != game->GetLocalPlayer().steamID) {
                                SteamMatchmaking()->SetLobbyOwner(game->GetLobbyID(), newHost);
                                std::cout << "[DEBUG] Transferred host to: " << newHost.ConvertToUint64() << std::endl;
                                break;
                            }
                        }
                    }
                }
                game->ReturnToMainMenu();
            } else if (event.key.code == sf::Keyboard::H) {
                showHealthBars = !showHealthBars;
                std::cout << "[DEBUG] Health bars " << (showHealthBars ? "enabled" : "disabled") << std::endl;
            }
        }
    }
    if (event.type == sf::Event::MouseButtonPressed && event.mouseButton.button == sf::Mouse::Left) {
        if (game->GetCurrentState() == GameState::Shopping) {
            HandleStorePurchase();
        } else if (game->GetCurrentState() == GameState::Playing) {
            ShootBullet();
        }
    }
}

void GameplayState::ShootBullet() {
    if (!game->GetLocalPlayer().isAlive || game->GetShootCooldown() > 0) return;

    float startX = game->GetLocalPlayer().x + 10;
    float startY = game->GetLocalPlayer().y + 10;
    sf::Vector2i mousePos = sf::Mouse::getPosition(game->GetWindow());
    sf::Vector2f worldPos = game->GetWindow().mapPixelToCoords(mousePos, game->GetView());

    Bullet b;
    b.initialize(startX, startY, worldPos.x, worldPos.y);
    b.lifetime = 2.0f;
    b.renderedX = startX;
    b.renderedY = startY;
    int bulletIdx = game->GetNextBulletId();
    b.id = static_cast<int>((game->GetLocalPlayer().steamID.ConvertToUint64() & 0xFFFF) << 16 | (bulletIdx & 0xFFFF));
    game->GetNextBulletId()++;

    game->GetBullets()[b.id] = b;
    game->GetShootCooldown() = 0.2f;

    char buffer[128];
    int shooterID = (b.id >> 16) & 0xFFFF;
    int bytes = snprintf(buffer, sizeof(buffer), "B|fire|%d|%d|%.1f|%.1f|%.1f|%.1f",
                         shooterID, bulletIdx, b.x, b.y, worldPos.x, worldPos.y);
    if (bytes > 0 && static_cast<size_t>(bytes) < sizeof(buffer)) {
        for (const auto& [steamID, conn] : game->GetConnections()) {
            game->GetNetworkingSockets()->SendMessageToConnection(conn, buffer, bytes + 1, k_nSteamNetworkingSend_Reliable, nullptr);
        }
    }
}

void GameplayState::CheckCollisions() {
    if (game->GetLocalPlayer().steamID != SteamMatchmaking()->GetLobbyOwner(game->GetLobbyID())) return;

    auto& bullets = game->GetBullets();
    auto& enemies = game->GetEnemies();

    for (auto bulletIt = bullets.begin(); bulletIt != bullets.end();) {
        bool bulletHit = false;
        for (auto enemyIt = enemies.begin(); enemyIt != enemies.end();) {
            if (bulletIt->second.shape.getGlobalBounds().intersects(enemyIt->second.shape.getGlobalBounds())) {
                enemyIt->second.health -= 25;
                char hitBuffer[64];
                int hitBytes = snprintf(hitBuffer, sizeof(hitBuffer), "H|%d|%llu",
                                        bulletIt->second.id, enemyIt->first);
                if (hitBytes > 0 && static_cast<size_t>(hitBytes) < sizeof(hitBuffer)) {
                    for (const auto& [steamID, conn] : game->GetConnections()) {
                        game->GetNetworkingSockets()->SendMessageToConnection(conn, hitBuffer, hitBytes + 1, k_nSteamNetworkingSend_Reliable, nullptr);
                    }
                }

                if (enemyIt->second.health <= 0) {
                    if (enemyIt->second.type == Enemy::Bomber && !enemyIt->second.exploded) {
                        enemyIt->second.exploded = true;
                        for (auto& [otherId, otherEnemy] : enemies) {
                            if (otherId != enemyIt->first) {
                                float dx = otherEnemy.x - enemyIt->second.x;
                                float dy = otherEnemy.y - enemyIt->second.y;
                                float dist = std::sqrt(dx * dx + dy * dy);
                                if (dist < 50.0f) {
                                    otherEnemy.health -= 30;
                                }
                            }
                        }
                    }

                    uint64_t shooterID = (bulletIt->second.id >> 16) & 0xFFFF;
                    CSteamID shooter(shooterID);
                    for (auto& [playerID, player] : game->GetPlayers()) {
                        if ((playerID.ConvertToUint64() & 0xFFFF) == shooterID) {
                            int moneyReward = (enemyIt->second.type == Enemy::Swarmlet) ? 5 :
                                              (enemyIt->second.type == Enemy::Sniper) ? 20 :
                                              (enemyIt->second.type == Enemy::Bomber) ? 15 :
                                              (enemyIt->second.type == Enemy::Brute) ? 25 :
                                              (enemyIt->second.type == Enemy::GravityWell) ? 30 : 10;
                            player.money += moneyReward;
                            if (playerID == game->GetLocalPlayer().steamID) {
                                game->GetLocalPlayer().money = player.money; // Sync local player
                            }

                            char killBuffer[64];
                            int killBytes = snprintf(killBuffer, sizeof(killBuffer), "K|%llu",
                                                     playerID.ConvertToUint64());
                            if (killBytes > 0 && static_cast<size_t>(killBytes) < sizeof(killBuffer)) {
                                auto it = game->GetConnections().find(playerID);
                                if (it != game->GetConnections().end()) {
                                    game->GetNetworkingSockets()->SendMessageToConnection(it->second, killBuffer, killBytes + 1, k_nSteamNetworkingSend_Reliable, nullptr);
                                }
                            }
                            break;
                        }
                    }
                    enemyIt = enemies.erase(enemyIt);
                } else {
                    ++enemyIt;
                }
                bulletHit = true;
                break;
            } else {
                ++enemyIt;
            }
        }
        if (bulletHit) {
            bulletIt = bullets.erase(bulletIt);
        } else {
            ++bulletIt;
        }
    }
}

void GameplayState::UpdateCamera(float dt) {
    if (game->GetPlayers().empty()) return;

    float minX = std::numeric_limits<float>::max();
    float maxX = std::numeric_limits<float>::min();
    float minY = std::numeric_limits<float>::max();
    float maxY = std::numeric_limits<float>::min();
    sf::Vector2f avgPos(0, 0);
    int aliveCount = 0;

    for (const auto& player : game->GetPlayers()) {
        if (player.second.isAlive && !std::isnan(player.second.renderedX) && !std::isnan(player.second.renderedY)) {
            minX = std::min(minX, player.second.renderedX);
            maxX = std::max(maxX, player.second.renderedX);
            minY = std::min(minY, player.second.renderedY);
            maxY = std::max(maxY, player.second.renderedY);
            avgPos.x += player.second.renderedX;
            avgPos.y += player.second.renderedY;
            aliveCount++;
        }
    }

    if (aliveCount == 0) return;

    avgPos.x /= aliveCount;
    avgPos.y /= aliveCount;

    if (std::isnan(avgPos.x) || std::isnan(avgPos.y)) {
        avgPos.x = SCREEN_WIDTH / 2.f;
        avgPos.y = SCREEN_HEIGHT / 2.f;
        std::cerr << "[ERROR] Camera avgPos was NaN, reset to center" << std::endl;
    }

    float width = std::max(maxX - minX + 100, 100.0f);
    float height = std::max(maxY - minY + 100, 100.0f);
    float targetZoom = std::max(width / SCREEN_WIDTH, height / SCREEN_HEIGHT);
    targetZoom = std::max(targetZoom, 1.0f);

    sf::Vector2f currentCenter = game->GetView().getCenter();
    float currentZoom = SCREEN_WIDTH / game->GetView().getSize().x;

    sf::Vector2f targetCenter(avgPos.x, avgPos.y);
    sf::Vector2f newCenter = currentCenter + (targetCenter - currentCenter) * 5.0f * dt;
    float newZoom = currentZoom + (targetZoom - currentZoom) * 5.0f * dt;

    if (std::isnan(newCenter.x) || std::isnan(newCenter.y) || std::isnan(newZoom)) {
        newCenter = sf::Vector2f(SCREEN_WIDTH / 2.f, SCREEN_HEIGHT / 2.f);
        newZoom = 1.0f;
        std::cerr << "[ERROR] Camera values were NaN, reset to default" << std::endl;
    }

    game->GetView().setCenter(newCenter);
    game->GetView().setSize(SCREEN_WIDTH * newZoom, SCREEN_HEIGHT * newZoom);
    game->GetWindow().setView(game->GetView());
}

void GameplayState::MoveEnemies(float dt) {
    if (game->GetLocalPlayer().steamID != SteamMatchmaking()->GetLobbyOwner(game->GetLobbyID())) return;

    uint64_t hostID = game->GetLocalPlayer().steamID.ConvertToUint64();
    int enemyIndex = 0;
    for (auto& [id, enemy] : game->GetEnemies()) {
        float targetX = SCREEN_WIDTH / 2.f;
        float targetY = SCREEN_HEIGHT / 2.f;
        float closestDist = std::numeric_limits<float>::max();

        for (const auto& [id, player] : game->GetPlayers()) {
            if (player.isAlive) {
                float dx = player.x - enemy.x;
                float dy = player.y - enemy.y;
                float dist = std::sqrt(dx * dx + dy * dy);
                if (dist < closestDist) {
                    closestDist = dist;
                    targetX = player.x;
                    targetY = player.y;
                }
            }
        }

        float oldX = enemy.x;
        float oldY = enemy.y;
        enemy.move(dt, targetX, targetY);
        enemy.update(dt);
        enemy.id = (hostID & 0xFFFF) << 16 | (enemyIndex++ & 0xFFFF);

        if (enemy.type == Enemy::Sniper && enemy.attackCooldown <= 0) {
            SpawnSniperBullet(enemy.id, targetX, targetY);
            enemy.attackCooldown = 3.0f;
        }

        if (enemy.type == Enemy::GravityWell) {
            for (auto& [pid, player] : game->GetPlayers()) {
                if (player.isAlive) {
                    float dx = enemy.x - player.x;
                    float dy = enemy.y - player.y;
                    float dist = std::sqrt(dx * dx + dy * dy);
                    if (dist < enemy.pullRadius && dist > 0) {
                        float pullSpeed = 50.0f * dt;
                        player.x += (dx / dist) * pullSpeed;
                        player.y += (dy / dist) * pullSpeed;
                    }
                }
            }
        }
    }

    static sf::Clock updateClock;
    if (updateClock.getElapsedTime().asSeconds() >= 0.1f) {
        SendEnemyUpdate();
        updateClock.restart();
    }
}

void GameplayState::SendEnemyUpdate() {
    if (game->GetLocalPlayer().steamID != SteamMatchmaking()->GetLobbyOwner(game->GetLobbyID())) return;

    for (const auto& [id, enemy] : game->GetEnemies()) {
        char buffer[128];
        int bytes = snprintf(buffer, sizeof(buffer), "E|%llu|%.1f|%.1f|%d|%.2f",
                             id, enemy.x, enemy.y, enemy.health, enemy.spawnDelay);
        if (bytes > 0 && static_cast<size_t>(bytes) < sizeof(buffer)) {
            for (const auto& [steamID, conn] : game->GetConnections()) {
                game->GetNetworkingSockets()->SendMessageToConnection(conn, buffer, bytes + 1, k_nSteamNetworkingSend_Reliable, nullptr);
            }
        }
    }
}

void GameplayState::UpdateBullets(float dt) {
    auto& bullets = game->GetBullets();
    for (auto it = bullets.begin(); it != bullets.end();) {
        it->second.update(dt);
        if (it->second.lifetime <= 0.0f) {
            it = bullets.erase(it);
        } else {
            ++it;
        }
    }
}

void GameplayState::SpawnSniperBullet(uint64_t enemyId, float targetX, float targetY) {
    Bullet b;
    float startX = game->GetEnemies()[enemyId].x;
    float startY = game->GetEnemies()[enemyId].y;
    b.initialize(startX, startY, targetX, targetY);
    b.lifetime = 2.0f;
    b.renderedX = startX;
    b.renderedY = startY;
    b.shape.setFillColor(sf::Color::Magenta);
    int bulletIdx = game->GetNextBulletId();
    b.id = static_cast<int>((enemyId & 0xFFFF) << 16 | (bulletIdx & 0xFFFF));
    game->GetNextBulletId()++;
    game->GetBullets()[b.id] = b;

    char buffer[128];
    int shooterID = (b.id >> 16) & 0xFFFF;
    int bytes = snprintf(buffer, sizeof(buffer), "B|fire|%d|%d|%.1f|%.1f|%.1f|%.1f",
                         shooterID, bulletIdx, b.x, b.y, targetX, targetY);
    if (bytes > 0 && static_cast<size_t>(bytes) < sizeof(buffer)) {
        for (const auto& [steamID, conn] : game->GetConnections()) {
            game->GetNetworkingSockets()->SendMessageToConnection(conn, buffer, bytes + 1, k_nSteamNetworkingSend_Reliable, nullptr);
        }
    }
}

void GameplayState::SpawnEnemies() {
    game->GetEnemies().clear();
    uint64_t hostID = game->GetLocalPlayer().steamID.ConvertToUint64();
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<> angleDist(0, 2 * M_PI);
    std::uniform_real_distribution<> distDist(200, 300);

    sf::Vector2f avgPos(0, 0);
    int aliveCount = 0;
    for (const auto& player : game->GetPlayers()) {
        if (player.second.isAlive) {
            avgPos.x += player.second.x;
            avgPos.y += player.second.y;
            aliveCount++;
        }
    }
    if (aliveCount > 0) {
        avgPos.x /= aliveCount;
        avgPos.y /= aliveCount;
    } else {
        avgPos = {SCREEN_WIDTH / 2.f, SCREEN_HEIGHT / 2.f};
    }

    const float SPAWN_RADIUS = 100.0f;
    for (int i = 0; i < game->GetEnemiesPerWave(); i++) {
        Enemy e;
        int typeRoll = rand() % 6;
        Enemy::Type enemyType = (typeRoll == 0 && game->GetCurrentLevel() >= 3) ? Enemy::Swarmlet :
                                (typeRoll == 1 && game->GetCurrentLevel() >= 5) ? Enemy::Sniper :
                                (typeRoll == 2 && game->GetCurrentLevel() >= 4) ? Enemy::Bomber :
                                (typeRoll == 3 && game->GetCurrentLevel() >= 2) ? Enemy::Brute :
                                Enemy::Default;
        e.initialize(enemyType);

        bool validPosition = false;
        int attempts = 0;
        const int MAX_ATTEMPTS = 20;

        while (!validPosition && attempts < MAX_ATTEMPTS) {
            float angle = angleDist(gen);
            float distance = distDist(gen);
            e.x = avgPos.x + distance * std::cos(angle);
            e.y = avgPos.y + distance * std::sin(angle);

            validPosition = true;
            for (const auto& player : game->GetPlayers()) {
                if (player.second.isAlive) {
                    float dx = e.x - player.second.x;
                    float dy = e.y - player.second.y;
                    float dist = std::sqrt(dx * dx + dy * dy);
                    if (dist < SPAWN_RADIUS) {
                        validPosition = false;
                        break;
                    }
                }
            }
            attempts++;
        }

        if (!validPosition) {
            e.x = avgPos.x + SPAWN_RADIUS * 1.5f;
            e.y = avgPos.y + SPAWN_RADIUS * 1.5f;
        }

        if (std::isnan(e.x) || std::isnan(e.y) || e.x < 0 || e.x > 2000 || e.y < 0 || e.y > 2000) {
            e.x = avgPos.x + SPAWN_RADIUS;
            e.y = avgPos.y + SPAWN_RADIUS;
        }

        e.renderedX = e.x;
        e.renderedY = e.y;
        e.shape.setPosition(e.x, e.y);
        e.id = (hostID & 0xFFFF) << 16 | (i & 0xFFFF);

        if (enemyType == Enemy::Swarmlet) {
            game->GetEnemies()[e.id] = e;
            for (int j = 1; j < 5; j++) {
                Enemy swarm;
                swarm.initialize(Enemy::Swarmlet);
                swarm.x = e.x + (rand() % 20 - 10);
                swarm.y = e.y + (rand() % 20 - 10);
                swarm.renderedX = swarm.x;
                swarm.renderedY = swarm.y;
                swarm.shape.setPosition(swarm.x, swarm.y);
                swarm.id = (hostID & 0xFFFF) << 16 | ((i + j) & 0xFFFF);
                game->GetEnemies()[swarm.id] = swarm;
            }
            i += 4;
        } else {
            game->GetEnemies()[e.id] = e;
        }
    }
}

void GameplayState::NextLevel() {
    game->GetCurrentLevel() += 1;
    game->GetEnemiesPerWave() += 2;
    game->GetCurrentState() = GameState::Shopping;
    game->SetShoppingTimer(5.0f);
}

void GameplayState::RenderStoreUI() {
    sf::Text moneyText;
    moneyText.setFont(font);
    moneyText.setString("Money: " + std::to_string(game->GetLocalPlayer().money));
    moneyText.setCharacterSize(20);
    moneyText.setFillColor(sf::Color::Yellow);
    sf::Vector2f viewCenter = game->GetView().getCenter();
    moneyText.setPosition(viewCenter.x - 80, viewCenter.y - 70);

    game->GetWindow().draw(storeTitle);
    game->GetWindow().draw(moneyText);
    game->GetWindow().draw(speedBoostText);
    game->GetWindow().draw(speedBoostButton);
}

void GameplayState::HandleStorePurchase() {
    if (!storeVisible) return;

    sf::Vector2i mousePos = sf::Mouse::getPosition(game->GetWindow());
    sf::Vector2f worldMousePos = game->GetWindow().mapPixelToCoords(mousePos, game->GetView());

    if (speedBoostButton.getGlobalBounds().contains(worldMousePos)) {
        Player& player = game->GetLocalPlayer();
        const int SPEED_BOOST_COST = 50;
        const float SPEED_BOOST_AMOUNT = 50.0f;

        speedBoostButton.setFillColor(sf::Color(200, 200, 255)); // Highlight on hover

        static sf::Clock clickCooldown;
        if (clickCooldown.getElapsedTime().asSeconds() > 0.5f) { // Debounce clicks
            if (player.money >= SPEED_BOOST_COST) {
                player.money -= SPEED_BOOST_COST;
                player.applySpeedBoost(SPEED_BOOST_AMOUNT);
                game->GetPlayers()[player.steamID].money = player.money; // Sync with players map
                game->GetPlayers()[player.steamID].speed = player.speed; // Sync speed
                SendPlayerUpdate(); // Notify all clients
                std::cout << "[DEBUG] Speed Boost Purchased. New Speed: " << player.speed 
                          << " | Remaining Money: " << player.money << std::endl;
                clickCooldown.restart();
            } else {
                std::cout << "[DEBUG] Not enough money for Speed Boost (Player Money: " 
                          << player.money << ")" << std::endl;
            }
        }
    } else {
        speedBoostButton.setFillColor(sf::Color(50, 150, 50)); // Default color
    }
}

void GameplayState::UpdateScoreboard() {
    std::string scoreboard = "Scoreboard:\n";
    for (const auto& player : game->GetPlayers()) {
        scoreboard += "Player " + std::to_string(player.second.steamID.ConvertToUint64() % 10000) +
                     ": Kills=" + std::to_string(player.second.kills) +
                     " HP=" + std::to_string(player.second.health) +
                     (player.second.isAlive ? " (Alive)" : " (Dead)") + "\n";
    }
    game->GetHUD().updateText("scoreboard", scoreboard);
}

void GameplayState::ShowInGameMenu() {
    if (menuVisible) {
        game->GetHUD().updateText("menu", "Press M to return to Main Menu");
    } else {
        game->GetHUD().updateText("menu", "");
    }
}

bool GameplayState::AllPlayersDead() {
    return std::all_of(game->GetPlayers().begin(), game->GetPlayers().end(),
        [](const auto& p) { return !p.second.isAlive; });
}