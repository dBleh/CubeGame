#pragma once

#include <SFML/Graphics.hpp>
#include <SFML/Window.hpp>
#include <SFML/OpenGL.hpp>
#include <GL/glu.h>
#include <iostream>
#include <memory>
#include <string>
#include "PlayerConfig.h"

// Forward declarations
class GameObject;
class PhysicsSystem;

/**
 * @class Player
 * @brief Represents a player in the game world
 * 
 * Handles player input, state, animation, and rendering.
 * Physics calculations are delegated to the PhysicsSystem.
 */
class Player {
public:
    // Types and enums
    enum class State {
        Idle,
        Walking,
        Running,
        Jumping,
        Falling,
        Crouching
    };

    // Structure to hold body proportions for character model
    struct BodyProportions {
        // Overall size
        float scale = 1.0f;
        
        // Basic dimensions
        float headRadius = 0.4f;
        float bodyWidth = 0.5f;
        float limbWidth = 0.25f;
        
        // Heights and lengths
        float headHeight = 0.8f;    // Height offset for head from top of torso
        float torsoLength = 2.0f;
        float shoulderWidth = 1.6f;
        float hipWidth = 0.8f;      // Distance between hips
        
        // Arm dimensions
        float shoulderHeight = 0.9f; // Height position on torso (0-1)
        float upperArmLength = 1.5f;
        float forearmLength = 1.4f;
        
        // Leg dimensions
        float thighLength = 1.8f;
        float calfLength = 1.7f;
        
        // Position adjustments
        float shoulderOffsetZ = 0.0f; // Forward/backward offset of shoulders
        float hipOffsetZ = 0.0f;      // Forward/backward offset of hips
    };

    // Constructors/destructors
    Player();
    ~Player();

    // Initialization
    bool initialize(sf::Window* window);
    bool loadConfig(const std::string& configPath);
    
    // Set physics system reference
    void setPhysicsSystem(PhysicsSystem* physics) { m_physicsSystem = physics; }
    
    // Update methods
    void update(float deltaTime);
    void updateCamera();
    
    // Animation
    void updateAnimation(float deltaTime);
    
    // Rendering
    void render();
    
    // Input handling
    void handleInput(float deltaTime);
    
    // Player state
    State getState() const { return m_state; }
    bool isGrounded() const { return m_isGrounded; }
    void setGrounded(bool grounded) { m_isGrounded = grounded; }
    float getGroundedTimer() const { return m_groundedTimer; }
    
    void setGroundedTimer(float time) { 
        m_groundedTimer = time; 
        if (time > 0) m_isGrounded = true;
    }
    
    float getHealth() const { return m_health; }
    void setHealth(float health) { m_health = health; }
    void damage(float amount);
    bool isEnabled() const { return m_health > 0.0f; }
    
    // Position and movement
    sf::Vector3f getPosition() const { return m_position; }
    void setPosition(const sf::Vector3f& position) { m_position = position; }
    
    sf::Vector3f getRotation() const { return m_rotation; }
    void setRotation(const sf::Vector3f& rotation) { m_rotation = rotation; }
    
    sf::Vector3f getVelocity() const { return m_velocity; }
    void setVelocity(const sf::Vector3f& velocity) { m_velocity = velocity; }
    
    sf::Vector3f getLookDirection() const;
    
    // Configuration access
    const PlayerConfig& getConfig() const { return m_config; }
    PlayerConfig& getConfig() { return m_config; }
    
    // Debug
    void toggleDebugMode() { m_debugMode = !m_debugMode; }
    bool isDebugMode() const { return m_debugMode; }

    // Body proportions setter
    void setBodyProportion(const std::string& propName, float value);

private:
    // Window reference
    sf::Window* m_window;
    
    // Physics system reference (not owned)
    PhysicsSystem* m_physicsSystem;
    
    // Configuration
    PlayerConfig m_config;
    
    // Player state
    sf::Vector3f m_position;      // x, y, z position
    sf::Vector3f m_rotation;      // x, y, z rotation in degrees
    sf::Vector3f m_velocity;      // Current velocity
    State m_state;                // Current player state
    float m_health;               // Player health (0-100)
    bool m_isGrounded;            // Is player on the ground?
    float m_groundedTimer;        // Timer to keep grounded state briefly
    
    // Animation state
    float m_animTime;             // Animation timer
    float m_footstepTimer;        // Time since last footstep sound
    
    // Input state
    bool m_moveForward;
    bool m_moveBackward;
    bool m_moveLeft;
    bool m_moveRight;
    bool m_jump;
    bool m_crouch;
    bool m_sprint;

    // Camera state
    sf::Vector2i m_lastMousePos;
    bool m_firstMouseMove;
    
    // Debug
    bool m_debugMode;
    
    // Model parameters
    BodyProportions m_bodyProps;
    float m_totalLegLength;       // Computed as thighLength + calfLength
    int m_segments;               // Level of detail for rendering
    
    // Private methods
    void initBodyProportions();
    void handleKeyboardInput();
    void handleMouseInput();
    void updateMouseLook();
    void handleMovement(float deltaTime);
    
    // Drawing methods
    void drawPlayerModel();
    void drawHead();
    void drawTorso();
    void drawArm(bool isLeft, float armSwing, float elbowBend);
    void drawLeg(bool isLeft, float legSwing, float kneeBend);
    void drawDebugInfo();
    
    // Sound methods
    void playFootstepSound();
    void playJumpSound();
    void playLandingSound();
};