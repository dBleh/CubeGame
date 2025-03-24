#include "CubeGame.h"

// Windows header must be included before OpenGL headers
#ifdef _WIN32
#include <windows.h>
#endif

#include "../Entities/Player/Player.h"
#include "../Entities/Objects/SquareObject.h"
#include "../Physics/PhysicsSystem.h"
#include <iostream>

CubeGame::CubeGame() 
    : m_running(false)
{
    // Create player and physics system
    m_player = std::make_unique<Player>();
    m_physicsSystem = std::make_unique<PhysicsSystem>();
    
    // Create root game object
    m_rootObject = std::make_shared<GameObject>("Root");
}

CubeGame::~CubeGame()
{
    // Clear game objects
    if (m_rootObject) {
        m_rootObject->clearChildren();
    }
    m_gameObjects.clear();
}

bool CubeGame::initialize()
{
    // Create the window with OpenGL context
    m_window.create(sf::VideoMode(1024, 768), "3D Cube Game with Player", 
                    sf::Style::Default, sf::ContextSettings(24, 8, 4, 3, 3));
    
    if (!m_window.isOpen()) {
        std::cerr << "[ERROR] Failed to create window!" << std::endl;
        return false;
    }
    
    // Setup OpenGL state
    setupOpenGL();
    
    // Initialize player
    if (!m_player->initialize(&m_window)) {
        std::cerr << "[ERROR] Failed to initialize player!" << std::endl;
        return false;
    }
    
    // Set initial player position slightly above the ground
    m_player->setPosition(sf::Vector3f(0.0f, 0.5f, -3.0f));
    
    // Load player configuration
    if (!m_player->loadConfig("player_config.ini")) {

        std::cout << "[WARNING] Failed to load player config, using defaults." << std::endl;
        
        // Setup jump force - this is crucial for jumping to work
        m_player->getConfig().setJumpForce(10.0f);
        m_player->getConfig().setGravity(9.8f);
    }
    
    // Initialize physics system
    m_physicsSystem->initialize();
    m_physicsSystem->setPlayer(m_player.get());
    
    // Connect player to physics system
    m_player->setPhysicsSystem(m_physicsSystem.get());
    
    // Create game objects
    setupGameObjects();
    
    m_running = true;
    
    std::cout << "[DEBUG] Game initialized successfully" << std::endl;
    return true;
}

void CubeGame::setupOpenGL()
{
    // Set the clear color
    glClearColor(0.2f, 0.2f, 0.3f, 1.0f);
    
    // Enable depth testing
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
    
    // Enable backface culling
    glEnable(GL_CULL_FACE);
    
    // Setup perspective projection
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(75.0f, (float)m_window.getSize().x / (float)m_window.getSize().y, 0.1f, 100.0f);
    
    glMatrixMode(GL_MODELVIEW);
}

void CubeGame::run()
{
    while (m_running) {
        float deltaTime = m_clock.restart().asSeconds();
        
        // Cap deltaTime to prevent large jumps
        if (deltaTime > 0.1f) deltaTime = 0.1f;
        
        handleEvents();
        update(deltaTime);
        render();
    }
}

void CubeGame::handleEvents()
{
    sf::Event event;
    while (m_window.pollEvent(event)) {
        if (event.type == sf::Event::Closed) {
            m_running = false;
        }
        else if (event.type == sf::Event::KeyPressed) {
            if (event.key.code == sf::Keyboard::Escape) {
                m_running = false;
            }
            // Toggle between first-person and third-person camera modes with the 'V' key
            else if (event.key.code == sf::Keyboard::V) {
                PlayerConfig& config = m_player->getConfig();
                bool currentMode = config.getFirstPersonMode();
                config.setFirstPersonMode(!currentMode);
                std::cout << "[INFO] Switched to " 
                          << (config.getFirstPersonMode() ? "first-person" : "third-person") 
                          << " camera mode" << std::endl;
            }
            // Toggle debug information with F1 key
            else if (event.key.code == sf::Keyboard::F1) {
                m_player->toggleDebugMode();
                m_physicsSystem->setDebugVisualization(m_player->isDebugMode());
                std::cout << "[INFO] Debug mode " 
                          << (m_player->isDebugMode() ? "enabled" : "disabled") << std::endl;
            }
        }
        else if (event.type == sf::Event::Resized) {
            // Adjust viewport on resize
            glViewport(0, 0, event.size.width, event.size.height);
            
            // Update projection matrix for new aspect ratio
            glMatrixMode(GL_PROJECTION);
            glLoadIdentity();
            gluPerspective(75.0f, (float)event.size.width / (float)event.size.height, 0.1f, 100.0f);
            glMatrixMode(GL_MODELVIEW);
        }
    }
}

void CubeGame::update(float deltaTime)
{
    // Update player first so movement inputs are applied
    m_player->update(deltaTime);
    
    // Update physics system after player to handle collisions
    m_physicsSystem->update(deltaTime);
    
    // Update all game objects
    m_rootObject->update(deltaTime);
}

void CubeGame::render()
{
    // Clear the screen
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    
    // Reset the modelview matrix
    glLoadIdentity();
    
    // Apply player camera transform
    m_player->updateCamera();
    
    // Draw the world
    drawWorld();
    
    // Render all game objects
    m_rootObject->render();
    
    // Draw the player
    m_player->render();
    
    // Draw physics debug visualization if enabled
    if (m_player->isDebugMode()) {
        m_physicsSystem->debugDraw();
    }
    
    // Display the rendered frame
    m_window.display();
}

void CubeGame::drawWorld()
{
    // Draw floor
    drawFloor();
}

void CubeGame::drawFloor()
{
    float size = 20.0f; // Size of the floor
    float tileSize = 1.0f; // Size of each tile
    
    glPushMatrix();
    
    // Draw a grid of tiles for the floor
    glBegin(GL_QUADS);
    
    for (float x = -size/2; x < size/2; x += tileSize) {
        for (float z = -size/2; z < size/2; z += tileSize) {
            // Checkerboard pattern
            if ((int(x/tileSize) + int(z/tileSize)) % 2 == 0) {
                glColor3f(0.8f, 0.8f, 0.8f); // Light gray
            } else {
                glColor3f(0.3f, 0.3f, 0.3f); // Dark gray
            }
            
            // Draw tile
            glVertex3f(x, 0, z);
            glVertex3f(x + tileSize, 0, z);
            glVertex3f(x + tileSize, 0, z + tileSize);
            glVertex3f(x, 0, z + tileSize);
        }
    }
    
    glEnd();
    
    glPopMatrix();
}

std::shared_ptr<GameObject> CubeGame::createCube(const std::string& name, 
                                               const sf::Vector3f& position, 
                                               const sf::Vector3f& size, 
                                               const sf::Color& color)
{
    // Create a square object
    std::shared_ptr<SquareObject> square = std::make_shared<SquareObject>(name, position, size);
    square->setColor(color);
    
    // Add to tracked game objects
    m_gameObjects.push_back(square);
    
    return square;
}

void CubeGame::setupGameObjects()
{
    // Create test cube in front of the player
    auto testCube = createCube(
        "TestCube",
        sf::Vector3f(0.0f, 1.0f, 0.0f),  // Position
        sf::Vector3f(3.0f, 2.0f, 3.0f),  // Size
        sf::Color(50, 100, 200));  // Blue color
        
    m_rootObject->addChild(testCube);
    
    // Add to physics system
    m_physicsSystem->addCollider(testCube, CollisionLayer::Environment);
    
    // Create a platform to jump onto
    auto platform = createCube(
        "Platform",
        sf::Vector3f(4.0f, 0.5f, 3.0f),  // Position
        sf::Vector3f(4.0f, 1.0f, 4.0f),  // Size
        sf::Color(200, 50, 50));  // Red color
        
    m_rootObject->addChild(platform);
    m_physicsSystem->addCollider(platform, CollisionLayer::Environment);
    
    // Create a trigger zone
    auto triggerZone = createCube(
        "TriggerZone",
        sf::Vector3f(-4.0f, 1.0f, -3.0f),  // Position
        sf::Vector3f(2.0f, 2.0f, 2.0f),  // Size
        sf::Color(255, 255, 0, 128));  // Yellow, semi-transparent
    
    triggerZone->setColor(sf::Color(255, 255, 0, 128)); // Make it semi-transparent
    m_rootObject->addChild(triggerZone);
    m_physicsSystem->addCollider(triggerZone, CollisionLayer::Trigger);
    
    std::cout << "[INFO] Created game objects" << std::endl;
}