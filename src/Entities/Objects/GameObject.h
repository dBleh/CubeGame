#pragma once

// Windows header must be included before OpenGL headers to avoid conflicts
#ifdef _WIN32
#include <windows.h>
#endif

#include <SFML/System/Vector3.hpp>
#include <SFML/Graphics/Color.hpp>
#include <vector>
#include <memory>
#include <string>

/**
 * @class GameObject
 * @brief Base class for all game objects in the scene
 */
class GameObject {
public:
    GameObject();
    GameObject(const std::string& name, const sf::Vector3f& position = sf::Vector3f(0, 0, 0));
    virtual ~GameObject();

    virtual void initialize();
    virtual void update(float deltaTime);
    virtual void render();
    
    sf::Vector3f getPosition() const { return m_position; }
    sf::Vector3f getRotation() const { return m_rotation; }
    sf::Vector3f getScale() const { return m_scale; }
    
    void setPosition(const sf::Vector3f& position) { m_position = position; }
    void setRotation(const sf::Vector3f& rotation) { m_rotation = rotation; }
    void setScale(const sf::Vector3f& scale) { m_scale = scale; }
    
    void addChild(std::shared_ptr<GameObject> child);
    void removeChild(GameObject* child);
    void clearChildren();
    
    const std::string& getName() const { return m_name; }
    void setName(const std::string& name) { m_name = name; }
    
    virtual bool checkCollision(const sf::Vector3f& point, float radius, float height, sf::Vector3f& collisionNormal) const;
    
    bool isEnabled() const { return m_enabled; }
    void setEnabled(bool enabled) { m_enabled = enabled; }

    const sf::Color& getColor() const { return m_color; }
    void setColor(const sf::Color& color) { m_color = color; }

protected:
    sf::Vector3f m_position;
    sf::Vector3f m_rotation;
    sf::Vector3f m_scale;
    
    GameObject* m_parent;
    std::vector<std::shared_ptr<GameObject>> m_children;
    
    std::string m_name;
    bool m_enabled;
    sf::Color m_color;
};