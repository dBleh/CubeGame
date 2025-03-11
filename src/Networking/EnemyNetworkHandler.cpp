#include "EnemyNetworkHandler.h"
#include "../Core/CubeGame.h"
#include <stdio.h>
#include <iostream>

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
            bool exists = game->entityManager->getEnemies().count(enemyID) > 0;
            update.type = exists ? EntityUpdate::Type::Update : EntityUpdate::Type::Spawn;
            update.id = enemyID;
            update.x = x;
            update.y = y;
            update.health = health;
            update.spawnDelay = spawnDelay;
            update.enemyType = static_cast<Enemy::Type>(type);
            update.timestamp = timestamp;
            game->entityManager->queueUpdate(update);
            game->networkManager->m_lastEnemyUpdateTime[enemyID] = timestamp;
            std::cout << "[EnemyNetworkHandler] " << (exists ? "Updated" : "Spawned") 
                      << " enemy " << enemyID << " from spawn msg at " << timestamp << std::endl;
        }
    } else {
        std::cout << "[EnemyNetworkHandler] Failed to parse spawn: " << msg << std::endl;
    }
}

void EnemyNetworkHandler::HandleEnemyUpdate(const std::string& msg) {
    uint64_t enemyID, timestamp;
    float x, y, spawnDelay;
    int health, type;
    int parsed = sscanf(msg.c_str(), "E|UPDATE|%llu|%f|%f|%d|%f|%d|%llu", 
                        &enemyID, &x, &y, &health, &spawnDelay, &type, &timestamp);
    if (parsed == 7) {
        if (!game->networkManager->m_lastEnemyUpdateTime.count(enemyID) || 
            game->networkManager->m_lastEnemyUpdateTime[enemyID] < timestamp) {
            EntityUpdate update;
            bool exists = game->entityManager->getEnemies().count(enemyID) > 0;
            update.type = exists ? EntityUpdate::Type::Update : EntityUpdate::Type::Spawn;
            update.id = enemyID;
            update.x = x;
            update.y = y;
            update.health = health;
            update.spawnDelay = spawnDelay;
            update.enemyType = static_cast<Enemy::Type>(type);
            update.timestamp = timestamp;
            game->entityManager->queueUpdate(update);
            game->networkManager->m_lastEnemyUpdateTime[enemyID] = timestamp;
            std::cout << "[EnemyNetworkHandler] " << (exists ? "Updated" : "Spawned") 
                      << " enemy " << enemyID << " from update msg at " << timestamp << std::endl;
        }
    } else {
        std::cout << "[EnemyNetworkHandler] Failed to parse update: " << msg << std::endl;
    }
}

void EnemyNetworkHandler::HandleEnemyDeath(const std::string& msg) {
    uint64_t enemyID, timestamp, killerID;
    int parsed = sscanf(msg.c_str(), "E|DEATH|%llu|%llu|%llu", &enemyID, &timestamp, &killerID);
    if (parsed == 3) {
        if (!game->networkManager->m_lastEnemyUpdateTime.count(enemyID) || 
            game->networkManager->m_lastEnemyUpdateTime[enemyID] < timestamp) {
            EntityUpdate update;
            update.type = EntityUpdate::Type::Remove;
            update.id = enemyID;
            update.timestamp = timestamp;
            game->entityManager->queueUpdate(update);
            game->networkManager->m_lastEnemyUpdateTime[enemyID] = timestamp;
            std::cout << "[EnemyNetworkHandler] Queued removal of enemy " << enemyID 
                      << " (killer: " << killerID << ") at " << timestamp << std::endl;
        }
    } else {
        std::cout << "[EnemyNetworkHandler] Failed to parse death: " << msg << std::endl;
    }
}

void EnemyNetworkHandler::HandleEnemyRemove(const std::string& msg) {
    uint64_t enemyID;
    int parsed = sscanf(msg.c_str(), "E|REMOVE|%llu", &enemyID);
    if (parsed == 1) {
        // Assuming NetworkManager broadcasts a timestamp with E|REMOVE in the future; for now, no timestamp check
        EntityUpdate update;
        update.type = EntityUpdate::Type::Remove;
        update.id = enemyID;
        update.timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()).count(); // Local timestamp as fallback
        game->entityManager->queueUpdate(update);
        std::cout << "[EnemyNetworkHandler] Queued removal of enemy " << enemyID << std::endl;
    } else {
        std::cout << "[EnemyNetworkHandler] Failed to parse remove: " << msg << std::endl;
    }
}