#pragma once

#include <string>
#include <unordered_map>
#include <SFML/System/Vector3.hpp>

/**
 * @class PlayerConfig
 * @brief Configuration class for player settings
 * 
 * This class loads and manages various player settings from a configuration file.
 * It provides defaults for all values and can be serialized for network transmission.
 */
class PlayerConfig {
public:
    PlayerConfig();
    ~PlayerConfig();

    // Load from file
    bool loadFromFile(const std::string& filepath);
    
    // Save to file
    bool saveToFile(const std::string& filepath) const;
    
    // Network serialization
    std::string serialize() const;
    bool deserialize(const std::string& data);
    
    // Movement settings
    float getMoveSpeed() const { return m_moveSpeed; }
    float getRotationSpeed() const { return m_rotationSpeed; }
    float getJumpForce() const { return m_jumpForce; }
    float getGravity() const { return m_gravity; }
    
    // Camera settings
    float getCameraHeight() const { return m_cameraHeight; }
    float getMouseSensitivity() const { return m_mouseSensitivity; }
    bool getInvertMouse() const { return m_invertMouse; }
    float getFOV() const { return m_fieldOfView; }
    float getCameraDistance() const { return m_cameraDistance; }
    float getCameraVerticalOffset() const { return m_cameraVerticalOffset; }
    bool getFirstPersonMode() const { return m_firstPersonMode; }
    
    // Physics settings
    float getPlayerRadius() const { return m_playerRadius; }
    float getPlayerHeight() const { return m_playerHeight; }
    float getGroundFriction() const { return m_groundFriction; }
    float getAirResistance() const { return m_airResistance; }
    
    // Network settings
    float getPositionSyncRate() const { return m_positionSyncRate; }
    bool getUseInterpolation() const { return m_useInterpolation; }
    float getInterpolationFactor() const { return m_interpolationFactor; }
    
    // Modify settings
    void setMoveSpeed(float value) { m_moveSpeed = value; }
    void setRotationSpeed(float value) { m_rotationSpeed = value; }
    void setJumpForce(float value) { m_jumpForce = value; }
    void setGravity(float value) { m_gravity = value; }
    void setCameraHeight(float value) { m_cameraHeight = value; }
    void setMouseSensitivity(float value) { m_mouseSensitivity = value; }
    void setInvertMouse(bool value) { m_invertMouse = value; }
    void setFOV(float value) { m_fieldOfView = value; }
    void setPlayerRadius(float value) { m_playerRadius = value; }
    void setPlayerHeight(float value) { m_playerHeight = value; }
    void setGroundFriction(float value) { m_groundFriction = value; }
    void setAirResistance(float value) { m_airResistance = value; }
    void setPositionSyncRate(float value) { m_positionSyncRate = value; }
    void setUseInterpolation(bool value) { m_useInterpolation = value; }
    void setInterpolationFactor(float value) { m_interpolationFactor = value; }
    void setCameraDistance(float value) { m_cameraDistance = value; }
    void setCameraVerticalOffset(float value) { m_cameraVerticalOffset = value; }
    void setFirstPersonMode(bool value) { m_firstPersonMode = value; }
    
    // Reset to defaults
    void resetToDefaults();

private:
    // Parse config values
    bool parseConfigLine(const std::string& line);
    
    // Movement settings
    float m_moveSpeed;            // Units per second
    float m_rotationSpeed;        // Degrees per second
    float m_jumpForce;            // Jump force
    float m_gravity;              // Gravity force
    
    // Camera settings
    float m_cameraHeight;         // Height of camera from player position
    float m_mouseSensitivity;     // Mouse movement multiplier
    bool m_invertMouse;           // Whether to invert mouse Y axis
    float m_fieldOfView;          // Field of view in degrees
    float m_cameraDistance;       // Distance behind player for third-person view
    float m_cameraVerticalOffset; // Height above player for third-person view
    bool m_firstPersonMode;       // Whether to use first-person (true) or third-person (false) view
    
    // Physics settings
    float m_playerRadius;         // Radius of player capsule
    float m_playerHeight;         // Height of player capsule (excluding hemispheres)
    float m_groundFriction;       // Friction coefficient when on ground
    float m_airResistance;        // Air resistance coefficient when jumping/falling
    
    // Network settings
    float m_positionSyncRate;     // How often to sync position (seconds)
    bool m_useInterpolation;      // Whether to use movement interpolation
    float m_interpolationFactor;  // Interpolation smoothing factor (0.0-1.0)
};