#pragma once

// Windows header must be included before OpenGL headers to avoid conflicts
#ifdef _WIN32
#include <windows.h>
#endif

#include <SFML/Graphics.hpp>
#include <SFML/Window.hpp>
#include <SFML/OpenGL.hpp>
#include <GL/glu.h>
#include <vector>
#include <memory>

// Forward declarations
class Player;
class GameObject;
class PhysicsSystem;

/**
 * @class CubeGame
 * @brief Main game class that coordinates all game systems
 * 
 * Handles initialization, game loop, rendering, and coordinates 
 * between player, physics, and game objects.
 */
class CubeGame {
public:
    CubeGame();
    ~CubeGame();

    // Core game loop methods
    bool initialize();
    void run();

private:
    // Game loop methods
    void handleEvents();
    void update(float deltaTime);
    void render();
    
    // Setup methods
    void setupOpenGL();
    void setupGameObjects();
    
    // Rendering methods
    void drawWorld();
    void drawFloor();
    
    // Factory methods for game objects
    std::shared_ptr<GameObject> createCube(const std::string& name, 
                                          const sf::Vector3f& position, 
                                          const sf::Vector3f& size, 
                                          const sf::Color& color);

    // SFML objects
    sf::RenderWindow m_window;
    sf::Clock m_clock;
    
    // Game state
    bool m_running;
    
    // Core systems
    std::unique_ptr<Player> m_player;
    std::unique_ptr<PhysicsSystem> m_physicsSystem;
    
    // Game objects
    std::shared_ptr<GameObject> m_rootObject;
    std::vector<std::shared_ptr<GameObject>> m_gameObjects;
};