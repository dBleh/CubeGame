#ifndef MESSAGEHANDLER_H
#define MESSAGEHANDLER_H

#include <string>
#include <SFML/Graphics.hpp>

// Define the message types we support.
enum class MessageType {
    Connection,
    Movement,
    Unknown
};

// A structure to hold parsed message data.
struct ParsedMessage {
    MessageType type;
    std::string steamID;      // For both connection and movement messages.
    std::string steamName;    // Only for connection messages.
    sf::Color color;          // Only for connection messages.
    sf::Vector2f position;    // Only for movement messages.
};

class MessageHandler {
public:
    // Format a connection message.
    // Format: "C|steamID|steamName|r,g,b"
    static std::string FormatConnectionMessage(const std::string& steamID, const std::string& steamName, const sf::Color& color);
    
    // Format a movement message.
    // Format: "P|steamID|x|y"
    static std::string FormatMovementMessage(const std::string& steamID, const sf::Vector2f& position);
    
    // Parse an incoming message and return a ParsedMessage struct.
    static ParsedMessage ParseMessage(const std::string& msg);
};

#endif // MESSAGE_HANDLER_H
