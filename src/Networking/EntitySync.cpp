#include "EntitySync.h"
#include "../States/GameplayState.h"
#include <chrono>

EntitySync::EntitySync(CubeGame* game, NetworkManager* networkManager)
    : game(game), networkManager(networkManager) {}

void EntitySync::SendPlayerUpdate() {
    Player& p = game->entityManager->getPlayers()[game->localSteamID];
    if (std::isnan(p.x) || std::isnan(p.y)) p.x = p.y = 0.0f;

    uint64_t timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();

    std::ostringstream oss;
    oss << "P|D|" << p.steamID.ConvertToUint64()
        << "|x|" << p.x
        << "|y|" << p.y
        << "|h|" << p.health
        << "|k|" << p.kills
        << "|r|" << (p.ready ? 1 : 0)
        << "|m|" << p.money
        << "|s|" << p.speed
        << "|a|" << (p.isAlive ? 1 : 0)
        << "|oca|" << p.orbitingCube.angle  // Current angle
        << "|st|" << p.startTimestamp       // Start timestamp for sync
        << "|t|" << timestamp;              // Current timestamp for reference

    std::string msg = oss.str();
    if (game->m_isHost) {
        broadcastMessage(msg);
    } else {
        const char* hostStr = SteamMatchmaking()->GetLobbyData(game->m_currentLobby, "host_steam_id");
        if (hostStr && *hostStr) {
            CSteamID hostID(std::stoull(hostStr));
            sendMessage(hostID, msg);
        }
    }

    p.lastUpdateTimestamp = timestamp;
    game->entityManager->getPlayers()[game->localSteamID] = p;
}
void EntitySync::ThrottledSendPlayerUpdate() {
    const float playerUpdateRate = 0.1f; // Send updates every 0.1 seconds
    if (m_playerUpdateClock.getElapsedTime().asSeconds() >= playerUpdateRate) {
        SendPlayerUpdate(); // Send update regardless of movement
        m_playerUpdateClock.restart();
    }
}
void EntitySync::SpawnEnemiesAndBroadcast() {
    if (!game->m_isHost) return;

    game->GetEntityManager()->spawnEnemies(
        game->GetEnemiesPerWave(),
        game->GetPlayers(),
        game->GetLocalPlayer().steamID.ConvertToUint64()
    );

    uint64_t timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();

    for (const auto& pair : game->GetEntityManager()->getEnemies()) {
        const Enemy& enemy = pair.second;
        char buffer[128];
        int bytes = snprintf(buffer, sizeof(buffer), "E|SPAWN|%llu|%.1f|%.1f|%d|%.2f|%d|%llu",
                             enemy.id, enemy.x, enemy.y, enemy.health, enemy.spawnDelay,
                             static_cast<int>(enemy.type), timestamp);
        if (bytes > 0 && static_cast<size_t>(bytes) < sizeof(buffer)) {
            broadcastMessage(std::string(buffer));
            m_lastEnemyUpdateTime[enemy.id] = timestamp;
        }
    }
}
void EntitySync::SyncEnemies() {
    if (!game->m_isHost) return;
    
    uint64_t timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    
    for (auto it = game->entityManager->getEnemies().begin(); it != game->entityManager->getEnemies().end();) {
        Enemy& enemy = it->second;
        if (enemy.health <= 0) {
            char buffer[64];
            int bytes = snprintf(buffer, sizeof(buffer), "E|REMOVE|%llu", enemy.id);
            if (bytes > 0 && static_cast<size_t>(bytes) < sizeof(buffer)) {
                broadcastMessage(std::string(buffer));
            }
            it = game->entityManager->getEnemies().erase(it);
        } else {
            char buffer[128];
            int bytes = snprintf(buffer, sizeof(buffer), "E|UPDATE|%llu|%.1f|%.1f|%d|%.2f|%llu",
                                enemy.id, enemy.x, enemy.y, enemy.health, enemy.spawnDelay, timestamp);
            if (bytes > 0 && static_cast<size_t>(bytes) < sizeof(buffer)) {
                broadcastMessage(std::string(buffer));
                m_lastEnemyUpdateTime[enemy.id] = timestamp;
            }
            ++it;
        }
    }
}
void EntitySync::SyncEnemiesFull() {
    if (!game->m_isHost) return;
    
    uint64_t timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    
    for (const auto& pair : game->entityManager->getEnemies()) {
        const Enemy& enemy = pair.second;
        if (enemy.health > 0) {
            char buffer[128];
            int bytes = snprintf(buffer, sizeof(buffer), "E|SPAWN|%llu|%.1f|%.1f|%d|%.2f|%llu",
                                enemy.id, enemy.x, enemy.y, enemy.health, enemy.spawnDelay, timestamp);
            if (bytes > 0 && static_cast<size_t>(bytes) < sizeof(buffer)) {
                broadcastMessage(std::string(buffer));
                m_lastEnemyUpdateTime[enemy.id] = timestamp;
            }
        }
    }
}
void EntitySync::BroadcastEnemyDeath(uint64_t enemyId, CSteamID killerID) {
    if (!game->m_isHost) return;
    
    uint64_t timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    
    char buffer[128];
    int bytes = snprintf(buffer, sizeof(buffer), "E|DEATH|%llu|%llu|%llu",
                        enemyId, timestamp, killerID.ConvertToUint64());
    if (bytes > 0 && static_cast<size_t>(bytes) < sizeof(buffer)) {
        broadcastMessage(std::string(buffer));
        game->entityManager->getEnemies().erase(enemyId);
        m_lastEnemyUpdateTime[enemyId] = timestamp;
    }
}
void EntitySync::HandleCollisionsAndSync(float dt, CubeGame* game) {
    // Access GameplayState for pendingHits and other state if needed
    GameplayState* gameplayState = game->GetGameplayState();
    if (!gameplayState) return;

    // Collision handling
    game->GetEntityManager()->checkCollisions(
        [&](const Bullet& b, uint64_t enemyId) {
            int damage = 10;
            if (game->IsHost() && game->GetEnemies().count(enemyId)) {
                if (b.shooterSteamID == game->GetLocalPlayer().steamID) {
                    Enemy& enemy = game->GetEnemies()[enemyId];
                    enemy.health -= damage;
                    if (enemy.health <= 0) {
                        game->GetEntityManager()->getEnemies().erase(enemyId);
                        Player& shooter = game->GetLocalPlayer();
                        shooter.kills += 1;
                        shooter.money += 10;
                        game->GetLocalPlayer() = shooter; // Update local player

                        char buffer[128];
                        int bytes = snprintf(buffer, sizeof(buffer), "P|D|%llu|k|%d|m|%d",
                                             shooter.steamID.ConvertToUint64(), shooter.kills, shooter.money);
                        if (bytes > 0 && static_cast<size_t>(bytes) < sizeof(buffer)) {
                            broadcastMessage(std::string(buffer));
                        }
                        bytes = snprintf(buffer, sizeof(buffer), "E|REMOVE|%llu", enemyId);
                        if (bytes > 0 && static_cast<size_t>(bytes) < sizeof(buffer)) {
                            broadcastMessage(std::string(buffer));
                        }
                    }
                } else {
                    uint64_t timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
                        std::chrono::system_clock::now().time_since_epoch()).count();
                    char hitBuffer[128];
                    snprintf(hitBuffer, sizeof(hitBuffer), "H|%llu|%llu|%llu|%d|%llu",
                             b.id, enemyId, b.shooterSteamID.ConvertToUint64(), damage, timestamp);
                    SendGameplayMessage(std::string(hitBuffer));
                }
            } else if (!game->IsHost()) {
                uint64_t timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
                    std::chrono::system_clock::now().time_since_epoch()).count();
                char hitBuffer[128];
                snprintf(hitBuffer, sizeof(hitBuffer), "H|%llu|%llu|%llu|%d|%llu",
                         b.id, enemyId, game->GetLocalPlayer().steamID.ConvertToUint64(), damage, timestamp);
                SendGameplayMessage(std::string(hitBuffer));

                if (game->GetEnemies().count(enemyId)) {
                    Enemy& enemy = game->GetEnemies()[enemyId];
                    enemy.health -= damage;
                    if (enemy.health <= 0) {
                        game->GetEntityManager()->getEnemies().erase(enemyId);
                    }
                }
            }
        },
        [&](CSteamID playerId, uint64_t enemyId) {
            if (game->GetPlayers().count(playerId)) {
                Player& player = game->GetPlayers()[playerId];
                if (player.isAlive && game->IsHost()) {
                    player.health -= 10;
                    if (player.health <= 0) {
                        player.isAlive = false;
                    }
                    char buffer[128];
                    int bytes = snprintf(buffer, sizeof(buffer), "P|D|%llu|h|%d|a|%d",
                                         player.steamID.ConvertToUint64(), player.health, player.isAlive ? 1 : 0);
                    if (bytes > 0 && static_cast<size_t>(bytes) < sizeof(buffer)) {
                        broadcastMessage(std::string(buffer));
                    }
                    game->GetPlayers()[playerId] = player;
                    if (playerId == game->GetLocalPlayer().steamID) {
                        game->GetLocalPlayer() = player;
                    }
                }
            }
        }
    );

    // Process pending hits
    for (auto it = gameplayState->pendingHits.begin(); it != gameplayState->pendingHits.end();) {
        it->retryTimer -= dt;
        if (it->retryTimer <= 0) {
            if (game->GetEnemies().count(it->enemyId)) {
                uint64_t timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
                    std::chrono::system_clock::now().time_since_epoch()).count();
                int damage = 10;
                char hitBuffer[128];
                snprintf(hitBuffer, sizeof(hitBuffer), "H|%llu|%llu|%llu|%d|%llu",
                         it->bulletId, it->enemyId, it->shooterSteamID, damage, timestamp);
                SendGameplayMessage(std::string(hitBuffer));
                it->retryTimer = 0.5f;
                ++it;
            } else {
                it = gameplayState->pendingHits.erase(it);
            }
        } else {
            ++it;
        }
    }

    // Handle orbiting cube collisions
    for (auto& [playerId, player] : game->GetPlayers()) {
        if (!player.orbitingCube.active || !player.isAlive) continue;

        int cx = int(player.orbitingCube.renderedX / 100.f);
        int cy = int(player.orbitingCube.renderedY / 100.f);
        for (int dx = -1; dx <= 1; ++dx) {
            for (int dy = -1; dy <= 1; ++dy) {
                int key = (cx + dx) * 1000 + (cy + dy);
                if (game->GetEntityManager()->collisionGrid.count(key)) {
                    auto& enemyIds = game->GetEntityManager()->collisionGrid[key].enemyIds;
                    for (auto it = enemyIds.begin(); it != enemyIds.end();) {
                        Enemy& enemy = game->GetEnemies()[*it];
                        if (enemy.health > 0 && 
                            player.getOrbitingCubeBounds().intersects(enemy.getBounds())) {
                            int damage = 10;
                            if (game->IsHost()) {
                                enemy.health -= damage;
                                if (enemy.health <= 0) {
                                    player.kills += 1;
                                    player.money += 10;
                                    game->GetPlayers()[playerId] = player;
                                    if (playerId == game->GetLocalPlayer().steamID) {
                                        game->GetLocalPlayer() = player;
                                    }

                                    char buffer[128];
                                    int bytes = snprintf(buffer, sizeof(buffer), 
                                                         "P|D|%llu|k|%d|m|%d",
                                                         player.steamID.ConvertToUint64(), 
                                                         player.kills, player.money);
                                    if (bytes > 0 && static_cast<size_t>(bytes) < sizeof(buffer)) {
                                        broadcastMessage(std::string(buffer));
                                    }

                                    bytes = snprintf(buffer, sizeof(buffer), 
                                                     "E|REMOVE|%llu", *it);
                                    if (bytes > 0 && static_cast<size_t>(bytes) < sizeof(buffer)) {
                                        broadcastMessage(std::string(buffer));
                                    }

                                    game->GetEntityManager()->getEnemies().erase(*it);
                                    it = enemyIds.erase(it);
                                } else {
                                    ++it;
                                }
                            } else { // Client
                                uint64_t timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
                                    std::chrono::system_clock::now().time_since_epoch()).count();
                                char hitBuffer[128];
                                snprintf(hitBuffer, sizeof(hitBuffer), 
                                         "H|%llu|%llu|%llu|%d|%llu",
                                         0ULL, *it, player.steamID.ConvertToUint64(), damage, timestamp);
                                SendGameplayMessage(std::string(hitBuffer));

                                if (enemy.health <= damage) { // Optimistic removal on client
                                    game->GetEntityManager()->getEnemies().erase(*it);
                                    it = enemyIds.erase(it);
                                } else {
                                    enemy.health -= damage; // Local prediction
                                    ++it;
                                }
                            }
                        } else {
                            ++it;
                        }
                    }
                }
            }
        }
    }
}
void EntitySync::SendGameplayMessage(const std::string& msg) {
    if (game->m_isHost) {
        networkManager->broadcastMessage(msg);
    } else {
        const char* hostStr = SteamMatchmaking()->GetLobbyData(game->m_currentLobby, "host_steam_id");
        if (hostStr && *hostStr) {
            CSteamID hostID(std::stoull(hostStr));
            networkManager->sendMessage(hostID, msg);
        }
    }
}