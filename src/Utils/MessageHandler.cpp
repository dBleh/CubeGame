#include "MessageHandler.h"
#include <sstream>
#include <vector>
#include <cstdlib>

// Helper function: Split a string by a given delimiter.
static std::vector<std::string> splitString(const std::string& s, char delimiter) {
    std::vector<std::string> tokens;
    std::stringstream ss(s);
    std::string token;
    while (std::getline(ss, token, delimiter)) {
        tokens.push_back(token);
    }
    return tokens;
}

std::string MessageHandler::FormatConnectionMessage(const std::string& steamID, const std::string& steamName, const sf::Color& color) {
    // Create a connection message in the format: "C|steamID|steamName|r,g,b"
    std::stringstream ss;
    ss << "C|" << steamID << "|" << steamName << "|" 
       << static_cast<int>(color.r) << "," 
       << static_cast<int>(color.g) << "," 
       << static_cast<int>(color.b);
    return ss.str();
}

std::string MessageHandler::FormatMovementMessage(const std::string& steamID, const sf::Vector2f& position) {
    // Create a movement message in the format: "P|steamID|x|y"
    std::stringstream ss;
    ss << "P|" << steamID << "|" << position.x << "|" << position.y;
    return ss.str();
}

ParsedMessage MessageHandler::ParseMessage(const std::string& msg) {
    ParsedMessage parsed;
    auto parts = splitString(msg, '|');
    if (parts.empty()) {
        parsed.type = MessageType::Unknown;
        return parsed;
    }
    
    // Identify the message type based on the first character.
    char msgType = parts[0][0];
    if (msgType == 'C') {
        parsed.type = MessageType::Connection;
        if (parts.size() >= 4) {
            parsed.steamID = parts[1];
            parsed.steamName = parts[2];
            // Parse the color from the format "r,g,b"
            auto colorParts = splitString(parts[3], ',');
            if (colorParts.size() >= 3) {
                int r = std::stoi(colorParts[0]);
                int g = std::stoi(colorParts[1]);
                int b = std::stoi(colorParts[2]);
                parsed.color = sf::Color(r, g, b);
            }
        }
    } else if (msgType == 'P') {
        parsed.type = MessageType::Movement;
        if (parts.size() >= 4) {
            parsed.steamID = parts[1];
            float x = std::stof(parts[2]);
            float y = std::stof(parts[3]);
            parsed.position = sf::Vector2f(x, y);
        }
    } else {
        parsed.type = MessageType::Unknown;
    }
    
    return parsed;
}
