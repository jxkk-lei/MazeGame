#include <SFML/Graphics.hpp>
#include <iostream>
#include <cstdlib>
#include <ctime>
#include <vector>
#include <map>
#include <queue>
#include <sstream>

using namespace std;

enum GameState { MAIN_MENU, PLAYING, FINISHED };
const int TILE_SIZE = 48;
const int VIEW_RADIUS = 6;
const int WALL = 0, PASS = 1;

class Maze {
private:
    int** maze;
    int width, height;

public:
    Maze(int w, int h) : width(w), height(h) {
        maze = new int*[height];
        for (int i = 0; i < height; ++i)
            maze[i] = new int[width];
    }

    ~Maze() {
        for (int i = 0; i < height; ++i)
            delete[] maze[i];
        delete[] maze;
    }

    int get(int x, int y) const {
        if (x < 0 || y < 0 || x >= width || y >= height) return WALL;
        return maze[y][x];
    }

    void set(int x, int y, int value) {
        if (x >= 0 && y >= 0 && x < width && y < height)
            maze[y][x] = value;
    }

    bool isWall(int x, int y) const {
        return get(x, y) == WALL;
    }

    int getWidth() const { return width; }
    int getHeight() const { return height; }

    bool isDeadEnd(int x, int y) const {
        int a = 0;
        if (x != 1 && get(x - 2, y) == PASS) a++; else if (x == 1) a++;
        if (y != 1 && get(x, y - 2) == PASS) a++; else if (y == 1) a++;
        if (x != width - 2 && get(x + 2, y) == PASS) a++; else if (x == width - 2) a++;
        if (y != height - 2 && get(x, y + 2) == PASS) a++; else if (y == height - 2) a++;
        return (a == 4);
    }

    void generate() {
        for (int i = 0; i < height; ++i)
            for (int j = 0; j < width; ++j)
                maze[i][j] = WALL;

        int x = 3, y = 3, a = 0;
        while (a < 10000) {
            maze[y][x] = PASS; a++;
            while (true) {
                int c = rand() % 4;
                switch (c) {
                    case 0: if (y != 1 && maze[y - 2][x] == WALL) {
                        maze[y - 1][x] = PASS; maze[y - 2][x] = PASS; y -= 2;
                    } break;
                    case 1: if (y != height - 2 && maze[y + 2][x] == WALL) {
                        maze[y + 1][x] = PASS; maze[y + 2][x] = PASS; y += 2;
                    } break;
                    case 2: if (x != 1 && maze[y][x - 2] == WALL) {
                        maze[y][x - 1] = PASS; maze[y][x - 2] = PASS; x -= 2;
                    } break;
                    case 3: if (x != width - 2 && maze[y][x + 2] == WALL) {
                        maze[y][x + 1] = PASS; maze[y][x + 2] = PASS; x += 2;
                    } break;
                }
                if (isDeadEnd(x, y)) break;
            }
            if (isDeadEnd(x, y)) {
                do {
                    x = 2 * (rand() % ((width - 1) / 2)) + 1;
                    y = 2 * (rand() % ((height - 1) / 2)) + 1;
                } while (maze[y][x] != PASS);
            }
        }
        maze[height - 2][width - 2] = PASS;
    }

    vector<pair<int, int>> findShortestPath(int startX, int startY, int endX, int endY) {
        vector<pair<int, int>> path;
        map<pair<int, int>, pair<int, int>> parent;
        queue<pair<int, int>> q;
        q.push({startY, startX});
        parent[{startY, startX}] = {-1, -1};

        int dx[] = {0, 0, -1, 1};
        int dy[] = {-1, 1, 0, 0};

        while (!q.empty()) {
            auto [y, x] = q.front(); q.pop();
            if (x == endX && y == endY) {
                while (!(x == -1 && y == -1)) {
                    path.push_back({y, x});
                    tie(y, x) = parent[{y, x}];
                }
                reverse(path.begin(), path.end());
                break;
            }
            for (int i = 0; i < 4; ++i) {
                int nx = x + dx[i];
                int ny = y + dy[i];
                if (nx >= 0 && ny >= 0 && nx < width && ny < height &&
                    maze[ny][nx] == PASS && !parent.count({ny, nx})) {
                    parent[{ny, nx}] = {y, x};
                    q.push({ny, nx});
                }
            }
        }
        return path;
    }

    void draw(sf::RenderWindow& window, sf::Sprite wallSprites[4], sf::RectangleShape& passRect,
              int playerX, int playerY, bool fullView) const {
        for (int i = 0; i < height; i++) {
            for (int j = 0; j < width; j++) {
                bool inView = fullView || (abs(i - playerY) <= VIEW_RADIUS && abs(j - playerX) <= VIEW_RADIUS);
                if (inView) {
                    if (maze[i][j] == WALL) {
                        int textureIndex = (i + j) % 4;
                        wallSprites[textureIndex].setPosition(j * TILE_SIZE, i * TILE_SIZE);
                        window.draw(wallSprites[textureIndex]);
                    } else {
                        passRect.setPosition(j * TILE_SIZE, i * TILE_SIZE);
                        window.draw(passRect);
                    }
                }
            }
        }
    }
};

class Unit {
protected:
    int x, y;
    bool isMoving;

public:
    int getX() const { return x; }
    int getY() const { return y; }
    void setX(int val) { x = val; }
    void setY(int val) { y = val; }
    Unit(int startX = 0, int startY = 0) : x(startX), y(startY), isMoving(false) {}

    virtual void move(int dx, int dy, const Maze& maze) {
        int newX = x + dx;
        int newY = y + dy;
        if (!maze.isWall(newX, newY)) {
            x = newX;
            y = newY;
            isMoving = true;
        } else {
            isMoving = false;
        }
    }

    virtual void update() = 0;
    virtual void draw(sf::RenderWindow& window) = 0;
};


class Player : public Unit {
private:
    vector<sf::Texture> animationFrames;
    sf::Sprite sprite;
    int currentFrame;
    sf::Clock animationClock;
    float frameTime;

public:
    bool movementKeyPressed;

    Player(int startX = 1, int startY = 1)
        : Unit(startX, startY), currentFrame(0), frameTime(0.45f),
          movementKeyPressed(false) {
        loadAnimationFrames({
            "Tiles/slime1.png",
            "Tiles/slime2.png",
            "Tiles/slime3.png",
            "Tiles/slime4.png",
            "Tiles/slime5.png",
            "Tiles/slime6.png"
        });
    }

    void loadAnimationFrames(const vector<string>& texturePaths) {
        for (const auto& path : texturePaths) {
            sf::Texture texture;
            if (!texture.loadFromFile(path)) {
                cerr << "Error loading texture: " << path << endl;
                exit(1);
            }
            animationFrames.push_back(texture);
        }
        if (!animationFrames.empty()) {
            sprite.setTexture(animationFrames[0]);
            sprite.setScale(
                TILE_SIZE / static_cast<float>(animationFrames[0].getSize().x),
                TILE_SIZE / static_cast<float>(animationFrames[0].getSize().y)
            );
        }
    }

    void update() override {
        if (isMoving && animationClock.getElapsedTime().asSeconds() >= frameTime) {
            currentFrame = (currentFrame + 1) % animationFrames.size();
            sprite.setTexture(animationFrames[currentFrame]);
            animationClock.restart();
        }
        if (!isMoving) {
            currentFrame = 0;
            sprite.setTexture(animationFrames[0]);
        }
        isMoving = false;
    }

    void draw(sf::RenderWindow& window) override {
        sprite.setPosition(x * TILE_SIZE, y * TILE_SIZE);
        window.draw(sprite);
    }
};
class GameUI {
private:
    sf::Font font;
    sf::RectangleShape playButton, exitButton;
    sf::Text playText, exitText;
    sf::Text timeText, resultText;

public:
    GameUI() {
        if (!font.loadFromFile("Fonts/PixelOperator8.ttf")) {
            cerr << "Error loading font\n";
            exit(1);
        }

        playButton.setSize({300, 80});
        exitButton.setSize({300, 80});
        playButton.setFillColor(sf::Color(100, 200, 100));
        exitButton.setFillColor(sf::Color(200, 100, 100));

        playText.setFont(font);
        exitText.setFont(font);
        playText.setString("Play");
        exitText.setString("Exit");
        playText.setCharacterSize(50);
        exitText.setCharacterSize(50);
        playText.setFillColor(sf::Color::White);
        exitText.setFillColor(sf::Color::White);

        timeText.setFont(font);
        timeText.setCharacterSize(30);
        timeText.setFillColor(sf::Color::White);

        resultText.setFont(font);
        resultText.setCharacterSize(60);
        resultText.setFillColor(sf::Color::White);
    }

    void updateTimeText(float seconds) {
        timeText.setString("Time: " + to_string((int)seconds));
    }

    void updateResultText(float seconds) {
        stringstream ss;
        ss << "Finished in " << seconds << " seconds!";
        resultText.setString(ss.str());
        resultText.setPosition(1920 / 2 - resultText.getLocalBounds().width / 2, 1080 / 2 - 50);
    }

    void drawMainMenu(sf::RenderWindow& window) {
        playButton.setPosition(window.getSize().x / 2 - 150, 400);
        playText.setPosition(playButton.getPosition().x + 100, playButton.getPosition().y + 15);
        exitButton.setPosition(window.getSize().x / 2 - 150, 500);
        exitText.setPosition(exitButton.getPosition().x + 100, exitButton.getPosition().y + 15);

        window.draw(playButton);
        window.draw(playText);
        window.draw(exitButton);
        window.draw(exitText);
    }

    void drawGameUI(sf::RenderWindow& window) {
        timeText.setPosition(1600, 20);
        window.draw(timeText);
    }

    void drawResult(sf::RenderWindow& window) {
        window.draw(resultText);
    }

    bool isPlayButtonClicked(sf::RenderWindow& window, sf::Event& event) {
        sf::Vector2f mousePos = window.mapPixelToCoords(sf::Mouse::getPosition(window));
        playButton.setPosition(window.getSize().x / 2 - 150, 400);
        return playButton.getGlobalBounds().contains(mousePos);
    }

    bool isExitButtonClicked(sf::RenderWindow& window, sf::Event& event) {
        sf::Vector2f mousePos = window.mapPixelToCoords(sf::Mouse::getPosition(window));
        exitButton.setPosition(window.getSize().x / 2 - 150, 500);
        return exitButton.getGlobalBounds().contains(mousePos);
    }
};

class Game {
private:
    const int width = 61, height = 61;
    GameState currentState = MAIN_MENU;
    sf::RenderWindow window;

    Maze maze;
    Player player;
    std::vector<std::pair<int, int>> currentPath;

    sf::RectangleShape passRect, exitRect;
    sf::Texture wallTextures[4];
    sf::Sprite wallSprites[4];

    sf::Clock gameClock, finishClock;
    sf::Time finishTime;
    bool tHeld = false;
    bool fullView = false;

    GameUI ui;

public:
    Game() : window(sf::VideoMode(1920, 1080), "Maze"), maze(width, height) {
        window.setFramerateLimit(60);
        srand(static_cast<unsigned>(time(NULL)));

        if (!wallTextures[0].loadFromFile("Tiles/FieldsTile_01.png") ||
            !wallTextures[1].loadFromFile("Tiles/FieldsTile_02.png") ||
            !wallTextures[2].loadFromFile("Tiles/FieldsTile_03.png") ||
            !wallTextures[3].loadFromFile("Tiles/FieldsTile_04.png")) {
            std::cerr << "Error loading wall textures\n";
            exit(1);
        }

        for (int i = 0; i < 4; ++i) {
            wallSprites[i].setTexture(wallTextures[i]);
            wallSprites[i].setScale(
                TILE_SIZE / static_cast<float>(wallTextures[i].getSize().x),
                TILE_SIZE / static_cast<float>(wallTextures[i].getSize().y)
            );
        }

        passRect.setSize({TILE_SIZE, TILE_SIZE});
        exitRect.setSize({TILE_SIZE, TILE_SIZE});
        passRect.setFillColor(sf::Color::White);
        exitRect.setFillColor(sf::Color::Green);
    }

    void run() {
        while (window.isOpen()) {
            processEvents();
            update();
            render();
        }
    }

private:
    void processEvents() {
        sf::Event event;
        while (window.pollEvent(event)) {
            if (event.type == sf::Event::Closed) {
                window.close();
            }

            if (event.type == sf::Event::KeyPressed && event.key.code == sf::Keyboard::Escape) {
                if (currentState == PLAYING)
                    currentState = MAIN_MENU;
            }

            if (currentState == MAIN_MENU && event.type == sf::Event::MouseButtonPressed) {
                if (ui.isPlayButtonClicked(window, event)) {
                    startNewGame();
                } else if (ui.isExitButtonClicked(window, event)) {
                    window.close();
                }
            } else if (currentState == PLAYING) {
                handleGameInput(event);
            }
        }
    }

    void startNewGame() {
        maze.generate();
        player = Player(1, 1);

        while (maze.get(player.getX(), player.getY()) != PASS) {
            player.setX(player.getX() + 1);
            if (player.getX() >= maze.getWidth()) {
                player.setX(1);
                player.setY(player.getY() + 1);
                if (player.getY() >= maze.getHeight()) {
                    break;
                }
            }
        }

        gameClock.restart();
        currentPath.clear();
        currentState = PLAYING;
    }

    void handleGameInput(sf::Event& event) {
        static sf::Clock moveClock;
        const float moveDelay = 0.15f;

        if (event.type == sf::Event::KeyPressed) {
            if (event.key.code == sf::Keyboard::Tab) {
                fullView = !fullView;
            }
            if (event.key.code == sf::Keyboard::T) {
                tHeld = true;
                currentPath = maze.findShortestPath(player.getX(), player.getY(), width - 2, height - 2);
            }
            if (event.key.code == sf::Keyboard::R) {
                currentPath.clear();
            }
        }

        if (event.type == sf::Event::KeyReleased && event.key.code == sf::Keyboard::T) {
            tHeld = false;
        }

        if (moveClock.getElapsedTime().asSeconds() > moveDelay) {
            if (sf::Keyboard::isKeyPressed(sf::Keyboard::W)) {
                player.move(0, -1, maze);
            }
            if (sf::Keyboard::isKeyPressed(sf::Keyboard::S)) {
                player.move(0, 1, maze);
            }
            if (sf::Keyboard::isKeyPressed(sf::Keyboard::A)) {
                player.move(-1, 0, maze);
            }
            if (sf::Keyboard::isKeyPressed(sf::Keyboard::D)) {
                player.move(1, 0, maze);
            }
            moveClock.restart();
        }
    }

    void update() {
        if (currentState == PLAYING) {
            if (player.getX() == width - 2 && player.getY() == height - 2) {
                finishTime = gameClock.getElapsedTime();
                finishClock.restart();
                currentState = FINISHED;
                ui.updateResultText(finishTime.asSeconds());
            }
            player.update();
            ui.updateTimeText(gameClock.getElapsedTime().asSeconds());
        } else if (currentState == FINISHED && finishClock.getElapsedTime().asSeconds() >= 5) {
            currentState = MAIN_MENU;
        }
    }

    void render() {
        window.clear();

        if (currentState == MAIN_MENU) {
            window.setView(window.getDefaultView());
            ui.drawMainMenu(window);
        } else if (currentState == PLAYING) {
            setGameView();
            drawGameWorld();
            window.setView(window.getDefaultView());
            ui.drawGameUI(window);
        } else if (currentState == FINISHED) {
            window.setView(window.getDefaultView());
            ui.drawResult(window);
        }

        window.display();
    }

    void setGameView() {
        sf::View view;
        if (fullView) {
            view.setSize(width * TILE_SIZE, height * TILE_SIZE);
            view.setCenter(width * TILE_SIZE / 2.f, height * TILE_SIZE / 2.f);
        } else {
            view.setSize(1920.f, 1080.f);
            view.setCenter(
                clamp(player.getX() * TILE_SIZE, 1920 / 2, width * TILE_SIZE - 1920 / 2),
                clamp(player.getY() * TILE_SIZE, 1080 / 2, height * TILE_SIZE - 1080 / 2)
            );
        }
        window.setView(view);
    }

    void drawGameWorld() {
        maze.draw(window, wallSprites, passRect, player.getX(), player.getY(), fullView);

        sf::RectangleShape pathRect(sf::Vector2f(TILE_SIZE, TILE_SIZE));
        pathRect.setFillColor(sf::Color(255, 255, 128));
        for (const auto& p : currentPath) {
            if (fullView || (abs(p.first - player.getY()) <= VIEW_RADIUS && abs(p.second - player.getX()) <= VIEW_RADIUS)) {
                pathRect.setPosition(p.second * TILE_SIZE, p.first * TILE_SIZE);
                window.draw(pathRect);
            }
        }

        exitRect.setPosition((width - 2) * TILE_SIZE, (height - 2) * TILE_SIZE);
        window.draw(exitRect);

        player.draw(window);
    }

    int clamp(int val, int minVal, int maxVal) {
        return std::max(minVal, std::min(val, maxVal));
    }
};

int main() {
    Game game;
    game.run();
    return 0;
}