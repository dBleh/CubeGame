#include "HUD.h"
#include <cmath>

HUD::HUD(sf::Font& font)
    : m_font(font)
{
}

void HUD::addElement(const std::string& id,
                     const std::string& content,
                     unsigned int size,
                     sf::Vector2f pos,
                     GameState visibleState,
                     RenderMode mode,
                     bool hoverable)
{
    sf::Text text;
    text.setFont(m_font);
    text.setString(content);
    text.setCharacterSize(size);

    // Minimal style: black text, no outline, normal (not bold)
    text.setFillColor(sf::Color::Black);
    text.setStyle(sf::Text::Regular);

    HUDElement element;
    element.text        = text;
    element.pos         = pos;
    element.visibleState= visibleState;
    element.mode        = mode;
    element.hoverable   = hoverable;
    element.baseColor   = sf::Color::Black;
    // Slightly darker gray on hover
    element.hoverColor  = sf::Color(60, 60, 60);

    m_elements[id] = element;
}

void HUD::updateText(const std::string& id, const std::string& content)
{
    auto it = m_elements.find(id);
    if (it != m_elements.end()) {
        it->second.text.setString(content);
    }
}

void HUD::updateBaseColor(const std::string& id, const sf::Color& color)
{
    auto it = m_elements.find(id);
    if (it != m_elements.end()) {
        it->second.baseColor = color;
        it->second.text.setFillColor(color);
    }
}

void HUD::render(sf::RenderWindow& window,
                 const sf::View& view,
                 GameState currentState)
{
    // If it's the main menu, fill the window with white
    if (currentState == GameState::MainMenu) {
        drawWhiteBackground(window);
    }
    if(currentState == GameState::Lobby) {
        drawWhiteBackground(window);
    }
    if(currentState == GameState::LobbyCreation) {
        drawWhiteBackground(window);
    }
    
    // Compute the top-left corner for "ViewSpace" elements
    sf::Vector2f viewTopLeft = view.getCenter() - (view.getSize() * 0.5f);

    // For each text element that should be visible in the current state
    for (auto& [id, element] : m_elements) {
        if (element.visibleState == currentState) {
            sf::Text& text = element.text;

            // Position text in the correct space
            if (element.mode == RenderMode::ScreenSpace) {
                text.setPosition(element.pos);
            } else {
                text.setPosition(viewTopLeft + element.pos);
            }

            // If hoverable, check if the mouse is over it, change color accordingly
            if (element.hoverable && isMouseOverText(window, text)) {
                text.setFillColor(element.hoverColor);
            } else {
                text.setFillColor(element.baseColor);
            }

            // Draw the text
            window.draw(text);
        }
    }
}

void HUD::drawWhiteBackground(sf::RenderWindow& window)
{
    sf::RectangleShape bg(sf::Vector2f(window.getSize().x, window.getSize().y));
    bg.setFillColor(sf::Color::White);
    bg.setPosition(0.f, 0.f);
    window.draw(bg);
}

bool HUD::isMouseOverText(const sf::RenderWindow& window, const sf::Text& text)
{
    // Convert mouse coords to float
    sf::Vector2i mousePos = sf::Mouse::getPosition(window);
    sf::Vector2f mousePosF(static_cast<float>(mousePos.x),
                           static_cast<float>(mousePos.y));

    // Get bounding box of the text
    sf::FloatRect bounds = text.getGlobalBounds();
    return bounds.contains(mousePosF);
}
