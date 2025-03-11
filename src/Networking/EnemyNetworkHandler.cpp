#include "EnemyNetworkHandler.h"
#include "../Core/CubeGame.h"
#include <stdio.h>

EnemyNetworkHandler::EnemyNetworkHandler(CubeGame* gameInstance) : game(gameInstance) {}

void EnemyNetworkHandler::HandleEnemySpawn(const std::string& msg) {
    uint64_t enemyID, timestamp;
    float x, y, spawnDelay;
    int health, type;
    int parsed = sscanf(msg.c_str(), "E|SPAWN|%llu|%f|%f|%d|%f|%d|%llu",
                        &enemyID, &x, &y, &health, &spawnDelay, &type, &timestamp);
    if (parsed == 7) {
        if (!game->networkManager->m_lastEnemyUpdateTime.count(enemyID) || 
            game->networkManager->m_lastEnemyUpdateTime[enemyID] < timestamp) {
            EntityUpdate update;
            update.type = EntityUpdate::Type::Spawn;
            update.id = enemyID;
            update.x = x;
            update.y = y;
            update.health = health;
            update.spawnDelay = spawnDelay;
            update.enemyType = static_cast<Enemy::Type>(type);
            update.timestamp = timestamp;
            game->entityManager->queueUpdate(update);
            game->networkManager->m_lastEnemyUpdateTime[enemyID] = timestamp;
        }
    }
}

void EnemyNetworkHandler::HandleEnemyUpdate(const std::string& msg) {
    uint64_t enemyID, timestamp;
    float x, y, spawnDelay;
    int health;
    if (sscanf(msg.c_str(), "E|UPDATE|%llu|%f|%f|%d|%f|%llu", &enemyID, &x, &y, &health, &spawnDelay, &timestamp) == 6) {
        if (!game->networkManager->m_lastEnemyUpdateTime.count(enemyID) || 
            game->networkManager->m_lastEnemyUpdateTime[enemyID] < timestamp) {
            EntityUpdate update;
            update.type = (game->entityManager->getEnemies().count(enemyID) > 0) 
                        ? EntityUpdate::Type::Update 
                        : EntityUpdate::Type::Spawn; // Treat as spawn if missing
            update.id = enemyID;
            update.x = x;
            update.y = y;
            update.health = health;
            update.spawnDelay = spawnDelay;
            update.timestamp = timestamp;
            // Assume default type if spawning (could improve with type in message)
            if (update.type == EntityUpdate::Type::Spawn) {
                update.enemyType = Enemy::Default; // Adjust as needed
            }
            game->entityManager->queueUpdate(update);
            game->networkManager->m_lastEnemyUpdateTime[enemyID] = timestamp;
        }
    }
}

void EnemyNetworkHandler::HandleEnemyDeath(const std::string& msg) {
    uint64_t enemyID, timestamp, killerID;
    if (sscanf(msg.c_str(), "E|DEATH|%llu|%llu|%llu", &enemyID, &timestamp, &killerID) == 3) {
        if (!game->networkManager->m_lastEnemyUpdateTime.count(enemyID) || 
            game->networkManager->m_lastEnemyUpdateTime[enemyID] < timestamp) {
            game->entityManager->getEnemies().erase(enemyID);
            game->networkManager->m_lastEnemyUpdateTime[enemyID] = timestamp;
        }
    }
}

void EnemyNetworkHandler::HandleEnemyRemove(const std::string& msg) {
    uint64_t enemyID;
    if (sscanf(msg.c_str(), "E|REMOVE|%llu", &enemyID) == 1) {
        EntityUpdate update;
        update.type = EntityUpdate::Type::Remove;
        update.id = enemyID;
        game->entityManager->queueUpdate(update);
    }
}