#include "EnemyNetworkHandler.h"
#include "NetworkManager.h"
#include "EntityManager.h"
#include <stdio.h>
#include <iostream>
#include <chrono>

// Define the static constant
const float EnemyNetworkHandler::ENEMY_UPDATE_INTERVAL = 0.5f; // 0.5s updates

// Constructor
EnemyNetworkHandler::EnemyNetworkHandler(CubeGame* game) : game(game) {}

// Send an enemy spawn message
void EnemyNetworkHandler::SendEnemySpawn(const Enemy& enemy) {
    uint64_t timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    char buffer[128];
    int bytes = snprintf(buffer, sizeof(buffer), "E|SPAWN|%llu|%.1f|%.1f|%d|%.2f|%d|%llu",
                         enemy.id, enemy.renderedX, enemy.renderedY, enemy.health, enemy.spawnDelay,
                         static_cast<int>(enemy.type), timestamp);
    if (bytes > 0 && static_cast<size_t>(bytes) < sizeof(buffer)) {
        std::string msg(buffer);
        NetworkManager* nm = game->GetNetworkManager();
        if (game->IsHost()) {
            nm->queueMessage(msg, NetworkManager::MessagePriority::High);
        } else {
            const char* hostStr = SteamMatchmaking()->GetLobbyData(game->GetCurrentLobby(), "host_steam_id");
            if (hostStr && *hostStr) {
                CSteamID hostID(std::stoull(hostStr));
                nm->queueMessage(msg, NetworkManager::MessagePriority::High, hostID);
            }
        }
    }
}

// Send an enemy update message
void EnemyNetworkHandler::SendEnemyUpdate(const Enemy& enemy) {
    uint64_t timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    char buffer[128];
    int bytes = snprintf(buffer, sizeof(buffer), "E|UPDATE|%llu|%.1f|%.1f|%d|%.2f|%llu",
                         enemy.id, enemy.renderedX, enemy.renderedY, enemy.health, enemy.spawnDelay, timestamp);
    if (bytes > 0 && static_cast<size_t>(bytes) < sizeof(buffer)) {
        std::string msg(buffer);
        NetworkManager* nm = game->GetNetworkManager();
        if (game->IsHost()) {
            nm->queueMessage(msg, NetworkManager::MessagePriority::Medium);
        } else {
            const char* hostStr = SteamMatchmaking()->GetLobbyData(game->GetCurrentLobby(), "host_steam_id");
            if (hostStr && *hostStr) {
                CSteamID hostID(std::stoull(hostStr));
                nm->queueMessage(msg, NetworkManager::MessagePriority::Medium, hostID);
            }
        }
    }
}

// Send an enemy death message
void EnemyNetworkHandler::SendEnemyDeath(uint64_t enemyId, CSteamID killerID) {
    uint64_t timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    char buffer[128];
    int bytes = snprintf(buffer, sizeof(buffer), "E|DEATH|%llu|%llu|%llu",
                         enemyId, timestamp, killerID.ConvertToUint64());
    if (bytes > 0 && static_cast<size_t>(bytes) < sizeof(buffer)) {
        std::string msg(buffer);
        NetworkManager* nm = game->GetNetworkManager();
        if (game->IsHost()) {
            nm->queueMessage(msg, NetworkManager::MessagePriority::High);
        } else {
            const char* hostStr = SteamMatchmaking()->GetLobbyData(game->GetCurrentLobby(), "host_steam_id");
            if (hostStr && *hostStr) {
                CSteamID hostID(std::stoull(hostStr));
                nm->queueMessage(msg, NetworkManager::MessagePriority::High, hostID);
            }
        }
    }
}

// Send an enemy removal message
void EnemyNetworkHandler::SendEnemyRemove(uint64_t enemyId) {
    char buffer[64];
    int bytes = snprintf(buffer, sizeof(buffer), "E|REMOVE|%llu", enemyId);
    if (bytes > 0 && static_cast<size_t>(bytes) < sizeof(buffer)) {
        std::string msg(buffer);
        NetworkManager* nm = game->GetNetworkManager();
        if (game->IsHost()) {
            nm->queueMessage(msg, NetworkManager::MessagePriority::High);
        } else {
            const char* hostStr = SteamMatchmaking()->GetLobbyData(game->GetCurrentLobby(), "host_steam_id");
            if (hostStr && *hostStr) {
                CSteamID hostID(std::stoull(hostStr));
                nm->queueMessage(msg, NetworkManager::MessagePriority::High, hostID);
            }
        }
    }
}

// Rate-limited enemy updates
void EnemyNetworkHandler::ThrottledSendEnemyUpdates(float dt) {
    lastEnemyUpdateTime += dt;
    if (lastEnemyUpdateTime >= ENEMY_UPDATE_INTERVAL && game->IsHost()) {
        auto& enemies = game->GetEntityManager()->getEnemies();
        for (const auto& [id, enemy] : enemies) {
            if (enemy.needsSync) {
                SendEnemyUpdate(enemy);
            }
        }
        lastEnemyUpdateTime = 0.0f;
    }
}

// Process incoming enemy messages
void EnemyNetworkHandler::HandleEnemyMessage(const std::string& msg, CSteamID sender) {
    if (msg.find("E|SPAWN") == 0) {
        uint64_t enemyID, timestamp;
        float x, y, spawnDelay;
        int health, type;
        int parsed = sscanf(msg.c_str(), "E|SPAWN|%llu|%f|%f|%d|%f|%d|%llu",
                            &enemyID, &x, &y, &health, &spawnDelay, &type, &timestamp);
        if (parsed == 7) {
            EntityUpdate update;
            update.type = EntityUpdate::Type::Spawn;
            update.id = enemyID;
            update.x = x;
            update.y = y;
            update.health = health;
            update.spawnDelay = spawnDelay;
            update.enemyType = static_cast<Enemy::Type>(type);
            update.timestamp = timestamp;
            game->GetEntityManager()->queueUpdate(update);
        }
    } else if (msg.find("E|UPDATE") == 0) {
        uint64_t enemyID, timestamp;
        float x, y, spawnDelay;
        int health;
        if (sscanf(msg.c_str(), "E|UPDATE|%llu|%f|%f|%d|%f|%llu", &enemyID, &x, &y, &health, &spawnDelay, &timestamp) == 6) {
            EntityUpdate update;
            update.type = EntityUpdate::Type::Update;
            update.id = enemyID;
            update.x = x;
            update.y = y;
            update.health = health;
            update.spawnDelay = spawnDelay;
            update.timestamp = timestamp;
            game->GetEntityManager()->queueUpdate(update);
        }
    } else if (msg.find("E|DEATH") == 0) {
        uint64_t enemyID, timestamp, killerID;
        if (sscanf(msg.c_str(), "E|DEATH|%llu|%llu|%llu", &enemyID, &timestamp, &killerID) == 3) {
            game->GetEntityManager()->getEnemies().erase(enemyID);
        }
    } else if (msg.find("E|REMOVE") == 0) {
        uint64_t enemyID;
        if (sscanf(msg.c_str(), "E|REMOVE|%llu", &enemyID) == 1) {
            EntityUpdate update;
            update.type = EntityUpdate::Type::Remove;
            update.id = enemyID;
            game->GetEntityManager()->queueUpdate(update);
        }
    } else {
        std::cout << "[EnemyNetworkHandler] Unhandled enemy message: " << msg << std::endl;
    }

    // If host, relay to other clients (except sender)
    if (game->IsHost() && sender != game->GetLocalPlayer().steamID) {
        NetworkManager* nm = game->GetNetworkManager();
        nm->queueMessage(msg, NetworkManager::MessagePriority::Medium);
    }
}