#include "Player.h"
#include <cmath>
#include "../Core/CubeGame.h" // For access to game functions and context

/**
 * @brief Initializes the player with default values.
 *
 * Sets up the player's shape, initial position, movement speed, health,
 * currency, and ready status.
 */
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

/**
 * @brief Handles movement based on keyboard input.
 *
 * Updates the player's position using WASD keys and sets the shape's position.
 *
 * @param dt Delta time since last update.
 * @return True if movement occurred, false otherwise.
 */
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

/**
 * @brief Applies a speed boost to the player.
 *
 * Increases the player's speed by the specified boost amount.
 *
 * @param boostAmount Amount to increase the speed.
 */
void Player::applySpeedBoost(float boostAmount) {
    speed += boostAmount;
    std::cout << "[DEBUG] Speed Boost Applied: " << speed << std::endl;
}

/**
 * @brief Shoots a bullet from the player.
 *
 * Computes the bullet's starting and target positions based on the player's position
 * and the current mouse location. Respects shoot cooldown and broadcasts the bullet
 * fire message in multiplayer mode.
 *
 * @param game Pointer to the CubeGame instance for context.
 */
void Player::ShootBullet(CubeGame* game) {
    // Do not shoot if on cooldown or if the player is not alive.
    if (game->GetShootCooldown() > 0 || !isAlive) {
        return;
    }

    // Define bullet start position offset from the player.
    float startX = x + 10.f;
    float startY = y + 10.f;

    // Convert mouse position to world coordinates.
    sf::Vector2i mousePos = sf::Mouse::getPosition(game->GetWindow());
    sf::Vector2f worldPos = game->GetWindow().mapPixelToCoords(mousePos, game->GetView());

    // Calculate direction from the center of the view.
    sf::View view = game->GetView();
    sf::Vector2f viewSize = view.getSize();

    float dx = static_cast<float>(worldPos.x) - (viewSize.x / 2.f);
    float dy = static_cast<float>(worldPos.y) - (viewSize.y / 2.f);

    // Normalize the direction vector.
    float length = std::sqrt(dx * dx + dy * dy);
    if (length > 0) {
        dx /= length;
        dy /= length;
    } else {
        dx = 0.f;
        dy = 0.f;
    }

    // Determine target far away in the shooting direction.
    const float rangeMultiplier = 2000.0f;
    float targetX = startX + dx * rangeMultiplier;
    float targetY = startY + dy * rangeMultiplier;

    // Create and initialize the bullet.
    Bullet b;
    b.initialize(startX, startY, targetX, targetY);
    b.lifetime = 2.0f;
    b.renderedX = startX;
    b.renderedY = startY;

    // Generate a unique bullet ID using the player's SteamID and a bullet counter.
    int bulletIdx = game->GetNextBulletId();
    uint64_t shooterSteamID = steamID.ConvertToUint64();
    b.id = (shooterSteamID << 32) | static_cast<uint32_t>(bulletIdx);
    game->GetNextBulletId()++;

    // Add the bullet to the EntityManager.
    game->GetEntityManager()->getBullets()[b.id] = b;
    // Reset shoot cooldown.
    game->GetShootCooldown() = 0.2f;

    // In multiplayer, broadcast the bullet fire message.
    if (game->GetPlayers().size() > 1) {
        static uint32_t localMessageCounter = 0;
        uint32_t messageID = (static_cast<uint32_t>(shooterSteamID & 0xFFFF) << 16) | (localMessageCounter++ & 0xFFFF);
        char buffer[128];
        int bytes = snprintf(buffer, sizeof(buffer), "B|fire|%u|%llu|%d|%.1f|%.1f|%.1f|%.1f|%.1f",
                             messageID, shooterSteamID, bulletIdx, b.x, b.y, targetX, targetY, b.lifetime);
        if (bytes > 0 && static_cast<size_t>(bytes) < sizeof(buffer)) {
            game->GetNetworkManager()->SendGameplayMessage(std::string(buffer));
        }
    }
}
