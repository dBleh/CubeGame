#include "Bullet.h"
#include <cmath>

void Bullet::initialize(float startX, float startY, float targetX, float targetY) {
    x = startX;
    y = startY;
    renderedX = x;
    renderedY = y;
    const float speed = 500.0f;
    float dx = targetX - startX;
    float dy = targetY - startY;
    float length = std::sqrt(dx * dx + dy * dy);
    if (length > 0 && std::isfinite(dx) && std::isfinite(dy)) {
        velocityX = (dx / length) * speed;
        velocityY = (dy / length) * speed;
    } else {
        velocityX = 0.0f;
        velocityY = 0.0f;
    }
    shape.setSize(sf::Vector2f(5.f, 5.f));
    shape.setFillColor(sf::Color::Yellow);
    shape.setPosition(renderedX, renderedY);
    lifetime = 2.0f;
}

void Bullet::update(float dt) {
    x += velocityX * dt;
    y += velocityY * dt;

    float interpFactor = std::min(30.0f * dt, 1.0f); // Increased for faster catch-up
    float oldRenderedX = renderedX, oldRenderedY = renderedY;
    renderedX += (x - renderedX) * interpFactor;
    renderedY += (y - renderedY) * interpFactor;
    shape.setPosition(renderedX, renderedY);

    float dx = x - renderedX;
    float dy = y - renderedY;
    if (std::abs(dx) > 10 || std::abs(dy) > 10) {
        std::cout << "[DEBUG] Bullet " << id << " jump: (" << dx << ", " << dy << ") from (" << oldRenderedX << ", " << oldRenderedY << ") to (" << renderedX << ", " << renderedY << ")" << std::endl;
    }

    lifetime -= dt;
}