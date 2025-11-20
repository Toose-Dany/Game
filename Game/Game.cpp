#include "raylib.h"
#include "rlgl.h"   
#include <vector>
#include <string>
#include <random>
#include <algorithm>

// Типы препятствий
enum class ObstacleType {
    JUMP_OVER,    // Можно перепрыгнуть
    DUCK_UNDER,   // Можно пригнуться
    WALL,         // Нельзя пройти
    LOW_BARRIER   // Низкий барьер - нельзя перепрыгнуть, можно пригнуться
};

// Типы усилений
enum class PowerUpType {
    SPEED_BOOST,      // Увеличение скорости
    INVINCIBILITY,    // Неуязвимость
    MAGNET,           // Магнит для монет
    DOUBLE_POINTS     // Двойные очки
};

// Структура для анимированной текстуры
struct AnimatedTexture {
    std::vector<Texture2D> frames;
    float frameDelay;
    float currentTime;
    int currentFrame;
    float scale;
    bool loaded;

    // Конструктор по умолчанию
    AnimatedTexture() : frameDelay(0.1f), currentTime(0.0f), currentFrame(0), scale(1.0f), loaded(false) {}
};

// Функция для загрузки анимированной текстуры из нескольких изображений
bool LoadAnimatedTexture(AnimatedTexture& animTex, const std::vector<std::string>& frameFiles, float frameDelay) {
    animTex.frames.clear();

    for (const auto& filepath : frameFiles) {
        if (FileExists(filepath.c_str())) {
            Image image = LoadImage(filepath.c_str());
            if (image.data != NULL) {
                Texture2D frameTexture = LoadTextureFromImage(image);
                animTex.frames.push_back(frameTexture);
                UnloadImage(image);
                TraceLog(LOG_INFO, "Loaded animation frame: %s", filepath.c_str());
            }
        }
        else {
            TraceLog(LOG_WARNING, "Animation frame not found: %s", filepath.c_str());
        }
    }

    if (!animTex.frames.empty()) {
        animTex.frameDelay = frameDelay;
        animTex.currentTime = 0.0f;
        animTex.currentFrame = 0;
        animTex.scale = 0.7f;
        animTex.loaded = true;
        TraceLog(LOG_INFO, "Animation loaded: %d frames", (int)animTex.frames.size());
        return true;
    }

    animTex.loaded = false;
    return false;
}

// Функция для обновления анимации
void UpdateAnimatedTexture(AnimatedTexture& animTex, float deltaTime) {
    if (!animTex.loaded || animTex.frames.empty()) return;

    animTex.currentTime += deltaTime;
    if (animTex.currentTime >= animTex.frameDelay) {
        animTex.currentTime = 0.0f;
        animTex.currentFrame = (animTex.currentFrame + 1) % animTex.frames.size();
    }
}

// Функция для получения текущего кадра
Texture2D GetCurrentFrame(const AnimatedTexture& animTex) {
    if (!animTex.loaded || animTex.frames.empty()) return { 0 };
    return animTex.frames[animTex.currentFrame];
}

// Структура для улучшений
struct Upgrade {
    std::string name;
    std::string description;
    int level;
    int maxLevel;
    int cost;
    float value;
    float increment;
};

// Структура для активных эффектов усилений
struct ActivePowerUp {
    PowerUpType type;
    float timer;
    float duration;
};

// Структура для игрока
struct Player {
    Vector3 position;
    Vector3 size;
    Color color;
    float speed;
    int lane; // 0 - левая, 1 - средняя, 2 - правая
    int targetLane; // Целевая полоса для плавного перемещения
    bool isJumping;
    bool isRolling; // ЗАМЕНА: вместо isDucking теперь isRolling
    float jumpVelocity;
    float gravity;
    int characterType;
    bool isOnObstacle; // Находится ли на препятствии
    float laneChangeSpeed; // Скорость перемещения между полосами
    float rollCooldownTimer; // Таймер кулдауна для переката
    float rollDuration; // Длительность текущего переката

    // Эффекты усилений (теперь могут комбинироваться)
    float originalSpeed;
    std::vector<ActivePowerUp> activePowerUps;
};

// Структура для препятствий
struct Obstacle {
    Vector3 position;
    Vector3 size;
    Color color;
    int lane;
    bool active;
    float speed;
    ObstacleType type;
    Texture2D texture; // Текстура для препятствия
    bool canLandOn; // Можно ли приземлиться сверху
};

// Структура для монет
struct Coin {
    Vector3 position;
    bool active;
    float speed;
};

// Структура для усилений
struct PowerUp {
    Vector3 position;
    bool active;
    float speed;
    PowerUpType type;
    float rotation; // Для анимации вращения
    Texture2D texture; // Текстура для способности
};

// Структура для локации
struct Location {
    std::string name;
    Color backgroundColor;
    Color groundColor;
    Color leftLaneColor;   // Цвет левой полосы
    Color middleLaneColor; // Цвет средней полосы
    Color rightLaneColor;  // Цвет правой полосы
    Texture2D jumpTexture;
    Texture2D duckTexture;
    Texture2D wallTexture;
    Texture2D lowBarrierTexture;
    Texture2D leftEnvironmentTexture;
    Texture2D rightEnvironmentTexture;
};

// Структура для персонажа
struct Character {
    std::string name;
    Texture2D texture;
    Color defaultColor;
    bool useAnimatedTexture;          // Флаг использования анимированной текстуры

    // Конструктор для удобной инициализации
    Character(const std::string& n, Color color)
        : name(n), texture({ 0 }), defaultColor(color), useAnimatedTexture(false) {}
};

// Структура для меню
struct Menu {
    bool isActive;
    int selectedLocation;
    int selectedCharacter;
    std::vector<Location> locations;
    std::vector<Character> characters;

    Menu() {
        isActive = true;
        selectedLocation = 0;
        selectedCharacter = 0;

        // Инициализация локаций с цветами для каждой полосы
        locations = {
            {"City", SKYBLUE, GRAY,
             DARKGRAY,    // Левая полоса - темно-серая
             GRAY,        // Средняя полоса - серая
             DARKGRAY,    // Правая полоса - темно-серая
             {0}, {0}, {0}, {0}, {0}, {0}},

            {"Forest", DARKGREEN, DARKGREEN,
             BROWN,   // Левая полоса - темно-зеленая
             { 210, 180, 140, 255 },       // Средняя полоса - зеленая
             BROWN,   // Правая полоса - темно-зеленая
             {0}, {0}, {0}, {0}, {0}, {0}},

            {"Desert", { 240, 200, 150, 255 }, { 210, 180, 140, 255 },
             { 180, 160, 120, 255 },  // Левая полоса - темно-песочная
             { 210, 180, 140, 255 },  // Средняя полоса - песочная
             { 180, 160, 120, 255 },  // Правая полоса - темно-песочная
             {0}, {0}, {0}, {0}, {0}, {0}},

            {"Winter", { 200, 220, 240, 255 }, WHITE,
             { 150, 150, 150, 255 },  // Левая полоса - серый
             WHITE,                    // Средняя полоса - белый
             { 150, 150, 150, 255 },  // Правая полоса - серый
             {0}, {0}, {0}, {0}, {0}, {0}}
        };

        // Инициализация персонажей с поддержкой анимированных текстур
        characters = {
            Character("Default", RED),
            Character("Ninja", BLACK),
            Character("Robot", BLUE),
            Character("Girl", PINK)
        };
    }
};

// Структура для магазина
struct Shop {
    bool isActive;
    int selectedUpgrade;
    std::vector<Upgrade> upgrades;
    int totalCoins;

    Shop() {
        isActive = false;
        selectedUpgrade = 0;
        totalCoins = 0;

        // Инициализация улучшений (теперь +2.5 секунды за уровень)
        upgrades = {
            {"Speed Boost", "Increase speed boost duration", 1, 5, 100, 2.5f, 2.5f},
            {"Invincibility", "Increase invincibility duration", 1, 5, 150, 2.5f, 2.5f},
            {"Coin Magnet", "Increase magnet range and duration", 1, 5, 120, 2.5f, 2.5f},
            {"Double Points", "Increase double points duration", 1, 5, 200, 2.5f, 2.5f},
            {"Coin Value", "Increase coins value", 1, 5, 250, 100.0f, 25.0f}
        };
    }
};

class Game {
private:
    const int screenWidth = 1200;
    const int screenHeight = 900;

    Player player;
    std::vector<Obstacle> obstacles;
    std::vector<Coin> coins;
    std::vector<PowerUp> powerUps;

    float obstacleSpawnTimer;
    float coinSpawnTimer;
    float powerUpSpawnTimer;
    const float obstacleSpawnInterval = 1.5f;
    const float coinSpawnInterval = 2.0f;
    const float powerUpSpawnInterval = 8.0f;

    int score;
    int coinsCollected;
    bool gameOver;

    float laneWidth;
    float lanePositions[3];

    Camera3D camera;
    float gameSpeed;

    Menu menu;
    Shop shop;
    float environmentOffset;

    // Текстуры для способностей (одинаковые на всех локациях)
    Texture2D speedBoostTexture;
    Texture2D invincibilityTexture;
    Texture2D magnetTexture;
    Texture2D doublePointsTexture;

    bool texturesLoaded;

    // Константа для дальности спавна
    const float spawnDistance = -30.0f;
    const float despawnDistance = 15.0f;

    // Анимированные текстуры для персонажей
    std::vector<AnimatedTexture> characterAnimations;

public:
    Game() {
        InitWindow(screenWidth, screenHeight, "Runner 3D with Character Animations");

        // Настройка дорожек
        laneWidth = 4.0f;
        lanePositions[0] = -laneWidth;
        lanePositions[1] = 0.0f;
        lanePositions[2] = laneWidth;

        // Инициализация игрока
        player.size = { 1.0f, 2.0f, 1.0f };
        player.position = { lanePositions[1], 1.0f, 0.0f };
        player.color = RED;
        player.speed = 5.0f;
        player.lane = 1;
        player.targetLane = 1;
        player.isJumping = false;
        player.isRolling = false;
        player.jumpVelocity = 0;
        player.gravity = 15.0f;
        player.characterType = 0;
        player.isOnObstacle = false;
        player.laneChangeSpeed = 15.0f;
        player.rollCooldownTimer = 0.0f;
        player.rollDuration = 0.0f;

        // Инициализация эффектов усилений
        player.originalSpeed = player.speed;
        player.activePowerUps.clear();

        // Инициализация 3D камеры
        camera.position = { 0.0f, 5.0f, 10.0f };
        camera.target = { player.position.x, player.position.y, player.position.z };
        camera.up = { 0.0f, 1.0f, 0.0f };
        camera.fovy = 45.0f;
        camera.projection = CAMERA_PERSPECTIVE;

        // Таймеры
        obstacleSpawnTimer = 0;
        coinSpawnTimer = 0;
        powerUpSpawnTimer = 0;

        // Счет
        score = 0;
        coinsCollected = 0;
        gameOver = false;

        gameSpeed = 5.0f;

        // Настройки по умолчанию
        texturesLoaded = false;
        environmentOffset = 0.0f;

        // Инициализация анимированных текстур
        characterAnimations.resize(menu.characters.size());

        // Загружаем текстуры
        LoadTextures();

        // Загружаем анимированные текстуры для персонажей
        LoadCharacterAnimations();

        SetTargetFPS(60);
    }

    ~Game() {
        // Выгружаем текстуры локаций
        UnloadLocationTextures();

        // Выгружаем текстуры персонажей
        UnloadCharacterTextures();

        // Выгружаем анимированные текстуры
        UnloadCharacterAnimations();

        // Выгружаем текстуры способностей
        UnloadTexture(speedBoostTexture);
        UnloadTexture(invincibilityTexture);
        UnloadTexture(magnetTexture);
        UnloadTexture(doublePointsTexture);

        CloseWindow();
    }

    void Run() {
        while (!WindowShouldClose()) {
            Update();
            Draw();
        }
    }

private:
    void LoadCharacterAnimations() {
        // Создаем списки файлов для анимаций каждого персонажа
        std::vector<std::vector<std::string>> characterFrameFiles = {
            // Default character frames
            {
                "default_frame1.png",
                "default_frame2.png",
                "default_frame3.png",
                "default_frame4.png"
            },
            // Ninja character frames
            {
                "ninja_frame1.png",
                "ninja_frame2.png",
                "ninja_frame3.png",
                "ninja_frame4.png"
            },
            // Robot character frames
            {
                "robot_frame1.png",
                "robot_frame2.png",
                "robot_frame3.png",
                "robot_frame4.png"
            },
            // Girl character frames  
            {
                "girl_frame1.png",
                "girl_frame2.png",
                "girl_frame3.png",
                "girl_frame4.png"
            }
        };

        for (int i = 0; i < (int)menu.characters.size(); i++) {
            if (LoadAnimatedTexture(characterAnimations[i], characterFrameFiles[i], 0.1f)) {
                menu.characters[i].useAnimatedTexture = true;
                TraceLog(LOG_INFO, "Animated texture loaded for character: %s", menu.characters[i].name.c_str());
            }
            else {
                menu.characters[i].useAnimatedTexture = false;
                TraceLog(LOG_WARNING, "Failed to load animated texture for character: %s", menu.characters[i].name.c_str());

                // Создаем простую анимацию из цветов как fallback
                CreateFallbackAnimation(characterAnimations[i], menu.characters[i].defaultColor);
            }
        }
    }

    void CreateFallbackAnimation(AnimatedTexture& animTex, Color baseColor) {
        animTex.frames.clear();

        // Создаем 4 кадра с пульсирующим эффектом
        for (int i = 0; i < 4; i++) {
            Image image = GenImageColor(64, 64, BLANK);

            float pulse = 0.7f + 0.3f * sin(i * PI / 2); // Пульсация между кадрами
            Color characterColor = ColorBrightness(baseColor, pulse);

            // Рисуем простого персонажа
            for (int y = 0; y < 64; y++) {
                for (int x = 0; x < 64; x++) {
                    Color color = characterColor;

                    // Тело
                    if (x > 15 && x < 49 && y > 15 && y < 49) {
                        color = ColorBrightness(characterColor, 0.8f);
                    }

                    // Глазы (анимированные - двигаются)
                    int eyeOffset = i;
                    if ((x >= 20 + eyeOffset && x <= 25 + eyeOffset && y >= 20 && y <= 25) ||
                        (x >= 35 - eyeOffset && x <= 40 - eyeOffset && y >= 20 && y <= 25)) {
                        color = BLACK;
                    }

                    // Рот (анимированный - меняет выражение)
                    int mouthY = 35 + (i % 2);
                    if (x >= 25 && x <= 39 && y >= mouthY && y <= mouthY + 2) {
                        color = BLACK;
                    }

                    ImageDrawPixel(&image, x, y, color);
                }
            }

            Texture2D frame = LoadTextureFromImage(image);
            animTex.frames.push_back(frame);
            UnloadImage(image);
        }

        animTex.frameDelay = 0.15f;
        animTex.currentTime = 0.0f;
        animTex.currentFrame = 0;
        animTex.scale = 0.7f;
        animTex.loaded = true;
    }

    void UnloadCharacterAnimations() {
        for (auto& animTex : characterAnimations) {
            for (auto& frame : animTex.frames) {
                if (IsTextureReady(frame)) {
                    UnloadTexture(frame);
                }
            }
            animTex.frames.clear();
            animTex.loaded = false;
        }
        characterAnimations.clear();
    }

    void UnloadLocationTextures() {
        for (auto& location : menu.locations) {
            if (IsTextureReady(location.jumpTexture)) UnloadTexture(location.jumpTexture);
            if (IsTextureReady(location.duckTexture)) UnloadTexture(location.duckTexture);
            if (IsTextureReady(location.wallTexture)) UnloadTexture(location.wallTexture);
            if (IsTextureReady(location.lowBarrierTexture)) UnloadTexture(location.lowBarrierTexture);
            if (IsTextureReady(location.leftEnvironmentTexture)) UnloadTexture(location.leftEnvironmentTexture);
            if (IsTextureReady(location.rightEnvironmentTexture)) UnloadTexture(location.rightEnvironmentTexture);
        }
    }

    void UnloadCharacterTextures() {
        for (auto& character : menu.characters) {
            if (IsTextureReady(character.texture)) UnloadTexture(character.texture);
        }
    }

    bool IsTextureReady(Texture2D texture) const {
        return texture.id != 0 && texture.width > 0 && texture.height > 0;
    }

    void LoadTextures() {
        // ДОБАВЛЕНО: выгружаем старые текстуры если они уже загружены
        if (texturesLoaded) {
            UnloadLocationTextures();
            UnloadCharacterTextures();
            UnloadTexture(speedBoostTexture);
            UnloadTexture(invincibilityTexture);
            UnloadTexture(magnetTexture);
            UnloadTexture(doublePointsTexture);
        }

        // Загружаем текстуры способностей (одинаковые для всех локаций)
        LoadPowerUpTextures();

        // ПОПЫТКА ЗАГРУЗИТЬ ТЕКСТУРЫ, НО НЕ БЛОКИРУЕМ ЗАПУСК
        try {
            LoadLocationTextures();
            LoadCharacterTextures();
        }
        catch (...) {
            TraceLog(LOG_WARNING, "Some textures failed to load, using defaults");
        }

        texturesLoaded = AreTexturesLoaded();

        TraceLog(LOG_INFO, "All textures loaded: %s", texturesLoaded ? "YES" : "NO");
    }

    // Функция для загрузки текстур способностей
    void LoadPowerUpTextures() {
        // Пытаемся загрузить пользовательские текстуры, если они существуют
        LoadPowerUpTexture("speed_boost.png", speedBoostTexture, CreateSpeedBoostTexture());
        LoadPowerUpTexture("invincibility.png", invincibilityTexture, CreateInvincibilityTexture());
        LoadPowerUpTexture("magnet.png", magnetTexture, CreateMagnetTexture());
        LoadPowerUpTexture("double_points.png", doublePointsTexture, CreateDoublePointsTexture());
    }

    // Функция для загрузки текстуры способности с fallback
    void LoadPowerUpTexture(const char* filepath, Texture2D& texture, Texture2D defaultTexture) {
        if (FileExists(filepath)) {
            Image image = LoadImage(filepath);
            if (image.data != NULL) {
                texture = LoadTextureFromImage(image);
                UnloadImage(image);
                TraceLog(LOG_INFO, "Successfully loaded power-up texture: %s", filepath);
                return;
            }
        }

        // Если файл не найден или не загружен, используем дефолтную текстуру
        texture = defaultTexture;
        TraceLog(LOG_WARNING, "Power-up texture not found: %s, using default", filepath);
    }

    void LoadLocationTextures() {
        // Загружаем текстуры для каждой локации
        // City
        LoadObstacleTexture("city_jump.png", menu.locations[0].jumpTexture);
        LoadObstacleTexture("city_duck.png", menu.locations[0].duckTexture);
        LoadObstacleTexture("city_wall.png", menu.locations[0].wallTexture);
        LoadObstacleTexture("city_barrier.png", menu.locations[0].lowBarrierTexture);
        LoadEnvironmentTexture("city_left.png", menu.locations[0].leftEnvironmentTexture);
        LoadEnvironmentTexture("city_right.png", menu.locations[0].rightEnvironmentTexture);

        // Forest
        LoadObstacleTexture("forest_jump.png", menu.locations[1].jumpTexture);
        LoadObstacleTexture("forest_duck.png", menu.locations[1].duckTexture);
        LoadObstacleTexture("forest_wall.png", menu.locations[1].wallTexture);
        LoadObstacleTexture("forest_barrier.png", menu.locations[1].lowBarrierTexture);
        LoadEnvironmentTexture("forest_left.png", menu.locations[1].leftEnvironmentTexture);
        LoadEnvironmentTexture("forest_right.png", menu.locations[1].rightEnvironmentTexture);

        // Desert
        LoadObstacleTexture("desert_jump.png", menu.locations[2].jumpTexture);
        LoadObstacleTexture("desert_duck.png", menu.locations[2].duckTexture);
        LoadObstacleTexture("desert_wall.png", menu.locations[2].wallTexture);
        LoadObstacleTexture("desert_barrier.png", menu.locations[2].lowBarrierTexture);
        LoadEnvironmentTexture("desert_left.png", menu.locations[2].leftEnvironmentTexture);
        LoadEnvironmentTexture("desert_right.png", menu.locations[2].rightEnvironmentTexture);

        // Winter
        LoadObstacleTexture("winter_jump.png", menu.locations[3].jumpTexture);
        LoadObstacleTexture("winter_duck.png", menu.locations[3].duckTexture);
        LoadObstacleTexture("winter_wall.png", menu.locations[3].wallTexture);
        LoadObstacleTexture("winter_barrier.png", menu.locations[3].lowBarrierTexture);
        LoadEnvironmentTexture("winter_left.png", menu.locations[3].leftEnvironmentTexture);
        LoadEnvironmentTexture("winter_right.png", menu.locations[3].rightEnvironmentTexture);
    }

    void LoadCharacterTextures() {
        // Загружаем текстуры для каждого персонажа (как fallback)
        LoadCharacterTexture("default_character.png", menu.characters[0].texture);
        LoadCharacterTexture("ninja_character.png", menu.characters[1].texture);
        LoadCharacterTexture("robot_character.png", menu.characters[2].texture);
        LoadCharacterTexture("girl_character.png", menu.characters[3].texture);
    }

    void LoadEnvironmentTexture(const char* filepath, Texture2D& texture) {
        if (FileExists(filepath)) {
            Image image = LoadImage(filepath);
            if (image.data != NULL) {
                texture = LoadTextureFromImage(image);
                UnloadImage(image);
                TraceLog(LOG_INFO, "Successfully loaded environment texture: %s", filepath);
            }
            else {
                TraceLog(LOG_ERROR, "Failed to load environment image: %s", filepath);
                texture = CreateDefaultEnvironmentTexture();
            }
        }
        else {
            TraceLog(LOG_WARNING, "Environment texture not found: %s, using default", filepath);
            texture = CreateDefaultEnvironmentTexture();
        }
    }

    Texture2D CreateDefaultEnvironmentTexture() {
        Image image = GenImageColor(128, 256, BLANK);
        Color baseColor = GRAY;

        for (int y = 0; y < 256; y++) {
            for (int x = 0; x < 128; x++) {
                Color color = baseColor;

                // Создаем простой узор для текстуры по умолчанию
                if ((x / 16 + y / 16) % 2 == 0) {
                    color = ColorBrightness(baseColor, 0.8f);
                }

                // Добавляем детали в зависимости от высоты (имитация окон/узоров)
                if (x % 16 == 0 || y % 16 == 0) {
                    color = ColorBrightness(baseColor, 0.6f);
                }

                ImageDrawPixel(&image, x, y, color);
            }
        }

        Texture2D texture = LoadTextureFromImage(image);
        UnloadImage(image);
        return texture;
    }

    void LoadObstacleTexture(const char* filepath, Texture2D& texture) {
        if (FileExists(filepath)) {
            Image image = LoadImage(filepath);
            if (image.data != NULL) {
                texture = LoadTextureFromImage(image);
                UnloadImage(image);
                TraceLog(LOG_INFO, "Successfully loaded obstacle texture: %s", filepath);
            }
            else {
                TraceLog(LOG_ERROR, "Failed to load image: %s", filepath);
                texture = CreateDefaultObstacleTexture();
            }
        }
        else {
            TraceLog(LOG_WARNING, "Obstacle texture not found: %s, using default", filepath);
            texture = CreateDefaultObstacleTexture();
        }
    }

    Texture2D CreateDefaultObstacleTexture() {
        // Создаем простую текстуру-заглушку
        Image image = GenImageColor(64, 64, GRAY);
        for (int y = 0; y < 64; y++) {
            for (int x = 0; x < 64; x++) {
                if (x == 0 || x == 63 || y == 0 || y == 63) {
                    ImageDrawPixel(&image, x, y, BLACK);
                }
            }
        }
        Texture2D texture = LoadTextureFromImage(image);
        UnloadImage(image);
        return texture;
    }

    void LoadCharacterTexture(const char* filepath, Texture2D& texture) {
        if (FileExists(filepath)) {
            Image image = LoadImage(filepath);
            if (image.data != NULL) {
                texture = LoadTextureFromImage(image);
                UnloadImage(image);
                TraceLog(LOG_INFO, "Successfully loaded character texture: %s", filepath);
            }
            else {
                TraceLog(LOG_ERROR, "Failed to load image: %s", filepath);
                texture = CreateDefaultCharacterTexture();
            }
        }
        else {
            TraceLog(LOG_WARNING, "Character texture not found: %s", filepath);
            texture = CreateDefaultCharacterTexture();
        }
    }

    Texture2D CreateDefaultCharacterTexture() {
        // Создаем простую текстуру-заглушку с цветом в зависимости от персонажа
        Image image = GenImageColor(64, 64, BLANK);
        Color baseColor = RED; // По умолчанию красный

        for (int y = 0; y < 64; y++) {
            for (int x = 0; x < 64; x++) {
                Color color = baseColor;

                // Простой рисунок "лица"
                if ((x > 20 && x < 44 && y > 20 && y < 44)) {
                    color = ColorBrightness(baseColor, 0.7f);
                }

                // Глазы
                if ((x >= 25 && x <= 30 && y >= 25 && y <= 30) ||
                    (x >= 34 && x <= 39 && y >= 25 && y <= 30)) {
                    color = BLACK;
                }

                // Рот
                if (x >= 25 && x <= 39 && y >= 35 && y <= 38) {
                    color = BLACK;
                }

                // Рамка
                if (x == 0 || x == 63 || y == 0 || y == 63) {
                    color = BLACK;
                }

                ImageDrawPixel(&image, x, y, color);
            }
        }
        Texture2D texture = LoadTextureFromImage(image);
        UnloadImage(image);
        return texture;
    }

    bool AreTexturesLoaded() const {
        // ПРОСТАЯ ПРОВЕРКА ОСНОВНЫХ ТЕКСТУР
        bool basicTexturesLoaded =
            IsTextureReady(speedBoostTexture) &&
            IsTextureReady(invincibilityTexture) &&
            IsTextureReady(magnetTexture) &&
            IsTextureReady(doublePointsTexture);

        return basicTexturesLoaded;
    }

    Texture2D GetObstacleTexture(ObstacleType type) {
        // Возвращаем текстуру препятствия в зависимости от текущей локации и типа
        const Location& currentLocation = menu.locations[menu.selectedLocation];

        switch (type) {
        case ObstacleType::JUMP_OVER:
            return IsTextureReady(currentLocation.jumpTexture) ? currentLocation.jumpTexture : CreateDefaultObstacleTexture();
        case ObstacleType::DUCK_UNDER:
            return IsTextureReady(currentLocation.duckTexture) ? currentLocation.duckTexture : CreateDefaultObstacleTexture();
        case ObstacleType::WALL:
            return IsTextureReady(currentLocation.wallTexture) ? currentLocation.wallTexture : CreateDefaultObstacleTexture();
        case ObstacleType::LOW_BARRIER:
            return IsTextureReady(currentLocation.lowBarrierTexture) ? currentLocation.lowBarrierTexture : CreateDefaultObstacleTexture();
        default:
            return CreateDefaultObstacleTexture();
        }
    }

    // Функция для получения текстуры способности (одинаковая на всех локациях)
    Texture2D GetPowerUpTexture(PowerUpType type) {
        switch (type) {
        case PowerUpType::SPEED_BOOST:
            return IsTextureReady(speedBoostTexture) ? speedBoostTexture : CreateSpeedBoostTexture();
        case PowerUpType::INVINCIBILITY:
            return IsTextureReady(invincibilityTexture) ? invincibilityTexture : CreateInvincibilityTexture();
        case PowerUpType::MAGNET:
            return IsTextureReady(magnetTexture) ? magnetTexture : CreateMagnetTexture();
        case PowerUpType::DOUBLE_POINTS:
            return IsTextureReady(doublePointsTexture) ? doublePointsTexture : CreateDoublePointsTexture();
        default:
            return CreateSpeedBoostTexture();
        }
    }

    Texture2D GetCharacterTexture() {
        // Возвращаем текстуру для выбранного персонажа
        Texture2D charTexture = menu.characters[player.characterType].texture;
        return IsTextureReady(charTexture) ? charTexture : CreateDefaultCharacterTexture();
    }

    Color GetCurrentBackgroundColor() {
        return menu.locations[menu.selectedLocation].backgroundColor;
    }

    Color GetCurrentGroundColor() {
        return menu.locations[menu.selectedLocation].groundColor;
    }

    // НОВЫЕ ФУНКЦИИ: получение цветов для каждой полосы
    Color GetLeftLaneColor() {
        return menu.locations[menu.selectedLocation].leftLaneColor;
    }

    Color GetMiddleLaneColor() {
        return menu.locations[menu.selectedLocation].middleLaneColor;
    }

    Color GetRightLaneColor() {
        return menu.locations[menu.selectedLocation].rightLaneColor;
    }

    // Функции создания текстур для усилений (3D объекты)
    Texture2D CreateSpeedBoostTexture() {
        Image image = GenImageColor(64, 64, BLANK);
        for (int y = 0; y < 64; y++) {
            for (int x = 0; x < 64; x++) {
                Color color = ORANGE;
                // Молния
                if ((x >= 20 && x <= 44 && y == 32) ||
                    (x >= 25 && x <= 39 && abs(y - 20) <= 5) ||
                    (x >= 30 && x <= 34 && abs(y - 44) <= 5)) {
                    color = YELLOW;
                }
                // Контуры
                if (x == 0 || x == 63 || y == 0 || y == 63) {
                    color = BLACK;
                }
                ImageDrawPixel(&image, x, y, color);
            }
        }
        Texture2D texture = LoadTextureFromImage(image);
        UnloadImage(image);
        return texture;
    }

    Texture2D CreateInvincibilityTexture() {
        Image image = GenImageColor(64, 64, BLANK);
        for (int y = 0; y < 64; y++) {
            for (int x = 0; x < 64; x++) {
                Color color = GOLD;
                // Звезда
                float centerX = 32.0f;
                float centerY = 32.0f;
                float dx = x - centerX;
                float dy = y - centerY;
                float distance = sqrt(dx * dx + dy * dy);
                float angle = atan2(dy, dx);

                // Форма звезды
                float starRadius = 25.0f * (0.5f + 0.5f * cos(5 * angle));
                if (distance < starRadius) {
                    color = YELLOW;
                }
                // Контуры
                if (x == 0 || x == 63 || y == 0 || y == 63) {
                    color = BLACK;
                }
                ImageDrawPixel(&image, x, y, color);
            }
        }
        Texture2D texture = LoadTextureFromImage(image);
        UnloadImage(image);
        return texture;
    }

    Texture2D CreateMagnetTexture() {
        Image image = GenImageColor(64, 64, BLANK);
        for (int y = 0; y < 64; y++) {
            for (int x = 0; x < 64; x++) {
                Color color = RED;
                // Форма магнита
                if ((x >= 15 && x <= 49 && y >= 20 && y <= 44) &&
                    !(x >= 25 && x <= 39 && y >= 25 && y <= 39)) {
                    color = BLUE;
                }
                // Контуры
                if (x == 0 || x == 63 || y == 0 || y == 63) {
                    color = BLACK;
                }
                ImageDrawPixel(&image, x, y, color);
            }
        }
        Texture2D texture = LoadTextureFromImage(image);
        UnloadImage(image);
        return texture;
    }

    Texture2D CreateDoublePointsTexture() {
        Image image = GenImageColor(64, 64, BLANK);
        for (int y = 0; y < 64; y++) {
            for (int x = 0; x < 64; x++) {
                Color color = GREEN;
                // Цифра 2
                if ((x >= 20 && x <= 44 && y == 20) || // Верхняя горизонтальная
                    (x >= 20 && x <= 44 && y == 32) || // Средняя горизонтальная
                    (x >= 20 && x <= 44 && y == 44) || // Нижняя горизонтальная
                    (x >= 40 && x <= 44 && y >= 20 && y <= 32) || // Правая верхняя вертикальная
                    (x >= 20 && x <= 24 && y >= 32 && y <= 44)) { // Левая нижняя вертикальная
                    color = LIME;
                }
                // Контуры
                if (x == 0 || x == 63 || y == 0 || y == 63) {
                    color = BLACK;
                }
                ImageDrawPixel(&image, x, y, color);
            }
        }
        Texture2D texture = LoadTextureFromImage(image);
        UnloadImage(image);
        return texture;
    }

    void DrawCubeTexture(Vector3 position, Vector3 size, Texture2D texture, Color color)
    {
        float x = position.x;
        float y = position.y;
        float z = position.z;
        float width = size.x;
        float height = size.y;
        float length = size.z;

        // ИСПРАВЛЕНИЕ: ПРОВЕРКА ВАЛИДНОСТИ ТЕКСТУРЫ
        if (!IsTextureReady(texture)) {
            // Если текстура не загружена, рисуем простой куб
            DrawCube(position, width, height, length, color);
            DrawCubeWires(position, width, height, length, BLACK);
            return;
        }

        rlSetTexture(texture.id);
        rlBegin(RL_QUADS);

        // ИСПРАВЛЕНИЕ: изменен порядок текстурных координат для правильной ориентации
        rlColor4ub(color.r, color.g, color.b, color.a);

        // ПЕРЕДНЯЯ ГРАНЬ - исправленные координаты
        rlNormal3f(0.0f, 0.0f, 1.0f);
        rlTexCoord2f(0.0f, 1.0f); rlVertex3f(x - width / 2, y - height / 2, z + length / 2);
        rlTexCoord2f(1.0f, 1.0f); rlVertex3f(x + width / 2, y - height / 2, z + length / 2);
        rlTexCoord2f(1.0f, 0.0f); rlVertex3f(x + width / 2, y + height / 2, z + length / 2);
        rlTexCoord2f(0.0f, 0.0f); rlVertex3f(x - width / 2, y + height / 2, z + length / 2);

        rlEnd();
        rlSetTexture(0);
    }

    void DrawObstacle(const Obstacle& obstacle) {
        if (obstacle.active) {
            // ИСПРАВЛЕНИЕ: улучшенная проверка текстур
            if (texturesLoaded && IsTextureReady(obstacle.texture)) {
                DrawCubeTexture(obstacle.position, obstacle.size, obstacle.texture, RAYWHITE);
            }
            else {
                // Fallback - рисуем простой цветной куб если текстура не загружена
                DrawCube(obstacle.position, obstacle.size.x, obstacle.size.y, obstacle.size.z, obstacle.color);
                DrawCubeWires(obstacle.position, obstacle.size.x, obstacle.size.y, obstacle.size.z, BLACK);
            }
        }
    }

    // Функция для отрисовки способности с текстурой
    void DrawPowerUp(const PowerUp& powerUp) {
        if (powerUp.active) {
            // ИСПРАВЛЕНИЕ: улучшенная проверка текстур
            if (texturesLoaded && IsTextureReady(powerUp.texture)) {
                // Добавляем анимацию вращения и пульсации
                float scale = 1.0f + 0.2f * sin(GetTime() * 5.0f);
                Vector3 scaledSize = { scale, scale, scale };

                DrawCubeTexture(powerUp.position, scaledSize, powerUp.texture, RAYWHITE);
            }
            else {
                // Fallback - рисуем простую сферу если текстура не загружена
                Color powerUpColor;
                switch (powerUp.type) {
                case PowerUpType::SPEED_BOOST: powerUpColor = ORANGE; break;
                case PowerUpType::INVINCIBILITY: powerUpColor = GOLD; break;
                case PowerUpType::MAGNET: powerUpColor = BLUE; break;
                case PowerUpType::DOUBLE_POINTS: powerUpColor = GREEN; break;
                default: powerUpColor = WHITE;
                }
                DrawSphere(powerUp.position, 0.7f, powerUpColor);
            }
        }
    }

    // НОВОЕ: Функция для отрисовки окружения с учетом локации
    void DrawEnvironment() {
        const Location& currentLocation = menu.locations[menu.selectedLocation];

        switch (menu.selectedLocation) {
        case 0: // City - здания (близко к дороге)
        {
            Vector3 envSize = { 3.0f, 8.0f, 3.0f };
            for (int i = -5; i <= 5; i++) {
                // Левая сторона с текстурой
                if (IsTextureReady(currentLocation.leftEnvironmentTexture)) {
                    DrawCubeTexture({ -8.0f, 4.0f, i * 10.0f + environmentOffset }, envSize, currentLocation.leftEnvironmentTexture, RAYWHITE);
                }
                else {
                    DrawCube({ -8.0f, 4.0f, i * 10.0f + environmentOffset }, envSize.x, envSize.y, envSize.z, GRAY);
                }
                // Правая сторона с текстурой
                if (IsTextureReady(currentLocation.rightEnvironmentTexture)) {
                    DrawCubeTexture({ 8.0f, 4.0f, i * 10.0f + environmentOffset }, envSize, currentLocation.rightEnvironmentTexture, RAYWHITE);
                }
                else {
                    DrawCube({ 8.0f, 4.0f, i * 10.0f + environmentOffset }, envSize.x, envSize.y, envSize.z, GRAY);
                }
            }
        }
        break;

        case 1: // Forest - деревья (дальше от дороги)
        {
            Vector3 envSize = { 2.0f, 6.0f, 2.0f };
            for (int i = -5; i <= 5; i++) {
                // Левая сторона с текстурой - дальше от дороги
                if (IsTextureReady(currentLocation.leftEnvironmentTexture)) {
                    DrawCubeTexture({ -8.0f, 3.0f, i * 8.0f + environmentOffset }, envSize, currentLocation.leftEnvironmentTexture, RAYWHITE);
                }
                else {
                    DrawCube({ -8.0f, 3.0f, i * 8.0f + environmentOffset }, envSize.x, envSize.y, envSize.z, GREEN);
                }
                // Правая сторона с текстурой - дальше от дороги
                if (IsTextureReady(currentLocation.rightEnvironmentTexture)) {
                    DrawCubeTexture({ 8.0f, 3.0f, i * 8.0f + environmentOffset }, envSize, currentLocation.rightEnvironmentTexture, RAYWHITE);
                }
                else {
                    DrawCube({ 8.0f, 3.0f, i * 8.0f + environmentOffset }, envSize.x, envSize.y, envSize.z, GREEN);
                }
            }
        }
        break;

        case 2: // Desert - камни (еще дальше от дороги)
        {
            Vector3 envSize = { 4.0f, 4.0f, 4.0f };
            for (int i = -5; i <= 5; i++) {
                // Левая сторона с текстурой - еще дальше от дороги
                if (IsTextureReady(currentLocation.leftEnvironmentTexture)) {
                    DrawCubeTexture({ -8.0f, 2.0f, i * 12.0f + environmentOffset }, envSize, currentLocation.leftEnvironmentTexture, RAYWHITE);
                }
                else {
                    DrawCube({ -8.0f, 2.0f, i * 12.0f + environmentOffset }, envSize.x, envSize.y, envSize.z, BROWN);
                }
                // Правая сторона с текстурой - еще дальше от дороги
                if (IsTextureReady(currentLocation.rightEnvironmentTexture)) {
                    DrawCubeTexture({ 8.0f, 2.0f, i * 12.0f + environmentOffset }, envSize, currentLocation.rightEnvironmentTexture, RAYWHITE);
                }
                else {
                    DrawCube({ 8.0f, 2.0f, i * 12.0f + environmentOffset }, envSize.x, envSize.y, envSize.z, BROWN);
                }
            }
        }
        break;

        case 3: // Winter - домики (самые далекие от дороги)
        {
            Vector3 envSize = { 4.0f, 5.0f, 4.0f };
            for (int i = -5; i <= 5; i++) {
                // Левая сторона с текстурой - самые далекие от дороги
                if (IsTextureReady(currentLocation.leftEnvironmentTexture)) {
                    DrawCubeTexture({ -8.0f, 2.5f, i * 15.0f + environmentOffset }, envSize, currentLocation.leftEnvironmentTexture, RAYWHITE);
                }
                else {
                    DrawCube({ -8.0f, 2.5f, i * 15.0f + environmentOffset }, envSize.x, envSize.y, envSize.z, WHITE);
                }
                // Правая сторона с текстурой - самые далекие от дороги
                if (IsTextureReady(currentLocation.rightEnvironmentTexture)) {
                    DrawCubeTexture({ 8.0f, 2.5f, i * 15.0f + environmentOffset }, envSize, currentLocation.rightEnvironmentTexture, RAYWHITE);
                }
                else {
                    DrawCube({ 8.0f, 2.5f, i * 15.0f + environmentOffset }, envSize.x, envSize.y, envSize.z, WHITE);
                }
            }
        }
        break;
        }
    }

    void Update() {
        if (menu.isActive) {
            UpdateMenu();
            return;
        }

        if (shop.isActive) {
            UpdateShop();
            return;
        }

        if (gameOver) {
            if (IsKeyPressed(KEY_R)) {
                ResetGame();
            }
            if (IsKeyPressed(KEY_M)) {
                menu.isActive = true;
            }
            if (IsKeyPressed(KEY_S)) {
                // Переход в магазин после игры
                shop.totalCoins += coinsCollected;
                shop.isActive = true;
            }
            return;
        }

        // ОБНОВЛЯЕМ АНИМАЦИИ ПЕРСОНАЖЕЙ
        float deltaTime = GetFrameTime();
        for (auto& animTex : characterAnimations) {
            UpdateAnimatedTexture(animTex, deltaTime);
        }

        HandleInput();
        UpdatePlayer();
        UpdateObstacles();
        UpdateCoins();
        UpdatePowerUps();
        UpdateCamera();
        CheckCollisions();
        UpdatePowerUpEffects();

        environmentOffset += gameSpeed * 0.3f * GetFrameTime();
        if (environmentOffset > 50.0f) environmentOffset = 0.0f;

        score += HasPowerUp(PowerUpType::DOUBLE_POINTS) ? 2 : 1;
    }

    void UpdateMenu() {
        int oldLocation = menu.selectedLocation;

        // Обработка входа в магазин из меню
        if (IsKeyPressed(KEY_S)) {
            shop.isActive = true;
            menu.isActive = false;
            return;
        }

        if (IsKeyPressed(KEY_UP)) {
            if (menu.selectedLocation > 0) menu.selectedLocation--;
        }
        if (IsKeyPressed(KEY_DOWN)) {
            if (menu.selectedLocation < (int)menu.locations.size() - 1) menu.selectedLocation++;
        }

        if (IsKeyPressed(KEY_A)) {
            if (menu.selectedCharacter > 0) menu.selectedCharacter--;
        }
        if (IsKeyPressed(KEY_D)) {
            if (menu.selectedCharacter < (int)menu.characters.size() - 1) menu.selectedCharacter++;
        }

        if (IsKeyPressed(KEY_ENTER)) {
            player.characterType = menu.selectedCharacter;
            menu.isActive = false;
        }

        if (oldLocation != menu.selectedLocation) {
            // При смене локации автоматически применяем настройки
        }
    }

    void UpdateShop() {
        // Навигация по магазину
        if (IsKeyPressed(KEY_UP)) {
            if (shop.selectedUpgrade > 0) shop.selectedUpgrade--;
        }
        if (IsKeyPressed(KEY_DOWN)) {
            if (shop.selectedUpgrade < (int)shop.upgrades.size() - 1) shop.selectedUpgrade++;
        }

        // Покупка улучшения
        if (IsKeyPressed(KEY_ENTER)) {
            BuyUpgrade(shop.selectedUpgrade);
        }

        // Выход из магазина - возврат в меню
        if (IsKeyPressed(KEY_ESCAPE) || IsKeyPressed(KEY_M) || IsKeyPressed(KEY_S)) {
            shop.isActive = false;
            menu.isActive = true;
        }
    }

    void BuyUpgrade(int index) {
        Upgrade& upgrade = shop.upgrades[index];

        if (upgrade.level < upgrade.maxLevel && shop.totalCoins >= upgrade.cost) {
            shop.totalCoins -= upgrade.cost;
            upgrade.level++;
            upgrade.value += upgrade.increment;

            // Увеличиваем стоимость для следующего уровня
            upgrade.cost = static_cast<int>(upgrade.cost * 1.5f);
        }
    }

    void HandleInput() {
        // Движение влево-вправо с плавным перемещением
        if (IsKeyPressed(KEY_LEFT) && player.targetLane > 0) {
            player.targetLane--;
        }
        if (IsKeyPressed(KEY_RIGHT) && player.targetLane < 2) {
            player.targetLane++;
        }

        // Прыжок
        if ((IsKeyPressed(KEY_SPACE) || IsKeyPressed(KEY_UP)) && !player.isJumping && !player.isRolling) {
            player.isJumping = true;
            player.jumpVelocity = 8.0f;
            player.isOnObstacle = false; // Сбрасываем статус нахождения на препятствии при прыжке
        }

        // ПЕРЕКАТ вместо приседания - теперь это мгновенное действие с кулдауном
        if (IsKeyPressed(KEY_DOWN) && !player.isJumping && !player.isRolling && player.rollCooldownTimer <= 0) {
            player.isRolling = true;
            player.rollDuration = 0.0f; // Сбрасываем длительность переката
            player.size.y = 1.0f; // Уменьшаем высоту для переката
            player.position.y = 0.5f;
            player.rollCooldownTimer = 1.5f; // КУЛДАУН 1.5 СЕКУНДЫ (было 1.0f)
        }
    }

    void UpdatePlayer() {
        // Обновляем таймер кулдауна переката
        if (player.rollCooldownTimer > 0) {
            player.rollCooldownTimer -= GetFrameTime();
        }

        // Обновление переката
        if (player.isRolling) {
            player.rollDuration += GetFrameTime();

            if (player.rollDuration >= 1.0f) { // ПЕРЕКАТ ДЛИТСЯ 1 СЕКУНДУ (было 0.5f)
                player.isRolling = false;
                player.size.y = 2.0f; // Возвращаем нормальную высоту
                if (!player.isJumping && !player.isOnObstacle) {
                    player.position.y = 1.0f;
                }
            }
        }

        // Плавное перемещение между полосами с УВЕЛИЧЕННОЙ СКОРОСТЬЮ
        float targetX = lanePositions[player.targetLane];
        if (fabs(player.position.x - targetX) > 0.01f) {
            float direction = (targetX > player.position.x) ? 1.0f : -1.0f;
            player.position.x += direction * player.laneChangeSpeed * GetFrameTime();

            // Ограничиваем позицию, чтобы не перескакивать целевую позицию
            if ((direction > 0 && player.position.x > targetX) ||
                (direction < 0 && player.position.x < targetX)) {
                player.position.x = targetX;
                player.lane = player.targetLane; // Обновляем текущую полосу когда достигаем цели
            }
        }
        else {
            player.lane = player.targetLane; // Обновляем текущую полосу
        }

        // Обновление прыжка
        if (player.isJumping) {
            player.position.y += player.jumpVelocity * GetFrameTime();
            player.jumpVelocity -= player.gravity * GetFrameTime();

            // Проверяем приземление на препятствие или землю
            if (player.jumpVelocity < 0) { // Падаем вниз
                float groundHeight = 1.0f;
                float obstacleHeight = CheckObstacleLanding();

                if (obstacleHeight > groundHeight) {
                    // Приземляемся на препятствие
                    if (player.position.y <= obstacleHeight) {
                        player.position.y = obstacleHeight;
                        player.isJumping = false;
                        player.jumpVelocity = 0;
                        player.isOnObstacle = true;
                    }
                }
                else {
                    // Приземляемся на землю
                    if (player.position.y <= groundHeight) {
                        player.position.y = groundHeight;
                        player.isJumping = false;
                        player.jumpVelocity = 0;
                        player.isOnObstacle = false;
                    }
                }
            }
        }
    }

    float CheckObstacleLanding() {
        float highestObstacle = 1.0f; // Высота земли по умолчанию

        for (auto& obstacle : obstacles) {
            if (obstacle.active && obstacle.lane == player.lane && obstacle.canLandOn) {
                // Проверяем только переднюю грань для приземления
                BoundingBox playerBox = GetPlayerFrontFaceBox();
                BoundingBox obstacleBox = GetObstacleFrontFaceBox(obstacle);

                if (CheckCollisionBoxes(playerBox, obstacleBox)) {
                    float obstacleTop = obstacle.position.y + obstacle.size.y / 2;
                    if (obstacleTop > highestObstacle) {
                        highestObstacle = obstacleTop;
                    }
                }
            }
        }

        return highestObstacle;
    }

    BoundingBox GetPlayerFrontFaceBox() {
        // Создаем bounding box только для передней грани игрока
        float frontOffset = player.size.z / 2;
        return {
            { player.position.x - player.size.x / 2, player.position.y - player.size.y / 2, player.position.z + frontOffset - 0.1f },
            { player.position.x + player.size.x / 2, player.position.y + player.size.y / 2, player.position.z + frontOffset + 0.1f }
        };
    }

    BoundingBox GetObstacleFrontFaceBox(const Obstacle& obstacle) {
        // Создаем bounding box только для передней грани препятствия
        float frontOffset = obstacle.size.z / 2;
        return {
            { obstacle.position.x - obstacle.size.x / 2, obstacle.position.y - obstacle.size.y / 2, obstacle.position.z + frontOffset - 0.1f },
            { obstacle.position.x + obstacle.size.x / 2, obstacle.position.y + obstacle.size.y / 2, obstacle.position.z + frontOffset + 0.1f }
        };
    }

    void UpdateObstacles() {
        // Спавн препятствий
        obstacleSpawnTimer += GetFrameTime();
        if (obstacleSpawnTimer >= obstacleSpawnInterval) {
            if (GetRandomValue(0, 100) < 40) {
                SpawnObstacleGroup();
            }
            else {
                SpawnSingleObstacle();
            }
            obstacleSpawnTimer = 0;
        }

        // Обновление позиций препятствий
        for (auto& obstacle : obstacles) {
            if (obstacle.active) {
                obstacle.position.z += obstacle.speed * GetFrameTime();

                // ИСПРАВЛЕНИЕ: используем новую дальность деактивации
                if (obstacle.position.z > despawnDistance) {
                    obstacle.active = false;
                }
            }
        }

        obstacles.erase(std::remove_if(obstacles.begin(), obstacles.end(),
            [](const Obstacle& o) { return !o.active; }), obstacles.end());
    }

    void SpawnSingleObstacle() {
        Obstacle obstacle;
        obstacle.lane = GetRandomValue(0, 2);

        int obstacleType = GetRandomValue(0, 3);
        switch (obstacleType) {
        case 0:
            obstacle.type = ObstacleType::JUMP_OVER;
            obstacle.size = { 1.0f, 1.0f, 1.0f }; // Можно перепрыгнуть
            obstacle.color = DARKGRAY;
            obstacle.canLandOn = true; // Можно приземлиться сверху
            break;
        case 1:
            obstacle.type = ObstacleType::DUCK_UNDER;
            obstacle.size = { 1.0f, 1.0f, 1.0f }; // ИЗМЕНЕНО: такая же высота как JUMP_OVER (было 1.5f)
            obstacle.color = BROWN;
            obstacle.canLandOn = false; // Нельзя приземлиться сверху
            break;
        case 2:
            obstacle.type = ObstacleType::WALL;
            obstacle.size = { 1.0f, 3.0f, 1.0f };
            obstacle.color = MAROON;
            obstacle.canLandOn = false; // Нельзя приземлиться сверху
            break;
        case 3:
            obstacle.type = ObstacleType::LOW_BARRIER;
            obstacle.size = { 1.0f, 2.5f, 1.0f }; // Выше - нельзя перепрыгнуть, можно пригнуться
            obstacle.color = { 150, 75, 0, 255 };
            obstacle.canLandOn = false; // Нельзя приземлиться сверху
            break;
        }

        // Назначаем текстуру в зависимости от локации и типа препятствия
        obstacle.texture = GetObstacleTexture(obstacle.type);

        // ИСПРАВЛЕНИЕ: используем новую дальность спавна
        obstacle.position = { lanePositions[obstacle.lane], obstacle.size.y / 2, spawnDistance };
        obstacle.active = true;
        obstacle.speed = gameSpeed + (static_cast<float>(score) / 1000.0f);

        obstacles.push_back(obstacle);
    }

    void SpawnObstacleGroup() {
        bool hasPassableLane = false;
        std::vector<ObstacleType> laneTypes(3);

        do {
            for (int lane = 0; lane < 3; lane++) {
                int obstacleType = GetRandomValue(0, 3);
                switch (obstacleType) {
                case 0:
                    laneTypes[lane] = ObstacleType::JUMP_OVER;
                    break;
                case 1:
                    laneTypes[lane] = ObstacleType::DUCK_UNDER;
                    break;
                case 2:
                    laneTypes[lane] = ObstacleType::WALL;
                    break;
                case 3:
                    laneTypes[lane] = ObstacleType::LOW_BARRIER;
                    break;
                }
            }

            for (int lane = 0; lane < 3; lane++) {
                if (laneTypes[lane] != ObstacleType::WALL) {
                    hasPassableLane = true;
                    break;
                }
            }
        } while (!hasPassableLane);

        for (int lane = 0; lane < 3; lane++) {
            Obstacle obstacle;
            obstacle.lane = lane;
            obstacle.type = laneTypes[lane];

            switch (obstacle.type) {
            case ObstacleType::JUMP_OVER:
                obstacle.size = { 1.0f, 1.0f, 1.0f }; // Можно перепрыгнуть
                obstacle.color = DARKGRAY;
                obstacle.canLandOn = true; // Можно приземлиться сверху
                break;
            case ObstacleType::DUCK_UNDER:
                obstacle.size = { 1.0f, 1.0f, 1.0f }; // ИЗМЕНЕНО: такая же высота как JUMP_OVER (было 1.5f)
                obstacle.color = BROWN;
                obstacle.canLandOn = false; // Нельзя приземлиться сверху
                break;
            case ObstacleType::WALL:
                obstacle.size = { 1.0f, 3.0f, 1.0f };
                obstacle.color = MAROON;
                obstacle.canLandOn = false; // Нельзя приземлиться сверху
                break;
            case ObstacleType::LOW_BARRIER:
                obstacle.size = { 1.0f, 2.5f, 1.0f }; // Выше - нельзя перепрыгнуть, можно пригнуться
                obstacle.color = { 150, 75, 0, 255 };
                obstacle.canLandOn = false; // Нельзя приземлиться сверху
                break;
            }

            // Назначаем текстуру в зависимости от локации и типа препятствия
            obstacle.texture = GetObstacleTexture(obstacle.type);

            // ИСПРАВЛЕНИЕ: используем новую дальность спавна
            obstacle.position = { lanePositions[obstacle.lane], obstacle.size.y / 2, spawnDistance };
            obstacle.active = true;
            obstacle.speed = gameSpeed + (static_cast<float>(score) / 1000.0f);

            obstacles.push_back(obstacle);
        }
    }

    void UpdateCoins() {
        // Спавн монет
        coinSpawnTimer += GetFrameTime();
        if (coinSpawnTimer >= coinSpawnInterval) {
            SpawnCoin();
            coinSpawnTimer = 0;
        }

        // Обновление позиций монет
        for (auto& coin : coins) {
            if (coin.active) {
                // Монеты движутся с той же скоростью, что и препятствия
                coin.speed = gameSpeed + (static_cast<float>(score) / 1000.0f);

                // Эффект магнита: монеты притягиваются к игроку
                if (HasPowerUp(PowerUpType::MAGNET)) {
                    float magnetRange = 5.0f + (shop.upgrades[2].level * 0.5f);
                    float dx = player.position.x - coin.position.x;
                    float dz = player.position.z - coin.position.z;
                    float distance = sqrt(dx * dx + dz * dz);

                    if (distance < magnetRange && distance > 0.5f) {
                        // СИЛА ПРИТЯЖЕНИЯ ЗАВИСИТ ОТ СКОРОСТИ И РАССТОЯНИЯ
                        float pullStrength = 20.0f + (coin.speed * 0.8f);

                        // ПЛАВНОЕ ПРИТЯЖЕНИЕ
                        float attraction = pullStrength * GetFrameTime() * (1.0f - distance / magnetRange);
                        coin.position.x += (dx / distance) * attraction;

                        // ОСНОВНОЕ ДВИЖЕНИЕ ВПЕРЕД + ДОПОЛНИТЕЛЬНОЕ УСКОРЕНИЕ К ИГРОКУ
                        coin.position.z += coin.speed * GetFrameTime();
                        coin.position.z += (dz / distance) * attraction * 2.0f; // Более сильное притяжение по Z

                        // ДОПОЛНИТЕЛЬНОЕ УСКОРЕНИЕ ПРИ БЛИЗКОМ РАССТОЯНИИ
                        if (distance < 2.0f) {
                            coin.position.z += coin.speed * 0.5f * GetFrameTime();
                        }
                    }
                    else {
                        // ОБЫЧНОЕ ДВИЖЕНИЕ ЕСЛИ МОНЕТА ВНЕ ДИАПАЗОНА МАГНИТА
                        coin.position.z += coin.speed * GetFrameTime();
                    }
                }
                else {
                    // ОБЫЧНОЕ ДВИЖЕНИЕ БЕЗ МАГНИТА
                    coin.position.z += coin.speed * GetFrameTime();
                }

                // Деактивация монет
                if (coin.position.z > despawnDistance) {
                    coin.active = false;
                }
            }
        }

        coins.erase(std::remove_if(coins.begin(), coins.end(),
            [](const Coin& c) { return !c.active; }), coins.end());
    }

    void SpawnCoin() {
        Coin coin;
        // ИСПРАВЛЕНИЕ: используем новую дальность спавна
        coin.position = { lanePositions[GetRandomValue(0, 2)], 1.5f, spawnDistance };
        coin.active = true;
        // Монеты теперь имеют ту же скорость, что и препятствия
        coin.speed = gameSpeed + (static_cast<float>(score) / 1000.0f);

        coins.push_back(coin);
    }

    void UpdatePowerUps() {
        // Спавн усилений
        powerUpSpawnTimer += GetFrameTime();
        if (powerUpSpawnTimer >= powerUpSpawnInterval) {
            SpawnPowerUp();
            powerUpSpawnTimer = 0;
        }

        // Обновление позиций и анимации усилений
        for (auto& powerUp : powerUps) {
            if (powerUp.active) {
                // Усиления также движутся с увеличивающейся скоростью
                powerUp.speed = gameSpeed + (static_cast<float>(score) / 1000.0f);
                powerUp.position.z += powerUp.speed * GetFrameTime();
                powerUp.rotation += 2.0f * GetFrameTime();

                // ИСПРАВЛЕНИЕ: используем новую дальность деактивации
                if (powerUp.position.z > despawnDistance) {
                    powerUp.active = false;
                }
            }
        }

        powerUps.erase(std::remove_if(powerUps.begin(), powerUps.end(),
            [](const PowerUp& p) { return !p.active; }), powerUps.end());
    }

    void SpawnPowerUp() {
        PowerUp powerUp;
        // ИСПРАВЛЕНИЕ: используем новую дальность спавна
        powerUp.position = { lanePositions[GetRandomValue(0, 2)], 1.5f, spawnDistance };
        powerUp.active = true;
        // Усиления также имеют увеличивающуюся скорость
        powerUp.speed = gameSpeed + (static_cast<float>(score) / 1000.0f);
        powerUp.rotation = 0.0f;

        int powerUpType = GetRandomValue(0, 3);
        switch (powerUpType) {
        case 0:
            powerUp.type = PowerUpType::SPEED_BOOST;
            break;
        case 1:
            powerUp.type = PowerUpType::INVINCIBILITY;
            break;
        case 2:
            powerUp.type = PowerUpType::MAGNET;
            break;
        case 3:
            powerUp.type = PowerUpType::DOUBLE_POINTS;
            break;
        }

        // Назначаем текстуру способности (одинаковую на всех локациях)
        powerUp.texture = GetPowerUpTexture(powerUp.type);

        powerUps.push_back(powerUp);
    }

    void ApplyPowerUp(PowerUpType type) {
        float baseDuration = 5.0f;
        float upgradeBonus = 0.0f;

        switch (type) {
        case PowerUpType::SPEED_BOOST:
            upgradeBonus = shop.upgrades[0].value;
            break;
        case PowerUpType::INVINCIBILITY:
            upgradeBonus = shop.upgrades[1].value;
            break;
        case PowerUpType::MAGNET:
            upgradeBonus = shop.upgrades[2].value;
            // УВЕЛИЧИВАЕМ БАЗОВУЮ ДЛИТЕЛЬНОСТЬ ДЛЯ МАГНИТА
            baseDuration = 8.0f; // Еще больше увеличили длительность
            break;
        case PowerUpType::DOUBLE_POINTS:
            upgradeBonus = shop.upgrades[3].value;
            break;
        }

        float totalDuration = baseDuration + upgradeBonus;

        // ОСОБЫЙ СЛУЧАЙ ДЛЯ МАГНИТА - СБРАСЫВАЕМ ТАЙМЕР ПРИ ПОВТОРНОМ ПОДБОРЕ
        if (type == PowerUpType::MAGNET) {
            for (auto it = player.activePowerUps.begin(); it != player.activePowerUps.end(); ) {
                if (it->type == PowerUpType::MAGNET) {
                    it = player.activePowerUps.erase(it);
                }
                else {
                    ++it;
                }
            }
        }
        else {
            // Для остальных усилений ищем существующее
            for (auto& activePowerUp : player.activePowerUps) {
                if (activePowerUp.type == type) {
                    activePowerUp.timer = totalDuration;
                    return;
                }
            }
        }

        ActivePowerUp newPowerUp;
        newPowerUp.type = type;
        newPowerUp.timer = totalDuration;
        newPowerUp.duration = totalDuration;
        player.activePowerUps.push_back(newPowerUp);

        if (type == PowerUpType::SPEED_BOOST) {
            player.speed = player.originalSpeed * 1.5f;
        }
    }

    bool HasPowerUp(PowerUpType type) {
        for (const auto& activePowerUp : player.activePowerUps) {
            if (activePowerUp.type == type) {
                return true;
            }
        }
        return false;
    }

    void UpdatePowerUpEffects() {
        for (auto it = player.activePowerUps.begin(); it != player.activePowerUps.end(); ) {
            it->timer -= GetFrameTime();

            if (it->timer <= 0) {
                if (it->type == PowerUpType::SPEED_BOOST) {
                    player.speed = player.originalSpeed;
                }
                it = player.activePowerUps.erase(it);
            }
            else {
                ++it;
            }
        }
    }

    void UpdateCamera() {
        camera.target = { player.position.x, player.position.y, player.position.z };
        camera.position = { player.position.x, player.position.y + 3.0f, player.position.z + 8.0f };
    }

    void CheckCollisions() {
        // Используем bounding box только для передней грани игрока
        BoundingBox playerFrontBox = GetPlayerFrontFaceBox();

        // Сбрасываем статус нахождения на препятствии
        bool wasOnObstacle = player.isOnObstacle;
        player.isOnObstacle = false;

        for (auto& obstacle : obstacles) {
            if (obstacle.active && player.lane == obstacle.lane) {
                // Используем bounding box только для передней грани препятствия
                BoundingBox obstacleFrontBox = GetObstacleFrontFaceBox(obstacle);

                if (CheckCollisionBoxes(playerFrontBox, obstacleFrontBox)) {
                    if (HasPowerUp(PowerUpType::INVINCIBILITY)) {
                        continue;
                    }

                    // Проверяем, находимся ли мы СВЕРХУ препятствия
                    float playerBottom = player.position.y - player.size.y / 2;
                    float obstacleTop = obstacle.position.y + obstacle.size.y / 2;

                    if (playerBottom >= obstacleTop - 0.1f && obstacle.canLandOn) {
                        // Игрок стоит сверху на препятствии
                        player.isOnObstacle = true;
                        continue; // Не считаем это столкновением
                    }

                    bool canAvoid = false;

                    switch (obstacle.type) {
                    case ObstacleType::JUMP_OVER:
                        // JUMP OVER: можно перепрыгнуть, но нельзя пригнуться
                        canAvoid = player.isJumping && !player.isRolling;
                        break;
                    case ObstacleType::DUCK_UNDER:
                        // DUCK UNDER: можно пригнуться, но нельзя перепрыгнуть
                        // ИЗМЕНЕНО: теперь kill zone такая же как у JUMP_OVER
                        canAvoid = player.isRolling && !player.isJumping;
                        break;
                    case ObstacleType::WALL:
                        canAvoid = false;
                        break;
                    case ObstacleType::LOW_BARRIER:
                        // LOW BARRIER: нельзя перепрыгнуть, можно ТОЛЬКО пригнуться
                        canAvoid = player.isRolling && !player.isJumping;
                        break;
                    }

                    if (!canAvoid) {
                        gameOver = true;
                        return;
                    }
                }
            }
        }

        // Если был на препятствии, но больше нет - сбрасываем высоту
        if (wasOnObstacle && !player.isOnObstacle && !player.isJumping) {
            player.position.y = 1.0f;
        }

        for (auto& coin : coins) {
            if (coin.active) {
                // Для монет используем обычную проверку сферы
                if (CheckCollisionBoxSphere(playerFrontBox, coin.position, 0.5f)) {
                    coin.active = false;
                    coinsCollected++;
                    int coinValue = 100 + static_cast<int>(shop.upgrades[4].value);
                    score += HasPowerUp(PowerUpType::DOUBLE_POINTS) ? coinValue * 2 : coinValue;
                }
            }
        }

        for (auto& powerUp : powerUps) {
            if (powerUp.active) {
                // Для усилений используем обычную проверку сферы
                if (CheckCollisionBoxSphere(playerFrontBox, powerUp.position, 0.5f)) {
                    powerUp.active = false;
                    ApplyPowerUp(powerUp.type);
                }
            }
        }
    }

    void ResetGame() {
        player.position = { lanePositions[1], 1.0f, 0.0f };
        player.lane = 1;
        player.targetLane = 1;
        player.isJumping = false;
        player.isRolling = false;
        player.jumpVelocity = 0;
        player.speed = player.originalSpeed;
        player.isOnObstacle = false;
        player.rollCooldownTimer = 0.0f;
        player.rollDuration = 0.0f;

        obstacles.clear();
        coins.clear();
        powerUps.clear();
        player.activePowerUps.clear();

        score = 0;
        coinsCollected = 0;
        gameOver = false;
        environmentOffset = 0.0f;
    }

    void Draw3DWorld() {
        DrawPlane({ 0.0f, 0.0f, 0.0f }, { 50.0f, 100.0f }, GetCurrentGroundColor());

        // НОВОЕ: рисуем три отдельные полосы с разными цветами
        for (int i = 0; i < 3; i++) {
            Color laneColor;
            switch (i) {
            case 0: laneColor = GetLeftLaneColor(); break;
            case 1: laneColor = GetMiddleLaneColor(); break;
            case 2: laneColor = GetRightLaneColor(); break;
            }
            DrawCube({ lanePositions[i], 0.01f, 0.0f }, laneWidth, 0.02f, 100.0f, laneColor);
        }

        // Рисуем окружение с учетом локации
        DrawEnvironment();

        // Рисуем препятствия с текстурами
        for (auto& obstacle : obstacles) {
            DrawObstacle(obstacle);
        }

        for (auto& coin : coins) {
            if (coin.active) {
                DrawSphere(coin.position, 0.5f, GOLD);
            }
        }

        // Рисуем способности с текстурами
        for (auto& powerUp : powerUps) {
            DrawPowerUp(powerUp);
        }

        DrawPlayer();

        if (HasPowerUp(PowerUpType::MAGNET)) {
            float magnetRadius = 2.0f + (shop.upgrades[2].level * 0.3f);
        }
    }

    void DrawPlayer() {
        // Если для текущего персонажа доступна анимированная текстура и она загружена
        if (menu.characters[player.characterType].useAnimatedTexture &&
            characterAnimations[player.characterType].loaded) {

            AnimatedTexture& animTex = characterAnimations[player.characterType];
            Vector3 scaledSize = {
                player.size.x * animTex.scale,
                player.size.y * animTex.scale,
                player.size.z * animTex.scale
            };

            // Используем текущий кадр анимации
            Texture2D currentFrame = GetCurrentFrame(animTex);
            DrawCubeTexture(player.position, scaledSize, currentFrame, WHITE);
        }
        else {
            // Fallback: используем статичную текстуру или цветной куб
            Texture2D characterTexture = GetCharacterTexture();

            if (IsTextureReady(characterTexture)) {
                DrawCubeTexture(player.position, player.size, characterTexture, RAYWHITE);
            }
            else {
                Color playerColor = menu.characters[player.characterType].defaultColor;
                if (HasPowerUp(PowerUpType::INVINCIBILITY) && ((int)(GetTime() * 10) % 2 == 0)) {
                    playerColor = GOLD;
                }
                DrawCube(player.position, player.size.x, player.size.y, player.size.z, playerColor);
                DrawCubeWires(player.position, player.size.x, player.size.y, player.size.z, BLACK);
            }
        }
    }

    void DrawShop() {
        ClearBackground(DARKBLUE);

        DrawText("UPGRADE SHOP", screenWidth / 2 - MeasureText("UPGRADE SHOP", 50) / 2, 50, 50, YELLOW);
        DrawText(TextFormat("Total Coins: %d", shop.totalCoins), screenWidth / 2 - MeasureText(TextFormat("Total Coins: %d", shop.totalCoins), 30) / 2, 120, 30, GOLD);

        int startY = 180;
        for (int i = 0; i < (int)shop.upgrades.size(); i++) {
            const Upgrade& upgrade = shop.upgrades[i];
            Color textColor = (i == shop.selectedUpgrade) ? GREEN : WHITE;
            Color levelColor = (upgrade.level < upgrade.maxLevel) ? LIME : GOLD;

            DrawText(TextFormat("%s (Level %d/%d)", upgrade.name.c_str(), upgrade.level, upgrade.maxLevel),
                100, startY + i * 80, 25, textColor);

            DrawText(upgrade.description.c_str(), 100, startY + i * 80 + 30, 18, LIGHTGRAY);

            std::string effectText;
            switch (i) {
            case 0: effectText = TextFormat("Duration: %.1fs", 5.0f + upgrade.value); break;
            case 1: effectText = TextFormat("Duration: %.1fs", 5.0f + upgrade.value); break;
            case 2: effectText = TextFormat("Duration: %.1fs | Range: +%.1f", 5.0f + upgrade.value, upgrade.level * 0.5f); break;
            case 3: effectText = TextFormat("Duration: %.1fs", 5.0f + upgrade.value); break;
            case 4: effectText = TextFormat("Value: %d", 100 + static_cast<int>(upgrade.value)); break;
            }
            DrawText(effectText.c_str(), 100, startY + i * 80 + 50, 16, SKYBLUE);

            if (upgrade.level < upgrade.maxLevel) {
                Color costColor = (shop.totalCoins >= upgrade.cost) ? GREEN : RED;
                DrawText(TextFormat("Cost: %d coins", upgrade.cost), screenWidth - 250, startY + i * 80 + 20, 20, costColor);

                if (i == shop.selectedUpgrade) {
                    DrawText("[ENTER] TO BUY", screenWidth - 250, startY + i * 80 + 45, 18, YELLOW);
                }
            }
            else {
                DrawText("MAX LEVEL", screenWidth - 250, startY + i * 80 + 20, 20, GOLD);
            }

            if (i == shop.selectedUpgrade) {
                DrawRectangle(90, startY + i * 80 - 5, screenWidth - 180, 70, Fade(BLUE, 0.2f));
                DrawRectangleLines(90, startY + i * 80 - 5, screenWidth - 180, 70, BLUE);
            }
        }

        DrawText("USE ARROWS TO NAVIGATE", screenWidth / 2 - MeasureText("USE ARROWS TO NAVIGATE", 20) / 2, screenHeight - 80, 20, LIGHTGRAY);
        DrawText("PRESS ENTER TO BUY UPGRADE", screenWidth / 2 - MeasureText("PRESS ENTER TO BUY UPGRADE", 20) / 2, screenHeight - 50, 20, LIGHTGRAY);
        DrawText("PRESS ESC, M OR S TO RETURN TO MENU", screenWidth / 2 - MeasureText("PRESS ESC, M OR S TO RETURN TO MENU", 20) / 2, screenHeight - 20, 20, LIGHTGRAY);
    }

    void Draw() {
        BeginDrawing();
        ClearBackground(GetCurrentBackgroundColor());

        if (menu.isActive) {
            DrawMenu();
        }
        else if (shop.isActive) {
            DrawShop();
        }
        else if (gameOver) {
            DrawRectangle(0, 0, screenWidth, screenHeight, Fade(BLACK, 0.5f));
            DrawText("GAME OVER", screenWidth / 2 - MeasureText("GAME OVER", 40) / 2, screenHeight / 2 - 80, 40, RED);
            DrawText(TextFormat("Final Score: %d", score), screenWidth / 2 - MeasureText(TextFormat("Final Score: %d", score), 20) / 2, screenHeight / 2 - 30, 20, WHITE);
            DrawText(TextFormat("Coins Collected: %d", coinsCollected), screenWidth / 2 - MeasureText(TextFormat("Coins Collected: %d", coinsCollected), 20) / 2, screenHeight / 2, 20, GOLD);
            DrawText("Press R to restart", screenWidth / 2 - MeasureText("Press R to restart", 20) / 2, screenHeight / 2 + 30, 20, WHITE);
            DrawText("Press M for menu", screenWidth / 2 - MeasureText("Press M for menu", 20) / 2, screenHeight / 2 + 60, 20, WHITE);
            DrawText("Press S for shop", screenWidth / 2 - MeasureText("Press S for shop", 20) / 2, screenHeight / 2 + 90, 20, GREEN);
        }
        else {
            BeginMode3D(camera);
            BeginBlendMode(BLEND_ALPHA);

            Draw3DWorld();

            EndBlendMode();
            EndMode3D();

            DrawText(TextFormat("Score: %d", score), 10, 10, 20, BLACK);
            DrawText(TextFormat("Coins: %d", coinsCollected), 10, 40, 20, BLACK);
            DrawText(TextFormat("Lane: %d", player.lane + 1), 10, 70, 20, BLACK);
            DrawText(TextFormat("Target Lane: %d", player.targetLane + 1), 10, 100, 15, DARKGRAY);
            DrawText(TextFormat("Location: %s", menu.locations[menu.selectedLocation].name.c_str()), 10, 120, 15, DARKGRAY);
            DrawText(TextFormat("Character: %s", menu.characters[player.characterType].name.c_str()), 10, 140, 15, DARKGRAY);

            int powerUpY = 160;
            if (!player.activePowerUps.empty()) {
                DrawText("ACTIVE POWER-UPS:", 10, powerUpY, 15, DARKPURPLE);
                powerUpY += 20;

                for (const auto& activePowerUp : player.activePowerUps) {
                    std::string powerUpName;
                    Color powerUpColor;

                    switch (activePowerUp.type) {
                    case PowerUpType::SPEED_BOOST:
                        powerUpName = "SPEED BOOST";
                        powerUpColor = ORANGE;
                        break;
                    case PowerUpType::INVINCIBILITY:
                        powerUpName = "INVINCIBILITY";
                        powerUpColor = GOLD;
                        break;
                    case PowerUpType::MAGNET:
                        powerUpName = "COIN MAGNET";
                        powerUpColor = BLUE;
                        break;
                    case PowerUpType::DOUBLE_POINTS:
                        powerUpName = "DOUBLE POINTS";
                        powerUpColor = GREEN;
                        break;
                    }

                    DrawText(TextFormat("%s: %.1fs", powerUpName.c_str(), activePowerUp.timer),
                        10, powerUpY, 15, powerUpColor);
                    powerUpY += 20;
                }
            }

            // ИСПРАВЛЕНО: правильное использование TextFormat
            char rollText[64];
            snprintf(rollText, sizeof(rollText), "ROLL: DOWN (Cooldown: %.1fs)", player.rollCooldownTimer);
            DrawText("JUMP: SPACE/UP", 10, powerUpY, 15, DARKGREEN);
            DrawText(rollText, 10, powerUpY + 20, 15, DARKBLUE);
            DrawText("MOVE: LEFT/RIGHT", 10, powerUpY + 40, 15, DARKPURPLE);
            DrawText("MENU: M", 10, powerUpY + 60, 15, DARKBROWN);

            DrawText("Obstacles:", 10, powerUpY + 90, 15, BLACK);
            DrawText("▲ - Jump Over (can land on top)", 10, powerUpY + 110, 12, DARKGREEN);
            DrawText("▼ - Roll Under", 10, powerUpY + 125, 12, DARKBLUE);
            DrawText("✕ - Wall (Avoid)", 10, powerUpY + 140, 12, RED);
            DrawText("▬ - Low Barrier (Roll)", 10, powerUpY + 155, 12, ORANGE);

            DrawText("Power-Ups:", 10, powerUpY + 175, 15, BLACK);
            DrawText("⚡ - Speed Boost", 10, powerUpY + 195, 12, ORANGE);
            DrawText("★ - Invincibility", 10, powerUpY + 210, 12, GOLD);
            DrawText("🧲 - Coin Magnet", 10, powerUpY + 225, 12, BLUE);
            DrawText("2X - Double Points", 10, powerUpY + 240, 12, GREEN);
        }

        EndDrawing();
    }

    void DrawMenu() {
        ClearBackground(DARKBLUE);

        DrawText("RUNNER 3D WITH CHARACTER ANIMATIONS", screenWidth / 2 - MeasureText("RUNNER 3D WITH CHARACTER ANIMATIONS", 40) / 2, 50, 40, YELLOW);
        DrawText(TextFormat("Total Coins: %d", shop.totalCoins), screenWidth / 2 - MeasureText(TextFormat("Total Coins: %d", shop.totalCoins), 30) / 2, 100, 30, GOLD);

        DrawText("SELECT LOCATION:", screenWidth / 2 - MeasureText("SELECT LOCATION:", 30) / 2, 150, 30, WHITE);
        for (int i = 0; i < (int)menu.locations.size(); i++) {
            Color color = (i == menu.selectedLocation) ? GREEN : WHITE;
            DrawText(menu.locations[i].name.c_str(), screenWidth / 2 - MeasureText(menu.locations[i].name.c_str(), 25) / 2, 200 + i * 40, 25, color);
        }

        DrawText("SELECT CHARACTER: (A/D to change)", screenWidth / 2 - MeasureText("SELECT CHARACTER: (A/D to change)", 30) / 2, 350, 30, WHITE);
        for (int i = 0; i < (int)menu.characters.size(); i++) {
            Color color = (i == menu.selectedCharacter) ? GREEN : WHITE;
            DrawText(menu.characters[i].name.c_str(), screenWidth / 2 - MeasureText(menu.characters[i].name.c_str(), 25) / 2, 400 + i * 40, 25, color);
        }

        DrawText("PRESS ENTER TO START", screenWidth / 2 - MeasureText("PRESS ENTER TO START", 30) / 2, 550, 30, YELLOW);
        DrawText("USE ARROWS TO NAVIGATE", screenWidth / 2 - MeasureText("USE ARROWS TO NAVIGATE", 20) / 2, 600, 20, LIGHTGRAY);
        DrawText("PRESS S FOR UPGRADE SHOP", screenWidth / 2 - MeasureText("PRESS S FOR UPGRADE SHOP", 20) / 2, 630, 20, LIME);
    }
};

int main() {
    Game game;
    game.Run();
    return 0;
}