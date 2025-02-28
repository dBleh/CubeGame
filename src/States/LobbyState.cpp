#include "LobbyState.h"

LobbyState::LobbyState(CubeGame* game) : State(game) {}

void LobbyState::Update(float dt) {
    UpdateLobbyMembers();
    size_t readyCount = std::count_if(game->GetPlayers().begin(), game->GetPlayers().end(), 
        [](const auto& p) { return p.second.ready; });
    std::string lobbyInfo = "Lobby - Players: " + std::to_string(game->GetPlayers().size()) + 
                           "\nReady: " + std::to_string(readyCount);
    game->GetHUD().updateText("lobbyStatus", lobbyInfo);

    // Explicitly control "startGame" visibility
    if (game->GetLocalPlayer().steamID == SteamMatchmaking()->GetLobbyOwner(game->GetLobbyID())) {
        game->GetHUD().updateText("startGame", "Press S to Start Game (Host Only)");
    } else {
        game->GetHUD().updateText("startGame", ""); // Hide for non-hosts
        game->GetHUD().updateText("returnMain", "Press M to Return to Main Menu"); // Ensure visible
    }
}

void LobbyState::Render() {
    game->GetWindow().clear(sf::Color::Black);
    for (const auto& player : game->GetPlayers()) {
        game->GetWindow().draw(player.second.shape);
    }
    sf::RectangleShape lobbyIndicator(sf::Vector2f(200, 50));
    lobbyIndicator.setFillColor(sf::Color::Yellow);
    lobbyIndicator.setPosition(SCREEN_WIDTH/2 - 100, 100);
    game->GetWindow().draw(lobbyIndicator);

    game->RenderHUDLayer();
    game->GetWindow().display();
}

void LobbyState::ProcessEvent(const sf::Event& event) {
    ProcessEvents(event);
}

void LobbyState::ProcessEvents(const sf::Event& event) {
    if (event.type == sf::Event::KeyPressed) {
        if (event.key.code == sf::Keyboard::R) {
            game->ToggleReady();
        } else if (event.key.code == sf::Keyboard::S && 
                   game->GetLocalPlayer().steamID == SteamMatchmaking()->GetLobbyOwner(game->GetLobbyID())) {
            std::cout << "[DEBUG] Host pressed S. Players: " << game->GetPlayers().size() 
                      << ", All ready: " << (AllPlayersReady() ? "Yes" : "No") << std::endl;
            if (game->GetPlayers().size() <= 1 || (game->GetPlayers().size() > 1 && (AllPlayersReady() || game->HasGameBeenPlayed()))) {
                game->StartGame();
            } else {
                std::cout << "[DEBUG] Cannot start: Multiple players not all ready" << std::endl;
            }
        } else if (event.key.code == sf::Keyboard::M) {
            game->ReturnToMainMenu();
            std::cout << "[DEBUG] Returning to main menu from lobby" << std::endl;
        }
    }
}

void LobbyState::UpdateLobbyMembers() {
    if (!game->IsInLobby()) return;
    int memberCount = SteamMatchmaking()->GetNumLobbyMembers(game->GetLobbyID());
    std::cout << "[DEBUG] Lobby member count: " << memberCount << std::endl;
    for (int i = 0; i < memberCount; ++i) {
        CSteamID member = SteamMatchmaking()->GetLobbyMemberByIndex(game->GetLobbyID(), i);
        if (!game->GetPlayers().count(member)) {
            Player p;
            p.initialize();
            p.steamID = member;
            game->GetPlayers()[member] = p;
            std::cout << "[DEBUG] Added player: " << member.ConvertToUint64() << std::endl;
        }
        const char* readyState = SteamMatchmaking()->GetLobbyMemberData(game->GetLobbyID(), member, "ready");
        game->GetPlayers()[member].ready = (readyState && std::string(readyState) == "1");
    }
    for (auto it = game->GetPlayers().begin(); it != game->GetPlayers().end();) {
        bool found = false;
        for (int i = 0; i < memberCount; ++i) {
            if (SteamMatchmaking()->GetLobbyMemberByIndex(game->GetLobbyID(), i) == it->first) {
                found = true;
                break;
            }
        }
        if (!found) {
            std::cout << "[DEBUG] Removed stale player: " << it->first.ConvertToUint64() << std::endl;
            it = game->GetPlayers().erase(it);
        } else {
            ++it;
        }
    }
    std::cout << "[DEBUG] Players after sync: " << game->GetPlayers().size() << std::endl;
}

bool LobbyState::AllPlayersReady() {
    return std::all_of(game->GetPlayers().begin(), game->GetPlayers().end(), 
        [](const auto& p) { return p.second.ready; });
}