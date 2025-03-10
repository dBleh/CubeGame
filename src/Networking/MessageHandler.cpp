#include "MessageHandler.h"
#include <sstream>
#include <vector>
#include <iostream>

MessageHandler::MessageHandler(CubeGame* game, NetworkManager* networkManager)
    : game(game), networkManager(networkManager) {}

void MessageHandler::ProcessNetworkMessages(const std::string& msg, CSteamID sender) {
    if (msg.empty()) return;
    if (msg.find("PLAYER_LOADED") == 0) HandlePlayerLoaded(msg);
    else if (msg[0] == 'P') HandlePlayerUpdate(msg);
    else if (msg.find("E|SPAWN") == 0) HandleEnemySpawn(msg);
    else if (msg.find("E|UPDATE") == 0) HandleEnemyUpdate(msg);
    else if (msg.find("E|DEATH") == 0) HandleEnemyDeath(msg);
    else if (msg.find("B|fire") == 0) HandleBulletFire(msg, sender);
    else if (msg[0] == 'H') HandleHit(msg, sender);
    else if (msg.find("E|REMOVE") == 0) HandleEnemyRemove(msg);
    else if (msg.find("S|START") == 0) HandleStart(msg);
    else if (msg.find("S|NEXT") == 0) HandleNextLevel(msg);
    else if (msg.find("S|TIMER") == 0) HandleTimer(msg);
    else if (msg.find("S|PLAY") == 0) HandlePlay(msg);
    else if (msg.find("S|GAMEOVER") == 0) HandleGameOver(msg);
    else if (msg.find("S|LOBBY") == 0) HandleLobbyReturn(msg);
    else std::cout << "[MessageHandler] Unhandled message: " << msg << std::endl;
}

void MessageHandler::HandlePlayerLoaded(const std::string& msg) {
    uint64_t steamID;
    if (sscanf(msg.c_str(), "PLAYER_LOADED|%llu", &steamID) == 1) {
        game->playerLoadedStatus[CSteamID(steamID)] = true;
    }
}
void MessageHandler::HandlePlayerUpdate(const std::string& msg) {
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

            // Sync start timestamp if newer
            if (receivedTimestamp > p.lastUpdateTimestamp) {
                p.lastUpdateTimestamp = receivedTimestamp;
                p.startTimestamp = receivedStartTimestamp;
                p.orbitingCube.angle = receivedAngle;
            }

            // Calculate current angle based on total elapsed time since start
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
void MessageHandler::HandleEnemySpawn(const std::string& msg) {
    uint64_t enemyID, timestamp;
    float x, y, spawnDelay;
    int health, type;
    int parsed = sscanf(msg.c_str(), "E|SPAWN|%llu|%f|%f|%d|%f|%d|%llu",
                        &enemyID, &x, &y, &health, &spawnDelay, &type, &timestamp);
    if (parsed == 7) {
        if (game->entityManager->getEnemies().count(enemyID) == 0 ||
            (m_lastEnemyUpdateTime.count(enemyID) && m_lastEnemyUpdateTime[enemyID] < timestamp)) {
            auto& newEnemy = game->entityManager->getEnemies().emplace(enemyID, Enemy()).first->second;
            newEnemy.initialize(static_cast<Enemy::Type>(type));
            newEnemy.id = enemyID;
            newEnemy.x = x;
            newEnemy.y = y;
            newEnemy.health = health;
            newEnemy.spawnDelay = spawnDelay;
            newEnemy.renderedX = x;
            newEnemy.renderedY = y;
            newEnemy.lastX = x;
            newEnemy.lastY = y;
            m_lastEnemyUpdateTime[enemyID] = timestamp;
            
        }
    } else {
        std::cout << "[DEBUG] Failed to parse enemy spawn: " << msg << "\n";
    }
}
void MessageHandler::HandleEnemyUpdate(const std::string& msg) {
    uint64_t enemyID, timestamp;
    float x, y, spawnDelay;
    int health;
    if (sscanf(msg.c_str(), "E|UPDATE|%llu|%f|%f|%d|%f|%llu", &enemyID, &x, &y, &health, &spawnDelay, &timestamp) == 6) {
        if (game->entityManager->getEnemies().count(enemyID) > 0) {
            if (!m_lastEnemyUpdateTime.count(enemyID) || m_lastEnemyUpdateTime[enemyID] < timestamp) {
                Enemy& e = game->entityManager->getEnemies()[enemyID];
                e.lastX = e.renderedX;
                e.lastY = e.renderedY;
                e.x = x;
                e.y = y;
                e.health = health;
                e.spawnDelay = spawnDelay;
                e.interpolationTime = INTERPOLATION_TIME;
                m_lastEnemyUpdateTime[enemyID] = timestamp;
            }
        }
    }
}
void MessageHandler::HandleEnemyDeath(const std::string& msg) {
    uint64_t enemyID, timestamp, killerID;
    if (sscanf(msg.c_str(), "E|DEATH|%llu|%llu|%llu", &enemyID, &timestamp, &killerID) == 3) {
        if (!m_lastEnemyUpdateTime.count(enemyID) || m_lastEnemyUpdateTime[enemyID] < timestamp) {
            game->entityManager->getEnemies().erase(enemyID);
            m_lastEnemyUpdateTime[enemyID] = timestamp;
           
        }
    }
}
void MessageHandler::HandleBulletFire(const std::string& msg, CSteamID sender) {
    uint32_t messageID;
    uint64_t shooterSteamID;
    int bulletIdx;
    float startX, startY, targetX, targetY, lifetime;
    int parsed = sscanf(msg.c_str(), "B|fire|%u|%llu|%d|%f|%f|%f|%f|%f", &messageID, &shooterSteamID, &bulletIdx,
                        &startX, &startY, &targetX, &targetY, &lifetime);
    if (parsed == 8) {
        if (game->processedBulletMessages.find(messageID) != game->processedBulletMessages.end()) {
            return;
        }
        game->processedBulletMessages.insert(messageID);

        uint64_t uniqueBulletId = (shooterSteamID << 32) | static_cast<uint32_t>(bulletIdx);
        if (game->entityManager->getBullets().find(uniqueBulletId) == game->entityManager->getBullets().end()) {
            Bullet newBullet;
            newBullet.initialize(startX, startY, targetX, targetY);
            newBullet.id = uniqueBulletId;
            newBullet.lifetime = lifetime;
            newBullet.renderedX = startX;
            newBullet.renderedY = startY;
            game->entityManager->getBullets()[uniqueBulletId] = newBullet;
        }
        if (game->m_isHost) {
            broadcastMessage(msg);
        }
    }
}
void MessageHandler::HandleHit(const std::string& msg, CSteamID sender) {
    uint64_t bulletId, enemyId, shooterSteamID, timestamp;
    int damage;
    if (sscanf(msg.c_str(), "H|%llu|%llu|%llu|%d|%llu", &bulletId, &enemyId, &shooterSteamID, &damage, &timestamp) != 5) {

        return;
    }
    if (game->m_isHost) {
        
        if (game->entityManager->getEnemies().count(enemyId)) {
            Enemy& e = game->entityManager->getEnemies()[enemyId];
            // Only process if this is a new hit or a later timestamp
            if (!m_lastEnemyUpdateTime.count(enemyId) || m_lastEnemyUpdateTime[enemyId] < timestamp) {
                e.health -= damage;
                m_lastEnemyUpdateTime[enemyId] = timestamp;
                if (e.health <= 0) {
                    CSteamID shooterID(shooterSteamID);
                    if (game->entityManager->getPlayers().count(shooterID)) {
                        Player& shooter = game->entityManager->getPlayers()[shooterID];
                       
                        shooter.kills += 1;
                        shooter.money += 10;

                        // Save updated player back to the map
                        game->entityManager->getPlayers()[shooterID] = shooter;

                        // Broadcast kill/money update
                        char buffer[128];
                        int bytes = snprintf(buffer, sizeof(buffer), "P|D|%llu|k|%d|m|%d",
                                             shooterSteamID, shooter.kills, shooter.money);
                        if (bytes > 0 && static_cast<size_t>(bytes) < sizeof(buffer)) {
                            broadcastMessage(std::string(buffer));
                            
                        }

                        // Broadcast enemy removal
                        bytes = snprintf(buffer, sizeof(buffer), "E|REMOVE|%llu", enemyId);
                        if (bytes > 0 && static_cast<size_t>(bytes) < sizeof(buffer)) {
                            broadcastMessage(std::string(buffer));
                        }

                        // Remove enemy and clear timestamp to allow new enemies with same ID
                        game->entityManager->getEnemies().erase(enemyId);
                        m_lastEnemyUpdateTime.erase(enemyId);
                        
                    } 
                }
            } 
        }
    }
}
void MessageHandler::HandleEnemyRemove(const std::string& msg) {
    uint64_t enemyID;
    if (sscanf(msg.c_str(), "E|REMOVE|%llu", &enemyID) == 1) {
        if (game->entityManager->getEnemies().count(enemyID)) {
            game->entityManager->getEnemies().erase(enemyID);
    
        }
    }
}
void MessageHandler::HandleStart(const std::string& msg) {
    if (game->currentState != GameState::Playing) {
        game->StartGame();
        game->currentState = GameState::Playing;
    }
}
void MessageHandler::HandleNextLevel(const std::string& msg) {
    float duration;
    if (sscanf(msg.c_str(), "S|NEXT|%f", &duration) == 1) {
        auto gameplayState = game->GetGameplayState();
        if (gameplayState) {
            gameplayState->StartNextLevelTimer(duration);
        }
    }
}
void MessageHandler::HandleTimer(const std::string& msg) {
    float timerValue;
    if (sscanf(msg.c_str(), "S|TIMER|%f", &timerValue) == 1) {
        auto gameplayState = game->GetGameplayState();
        if (gameplayState) {
            gameplayState->nextLevelTimer = timerValue;
            gameplayState->timerActive = (timerValue > 0);
        }
    }
}
void MessageHandler::HandlePlay(const std::string& msg) {
    game->currentState = GameState::Playing;
}

void MessageHandler::HandleGameOver(const std::string& msg) {
    game->currentState = GameState::GameOver;
}

void MessageHandler::HandleLobbyReturn(const std::string& msg) {
    game->ReturnToLobby();
}