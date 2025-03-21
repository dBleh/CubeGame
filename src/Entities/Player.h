#ifndef PLAYER_H
#define PLAYER_H

#include <SFML/Graphics.hpp>
#include <steam/steam_api.h>  // Optional: if you want to associate a SteamID with the player

class Player {
public:
    // Default constructor with preset starting position, color, and movement speed.
    Player();
    // Constructor allowing to set starting position and color.
    Player(const sf::Vector2f& startPosition, const sf::Color& color = sf::Color::Blue);
    
    // Update the player's state (handle movement).
    void Update(float dt);
    
    // Get the player's current position.
    sf::Vector2f GetPosition() const;
    
    // Set the player's position.
    void SetPosition(const sf::Vector2f& pos);
    
    sf::RectangleShape& GetShape();
    // Add a const overload for read-only access (used when drawing, etc.).
    const sf::RectangleShape& GetShape() const;

    
    // Set the movement speed.
    void SetSpeed(float speed);
    // Get the movement speed.
    float GetSpeed() const;

private:
    sf::RectangleShape shape;  // Represents the player as a cube.
    float movementSpeed;       // Movement speed in pixels per second.
};

#endif // PLAYER_H
