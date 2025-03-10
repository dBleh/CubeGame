#include "EntityManager.h"
#include "CubeGame.h"
#include <cmath>
#include <random>
#include <limits>
#include <iostream>
#include <algorithm>
#include <chrono>

//-------------------------------------------------------------------------
// Constructor & Destructor
//-------------------------------------------------------------------------
EntityManager::EntityManager() : lastEnemyUpdateTime(0.0f), enemyUpdateInterval(0.5f) {}

EntityManager::~EntityManager() {}

//-------------------------------------------------------------------------
// Accessor Methods
//-------------------------------------------------------------------------
std::unordered_map<CSteamID, Player, CSteamIDHash>& EntityManager::getPlayers() {
    return m_players;
}

std::unordered_map<uint64_t, Bullet>& EntityManager::getBullets() {
    return m_bullets;
}

std::unordered_map<uint64_t, Enemy>& EntityManager::getEnemies() {
    return m_enemies;
}

Player& EntityManager::getLocalPlayer(CubeGame* game) {
    // This function returns the local player.
    // In this simple implementation, we assume the local player is the first in the map.
    return m_players.begin()->second;
}

//-------------------------------------------------------------------------
// Update Entities
//-------------------------------------------------------------------------

void EntityManager::updateEntities(float dt) {
    for (auto it = m_bullets.begin(); it != m_bullets.end();) {
        it->second.update(dt);
        if (it->second.lifetime <= 0) {
            it = m_bullets.erase(it);
        } else {
            ++it;
        }
    }
    for (auto& [id, player] : m_players) {
        player.updateOrbitingCube(dt);
    }

    updateCollisionGrid();
    lastEnemyUpdateTime += dt;
    bool shouldSendUpdate = lastEnemyUpdateTime >= enemyUpdateInterval;
    std::vector<uint64_t> enemiesToRemove;

    for (const auto& [playerId, player] : m_players) {
        if (!player.isAlive) continue;
        int px = int(player.renderedX / 100.f);
        int py = int(player.renderedY / 100.f);
        for (int dx = -4; dx <= 4; ++dx) {
            for (int dy = -4; dy <= 4; ++dy) {
                int key = (px + dx) * 1000 + (py + dy);
                if (collisionGrid.count(key)) {
                    auto& enemyIds = collisionGrid[key].enemyIds;
                    for (auto it = enemyIds.begin(); it != enemyIds.end(); ) {
                        uint64_t enemyId = *it;
                        Enemy& enemy = m_enemies[enemyId];
                        if (enemy.health <= 0) {
                            it = enemyIds.erase(it);
                            continue;
                        }

                        enemy.update(dt);

                        if (enemy.type == Enemy::Splitter && enemy.splitTimer <= 0 && !enemy.isSplitting && enemy.splitCount < enemy.maxSplits) {
                            static uint64_t splitCounter = 0;
                            uint64_t newId = enemy.id + (splitCounter << 32) + 1;
                            splitCounter++;

                            enemy.size *= 0.7f;
                            enemy.health /= 2;
                            enemy.splitCount++;
                            enemy.splitTimer = enemy.splitInterval;
                            enemy.isSplitting = false;
                            enemy.shouldStopMoving = false;
                            enemy.needsSync = true; // Flag for sync

                            auto& newEnemy = m_enemies.emplace(newId, Enemy()).first->second;
                            newEnemy.initialize(Enemy::Splitter);
                            newEnemy.health = enemy.health;
                            newEnemy.size = enemy.size;
                            newEnemy.color = enemy.color;
                            newEnemy.x = enemy.renderedX + 20.f;
                            newEnemy.y = enemy.renderedY + 20.f;
                            newEnemy.renderedX = newEnemy.x;
                            newEnemy.renderedY = newEnemy.y;
                            newEnemy.lastX = newEnemy.x;
                            newEnemy.lastY = newEnemy.y;
                            newEnemy.lastSentX = newEnemy.x;
                            newEnemy.lastSentY = newEnemy.y;
                            newEnemy.interpolationTime = 0.f;
                            newEnemy.spawnDelay = 0.1f;
                            newEnemy.needsSync = true; // Flag new enemy

                            std::cout << "Splitter " << enemy.id << " split at (" << enemy.renderedX << ", " << enemy.renderedY << ")\n";
                        }

                        float minDist = std::numeric_limits<float>::max();
                        sf::Vector2f targetPos(enemy.x, enemy.y);
                        bool foundAlivePlayer = false;
                        for (const auto& pPair : m_players) {
                            const Player& p = pPair.second;
                            if (!p.isAlive) continue;
                            float dx = p.renderedX - enemy.x;
                            float dy = p.renderedY - enemy.y;
                            float dist = std::sqrt(dx * dx + dy * dy);
                            if (dist < minDist) {
                                minDist = dist;
                                targetPos = sf::Vector2f(p.renderedX, p.renderedY);
                                foundAlivePlayer = true;
                            }
                        }

                        sf::Vector2f separationForce(0.f, 0.f);
                        int ex = int(enemy.renderedX / 100.f);
                        int ey = int(enemy.renderedY / 100.f);
                        for (int dx = -1; dx <= 1; ++dx) {
                            for (int dy = -1; dy <= 1; ++dy) {
                                int sepKey = (ex + dx) * 1000 + (ey + dy);
                                if (collisionGrid.count(sepKey)) {
                                    const auto& nearbyIds = collisionGrid[sepKey].enemyIds;
                                    for (uint64_t otherId : nearbyIds) {
                                        if (otherId == enemyId) continue;
                                        const Enemy& other = m_enemies[otherId];
                                        sf::Vector2f sep = enemy.calculateSeparation(other);
                                        separationForce.x += sep.x;
                                        separationForce.y += sep.y;
                                    }
                                }
                            }
                        }

                        if (foundAlivePlayer) {
                            enemy.move(dt, targetPos.x, targetPos.y);
                        }
                        float separationStrength = 100.0f;
                        enemy.x += separationForce.x * separationStrength * dt;
                        enemy.y += separationForce.y * separationStrength * dt;

                        if (shouldSendUpdate && (std::abs(enemy.x - enemy.lastSentX) > 10.0f || std::abs(enemy.y - enemy.lastSentY) > 10.0f)) {
                            enemy.needsSync = true; // Flag position change
                        }
                        ++it;
                    }
                }
            }
        }
    }

    if (shouldSendUpdate) {
        lastEnemyUpdateTime = 0.0f;
    }
}


//-------------------------------------------------------------------------
// Spawn Enemies
//-------------------------------------------------------------------------
void EntityManager::spawnEnemies(int enemiesPerWave, const std::unordered_map<CSteamID, Player, CSteamIDHash>& players, uint64_t hostID) {
    // Clear any existing enemies.
    m_enemies.clear();
    
    // Calculate the average position of alive players.
    sf::Vector2f avgPos(0.f, 0.f);
    int alivePlayers = 0;
    for (const auto& pair : players) {
        if (pair.second.isAlive) {
            avgPos.x += pair.second.x;
            avgPos.y += pair.second.y;
            alivePlayers++;
        }
    }
    if (alivePlayers > 0) {
        avgPos.x /= alivePlayers;
        avgPos.y /= alivePlayers;
    }

    // Randomize spawn parameters.
    float spawnRadius = 100.0f; // (Unused, but could be used for future logic)
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<> angleDist(0, 2 * M_PI);
    std::uniform_real_distribution<> distDist(200, 300);
    std::uniform_int_distribution<> typeDist(0, 1);

    for (int i = 0; i < enemiesPerWave; i++) {
        Enemy e;
        // Randomly choose between Splitter and Default enemy types.
        e.initialize(typeDist(gen) == 1 ? Enemy::Splitter : Enemy::Default);
        float angle = angleDist(gen);
        float dist = distDist(gen);
        e.x = avgPos.x + std::cos(angle) * dist;
        e.y = avgPos.y + std::sin(angle) * dist;
        e.renderedX = e.x;
        e.renderedY = e.y;
        e.lastX = e.x;
        e.lastY = e.y;
        e.lastSentX = e.x;
        e.lastSentY = e.y;
        e.shape.setPosition(e.x, e.y);
        // Generate enemy ID based on hostID and enemy index.
        e.id = ((hostID & 0xFFFF) << 16) | (i & 0xFFFF);
        m_enemies[e.id] = e;
    }
}

//-------------------------------------------------------------------------
// Interpolate Entities
//-------------------------------------------------------------------------
void EntityManager::interpolateEntities(float alpha, CubeGame* game) {
    const float fixedDt = 1.0f / 60.0f;
    updateEntities(fixedDt);

    for (auto& [id, player] : m_players) {
        if (id == game->GetLocalPlayer().steamID) {
            player.renderedX = player.x;
            player.renderedY = player.y;
            player.orbitingCube.renderedX = player.orbitingCube.x;
            player.orbitingCube.renderedY = player.orbitingCube.y;
        } else {
            if (player.interpolationTime > 0) {
                player.interpolationTime -= fixedDt;
                if (player.interpolationTime < 0) player.interpolationTime = 0;
                float t = 1.0f - (player.interpolationTime / Player::INTERPOLATION_DURATION);
                player.renderedX = player.lastX + (player.x - player.lastX) * t;
                player.renderedY = player.lastY + (player.y - player.lastY) * t;
            } else {
                // Predict movement when interpolation ends
                player.renderedX += player.velocityX * fixedDt;
                player.renderedY += player.velocityY * fixedDt;
                player.x = player.renderedX;
                player.y = player.renderedY;
            }
            player.orbitingCube.renderedX = player.renderedX + player.orbitingCube.radius * std::cos(player.orbitingCube.angle);
            player.orbitingCube.renderedY = player.renderedY + player.orbitingCube.radius * std::sin(player.orbitingCube.angle);
        }
        player.shape.setPosition(player.renderedX, player.renderedY);
        player.orbitingCube.shape.setPosition(player.orbitingCube.renderedX, player.orbitingCube.renderedY);
    }
    for (auto& [id, bullet] : m_bullets) {
        bullet.renderedX = bullet.lastX + (bullet.x - bullet.lastX) * alpha;
        bullet.renderedY = bullet.lastY + (bullet.y - bullet.lastY) * alpha;
        bullet.shape.setPosition(bullet.renderedX, bullet.renderedY);
    }
    for (auto& [id, enemy] : m_enemies) {
        enemy.renderedX = enemy.lastX + (enemy.x - enemy.lastX) * alpha;
        enemy.renderedY = enemy.lastY + (enemy.y - enemy.lastY) * alpha;
    }
}

//-------------------------------------------------------------------------
// Update Collision Grid
//-------------------------------------------------------------------------
void EntityManager::updateCollisionGrid() {
    collisionGrid.clear();
    const float cellSize = 100.f;
    // Populate the grid with enemy positions.
    for (const auto& [id, enemy] : m_enemies) {
        if (enemy.health <= 0) continue;
        int key = (int(enemy.renderedX / cellSize)) * 1000 + (int(enemy.renderedY / cellSize));
        collisionGrid[key].enemyIds.push_back(id);
    }
    // Populate the grid with bullet positions.
    for (const auto& [id, bullet] : m_bullets) {
        int key = (int(bullet.renderedX / cellSize)) * 1000 + (int(bullet.renderedY / cellSize));
        collisionGrid[key].bulletIds.push_back(id);
    }
}

//-------------------------------------------------------------------------
// Collision Detection
//-------------------------------------------------------------------------
void EntityManager::checkCollisions(
    std::function<void(const Bullet&, uint64_t enemyId)> onBulletEnemyCollision,
    std::function<void(CSteamID playerId, uint64_t enemyId)> onEnemyPlayerCollision
) {
    updateCollisionGrid();

    static uint64_t deathSequence = 0; // Unique sequence number for deaths

    // Bullet collisions
    for (auto bulletIt = m_bullets.begin(); bulletIt != m_bullets.end();) {
        bool bulletHit = false;
        int bx = int(bulletIt->second.renderedX / 100.f);
        int by = int(bulletIt->second.renderedY / 100.f);
        for (int dx = -1; dx <= 1; ++dx) {
            for (int dy = -1; dy <= 1; ++dy) {
                int key = (bx + dx) * 1000 + (by + dy);
                if (collisionGrid.count(key)) {
                    const GridCell& cell = collisionGrid[key];
                    for (uint64_t enemyId : cell.enemyIds) {
                        Enemy& enemy = m_enemies[enemyId];
                        if (enemy.health > 0 && 
                            bulletIt->second.shape.getGlobalBounds().intersects(enemy.getBounds())) {
                            onBulletEnemyCollision(bulletIt->second, enemyId);
                            bulletHit = true;
                            if (enemy.health <= 0) {
                                deathSequence++;
                                enemy.needsSync = true; // Flag for sync
                                enemy.deathSequence = deathSequence; // Store sequence
                            }
                            break;
                        }
                    }
                }
                if (bulletHit) break;
            }
            if (bulletHit) break;
        }
        if (bulletHit) {
            bulletIt = m_bullets.erase(bulletIt);
        } else {
            ++bulletIt;
        }
    }

    // Orbiting cube collisions
    for (auto& [playerId, player] : m_players) {
        if (!player.orbitingCube.active || !player.isAlive) continue;

        int cx = int(player.orbitingCube.renderedX / 100.f);
        int cy = int(player.orbitingCube.renderedY / 100.f);
        for (int dx = -1; dx <= 1; ++dx) {
            for (int dy = -1; dy <= 1; ++dy) {
                int key = (cx + dx) * 1000 + (cy + dy);
                if (collisionGrid.count(key)) {
                    auto& enemyIds = collisionGrid[key].enemyIds;
                    for (auto it = enemyIds.begin(); it != enemyIds.end();) {
                        Enemy& enemy = m_enemies[*it];
                        if (enemy.health > 0 && 
                            player.getOrbitingCubeBounds().intersects(enemy.getBounds())) {
                            enemy.health -= 10;
                            enemy.needsSync = true;
                            if (enemy.health <= 0) {
                                deathSequence++;
                                enemy.deathSequence = deathSequence;
                                player.kills += 1;
                                player.money += 10;
                                m_enemies.erase(*it);
                                it = enemyIds.erase(it);
                            } else {
                                ++it;
                            }
                        } else {
                            ++it;
                        }
                    }
                }
            }
        }
    }

    // Player-enemy collisions
    for (auto playerIt = m_players.begin(); playerIt != m_players.end(); ++playerIt) {
        int px = int(playerIt->second.renderedX / 100.f);
        int py = int(playerIt->second.renderedY / 100.f);
        for (int dx = -1; dx <= 1; ++dx) {
            for (int dy = -1; dy <= 1; ++dy) {
                int key = (px + dx) * 1000 + (py + dy);
                if (collisionGrid.count(key)) {
                    auto& enemyIds = collisionGrid[key].enemyIds;
                    for (auto it = enemyIds.begin(); it != enemyIds.end();) {
                        Enemy& enemy = m_enemies[*it];
                        if (playerIt->second.shape.getGlobalBounds().intersects(enemy.getBounds())) {
                            onEnemyPlayerCollision(playerIt->first, *it);
                            enemy.needsSync = true;
                            deathSequence++;
                            enemy.deathSequence = deathSequence;
                            m_enemies.erase(*it);
                            it = enemyIds.erase(it);
                        } else {
                            ++it;
                        }
                    }
                }
            }
        }
    }
}

//-------------------------------------------------------------------------
// Set Enemy Update Callback
//-------------------------------------------------------------------------


//-------------------------------------------------------------------------
// Check if Entities are Initialized
//-------------------------------------------------------------------------
bool EntityManager::areEntitiesInitialized() const {
    // Simple check: there should be at least one player.
    return !m_players.empty();
}

void EntityManager::queueUpdate(const EntityUpdate& update) {
    updateQueue.push(update);
}

void EntityManager::applyQueuedUpdates() {
    while (!updateQueue.empty()) {
        const EntityUpdate& update = updateQueue.front();
        switch (update.type) {
            case EntityUpdate::Type::Spawn:
                if (m_enemies.count(update.id) == 0) {
                    auto& newEnemy = m_enemies.emplace(update.id, Enemy()).first->second;
                    newEnemy.initialize(update.enemyType);
                    newEnemy.id = update.id;
                    newEnemy.x = update.x;
                    newEnemy.y = update.y;
                    newEnemy.health = update.health;
                    newEnemy.spawnDelay = update.spawnDelay;
                    newEnemy.renderedX = update.x;
                    newEnemy.renderedY = update.y;
                    newEnemy.lastX = update.x;
                    newEnemy.lastY = update.y;
                }
                break;
            case EntityUpdate::Type::Update:
                if (m_enemies.count(update.id) > 0) {
                    Enemy& e = m_enemies[update.id];
                    e.lastX = e.renderedX;
                    e.lastY = e.renderedY;
                    e.x = update.x;
                    e.y = update.y;
                    e.health = update.health;
                    e.spawnDelay = update.spawnDelay;
                    e.interpolationTime = INTERPOLATION_TIME;
                }
                break;
            case EntityUpdate::Type::Remove:
                m_enemies.erase(update.id);
                break;
        }
        updateQueue.pop();
    }
}