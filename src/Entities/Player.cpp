#include "Player.h"
#include <cmath>

void Player::initialize() {
    shape.setSize(sf::Vector2f(20.0f, 20.0f));
    shape.setFillColor(sf::Color::Blue); // Distinct player color
    x = SCREEN_WIDTH / 2.f;
    y = SCREEN_HEIGHT / 2.f;
    renderedX = x;
    renderedY = y;
}

bool Player::move(float dt) {
    bool moved = false;
    float speed = 200.0f;
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::W)) { y -= speed * dt; moved = true; }
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::S)) { y += speed * dt; moved = true; }
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::A)) { x -= speed * dt; moved = true; }
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::D)) { x += speed * dt; moved = true; }
    renderedX = x; // Simplified for now; interpolation handled elsewhere
    renderedY = y;
    shape.setPosition(renderedX, renderedY);
    return moved;
}