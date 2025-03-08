#include "EntityManager.h"
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
    // Update each bullet and remove it if its lifetime has expired.
    for (auto it = m_bullets.begin(); it != m_bullets.end();) {
        it->second.update(dt);
        if (it->second.lifetime <= 0) {
            it = m_bullets.erase(it);
        } else {
            ++it;
        }
    }

    // Refresh the collision grid for the current frame.
    updateCollisionGrid();

    // Increment the enemy update timer.
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

                        // Handle Splitter logic
                        if (enemy.type == Enemy::Splitter && enemy.splitTimer <= 0 && !enemy.isSplitting && enemy.splitCount < enemy.maxSplits) {
                            static uint64_t splitCounter = 0;
                            uint64_t newId = enemy.id + (splitCounter << 32) + 1;
                            splitCounter++;

                            // Shrink the original enemy
                            enemy.size *= 0.7f; // Reduce size by 70%
                            enemy.health /= 2;  // Halve health
                            enemy.splitCount++; // Increment split counter
                            enemy.splitTimer = enemy.splitInterval; // Reset split timer
                            enemy.isSplitting = false; // Reset splitting state
                            enemy.shouldStopMoving = false; // Allow movement again

                            // Create a new enemy as a "copy"
                            auto& newEnemy = m_enemies.emplace(newId, Enemy()).first->second;
                            newEnemy.initialize(Enemy::Splitter);
                            newEnemy.health = enemy.health; // Same health as the shrunk original
                            newEnemy.size = enemy.size;     // Same size as the shrunk original
                            newEnemy.color = enemy.color;
                            newEnemy.x = enemy.renderedX + 20.f; // Offset slightly
                            newEnemy.y = enemy.renderedY + 20.f;
                            newEnemy.renderedX = newEnemy.x;
                            newEnemy.renderedY = newEnemy.y;
                            newEnemy.lastX = newEnemy.x;
                            newEnemy.lastY = newEnemy.y;
                            newEnemy.lastSentX = newEnemy.x;
                            newEnemy.lastSentY = newEnemy.y;
                            newEnemy.interpolationTime = 0.f;
                            newEnemy.spawnDelay = 0.1f; // Small delay for spawn effect

                            std::cout << "Splitter " << enemy.id << " split at (" << enemy.renderedX << ", " << enemy.renderedY << ")\n";
                            std::cout << "NewEnemy ID " << newId << " spawned at (" << newEnemy.x << ", " << newEnemy.y << ")\n";

                            // Network update for the new enemy
                            uint64_t timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
                                std::chrono::system_clock::now().time_since_epoch()).count();
                            char buffer[128];
                            int bytes = snprintf(buffer, sizeof(buffer), "E|SPAWN|%llu|%.1f|%.1f|%d|%.2f|%llu",
                                                 newId, newEnemy.x, newEnemy.y, newEnemy.health, newEnemy.spawnDelay, timestamp);
                            if (bytes > 0 && static_cast<size_t>(bytes) < sizeof(buffer) && onEnemyUpdate)
                                onEnemyUpdate(std::string(buffer));

                            // Network update for the modified original enemy
                            char origBuffer[128];
                            int origBytes = snprintf(origBuffer, sizeof(origBuffer), "E|UPDATE|%llu|%.1f|%.1f|%d|%.2f|%llu",
                                                     enemy.id, enemy.x, enemy.y, enemy.health, enemy.spawnDelay, timestamp);
                            if (origBytes > 0 && static_cast<size_t>(origBytes) < sizeof(origBuffer) && onEnemyUpdate)
                                onEnemyUpdate(std::string(origBuffer));
                        }

                        // Move enemy toward nearest player
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

                        // Calculate separation force from nearby enemies
                        sf::Vector2f separationForce(0.f, 0.f);
                        int ex = int(enemy.renderedX / 100.f);
                        int ey = int(enemy.renderedY / 100.f);
                        for (int dx = -1; dx <= 1; ++dx) {
                            for (int dy = -1; dy <= 1; ++dy) {
                                int sepKey = (ex + dx) * 1000 + (ey + dy);
                                if (collisionGrid.count(sepKey)) {
                                    const auto& nearbyIds = collisionGrid[sepKey].enemyIds;
                                    for (uint64_t otherId : nearbyIds) {
                                        if (otherId == enemyId) continue; // Skip self
                                        const Enemy& other = m_enemies[otherId];
                                        sf::Vector2f sep = enemy.calculateSeparation(other);
                                        separationForce.x += sep.x;
                                        separationForce.y += sep.y;
                                    }
                                }
                            }
                        }

                        // Apply movement and separation
                        if (foundAlivePlayer) {
                            enemy.move(dt, targetPos.x, targetPos.y);
                        }
                        float separationStrength = 100.0f;
                        enemy.x += separationForce.x * separationStrength * dt;
                        enemy.y += separationForce.y * separationStrength * dt;

                        // Network synchronization
                        float posDeltaX = enemy.x - enemy.lastSentX;
                        float posDeltaY = enemy.y - enemy.lastSentY;
                        bool positionChanged = std::abs(posDeltaX) > 10.0f || std::abs(posDeltaY) > 10.0f;
                        if (shouldSendUpdate && positionChanged && onEnemyUpdate) {
                            enemy.lastSentX = enemy.x;
                            enemy.lastSentY = enemy.y;
                            uint64_t timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
                                std::chrono::system_clock::now().time_since_epoch()).count();
                            char buffer[128];
                            int bytes = snprintf(buffer, sizeof(buffer), "E|UPDATE|%llu|%.1f|%.1f|%d|%.2f|%llu",
                                                 enemy.id, enemy.x, enemy.y, enemy.health, enemy.spawnDelay, timestamp);
                            if (bytes > 0 && static_cast<size_t>(bytes) < sizeof(buffer))
                                onEnemyUpdate(std::string(buffer));
                        }
                        ++it;
                    }
                }
            }
        }
    }

    // Process any remaining removals (though not needed for Splitters anymore)
    for (const auto& id : enemiesToRemove) {
        if (m_enemies.erase(id)) {
            std::cout << "[DEBUG] Deferred removal of enemy " << id << "\n";
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
void EntityManager::interpolateEntities(float dt) {
    // For each player, update the rendered position to match the logical position.
    for (auto& [id, player] : m_players) {
        player.renderedX = player.x;
        player.renderedY = player.y;
        player.shape.setPosition(player.renderedX, player.renderedY);
    }
    // For enemies with network updates, interpolate smoothly between the last and current positions.
    for (auto& [id, enemy] : m_enemies) {
        if (enemy.interpolationTime > 0) {
            enemy.interpolationTime -= dt;
            float t = 1.0f - (enemy.interpolationTime / 0.2f);
            enemy.renderedX = enemy.lastX + (enemy.x - enemy.lastX) * t;
            enemy.renderedY = enemy.lastY + (enemy.y - enemy.lastY) * t;
        }
    }
    // Continue updating entity states.
    updateEntities(dt);
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
    // Update the collision grid to reflect current positions.
    updateCollisionGrid();
    
    // Process bullet-enemy collisions.
    for (auto bulletIt = m_bullets.begin(); bulletIt != m_bullets.end();) {
        bool bulletHit = false;
        int bx = int(bulletIt->second.renderedX / 100.f);
        int by = int(bulletIt->second.renderedY / 100.f);
        // Check neighboring grid cells.
        for (int dx = -1; dx <= 1; ++dx) {
            for (int dy = -1; dy <= 1; ++dy) {
                int key = (bx + dx) * 1000 + (by + dy);
                if (collisionGrid.count(key)) {
                    const GridCell& cell = collisionGrid[key];
                    for (uint64_t enemyId : cell.enemyIds) {
                        Enemy& enemy = m_enemies[enemyId];
                        if (enemy.health > 0 && 
                            bulletIt->second.shape.getGlobalBounds().intersects(enemy.getBounds())) {
                            // Notify of bullet-enemy collision.
                            onBulletEnemyCollision(bulletIt->second, enemyId);
                            bulletHit = true;
                            break; // A bullet only hits one enemy.
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

    // Process player-enemy collisions.
    for (auto playerIt = m_players.begin(); playerIt != m_players.end(); ++playerIt) {
        int px = int(playerIt->second.renderedX / 100.f);
        int py = int(playerIt->second.renderedY / 100.f);
        for (int dx = -1; dx <= 1; ++dx) {
            for (int dy = -1; dy <= 1; ++dy) {
                int key = (px + dx) * 1000 + (py + dy);
                if (collisionGrid.count(key)) {
                    auto& enemyIds = collisionGrid[key].enemyIds;
                    // Iterate over enemy IDs in the cell.
                    for (auto it = enemyIds.begin(); it != enemyIds.end();) {
                        Enemy& enemy = m_enemies[*it];
                        if (playerIt->second.shape.getGlobalBounds().intersects(enemy.getBounds())) {
                            // Notify of enemy-player collision.
                            onEnemyPlayerCollision(playerIt->first, *it);
                            it = enemyIds.erase(it); // Remove enemy from grid cell.
                            m_enemies.erase(*it);    // Also remove from main enemy container.
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
void EntityManager::setEnemyUpdateCallback(std::function<void(const std::string&)> callback) {
    onEnemyUpdate = callback;
}

//-------------------------------------------------------------------------
// Check if Entities are Initialized
//-------------------------------------------------------------------------
bool EntityManager::areEntitiesInitialized() const {
    // Simple check: there should be at least one player.
    return !m_players.empty();
}
