#include "PlayerNetworkHandler.h"
#include "NetworkManager.h"
#include "../Core/CubeGame.h"
#include <vector>
#include <cmath>

PlayerNetworkHandler::PlayerNetworkHandler(CubeGame* gameInstance, NetworkManager* networkMgr)
    : game(gameInstance), networkManager(networkMgr) {
}

void PlayerNetworkHandler::HandlePlayerLoaded(const std::string& msg) {
    uint64_t steamID;
    if (sscanf(msg.c_str(), "PLAYER_LOADED|%llu", &steamID) == 1) {
        game->playerLoadedStatus[CSteamID(steamID)] = true;
    }
}

void PlayerNetworkHandler::HandlePlayerUpdate(const std::string& msg) {
    std::vector<std::string> parts;
    std::stringstream ss(msg);
    std::string part;
    while (std::getline(ss, part, '|')) parts.push_back(part);

    if (parts.empty() || parts[0] != "P") return;

    bool isKeyValue = (parts.size() > 1 && parts[1] == "D");
    size_t minParts = isKeyValue ? 3 : 12;
    if (parts.size() < minParts) return;

    size_t idIndex = isKeyValue ? 2 : 1;
    CSteamID id(std::stoull(parts[idIndex]));

    if (game->entityManager->getPlayers().count(id) == 0) {
        Player newPlayer;
        newPlayer.initialize();
        newPlayer.steamID = id;
        game->entityManager->getPlayers()[id] = newPlayer;
    }
    Player& p = game->entityManager->getPlayers()[id];

    if (id == game->localSteamID) {
        if (isKeyValue) {
            size_t i = 3;
            while (i + 1 < parts.size()) {
                if (parts[i] == "x") p.x = std::stof(parts[i + 1]);
                else if (parts[i] == "y") p.y = std::stof(parts[i + 1]);
                else if (parts[i] == "h") p.health = std::stoi(parts[i + 1]);
                else if (parts[i] == "a") p.isAlive = std::stoi(parts[i + 1]) != 0;
                else if (parts[i] == "k") p.kills = std::stoi(parts[i + 1]);
                else if (parts[i] == "m") p.money = std::stoi(parts[i + 1]);
                else if (parts[i] == "t") p.lastUpdateTimestamp = std::stoull(parts[i + 1]);
                i += 2;
            }
            game->GetLocalPlayer() = p;
        }
    } else {
        if (!isKeyValue) {
            p.lastX = p.x;
            p.lastY = p.y;
            p.x = std::stof(parts[2]);
            p.y = std::stof(parts[3]);
            p.renderedX = std::stof(parts[4]);
            p.renderedY = std::stof(parts[5]);
            p.health = std::stoi(parts[6]);
            p.kills = std::stoi(parts[7]);
            p.ready = std::stoi(parts[8]) != 0;
            p.money = std::stoi(parts[9]);
            p.speed = std::stof(parts[10]);
            p.isAlive = std::stoi(parts[11]) != 0;
        } else {
            float receivedAngle = p.orbitingCube.angle;
            uint64_t receivedStartTimestamp = p.startTimestamp;
            uint64_t receivedTimestamp = p.lastUpdateTimestamp;
            size_t i = 3;
            while (i + 1 < parts.size()) {
                if (parts[i] == "x") p.x = std::stof(parts[i + 1]);
                else if (parts[i] == "y") p.y = std::stof(parts[i + 1]);
                else if (parts[i] == "rx") p.renderedX = std::stof(parts[i + 1]);
                else if (parts[i] == "ry") p.renderedY = std::stof(parts[i + 1]);
                else if (parts[i] == "h") p.health = std::stoi(parts[i + 1]);
                else if (parts[i] == "a") p.isAlive = std::stoi(parts[i + 1]) != 0;
                else if (parts[i] == "k") p.kills = std::stoi(parts[i + 1]);
                else if (parts[i] == "m") p.money = std::stoi(parts[i + 1]);
                else if (parts[i] == "r") p.ready = std::stoi(parts[i + 1]) != 0;
                else if (parts[i] == "s") p.speed = std::stof(parts[i + 1]);
                else if (parts[i] == "oca") receivedAngle = std::stof(parts[i + 1]);
                else if (parts[i] == "st") receivedStartTimestamp = std::stoull(parts[i + 1]);
                else if (parts[i] == "t") receivedTimestamp = std::stoull(parts[i + 1]);
                i += 2;
            }

            if (receivedTimestamp > p.lastUpdateTimestamp) {
                p.lastUpdateTimestamp = receivedTimestamp;
                p.startTimestamp = receivedStartTimestamp;
                p.orbitingCube.angle = receivedAngle;
            }

            uint64_t now = std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::system_clock::now().time_since_epoch()).count();
            float elapsedTime = (now - p.startTimestamp) / 1000.0f;
            p.orbitingCube.angle = p.orbitingCube.angularSpeed * elapsedTime;
            p.orbitingCube.angle = std::fmod(p.orbitingCube.angle, 2 * M_PI);

            p.orbitingCube.x = p.x + p.orbitingCube.radius * std::cos(p.orbitingCube.angle);
            p.orbitingCube.y = p.y + p.orbitingCube.radius * std::sin(p.orbitingCube.angle);
            p.lastX = p.x;
            p.lastY = p.y;
        }
        p.shape.setPosition(p.renderedX, p.renderedY);
        p.orbitingCube.shape.setPosition(p.orbitingCube.x, p.orbitingCube.y);
    }
}

void PlayerNetworkHandler::SendPlayerUpdate() {
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
        << "|oca|" << p.orbitingCube.angle
        << "|st|" << p.startTimestamp
        << "|t|" << timestamp;
    
    std::string msg = oss.str();
    if (game->IsHost()) {
        networkManager->broadcastMessage(msg);
    } else {
        const char* hostStr = SteamMatchmaking()->GetLobbyData(game->GetCurrentLobby(), "host_steam_id");
        if (hostStr && *hostStr) {
            CSteamID hostID(std::stoull(hostStr));
            networkManager->sendMessage(hostID, msg);
        }
    }

    p.lastUpdateTimestamp = timestamp;
    game->entityManager->getPlayers()[game->localSteamID] = p;
}

void PlayerNetworkHandler::ThrottledSendPlayerUpdate() {
    const float playerUpdateRate = 0.033f; // Send updates every 0.033 seconds (~30Hz)
    if (m_playerUpdateClock.getElapsedTime().asSeconds() >= playerUpdateRate) {
        SendPlayerUpdate();
        m_playerUpdateClock.restart();
    }
}
