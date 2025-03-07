#include "Player.h"
#include <cmath>

void Player::initialize() {
    shape.setSize(sf::Vector2f(20.0f, 20.0f));
    shape.setFillColor(sf::Color::Blue);
    x = SCREEN_WIDTH / 2.f;
    y = SCREEN_HEIGHT / 2.f;
    renderedX = x;
    renderedY = y;
    speed = PLAYER_SPEED;
    money = 0;
    health = 100;
    kills = 0;
    ready = false;
}

bool Player::move(float dt) {
    bool moved = false;
    float effectiveSpeed = speed > 0 ? speed : PLAYER_SPEED;
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::W)) { y -= effectiveSpeed * dt; moved = true; }
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::S)) { y += effectiveSpeed * dt; moved = true; }
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::A)) { x -= effectiveSpeed * dt; moved = true; }
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::D)) { x += effectiveSpeed * dt; moved = true; }
    renderedX = x;
    renderedY = y;
    shape.setPosition(renderedX, renderedY);
    return moved;
}

void Player::applySpeedBoost(float boostAmount) {
    speed += boostAmount;
    std::cout << "[DEBUG] Speed Boost Applied: " << speed << std::endl;
}