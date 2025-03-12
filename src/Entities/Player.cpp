#include "Player.h"

// Default constructor: initializes the cube with default position, color, and speed.
Player::Player()
    : movementSpeed(200.f) // 200 pixels per second
{
    shape.setSize(sf::Vector2f(50.f, 50.f));  // Cube of 50x50 pixels
    shape.setFillColor(sf::Color::Blue);
    shape.setPosition(100.f, 100.f);  // Default starting position
}

// Parameterized constructor.
Player::Player(const sf::Vector2f& startPosition, const sf::Color& color)
    : movementSpeed(200.f)
{
    shape.setSize(sf::Vector2f(50.f, 50.f));
    shape.setFillColor(color);
    shape.setPosition(startPosition);
}

// Update method: moves the player based on keyboard input (WASD).
void Player::Update(float dt)
{
    sf::Vector2f movement(0.f, 0.f);
    
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::W))
        movement.y -= movementSpeed * dt;
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::S))
        movement.y += movementSpeed * dt;
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::A))
        movement.x -= movementSpeed * dt;
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::D))
        movement.x += movementSpeed * dt;
    
    shape.move(movement);
}

sf::Vector2f Player::GetPosition() const
{
    return shape.getPosition();
}

void Player::SetPosition(const sf::Vector2f& pos)
{
    shape.setPosition(pos);
}

sf::RectangleShape& Player::GetShape() {
    return shape;
}

const sf::RectangleShape& Player::GetShape() const {
    return shape;
}

void Player::SetSpeed(float speed)
{
    movementSpeed = speed;
}

float Player::GetSpeed() const
{
    return movementSpeed;
}
