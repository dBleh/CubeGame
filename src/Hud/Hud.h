#ifndef HUD_H
#define HUD_H

#include <SFML/Graphics.hpp>
#include <string>
#include <unordered_map>
#include "../States/GameState.h"
#include "../Utils/Config.h"
#include "../Entities/Player.h"
#include "../Utils/SteamHelpers.h"

/**
 * @brief Class for managing Heads-Up Display (HUD) elements.
 *
 * Provides functionality to add, update, and render HUD elements on screen.
 */
class HUD
{
public:
    /**
     * @brief Rendering modes for HUD elements.
     */
    enum class RenderMode {
        ScreenSpace, ///< Render relative to the screen.
        ViewSpace    ///< Render relative to the game view.
    };

    /**
     * @brief Structure representing a single HUD element.
     */
    struct HUDElement {
        sf::Text text;          ///< Text for the HUD element.
        sf::Vector2f pos;       ///< Position of the element.
        GameState visibleState; ///< Game state in which the element is visible.
        RenderMode mode;        ///< Rendering mode.
        bool hoverable;         ///< Whether the element responds to mouse hover.
        sf::Color baseColor;    ///< Default text color.
        sf::Color hoverColor;   ///< Text color when hovered.
    };

    /**
     * @brief Constructor.
     * @param font Reference to the font used for HUD elements.
     */
    explicit HUD(sf::Font& font);

    /**
     * @brief Checks if the HUD is fully loaded.
     * @return True if at least one HUD element exists.
     */
    bool isFullyLoaded() const;

    /**
     * @brief Updates the scoreboard with current player stats.
     * @param players Map of players.
     */
    void updateScoreboard(const std::unordered_map<CSteamID, Player, CSteamIDHash>& players);

    /**
     * @brief Adds a HUD element.
     * @param id Unique identifier for the element.
     * @param content Text content.
     * @param size Character size.
     * @param pos Position vector.
     * @param visibleState Game state when visible.
     * @param mode Render mode.
     * @param hoverable Whether it is hoverable.
     */
    void addElement(const std::string& id,
                    const std::string& content,
                    unsigned int size,
                    sf::Vector2f pos,
                    GameState visibleState,
                    RenderMode mode = RenderMode::ScreenSpace,
                    bool hoverable = false);

    /**
     * @brief Refreshes game info on the HUD.
     * @param winSize Window size.
     * @param currentLevel Current level.
     * @param enemyCount Number of enemies.
     * @param localPlayer Reference to local player.
     * @param nextLevelTimer Timer for next wave.
     * @param players Map of players.
     */
    void refreshGameInfo(const sf::Vector2u& winSize,
                         int currentLevel,
                         size_t enemyCount,
                         const Player& localPlayer,
                         float nextLevelTimer,
                         const std::unordered_map<CSteamID, Player, CSteamIDHash>& players);

    /**
     * @brief Updates the text content of a HUD element.
     * @param id Element ID.
     * @param content New text content.
     */
    void updateText(const std::string& id, const std::string& content);

    /**
     * @brief Updates the base color of a HUD element.
     * @param id Element ID.
     * @param color New base color.
     */
    void updateBaseColor(const std::string& id, const sf::Color& color);

    /**
     * @brief Updates the position of a HUD element.
     * @param id Element ID.
     * @param pos New position.
     */
    void updateElementPosition(const std::string& id, const sf::Vector2f& pos);

    /**
     * @brief Renders HUD elements on the window.
     * @param window Render window.
     * @param view Current game view.
     * @param currentState Current game state.
     */
    void render(sf::RenderWindow& window, const sf::View& view, GameState currentState);

    /**
     * @brief Returns a constant reference to the HUD elements.
     * @return Map of HUD elements.
     */
    const std::unordered_map<std::string, HUDElement>& getElements() const { return m_elements; }

    /**
     * @brief Configures HUD elements for gameplay.
     * @param winSize Window size.
     */
    void configureGameplayHUD(const sf::Vector2u& winSize);

    /**
     * @brief Configures HUD elements for the store.
     * @param winSize Window size.
     */
    void configureStoreHUD(const sf::Vector2u& winSize);

    /**
     * @brief Refreshes HUD content based on the current state.
     * @param currentState Current game state.
     * @param menuVisible Whether the pause menu is visible.
     * @param shopOpen Whether the store is open.
     * @param winSize Window size.
     * @param localPlayer Reference to local player.
     */
    void refreshHUDContent(GameState currentState, bool menuVisible, bool shopOpen, const sf::Vector2u& winSize, const Player& localPlayer);

private:
    sf::Font& m_font; ///< Reference to the font used for HUD elements.
    std::unordered_map<std::string, HUDElement> m_elements; ///< Map of HUD elements.

    /**
     * @brief Draws a white background on the window.
     * @param window Render window.
     */
    void drawWhiteBackground(sf::RenderWindow& window);

    /**
     * @brief Checks if the mouse is over the given text.
     * @param window Render window.
     * @param text SFML text object.
     * @return True if the mouse is over the text.
     */
    bool isMouseOverText(const sf::RenderWindow& window, const sf::Text& text);
};

#endif // HUD_H
