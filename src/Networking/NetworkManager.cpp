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
    playerHandler = new PlayerNetworkHandler(gameInstance); // Initialize handler
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
    delete playerHandler;
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
    uint32 packetSize;
    while (SteamNetworking()->IsP2PPacketAvailable(&packetSize)) {
        std::vector<char> buffer(packetSize);
        CSteamID sender;
        uint32 bytesRead;

        if (SteamNetworking()->ReadP2PPacket(buffer.data(), packetSize, &bytesRead, &sender)) {
            std::string msg(buffer.data(), bytesRead);
            // Handle batched messages
            size_t pos = 0;
            std::string delimiter = "|BATCH|";
            while ((pos = msg.find(delimiter)) != std::string::npos) {
                std::string singleMsg = msg.substr(0, pos);
                ProcessNetworkMessages(singleMsg, sender);
                msg.erase(0, pos + delimiter.length());
            }
            ProcessNetworkMessages(msg, sender); // Process last or only message
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

    if (msg.find("PLAYER_LOADED") == 0) {
        playerHandler->HandlePlayerUpdate(msg, sender); // Delegate
    } else if (msg[0] == 'P') {
        playerHandler->HandlePlayerUpdate(msg, sender); // Delegate
    } else if (msg.find("E|SPAWN") == 0) {
        HandleEnemySpawn(msg);
    } else if (msg.find("E|UPDATE") == 0) {
        HandleEnemyUpdate(msg);
    } else if (msg.find("E|DEATH") == 0) {
        HandleEnemyDeath(msg);
    } else if (msg.find("B|fire") == 0) {
        HandleBulletFire(msg, sender);
    } else if (msg[0] == 'H') {
        HandleHit(msg, sender);
    } else if (msg.find("E|REMOVE") == 0) {
        HandleEnemyRemove(msg);
    } else if (msg.find("S|START") == 0) {
        HandleStart(msg);
    } else if (msg.find("S|NEXT") == 0) {
        HandleNextLevel(msg);
    } else if (msg.find("S|TIMER") == 0) {
        HandleTimer(msg);
    } else if (msg.find("S|PLAY") == 0) {
        HandlePlay(msg);
    } else if (msg.find("S|GAMEOVER") == 0) {
        HandleGameOver(msg);
    } else if (msg.find("S|LOBBY") == 0) {
        HandleLobbyReturn(msg);
    } else {
        std::cout << "[NetworkManager] Unhandled message: " << msg << std::endl;
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

void NetworkManager::HandleEnemyRemove(const std::string& msg) {
    uint64_t enemyID;
    if (sscanf(msg.c_str(), "E|REMOVE|%llu", &enemyID) == 1) {
        EntityUpdate update;
        update.type = EntityUpdate::Type::Remove;
        update.id = enemyID;
        game->entityManager->queueUpdate(update);
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
        queueMessage(std::string(buffer), MessagePriority::Critical);
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
                    queueMessage(std::string(buffer), MessagePriority::Medium);
                }
                it = em->getEnemies().erase(it);
            } else {
                char buffer[128];
                int bytes = snprintf(buffer, sizeof(buffer), "E|UPDATE|%llu|%.1f|%.1f|%d|%.2f|%llu",
                                     enemy.id, enemy.x, enemy.y, enemy.health, enemy.spawnDelay, timestamp);
                if (bytes > 0 && static_cast<size_t>(bytes) < sizeof(buffer)) {
                    queueMessage(std::string(buffer), MessagePriority::Medium);
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

void NetworkManager::queueMessage(const std::string& content, MessagePriority priority, CSteamID target) {
    uint64_t timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    
    float throttleDelay = 0.0f;
    switch (priority) {
        case MessagePriority::Critical: throttleDelay = 0.05f; break;
        case MessagePriority::High: throttleDelay = 0.1f; break;
        case MessagePriority::Medium: throttleDelay = 0.5f; break;
        case MessagePriority::Low: throttleDelay = 1.0f; break;
    }

    NetworkMessage msg;
    msg.content = content;
    msg.priority = priority;
    msg.target = target;
    msg.timestamp = timestamp;
    msg.sendAfter = throttleDelay;
    messageQueue.push(msg);
}

void NetworkManager::processMessageQueue(float dt) {
    timeSinceLastSend += dt;
    const float minSendInterval = 0.05f; // Max 20 messages/sec
    const int maxMessagesPerFrame = 5;
    int messagesSent = 0;

    std::vector<NetworkMessage> batch;
    while (!messageQueue.empty() && messagesSent < maxMessagesPerFrame && timeSinceLastSend >= minSendInterval) {
        NetworkMessage msg = messageQueue.top();
        if (timeSinceLastSend < msg.sendAfter) break;

        batch.push_back(msg);
        messageQueue.pop();

        // Batch messages with the same target or broadcast
        while (!messageQueue.empty() && batch.size() < 10) { // Arbitrary batch limit
            NetworkMessage nextMsg = messageQueue.top();
            if (nextMsg.sendAfter > timeSinceLastSend || nextMsg.priority != msg.priority ||
                (nextMsg.target != msg.target && nextMsg.target != k_steamIDNil)) break;
            batch.push_back(nextMsg);
            messageQueue.pop();
        }

        if (batch.size() > 1) {
            std::string batchedMsg;
            for (size_t i = 0; i < batch.size(); ++i) {
                batchedMsg += batch[i].content;
                if (i < batch.size() - 1) batchedMsg += "|BATCH|";
            }
            if (msg.target == k_steamIDNil) {
                broadcastMessage(batchedMsg);
            } else {
                sendMessage(msg.target, batchedMsg);
            }
        } else {
            if (msg.target == k_steamIDNil) {
                broadcastMessage(msg.content);
            } else {
                sendMessage(msg.target, msg.content);
            }
        }

        messagesSent++;
        timeSinceLastSend = 0.0f;
        batch.clear();
    }
}