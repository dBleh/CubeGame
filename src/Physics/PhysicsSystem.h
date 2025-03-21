#pragma once

#include <vector>
#include <memory>
#include <unordered_map>
#include <SFML/System/Vector3.hpp>

// Forward declarations
class GameObject;
class Player;
class SquareObject;

/**
 * @enum CollisionLayer
 * @brief Defines different collision layers for filtering
 */
enum class CollisionLayer {
    Default = 0,
    Player = 1,
    Environment = 2,
    Trigger = 3,
    Pickup = 4,
    Count
};

/**
 * @struct CollisionInfo
 * @brief Contains information about a collision between two objects
 */
struct CollisionInfo {
    sf::Vector3f collisionNormal;
    float penetrationDepth;
    bool isTop;  // Is this a top collision (standing on something)
    
    CollisionInfo() : penetrationDepth(0.0f), isTop(false) {}
    
    CollisionInfo(const sf::Vector3f& normal, float depth) 
        : collisionNormal(normal), penetrationDepth(depth) {
        // Determine if this is a top collision (normal pointing mostly up)
        isTop = (normal.y > 0.7f);
    }
};

/**
 * @class PhysicsSystem
 * @brief Centralized system for handling all physics and collisions
 */
class PhysicsSystem {
public:
    PhysicsSystem();
    ~PhysicsSystem();
    
    // Initialization and update
    void initialize();
    void update(float deltaTime);
    
    // Object management
    void addCollider(std::shared_ptr<GameObject> object, CollisionLayer layer = CollisionLayer::Default);
    void removeCollider(GameObject* object);
    void clearColliders();
    
    // Player reference for physics calculations
    void setPlayer(Player* player) { m_player = player; }
    
    // Layer collision settings
    void setLayerCollision(CollisionLayer layerA, CollisionLayer layerB, bool canCollide);
    bool canLayersCollide(CollisionLayer layerA, CollisionLayer layerB) const;
    
    // Raycast for environment queries
    std::shared_ptr<GameObject> raycast(const sf::Vector3f& origin, 
                                       const sf::Vector3f& direction, 
                                       float maxDistance,
                                       CollisionLayer layer = CollisionLayer::Default);
    
    // Debug drawing
    void setDebugVisualization(bool enabled) { m_debugVisualization = enabled; }
    void debugDraw();
    
    // Physics constants
    static constexpr float GRAVITY = 9.81f;
    
private:
    // Core physics calculations
    void applyGravity(float deltaTime);
    void applyFriction(float deltaTime);
    
    // Main collision processing methods
    void detectCollisions();
    void resolveCollisions();
    
    // Player-specific collision resolution (kept separate for optimization)
    bool checkPlayerCollisions(sf::Vector3f& resolvedPosition, sf::Vector3f& resolvedVelocity);
    
    // Collision detection helpers
    bool checkCollision(std::shared_ptr<GameObject> objectA, 
                       std::shared_ptr<GameObject> objectB,
                       CollisionInfo& collisionInfo);
                       
    bool checkPlayerObjectCollision(const sf::Vector3f& playerPos,
                                  float playerRadius,
                                  float playerHeight,
                                  std::shared_ptr<GameObject> object,
                                  CollisionInfo& collisionInfo);
    
    // Ground detection for player
    void updatePlayerGroundedState();
    
    // Data members
    Player* m_player;
    bool m_debugVisualization;
    
    // Collision layer management
    std::unordered_map<CollisionLayer, std::vector<std::shared_ptr<GameObject>>> m_collisionLayers;
    bool m_layerCollisionMatrix[static_cast<int>(CollisionLayer::Count)][static_cast<int>(CollisionLayer::Count)];
    
    // Current frame collision data (for resolving and debug)
    struct CollisionData {
        std::shared_ptr<GameObject> objectA;
        std::shared_ptr<GameObject> objectB;
        CollisionInfo info;
    };
    std::vector<CollisionData> m_collisions;
};