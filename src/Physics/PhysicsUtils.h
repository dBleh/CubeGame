#pragma once

#include <SFML/System/Vector3.hpp>
#include <cmath>

namespace PhysicsUtils {
    /**
     * @brief Physics-related constants
     */
    namespace Constants {
        constexpr float GRAVITY = 9.81f;
        constexpr float AIR_DENSITY = 1.225f; // kg/m^3
        constexpr float EARTH_RADIUS = 6371000.0f; // meters
    }
    
    /**
     * @brief Calculate the distance between two 3D points
     * @param a First point
     * @param b Second point
     * @return Distance between the points
     */
    inline float distance(const sf::Vector3f& a, const sf::Vector3f& b) {
        float dx = b.x - a.x;
        float dy = b.y - a.y;
        float dz = b.z - a.z;
        return std::sqrt(dx * dx + dy * dy + dz * dz);
    }
    
    /**
     * @brief Calculate the length (magnitude) of a 3D vector
     * @param v Vector to calculate length of
     * @return Length of the vector
     */
    inline float length(const sf::Vector3f& v) {
        return std::sqrt(v.x * v.x + v.y * v.y + v.z * v.z);
    }
    
    /**
     * @brief Normalize a 3D vector (make it unit length)
     * @param v Vector to normalize
     * @return Normalized vector
     */
    inline sf::Vector3f normalize(const sf::Vector3f& v) {
        float len = length(v);
        if (len < 0.0001f) {
            return sf::Vector3f(0, 0, 0);
        }
        return sf::Vector3f(v.x / len, v.y / len, v.z / len);
    }
    
    /**
     * @brief Calculate the dot product of two 3D vectors
     * @param a First vector
     * @param b Second vector
     * @return Dot product
     */
    inline float dot(const sf::Vector3f& a, const sf::Vector3f& b) {
        return a.x * b.x + a.y * b.y + a.z * b.z;
    }
    
    /**
     * @brief Calculate the cross product of two 3D vectors
     * @param a First vector
     * @param b Second vector
     * @return Cross product
     */
    inline sf::Vector3f cross(const sf::Vector3f& a, const sf::Vector3f& b) {
        return sf::Vector3f(
            a.y * b.z - a.z * b.y,
            a.z * b.x - a.x * b.z,
            a.x * b.y - a.y * b.x
        );
    }
    
    /**
     * @brief Linear interpolation between two vectors
     * @param a Start vector
     * @param b End vector
     * @param t Interpolation factor (0.0 to 1.0)
     * @return Interpolated vector
     */
    inline sf::Vector3f lerp(const sf::Vector3f& a, const sf::Vector3f& b, float t) {
        return sf::Vector3f(
            a.x + (b.x - a.x) * t,
            a.y + (b.y - a.y) * t,
            a.z + (b.z - a.z) * t
        );
    }
    
    /**
     * @brief Calculate reflection vector
     * @param incident Incident vector
     * @param normal Surface normal
     * @return Reflected vector
     */
    inline sf::Vector3f reflect(const sf::Vector3f& incident, const sf::Vector3f& normal) {
        float d = dot(incident, normal);
        return sf::Vector3f(
            incident.x - 2.0f * d * normal.x,
            incident.y - 2.0f * d * normal.y,
            incident.z - 2.0f * d * normal.z
        );
    }
}