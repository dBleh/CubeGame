#include "Player.h"
#include "../Physics/PhysicsSystem.h"
#include <cmath>
#include <algorithm>

Player::Player()
    : m_window(nullptr),
      m_physicsSystem(nullptr),
      m_position(0.0f, 0.0f, 0.0f),
      m_rotation(0.0f, 0.0f, 0.0f),
      m_velocity(0.0f, 0.0f, 0.0f),
      m_state(State::Idle),
      m_health(100.0f),
      m_isGrounded(true),
      m_groundedTimer(0.0f),
      m_animTime(0.0f),
      m_footstepTimer(0.0f),
      m_moveForward(false),
      m_moveBackward(false),
      m_moveLeft(false),
      m_moveRight(false),
      m_jump(false),
      m_crouch(false),
      m_sprint(false),
      m_firstMouseMove(true),
      m_debugMode(false)
{
}

Player::~Player()
{
    // Nothing to clean up - we don't own any resources
}

bool Player::initialize(sf::Window* window)
{
    if (!window) {
        std::cerr << "[ERROR] Player initialization failed: Invalid window pointer" << std::endl;
        return false;
    }

    m_window = window;

    // Center the mouse in the window
    sf::Vector2i windowCenter(m_window->getSize().x / 2, m_window->getSize().y / 2);
    sf::Mouse::setPosition(windowCenter, *m_window);
    m_lastMousePos = windowCenter;

    // Hide the cursor for first-person view
    m_window->setMouseCursorVisible(false);

    std::cout << "[INFO] Player initialized successfully" << std::endl;
    return true;
}

bool Player::loadConfig(const std::string& configPath)
{
    if (configPath.empty()) {
        m_config.resetToDefaults();
        std::cout << "[INFO] Using default player configuration" << std::endl;
        return true;
    }

    if (!m_config.loadFromFile(configPath)) {
        std::cerr << "[WARNING] Failed to load player config from " << configPath << ", using defaults" << std::endl;
        m_config.resetToDefaults();
        return false;
    }

    std::cout << "[INFO] Loaded player configuration from " << configPath << std::endl;
    return true;
}

void Player::update(float deltaTime)
{
    // Update the grounded timer
    if (m_groundedTimer > 0) {
        m_groundedTimer -= deltaTime;
        m_isGrounded = true;
    }
    
    // Handle user input and movement
    handleInput(deltaTime);
    
    // Update state based on velocity and grounded state
    if (!m_isGrounded) {
        if (m_velocity.y > 0) {
            m_state = State::Jumping;
        } else if (m_velocity.y < 0) {
            m_state = State::Falling;
        }
    }
    
    // Update animation state
    updateAnimation(deltaTime);
    
    // Store previous Y position for edge detection
    float prevY = m_position.y;
    
    // Update position based on velocity - physics system will handle collisions later
    m_position.x += m_velocity.x * deltaTime;
    m_position.y += m_velocity.y * deltaTime;
    m_position.z += m_velocity.z * deltaTime;
    
    // Debug output for slow falling
    if (m_debugMode && !m_isGrounded && std::abs(m_velocity.y) < 2.0f) {
        std::cout << "[DEBUG] Slow falling detected. Position: " << m_position.y 
                  << ", Velocity.y: " << m_velocity.y << std::endl;
    }
    
    // Edge detection - if we moved horizontally but not vertically while grounded,
    // check if we're at an edge and should start falling
    if (m_isGrounded && 
        (std::abs(m_velocity.x) > 0.01f || std::abs(m_velocity.z) > 0.01f) && 
        std::abs(m_position.y - prevY) < 0.01f) {
        
        // Edge detection will be handled by physics system in the next frame
        m_groundedTimer = 0.0f; // Allow immediate ungrounding
    }
    
    // Basic ground check (will be refined by physics system)
    if (m_position.y < m_config.getPlayerRadius()) {
        m_position.y = m_config.getPlayerRadius();
        m_velocity.y = 0;
        m_isGrounded = true;
        
        // If we were falling, play landing sound
        if (m_state == State::Falling) {
            playLandingSound();
            m_state = State::Idle;
        }
    }
    
    if (m_debugMode && m_jump) {
        std::cout << "[DEBUG] Jump key pressed, grounded: " << (m_isGrounded ? "Yes" : "No") 
                 << ", velocity.y: " << m_velocity.y << std::endl;
    }
}

void Player::handleInput(float deltaTime)
{
    handleKeyboardInput();
    handleMouseInput();
    handleMovement(deltaTime);
}

void Player::handleKeyboardInput()
{
    // Reset input state
    m_moveForward = false;
    m_moveBackward = false;
    m_moveLeft = false;
    m_moveRight = false;
    m_jump = false;
    m_crouch = false;
    m_sprint = false;

    // Process keyboard input
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::W)) {
        m_moveForward = true;
    }
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::S)) {
        m_moveBackward = true;
    }
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::A)) {
        m_moveLeft = true;
    }
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::D)) {
        m_moveRight = true;
    }
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Space)) {
        m_jump = true;
    }
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::LControl)) {
        m_crouch = true;
    }
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::LShift)) {
        m_sprint = true;
    }
}

void Player::handleMouseInput()
{
    updateMouseLook();
}

void Player::updateMouseLook()
{
    if (!m_window) return;

    // Get current mouse position
    sf::Vector2i currentMousePos = sf::Mouse::getPosition(*m_window);

    // Skip on first mouse move to avoid jumps
    if (m_firstMouseMove) {
        m_lastMousePos = currentMousePos;
        m_firstMouseMove = false;
        return;
    }

    // Calculate mouse offset
    float xOffset = static_cast<float>(currentMousePos.x - m_lastMousePos.x);
    float yOffset = static_cast<float>(currentMousePos.y - m_lastMousePos.y);

    // Apply sensitivity
    xOffset *= m_config.getMouseSensitivity();
    yOffset *= m_config.getMouseSensitivity();

    // Apply inversion if configured
    if (m_config.getInvertMouse()) {
        yOffset = -yOffset;
    }

    // Update rotation
    m_rotation.y += xOffset;
    m_rotation.x -= yOffset;  // Inverted because screen y-coordinates increase downward

    // Constrain pitch (looking up/down) to prevent flipping
    if (m_rotation.x > 89.0f) m_rotation.x = 89.0f;
    if (m_rotation.x < -89.0f) m_rotation.x = -89.0f;

    // Reset mouse position to center of window to prevent reaching window bounds
    sf::Vector2i windowCenter(m_window->getSize().x / 2, m_window->getSize().y / 2);
    sf::Mouse::setPosition(windowCenter, *m_window);
    m_lastMousePos = windowCenter;
}

void Player::handleMovement(float deltaTime)
{
    // Reset horizontal velocity components
    m_velocity.x = 0.0f;
    m_velocity.z = 0.0f;

    // Get camera look direction
    sf::Vector3f lookDir = getLookDirection();

    // Calculate the horizontal look direction (ignoring Y component for movement)
    sf::Vector3f forward = sf::Vector3f(lookDir.x, 0.0f, lookDir.z);

    // Normalize the forward direction
    float forwardLength = std::sqrt(forward.x * forward.x + forward.z * forward.z);
    if (forwardLength > 0.0001f) {
        forward.x /= forwardLength;
        forward.z /= forwardLength;
    }

    // Calculate the right vector (cross product of world up and forward)
    sf::Vector3f worldUp(0.0f, 1.0f, 0.0f);
    sf::Vector3f right;
    right.x = worldUp.y * forward.z - worldUp.z * forward.y;
    right.y = worldUp.z * forward.x - worldUp.x * forward.z;
    right.z = worldUp.x * forward.y - worldUp.y * forward.x;

    // Track if we're moving in any direction
    bool isMoving = false;
    sf::Vector3f moveDirection(0.0f, 0.0f, 0.0f);

    // Apply movement based on input flags
    if (m_moveForward) {
        moveDirection.x += forward.x;
        moveDirection.z += forward.z;
        isMoving = true;
    }
    if (m_moveBackward) {
        moveDirection.x -= forward.x;
        moveDirection.z -= forward.z;
        isMoving = true;
    }
    if (m_moveLeft) {
        moveDirection.x += right.x;
        moveDirection.z += right.z;
        isMoving = true;
    }
    if (m_moveRight) {
        moveDirection.x -= right.x;
        moveDirection.z -= right.z;
        isMoving = true;
    }

    // Normalize movement direction for consistent diagonal speed
    if (isMoving) {
        float moveLength = std::sqrt(moveDirection.x * moveDirection.x + moveDirection.z * moveDirection.z);
        if (moveLength > 0.0001f) {
            moveDirection.x /= moveLength;
            moveDirection.z /= moveLength;
        }
        
        // Determine movement speed based on state
        float currentSpeed = m_config.getMoveSpeed();
        
        if (m_crouch) {
            currentSpeed *= 0.5f; // Slower when crouching
            m_state = State::Crouching;
        } else if (m_sprint) {
            currentSpeed *= 1.6f; // Faster when sprinting
            m_state = State::Running;
        } else {
            m_state = State::Walking;
        }
        
        // Apply movement speed
        m_velocity.x = moveDirection.x * currentSpeed;
        m_velocity.z = moveDirection.z * currentSpeed;
    } else if (m_isGrounded) {
        m_state = m_crouch ? State::Crouching : State::Idle;
    }

    // Jump if on ground
    if (m_jump && m_isGrounded) {
        // Apply jump force
        m_velocity.y = m_config.getJumpForce();
        
        
        
        m_isGrounded = false;
        m_state = State::Jumping;
        playJumpSound();
        
        // Add a small upward position adjustment to avoid immediate collision with the ground
        m_position.y += 0.1f;
        
        if (m_debugMode) {
            std::cout << "[DEBUG] Player jumped with velocity " << m_velocity.y << std::endl;
        }
    }
}

sf::Vector3f Player::getLookDirection() const
{
    // Convert rotation in degrees to radians
    float yawRad = m_rotation.y * 3.14159f / 180.0f;
    float pitchRad = m_rotation.x * 3.14159f / 180.0f;

    // Calculate the look direction vector using correct OpenGL conventions
    // In OpenGL, looking down negative Z axis is forward with 0 degrees rotation
    sf::Vector3f dir;
    dir.x = sin(yawRad) * cos(pitchRad);
    dir.y = sin(pitchRad);
    dir.z = -cos(yawRad) * cos(pitchRad);

    // Normalize the direction vector
    float length = std::sqrt(dir.x * dir.x + dir.y * dir.y + dir.z * dir.z);
    if (length > 0.0001f) {
        dir.x /= length;
        dir.y /= length;
        dir.z /= length;
    }

    return dir;
}

void Player::updateAnimation(float deltaTime)
{
    // Update animation timer
    m_animTime += deltaTime;

    // Different animation speeds based on state
    switch (m_state) {
        case State::Walking:
            // Reset animation after 1 second
            if (m_animTime > 1.0f) m_animTime -= 1.0f;
            break;
            
        case State::Running:
            // Reset animation after 0.6 seconds (faster)
            if (m_animTime > 0.6f) m_animTime -= 0.6f;
            break;
            
        case State::Jumping:
        case State::Falling:
            // Continuous animation for jumping/falling
            break;
            
        case State::Crouching:
            // Slower animation for crouching
            if (m_animTime > 1.5f) m_animTime -= 1.5f;
            break;
            
        case State::Idle:
        default:
            // Idle breathing animation (4-second cycle)
            if (m_animTime > 4.0f) m_animTime -= 4.0f;
            break;
    }

    // Update footstep sounds
    if (m_isGrounded && (m_state == State::Walking || m_state == State::Running)) {
        // Time between footsteps depends on movement speed
        float footstepInterval = (m_state == State::Running) ? 0.3f : 0.5f;
        
        m_footstepTimer += deltaTime;
        if (m_footstepTimer >= footstepInterval) {
            playFootstepSound();
            m_footstepTimer = 0.0f;
        }
    }
}

void Player::updateCamera()
{
    // Reset the view matrix
    glLoadIdentity();
    
    // Get the look direction
    sf::Vector3f lookDir = getLookDirection();
    
    // Get camera settings from config
    float cameraHeight = m_position.y + m_config.getCameraHeight();
    
    // If crouching, lower the camera height
    if (m_state == State::Crouching) {
        cameraHeight -= 0.5f;
    }
    
    if (m_config.getFirstPersonMode()) {
        // First-person camera
        // Set up the camera at player position + camera height
        
        // Calculate look target with corrected vertical direction
        float verticalLook = m_config.getInvertMouse() ? -lookDir.y : lookDir.y;
        
        gluLookAt(
            m_position.x, cameraHeight, m_position.z,  // Camera position
            m_position.x + lookDir.x, cameraHeight + verticalLook, m_position.z + lookDir.z,  // Look target
            0.0f, 1.0f, 0.0f  // Up vector
        );
    } else {
        // Third-person camera
        float cameraDistance = m_config.getCameraDistance();
        float cameraVerticalOffset = m_config.getCameraVerticalOffset();
        
        // Reverse the direction vector and scale by distance for position behind player
        sf::Vector3f cameraOffset;
        cameraOffset.x = -lookDir.x * cameraDistance;
        cameraOffset.z = -lookDir.z * cameraDistance;
        cameraOffset.y = cameraVerticalOffset;  // Camera height above player
        
        // Calculate final camera position
        sf::Vector3f cameraPos = m_position + cameraOffset;
        
        // Set up the camera view - look at the player or slightly ahead
        gluLookAt(
            cameraPos.x, cameraHeight + cameraVerticalOffset, cameraPos.z,  // Camera position
            m_position.x, cameraHeight, m_position.z,  // Look at position
            0.0f, 1.0f, 0.0f  // Up vector
        );
    }
}

void Player::render()
{
    // Only render the player model in third-person mode
    if (!m_config.getFirstPersonMode()) {
        // Save current matrix state
        glPushMatrix();
        
        // Move to player position
        glTranslatef(m_position.x, m_position.y, m_position.z);
        
        // Apply ONLY the Y rotation (yaw) to match camera's horizontal look direction
        glRotatef(-m_rotation.y, 0.0f, 1.0f, 0.0f);  // Yaw (left/right) only
        
        // Draw the player model
        drawPlayerModel();
        
        // Restore matrix state
        glPopMatrix();
    }
    
    // If in debug mode, draw additional debug info regardless of camera mode
    if (m_debugMode) {
        drawDebugInfo();
    }
}

void Player::drawPlayerModel()
{
    // Player dimensions from config
    float radius = m_config.getPlayerRadius();
    float height = m_config.getPlayerHeight();
    float totalHeight = height + 2 * radius; // Total height including hemispheres
    
    // Number of segments for smoothness
    int segments = 16;         // Around the circumference
    int rings = 8;             // For the hemispheres
    
    // Colors based on state
    float bodyColor[3] = {0.2f, 0.2f, 0.8f}; // Default blue
    
    // Change color based on state
    switch (m_state) {
        case State::Running:
            bodyColor[0] = 0.8f; bodyColor[1] = 0.2f; bodyColor[2] = 0.2f; // Red for running
            break;
        case State::Jumping:
        case State::Falling:
            bodyColor[0] = 0.2f; bodyColor[1] = 0.8f; bodyColor[2] = 0.2f; // Green for jumping/falling
            break;
        case State::Crouching:
            bodyColor[0] = 0.8f; bodyColor[1] = 0.8f; bodyColor[2] = 0.2f; // Yellow for crouching
            break;
        default:
            break; // Keep default blue
    }
    
    // Draw the cylindrical part of the capsule
    glPushMatrix();
    
    // Offset to center the capsule
    glTranslatef(0.0f, radius, 0.0f);
    
    // Apply animation effects
    switch (m_state) {
        case State::Walking:
        case State::Running:
            // Slight bobbing motion while walking/running
            glTranslatef(0.0f, sin(m_animTime * 10.0f) * 0.05f, 0.0f);
            break;
        case State::Jumping:
            // Stretch upward while jumping
            glScalef(0.9f, 1.1f, 0.9f);
            break;
        case State::Falling:
            // Stretch downward while falling
            glScalef(0.9f, 0.9f, 0.9f);
            break;
        case State::Crouching:
            // Squash while crouching
            glScalef(1.1f, 0.7f, 1.1f);
            break;
        default:
            // Slight breathing animation for idle
            glScalef(1.0f + sin(m_animTime * 2.0f) * 0.01f,
                     1.0f + sin(m_animTime * 2.0f) * 0.01f,
                     1.0f + sin(m_animTime * 2.0f) * 0.01f);
            break;
    }
    
    // Set the color
    glColor3f(bodyColor[0], bodyColor[1], bodyColor[2]);
    
    // Draw the cylinder
    glBegin(GL_QUADS);
    for (int i = 0; i < segments; i++) {
        float angle1 = ((float)i / segments) * 2.0f * 3.14159f;
        float angle2 = ((float)(i+1) / segments) * 2.0f * 3.14159f;
        
        float x1 = radius * sin(angle1);
        float z1 = radius * cos(angle1);
        float x2 = radius * sin(angle2);
        float z2 = radius * cos(angle2);
        
        // Quad for one segment of the cylinder
        glVertex3f(x1, 0.0f, z1);
        glVertex3f(x2, 0.0f, z2);
        glVertex3f(x2, height, z2);
        glVertex3f(x1, height, z1);
    }
    glEnd();
    
    // Draw hemispheres - omitted for brevity
    
    // Restore matrix state
    glPopMatrix();
    
    // Draw direction line
    float lineLength = 2.0f * radius;
    float lineColor[3] = {1.0f, 0.0f, 0.0f}; // Red for direction
    
    glLineWidth(2.0f);
    glColor3f(lineColor[0], lineColor[1], lineColor[2]);
    
    glBegin(GL_LINES);
    glVertex3f(0.0f, totalHeight / 2.0f, 0.0f);
    glVertex3f(0.0f, totalHeight / 2.0f, -lineLength);
    glEnd();
    
    // Reset line width
    glLineWidth(1.0f);
}

void Player::drawDebugInfo()
{
    // Draw a bounding box around the player for debugging
    float radius = m_config.getPlayerRadius();
    float height = m_config.getPlayerHeight();
    float totalHeight = height + 2 * radius;

    // Save current matrix
    glPushMatrix();

    // Move to player position
    glTranslatef(m_position.x, m_position.y, m_position.z);

    // Set color for debug info
    glColor3f(1.0f, 1.0f, 1.0f); // White

    // Draw axis lines
    glBegin(GL_LINES);
    // X axis (red)
    glColor3f(1.0f, 0.0f, 0.0f);
    glVertex3f(0.0f, 0.0f, 0.0f);
    glVertex3f(radius * 2.0f, 0.0f, 0.0f);

    // Y axis (green)
    glColor3f(0.0f, 1.0f, 0.0f);
    glVertex3f(0.0f, 0.0f, 0.0f);
    glVertex3f(0.0f, totalHeight, 0.0f);

    // Z axis (blue)
    glColor3f(0.0f, 0.0f, 1.0f);
    glVertex3f(0.0f, 0.0f, 0.0f);
    glVertex3f(0.0f, 0.0f, radius * 2.0f);
    glEnd();

    // Draw velocity vector
    glColor3f(1.0f, 1.0f, 0.0f); // Yellow
    glBegin(GL_LINES);
    glVertex3f(0.0f, radius, 0.0f);
    glVertex3f(m_velocity.x, radius + m_velocity.y, m_velocity.z);
    glEnd();

    // Restore matrix
    glPopMatrix();
}

void Player::damage(float amount)
{
    if (amount <= 0.0f) return;

    m_health -= amount;

    // Clamp health to 0-100
    if (m_health < 0.0f) m_health = 0.0f;

    std::cout << "[INFO] Player took " << amount << " damage. Health: " << m_health << std::endl;
}

void Player::playFootstepSound()
{
    // Placeholder for actual sound system implementation
    if (m_debugMode) {
        std::string surfaceType = "wood"; // In a real game, this would be determined by the surface
        std::cout << "[SOUND] Footstep on " << surfaceType << std::endl;
    }
}

void Player::playJumpSound()
{
    // Placeholder for actual sound system implementation
    if (m_debugMode) {
        std::cout << "[SOUND] Jump" << std::endl;
    }
}

void Player::playLandingSound()
{
    // Placeholder for actual sound system implementation
    if (m_debugMode) {
        std::cout << "[SOUND] Landing impact" << std::endl;
    }
}