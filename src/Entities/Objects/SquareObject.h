#pragma once

// Windows header must be included before OpenGL headers to avoid conflicts
#ifdef _WIN32
#include <windows.h>
#endif

#include "GameObject.h"

/**
 * @class SquareObject
 * @brief A square/cube object that the player can collide with
 */
class SquareObject : public GameObject {
public:
    SquareObject();
    SquareObject(const std::string& name, const sf::Vector3f& position = sf::Vector3f(0, 0, 0));
    SquareObject(const std::string& name, const sf::Vector3f& position, const sf::Vector3f& size);
    virtual ~SquareObject();

    virtual void render() override;
    
    virtual bool checkCollision(const sf::Vector3f& point, float radius, float height, sf::Vector3f& collisionNormal) const override;
    
    const sf::Vector3f& getSize() const { return m_size; }
    void setSize(const sf::Vector3f& size) { m_size = size; }
    
private:
    sf::Vector3f m_size;  // Width, height, depth
};