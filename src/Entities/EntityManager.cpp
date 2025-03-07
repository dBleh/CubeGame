#include "EntityManager.h"
#include <cmath>
#include <random>
#include <limits>
#include <iostream>
#include <algorithm>

EntityManager::EntityManager() : lastEnemyUpdateTime(0.0f), enemyUpdateInterval(0.5f) {}

EntityManager::~EntityManager() {}

std::unordered_map<CSteamID, Player, CSteamIDHash>& EntityManager::getPlayers() {
    return m_players;
}

std::unordered_map<uint64_t, Bullet>& EntityManager::getBullets() {
    return m_bullets;
}

std::unordered_map<uint64_t, Enemy>& EntityManager::getEnemies() {
    return m_enemies;
}

// EntityManager.cpp
void EntityManager::updateEntities(float dt) {
    for (auto it = m_bullets.begin(); it != m_bullets.end();) {
        it->second.update(dt);
        if (it->second.lifetime <= 0) {
            it = m_bullets.erase(it);
        } else {
            ++it;
        }
    }

    lastEnemyUpdateTime += dt;
    bool shouldSendUpdate = lastEnemyUpdateTime >= enemyUpdateInterval;

    // Use a vector to collect enemies to remove after iteration
    std::vector<uint64_t> enemiesToRemove;

    for (auto &pair : m_enemies) {
        Enemy& enemy = pair.second;
        if (enemy.health > 0) {
            enemy.update(dt);

            if (enemy.type == Enemy::Splitter && enemy.splitTimer <= 0) {
                static uint64_t splitCounter = 0;
                uint64_t newId1 = enemy.id + (splitCounter++ << 32);
                uint64_t newId2 = enemy.id + (splitCounter++ << 32);

                // Create two new enemies
                auto& newEnemy1 = m_enemies.emplace(newId1, Enemy()).first->second;
                auto& newEnemy2 = m_enemies.emplace(newId2, Enemy()).first->second;

                newEnemy1.initialize(Enemy::Splitter);
                newEnemy2.initialize(Enemy::Splitter);

                newEnemy1.health = enemy.health / 2;
                newEnemy2.health = enemy.health / 2;
                newEnemy1.shape.setSize(sf::Vector2f(enemy.shape.getSize().x * 0.7f, enemy.shape.getSize().y * 0.7f));
                newEnemy2.shape.setSize(sf::Vector2f(enemy.shape.getSize().x * 0.7f, enemy.shape.getSize().y * 0.7f));
                newEnemy1.shape.setFillColor(enemy.shape.getFillColor());
                newEnemy2.shape.setFillColor(enemy.shape.getFillColor());

                newEnemy1.x = enemy.x + 20.f;
                newEnemy1.y = enemy.y + 20.f;
                newEnemy2.x = enemy.x - 20.f;
                newEnemy2.y = enemy.y - 20.f;
                newEnemy1.renderedX = newEnemy1.x;
                newEnemy1.renderedY = newEnemy1.y;
                newEnemy2.renderedX = newEnemy2.x;
                newEnemy2.renderedY = newEnemy2.y;
                newEnemy1.shape.setPosition(newEnemy1.renderedX, newEnemy1.renderedY);
                newEnemy2.shape.setPosition(newEnemy2.renderedX, newEnemy2.renderedY);

                // Change original color before destruction (optional visual cue)
                if (enemy.shape.getFillColor() == sf::Color::Cyan) {
                    enemy.shape.setFillColor(sf::Color::Yellow);
                } else if (enemy.shape.getFillColor() == sf::Color::Yellow) {
                    enemy.shape.setFillColor(sf::Color::Magenta);
                } else if (enemy.shape.getFillColor() == sf::Color::Magenta) {
                    enemy.shape.setFillColor(sf::Color::White);
                }

                

                // Broadcast new enemy spawns
                uint64_t timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
                    std::chrono::system_clock::now().time_since_epoch()).count();
                char buffer1[128], buffer2[128];
                int bytes1 = snprintf(buffer1, sizeof(buffer1), "E|SPAWN|%llu|%.1f|%.1f|%d|%.2f|%llu",
                                     newId1, newEnemy1.x, newEnemy1.y, newEnemy1.health, newEnemy1.spawnDelay, timestamp);
                int bytes2 = snprintf(buffer2, sizeof(buffer2), "E|SPAWN|%llu|%.1f|%.1f|%d|%.2f|%llu",
                                     newId2, newEnemy2.x, newEnemy2.y, newEnemy2.health, newEnemy2.spawnDelay, timestamp);
                if (bytes1 > 0 && static_cast<size_t>(bytes1) < sizeof(buffer1) && onEnemyUpdate) {
                    onEnemyUpdate(std::string(buffer1));
                }
                if (bytes2 > 0 && static_cast<size_t>(bytes2) < sizeof(buffer2) && onEnemyUpdate) {
                    onEnemyUpdate(std::string(buffer2));
                }

                // Mark original enemy for removal
                enemiesToRemove.push_back(enemy.id);
                

                // Broadcast removal of original enemy
                char removeBuffer[64];
                int removeBytes = snprintf(removeBuffer, sizeof(removeBuffer), "E|REMOVE|%llu", enemy.id);
                if (removeBytes > 0 && static_cast<size_t>(removeBytes) < sizeof(removeBuffer) && onEnemyUpdate) {
                    onEnemyUpdate(std::string(removeBuffer));
                }
            }

            float minDist = std::numeric_limits<float>::max();
            sf::Vector2f targetPos(enemy.x, enemy.y);
            bool foundAlivePlayer = false;

            for (const auto& playerPair : m_players) {
                const Player& player = playerPair.second;
                if (!player.isAlive) continue;
                float dx = player.renderedX - enemy.x;
                float dy = player.renderedY - enemy.y;
                float dist = std::sqrt(dx * dx + dy * dy);
                if (dist < minDist) {
                    minDist = dist;
                    targetPos = sf::Vector2f(player.renderedX, player.renderedY);
                    foundAlivePlayer = true;
                }
            }

            if (foundAlivePlayer) {
                float speed = (enemy.type == Enemy::Splitter) ? 80.0f : 100.0f;
                float dx = targetPos.x - enemy.x;
                float dy = targetPos.y - enemy.y;
                float length = std::sqrt(dx * dx + dy * dy);
                if (length > 0) {
                    dx /= length;
                    dy /= length;
                    enemy.x += dx * speed * dt;
                    enemy.y += dy * speed * dt;
                    enemy.renderedX = enemy.x;
                    enemy.renderedY = enemy.y;
                    enemy.shape.setPosition(enemy.renderedX, enemy.renderedY);
                }
            }

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
                if (bytes > 0 && static_cast<size_t>(bytes) < sizeof(buffer)) {
                    onEnemyUpdate(std::string(buffer));
                }
            }
        }
    }

    // Remove split enemies after iteration to avoid iterator invalidation
    for (const auto& id : enemiesToRemove) {
        m_enemies.erase(id);
    }

    if (shouldSendUpdate) {
        lastEnemyUpdateTime = 0.0f;
    }
}
void EntityManager::spawnEnemies(int enemiesPerWave, const std::unordered_map<CSteamID, Player, CSteamIDHash>& players, uint64_t hostID) {
    m_enemies.clear();
    
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

    const float SPAWN_RADIUS = 100.0f;
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<> angleDist(0, 2 * M_PI);
    std::uniform_real_distribution<> distDist(200, 300);
    std::uniform_int_distribution<> typeDist(0, 1);

    for (int i = 0; i < enemiesPerWave; i++) {
        Enemy e;
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
        e.id = ((hostID & 0xFFFF) << 16) | (i & 0xFFFF);
        m_enemies[e.id] = e; // This assignment should now work
    }
}
void EntityManager::interpolateEntities(float dt) {
    // Interpolate all player positions.
    for (auto& [id, player] : m_players) {
        player.renderedX = player.x;
        player.renderedY = player.y;
        player.shape.setPosition(player.renderedX, player.renderedY);
    }
    
    // Interpolate enemy positions.
    for (auto& [id, enemy] : m_enemies) {
        if (enemy.interpolationTime > 0) {
            enemy.interpolationTime -= dt;
            float t = 1.0f - (enemy.interpolationTime / 0.2f);
            enemy.renderedX = enemy.lastX + (enemy.x - enemy.lastX) * t;
            enemy.renderedY = enemy.lastY + (enemy.y - enemy.lastY) * t;
        } else {
            enemy.renderedX = enemy.x;
            enemy.renderedY = enemy.y;
        }
        enemy.shape.setPosition(enemy.renderedX, enemy.renderedY);
    }
    
    // Call updateEntities to handle bullets and enemy movements.
    updateEntities(dt);
}

void EntityManager::checkCollisions(
    std::function<void(const Bullet&, uint64_t enemyId)> onBulletEnemyCollision,
    std::function<void(CSteamID playerId, uint64_t enemyId)> onEnemyPlayerCollision
) {
    for (auto bulletIt = m_bullets.begin(); bulletIt != m_bullets.end();) {
        bool bulletHit = false;
        for (auto enemyIt = m_enemies.begin(); enemyIt != m_enemies.end();) {
            if (bulletIt->second.shape.getGlobalBounds().intersects(enemyIt->second.shape.getGlobalBounds())) {
                onBulletEnemyCollision(bulletIt->second, enemyIt->first);
                bulletHit = true;
                break;
            } else {
                ++enemyIt;
            }
        }
        if (bulletHit) {
            bulletIt = m_bullets.erase(bulletIt);
        } else {
            ++bulletIt;
        }
    }

    for (auto playerIt = m_players.begin(); playerIt != m_players.end(); ++playerIt) {
        for (auto enemyIt = m_enemies.begin(); enemyIt != m_enemies.end();) {
            if (playerIt->second.shape.getGlobalBounds().intersects(enemyIt->second.shape.getGlobalBounds())) {
                onEnemyPlayerCollision(playerIt->first, enemyIt->first);
                enemyIt = m_enemies.erase(enemyIt);
            } else {
                ++enemyIt;
            }
        }
    }
}

void EntityManager::setEnemyUpdateCallback(std::function<void(const std::string&)> callback) {
    onEnemyUpdate = callback;
}

bool EntityManager::areEntitiesInitialized() const {
    // Simple check: at least the player entities container should be initialized
    return !m_players.empty();
}
