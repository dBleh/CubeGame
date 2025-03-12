#include "Game.h"
#include "states/MainMenuState.h"
#include "states/LobbyCreationState.h"
#include "states/LobbySearchState.h"
#include "states/LobbyState.h"
#include <steam/steam_api.h>
#include <iostream>

Game::Game() : hud(font) {
    // Initialize window
    window.create(sf::VideoMode(1280, 720), "SteamGame");
    window.setFramerateLimit(60);

    // Load font
    if (!font.loadFromFile("Roboto-Regular.ttf")) {
        if(!font.loadFromFile("../../Roboto-Regular.ttf")){
            std::cerr << "[ERROR] Failed to load font!" << std::endl;
            std::exit(1);
        }
    }

    // Initialize Steam
    if (SteamAPI_Init()) {
        steamInitialized = true;
        // Store the local player's Steam ID
        localSteamID = SteamUser()->GetSteamID();
        // Alternatively, you can use: SetLocalSteamID(SteamUser()->GetSteamID());
    } else {
        std::cerr << "[ERROR] Steam API initialization failed!" << std::endl;
    }

    // Initialize NetworkManager
    networkManager = std::make_unique<NetworkManager>(this);

    // Set initial state
    state = std::make_unique<MainMenuState>(this);
}

Game::~Game() {
    if (inLobby) {
        SteamMatchmaking()->LeaveLobby(currentLobby);
    }
    SteamAPI_Shutdown();
}

void Game::Run() {
    sf::Clock clock;
    while (window.isOpen()) {
        // Process Steam callbacks
        if (steamInitialized) {
            SteamAPI_RunCallbacks();
        }
        networkManager->ReceiveMessages();

        // Calculate delta time
        float dt = clock.restart().asSeconds();

        // Handle events
        sf::Event event;
        while (window.pollEvent(event)) {
            ProcessEvents(event);
            if (state) state->ProcessEvent(event);
        }

        // Update state
        if (state) state->Update(dt);

        // State transition
        switch (currentState) {
            case GameState::MainMenu:
                if (!dynamic_cast<MainMenuState*>(state.get()))
                    state = std::make_unique<MainMenuState>(this);
                break;
            case GameState::LobbyCreation:
                if (!dynamic_cast<LobbyCreationState*>(state.get()))
                    state = std::make_unique<LobbyCreationState>(this);
                break;
            case GameState::LobbySearch:
                if (!dynamic_cast<LobbySearchState*>(state.get()))
                    state = std::make_unique<LobbySearchState>(this);
                break;
            case GameState::Lobby:
                if (!dynamic_cast<LobbyState*>(state.get()))
                state = std::make_unique<LobbyState>(this);
                break;
            default:
                break;
        }

        // Render
        if (state) state->Render();
    }
}

void Game::SetCurrentState(GameState newState) {
    currentState = newState;
}

void Game::ProcessEvents(sf::Event& event) {
    if (event.type == sf::Event::Closed) {
        window.close();
    }
    if (event.type == sf::Event::Resized) {
        AdjustViewToWindow();
    }
}

void Game::AdjustViewToWindow() {
    sf::Vector2u winSize = window.getSize();
    sf::View view(sf::FloatRect(0.f, 0.f, static_cast<float>(winSize.x), static_cast<float>(winSize.y)));
    window.setView(view);
}