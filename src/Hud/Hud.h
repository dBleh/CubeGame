#ifndef HUD_H
#define HUD_H

#include <SFML/Graphics.hpp>
#include <string>
#include <unordered_map>
#include "../States/GameState.h"

/**
 * A minimal HUD class:
 * - Draws a plain white background if we're in MainMenu.
 * - Renders black text, turning gray on hover if marked hoverable.
 */
class HUD
{
public:
    // Which space to render: absolute screen coordinates or offset by the view
    enum class RenderMode {
        ScreenSpace,
        ViewSpace
    };

    // Create the HUD with a loaded sf::Font reference
    explicit HUD(sf::Font& font);

    // Add a text element
    void addElement(const std::string& id,
                    const std::string& content,
                    unsigned int size,
                    sf::Vector2f pos,
                    GameState visibleState,
                    RenderMode mode = RenderMode::ScreenSpace,
                    bool hoverable = false);

    // Change the displayed string for an existing text element
    void updateText(const std::string& id, const std::string& content);

    // Set the base (non-hover) color for an element
    void updateBaseColor(const std::string& id, const sf::Color& color);

    // Render the HUD elements that match the current state
    void render(sf::RenderWindow& window, const sf::View& view, GameState currentState);

private:
    // Internal structure for each text element
    struct HUDElement {
        sf::Text text;
        sf::Vector2f pos;
        GameState visibleState;
        RenderMode mode;
        bool hoverable;
        sf::Color baseColor;   // color when not hovered
        sf::Color hoverColor;  // color when hovered
    };

    sf::Font& m_font;
    std::unordered_map<std::string, HUDElement> m_elements;

    // Draws a plain white background covering the entire window
    void drawWhiteBackground(sf::RenderWindow& window);

    // Checks if the mouse is over the text bounding box
    bool isMouseOverText(const sf::RenderWindow& window, const sf::Text& text);
};

#endif // HUD_H
