#include "NetworkManager.h"
#include "../Core/CubeGame.h"
#include "../States/GameplayState.h"
#include <steam/steam_api.h>
#include <iostream>
#include <cstring>
#include <sstream>
#include <vector>
#include <chrono>



NetworkManager::NetworkManager(bool debugMode, CubeGame* gameInstance)
    : debugMode(debugMode), m_networking(nullptr), isConnectedToHost(false), game(gameInstance),
      m_cbLobbyCreated(this, &NetworkManager::OnLobbyCreated),
      m_cbGameLobbyJoinRequested(this, &NetworkManager::OnGameLobbyJoinRequested),
      m_cbLobbyEnter(this, &NetworkManager::OnLobbyEnter),
      m_cbP2PSessionRequest(this, &NetworkManager::OnP2PSessionRequest),
      m_cbP2PSessionConnectFail(this, &NetworkManager::OnP2PSessionConnectFail),
      m_cbLobbyMatchList(this, &NetworkManager::OnLobbyMatchList)
{
    if (!debugMode) {
        if (!SteamAPI_Init()) {
            std::cerr << "[NetworkManager] Steam API initialization failed!" << std::endl;
            std::exit(1);
        }
        m_networking = SteamNetworking();
        if (!m_networking) {
            std::cerr << "[NetworkManager] Could not get SteamNetworking interface." << std::endl;
            std::exit(1);
        }
    } else {
        std::cout << "[NetworkManager] Running in debug mode, Steam not initialized." << std::endl;
    }
    
    setMessageHandler([this](const std::string &msg, CSteamID sender) {
        this->ProcessNetworkMessages(msg, sender);
    });

    usageClock.restart();
}

NetworkManager::~NetworkManager() {
    SteamAPI_Shutdown();
}

bool NetworkManager::isInitialized() const {
    return debugMode || (SteamUser() && SteamUser()->BLoggedOn());
}

bool NetworkManager::sendMessage(CSteamID target, const std::string &msg) {
    if (!m_networking || !SteamUser()) return false;
    uint32 msgSize = static_cast<uint32>(msg.size() + 1);
    if (m_networking->SendP2PPacket(target, msg.c_str(), msgSize, k_EP2PSendReliable)) {
        std::string msgType = msg.substr(0, msg.find('|'));
        if (msgType.empty()) msgType = msg;
        networkUsage[msgType].bytesSent += msgSize;
        networkUsage[msgType].messageCountSent++;
        return true;
    }
    return false;
}

bool NetworkManager::broadcastMessage(const std::string &msg) {
    bool success = true;
    for (const auto &client : m_connectedClients) {
        if (!sendMessage(client.first, msg)) {
            success = false;
        }
    }
    return success;
}

void NetworkManager::processCallbacks() {
    SteamAPI_RunCallbacks();

    static sf::Clock frameClock;
    float dt = frameClock.restart().asSeconds();

    for (auto& [id, player] : game->entityManager->getPlayers()) {
        if (id != game->localSteamID) { // Only interpolate remote players
            if (player.interpolationTime > 0.0f) {
                float t = std::min(1.0f, dt / Player::INTERPOLATION_TIME);
                player.renderedX = player.lastX + (player.targetX - player.lastX) * t;
                player.renderedY = player.lastY + (player.targetY - player.lastY) * t;
                player.interpolationTime -= dt;


                if (player.interpolationTime <= 0.0f) {
                    player.renderedX = player.targetX;
                    player.renderedY = player.targetY;
                    player.lastX = player.renderedX;
                    player.lastY = player.renderedY;
                    player.interpolationTime = 0.0f;
                }
                player.shape.setPosition(player.renderedX, player.renderedY);
                player.orbitingCube.renderedX = player.renderedX + player.orbitingCube.radius * std::cos(player.orbitingCube.angle);
                player.orbitingCube.renderedY = player.renderedY + player.orbitingCube.radius * std::sin(player.orbitingCube.angle);
                player.orbitingCube.shape.setPosition(player.orbitingCube.renderedX, player.orbitingCube.renderedY);
            } else if (player.renderedX != player.targetX || player.renderedY != player.targetY) {
                // Snap to target if no interpolation is active but positions differ
                
                player.renderedX = player.targetX;
                player.renderedY = player.targetY;
                player.lastX = player.renderedX;
                player.lastY = player.renderedY;
                player.shape.setPosition(player.renderedX, player.renderedY);
            }
        }
    }

    if (usageClock.getElapsedTime().asSeconds() >= usageReportInterval) {
        ResetNetworkUsage();
        usageClock.restart();
    }
}

void NetworkManager::receiveMessages() {
    if (!m_networking || !SteamUser()) return;
    
    uint32 msgSize;
    while (m_networking->IsP2PPacketAvailable(&msgSize)) {
        char buffer[1024];
        CSteamID sender;
        if (msgSize > sizeof(buffer) - 1) continue;
        
        if (m_networking->ReadP2PPacket(buffer, sizeof(buffer), &msgSize, &sender)) {
            buffer[msgSize] = '\0';
            std::string msg(buffer);
            
            if (m_connectedClients.find(sender) == m_connectedClients.end()) {
                acceptSession(sender);
                m_connectedClients[sender] = true;
            }
            
            if (!game->m_isHost) {
                const char* hostStr = SteamMatchmaking()->GetLobbyData(game->m_currentLobby, "host_steam_id");
                if (hostStr && *hostStr && CSteamID(std::stoull(hostStr)) == sender) {
                    isConnectedToHost = true;
                }
            }
            
            std::string msgType = msg.substr(0, msg.find('|'));
            if (msgType.empty()) msgType = msg;
            networkUsage[msgType].bytesReceived += msgSize;
            networkUsage[msgType].messageCountReceived++;
            if (messageHandler) {
                messageHandler(msg, sender);
            }
        }
    }
}

void NetworkManager::setMessageHandler(std::function<void(const std::string&, CSteamID)> handler) {
    messageHandler = handler;
}

void NetworkManager::acceptSession(CSteamID remoteID) {
    if (m_networking && m_networking->AcceptP2PSessionWithUser(remoteID)) {
        if (game->m_isHost) {
            isConnectedToHost = true;
        }
    }
}

const std::unordered_map<CSteamID, bool, CSteamIDHash>& NetworkManager::getConnectedClients() const {
    return m_connectedClients;
}

bool NetworkManager::AcceptP2PSessionWithUser(CSteamID user) {
    if (!m_networking) return false;
    return m_networking->AcceptP2PSessionWithUser(user);
}

bool NetworkManager::isLoaded() {
    return isConnectedToHost;
}

void NetworkManager::setIsConnectedToHost(bool b) {
    isConnectedToHost = b;
}

void NetworkManager::ProcessNetworkMessages(const std::string& msg, CSteamID sender) {
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
    else {
        std::cout << "[NetworkManager] Unhandled message: " << msg << std::endl;
    }
}

void NetworkManager::HandlePlayerLoaded(const std::string& msg) {
    uint64_t steamID;
    if (sscanf(msg.c_str(), "PLAYER_LOADED|%llu", &steamID) == 1) {
        game->playerLoadedStatus[CSteamID(steamID)] = true;
    }
}
void NetworkManager::HandlePlayerUpdate(const std::string& msg) {
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
void NetworkManager::HandleEnemySpawn(const std::string& msg) {
    uint64_t enemyID, timestamp;
    float x, y, spawnDelay;
    int health, type;
    int parsed = sscanf(msg.c_str(), "E|SPAWN|%llu|%f|%f|%d|%f|%d|%llu",
                        &enemyID, &x, &y, &health, &spawnDelay, &type, &timestamp);
    if (parsed == 7) {
        if (!m_lastEnemyUpdateTime.count(enemyID) || m_lastEnemyUpdateTime[enemyID] < timestamp) {
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
            m_lastEnemyUpdateTime[enemyID] = timestamp;
        }
    }
}

void NetworkManager::HandleEnemyUpdate(const std::string& msg) {
    uint64_t enemyID, timestamp;
    float x, y, spawnDelay;
    int health;
    if (sscanf(msg.c_str(), "E|UPDATE|%llu|%f|%f|%d|%f|%llu", &enemyID, &x, &y, &health, &spawnDelay, &timestamp) == 6) {
        if (!m_lastEnemyUpdateTime.count(enemyID) || m_lastEnemyUpdateTime[enemyID] < timestamp) {
            EntityUpdate update;
            update.type = EntityUpdate::Type::Update;
            update.id = enemyID;
            update.x = x;
            update.y = y;
            update.health = health;
            update.spawnDelay = spawnDelay;
            update.timestamp = timestamp;
            game->entityManager->queueUpdate(update);
            m_lastEnemyUpdateTime[enemyID] = timestamp;
        }
    }
}

void NetworkManager::HandleEnemyRemove(const std::string& msg) {
    uint64_t enemyID;
    if (sscanf(msg.c_str(), "E|REMOVE|%llu", &enemyID) == 1) {
        EntityUpdate update;
        update.type = EntityUpdate::Type::Remove;
        update.id = enemyID;
        game->entityManager->queueUpdate(update);
    }
}
void NetworkManager::HandleEnemyDeath(const std::string& msg) {
    uint64_t enemyID, timestamp, killerID;
    if (sscanf(msg.c_str(), "E|DEATH|%llu|%llu|%llu", &enemyID, &timestamp, &killerID) == 3) {
        if (!m_lastEnemyUpdateTime.count(enemyID) || m_lastEnemyUpdateTime[enemyID] < timestamp) {
            game->entityManager->getEnemies().erase(enemyID);
            m_lastEnemyUpdateTime[enemyID] = timestamp;
           
        }
    }
}

void NetworkManager::HandleBulletFire(const std::string& msg, CSteamID sender) {
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

void NetworkManager::HandleHit(const std::string& msg, CSteamID sender) {
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
void NetworkManager::HandleStart(const std::string& msg) {
    if (game->currentState != GameState::Playing) {
        game->StartGame();
        game->currentState = GameState::Playing;
    }
}

void NetworkManager::HandleNextLevel(const std::string& msg) {
    float duration;
    if (sscanf(msg.c_str(), "S|NEXT|%f", &duration) == 1) {
        auto gameplayState = game->GetGameplayState();
        if (gameplayState) {
            gameplayState->StartNextLevelTimer(duration);
        }
    }
}

void NetworkManager::HandleTimer(const std::string& msg) {
    float timerValue;
    if (sscanf(msg.c_str(), "S|TIMER|%f", &timerValue) == 1) {
        auto gameplayState = game->GetGameplayState();
        if (gameplayState) {
            gameplayState->nextLevelTimer = timerValue;
            gameplayState->timerActive = (timerValue > 0);
        }
    }
}

void NetworkManager::HandlePlay(const std::string& msg) {
    game->currentState = GameState::Playing;
}

void NetworkManager::HandleGameOver(const std::string& msg) {
    game->currentState = GameState::GameOver;
}

void NetworkManager::HandleLobbyReturn(const std::string& msg) {
    game->ReturnToLobby();
}

void NetworkManager::ReportNetworkUsage() const {
    std::cout << "\n[Network Usage Report] Period: " << usageReportInterval << " seconds\n";
    std::cout << "--------------------------------------------------\n";
    std::cout << "Message Type | Bytes Sent | Bytes Received | Sent Count | Received Count\n";
    std::cout << "--------------------------------------------------\n";

    size_t totalBytesSent = 0, totalBytesReceived = 0;
    size_t totalSentCount = 0, totalReceivedCount = 0;

    for (const auto& [msgType, stats] : networkUsage) {
        std::cout << msgType << " | "
                  << stats.bytesSent << " | "
                  << stats.bytesReceived << " | "
                  << stats.messageCountSent << " | "
                  << stats.messageCountReceived << "\n";
        totalBytesSent += stats.bytesSent;
        totalBytesReceived += stats.bytesReceived;
        totalSentCount += stats.messageCountSent;
        totalReceivedCount += stats.messageCountReceived;
    }

    std::cout << "--------------------------------------------------\n";
    std::cout << "Total | " << totalBytesSent << " | " << totalBytesReceived << " | "
              << totalSentCount << " | " << totalReceivedCount << "\n";
    std::cout << "Bandwidth (KB/s): Sent = " << (totalBytesSent / 1024.0f) / usageReportInterval
              << ", Received = " << (totalBytesReceived / 1024.0f) / usageReportInterval << "\n";
}

void NetworkManager::ResetNetworkUsage() {
    networkUsage.clear();
}

void NetworkManager::SendPlayerUpdate() {
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

void NetworkManager::SendGameplayMessage(const std::string& msg) {
    if (game->m_isHost) {
        broadcastMessage(msg);
    } else {
        const char* hostStr = SteamMatchmaking()->GetLobbyData(game->m_currentLobby, "host_steam_id");
        if (hostStr && *hostStr) {
            CSteamID hostID(std::stoull(hostStr));
            sendMessage(hostID, msg);
        }
    }
}

void NetworkManager::ThrottledSendPlayerUpdate() {
    const float playerUpdateRate = 0.033f; // Send updates every 0.1 seconds
    if (m_playerUpdateClock.getElapsedTime().asSeconds() >= playerUpdateRate) {
        SendPlayerUpdate(); // Send update regardless of movement
        m_playerUpdateClock.restart();
    }
}

void NetworkManager::SpawnEnemiesAndBroadcast() {
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
void NetworkManager::SyncEnemies() {
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

void NetworkManager::SyncEnemiesFull() {
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

void NetworkManager::BroadcastEnemyDeath(uint64_t enemyId, CSteamID killerID) {
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

void NetworkManager::OnLobbyCreated(LobbyCreated_t* pParam) {
    if (pParam->m_eResult != k_EResultOK) {
        std::cerr << "[LOBBY] Failed to create lobby. EResult=" << pParam->m_eResult << "\n";
        game->currentState = GameState::MainMenu;
        return;
    }
    game->m_currentLobby = CSteamID(pParam->m_ulSteamIDLobby);
    game->inLobby = true;
    game->currentState = GameState::Lobby;
    SteamMatchmaking()->SetLobbyData(game->m_currentLobby, "name", game->lobbyNameInput.c_str());
    SteamMatchmaking()->SetLobbyData(game->m_currentLobby, "game_id", CubeGame::GAME_ID);
    CSteamID myID = SteamUser()->GetSteamID();
    std::string hostStr = std::to_string(myID.ConvertToUint64());
    SteamMatchmaking()->SetLobbyData(game->m_currentLobby, "host_steam_id", hostStr.c_str());
    m_connectedClients[myID] = true;
}

void NetworkManager::OnGameLobbyJoinRequested(GameLobbyJoinRequested_t* pParam) {
    JoinLobbyFromNetwork(pParam->m_steamIDLobby);
}

void NetworkManager::JoinLobbyFromNetwork(CSteamID lobby) {
    if (game->inLobby) return;
    game->m_isHost = false;
    SteamAPICall_t call = SteamMatchmaking()->JoinLobby(lobby);
    if (call == k_uAPICallInvalid) {
        std::cerr << "[LOBBY] JoinLobby call failed immediately for " 
                  << lobby.ConvertToUint64() << "\n";
        game->currentState = GameState::MainMenu;
    }
}

void NetworkManager::OnLobbyEnter(LobbyEnter_t* pParam) {
    if (pParam->m_EChatRoomEnterResponse != k_EChatRoomEnterResponseSuccess) {
        std::cerr << "[ERROR] Failed to enter lobby, returning to main menu." << std::endl;
        game->currentState = GameState::MainMenu;
        return;
    }

    game->m_currentLobby = CSteamID(pParam->m_ulSteamIDLobby);
    game->inLobby = true;
    game->currentState = GameState::Lobby;

    game->entityManager->getPlayers().clear();
    game->playerLoadedStatus.clear();
    m_playerStates.clear();
    m_lastEnemyUpdateTime.clear();

    int memberCount = SteamMatchmaking()->GetNumLobbyMembers(game->m_currentLobby);
    for (int i = 0; i < memberCount; i++) {
        CSteamID member = SteamMatchmaking()->GetLobbyMemberByIndex(game->m_currentLobby, i);
        Player newPlayer;
        newPlayer.initialize();
        newPlayer.steamID = member;
        game->entityManager->getPlayers()[member] = newPlayer;
        game->playerLoadedStatus[member] = false;
    }

    if (game->m_isHost) {
        game->playerLoadedStatus[game->localSteamID] = true;
        setIsConnectedToHost(false);
    } else {
        game->playerLoadedStatus[game->localSteamID] = false;
        const char* hostStr = SteamMatchmaking()->GetLobbyData(game->m_currentLobby, "host_steam_id");
        if (hostStr && *hostStr) {
            CSteamID hostID(std::stoull(hostStr));
            if (m_networking->AcceptP2PSessionWithUser(hostID)) {
                setIsConnectedToHost(true);
            }
        }
    }
}

void NetworkManager::OnP2PSessionRequest(P2PSessionRequest_t* pParam) {
    if (game->m_isHost) {
        if (AcceptP2PSessionWithUser(pParam->m_steamIDRemote)) {
            m_connectedClients[pParam->m_steamIDRemote] = true;
            sendMessage(pParam->m_steamIDRemote, "Welcome to the lobby!");
        }
    }
}

void NetworkManager::OnP2PSessionConnectFail(P2PSessionConnectFail_t* pParam) {
    if (!game->m_isHost && pParam->m_steamIDRemote == k_steamIDNil) {
        std::cerr << "[P2P] Lost connection to host. Returning to main menu.\n";
        game->ReturnToMainMenu();
        setIsConnectedToHost(false);
    }
}

void NetworkManager::OnLobbyMatchList(LobbyMatchList_t* pParam) {
    game->lobbyList.clear();
    for (uint32 i = 0; i < pParam->m_nLobbiesMatching; ++i) {
        CSteamID lobbyID = SteamMatchmaking()->GetLobbyByIndex(i);
        const char* lobbyName = SteamMatchmaking()->GetLobbyData(lobbyID, "name");
        if (lobbyName && *lobbyName) {
            game->lobbyList.emplace_back(lobbyID, std::string(lobbyName));
        }
    }
    game->lobbyListUpdated = true;
    game->hud.updateText("searchStatus", "Lobby Search");
}


void NetworkManager::HandleCollisionsAndSync(float dt, CubeGame* game) {
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

void NetworkManager::syncTimer(float timerValue) {
    if (!game->m_isHost) return;
    char buffer[64];
    int bytes = snprintf(buffer, sizeof(buffer), "S|TIMER|%.1f", timerValue);
    if (bytes > 0 && static_cast<size_t>(bytes) < sizeof(buffer)) {
        broadcastMessage(std::string(buffer));
    }
}

void NetworkManager::broadcastGameOver() {
    if (!game->m_isHost) return;
    char buffer[32];
    int bytes = snprintf(buffer, sizeof(buffer), "S|GAMEOVER");
    if (bytes > 0 && static_cast<size_t>(bytes) < sizeof(buffer)) {
        broadcastMessage(std::string(buffer));
    }
}

void NetworkManager::syncLevelTransition(float duration) {
    if (!game->m_isHost) return;
    char buffer[64];
    int bytes = snprintf(buffer, sizeof(buffer), "S|NEXT|%.1f", duration);
    if (bytes > 0 && static_cast<size_t>(bytes) < sizeof(buffer)) {
        broadcastMessage(std::string(buffer));
    }
}

void NetworkManager::syncEntities(EntityManager* em) {
    if (!game->m_isHost) return;

    uint64_t timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();

    for (auto it = em->getEnemies().begin(); it != em->getEnemies().end();) {
        Enemy& enemy = it->second;
        if (enemy.needsSync) {
            if (enemy.health <= 0) {
                char buffer[64];
                int bytes = snprintf(buffer, sizeof(buffer), "E|REMOVE|%llu", enemy.id);
                if (bytes > 0 && static_cast<size_t>(bytes) < sizeof(buffer)) {
                    broadcastMessage(std::string(buffer));
                }
                it = em->getEnemies().erase(it);
            } else {
                char buffer[128];
                int bytes = snprintf(buffer, sizeof(buffer), "E|UPDATE|%llu|%.1f|%.1f|%d|%.2f|%llu",
                                     enemy.id, enemy.x, enemy.y, enemy.health, enemy.spawnDelay, timestamp);
                if (bytes > 0 && static_cast<size_t>(bytes) < sizeof(buffer)) {
                    broadcastMessage(std::string(buffer));
                }
                enemy.lastSentX = enemy.x;
                enemy.lastSentY = enemy.y;
                enemy.needsSync = false;
                ++it;
            }
        } else {
            ++it;
        }
    }
}