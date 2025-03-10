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
    isAlive = true;

    // Initialize orbiting cube
    orbitingCube.angle = 0.0f;
    orbitingCube.x = x + orbitingCube.radius * std::cos(orbitingCube.angle);
    orbitingCube.y = y + orbitingCube.radius * std::sin(orbitingCube.angle);
    orbitingCube.renderedX = orbitingCube.x;
    orbitingCube.renderedY = orbitingCube.y;
    orbitingCube.lastX = orbitingCube.x;
    orbitingCube.lastY = orbitingCube.y;
    orbitingCube.shape.setPosition(orbitingCube.renderedX, orbitingCube.renderedY);
    orbitingCube.active = true;

    // Set start timestamp for continuous rotation
    startTimestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    lastUpdateTimestamp = startTimestamp;
}

void Player::updateOrbitingCube(float dt) {
    if (!orbitingCube.active || !isAlive) return;

    orbitingCube.lastX = orbitingCube.x;
    orbitingCube.lastY = orbitingCube.y;

    // Get current time in milliseconds
    uint64_t now = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();

    // Calculate total elapsed time since start in seconds
    float elapsedTime = (now - startTimestamp) / 1000.0f;

    // Update angle based on total elapsed time
    orbitingCube.angle = orbitingCube.angularSpeed * elapsedTime;

    // Keep angle within [0, 2π)
    orbitingCube.angle = std::fmod(orbitingCube.angle, 2 * M_PI);

    // Update position based on player's current position and angle
    orbitingCube.x = x + orbitingCube.radius * std::cos(orbitingCube.angle);
    orbitingCube.y = y + orbitingCube.radius * std::sin(orbitingCube.angle);
}

sf::FloatRect Player::getOrbitingCubeBounds() const {
    return sf::FloatRect(orbitingCube.renderedX, orbitingCube.renderedY,
                         orbitingCube.shape.getSize().x, orbitingCube.shape.getSize().y);
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
    lastX = x; // Store previous position
    lastY = y;
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::W)) { y -= effectiveSpeed * dt; moved = true; }
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::S)) { y += effectiveSpeed * dt; moved = true; }
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::A)) { x -= effectiveSpeed * dt; moved = true; }
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::D)) { x += effectiveSpeed * dt; moved = true; }
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
    if (!isAlive || game->GetShootCooldown()> 0) return;

    Bullet b;
    b.id = id;
    b.shooterSteamID = steamID;
    static uint64_t bulletId = 0;
    b.id = bulletId++;
    b.x = b.renderedX = renderedX;
    b.y = b.renderedY = renderedY;

    sf::Vector2i mousePosition = sf::Mouse::getPosition(game->GetWindow());
    sf::Vector2f worldPos = game->GetWindow().mapPixelToCoords(mousePosition);
    float dx = worldPos.x - renderedX;
    float dy = worldPos.y - renderedY;
    float magnitude = std::sqrt(dx * dx + dy * dy);
    b.velocityX = (dx / magnitude) * BULLET_SPEED;
    b.velocityY = (dy / magnitude) * BULLET_SPEED;

    game->GetEntityManager()->getBullets()[b.id] = b;
    game->SetShootCooldown() = 0.15f;

    char buffer[128];
    int bytes = snprintf(buffer, sizeof(buffer), "B|%llu|%llu|%.1f|%.1f|%.1f|%.1f",
                         b.id, shooterSteamID.ConvertToUint64(), b.x, b.y, b.velocityX, b.velocityY);
    if (bytes > 0 && static_cast<size_t>(bytes) < sizeof(buffer)) {
        if (game->IsHost()) {
            game->GetNetworkManager()->broadcastMessage(std::string(buffer));
        } else {
            CSteamID hostID(std::stoull(SteamMatchmaking()->GetLobbyData(game->m_currentLobby, "host_steam_id")));
            game->GetNetworkManager()->sendMessage(hostID, std::string(buffer));
        }
    }
}