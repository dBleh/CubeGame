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
      m_debugMode(false),
      m_segments(16)
{
    initBodyProportions();
}

Player::~Player()
{
    // Nothing to clean up - we don't own any resources
}

void Player::initBodyProportions()
{
    float radius = m_config.getPlayerRadius();
    float height = m_config.getPlayerHeight();
    
    // Set default body proportions based on player radius and height
    m_bodyProps.headRadius = radius * 0.6f;
    m_bodyProps.bodyWidth = radius * 0.4f;
    m_bodyProps.limbWidth = radius * 0.2f;
    m_bodyProps.torsoLength = height * 0.4f;
    m_bodyProps.upperArmLength = m_bodyProps.torsoLength * 0.5f;
    m_bodyProps.forearmLength = m_bodyProps.torsoLength * 0.45f;
    m_bodyProps.thighLength = height * 0.25f;
    m_bodyProps.calfLength = height * 0.25f;
    m_bodyProps.shoulderWidth = m_bodyProps.bodyWidth * 2.0f;
    
    // Calculate dependent values
    m_totalLegLength = m_bodyProps.thighLength + m_bodyProps.calfLength;
}

void Player::setBodyProportion(const std::string& propName, float value)
{
    if (propName == "scale") m_bodyProps.scale = value;
    else if (propName == "headRadius") m_bodyProps.headRadius = value;
    else if (propName == "bodyWidth") m_bodyProps.bodyWidth = value;
    else if (propName == "limbWidth") m_bodyProps.limbWidth = value;
    else if (propName == "headHeight") m_bodyProps.headHeight = value;
    else if (propName == "torsoLength") m_bodyProps.torsoLength = value;
    else if (propName == "shoulderWidth") m_bodyProps.shoulderWidth = value;
    else if (propName == "hipWidth") m_bodyProps.hipWidth = value;
    else if (propName == "shoulderHeight") m_bodyProps.shoulderHeight = value;
    else if (propName == "upperArmLength") m_bodyProps.upperArmLength = value;
    else if (propName == "forearmLength") m_bodyProps.forearmLength = value;
    else if (propName == "thighLength") m_bodyProps.thighLength = value;
    else if (propName == "calfLength") m_bodyProps.calfLength = value;
    else if (propName == "shoulderOffsetZ") m_bodyProps.shoulderOffsetZ = value;
    else if (propName == "hipOffsetZ") m_bodyProps.hipOffsetZ = value;
    
    // Recalculate dependent values
    m_totalLegLength = m_bodyProps.thighLength + m_bodyProps.calfLength;
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
        // Keep grounded state true while timer is active
        // Important: Don't set to false here; let physics system handle that
        if (m_groundedTimer > 0) {
            m_isGrounded = true;
        }
    }
    
    // Handle user input and movement
    handleInput(deltaTime);
    
    // Update state based on velocity and grounded state
    if (!m_isGrounded) {
        // Add a small threshold to prevent flickering from tiny velocity changes
        if (m_velocity.y > 0.01f) {
            m_state = State::Jumping;
        } else if (m_velocity.y < -0.01f) {
            m_state = State::Falling;
        }
    } else if (m_isGrounded && m_state == State::Falling) {
        // We were falling but just landed
        playLandingSound();
        m_state = State::Idle;
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
        
        // Only reset the timer if we're not sitting on a platform
        // This is important to prevent flickering - don't reset the timer to 0 here
        // Let the physics system determine when we're actually off an edge
    }
    
    // Basic ground check (will be refined by physics system)
    if (m_position.y < m_config.getPlayerRadius()) {
        m_position.y = m_config.getPlayerRadius();
        m_velocity.y = 0;
        m_isGrounded = true;
        m_groundedTimer = 0.1f;  // Set grounded timer
        
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
        // Apply jump force - this is a fixed velocity, not additive
        m_velocity.y = m_config.getJumpForce();
        
        // Clear grounded state and timer
        m_isGrounded = false;
        m_groundedTimer = 0.0f;
        
        m_state = State::Jumping;
        playJumpSound();
        
        // Add a small upward position adjustment to avoid immediate collision with the ground
        // This small adjustment helps clear any ground collision for the next frame
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

void Player::drawPlayerModel() {
    // Colors based on state
    float bodyColor[3] = {0.2f, 0.2f, 0.8f}; // Default blue
    switch (m_state) {
        case State::Running:
            bodyColor[0] = 0.8f; bodyColor[1] = 0.2f; bodyColor[2] = 0.2f; // Red
            break;
        case State::Jumping:
        case State::Falling:
            bodyColor[0] = 0.2f; bodyColor[1] = 0.8f; bodyColor[2] = 0.2f; // Green
            break;
        case State::Crouching:
            bodyColor[0] = 0.8f; bodyColor[1] = 0.8f; bodyColor[2] = 0.2f; // Yellow
            break;
        default:
            break;
    }

    // Push matrix to isolate transformations
    glPushMatrix();
    
    // Apply overall scale
    glScalef(m_bodyProps.scale, m_bodyProps.scale, m_bodyProps.scale);

    // Apply animation effects to entire model
    switch (m_state) {
        case State::Walking:
        case State::Running:
            glTranslatef(0.0f, sin(m_animTime * 10.0f) * 0.05f, 0.0f);
            break;
        case State::Jumping:
            glScalef(0.9f, 1.1f, 0.9f);
            break;
        case State::Falling:
            glScalef(0.9f, 0.9f, 0.9f);
            break;
        case State::Crouching:
            glScalef(1.1f, 0.7f, 1.1f);
            break;
        default:
            glScalef(1.0f + sin(m_animTime * 2.0f) * 0.01f,
                     1.0f + sin(m_animTime * 2.0f) * 0.01f,
                     1.0f + sin(m_animTime * 2.0f) * 0.01f);
            break;
    }

    // Set the color
    glColor3f(bodyColor[0], bodyColor[1], bodyColor[2]);

    // Calculate animation parameters
    float armSwingLeft, armSwingRight, legSwingLeft, legSwingRight, elbowBend, kneeBend;
    float movementFactor = m_moveBackward ? -1.0f : 1.0f;

    if (m_state == State::Walking || m_state == State::Running) {
        float swingMultiplier = (m_state == State::Running) ? 1.5f : 1.0f;
        float animSpeed = 10.0f * swingMultiplier;
        float baseLegSwing = sin(m_animTime * animSpeed) * 30.0f * movementFactor;
        legSwingLeft = baseLegSwing;
        legSwingRight = -baseLegSwing;
        armSwingLeft = -baseLegSwing * (45.0f / 30.0f); // Opposite to legs, scaled
        armSwingRight = baseLegSwing * (45.0f / 30.0f);
        elbowBend = 15.0f + sin(m_animTime * animSpeed) * 10.0f;
        kneeBend = 10.0f + abs(sin(m_animTime * animSpeed)) * 40.0f; 
        
    } else if (m_state == State::Jumping) {
        armSwingLeft = 120.0f;
        armSwingRight = 120.0f;
        elbowBend = 45.0f;
        legSwingLeft = -15.0f;
        legSwingRight = -15.0f;
        kneeBend = 30.0f;
    } else if (m_state == State::Falling) {
        armSwingLeft = 30.0f;
        armSwingRight = 30.0f;
        elbowBend = 70.0f;
        legSwingLeft = 15.0f;
        legSwingRight = 15.0f;
        kneeBend = 60.0f;
    } else if (m_state == State::Crouching) {
        armSwingLeft = -20.0f;
        armSwingRight = -20.0f;
        elbowBend = 90.0f;
        legSwingLeft = -40.0f;
        legSwingRight = -40.0f;
        kneeBend = 110.0f;
    } else {
        armSwingLeft = 0.0f;
        armSwingRight = 0.0f;
        legSwingLeft = 0.0f;
        legSwingRight = 0.0f;
        elbowBend = 15.0f;
        kneeBend = 5.0f;
    }

    // Draw the model in order: legs, torso, head, arms
    drawLeg(true, legSwingLeft, kneeBend);
    drawLeg(false, legSwingRight, kneeBend);
    drawTorso();
    drawHead();
    drawArm(true, armSwingLeft, elbowBend);
    drawArm(false, armSwingRight, elbowBend);

    // Restore matrix
    glPopMatrix();

    // Draw direction indicator
    if (m_debugMode) {
        float lineLength = 2.0f * m_config.getPlayerRadius();
        float chestHeight = m_totalLegLength + (m_bodyProps.torsoLength * 0.5f);
        glLineWidth(2.0f);
        glColor3f(1.0f, 0.0f, 0.0f);
        glBegin(GL_LINES);
        glVertex3f(0.0f, chestHeight, 0.0f);
        glVertex3f(0.0f, chestHeight, -lineLength);
        glEnd();
        glLineWidth(1.0f);
    }
}


void Player::drawHead() {
    glPushMatrix();
    // Position head at top of torso plus head height offset
    glTranslatef(0.0f, 
                m_totalLegLength + m_bodyProps.torsoLength + m_bodyProps.headHeight, 
                0.0f);
    
    GLUquadric* quadric = gluNewQuadric();
    gluQuadricDrawStyle(quadric, GLU_FILL);
    gluSphere(quadric, m_bodyProps.headRadius, m_segments, m_segments);
    gluDeleteQuadric(quadric);
    glPopMatrix();
}

void Player::drawTorso() {
    glPushMatrix();
    // Position torso at top of legs
    glTranslatef(0.0f, m_totalLegLength+ 1.0f, 0.0f);
    
    GLUquadric* quadric = gluNewQuadric();
    gluQuadricDrawStyle(quadric, GLU_FILL);
    glRotatef(90.0f, 1.0f, 0.0f, 0.0f);
    
    // Draw main torso
    gluCylinder(quadric, m_bodyProps.bodyWidth, m_bodyProps.bodyWidth, 
                m_bodyProps.torsoLength, m_segments, 1);
    
    
    
    gluDeleteQuadric(quadric);
    glPopMatrix();
}

void Player::drawArm(bool isLeft, float armSwing, float elbowBend) {
    float shoulderX = isLeft ? -m_bodyProps.shoulderWidth * 0.5f : m_bodyProps.shoulderWidth * 0.5f;
    
    glPushMatrix();
    // Position at shoulder height on torso
    glTranslatef(shoulderX, 
                m_totalLegLength + m_bodyProps.torsoLength * m_bodyProps.shoulderHeight, 
                m_bodyProps.shoulderOffsetZ);
    
    // Swing around x-axis (yz-plane)
    glRotatef(armSwing, 1.0f, 0.0f, 0.0f);
    
    // Shoulder joint
    GLUquadric* quadric = gluNewQuadric();
    gluSphere(quadric, m_bodyProps.limbWidth * 1.2f, 8, 8);
    gluDeleteQuadric(quadric);
    
    // Upper arm (along -y)
    glPushMatrix();
    glRotatef(-90.0f, 1.0f, 0.0f, 0.0f); // z to -y
    quadric = gluNewQuadric();
    gluCylinder(quadric, m_bodyProps.limbWidth, m_bodyProps.limbWidth, 
                m_bodyProps.upperArmLength, 8, 1);
    gluDeleteQuadric(quadric);
    glPopMatrix();
    
    // Move to elbow and bend
    glTranslatef(0.0f, -m_bodyProps.upperArmLength, 0.0f);
    glRotatef(elbowBend, 1.0f, 0.0f, 0.0f);
    
    // Elbow joint
    quadric = gluNewQuadric();
    gluSphere(quadric, m_bodyProps.limbWidth * 1.2f, 8, 8);
    gluDeleteQuadric(quadric);
    
    // Forearm
    glPushMatrix();
    glRotatef(-90.0f, 1.0f, 0.0f, 0.0f);
    quadric = gluNewQuadric();
    gluCylinder(quadric, m_bodyProps.limbWidth, m_bodyProps.limbWidth, 
                m_bodyProps.forearmLength, 8, 1);
    gluDeleteQuadric(quadric);
    glPopMatrix();
    
    // Hand could be added here
    
    glPopMatrix();
}

void Player::drawLeg(bool isLeft, float legSwing, float kneeBend) {
    float hipX = isLeft ? -m_bodyProps.hipWidth * 0.5f : m_bodyProps.hipWidth * 0.5f;
    
    glPushMatrix();
    // Position at hip
    glTranslatef(hipX, m_totalLegLength, m_bodyProps.hipOffsetZ);
    
    // Swing around x-axis (yz-plane)
    glRotatef(legSwing, 1.0f, 0.0f, 0.0f);
    
    // Hip joint
    GLUquadric* quadric = gluNewQuadric();
    gluSphere(quadric, m_bodyProps.limbWidth * 1.2f, 8, 8);
    gluDeleteQuadric(quadric);
    
    // Thigh (along -y)
    glPushMatrix();
    glRotatef(-90.0f, 1.0f, 0.0f, 0.0f);
    quadric = gluNewQuadric();
    gluCylinder(quadric, m_bodyProps.limbWidth, m_bodyProps.limbWidth, 
                m_bodyProps.thighLength, 8, 1);
    gluDeleteQuadric(quadric);
    glPopMatrix();
    
    // Move to knee and bend
    glTranslatef(0.0f, -m_bodyProps.thighLength, 0.0f);
    
    // Knee joint
    quadric = gluNewQuadric();
    gluSphere(quadric, m_bodyProps.limbWidth * 1.2f, 8, 8);
    gluDeleteQuadric(quadric);
    
    glRotatef(kneeBend, 1.0f, 0.0f, 0.0f);
    
    // Calf
    glPushMatrix();
    glRotatef(-90.0f, 1.0f, 0.0f, 0.0f);
    quadric = gluNewQuadric();
    gluCylinder(quadric, m_bodyProps.limbWidth, m_bodyProps.limbWidth, 
                m_bodyProps.calfLength, 8, 1);
    gluDeleteQuadric(quadric);
    glPopMatrix();
    
    // Foot could be added here
    
    glPopMatrix();
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