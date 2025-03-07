#ifndef ROUNDEDRECTANGLESHAPE_HPP
#define ROUNDEDRECTANGLESHAPE_HPP

#include <SFML/Graphics.hpp>
#include <cmath>

class RoundedRectangleShape : public sf::Shape {
public:
    // size: overall dimensions, radius: corner rounding, cornerPointCount: smoothness of the arc
    explicit RoundedRectangleShape(const sf::Vector2f& size = sf::Vector2f(0, 0), float radius = 0, std::size_t cornerPointCount = 10)
        : m_size(size), m_radius(radius), m_cornerPointCount(cornerPointCount)
    {
        update();
    }

    void setSize(const sf::Vector2f& size) {
        m_size = size;
        update();
    }

    const sf::Vector2f& getSize() const {
        return m_size;
    }

    void setCornersRadius(float radius) {
        m_radius = radius;
        update();
    }

    float getCornersRadius() const {
        return m_radius;
    }

    void setCornerPointCount(std::size_t count) {
        m_cornerPointCount = count;
        update();
    }

    virtual std::size_t getPointCount() const override {
        return m_cornerPointCount * 4;
    }

    virtual sf::Vector2f getPoint(std::size_t index) const override {
        std::size_t corner = index / m_cornerPointCount;
        float angle = 90.f * (index % m_cornerPointCount) / (m_cornerPointCount - 1);
        float rad = angle * 3.14159265f / 180.f;
        sf::Vector2f offset(std::cos(rad) * m_radius, std::sin(rad) * m_radius);

        // The four corners are processed clockwise starting from top-right
        switch (corner) {
            case 0: return sf::Vector2f(m_size.x - m_radius, m_radius) + sf::Vector2f( offset.x,  offset.y);
            case 1: return sf::Vector2f(m_radius, m_radius) + sf::Vector2f(-offset.x,  offset.y);
            case 2: return sf::Vector2f(m_radius, m_size.y - m_radius) + sf::Vector2f(-offset.x, -offset.y);
            case 3: return sf::Vector2f(m_size.x - m_radius, m_size.y - m_radius) + sf::Vector2f( offset.x, -offset.y);
            default: return sf::Vector2f(0, 0);
        }
    }

private:
    sf::Vector2f m_size;
    float m_radius;
    std::size_t m_cornerPointCount;
};

#endif // ROUNDEDRECTANGLESHAPE_HPP
