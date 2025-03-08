#ifndef ROUNDEDRECTANGLESHAPE_HPP
#define ROUNDEDRECTANGLESHAPE_HPP

#include <SFML/Graphics.hpp>
#include <cmath>

/**
 * @brief A custom SFML Shape that represents a rounded rectangle.
 *
 * This class extends sf::Shape to create a rectangle with rounded corners.
 */
class RoundedRectangleShape : public sf::Shape {
public:
    /**
     * @brief Constructor.
     * @param size Overall dimensions of the rectangle.
     * @param radius Radius of the rounded corners.
     * @param cornerPointCount Number of points used to approximate each corner arc.
     */
    explicit RoundedRectangleShape(const sf::Vector2f& size = sf::Vector2f(0, 0), float radius = 0, std::size_t cornerPointCount = 10)
        : m_size(size), m_radius(radius), m_cornerPointCount(cornerPointCount)
    {
        update();
    }

    /// Set the overall size of the rectangle.
    void setSize(const sf::Vector2f& size) {
        m_size = size;
        update();
    }

    /// Get the current size of the rectangle.
    const sf::Vector2f& getSize() const {
        return m_size;
    }

    /// Set the radius for the rounded corners.
    void setCornersRadius(float radius) {
        m_radius = radius;
        update();
    }

    /// Get the current corner radius.
    float getCornersRadius() const {
        return m_radius;
    }

    /// Set the number of points to approximate each corner.
    void setCornerPointCount(std::size_t count) {
        m_cornerPointCount = count;
        update();
    }

    /// Returns the total number of points defining the shape.
    virtual std::size_t getPointCount() const override {
        return m_cornerPointCount * 4;
    }

    /// Returns the position of a given point in the shape.
    virtual sf::Vector2f getPoint(std::size_t index) const override {
        std::size_t corner = index / m_cornerPointCount;
        float angle = 90.f * (index % m_cornerPointCount) / (m_cornerPointCount - 1);
        float rad = angle * 3.14159265f / 180.f;
        sf::Vector2f offset(std::cos(rad) * m_radius, std::sin(rad) * m_radius);

        // Process corners clockwise starting from the top-right corner.
        switch (corner) {
            case 0: return sf::Vector2f(m_size.x - m_radius, m_radius) + sf::Vector2f( offset.x,  offset.y);
            case 1: return sf::Vector2f(m_radius, m_radius) + sf::Vector2f(-offset.x,  offset.y);
            case 2: return sf::Vector2f(m_radius, m_size.y - m_radius) + sf::Vector2f(-offset.x, -offset.y);
            case 3: return sf::Vector2f(m_size.x - m_radius, m_size.y - m_radius) + sf::Vector2f( offset.x, -offset.y);
            default: return sf::Vector2f(0, 0);
        }
    }

private:
    sf::Vector2f m_size;          ///< Overall dimensions of the rectangle.
    float m_radius;               ///< Radius for the rounded corners.
    std::size_t m_cornerPointCount; ///< Number of points per corner.
};

#endif // ROUNDEDRECTANGLESHAPE_HPP
