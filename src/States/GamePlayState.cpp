#include "GameplayState.h"
#include <cmath>
#include <random>

GameplayState::GameplayState(CubeGame* game) : State(game), menuVisible(false) {
    if (game->GetLocalPlayer().steamID == SteamMatchmaking()->GetLobbyOwner(game->GetLobbyID())) {
        SpawnEnemies();
    }
    std::cout << "[DEBUG] GameplayState initialized for " << (game->GetLocalPlayer().steamID == SteamMatchmaking()->GetLobbyOwner(game->GetLobbyID()) ? "host" : "client") << std::endl;
}

void GameplayState::SendPlayerUpdate() {
    static sf::Clock playerUpdateClock;
    const float playerUpdateRate = 0.016f;

    if (playerUpdateClock.getElapsedTime().asSeconds() < playerUpdateRate) return;
    playerUpdateClock.restart();

    static float lastSentX = -1.0f, lastSentY = -1.0f;
    Player& p = game->GetLocalPlayer();

    if (std::abs(p.x - lastSentX) < 1.5f && std::abs(p.y - lastSentY) < 1.5f) {
        return;
    }

    lastSentX = p.x;
    lastSentY = p.y;

    char buffer[128];
    int bytes = snprintf(buffer, sizeof(buffer), "P|%llu|%.1f|%.1f|%d|%d|%d", 
                         p.steamID.ConvertToUint64(), p.x, p.y, p.health, p.kills, p.ready ? 1 : 0);
    if (bytes > 0 && static_cast<size_t>(bytes) < sizeof(buffer)) {
        SteamMatchmaking()->SendLobbyChatMsg(game->GetLobbyID(), buffer, bytes + 1);
    }
}

void GameplayState::Update(float dt) {
    ShowInGameMenu();

    if (game->GetCurrentState() == GameState::Playing) {
        bool playerMoved = game->GetLocalPlayer().move(dt);
        if (playerMoved) {
            SendPlayerUpdate();
        }

        if (game->GetLocalPlayer().steamID != SteamMatchmaking()->GetLobbyOwner(game->GetLobbyID())) {
            for (auto& [id, bullet] : game->GetBullets()) {
                bullet.update(dt);
            }
            for (auto& [id, enemy] : game->GetEnemies()) {
                enemy.update(dt);
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
        game->GetPlayers()[game->GetLocalPlayer().steamID] = game->GetLocalPlayer();
        UpdateCamera(dt);
    }

    if (game->GetShootCooldown() > 0) game->GetShootCooldown() -= dt;

    game->GetHUD().updateText("level", "Level: " + std::to_string(game->GetCurrentLevel()) + 
                             "\nEnemies: " + std::to_string(game->GetEnemies().size()) + 
                             "\nHP: " + std::to_string(game->GetPlayers()[game->GetLocalPlayer().steamID].health));
    game->GetHUD().updateText("gameStatus", "Playing");
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

    if (game->GetCurrentState() == GameState::Playing) {
        for (auto& [id, enemy] : game->GetEnemies()) {
            if (enemy.health > 0 && !std::isnan(enemy.renderedX) && !std::isnan(enemy.renderedY)) {
                enemy.shape.setPosition(enemy.renderedX, enemy.renderedY);
                game->GetWindow().draw(enemy.shape);
            }
        }

        for (auto& [id, bullet] : game->GetBullets()) {
            if (!std::isnan(bullet.renderedX) && !std::isnan(bullet.renderedY)) {
                bullet.shape.setPosition(bullet.renderedX, bullet.renderedY);
                game->GetWindow().draw(bullet.shape);
            }
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
        if (game->GetCurrentState() == GameState::Playing) {
            if (event.key.code == sf::Keyboard::Escape) {
                menuVisible = !menuVisible;
            }
            else if (event.key.code == sf::Keyboard::M) {
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
            }
        }
    }
    if (game->GetCurrentState() == GameState::Playing && sf::Mouse::isButtonPressed(sf::Mouse::Left)) {
        ShootBullet();
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

    if (aliveCount == 0) {
        std::cout << "[DEBUG] No alive players for camera update" << std::endl;
        return;
    }

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
    if (game->GetLocalPlayer().steamID != SteamMatchmaking()->GetLobbyOwner(game->GetLobbyID())) {
        return;
    }

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

        enemy.move(dt, targetX, targetY); // Use move() to respect spawnDelay
        enemy.update(dt);
        enemy.id = (hostID & 0xFFFF) << 16 | (enemyIndex++ & 0xFFFF);
    }

    static sf::Clock updateClock;
    if (updateClock.getElapsedTime().asSeconds() >= enemyUpdateRate) {
        SendEnemyUpdate();
        updateClock.restart();
    }
}

void GameplayState::SendEnemyUpdate() {
    if (game->GetLocalPlayer().steamID != SteamMatchmaking()->GetLobbyOwner(game->GetLobbyID())) return;

    char buffer[1024];
    std::string enemyData = "E|";

    for (const auto& [id, enemy] : game->GetEnemies()) {
        enemyData += std::to_string(id) + "," +
                     std::to_string((int)enemy.x) + "," +
                     std::to_string((int)enemy.y) + "," +
                     std::to_string((int)enemy.velocityX) + "," +
                     std::to_string((int)enemy.velocityY) + ";";
    }

    int bytes = snprintf(buffer, sizeof(buffer), "%s", enemyData.c_str());
    if (bytes > 0 && static_cast<size_t>(bytes) < sizeof(buffer)) {
        SteamMatchmaking()->SendLobbyChatMsg(game->GetLobbyID(), buffer, bytes + 1);
    } else {
        std::cout << "[ERROR] Enemy update message too large or failed: " << bytes << std::endl;
    }
}

void GameplayState::ShootBullet() {
    if (!game->GetLocalPlayer().isAlive || game->GetShootCooldown() > 0) return;

    Bullet b;
    sf::Vector2i mousePos = sf::Mouse::getPosition(game->GetWindow());
    sf::Vector2f worldPos = game->GetWindow().mapPixelToCoords(mousePos, game->GetView());

    float startX = game->GetLocalPlayer().x + 10;
    float startY = game->GetLocalPlayer().y + 10;
    b.initialize(startX, startY, worldPos.x, worldPos.y);
    b.lifetime = 2.0f;
    b.renderedX = startX;
    b.renderedY = startY;
    b.shape.setPosition(b.renderedX, b.renderedY);
    int bulletIdx = game->GetNextBulletId();
    b.id = static_cast<int>((game->GetLocalPlayer().steamID.ConvertToUint64() & 0xFFFF) << 16 | (bulletIdx & 0xFFFF));
    game->GetNextBulletId()++;

    std::cout << "Bullet fired at (" << startX << ", " << startY << ")" << std::endl;

    game->GetBullets()[b.id] = b;
    SendBulletData(b);
    game->GetShootCooldown() = 0.1f;
}

void GameplayState::SendBulletData(const Bullet& b) {
    char buffer[128];
    int shooterID = (b.id >> 16) & 0xFFFF;
    int bulletIdx = b.id & 0xFFFF;
    int bytes = snprintf(buffer, sizeof(buffer), "B|%d|%d|%f|%f|%f|%f", shooterID, bulletIdx, b.x, b.y, b.velocityX, b.velocityY);
    if (bytes > 0 && static_cast<size_t>(bytes) < sizeof(buffer)) {
        SteamMatchmaking()->SendLobbyChatMsg(game->GetLobbyID(), buffer, bytes + 1);
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

void GameplayState::CheckCollisions() {
    auto& bullets = game->GetBullets();
    auto& enemies = game->GetEnemies();

    for (auto bulletIt = bullets.begin(); bulletIt != bullets.end();) {
        bool bulletHit = false;
        for (auto enemyIt = enemies.begin(); enemyIt != enemies.end();) {
            if (bulletIt->second.shape.getGlobalBounds().intersects(enemyIt->second.shape.getGlobalBounds())) {
                enemyIt->second.health -= 25;
                if (enemyIt->second.health <= 0) {
                    if (game->GetLocalPlayer().steamID == SteamMatchmaking()->GetLobbyOwner(game->GetLobbyID())) {
                        game->GetLocalPlayer().kills++;
                        SendPlayerUpdate();
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

void GameplayState::SpawnEnemies() {
    game->GetEnemies().clear();
    uint64_t hostID = game->GetLocalPlayer().steamID.ConvertToUint64();
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<> angleDist(0, 2 * M_PI);
    std::uniform_real_distribution<> distDist(200, 300); // Increased min to 200

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
        e.initialize();
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
            std::cout << "[DEBUG] Fallback spawn for enemy " << i << " at (" << e.x << ", " << e.y << ")" << std::endl;
        }

        e.shape.setPosition(e.x, e.y);
        e.health = 50;
        e.id = (hostID & 0xFFFF) << 16 | (i & 0xFFFF);
        game->GetEnemies()[e.id] = e;
        std::cout << "[DEBUG] Spawned enemy " << e.id << " at (" << e.x << ", " << e.y << ")" << std::endl;
    }
}

void GameplayState::NextLevel() {
    game->GetCurrentLevel() += 1;
    game->GetEnemiesPerWave() += 2;
    SpawnEnemies();
    std::cout << "[DEBUG] Advanced to level " << game->GetCurrentLevel() << std::endl;
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