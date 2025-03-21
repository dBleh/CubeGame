#include "GameObject.h"

// Make sure Windows.h is included before OpenGL headers
#ifdef _WIN32
#include <windows.h>
#endif

#include <GL/glu.h>
#include <algorithm>
#include <iostream>

GameObject::GameObject() 
    : m_name("GameObject"), 
      m_position(0.0f, 0.0f, 0.0f),
      m_rotation(0.0f, 0.0f, 0.0f), 
      m_scale(1.0f, 1.0f, 1.0f),
      m_parent(nullptr),
      m_enabled(true),
      m_color(sf::Color::White)
{
}

GameObject::GameObject(const std::string& name, const sf::Vector3f& position)
    : m_name(name),
      m_position(position),
      m_rotation(0.0f, 0.0f, 0.0f),
      m_scale(1.0f, 1.0f, 1.0f),
      m_parent(nullptr),
      m_enabled(true),
      m_color(sf::Color::White)
{
}

GameObject::~GameObject() 
{
    // Clear children
    clearChildren();
}

void GameObject::initialize() 
{
    // Initialize all children
    for (auto& child : m_children) {
        child->initialize();
    }
}

void GameObject::update(float deltaTime) 
{
    if (!m_enabled) return;
    
    // Update all children
    for (auto& child : m_children) {
        child->update(deltaTime);
    }
}

void GameObject::render() 
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
    
    // Render all children
    for (auto& child : m_children) {
        child->render();
    }
    
    // Restore matrix
    glPopMatrix();
}

void GameObject::addChild(std::shared_ptr<GameObject> child) 
{
    // Check if child is already in the hierarchy
    if (child->m_parent != nullptr) {
        child->m_parent->removeChild(child.get());
    }
    
    // Set parent and add to children
    child->m_parent = this;
    m_children.push_back(child);
}

void GameObject::removeChild(GameObject* child) 
{
    // Find and remove child
    auto it = std::find_if(m_children.begin(), m_children.end(), 
                          [child](const std::shared_ptr<GameObject>& ptr) { 
                              return ptr.get() == child; 
                          });
    
    if (it != m_children.end()) {
        (*it)->m_parent = nullptr;
        m_children.erase(it);
    }
}

void GameObject::clearChildren() 
{
    for (auto& child : m_children) {
        child->m_parent = nullptr;
    }
    
    m_children.clear();
}

bool GameObject::checkCollision(const sf::Vector3f& point, float radius, float height, sf::Vector3f& collisionNormal) const 
{
    // Base GameObject has no collision
    return false;
}