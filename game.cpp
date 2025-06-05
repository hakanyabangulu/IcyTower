#include <SFML/Graphics.hpp>
#include <vector>
#include <cstdlib>
#include <ctime>
#include <string>
#include <sstream>
#include <iostream>
#include <cmath>

const int WIDTH = 400;
const int HEIGHT = 600;
const int PLATFORM_COUNT = 12;
const float GRAVITY = 0.4f; // Yavaşlatıldı
const float JUMP_FORCE = -13.0f; // Yavaşlatıldı
const float WALL_JUMP_FORCE = -9.0f; // Yavaşlatıldı
const float PLAYER_MAX_SPEED = 6.0f; // Yavaşlatıldı
const float DOUBLE_JUMP_FORCE = -11.0f; // Yavaşlatıldı
const float POWERUP_DURATION = 5.0f;

enum class PlatformType { Normal, Moving };
enum class PowerUpType { None, SpeedBoost, ExtraJump };

class TextureManager {
public:
    sf::Texture playerTexture;
    sf::Texture platformTexture;
    sf::Texture backgroundTexture;
    sf::Texture gameOverTexture;
    sf::Font font;

    TextureManager() {
        if (!playerTexture.loadFromFile("pop.png")) {
            std::cerr << "Failed to load player.png." << std::endl;
            exit(1);
        }
        if (!platformTexture.loadFromFile("step.png")) {
            std::cerr << "Failed to load step.png." << std::endl;
            exit(1);
        }
        if (!backgroundTexture.loadFromFile("background.png")) {
            std::cerr << "Failed to load background.png." << std::endl;
            exit(1);
        }
        if (!gameOverTexture.loadFromFile("gameover.png")) {
            std::cerr << "Failed to load gameover.png." << std::endl;
            exit(1);
        }
        if (!font.loadFromFile("DejaVuSans.ttf")) {
            std::cerr << "Failed to load DejaVuSans.ttf." << std::endl;
            exit(1);
        }
    }
};

class Platform {
public:
    sf::Sprite shape;
    sf::Sprite shadow;
    PlatformType type;
    float speed;
    bool active;
    float pulseTime;
    bool scored;
    PowerUpType powerUp;

    Platform(float x, float y, PlatformType t, sf::Texture& texture) 
        : type(t), speed(0), active(true), pulseTime(0), scored(false), powerUp(PowerUpType::None) {
        shape.setTexture(texture);
        shape.setPosition(x, y);
        sf::Vector2u textureSize = texture.getSize();
        shape.setScale(100.0f / textureSize.x, 15.0f / textureSize.y);
        if (type == PlatformType::Moving) {
            shape.setColor(sf::Color(200, 150, 150, 255));
        }

        shadow.setTexture(texture);
        shadow.setPosition(x + 5, y + 5);
        shadow.setScale(100.0f / textureSize.x, 15.0f / textureSize.y);
        shadow.setColor(sf::Color(0, 0, 0, 100));
        if (type == PlatformType::Moving) {
            shadow.setColor(sf::Color(0, 0, 0, 50));
            speed = (rand() % 3 + 1) * 1.0f * (rand() % 2 == 0 ? 1.0f : -1.0f); // Yavaşlatıldı
        }

        if (rand() % 100 < 10) {
            powerUp = static_cast<PowerUpType>(rand() % 2 + 1);
        }
    }

    void update(float deltaTime) {
        if (type == PlatformType::Moving) {
            shape.move(speed * deltaTime * 60.0f, 0);
            shadow.move(speed * deltaTime * 60.0f, 0);
            pulseTime += deltaTime;
            float scale = 1.0f + 0.1f * std::sin(pulseTime * 3.0f);
            sf::Vector2u textureSize = shape.getTexture()->getSize();
            shape.setScale((100.0f / textureSize.x) * scale, 15.0f / textureSize.y);
            shadow.setScale((100.0f / textureSize.x) * scale, 15.0f / textureSize.y);
            if (shape.getPosition().x < 0 || shape.getPosition().x > WIDTH - shape.getGlobalBounds().width) {
                speed = -speed;
            }
        }
    }
};

class Particle {
public:
    sf::CircleShape shape;
    sf::Vector2f velocity;
    float lifetime;

    Particle(float x, float y, float size = 2.0f) : lifetime(0.5f) {
        shape.setRadius(size);
        shape.setFillColor(sf::Color(rand() % 255, rand() % 255, rand() % 255, 200));
        shape.setPosition(x, y);
        velocity = sf::Vector2f((rand() % 200 - 100) / 100.0f, (rand() % 200 - 100) / 100.0f);
    }

    bool update(float deltaTime) {
        shape.move(velocity * deltaTime * 60.0f);
        lifetime -= deltaTime;
        shape.setFillColor(sf::Color(
            shape.getFillColor().r,
            shape.getFillColor().g,
            shape.getFillColor().b,
            static_cast<sf::Uint8>(200 * lifetime / 0.5f)
        ));
        return lifetime <= 0;
    }
};

class Player {
public:
    sf::Sprite characterSprite;
    float dx, dy;
    bool canJump, canDoubleJump, isWallJumping, facingRight;
    sf::Vector2f lastWall;
    bool hasSpeedBoost, hasExtraJump;
    float powerUpTimer;

    Player(float x, float y, sf::Texture& texture) 
        : dx(0), dy(0), canJump(true), canDoubleJump(false), isWallJumping(false), facingRight(true),
          hasSpeedBoost(false), hasExtraJump(false), powerUpTimer(0.0f) {
        characterSprite.setTexture(texture);
        characterSprite.setPosition(x, y);
        sf::Vector2u textureSize = texture.getSize();
        characterSprite.setScale(40.0f / textureSize.x, 40.0f / textureSize.y);
    }

    void reset(float x, float y) {
        characterSprite.setPosition(x, y);
        dx = 0;
        dy = 0;
        canJump = true;
        canDoubleJump = false;
        isWallJumping = false;
        facingRight = true;
        hasSpeedBoost = false;
        hasExtraJump = false;
        powerUpTimer = 0.0f;
    }

    void update(float deltaTime) {
        dy += GRAVITY * deltaTime * 60.0f;
        characterSprite.move(dx * deltaTime * 60.0f, dy * deltaTime * 60.0f);

        if (hasSpeedBoost || hasExtraJump) {
            powerUpTimer -= deltaTime;
            if (powerUpTimer <= 0) {
                hasSpeedBoost = false;
                hasExtraJump = false;
            }
        }

        if (dx > 0 && !facingRight) {
            facingRight = true;
            characterSprite.setScale(std::abs(characterSprite.getScale().x), characterSprite.getScale().y);
        } else if (dx < 0 && facingRight) {
            facingRight = false;
            characterSprite.setScale(-std::abs(characterSprite.getScale().x), characterSprite.getScale().y);
        }
    }

    void handleInput() {
        dx = 0;
        float speed = hasSpeedBoost ? PLAYER_MAX_SPEED * 1.5f : PLAYER_MAX_SPEED;
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Left)) dx = -speed;
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Right)) dx = speed;
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Space) && (canJump || canDoubleJump || (hasExtraJump && !canDoubleJump))) {
            if (canJump) {
                dy = JUMP_FORCE;
                canJump = false;
                canDoubleJump = true;
            } else if (canDoubleJump) {
                dy = DOUBLE_JUMP_FORCE;
                canDoubleJump = false;
            } else if (hasExtraJump) {
                dy = DOUBLE_JUMP_FORCE;
                hasExtraJump = false;
            }
        }
    }

    void checkWallJump() {
        sf::FloatRect bounds = getBounds();
        if (!canJump && bounds.left <= 0 && sf::Keyboard::isKeyPressed(sf::Keyboard::Space)) {
            dy = WALL_JUMP_FORCE;
            dx = PLAYER_MAX_SPEED;
            isWallJumping = true;
            lastWall = sf::Vector2f(0, bounds.top);
        } else if (!canJump && bounds.left >= WIDTH - bounds.width && sf::Keyboard::isKeyPressed(sf::Keyboard::Space)) {
            dy = WALL_JUMP_FORCE;
            dx = -PLAYER_MAX_SPEED;
            isWallJumping = true;
            lastWall = sf::Vector2f(WIDTH, bounds.top);
        } else if (isWallJumping && std::abs(bounds.top - lastWall.y) > 50) {
            isWallJumping = false;
            if (sf::Keyboard::isKeyPressed(sf::Keyboard::Left)) dx = -PLAYER_MAX_SPEED;
            else if (sf::Keyboard::isKeyPressed(sf::Keyboard::Right)) dx = PLAYER_MAX_SPEED;
            else dx = 0;
        }
    }

    void wrapScreen() {
        sf::FloatRect bounds = getBounds();
        if (bounds.left < 0) {
            characterSprite.setPosition(-bounds.left, bounds.top);
        }
        if (bounds.left > WIDTH - bounds.width) {
            characterSprite.setPosition(WIDTH - bounds.width, bounds.top);
        }
    }

    sf::FloatRect getBounds() const {
        return characterSprite.getGlobalBounds();
    }
};

class Menu {
public:
    sf::RenderWindow window;
    std::vector<sf::Text> menuItems;
    sf::Text welcomeText;
    int selectedItem;
    sf::Font& font;
    sf::Sprite backgroundSprite;

    Menu(sf::Font& f, sf::Texture& bgTexture) : window(sf::VideoMode(WIDTH, HEIGHT), "Icy Tower Menu"), selectedItem(0), font(f) {
        backgroundSprite.setTexture(bgTexture);
        backgroundSprite.setScale(
            static_cast<float>(WIDTH) / bgTexture.getSize().x,
            static_cast<float>(HEIGHT) / bgTexture.getSize().y
        );

        welcomeText.setFont(font);
        welcomeText.setCharacterSize(24);
        welcomeText.setFillColor(sf::Color::White);
        welcomeText.setString("Jump to the Top!\nUse Arrows & Space\nPress Enter to Start");
        welcomeText.setPosition(WIDTH / 2 - welcomeText.getGlobalBounds().width / 2, HEIGHT / 4);

        std::string choices[] = {"Play", "Exit"};
        for (int i = 0; i < 2; i++) {
            sf::Text text(choices[i], font, 30);
            text.setFillColor(i == selectedItem ? sf::Color::Yellow : sf::Color::White);
            text.setPosition(WIDTH / 2 - text.getGlobalBounds().width / 2, HEIGHT / 2 + i * 50);
            menuItems.push_back(text);
        }
    }

    bool run() {
        while (window.isOpen()) {
            sf::Event event;
            while (window.pollEvent(event)) {
                if (event.type == sf::Event::Closed) window.close();
                if (event.type == sf::Event::KeyReleased) {
                    if (event.key.code == sf::Keyboard::Up) {
                        menuItems[selectedItem].setFillColor(sf::Color::White);
                        selectedItem = (selectedItem - 1 + 2) % 2;
                        menuItems[selectedItem].setFillColor(sf::Color::Yellow);
                    }
                    if (event.key.code == sf::Keyboard::Down) {
                        menuItems[selectedItem].setFillColor(sf::Color::White);
                        selectedItem = (selectedItem + 1) % 2;
                        menuItems[selectedItem].setFillColor(sf::Color::Yellow);
                    }
                    if (event.key.code == sf::Keyboard::Return) {
                        window.close();
                        return selectedItem == 0;
                    }
                }
            }
            window.clear();
            window.draw(backgroundSprite);
            window.draw(welcomeText);
            for (const auto& item : menuItems) {
                window.draw(item);
            }
            window.display();
        }
        return false;
    }
};

class Game {
private:
    sf::RenderWindow window;
    TextureManager textures;
    Player player;
    std::vector<Platform> platforms;
    std::vector<Particle> particles;
    sf::Sprite backgroundSprite;
    sf::Sprite gameOverSprite;
    sf::Text scoreText, highScoreText, retryText, quitText, pauseText, finalScoreText, powerUpText;
    float highestY;
    int score, highScore;
    bool gameOver;
    bool paused;
    float gameSpeed;
    float cameraY;
    float backgroundOffset;
    float powerUpMessageTimer;

public:
    Game() : player(WIDTH / 2, HEIGHT - 100, textures.playerTexture),
             highestY(HEIGHT - 100), score(0), highScore(0), gameOver(false), paused(false), 
             gameSpeed(1.0f), cameraY(0), backgroundOffset(0), powerUpMessageTimer(0.0f) {
        backgroundSprite.setTexture(textures.backgroundTexture);
        backgroundSprite.setScale(
            static_cast<float>(WIDTH) / textures.backgroundTexture.getSize().x,
            static_cast<float>(HEIGHT) / textures.backgroundTexture.getSize().y
        );

        gameOverSprite.setTexture(textures.gameOverTexture);
        gameOverSprite.setScale(
            static_cast<float>(WIDTH) / gameOverSprite.getTexture()->getSize().x,
            static_cast<float>(HEIGHT) / gameOverSprite.getTexture()->getSize().y
        );
        gameOverSprite.setPosition(0, 0);

        scoreText.setFont(textures.font);
        scoreText.setCharacterSize(24);
        scoreText.setFillColor(sf::Color::White);
        scoreText.setPosition(10, 10);

        highScoreText.setFont(textures.font);
        highScoreText.setCharacterSize(24);
        highScoreText.setFillColor(sf::Color::Yellow);
        highScoreText.setPosition(10, 40);

        retryText.setFont(textures.font);
        retryText.setCharacterSize(20);
        retryText.setFillColor(sf::Color::Green);
        retryText.setString("R to Retry!");
        retryText.setPosition(WIDTH / 2 - retryText.getGlobalBounds().width / 2, HEIGHT / 2 + 60);

        quitText.setFont(textures.font);
        quitText.setCharacterSize(20);
        quitText.setFillColor(sf::Color::Red);
        quitText.setString("Q to Quit!");
        quitText.setPosition(WIDTH / 2 - quitText.getGlobalBounds().width / 2, HEIGHT / 2 + 90);

        pauseText.setFont(textures.font);
        pauseText.setCharacterSize(30);
        pauseText.setFillColor(sf::Color::Yellow);
        pauseText.setString("Paused");
        pauseText.setPosition(WIDTH / 2 - pauseText.getGlobalBounds().width / 2, HEIGHT / 2);

        finalScoreText.setFont(textures.font);
        finalScoreText.setCharacterSize(20);
        finalScoreText.setFillColor(sf::Color::White);
        finalScoreText.setPosition(WIDTH / 2 - finalScoreText.getGlobalBounds().width / 2, HEIGHT / 2 - 20);

        powerUpText.setFont(textures.font);
        powerUpText.setCharacterSize(18);
        powerUpText.setFillColor(sf::Color::Cyan);
        powerUpText.setPosition(WIDTH / 2 - powerUpText.getGlobalBounds().width / 2, HEIGHT / 2);

        srand(static_cast<unsigned>(time(0)));
        platforms.emplace_back(WIDTH / 2 - 200, HEIGHT - 15, PlatformType::Normal, textures.platformTexture);
        platforms.back().shape.setScale(400.0f / textures.platformTexture.getSize().x, 15.0f / textures.platformTexture.getSize().y);
        platforms.back().shadow.setScale(400.0f / textures.platformTexture.getSize().x, 15.0f / textures.platformTexture.getSize().y);
        for (int i = 0; i < PLATFORM_COUNT - 1; i++) {
            float x = rand() % (WIDTH - 100);
            float y = HEIGHT - ((i + 1) * HEIGHT / (PLATFORM_COUNT - 1));
            PlatformType type = static_cast<PlatformType>(rand() % 2);
            platforms.emplace_back(x, y, type, textures.platformTexture);
        }
    }

    void spawnParticles(float x, float y, float size = 2.0f) {
        for (int i = 0; i < 5; i++) {
            particles.emplace_back(x, y, size);
        }
    }

    void handleCollisions() {
        if (player.dy <= 0) return;

        for (auto& plat : platforms) {
            if (!plat.active) continue;
            sf::FloatRect playerBounds = player.getBounds();
            sf::FloatRect platformBounds = plat.shape.getGlobalBounds();

            if (playerBounds.intersects(platformBounds)) {
                if (player.dy > 0 && playerBounds.top + playerBounds.height > platformBounds.top &&
                    playerBounds.top + playerBounds.height <= platformBounds.top + 5) {
                    player.dy = 0;
                    player.characterSprite.setPosition(playerBounds.left, platformBounds.top - playerBounds.height);
                    player.canJump = true;
                    player.canDoubleJump = true;
                    spawnParticles(playerBounds.left, playerBounds.top + playerBounds.height, 3.0f);

                    int points = plat.type == PlatformType::Moving ? 15 : 10;
                    if (!plat.scored) {
                        score += points;
                        plat.scored = true;
                    }

                    if (plat.powerUp != PowerUpType::None) {
                        if (plat.powerUp == PowerUpType::SpeedBoost) {
                            player.hasSpeedBoost = true;
                            player.powerUpTimer = POWERUP_DURATION;
                            powerUpText.setString("Speed Boost!");
                            powerUpMessageTimer = 1.0f;
                        } else if (plat.powerUp == PowerUpType::ExtraJump) {
                            player.hasExtraJump = true;
                            player.powerUpTimer = POWERUP_DURATION;
                            powerUpText.setString("Extra Jump!");
                            powerUpMessageTimer = 1.0f;
                        }
                        plat.powerUp = PowerUpType::None;
                        spawnParticles(playerBounds.left, playerBounds.top, 5.0f);
                    }
                }
            }
        }
    }

    void updateCamera(float deltaTime) {
        sf::FloatRect playerBounds = player.getBounds();
        float targetCameraY = playerBounds.top - HEIGHT / 2;
        if (targetCameraY > 0) targetCameraY = 0;
        cameraY += (targetCameraY - cameraY) * 3.0f * deltaTime; // Yavaşlatıldı

        player.characterSprite.move(0, -cameraY);
        for (auto& plat : platforms) {
            plat.shape.move(0, -cameraY);
            plat.shadow.move(0, -cameraY);
            if (plat.shape.getPosition().y > HEIGHT) {
                plat.shape.setPosition(rand() % (WIDTH - 100), -15);
                plat.shadow.setPosition(plat.shape.getPosition().x + 5, -15 + 5);
                plat.active = true;
                plat.scored = false;
                plat.powerUp = PowerUpType::None;
                plat.type = static_cast<PlatformType>(rand() % 2);
                if (plat.type == PlatformType::Moving) {
                    plat.speed = (rand() % 3 + 1) * 1.0f * (rand() % 2 == 0 ? 1.0f : -1.0f);
                    plat.shape.setColor(sf::Color(200, 150, 150, 255));
                    plat.shadow.setColor(sf::Color(0, 0, 0, 50));
                } else {
                    plat.shape.setColor(sf::Color::White);
                    plat.shadow.setColor(sf::Color(0, 0, 0, 100));
                }
            }
        }

        highestY = std::min(highestY, playerBounds.top);
        gameSpeed = 1.0f + score / 500.0f; // Yavaşlatıldı

        for (auto& plat : platforms) {
            if (plat.type == PlatformType::Moving) {
                plat.speed *= (1.0f + score / 10000.0f); // Dengelendi
            }
        }
    }

    void updateText() {
        std::stringstream ss;
        ss << "Score: " << score;
        scoreText.setString(ss.str());
        ss.str("");
        ss << "High Score: " << highScore;
        highScoreText.setString(ss.str());
        if (scoreText.getPosition().y != 10 - cameraY) scoreText.setPosition(10, 10 - cameraY);
        if (highScoreText.getPosition().y != 40 - cameraY) highScoreText.setPosition(10, 40 - cameraY);

        ss.str("");
        ss << "Score: " << score << "\nBest: " << highScore;
        finalScoreText.setString(ss.str());

        powerUpText.setPosition(WIDTH / 2 - powerUpText.getGlobalBounds().width / 2, HEIGHT / 2 - cameraY);
    }

    void checkGameOver() {
        if (player.getBounds().top > HEIGHT) {
            gameOver = true;
            highScore = std::max(score, highScore);
        }
    }

    void reset() {
        player.reset(WIDTH / 2, HEIGHT - 100);
        cameraY = 0;
        highestY = HEIGHT - 100;
        score = 0;
        gameSpeed = 1.0f;
        backgroundOffset = 0;
        platforms.clear();
        particles.clear();
        platforms.emplace_back(WIDTH / 2 - 200, HEIGHT - 15, PlatformType::Normal, textures.platformTexture);
        platforms.back().shape.setScale(400.0f / textures.platformTexture.getSize().x, 15.0f / textures.platformTexture.getSize().y);
        platforms.back().shadow.setScale(400.0f / textures.platformTexture.getSize().x, 15.0f / textures.platformTexture.getSize().y);
        platforms.back().scored = true;
        for (int i = 0; i < PLATFORM_COUNT - 1; i++) {
            float x = rand() % (WIDTH - 100);
            float y = HEIGHT - ((i + 1) * HEIGHT / (PLATFORM_COUNT - 1));
            PlatformType type = static_cast<PlatformType>(rand() % 2);
            platforms.emplace_back(x, y, type, textures.platformTexture);
        }
        gameOver = false;
        paused = false;
        powerUpMessageTimer = 0.0f;
    }

    void run() {
        Menu menu(textures.font, textures.backgroundTexture);
        if (!menu.run()) return;
        menu.window.close();

        window.create(sf::VideoMode(WIDTH, HEIGHT), "Icy Tower");
        window.setFramerateLimit(60);
        window.setVerticalSyncEnabled(false);

        sf::Clock clock;
        while (window.isOpen()) {
            float deltaTime = clock.restart().asSeconds() * 1.0f; // Yavaşlatıldı

            sf::Event e;
            while (window.pollEvent(e)) {
                if (e.type == sf::Event::Closed) window.close();
                if (e.type == sf::Event::KeyPressed) {
                    if (e.key.code == sf::Keyboard::P && !gameOver) {
                        paused = !paused;
                    }
                    if (e.key.code == sf::Keyboard::R && gameOver) reset();
                    if (e.key.code == sf::Keyboard::Q && gameOver) window.close();
                }
            }

            if (!gameOver && !paused) {
                player.handleInput();
                player.checkWallJump();
                player.update(deltaTime);
                player.wrapScreen();
                for (auto& plat : platforms) plat.update(deltaTime);
                handleCollisions();
                updateCamera(deltaTime);

                for (auto it = particles.begin(); it != particles.end();) {
                    if (it->update(deltaTime)) it = particles.erase(it);
                    else ++it;
                }

                backgroundOffset -= 5 * deltaTime; // Yavaşlatıldı
                if (backgroundOffset <= -HEIGHT) backgroundOffset = 0;

                if (powerUpMessageTimer > 0) {
                    powerUpMessageTimer -= deltaTime;
                }
            }

            checkGameOver();
            updateText();

            if (!gameOver) {
                window.clear();
                backgroundSprite.setPosition(0, backgroundOffset);
                window.draw(backgroundSprite);
                if (backgroundOffset < 0) {
                    backgroundSprite.setPosition(0, backgroundOffset + HEIGHT);
                    window.draw(backgroundSprite);
                }

                for (const auto& plat : platforms) {
                    if (plat.active) {
                        window.draw(plat.shadow);
                        window.draw(plat.shape);
                    }
                }
                for (const auto& particle : particles) window.draw(particle.shape);
                window.draw(player.characterSprite);
                window.draw(scoreText);
                window.draw(highScoreText);
                if (powerUpMessageTimer > 0) {
                    window.draw(powerUpText);
                }
                if (paused) {
                    sf::RectangleShape overlay(sf::Vector2f(WIDTH, HEIGHT));
                    overlay.setFillColor(sf::Color(0, 0, 0, 128));
                    window.draw(overlay);
                    window.draw(pauseText);
                }
            } else {
                window.clear();
                gameOverSprite.setPosition(0, 0);
                retryText.setPosition(WIDTH / 2 - retryText.getGlobalBounds().width / 2, HEIGHT / 2 + 60);
                quitText.setPosition(WIDTH / 2 - quitText.getGlobalBounds().width / 2, HEIGHT / 2 + 90);
                window.draw(gameOverSprite);
                window.draw(finalScoreText);
                window.draw(retryText);
                window.draw(quitText);
            }
            window.display();
        }
    }
};

int main() {
    Game game;
    game.run();
    return 0;
}