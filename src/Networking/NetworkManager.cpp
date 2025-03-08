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
    
    for (auto& [id, state] : m_playerStates) {
        if (game->entityManager->getPlayers().count(id) > 0) {
            Player& p = game->entityManager->getPlayers()[id];
            if (id != game->localSteamID) {
                float t = state.interpolationClock.getElapsedTime().asSeconds() / INTERPOLATION_TIME;
                if (t <= 1.0f) {
                    p.renderedX = state.lastX + (state.targetX - state.lastX) * t;
                    p.renderedY = state.lastY + (state.targetY - state.lastY) * t;
                    p.shape.setPosition(p.renderedX, p.renderedY);
                }
            }
        }
    }

    if (usageClock.getElapsedTime().asSeconds() >= usageReportInterval) {
        // ReportNetworkUsage();
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

    if (parts[0] != "P" || parts.size() < 3) return;

    CSteamID id;
    size_t startIdx = 1;
    if (parts[1] == "D") {
        id = CSteamID(std::stoull(parts[2]));
        startIdx = 3;
    } else {
        id = CSteamID(std::stoull(parts[1]));
        startIdx = 2;
    }

    if (game->entityManager->getPlayers().count(id) == 0) {
        Player newPlayer;
        newPlayer.initialize();
        newPlayer.steamID = id;
        game->entityManager->getPlayers()[id] = newPlayer;
    }
    Player& p = game->entityManager->getPlayers()[id];

    if (id != game->localSteamID) {
        PlayerState& state = m_playerStates[id];
        state.lastX = p.renderedX;
        state.lastY = p.renderedY;
        state.interpolationClock.restart();
    }

    for (size_t i = startIdx; i < parts.size(); i += (startIdx == 2 ? 1 : 2)) {
        if (startIdx == 2) { // Full format: "P|<steamID>|x|...|k|<kills>|..."
            if (i == 1) continue; // Skip steamID
            if (i == 2) p.x = std::stof(parts[i]);
            else if (i == 3) p.y = std::stof(parts[i]);
            else if (i == 4) p.renderedX = std::stof(parts[i]);
            else if (i == 5) p.renderedY = std::stof(parts[i]);
            else if (i == 6) p.health = std::stoi(parts[i]);
            else if (i == 7) p.kills = std::stoi(parts[i]);
            else if (i == 8) p.ready = std::stoi(parts[i]) != 0;
            else if (i == 9) p.money = std::stoi(parts[i]);
            else if (i == 10) p.speed = std::stof(parts[i]);
            else if (i == 11) p.isAlive = std::stoi(parts[i]) != 0;
        } else { // Key-value format: "P|D|<steamID>|k|<kills>|m|<money>|..."
            if (i + 1 >= parts.size()) break;
            if (parts[i] == "x") p.x = std::stof(parts[i + 1]);
            else if (parts[i] == "y") p.y = std::stof(parts[i + 1]);
            else if (parts[i] == "rx") {
                p.renderedX = std::stof(parts[i + 1]);
                if (id != game->localSteamID) m_playerStates[id].targetX = p.renderedX;
            }
            else if (parts[i] == "ry") {
                p.renderedY = std::stof(parts[i + 1]);
                if (id != game->localSteamID) m_playerStates[id].targetY = p.renderedY;
            }
            else if (parts[i] == "h") p.health = std::stoi(parts[i + 1]);
            else if (parts[i] == "k") p.kills = std::stoi(parts[i + 1]);
            else if (parts[i] == "r") p.ready = std::stoi(parts[i + 1]) != 0;
            else if (parts[i] == "m") p.money = std::stoi(parts[i + 1]);
            else if (parts[i] == "s") p.speed = std::stof(parts[i + 1]);
            else if (parts[i] == "a") p.isAlive = std::stoi(parts[i + 1]) != 0;
        }
    }
}
void NetworkManager::HandleEnemySpawn(const std::string& msg) {
    uint64_t enemyID, timestamp;
    float x, y, spawnDelay;
    int health, type;
    int parsed = sscanf(msg.c_str(), "E|SPAWN|%llu|%f|%f|%d|%f|%d|%llu", 
                        &enemyID, &x, &y, &health, &spawnDelay, &type, &timestamp);
    if (parsed == 7) { // Now expecting 7 parameters
        if (game->entityManager->getEnemies().count(enemyID) == 0 || 
            (m_lastEnemyUpdateTime.count(enemyID) && m_lastEnemyUpdateTime[enemyID] < timestamp)) {
            auto& newEnemy = game->entityManager->getEnemies().emplace(enemyID, Enemy()).first->second;
            newEnemy.initialize(static_cast<Enemy::Type>(type)); // Use the received type
            newEnemy.id = enemyID;
            newEnemy.x = x;
            newEnemy.y = y;
            newEnemy.health = health;
            newEnemy.spawnDelay = spawnDelay;
            newEnemy.renderedX = x;
            newEnemy.renderedY = y;
            newEnemy.lastSentX = x;
            newEnemy.lastSentY = y;
            newEnemy.interpolationTime = INTERPOLATION_TIME;
            m_lastEnemyUpdateTime[enemyID] = timestamp;
        }
    }
}

void NetworkManager::HandleEnemyUpdate(const std::string& msg) {
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

void NetworkManager::HandleEnemyDeath(const std::string& msg) {
    uint64_t enemyID, timestamp, killerID;
    if (sscanf(msg.c_str(), "E|DEATH|%llu|%llu|%llu", &enemyID, &timestamp, &killerID) == 3) {
        if (!m_lastEnemyUpdateTime.count(enemyID) || m_lastEnemyUpdateTime[enemyID] < timestamp) {
            game->entityManager->getEnemies().erase(enemyID);
            m_lastEnemyUpdateTime[enemyID] = timestamp;
            std::cout << "[DEBUG] Enemy " << enemyID << " marked as dead by killer " << killerID << std::endl;
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

void NetworkManager::HandleEnemyRemove(const std::string& msg) {
    uint64_t enemyID;
    if (sscanf(msg.c_str(), "E|REMOVE|%llu", &enemyID) == 1) {
        if (game->entityManager->getEnemies().count(enemyID)) {
            game->entityManager->getEnemies().erase(enemyID);
            std::cout << "[DEBUG] Removed enemy " << enemyID << " from client" << std::endl;
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
        if (game->entityManager->getEnemies().count(enemyId) > 0) {
            Enemy& e = game->entityManager->getEnemies()[enemyId];
            if (!m_lastEnemyUpdateTime.count(enemyId) || m_lastEnemyUpdateTime[enemyId] < timestamp) {
                e.health -= damage;
                m_lastEnemyUpdateTime[enemyId] = timestamp;
                
                CSteamID shooterID(shooterSteamID);
                if (e.health <= 0) {
                    // Update player stats
                    if (game->entityManager->getPlayers().count(shooterID) > 0) {
                        Player& p = game->entityManager->getPlayers()[shooterID];
                        p.kills++;
                        p.money += 10;
                        // Broadcast updated player state
                        std::string playerUpdateMsg = game->FormatPlayerUpdate(p);
                        if (!playerUpdateMsg.empty()) {
                            broadcastMessage(playerUpdateMsg);
                        }
                    }
                    // Broadcast enemy removal
                    char buffer[64];
                    int bytes = snprintf(buffer, sizeof(buffer), "E|REMOVE|%llu", enemyId);
                    if (bytes > 0 && static_cast<size_t>(bytes) < sizeof(buffer)) {
                        broadcastMessage(std::string(buffer));
                    }
                    game->entityManager->getEnemies().erase(enemyId);
                } else {
                    // Broadcast updated enemy state
                    char buffer[128];
                    int bytes = snprintf(buffer, sizeof(buffer), "E|UPDATE|%llu|%.1f|%.1f|%d|%.2f|%llu",
                                        enemyId, e.x, e.y, e.health, e.spawnDelay, timestamp);
                    if (bytes > 0 && static_cast<size_t>(bytes) < sizeof(buffer)) {
                        broadcastMessage(std::string(buffer));
                    }
                }
                // Remove bullet on hit
                game->entityManager->getBullets().erase(bulletId);
            }
        }
    } else {
        // Client-side: Queue hit if enemy not found
        if (!game->entityManager->getEnemies().count(enemyId)) {
            GameplayState* gameplayState = game->GetGameplayState();
            if (gameplayState) {
                gameplayState->pendingHits.push_back({bulletId, enemyId, shooterSteamID, 0.5f});
                std::cout << "[DEBUG] Client queued hit for enemy " << enemyId << " (not found yet)" << std::endl;
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

    std::ostringstream oss;
    oss << "P|D|" << p.steamID.ConvertToUint64()
        << "|x|" << p.x
        << "|y|" << p.y
        << "|rx|" << p.renderedX
        << "|ry|" << p.renderedY
        << "|h|" << p.health
        << "|k|" << p.kills
        << "|r|" << (p.ready ? 1 : 0)
        << "|m|" << p.money
        << "|s|" << p.speed
        << "|a|" << (p.isAlive ? 1 : 0);

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
    const float playerUpdateRate = 0.016f; // ~62.5 Hz
    if (m_playerUpdateClock.getElapsedTime().asSeconds() >= playerUpdateRate) {
        SendPlayerUpdate();
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