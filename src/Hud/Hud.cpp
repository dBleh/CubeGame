#include "HUD.h"
#include <cmath>

//-------------------------------------------------------------------------
// Constructor
//-------------------------------------------------------------------------
HUD::HUD(sf::Font& font)
    : m_font(font)
{
}

//-------------------------------------------------------------------------
// HUD Element Management
//-------------------------------------------------------------------------
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
    text.setFillColor(sf::Color::Black);
    text.setStyle(sf::Text::Regular);

    HUDElement element;
    element.text         = text;
    element.pos          = pos;
    element.visibleState = visibleState;
    element.mode         = mode;
    element.hoverable    = hoverable;
    element.baseColor    = sf::Color::Black;
    element.hoverColor   = sf::Color(60, 60, 60);

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

void HUD::updateElementPosition(const std::string& id, const sf::Vector2f& pos)
{
    auto it = m_elements.find(id);
    if (it != m_elements.end()) {
        it->second.pos = pos;
    }
}

//-------------------------------------------------------------------------
// Rendering Methods
//-------------------------------------------------------------------------
void HUD::render(sf::RenderWindow& window, const sf::View& view, GameState currentState) {
    sf::View originalView = window.getView();
    sf::Vector2f viewTopLeft = view.getCenter() - (view.getSize() * 0.5f);

    for (auto& [id, element] : m_elements) {
        if (element.visibleState == currentState) {
            sf::Text& text = element.text;
            if (element.mode == RenderMode::ScreenSpace) {
                window.setView(window.getDefaultView());
                text.setPosition(element.pos);
            } else {
                window.setView(view);
                text.setPosition(viewTopLeft + element.pos);
            }
            if (element.hoverable && isMouseOverText(window, text))
                text.setFillColor(element.hoverColor);
            else
                text.setFillColor(element.baseColor);
            window.draw(text);
        }
    }
    window.setView(originalView);
}

void HUD::drawWhiteBackground(sf::RenderWindow& window)
{
    sf::RectangleShape bg(sf::Vector2f(static_cast<float>(window.getSize().x), 
                                         static_cast<float>(window.getSize().y)));
    bg.setFillColor(sf::Color::White);
    bg.setPosition(0.f, 0.f);
    window.draw(bg);
}

bool HUD::isMouseOverText(const sf::RenderWindow& window, const sf::Text& text)
{
    sf::Vector2i mousePos = sf::Mouse::getPosition(window);
    sf::Vector2f mousePosF(static_cast<float>(mousePos.x), static_cast<float>(mousePos.y));
    sf::FloatRect bounds = text.getGlobalBounds();
    return bounds.contains(mousePosF);
}

bool HUD::isFullyLoaded() const {
    return !m_elements.empty();
}

//-------------------------------------------------------------------------
// HUD Configuration Methods
//-------------------------------------------------------------------------
void HUD::configureGameplayHUD(const sf::Vector2u& winSize) {
    // Game status element
    addElement("gameStatus", "Playing", 16,
               sf::Vector2f(10.f, 10.f),
               GameState::Playing, RenderMode::ViewSpace, false);
    updateBaseColor("gameStatus", sf::Color::White);

    // Level display element
    addElement("level", "Level: 0\nEnemies: 0\nHP: 100", 16,
               sf::Vector2f(10.f, 50.f),
               GameState::Playing, RenderMode::ViewSpace, false);
    updateBaseColor("level", sf::Color::White);

    // Scoreboard element
    addElement("scoreboard", "Scoreboard:\n", 16,
               sf::Vector2f(SCREEN_WIDTH - 200.f, 10.f),
               GameState::Playing, RenderMode::ViewSpace, false);
    updateBaseColor("scoreboard", sf::Color::White);

    // Next level timer element
    addElement("nextLevelTimer", "", 16,
               sf::Vector2f(0.5f * winSize.x - 50.f, 0.10f * winSize.y),
               GameState::Playing, RenderMode::ViewSpace, false);
    updateBaseColor("nextLevelTimer", sf::Color::White);

    // Pause menu element
    addElement("pauseMenu", "Paused\nPress M to Return to Main Menu\nPress ESC to Resume", 24,
               sf::Vector2f(0.5f * winSize.x - 150.f, 0.3f * winSize.y),
               GameState::Playing, RenderMode::ScreenSpace, false);
    updateBaseColor("pauseMenu", sf::Color::White);
}

void HUD::configureStoreHUD(const sf::Vector2u& winSize) {
    // Store title element
    addElement("storeTitle", "Store (Press B to Close)", 24,
               sf::Vector2f(0.5f * winSize.x - 100.f, 0.05f * winSize.y),
               GameState::Playing, RenderMode::ScreenSpace, false);
    updateBaseColor("storeTitle", sf::Color::White);

    // Store money element
    addElement("storeMoney", "Money: 0", 20,
               sf::Vector2f(0.5f * winSize.x - 80.f, 0.15f * winSize.y),
               GameState::Playing, RenderMode::ScreenSpace, false);
    updateBaseColor("storeMoney", sf::Color::Yellow);

    // Speed boost button element
    addElement("speedBoostButton", "Speed Boost (+50) - 50", 20,
               sf::Vector2f(0.5f * winSize.x - 80.f, 0.25f * winSize.y),
               GameState::Playing, RenderMode::ScreenSpace, true);
    updateBaseColor("speedBoostButton", sf::Color::White);
}

//-------------------------------------------------------------------------
// HUD Refresh Methods
//-------------------------------------------------------------------------
/*void HUD::refreshHUDContent(GameState currentState, bool menuVisible, bool shopOpen, const sf::Vector2u& winSize, const Player& localPlayer) {
    // Update store elements if shop is open.
    if (shopOpen) {
        updateElementPosition("storeTitle", sf::Vector2f(0.5f * winSize.x - 100.f, 0.05f * winSize.y));
        updateElementPosition("storeMoney", sf::Vector2f(0.5f * winSize.x - 80.f, 0.15f * winSize.y));
        updateElementPosition("speedBoostButton", sf::Vector2f(0.5f * winSize.x - 80.f, 0.25f * winSize.y));
        updateText("storeMoney", "Money: " + std::to_string(localPlayer.money));
        updateText("storeTitle", "Store (Press B to Close)");
        updateText("speedBoostButton", "Speed Boost (+50) - 50");
    } else {
        updateText("storeTitle", "");
        updateText("storeMoney", "");
        updateText("speedBoostButton", "");
    }
    
    // Update pause menu element.
    if (currentState == GameState::Playing)
        updateText("pauseMenu", menuVisible ? "Paused\nPress M to Return to Main Menu\nPress ESC to Resume" : "");
    else
        updateText("pauseMenu", "");
}
*/
/*
void HUD::refreshGameInfo(const sf::Vector2u& winSize,
                          int currentLevel,
                          size_t enemyCount,
                          const Player& localPlayer,
                          float nextLevelTimer,
                          const std::unordered_map<CSteamID, Player, CSteamIDHash>& players)
{
    // Update level display.
    updateElementPosition("level", sf::Vector2f(0.05f * winSize.x, 0.10f * winSize.y));
    updateText("level",
        "Level: " + std::to_string(currentLevel) +
        "\nEnemies: " + std::to_string(enemyCount) +
        "\nHP: " + std::to_string(localPlayer.health) +
        "\nKills: " + std::to_string(localPlayer.kills) +
        "\nMoney: " + std::to_string(localPlayer.money)
    );

    // Update game status.
    updateElementPosition("gameStatus", sf::Vector2f(0.05f * winSize.x, 0.05f * winSize.y));
    updateText("gameStatus",
        (nextLevelTimer > 0) ?
            "Next Wave in: " + std::to_string(static_cast<int>(nextLevelTimer + 0.5f)) + "s"
          : "Playing"
    );

    // Update next level timer element.
    updateElementPosition("nextLevelTimer", sf::Vector2f(0.5f * winSize.x - 50.f, 0.10f * winSize.y));
    if (nextLevelTimer > 0)
        updateText("nextLevelTimer", "Next Wave: " + std::to_string(static_cast<int>(nextLevelTimer + 0.5f)) + "s");
    else
        updateText("nextLevelTimer", "");

    // Update scoreboard.
    updateElementPosition("scoreboard", sf::Vector2f(0.75f * winSize.x, 0.05f * winSize.y));
    std::string scoreboard = "Scoreboard:\n";
    for (const auto& playerPair : players) {
        const Player& player = playerPair.second;
        const char* steamName = SteamFriends() ? SteamFriends()->GetFriendPersonaName(player.steamID) : "Unknown";
        if (!steamName || steamName[0] == '\0')
            steamName = "Unknown";
        scoreboard += std::string(steamName) +
                      ": Kills=" + std::to_string(player.kills) +
                      ", HP=" + std::to_string(player.health) + "\n";
    }
    updateText("scoreboard", scoreboard);
}
*/
/*
void HUD::updateScoreboard(const std::unordered_map<CSteamID, Player, CSteamIDHash>& players) {
    std::string scoreboard = "Scoreboard:\n";
    for (const auto& playerPair : players) {
        const Player& player = playerPair.second;
        const char* steamName = SteamFriends() ? SteamFriends()->GetFriendPersonaName(player.steamID) : "Unknown";
        if (!steamName || steamName[0] == '\0')
            steamName = "Unknown";
        scoreboard += std::string(steamName) +
                      ": Kills=" + std::to_string(player.kills) +
                      ", HP=" + std::to_string(player.health) + "\n";
    }
    updateText("scoreboard", scoreboard);
}
    */