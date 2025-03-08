#include "GameplayState.h"
#include "../Hud/HUD.h"
#include <cmath>
#include <random>
#include <iostream>

//---------------------------------------------------------
// Constructor & HUD Initialization
//---------------------------------------------------------
GameplayState::GameplayState(CubeGame* game)
    : State(game),
      shopOpen(false),
      nextLevelTimer(0.0f),
      lastEnemyUpdateTime(0.0f),
      enemyUpdateRate(0.01f),
      menuVisible(false)
{
    // Host spawns initial enemies and performs a full enemy sync before the game starts.
    if (game->IsHost() && !game->IsGameStarted()) {
        game->GetEntityManager()->spawnEnemies(game->GetEnemiesPerWave(),
                                                 game->GetPlayers(),
                                                 game->GetLocalPlayer().steamID.ConvertToUint64());
        game->GetNetworkManager()->SyncEnemiesFull();
    }
    
    // Configure HUD elements for gameplay and store.
    sf::Vector2u winSize = game->GetWindow().getSize();
    game->GetHUD().configureGameplayHUD(winSize);
    game->GetHUD().configureStoreHUD(winSize);
}

//---------------------------------------------------------
// Update Function
//---------------------------------------------------------
void GameplayState::Update(float dt) {
    // Host handles network messages and level progression.
    if (game->IsHost()) {
        game->GetNetworkManager()->receiveMessages();
        CheckAndAdvanceLevel();

        // Timer for next level wave.
        if (nextLevelTimer > 0) {
            nextLevelTimer -= dt;
            static float lastTimerSync = 0.0f;
            lastTimerSync += dt;
            if (lastTimerSync >= 0.5f) {
                char buffer[64];
                int bytes = snprintf(buffer, sizeof(buffer), "S|TIMER|%.1f", nextLevelTimer);
                if (bytes > 0 && static_cast<size_t>(bytes) < sizeof(buffer)) {
                    game->GetNetworkManager()->SendGameplayMessage(std::string(buffer));
                }
                lastTimerSync = 0.0f;
            }
            if (nextLevelTimer <= 0) {
                nextLevelTimer = 0;
                timerActive = false;
                if (game->IsHost()) {
                    game->GetNetworkManager()->SpawnEnemiesAndBroadcast();
                    std::cout << "[DEBUG] Host spawned next wave" << std::endl;
                }
            }
        }

        // Check if all players are dead; if so, transition to GameOver.
        bool allDead = std::all_of(game->GetPlayers().begin(), game->GetPlayers().end(),
            [](const auto& pair) { return !pair.second.isAlive; });
        
        if (allDead && game->GetCurrentState() != GameState::GameOver) {
            std::cout << "[DEBUG] All players dead, transitioning to GameOver" << std::endl;
            game->SetCurrentState(GameState::GameOver);
            char gameOverBuffer[32];
            int goBytes = snprintf(gameOverBuffer, sizeof(gameOverBuffer), "S|GAMEOVER");
            if (goBytes > 0 && static_cast<size_t>(goBytes) < sizeof(gameOverBuffer)) {
                game->GetNetworkManager()->broadcastMessage(std::string(gameOverBuffer));
            }
            return;
        }
    }

    // Update HUD elements.
    sf::Vector2u winSize = game->GetWindow().getSize();
    game->GetHUD().refreshHUDContent(game->GetCurrentState(), menuVisible, shopOpen, winSize, game->GetLocalPlayer());

    // Update playing state if not in pause menu.
    if (game->GetCurrentState() == GameState::Playing) {
        if (!menuVisible) {
            UpdatePlayingState(dt);
        }
    }

    // Interpolate entity positions.
    game->GetEntityManager()->interpolateEntities(dt);

    // Refresh game info on HUD (level, enemy count, etc.).
    game->GetHUD().refreshGameInfo(winSize,
                               game->GetCurrentLevel(),
                               game->GetEnemies().size(),
                               game->GetLocalPlayer(),
                               nextLevelTimer,
                               game->GetPlayers());
}

//---------------------------------------------------------
// Level & Timer Helpers
//---------------------------------------------------------
void GameplayState::CheckAndAdvanceLevel() {
    if (game->GetEnemies().empty() && nextLevelTimer <= 0 && game->GetCurrentState() != GameState::GameOver) {
        NextLevel();
    }
}

void GameplayState::StartNextLevelTimer(float duration) {
    if (!timerActive) {
        nextLevelTimer = duration;
        timerActive = true;
        if (game->IsHost()) {
            char buffer[64];
            int bytes = snprintf(buffer, sizeof(buffer), "S|NEXT|%.1f", duration);
            if (bytes > 0 && static_cast<size_t>(bytes) < sizeof(buffer)) {
                game->GetNetworkManager()->SendGameplayMessage(std::string(buffer));
            }
        }
    }
}

//---------------------------------------------------------
// Update Playing State
//---------------------------------------------------------
void GameplayState::UpdatePlayingState(float dt) {
    // Update camera position.
    UpdateCamera(dt);

    // Update local player movement and send updates.
    Player& localPlayer = game->GetLocalPlayer();
    if (localPlayer.isAlive) {
        bool playerMoved = localPlayer.move(dt);
        if (playerMoved) {
            localPlayer.renderedX = localPlayer.x;
            localPlayer.renderedY = localPlayer.y;
            localPlayer.shape.setPosition(localPlayer.renderedX, localPlayer.renderedY);
            game->GetNetworkManager()->ThrottledSendPlayerUpdate();
        }
        // Allow shooting on left mouse click.
        if (sf::Mouse::isButtonPressed(sf::Mouse::Left)) {
            game->GetLocalPlayer().ShootBullet(game);
        }
    }

    // Check for collisions between bullets and enemies, and between players and enemies.
    game->GetEntityManager()->checkCollisions(
        // Lambda for bullet-enemy collision.
        [&](const Bullet& b, uint64_t enemyId) {
            uint64_t timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::system_clock::now().time_since_epoch()).count();
            int damage = 10;
            char hitBuffer[128];
            snprintf(hitBuffer, sizeof(hitBuffer), "H|%llu|%llu|%llu|%d|%llu",
                     b.id, enemyId, game->GetLocalPlayer().steamID.ConvertToUint64(), damage, timestamp);
            game->GetNetworkManager()->SendGameplayMessage(std::string(hitBuffer));

            // Client-side prediction: if enemy exists, reduce health locally.
            if (!game->IsHost() && game->GetEnemies().count(enemyId)) {
                Enemy& enemy = game->GetEnemies()[enemyId];
                enemy.health -= damage;
                if (enemy.health <= 0) {
                    game->GetEntityManager()->getEnemies().erase(enemyId);
                }
            }
        },
        // Lambda for enemy-player collision.
        [&](CSteamID playerId, uint64_t enemyId) {
            if (game->GetPlayers().count(playerId)) {
                Player& player = game->GetPlayers()[playerId];
                if (player.isAlive) {
                    player.health -= 10;
                    if (player.health <= 0) {
                        player.isAlive = false;
                        std::cout << "[DEBUG] Player " << playerId.ConvertToUint64() << " died" << std::endl;
                        // Send updated player state to network.
                        char buffer[256];
                        int bytes = snprintf(buffer, sizeof(buffer),
                                             "P|%llu|%.1f|%.1f|%.1f|%.1f|%d|%d|%d|%d|%.1f|%d",
                                             player.steamID.ConvertToUint64(), player.x, player.y,
                                             player.renderedX, player.renderedY, player.health,
                                             player.kills, player.ready ? 1 : 0, player.money, player.speed, player.isAlive ? 1 : 0);
                        if (bytes > 0 && static_cast<size_t>(bytes) < sizeof(buffer)) {
                            if (game->IsHost()) {
                                game->GetNetworkManager()->broadcastMessage(std::string(buffer));
                            } else {
                                const char* hostStr = SteamMatchmaking()->GetLobbyData(game->GetCurrentLobby(), "host_steam_id");
                                if (hostStr && *hostStr) {
                                    CSteamID hostID(std::stoull(hostStr));
                                    game->GetNetworkManager()->sendMessage(hostID, std::string(buffer));
                                }
                            }
                        }
                        game->GetLocalPlayer() = player;
                    }
                }
            }
        }
    );

    // Process pending hits (for client-side delayed updates).
    for (auto it = pendingHits.begin(); it != pendingHits.end();) {
        it->retryTimer -= dt;
        if (it->retryTimer <= 0) {
            if (game->GetEnemies().count(it->enemyId)) {
                uint64_t timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
                    std::chrono::system_clock::now().time_since_epoch()).count();
                int damage = 10;
                char hitBuffer[128];
                snprintf(hitBuffer, sizeof(hitBuffer), "H|%llu|%llu|%llu|%d|%llu",
                         it->bulletId, it->enemyId, it->shooterSteamID, damage, timestamp);
                game->GetNetworkManager()->SendGameplayMessage(std::string(hitBuffer));
                it->retryTimer = 0.5f;
                ++it;
            } else {
                it = pendingHits.erase(it);
            }
        } else {
            ++it;
        }
    }
}


//---------------------------------------------------------
// Process Event
//---------------------------------------------------------
void GameplayState::ProcessEvent(const sf::Event& event) {
    if (event.type == sf::Event::KeyPressed) {
        if (event.key.code == sf::Keyboard::Escape) {
            menuVisible = !menuVisible;
        } else if (menuVisible && event.key.code == sf::Keyboard::M) {
            game->ReturnToMainMenu();
        } else if (event.key.code == sf::Keyboard::B) {
            shopOpen = !shopOpen;
        }
    }

    if (event.type == sf::Event::MouseButtonPressed && event.mouseButton.button == sf::Mouse::Left) {
        if (shopOpen) {
            HandleStorePurchase();
        } else if (game->GetCurrentState() == GameState::Playing && game->GetLocalPlayer().isAlive && !menuVisible) {
            game->GetLocalPlayer().ShootBullet(game);  
        }
    }
}

//---------------------------------------------------------
// Camera Update
//---------------------------------------------------------
void GameplayState::UpdateCamera(float dt) {
    const Player& localPlayer = game->GetLocalPlayer();
    sf::View view = game->GetView();
    view.setCenter(localPlayer.renderedX, localPlayer.renderedY);
    game->GetWindow().setView(view);
}

//---------------------------------------------------------
// Rendering Functions
//---------------------------------------------------------
void GameplayState::Render() {
    game->GetWindow().clear(sf::Color::Black);
    sf::View currentView = game->GetWindow().getView();
    RenderGrid(game->GetWindow(), currentView);
    RenderPlayers();
    RenderEnemies();
    RenderBullets();
    game->GetHUD().render(game->GetWindow(), currentView, game->GetCurrentState());
    game->GetWindow().display();
}

void GameplayState::RenderPlayers() {
    for (auto& playerPair : game->GetPlayers()) {
        Player& player = playerPair.second;
        if (!std::isnan(player.renderedX) && !std::isnan(player.renderedY)) {
            player.shape.setPosition(player.renderedX, player.renderedY);
            game->GetWindow().draw(player.shape);
        }
    }
}

void GameplayState::RenderEnemies() {
    updateEnemyVertices();
    game->GetWindow().draw(enemyVertices);
}

void GameplayState::RenderBullets() {
    for (auto& bulletPair : game->GetEntityManager()->getBullets()) {
        Bullet& bullet = bulletPair.second;
        if (!std::isnan(bullet.renderedX) && !std::isnan(bullet.renderedY)) {
            bullet.shape.setPosition(bullet.renderedX, bullet.renderedY);
            game->GetWindow().draw(bullet.shape);
        }
    }
}

//---------------------------------------------------------
// Grid Rendering (for debugging or visual effect)
//---------------------------------------------------------
void GameplayState::RenderGrid(sf::RenderWindow& window, const sf::View& camera) {
    const float gridSize = 50.f;
    const int maxLines = 1000;
    sf::Vector2f camCenter = camera.getCenter();
    sf::Vector2f camSize = camera.getSize();

    int startX = static_cast<int>(std::floor((camCenter.x - camSize.x / 2) / gridSize)) - 1;
    int endX = static_cast<int>(std::ceil((camCenter.x + camSize.x / 2) / gridSize)) + 1;
    int startY = static_cast<int>(std::floor((camCenter.y - camSize.y / 2) / gridSize)) - 1;
    int endY = static_cast<int>(std::ceil((camCenter.y + camSize.y / 2) / gridSize)) + 1;

    int numVertical = endX - startX + 1;
    int numHorizontal = endY - startY + 1;
    if (numVertical * numHorizontal > maxLines) {
        float factor = std::sqrt(static_cast<float>(maxLines) / (numVertical * numHorizontal));
        int newWidth = static_cast<int>(numVertical * factor);
        int newHeight = static_cast<int>(numHorizontal * factor);
        startX = static_cast<int>(camCenter.x / gridSize) - newWidth / 2;
        endX = startX + newWidth;
        startY = static_cast<int>(camCenter.y / gridSize) - newHeight / 2;
        endY = startY + newHeight;
    }

    sf::VertexArray lines(sf::Lines);
    for (int x = startX; x <= endX; ++x) {
        float lineX = x * gridSize;
        lines.append(sf::Vertex(sf::Vector2f(lineX, startY * gridSize), sf::Color(50, 50, 50)));
        lines.append(sf::Vertex(sf::Vector2f(lineX, endY * gridSize), sf::Color(50, 50, 50)));
    }
    for (int y = startY; y <= endY; ++y) {
        float lineY = y * gridSize;
        lines.append(sf::Vertex(sf::Vector2f(startX * gridSize, lineY), sf::Color(50, 50, 50)));
        lines.append(sf::Vertex(sf::Vector2f(endX * gridSize, lineY), sf::Color(50, 50, 50)));
    }
    window.draw(lines);
}

//---------------------------------------------------------
// Level Transition and Store Purchase
//---------------------------------------------------------
void GameplayState::NextLevel() {
    game->GetCurrentLevel() += 1;
    game->GetEnemiesPerWave() += 2;
    StartNextLevelTimer(5.0f);
    if (game->IsHost()) {
        game->GetNetworkManager()->SyncEnemiesFull();
    }
}

void GameplayState::HandleStorePurchase() {
    sf::Vector2i mousePos = sf::Mouse::getPosition(game->GetWindow());
    sf::Vector2f viewPos = game->GetWindow().mapPixelToCoords(mousePos, game->GetView());
    const sf::Text& buttonText = game->GetHUD().getElements().at("speedBoostButton").text;
    if (buttonText.getGlobalBounds().contains(viewPos) && game->GetLocalPlayer().money >= 50) {
        Player& localPlayer = game->GetLocalPlayer();
        localPlayer.money -= 50;
        localPlayer.speed += 50.f;
        game->GetPlayers()[localPlayer.steamID] = localPlayer;
        game->GetNetworkManager()->ThrottledSendPlayerUpdate();
    }
}

//---------------------------------------------------------
// Enemy Vertex Update for Batch Rendering
//---------------------------------------------------------
void GameplayState::updateEnemyVertices() {
    enemyVertices.clear();
    enemyVertices.setPrimitiveType(sf::Quads);
    enemyVertices.resize(game->GetEnemies().size() * 4);
    size_t i = 0;
    for (const auto& [id, enemy] : game->GetEnemies()) {
        if (enemy.health <= 0 || std::isnan(enemy.renderedX) || std::isnan(enemy.renderedY))
            continue;
        float x = enemy.renderedX;
        float y = enemy.renderedY;
        if (enemy.isSplitting && enemy.shakeTimer > 0) {
            float shake = enemy.shakeTimer / enemy.shakeDuration;
            x += (rand() % 10 - 5) * shake;
            y += (rand() % 10 - 5) * shake;
        }
        float w = enemy.size.x, h = enemy.size.y;
        sf::Color c = enemy.color;
        enemyVertices[i * 4 + 0].position = {x, y};
        enemyVertices[i * 4 + 1].position = {x + w, y};
        enemyVertices[i * 4 + 2].position = {x + w, y + h};
        enemyVertices[i * 4 + 3].position = {x, y + h};
        for (int j = 0; j < 4; ++j)
            enemyVertices[i * 4 + j].color = c;
        ++i;
    }
    enemyVertices.resize(i * 4); // Trim unused vertices
}
