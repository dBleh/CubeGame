#include "PhysicsSystem.h"

// Prevent Windows.h from defining min/max macros
#define NOMINMAX

// Windows header must be included before OpenGL headers
#ifdef _WIN32
#include <windows.h>
#endif

#include <GL/glu.h>
#include <algorithm>
#include <iostream>
#include "../Entities/Objects/SquareObject.h"
#include "../Entities/Player/Player.h"
#include "PhysicsUtils.h"

PhysicsSystem::PhysicsSystem()
    : m_player(nullptr), m_debugVisualization(false)
{
}

PhysicsSystem::~PhysicsSystem()
{
    clearColliders();
}

void PhysicsSystem::initialize()
{
    // Initialize the layer collision matrix
    // By default, all layers collide with each other
    for (int i = 0; i < static_cast<int>(CollisionLayer::Count); ++i) {
        for (int j = 0; j < static_cast<int>(CollisionLayer::Count); ++j) {
            m_layerCollisionMatrix[i][j] = true;
        }
    }
    
    // Triggers don't collide with each other
    setLayerCollision(CollisionLayer::Trigger, CollisionLayer::Trigger, false);
    
    // Pickups don't collide with triggers
    setLayerCollision(CollisionLayer::Pickup, CollisionLayer::Trigger, false);
    
    // Clear any existing colliders
    clearColliders();
    
    std::cout << "[INFO] PhysicsSystem initialized" << std::endl;
}

void PhysicsSystem::update(float deltaTime)
{
    if (!m_player) return;
    
    // Store original position before physics updates
    sf::Vector3f originalPos = m_player->getPosition();
    bool wasGrounded = m_player->isGrounded();
    sf::Vector3f originalVel = m_player->getVelocity();
    
    // Apply physics forces
    applyGravity(deltaTime);
    applyFriction(deltaTime);
    
    // Store post-gravity velocity to check if it was changed
    sf::Vector3f postGravityVel = m_player->getVelocity();
    
    // Detect and resolve collisions
    detectCollisions();
    resolveCollisions();
    
    // Update player's grounded state
    updatePlayerGroundedState();
    
    // Edge detection - player was grounded but now isn't
    if (wasGrounded && !m_player->isGrounded()) {
        sf::Vector3f vel = m_player->getVelocity();
        vel.y = -5.0f; // Strong downward velocity when stepping off edge
        m_player->setVelocity(vel);
        
        if (m_player->isDebugMode()) {
            std::cout << "[DEBUG] Edge detected! Applying strong initial fall." << std::endl;
        }
    }
    
    // Clear any temporary data
    m_collisions.clear();
}

void PhysicsSystem::applyGravity(float deltaTime)
{
    if (!m_player) return;
    
    // Always apply gravity regardless of grounded state
    sf::Vector3f velocity = m_player->getVelocity();
    float gravity = m_player->getConfig().getGravity();
    
    // Ensure reasonable gravity (in case config is wrong)
    if (gravity < 0.1f) gravity = 9.8f;
    
    velocity.y -= gravity * deltaTime;
    
    // Terminal velocity
    if (velocity.y < -20.0f) {
        velocity.y = -20.0f;
    }
    
    m_player->setVelocity(velocity);
    
    // If we're falling, update player state accordingly
    if (velocity.y < 0 && !m_player->isGrounded()) {
        // Set state to falling if we're moving down
        if (m_player->getState() != Player::State::Falling) {
            if (m_player->isDebugMode()) {
                std::cout << "[DEBUG] Player state changed to falling" << std::endl;
            }
        }
    }
}

void PhysicsSystem::applyFriction(float deltaTime)
{
    if (!m_player) return;
    
    sf::Vector3f velocity = m_player->getVelocity();
    
    // Apply ground friction or air resistance based on state
    if (m_player->isGrounded()) {
        // Ground friction
        float friction = m_player->getConfig().getGroundFriction() * deltaTime;
        if (friction > 1.0f) friction = 1.0f; // Clamp to prevent overcorrection
        
        velocity.x *= (1.0f - friction);
        velocity.z *= (1.0f - friction);
    } else {
        // Air resistance
        float resistance = m_player->getConfig().getAirResistance() * deltaTime;
        
        velocity.x *= (1.0f - resistance);
        velocity.z *= (1.0f - resistance);
    }
    
    m_player->setVelocity(velocity);
}

void PhysicsSystem::addCollider(std::shared_ptr<GameObject> object, CollisionLayer layer)
{
    if (!object) {
        std::cerr << "[ERROR] PhysicsSystem::addCollider - Attempting to add null object" << std::endl;
        return;
    }
    
    // Add the object to the appropriate layer
    m_collisionLayers[layer].push_back(object);
}

void PhysicsSystem::removeCollider(GameObject* object)
{
    if (!object) return;
    
    // Remove from all layers
    for (auto& layerPair : m_collisionLayers) {
        auto& objects = layerPair.second;
        
        objects.erase(
            std::remove_if(objects.begin(), objects.end(),
                [object](const std::shared_ptr<GameObject>& obj) {
                    return obj.get() == object;
                }),
            objects.end()
        );
    }
}

void PhysicsSystem::clearColliders()
{
    m_collisionLayers.clear();
}

void PhysicsSystem::setLayerCollision(CollisionLayer layerA, CollisionLayer layerB, bool canCollide)
{
    int a = static_cast<int>(layerA);
    int b = static_cast<int>(layerB);
    
    if (a >= 0 && a < static_cast<int>(CollisionLayer::Count) &&
        b >= 0 && b < static_cast<int>(CollisionLayer::Count)) {
        // Set both directions since collision matrix is symmetric
        m_layerCollisionMatrix[a][b] = canCollide;
        m_layerCollisionMatrix[b][a] = canCollide;
    }
}

bool PhysicsSystem::canLayersCollide(CollisionLayer layerA, CollisionLayer layerB) const
{
    int a = static_cast<int>(layerA);
    int b = static_cast<int>(layerB);
    
    if (a >= 0 && a < static_cast<int>(CollisionLayer::Count) &&
        b >= 0 && b < static_cast<int>(CollisionLayer::Count)) {
        return m_layerCollisionMatrix[a][b];
    }
    
    // Default to no collision if invalid layers
    return false;
}

void PhysicsSystem::detectCollisions()
{
    // Clear previous collisions
    m_collisions.clear();
    
    if (!m_player) return;
    
    // Get player properties
    sf::Vector3f playerPos = m_player->getPosition();
    float playerRadius = m_player->getConfig().getPlayerRadius();
    float playerHeight = m_player->getConfig().getPlayerHeight();
    
    // Check each layer for collisions with player
    for (const auto& layerPair : m_collisionLayers) {
        CollisionLayer layer = layerPair.first;
        
        // Skip layers that don't collide with player
        if (!canLayersCollide(CollisionLayer::Player, layer)) {
            continue;
        }
        
        const auto& objects = layerPair.second;
        
        for (const auto& object : objects) {
            if (!object->isEnabled()) continue;
            
            // Check for collision
            CollisionInfo collisionInfo;
            
            if (checkPlayerObjectCollision(playerPos, playerRadius, playerHeight, object, collisionInfo)) {
                // Store the collision for resolution
                CollisionData collision;
                collision.objectA = nullptr; // Null means player
                collision.objectB = object;
                collision.info = collisionInfo;
                m_collisions.push_back(collision);
            }
        }
    }
    
    // Todo: Add object-to-object collision detection if needed
}

bool PhysicsSystem::checkPlayerObjectCollision(const sf::Vector3f& playerPos,
                                             float playerRadius,
                                             float playerHeight,
                                             std::shared_ptr<GameObject> object,
                                             CollisionInfo& collisionInfo)
{
    // Try to cast to SquareObject for more specific collision
    SquareObject* squareObj = dynamic_cast<SquareObject*>(object.get());
    if (squareObj) {
        // Get object properties
        sf::Vector3f objPos = object->getPosition();
        sf::Vector3f objSize = squareObj->getSize();
        
        // Calculate capsule vs. AABB collision
        
        // Calculate player's capsule boundaries
        float playerMinY = playerPos.y - playerRadius;                // Bottom hemisphere center
        float playerMaxY = playerPos.y + playerHeight + playerRadius; // Top hemisphere center
        
        // Calculate box boundaries
        float minX = objPos.x - objSize.x / 2.0f;
        float maxX = objPos.x + objSize.x / 2.0f;
        float minY = objPos.y - objSize.y / 2.0f;
        float maxY = objPos.y + objSize.y / 2.0f;
        float minZ = objPos.z - objSize.z / 2.0f;
        float maxZ = objPos.z + objSize.z / 2.0f;
        
        // Quick AABB check first
        if (playerPos.x + playerRadius < minX || playerPos.x - playerRadius > maxX ||
            playerMaxY < minY || playerMinY > maxY ||
            playerPos.z + playerRadius < minZ || playerPos.z - playerRadius > maxZ) {
            return false;
        }
        
        // Special case for top landings - player is falling onto a cube
        float fallTolerance = 0.1f;
        
        // If player's bottom is close to the top face and player is within the horizontal bounds
        if (std::abs(playerMinY - maxY) < fallTolerance && 
            playerPos.x + playerRadius > minX && playerPos.x - playerRadius < maxX && 
            playerPos.z + playerRadius > minZ && playerPos.z - playerRadius < maxZ) {
            
            // Verify this is a good top landing candidate - player must be above or very close to the top
            if (playerMinY >= maxY - fallTolerance) {
                collisionInfo.collisionNormal = sf::Vector3f(0.0f, 1.0f, 0.0f);
                collisionInfo.penetrationDepth = fallTolerance - std::abs(playerMinY - maxY);
                collisionInfo.isTop = true;
                return true;
            }
        }
        
        // More detailed collision check
        // Find closest point on box to player center
        sf::Vector3f closestPoint;
        closestPoint.x = (std::max)(minX, (std::min)(playerPos.x, maxX));
        closestPoint.y = (std::max)(minY, (std::min)(playerPos.y, maxY));
        closestPoint.z = (std::max)(minZ, (std::min)(playerPos.z, maxZ));
        
        // Calculate distance squared
        float distSquared = 
            (closestPoint.x - playerPos.x) * (closestPoint.x - playerPos.x) +
            (closestPoint.y - playerPos.y) * (closestPoint.y - playerPos.y) +
            (closestPoint.z - playerPos.z) * (closestPoint.z - playerPos.z);
        
        // If distance is less than radius, we have a collision
        if (distSquared < playerRadius * playerRadius) {
            // Calculate normal and penetration
            sf::Vector3f normal(playerPos.x - closestPoint.x, 
                              playerPos.y - closestPoint.y, 
                              playerPos.z - closestPoint.z);
                              
            float distance = std::sqrt(distSquared);
            
            // Normalize the normal
            if (distance > 0.0001f) {
                normal.x /= distance;
                normal.y /= distance;
                normal.z /= distance;
            } else {
                // If player is exactly at closest point, use a default normal
                normal = sf::Vector3f(0.0f, 1.0f, 0.0f);
            }
            
            collisionInfo.collisionNormal = normal;
            collisionInfo.penetrationDepth = playerRadius - distance;
            collisionInfo.isTop = (normal.y > 0.7f);
            
            return true;
        }
        
        // No collision
        return false;
    }
    
    // For non-cube objects, delegate to their own collision method
    sf::Vector3f normal;
    bool collision = object->checkCollision(playerPos, playerRadius, playerHeight, normal);
    
    if (collision) {
        collisionInfo.collisionNormal = normal;
        collisionInfo.penetrationDepth = playerRadius * 0.2f; // Default penetration depth
        collisionInfo.isTop = (normal.y > 0.7f);
        return true;
    }
    
    return false;
}

void PhysicsSystem::resolveCollisions()
{
    if (!m_player) return;
    
    // Sort collisions by penetration depth (highest first)
    std::sort(m_collisions.begin(), m_collisions.end(), 
        [](const CollisionData& a, const CollisionData& b) {
            return a.info.penetrationDepth > b.info.penetrationDepth;
        }
    );
    
    // Get current player state
    sf::Vector3f playerPos = m_player->getPosition();
    sf::Vector3f playerVel = m_player->getVelocity();
    bool wasGrounded = m_player->isGrounded();
    bool isGrounded = false;
    
    // Process each collision
    for (const auto& collision : m_collisions) {
        const CollisionInfo& info = collision.info;
        
        // For top collisions (landing on an object)
        if (info.isTop) {
            // Only adjust position and velocity if the player isn't actively jumping
            if (playerVel.y <= 0) {
                SquareObject* squareObj = dynamic_cast<SquareObject*>(collision.objectB.get());
                if (squareObj) {
                    float surfaceY = collision.objectB->getPosition().y + (squareObj->getSize().y / 2.0f);
                    playerPos.y = surfaceY + m_player->getConfig().getPlayerRadius() + 0.01f;
                }
                
                // Zero out vertical velocity if falling
                if (playerVel.y < 0) {
                    playerVel.y = 0.0f;
                }
                
                // Mark as grounded
                isGrounded = true;
            }
        }
        // For bottom collisions (hitting ceiling)
        else if (info.collisionNormal.y < -0.7f) {
            // Cancel upward velocity
            if (playerVel.y > 0) {
                playerVel.y = 0.0f;
            }
            
            // Push player down
            playerPos.y = playerPos.y - info.penetrationDepth * 1.1f;
        }
        // Side collisions
        else {
            // Calculate the reflection of velocity vector
            float velocityDotNormal = PhysicsUtils::dot(playerVel, info.collisionNormal);
            
            // Only reflect if moving into the surface
            if (velocityDotNormal < 0) {
                // Calculate reflection and apply friction
                float frictionFactor = m_player->isGrounded() ? 0.2f : 0.8f;
                
                playerVel.x -= info.collisionNormal.x * velocityDotNormal * (1.0f + frictionFactor);
                playerVel.y -= info.collisionNormal.y * velocityDotNormal * (1.0f + frictionFactor);
                playerVel.z -= info.collisionNormal.z * velocityDotNormal * (1.0f + frictionFactor);
                
                // Preserve the y component if it's a side collision
                if (std::abs(info.collisionNormal.y) < 0.7f) {
                    playerVel.y = m_player->getVelocity().y;
                }
            }
            
            // Apply position correction with a small buffer
            float correctionFactor = info.penetrationDepth + 0.01f;
            
            playerPos.x += info.collisionNormal.x * correctionFactor;
            // For side collisions, don't modify Y position if player is grounded
            if (!(std::abs(info.collisionNormal.y) < 0.7f && m_player->isGrounded())) {
                playerPos.y += info.collisionNormal.y * correctionFactor;
            }
            playerPos.z += info.collisionNormal.z * correctionFactor;
        }
    }
    
    // Update player state
    m_player->setPosition(playerPos);
    m_player->setVelocity(playerVel);
    m_player->setGrounded(isGrounded);
}

void PhysicsSystem::updatePlayerGroundedState()
{
    if (!m_player) return;
    
    // Don't overwrite grounded state during jump
    if (m_player->getVelocity().y > 0) {
        m_player->setGrounded(false);
        return;
    }
    
    // If player was already set as grounded during collision resolution, maintain that state
    // with a short timer to prevent flicker
    if (m_player->isGrounded()) {
        // If we're on a cube (determined during collision resolution), give the player
        // a short "grounded memory" so the state doesn't flicker between frames
        if (!m_collisions.empty()) {
            // Set a short grounded timer to maintain state between frames
            m_player->setGroundedTimer(0.1f);  // 100ms of "grounded memory"
            return;
        }
    }
    
    // Get player position
    sf::Vector3f playerPos = m_player->getPosition();
    float playerRadius = m_player->getConfig().getPlayerRadius();
    
    // If on ground level
    if (playerPos.y <= playerRadius + 0.01f) {
        // Set to exactly on ground
        m_player->setPosition(sf::Vector3f(playerPos.x, playerRadius, playerPos.z));
        m_player->setGrounded(true);
        
        // Cancel any downward velocity
        sf::Vector3f vel = m_player->getVelocity();
        if (vel.y < 0) {
            vel.y = 0;
            m_player->setVelocity(vel);
        }
        
        return; // Stay grounded
    }
    
    // Get current grounded state for change detection
    bool wasGrounded = m_player->isGrounded();
    
    // Cast multiple rays for better edge detection
    // Sample 5 points: center and four corners
    bool foundGround = false;
    float checkRadius = playerRadius * 0.7f; // Slightly smaller than player radius
    
    // Points to check (center + corners)
    std::vector<sf::Vector3f> checkPoints = {
        sf::Vector3f(0, 0, 0),               // Center
        sf::Vector3f(checkRadius, 0, 0),     // Right
        sf::Vector3f(-checkRadius, 0, 0),    // Left
        sf::Vector3f(0, 0, checkRadius),     // Front
        sf::Vector3f(0, 0, -checkRadius)     // Back
    };
    
    // Modified: Use a much shorter raycast length to ensure the player is very close to the ground
    // Original: float rayLength = playerRadius + .0001f;
    float rayLength = 0.05f; // Only detect ground when very close
    
    // Track the actual distance to ground
    float minGroundDistance = rayLength;
    
    // Check all points
    for (const auto& offset : checkPoints) {
        sf::Vector3f rayOrigin = playerPos + offset;
        sf::Vector3f rayDirection(0.0f, -1.0f, 0.0f);
        
        std::shared_ptr<GameObject> objectBelow = raycast(rayOrigin, rayDirection, rayLength, CollisionLayer::Environment);
        
        if (objectBelow) {
            foundGround = true;
            break;
        }
    }
    
    // NEW: Only set grounded state if the player is ACTUALLY touching the ground
    // or very close to it (within a small threshold)
    if (foundGround && minGroundDistance < 0.02f) {
        m_player->setGrounded(true);
        
        // If we were falling and just became grounded, cancel vertical velocity
        if (!wasGrounded) {
            sf::Vector3f vel = m_player->getVelocity();
            if (vel.y < 0) {
                vel.y = 0.0f;
                m_player->setVelocity(vel);
            }
        }
    } else {
        m_player->setGrounded(false);
    }
    
    // If player just became ungrounded (stepped off an edge), apply a strong initial downward velocity
    if (wasGrounded && !m_player->isGrounded()) {
        sf::Vector3f vel = m_player->getVelocity();
        vel.y = -5.0f; // Strong initial downward velocity
        m_player->setVelocity(vel);
        
        if (m_player->isDebugMode()) {
            std::cout << "[DEBUG] Player stepped off edge and is now falling" << std::endl;
        }
    }
}

std::shared_ptr<GameObject> PhysicsSystem::raycast(const sf::Vector3f& origin, 
                                                const sf::Vector3f& direction, 
                                                float maxDistance,
                                                CollisionLayer layer)
{
    // Normalize direction
    sf::Vector3f normalizedDir = PhysicsUtils::normalize(direction);
    if (normalizedDir.x == 0 && normalizedDir.y == 0 && normalizedDir.z == 0) {
        return nullptr; // Invalid direction
    }
    
    // Determine which layers to check
    std::vector<std::pair<CollisionLayer, std::vector<std::shared_ptr<GameObject>>*>> layersToCheck;
    
    if (layer == CollisionLayer::Default) {
        // Check all layers
        for (auto& layerPair : m_collisionLayers) {
            layersToCheck.push_back(std::make_pair(layerPair.first, &layerPair.second));
        }
    } else {
        // Check only the specified layer
        auto it = m_collisionLayers.find(layer);
        if (it != m_collisionLayers.end()) {
            layersToCheck.push_back(std::make_pair(it->first, &it->second));
        }
    }
    
    // Check for ray intersection
    std::shared_ptr<GameObject> closestObject = nullptr;
    float closestDistance = maxDistance;
    
    for (const auto& layerPair : layersToCheck) {
        const std::vector<std::shared_ptr<GameObject>>& objects = *layerPair.second;
        
        for (const auto& object : objects) {
            if (!object->isEnabled()) continue;
            
            // For now, just handle SquareObject
            SquareObject* squareObj = dynamic_cast<SquareObject*>(object.get());
            if (!squareObj) continue;
            
            // Get object properties
            sf::Vector3f objPos = object->getPosition();
            sf::Vector3f objSize = squareObj->getSize();
            
            // Calculate AABB bounds
            float minX = objPos.x - objSize.x / 2.0f;
            float maxX = objPos.x + objSize.x / 2.0f;
            float minY = objPos.y - objSize.y / 2.0f;
            float maxY = objPos.y + objSize.y / 2.0f;
            float minZ = objPos.z - objSize.z / 2.0f;
            float maxZ = objPos.z + objSize.z / 2.0f;
            
            // Calculate ray-AABB intersection
            float tmin = -INFINITY;
            float tmax = INFINITY;
            
            // X slabs
            if (std::abs(normalizedDir.x) > 0.0001f) {
                float t1 = (minX - origin.x) / normalizedDir.x;
                float t2 = (maxX - origin.x) / normalizedDir.x;
                tmin = (t1 < t2) ? ((tmin > t1) ? tmin : t1) : ((tmin > t2) ? tmin : t2);
                tmax = (t1 > t2) ? ((tmax < t1) ? tmax : t1) : ((tmax < t2) ? tmax : t2);
            }
            
            // Y slabs
            if (std::abs(normalizedDir.y) > 0.0001f) {
                float t1 = (minY - origin.y) / normalizedDir.y;
                float t2 = (maxY - origin.y) / normalizedDir.y;
                tmin = (t1 < t2) ? ((tmin > t1) ? tmin : t1) : ((tmin > t2) ? tmin : t2);
                tmax = (t1 > t2) ? ((tmax < t1) ? tmax : t1) : ((tmax < t2) ? tmax : t2);
            }
            
            // Z slabs
            if (std::abs(normalizedDir.z) > 0.0001f) {
                float t1 = (minZ - origin.z) / normalizedDir.z;
                float t2 = (maxZ - origin.z) / normalizedDir.z;
                tmin = (t1 < t2) ? ((tmin > t1) ? tmin : t1) : ((tmin > t2) ? tmin : t2);
                tmax = (t1 > t2) ? ((tmax < t1) ? tmax : t1) : ((tmax < t2) ? tmax : t2);
            }
            
            // If tmax < 0, ray is intersecting AABB, but entire AABB is behind ray
            // If tmin > tmax, ray doesn't intersect AABB
            if (tmax < 0 || tmin > tmax) {
                continue;
            }
            
            // Get the actual intersection point (use tmin or 0 if we're inside the AABB)
            float t = (tmin < 0) ? 0 : tmin;
            
            // If t is within our max distance and closer than any previous hit
            if (t < closestDistance) {
                closestDistance = t;
                closestObject = object;
            }
        }
    }
    
    return closestObject;
}

void PhysicsSystem::debugDraw()
{
    if (!m_debugVisualization) return;
    
    // Draw colliders
    for (const auto& layerPair : m_collisionLayers) {
        CollisionLayer layer = layerPair.first;
        const auto& objects = layerPair.second;
        
        // Use different colors for different layers
        sf::Color layerColor;
        switch (layer) {
            case CollisionLayer::Player:
                layerColor = sf::Color(0, 255, 0); // Green
                break;
            case CollisionLayer::Environment:
                layerColor = sf::Color(128, 128, 128); // Gray
                break;
            case CollisionLayer::Trigger:
                layerColor = sf::Color(255, 255, 0); // Yellow
                break;
            case CollisionLayer::Pickup:
                layerColor = sf::Color(0, 255, 255); // Cyan
                break;
            default:
                layerColor = sf::Color(255, 255, 255); // White
                break;
        }
        
        float r = layerColor.r / 255.0f;
        float g = layerColor.g / 255.0f;
        float b = layerColor.b / 255.0f;
        
        for (const auto& object : objects) {
            if (!object->isEnabled()) continue;
            
            // Draw wire-frame box around object
            SquareObject* squareObj = dynamic_cast<SquareObject*>(object.get());
            if (squareObj) {
                sf::Vector3f position = object->getPosition();
                sf::Vector3f size = squareObj->getSize();
                
                glPushMatrix();
                glTranslatef(position.x, position.y, position.z);
                
                // Apply object rotation
                sf::Vector3f rotation = object->getRotation();
                glRotatef(rotation.x, 1.0f, 0.0f, 0.0f);
                glRotatef(rotation.y, 0.0f, 1.0f, 0.0f);
                glRotatef(rotation.z, 0.0f, 0.0f, 1.0f);
                
                // Draw wireframe with layer color
                glColor3f(r, g, b);
                
                float halfWidth = size.x / 2.0f;
                float halfHeight = size.y / 2.0f;
                float halfDepth = size.z / 2.0f;
                
                glBegin(GL_LINE_LOOP);
                glVertex3f(-halfWidth, -halfHeight, -halfDepth);
                glVertex3f(halfWidth, -halfHeight, -halfDepth);
                glVertex3f(halfWidth, halfHeight, -halfDepth);
                glVertex3f(-halfWidth, halfHeight, -halfDepth);
                glEnd();
                
                glBegin(GL_LINE_LOOP);
                glVertex3f(-halfWidth, -halfHeight, halfDepth);
                glVertex3f(halfWidth, -halfHeight, halfDepth);
                glVertex3f(halfWidth, halfHeight, halfDepth);
                glVertex3f(-halfWidth, halfHeight, halfDepth);
                glEnd();
                
                glBegin(GL_LINES);
                glVertex3f(-halfWidth, -halfHeight, -halfDepth);
                glVertex3f(-halfWidth, -halfHeight, halfDepth);
                
                glVertex3f(halfWidth, -halfHeight, -halfDepth);
                glVertex3f(halfWidth, -halfHeight, halfDepth);
                
                glVertex3f(halfWidth, halfHeight, -halfDepth);
                glVertex3f(halfWidth, halfHeight, halfDepth);
                
                glVertex3f(-halfWidth, halfHeight, -halfDepth);
                glVertex3f(-halfWidth, halfHeight, halfDepth);
                glEnd();
                
                glPopMatrix();
            }
        }
    }
    
    // Draw player collision shape
    if (m_player) {
        sf::Vector3f position = m_player->getPosition();
        float radius = m_player->getConfig().getPlayerRadius();
        float height = m_player->getConfig().getPlayerHeight();
        
        // Player collider in blue
        glColor3f(0.0f, 0.0f, 1.0f);
        
        // Draw cylinder
        const int segments = 16;
        
        // Draw bottom circle
        glBegin(GL_LINE_LOOP);
        for (int i = 0; i < segments; i++) {
            float angle = ((float)i / segments) * 2.0f * 3.14159f;
            float x = position.x + radius * sin(angle);
            float z = position.z + radius * cos(angle);
            glVertex3f(x, position.y, z);
        }
        glEnd();
        
        // Draw top circle
        glBegin(GL_LINE_LOOP);
        for (int i = 0; i < segments; i++) {
            float angle = ((float)i / segments) * 2.0f * 3.14159f;
            float x = position.x + radius * sin(angle);
            float z = position.z + radius * cos(angle);
            glVertex3f(x, position.y + height, z);
        }
        glEnd();
        
        // Draw connecting lines
        glBegin(GL_LINES);
        for (int i = 0; i < segments; i++) {
            float angle = ((float)i / segments) * 2.0f * 3.14159f;
            float x = position.x + radius * sin(angle);
            float z = position.z + radius * cos(angle);
            glVertex3f(x, position.y, z);
            glVertex3f(x, position.y + height, z);
        }
        glEnd();
        
        // Draw grounded status indicator
        if (m_player->isGrounded()) {
            glColor3f(0.0f, 1.0f, 0.0f); // Green when grounded
        } else {
            glColor3f(1.0f, 0.0f, 0.0f); // Red when in air
        }
        
        glPointSize(5.0f);
        glBegin(GL_POINTS);
        glVertex3f(position.x, position.y - radius * 0.5f, position.z);
        glEnd();
        glPointSize(1.0f);
    }
    
    // Draw collision info
    for (const auto& collision : m_collisions) {
        if (!collision.objectA) { // Player collision
            sf::Vector3f objPos = collision.objectB->getPosition();
            sf::Vector3f playerPos = m_player->getPosition();
            float playerHeight = m_player->getConfig().getPlayerHeight();
            
            // Draw collision normal
            glColor3f(1.0f, 0.0f, 0.0f); // Red for collision
            
            // Calculate collision point (approximate)
            sf::Vector3f collisionPoint = playerPos;
            collisionPoint.x -= collision.info.collisionNormal.x * m_player->getConfig().getPlayerRadius();
            collisionPoint.y -= collision.info.collisionNormal.y * m_player->getConfig().getPlayerRadius();
            collisionPoint.z -= collision.info.collisionNormal.z * m_player->getConfig().getPlayerRadius();
            
            // If normal is pointing up or down, adjust the y-coordinate
            if (collision.info.isTop) {
                // Bottom collision - use bottom of player
                collisionPoint.y = playerPos.y;
            } else if (collision.info.collisionNormal.y < -0.7f) {
                // Top collision - use top of player
                collisionPoint.y = playerPos.y + playerHeight;
            }
            
            // Draw point
            glPointSize(5.0f);
            glBegin(GL_POINTS);
            glVertex3f(collisionPoint.x, collisionPoint.y, collisionPoint.z);
            glEnd();
            glPointSize(1.0f);
            
            // Draw normal
            glBegin(GL_LINES);
            glVertex3f(collisionPoint.x, collisionPoint.y, collisionPoint.z);
            glVertex3f(collisionPoint.x + collision.info.collisionNormal.x * 0.5f,
                      collisionPoint.y + collision.info.collisionNormal.y * 0.5f,
                      collisionPoint.z + collision.info.collisionNormal.z * 0.5f);
            glEnd();
        }
    }
}