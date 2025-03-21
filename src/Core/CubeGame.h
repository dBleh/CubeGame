#pragma once

#include <SFML/Graphics.hpp>
#include <SFML/Window.hpp>
#include <SFML/OpenGL.hpp>
#include <GL/glu.h>

class CubeGame {
public:
    CubeGame();
    ~CubeGame();

    bool initialize();
    void run();

private:
    void handleEvents();
    void update(float deltaTime);
    void render();
    void setupOpenGL();
    void drawCube();

    sf::RenderWindow m_window;
    sf::Clock m_clock;
    
    // Camera position and rotation
    float m_rotationX;
    float m_rotationY;
    bool m_running;
};