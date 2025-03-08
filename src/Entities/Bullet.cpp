#include "Bullet.h"
#include <cmath>

/**
 * @brief Initializes the bullet with starting and target positions.
 *
 * This sets the initial position, computes the velocity based on the target,
 * configures the bullet's visual properties, and sets its lifetime.
 */
void Bullet::initialize(float startX, float startY, float targetX, float targetY) {
    // Set initial logical and rendered positions.
    x = startX;
    y = startY;
    renderedX = x;
    renderedY = y;

    // Calculate normalized velocity vector scaled by bullet speed.
    const float speed = 500.0f;  // Hardcoded bullet speed; consider using a constant if defined.
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

    // Set up the bullet's shape.
    shape.setSize(sf::Vector2f(5.f, 5.f));
    shape.setFillColor(sf::Color::Yellow);
    shape.setPosition(renderedX, renderedY);

    // Set initial lifetime (in seconds).
    lifetime = 2.0f;
}

/**
 * @brief Updates the bullet's position and visual interpolation.
 *
 * Moves the bullet according to its velocity, updates the rendered position
 * with interpolation, and decreases the lifetime.
 *
 * @param dt Delta time (time elapsed since the last update).
 */
void Bullet::update(float dt) {
    // Update logical position based on velocity.
    x += velocityX * dt;
    y += velocityY * dt;

    // Interpolate the rendered position for smooth visuals.
    float interpFactor = std::min(30.0f * dt, 1.0f); // Fast catch-up factor
    float oldRenderedX = renderedX, oldRenderedY = renderedY;
    renderedX += (x - renderedX) * interpFactor;
    renderedY += (y - renderedY) * interpFactor;
    shape.setPosition(renderedX, renderedY);

    // Debug log if the bullet "jumps" significantly.
    float dx = x - renderedX;
    float dy = y - renderedY;
    if (std::abs(dx) > 10 || std::abs(dy) > 10) {
        std::cout << "[DEBUG] Bullet " << id << " jump: (" << dx << ", " << dy << ") from ("
                  << oldRenderedX << ", " << oldRenderedY << ") to ("
                  << renderedX << ", " << renderedY << ")" << std::endl;
    }

    // Decrease lifetime.
    lifetime -= dt;
}
