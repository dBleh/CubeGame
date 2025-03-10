#include "PlayerNetworkHandler.h"
#include "NetworkManager.h"  // For queueMessage and MessagePriority
#include "CubeGame.h"        // For game state access
#include "Player.h"          // For Player class (assumed)
#include "steam/steam_api.h" // For CSteamID
#include <stdio.h>           // For snprintf
#include <iostream>          // For std::cout

// Define the static constant
const float PlayerNetworkHandler::PLAYER_UPDATE_INTERVAL = 0.003f; // 10 updates/sec

// Constructor
PlayerNetworkHandler::PlayerNetworkHandler(CubeGame* game) : game(game) {}

// Send local player state update
void PlayerNetworkHandler::SendPlayerUpdate() {
    const char* hostStr = SteamMatchmaking()->GetLobbyData(game->GetCurrentLobby(), "host_steam_id");
    CSteamID hostID(std::stoull(hostStr ? hostStr : "0"));
    char buffer[256];
    Player& localPlayer = game->GetLocalPlayer();
    int bytes = snprintf(buffer, sizeof(buffer),
                         "P|D|%llu|x|%.1f|y|%.1f|h|%d|k|%d|m|%d|s|%.1f|a|%d",
                         localPlayer.steamID.ConvertToUint64(), localPlayer.renderedX, localPlayer.renderedY,
                         localPlayer.health, localPlayer.kills, localPlayer.money,
                         localPlayer.speed, localPlayer.isAlive);
    if (bytes > 0 && static_cast<size_t>(bytes) < sizeof(buffer)) {
        std::string msg(buffer);
        NetworkManager* nm = game->GetNetworkManager();
        if (game->IsHost()) {
            nm->queueMessage(msg, NetworkManager::MessagePriority::Critical);
        } else if (hostID.IsValid()) {
            nm->queueMessage(msg, NetworkManager::MessagePriority::Critical, hostID);
        }
    }
}

// Rate-limited player update
void PlayerNetworkHandler::ThrottledSendPlayerUpdate(float dt) {
    lastPlayerUpdateTime += dt;
    if (lastPlayerUpdateTime >= PLAYER_UPDATE_INTERVAL) {
        SendPlayerUpdate();
        lastPlayerUpdateTime = 0.0f;
    }
}

// Process incoming player messages
void PlayerNetworkHandler::HandlePlayerUpdate(const std::string& msg, CSteamID sender) {
    if (msg.find("P|D|") == 0) {
        uint64_t steamID;
        float x, y, speed;
        int health, kills, money;
        int isAlive;

        if (sscanf(msg.c_str(), "P|D|%llu|x|%f|y|%f|h|%d|k|%d|m|%d|s|%f|a|%d",
                   &steamID, &x, &y, &health, &kills, &money, &speed, &isAlive) == 8) {
            CSteamID id(steamID);
            auto& players = game->GetEntityManager()->getPlayers();
            if (players.count(id)) {
                Player& p = players[id];
                // Set last positions from current rendered positions
                p.lastX = p.renderedX;
                p.lastY = p.renderedY;
                // Update target positions and state
                p.x = x;  // Target position from network (renderedX sent by sender)
                p.y = y;  // Target position from network (renderedY sent by sender)
                p.health = health;
                p.kills = kills;
                p.money = money;
                p.speed = speed;
                p.isAlive = isAlive != 0;
                p.interpolationTime = INTERPOLATION_TIME; // Reset interpolation timer

                // If host, relay to other clients (except sender and self)
                if (game->IsHost() && id != game->GetLocalPlayer().steamID) {
                    SendPlayerUpdate();
                }
            }
        }
    } else if (msg.find("PLAYER_LOADED|") == 0) {
        uint64_t steamID;
        if (sscanf(msg.c_str(), "PLAYER_LOADED|%llu", &steamID) == 1) {
            CSteamID id(steamID);
            auto& players = game->GetEntityManager()->getPlayers();
            if (players.count(id)) {
                game->playerLoadedStatus[id] = true; // Use CubeGame's map instead of Player::loaded
                std::cout << "Player " << steamID << " loaded\n";
            }
        }
    }
}