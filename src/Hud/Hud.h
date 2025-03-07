#ifndef HUD_H
#define HUD_H

#include <SFML/Graphics.hpp>
#include <string>
#include <unordered_map>
#include "../States/GameState.h"
#include "../Utils/Config.h"
#include "../Entities/Player.h"
#include <unordered_map>
#include "../Utils/SteamHelpers.h"
class HUD
{
public:
    enum class RenderMode {
        ScreenSpace,
        ViewSpace
    };

    struct HUDElement {
        sf::Text text;
        sf::Vector2f pos;
        GameState visibleState;
        RenderMode mode;
        bool hoverable;
        sf::Color baseColor;
        sf::Color hoverColor;
    };

    explicit HUD(sf::Font& font);
    bool isFullyLoaded() const;

void updateScoreboard(const std::unordered_map<CSteamID, Player, CSteamIDHash>& players);

    void addElement(const std::string& id,
                    const std::string& content,
                    unsigned int size,
                    sf::Vector2f pos,
                    GameState visibleState,
                    RenderMode mode = RenderMode::ScreenSpace,
                    bool hoverable = false);

    void refreshGameInfo(const sf::Vector2u& winSize,
                     int currentLevel,
                     size_t enemyCount,
                     const Player& localPlayer,
                     float nextLevelTimer,
                     const std::unordered_map<CSteamID, Player, CSteamIDHash>& players);

    void updateText(const std::string& id, const std::string& content);
    void updateBaseColor(const std::string& id, const sf::Color& color);
    void updateElementPosition(const std::string& id, const sf::Vector2f& pos); // New method
    void render(sf::RenderWindow& window, const sf::View& view, GameState currentState);

    const std::unordered_map<std::string, HUDElement>& getElements() const { return m_elements; }
    void configureGameplayHUD(const sf::Vector2u& winSize);
    void configureStoreHUD(const sf::Vector2u& winSize);
    void refreshHUDContent(GameState currentState, bool menuVisible, bool shopOpen, const sf::Vector2u& winSize, const Player& localPlayer);

private:
    sf::Font& m_font;
    std::unordered_map<std::string, HUDElement> m_elements;

    void drawWhiteBackground(sf::RenderWindow& window);
    bool isMouseOverText(const sf::RenderWindow& window, const sf::Text& text);
};

#endif // HUD_H