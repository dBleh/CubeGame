#include "CubeGame.h"
#include <iostream>

CubeGame::CubeGame() 
    : m_rotationX(0.0f), m_rotationY(0.0f), m_running(false)
{
}

CubeGame::~CubeGame() {
}

bool CubeGame::initialize() {
    // Create the window with OpenGL context
    m_window.create(sf::VideoMode(800, 600), "SFML 3D Cube with OpenGL", 
                    sf::Style::Default, sf::ContextSettings(24, 8, 4, 3, 3));
    
    if (!m_window.isOpen()) {
        std::cerr << "[ERROR] Failed to create window!" << std::endl;
        return false;
    }
    
    // Setup OpenGL state
    setupOpenGL();
    m_running = true;
    
    std::cout << "[DEBUG] Game initialized successfully" << std::endl;
    return true;
}

void CubeGame::setupOpenGL() {
    // Set the clear color
    glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
    
    // Enable depth testing
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
    
    // Enable backface culling
    glEnable(GL_CULL_FACE);
    
    // Setup perspective projection
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(45.0f, (float)m_window.getSize().x / (float)m_window.getSize().y, 0.1f, 100.0f);
    
    glMatrixMode(GL_MODELVIEW);
}

void CubeGame::run() {
    while (m_running) {
        float deltaTime = m_clock.restart().asSeconds();
        
        handleEvents();
        update(deltaTime);
        render();
    }
}

void CubeGame::handleEvents() {
    sf::Event event;
    while (m_window.pollEvent(event)) {
        if (event.type == sf::Event::Closed) {
            m_running = false;
        }
        else if (event.type == sf::Event::KeyPressed) {
            if (event.key.code == sf::Keyboard::Escape) {
                m_running = false;
            }
        }
    }
    
    // Simple keyboard controls for rotating the cube
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Left)) {
        m_rotationY -= 1.0f;
    }
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Right)) {
        m_rotationY += 1.0f;
    }
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Up)) {
        m_rotationX -= 1.0f;
    }
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Down)) {
        m_rotationX += 1.0f;
    }
}

void CubeGame::update(float deltaTime) {
    // Update game logic here
}

void CubeGame::render() {
    // Clear the screen
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    
    // Reset the modelview matrix
    glLoadIdentity();
    
    // Position the camera (move back 5 units)
    glTranslatef(0.0f, 0.0f, -5.0f);
    
    // Apply rotations
    glRotatef(m_rotationX, 1.0f, 0.0f, 0.0f);
    glRotatef(m_rotationY, 0.0f, 1.0f, 0.0f);
    
    // Draw the cube
    drawCube();
    
    // Display the rendered frame
    m_window.display();
}

void CubeGame::drawCube() {
    // Define the cube vertices (corners)
    static const GLfloat vertices[] = {
        // Front face
        -0.5f, -0.5f,  0.5f,
         0.5f, -0.5f,  0.5f,
         0.5f,  0.5f,  0.5f,
        -0.5f,  0.5f,  0.5f,
        
        // Back face
        -0.5f, -0.5f, -0.5f,
         0.5f, -0.5f, -0.5f,
         0.5f,  0.5f, -0.5f,
        -0.5f,  0.5f, -0.5f
    };
    
    // Define the colors for each vertex
    static const GLfloat colors[] = {
        1.0f, 0.0f, 0.0f,  // Red
        0.0f, 1.0f, 0.0f,  // Green
        0.0f, 0.0f, 1.0f,  // Blue
        1.0f, 1.0f, 0.0f,  // Yellow
        
        1.0f, 0.0f, 1.0f,  // Magenta
        0.0f, 1.0f, 1.0f,  // Cyan
        1.0f, 1.0f, 1.0f,  // White
        0.5f, 0.5f, 0.5f   // Gray
    };
    
    // Draw the cube using immediate mode (old OpenGL, but simple for demonstration)
    glBegin(GL_QUADS);
    
    // Front face
    glColor3f(colors[0], colors[1], colors[2]);
    glVertex3f(vertices[0], vertices[1], vertices[2]);
    glColor3f(colors[3], colors[4], colors[5]);
    glVertex3f(vertices[3], vertices[4], vertices[5]);
    glColor3f(colors[6], colors[7], colors[8]);
    glVertex3f(vertices[6], vertices[7], vertices[8]);
    glColor3f(colors[9], colors[10], colors[11]);
    glVertex3f(vertices[9], vertices[10], vertices[11]);
    
    // Back face
    glColor3f(colors[12], colors[13], colors[14]);
    glVertex3f(vertices[15], vertices[16], vertices[17]);
    glColor3f(colors[15], colors[16], colors[17]);
    glVertex3f(vertices[18], vertices[19], vertices[20]);
    glColor3f(colors[18], colors[19], colors[20]);
    glVertex3f(vertices[21], vertices[22], vertices[23]);
    glColor3f(colors[21], colors[22], colors[23]);
    glVertex3f(vertices[12], vertices[13], vertices[14]);
    
    // Top face
    glColor3f(colors[6], colors[7], colors[8]);
    glVertex3f(vertices[6], vertices[7], vertices[8]);
    glColor3f(colors[9], colors[10], colors[11]);
    glVertex3f(vertices[9], vertices[10], vertices[11]);
    glColor3f(colors[21], colors[22], colors[23]);
    glVertex3f(vertices[21], vertices[22], vertices[23]);
    glColor3f(colors[18], colors[19], colors[20]);
    glVertex3f(vertices[18], vertices[19], vertices[20]);
    
    // Bottom face
    glColor3f(colors[0], colors[1], colors[2]);
    glVertex3f(vertices[0], vertices[1], vertices[2]);
    glColor3f(colors[12], colors[13], colors[14]);
    glVertex3f(vertices[12], vertices[13], vertices[14]);
    glColor3f(colors[15], colors[16], colors[17]);
    glVertex3f(vertices[15], vertices[16], vertices[17]);
    glColor3f(colors[3], colors[4], colors[5]);
    glVertex3f(vertices[3], vertices[4], vertices[5]);
    
    // Right face
    glColor3f(colors[3], colors[4], colors[5]);
    glVertex3f(vertices[3], vertices[4], vertices[5]);
    glColor3f(colors[15], colors[16], colors[17]);
    glVertex3f(vertices[15], vertices[16], vertices[17]);
    glColor3f(colors[18], colors[19], colors[20]);
    glVertex3f(vertices[18], vertices[19], vertices[20]);
    glColor3f(colors[6], colors[7], colors[8]);
    glVertex3f(vertices[6], vertices[7], vertices[8]);
    
    // Left face
    glColor3f(colors[0], colors[1], colors[2]);
    glVertex3f(vertices[0], vertices[1], vertices[2]);
    glColor3f(colors[9], colors[10], colors[11]);
    glVertex3f(vertices[9], vertices[10], vertices[11]);
    glColor3f(colors[21], colors[22], colors[23]);
    glVertex3f(vertices[21], vertices[22], vertices[23]);
    glColor3f(colors[12], colors[13], colors[14]);
    glVertex3f(vertices[12], vertices[13], vertices[14]);
    
    glEnd();
}