#include "SquareObject.h"

// Make sure Windows.h is included before OpenGL headers
#ifdef _WIN32
#include <windows.h>
#endif

#include <GL/glu.h>
#include <cmath>
#include <algorithm>
#include <iostream>

SquareObject::SquareObject()
    : GameObject("SquareObject"),
      m_size(1.0f, 1.0f, 1.0f)
{
}

SquareObject::SquareObject(const std::string& name, const sf::Vector3f& position)
    : GameObject(name, position),
      m_size(1.0f, 1.0f, 1.0f)
{
}

SquareObject::SquareObject(const std::string& name, const sf::Vector3f& position, const sf::Vector3f& size)
    : GameObject(name, position),
      m_size(size)
{
}

SquareObject::~SquareObject()
{
}

void SquareObject::render()
{
    if (!m_enabled) return;
    
    // Save current matrix
    glPushMatrix();
    
    // Apply local transform
    glTranslatef(m_position.x, m_position.y, m_position.z);
    glRotatef(m_rotation.x, 1.0f, 0.0f, 0.0f);
    glRotatef(m_rotation.y, 0.0f, 1.0f, 0.0f);
    glRotatef(m_rotation.z, 0.0f, 0.0f, 1.0f);
    glScalef(m_scale.x, m_scale.y, m_scale.z);
    
    // Set the color
    glColor4f(m_color.r / 255.0f, m_color.g / 255.0f, m_color.b / 255.0f, m_color.a / 255.0f);
    
    // Draw the cube
    float w = m_size.x / 2.0f;
    float h = m_size.y / 2.0f;
    float d = m_size.z / 2.0f;
    
    glBegin(GL_QUADS);
    
    // Front face
    glVertex3f(-w, -h, d);
    glVertex3f(w, -h, d);
    glVertex3f(w, h, d);
    glVertex3f(-w, h, d);
    
    // Back face
    glVertex3f(-w, -h, -d);
    glVertex3f(-w, h, -d);
    glVertex3f(w, h, -d);
    glVertex3f(w, -h, -d);
    
    // Left face
    glVertex3f(-w, -h, d);
    glVertex3f(-w, h, d);
    glVertex3f(-w, h, -d);
    glVertex3f(-w, -h, -d);
    
    // Right face
    glVertex3f(w, -h, d);
    glVertex3f(w, -h, -d);
    glVertex3f(w, h, -d);
    glVertex3f(w, h, d);
    
    // Top face
    glVertex3f(-w, h, d);
    glVertex3f(w, h, d);
    glVertex3f(w, h, -d);
    glVertex3f(-w, h, -d);
    
    // Bottom face
    glVertex3f(-w, -h, d);
    glVertex3f(-w, -h, -d);
    glVertex3f(w, -h, -d);
    glVertex3f(w, -h, d);
    
    glEnd();
    
    // Render all children
    for (auto& child : m_children) {
        child->render();
    }
    
    // Restore matrix
    glPopMatrix();
}

bool SquareObject::checkCollision(const sf::Vector3f& point, float radius, float playerHeight, sf::Vector3f& collisionNormal) const
{
    // Calculate box boundaries
    float minX = m_position.x - m_size.x / 2.0f;
    float maxX = m_position.x + m_size.x / 2.0f;
    float minY = m_position.y - m_size.y / 2.0f;
    float maxY = m_position.y + m_size.y / 2.0f;
    float minZ = m_position.z - m_size.z / 2.0f;
    float maxZ = m_position.z + m_size.z / 2.0f;
    
    // Calculate player's capsule boundaries (treating it as a vertical capsule)
    float playerMinY = point.y - radius;                // Bottom hemisphere center
    float playerMaxY = point.y + playerHeight + radius; // Top hemisphere center
    
    // Expanded horizontal check for top/bottom landing
    float expandedRadius = radius * 1.1f;  // Slightly expand radius for top/bottom landing
    
    // First, do a quick AABB check to see if there's any possibility of collision
    if (point.x + expandedRadius < minX || point.x - expandedRadius > maxX ||
        playerMaxY < minY || playerMinY > maxY ||
        point.z + expandedRadius < minZ || point.z - expandedRadius > maxZ) {
        return false;
    }
    
    // Special case for top landings - player is falling onto a cube
    // Check if player's bottom is around the top face of the cube
    float fallTolerance = 0.1f;  // Allow some tolerance for catching falls
    
    // If player's bottom is close to the top face and player is within the horizontal bounds
    if (std::abs(playerMinY - maxY) < fallTolerance && 
        point.x + radius > minX && point.x - radius < maxX && 
        point.z + radius > minZ && point.z - radius < maxZ) {
        
        // Verify this is a good top landing candidate - player must be above or very close to the top
        if (playerMinY >= maxY - fallTolerance) {
            collisionNormal = sf::Vector3f(0.0f, 1.0f, 0.0f);
            return true;
        }
    }
    
    // For more accurate collision, we'll use a capsule vs. box test
    // Helper variables
    float capsuleRadius = radius;
    float capsuleHalfHeight = playerHeight / 2.0f;
    sf::Vector3f capsuleCenter(point.x, point.y + capsuleHalfHeight, point.z); // Center of capsule
    
    // Compute the closest point on the box to the capsule center
    sf::Vector3f closestPoint;
    closestPoint.x = (minX > capsuleCenter.x) ? minX : ((maxX < capsuleCenter.x) ? maxX : capsuleCenter.x);
    closestPoint.y = (minY > capsuleCenter.y) ? minY : ((maxY < capsuleCenter.y) ? maxY : capsuleCenter.y);
    closestPoint.z = (minZ > capsuleCenter.z) ? minZ : ((maxZ < capsuleCenter.z) ? maxZ : capsuleCenter.z);
    
    // Calculate the vector from the capsule center to the closest box point
    sf::Vector3f centerToClosest(
        closestPoint.x - capsuleCenter.x,
        closestPoint.y - capsuleCenter.y,
        closestPoint.z - capsuleCenter.z
    );
    
    // Calculate square distance
    float axisDistanceSquared = centerToClosest.x * centerToClosest.x +
                               centerToClosest.y * centerToClosest.y +
                               centerToClosest.z * centerToClosest.z;
    
    // If the distance is less than the radius, we have a collision
    if (axisDistanceSquared < capsuleRadius * capsuleRadius) {
        // Calculate distances to each face to determine best normal
        float distToTop = std::abs(playerMinY - maxY);
        float distToBottom = std::abs(playerMaxY - minY);
        float distToLeft = std::abs(point.x + radius - minX);
        float distToRight = std::abs(point.x - radius - maxX);
        float distToFront = std::abs(point.z + radius - minZ);
        float distToBack = std::abs(point.z - radius - maxZ);
        
        // Find the smallest distance without using std::min directly
        float minDist = distToTop;
        if (distToBottom < minDist) minDist = distToBottom;
        if (distToLeft < minDist) minDist = distToLeft;
        if (distToRight < minDist) minDist = distToRight;
        if (distToFront < minDist) minDist = distToFront;
        if (distToBack < minDist) minDist = distToBack;
        
        // Set normal based on the closest face
        if (minDist == distToTop) {
            collisionNormal = sf::Vector3f(0.0f, 1.0f, 0.0f);
        } else if (minDist == distToBottom) {
            collisionNormal = sf::Vector3f(0.0f, -1.0f, 0.0f);
        } else if (minDist == distToLeft) {
            collisionNormal = sf::Vector3f(-1.0f, 0.0f, 0.0f);
        } else if (minDist == distToRight) {
            collisionNormal = sf::Vector3f(1.0f, 0.0f, 0.0f);
        } else if (minDist == distToFront) {
            collisionNormal = sf::Vector3f(0.0f, 0.0f, -1.0f);
        } else {
            collisionNormal = sf::Vector3f(0.0f, 0.0f, 1.0f);
        }
        
        return true;
    }
    
    return false;
}
