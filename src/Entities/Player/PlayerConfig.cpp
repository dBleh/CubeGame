#include "PlayerConfig.h"
#include <fstream>
#include <sstream>
#include <iostream>

PlayerConfig::PlayerConfig()
{
    resetToDefaults();
}

PlayerConfig::~PlayerConfig()
{
}

void PlayerConfig::resetToDefaults()
{
    // Movement settings
    m_moveSpeed = 5.0f;
    m_rotationSpeed = 100.0f;
    m_jumpForce = 5.0f;
    m_gravity = 9.8f;
    
    // Camera settings
    m_cameraHeight = 2.0f;
    m_mouseSensitivity = 0.1f;
    m_invertMouse = true;
    m_fieldOfView = 75.0f;
    m_cameraDistance = 3.0f;
    m_cameraVerticalOffset = 1.0f;
    m_firstPersonMode = false;    // Default to third-person view
    
    // Physics settings
    m_playerRadius = 0.3f;
    m_playerHeight = 1.0f;
    m_groundFriction = 6.0f;
    m_airResistance = 0.5f;
    
    // Network settings
    m_positionSyncRate = 0.05f;  // 20 times per second
    m_useInterpolation = true;
    m_interpolationFactor = 0.8f;
}

bool PlayerConfig::loadFromFile(const std::string& filepath)
{
    std::ifstream file(filepath);
    if (!file.is_open())
    {
        std::cerr << "[ERROR] Could not open config file: " << filepath << std::endl;
        return false;
    }
    
    // Reset to defaults first
    resetToDefaults();
    
    std::string line;
    while (std::getline(file, line))
    {
        // Skip empty lines and comments
        if (line.empty() || line[0] == '#' || line[0] == '/')
            continue;
            
        if (!parseConfigLine(line))
        {
            std::cerr << "[WARNING] Failed to parse config line: " << line << std::endl;
        }
    }
    
    file.close();
    return true;
}

bool PlayerConfig::saveToFile(const std::string& filepath) const
{
    std::ofstream file(filepath);
    if (!file.is_open())
    {
        std::cerr << "[ERROR] Could not open config file for writing: " << filepath << std::endl;
        return false;
    }
    
    // File header
    file << "# Player Configuration File" << std::endl;
    file << "# Generated automatically" << std::endl << std::endl;
    
    // Movement settings
    file << "# Movement Settings" << std::endl;
    file << "moveSpeed=" << m_moveSpeed << std::endl;
    file << "rotationSpeed=" << m_rotationSpeed << std::endl;
    file << "jumpForce=" << m_jumpForce << std::endl;
    file << "gravity=" << m_gravity << std::endl << std::endl;
    
    // Camera settings
    file << "# Camera Settings" << std::endl;
    file << "cameraHeight=" << m_cameraHeight << std::endl;
    file << "mouseSensitivity=" << m_mouseSensitivity << std::endl;
    file << "invertMouse=" << (m_invertMouse ? "true" : "false") << std::endl;
    file << "fieldOfView=" << m_fieldOfView << std::endl;
    file << "cameraDistance=" << m_cameraDistance << std::endl;
    file << "cameraVerticalOffset=" << m_cameraVerticalOffset << std::endl;
    file << "firstPersonMode=" << (m_firstPersonMode ? "true" : "false") << std::endl << std::endl;
    
    // Physics settings
    file << "# Physics Settings" << std::endl;
    file << "playerRadius=" << m_playerRadius << std::endl;
    file << "playerHeight=" << m_playerHeight << std::endl;
    file << "groundFriction=" << m_groundFriction << std::endl;
    file << "airResistance=" << m_airResistance << std::endl << std::endl;
    
    // Network settings
    file << "# Network Settings" << std::endl;
    file << "positionSyncRate=" << m_positionSyncRate << std::endl;
    file << "useInterpolation=" << (m_useInterpolation ? "true" : "false") << std::endl;
    file << "interpolationFactor=" << m_interpolationFactor << std::endl;
    
    file.close();
    return true;
}

std::string PlayerConfig::serialize() const
{
    std::stringstream ss;
    
    // Format all settings as key-value pairs
    ss << "moveSpeed:" << m_moveSpeed << ";";
    ss << "rotationSpeed:" << m_rotationSpeed << ";";
    ss << "jumpForce:" << m_jumpForce << ";";
    ss << "gravity:" << m_gravity << ";";
    ss << "cameraHeight:" << m_cameraHeight << ";";
    ss << "cameraDistance:" << m_cameraDistance << ";";
    ss << "cameraVerticalOffset:" << m_cameraVerticalOffset << ";";
    ss << "mouseSensitivity:" << m_mouseSensitivity << ";";
    ss << "invertMouse:" << (m_invertMouse ? "1" : "0") << ";";
    ss << "fieldOfView:" << m_fieldOfView << ";";
    ss << "firstPersonMode:" << (m_firstPersonMode ? "1" : "0") << ";";
    ss << "playerRadius:" << m_playerRadius << ";";
    ss << "playerHeight:" << m_playerHeight << ";";
    ss << "groundFriction:" << m_groundFriction << ";";
    ss << "airResistance:" << m_airResistance << ";";
    ss << "positionSyncRate:" << m_positionSyncRate << ";";
    ss << "useInterpolation:" << (m_useInterpolation ? "1" : "0") << ";";
    ss << "interpolationFactor:" << m_interpolationFactor;
    
    return ss.str();
}

bool PlayerConfig::deserialize(const std::string& data)
{
    std::stringstream ss(data);
    std::string pair;
    
    // Reset to defaults first
    resetToDefaults();
    
    // Parse semicolon-separated key-value pairs
    while (std::getline(ss, pair, ';'))
    {
        if (!parseConfigLine(pair))
        {
            std::cerr << "[WARNING] Failed to parse config data: " << pair << std::endl;
        }
    }
    
    return true;
}

bool PlayerConfig::parseConfigLine(const std::string& line)
{
    std::size_t separatorPos = line.find('=');
    if (separatorPos == std::string::npos)
    {
        // Try with colon (used in serialized format)
        separatorPos = line.find(':');
        if (separatorPos == std::string::npos)
            return false;
    }
    
    std::string key = line.substr(0, separatorPos);
    std::string value = line.substr(separatorPos + 1);
    
    // Parse the value based on the key
    try {
        if (key == "moveSpeed")
            m_moveSpeed = std::stof(value);
        else if (key == "rotationSpeed")
            m_rotationSpeed = std::stof(value);
        else if (key == "jumpForce")
            m_jumpForce = std::stof(value);
        else if (key == "gravity")
            m_gravity = std::stof(value);
        else if (key == "cameraHeight")
            m_cameraHeight = std::stof(value);
        else if (key == "cameraDistance")
            m_cameraDistance = std::stof(value);
        else if (key == "cameraVerticalOffset")
            m_cameraVerticalOffset = std::stof(value);
        else if (key == "mouseSensitivity")
            m_mouseSensitivity = std::stof(value);
        else if (key == "invertMouse")
            m_invertMouse = (value == "true" || value == "1");
        else if (key == "fieldOfView")
            m_fieldOfView = std::stof(value);
        else if (key == "firstPersonMode")
            m_firstPersonMode = (value == "true" || value == "1");
        else if (key == "playerRadius")
            m_playerRadius = std::stof(value);
        else if (key == "playerHeight")
            m_playerHeight = std::stof(value);
        else if (key == "groundFriction")
            m_groundFriction = std::stof(value);
        else if (key == "airResistance")
            m_airResistance = std::stof(value);
        else if (key == "positionSyncRate")
            m_positionSyncRate = std::stof(value);
        else if (key == "useInterpolation")
            m_useInterpolation = (value == "true" || value == "1");
        else if (key == "interpolationFactor")
            m_interpolationFactor = std::stof(value);
        else
            return false; // Unknown key
    }
    catch (const std::exception& e) {
        std::cerr << "[ERROR] Failed to parse value '" << value << "' for key '" << key << "': " << e.what() << std::endl;
        return false;
    }
    
    return true;
}