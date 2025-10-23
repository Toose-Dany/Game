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
    bool isJumping;
    bool isDucking;
    float jumpVelocity;
    float gravity;
    int characterType;

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
};

// Структура для меню
struct Menu {
    bool isActive;
    int selectedLocation;
    int selectedCharacter;
    std::vector<std::string> locations;
    std::vector<std::string> characters;

    Menu() {
        isActive = true;
        selectedLocation = 0;
        selectedCharacter = 0;
        locations = { "City", "Forest", "Desert", "Winter" };
        characters = { "Default", "Ninja", "Robot", "Girl" };
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
    const int screenWidth = 1100;
    const int screenHeight = 800;

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
    Color backgroundColor;
    Color groundColor;

    // Текстуры для препятствий по локациям
    Texture2D cityJumpTexture;
    Texture2D cityDuckTexture;
    Texture2D cityWallTexture;
    Texture2D cityLowBarrierTexture;

    Texture2D forestJumpTexture;
    Texture2D forestDuckTexture;
    Texture2D forestWallTexture;
    Texture2D forestLowBarrierTexture;

    Texture2D desertJumpTexture;
    Texture2D desertDuckTexture;
    Texture2D desertWallTexture;
    Texture2D desertLowBarrierTexture;

    Texture2D winterJumpTexture;
    Texture2D winterDuckTexture;
    Texture2D winterWallTexture;
    Texture2D winterLowBarrierTexture;

    // Текстуры для усилений
    Texture2D speedBoostTexture;
    Texture2D invincibilityTexture;
    Texture2D magnetTexture;
    Texture2D doublePointsTexture;

    bool texturesLoaded;
    float environmentOffset;

    // Текстуры персонажей (основные)
    Texture2D characterTexture1; // Default
    Texture2D characterTexture2; // Ninja
    Texture2D characterTexture3; // Robot
    Texture2D characterTexture4; // Girl

public:
    Game() {
        InitWindow(screenWidth, screenHeight, "Runner 3D with Character Textures");

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
        player.isJumping = false;
        player.isDucking = false;
        player.jumpVelocity = 0;
        player.gravity = 15.0f;
        player.characterType = 0;

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
        backgroundColor = SKYBLUE;
        groundColor = GREEN;
        texturesLoaded = false;
        environmentOffset = 0.0f;

        // Инициализация текстур персонажей как пустых
        characterTexture1 = { 0 };
        characterTexture2 = { 0 };
        characterTexture3 = { 0 };
        characterTexture4 = { 0 };

        // Загружаем текстуры
        LoadTextures();

        SetTargetFPS(60);
    }

    ~Game() {
        // Выгружаем текстуры препятствий
        UnloadLocationTextures();

        // Выгружаем текстуры усилений
        UnloadTexture(speedBoostTexture);
        UnloadTexture(invincibilityTexture);
        UnloadTexture(magnetTexture);
        UnloadTexture(doublePointsTexture);

        // Выгружаем текстуры персонажей
        if (characterTexture1.id != 0) UnloadTexture(characterTexture1);
        if (characterTexture2.id != 0) UnloadTexture(characterTexture2);
        if (characterTexture3.id != 0) UnloadTexture(characterTexture3);
        if (characterTexture4.id != 0) UnloadTexture(characterTexture4);

        CloseWindow();
    }

    void Run() {
        while (!WindowShouldClose()) {
            Update();
            Draw();
        }
    }

private:
    void UnloadLocationTextures() {
        // Выгружаем текстуры для City
        if (cityJumpTexture.id != 0) UnloadTexture(cityJumpTexture);
        if (cityDuckTexture.id != 0) UnloadTexture(cityDuckTexture);
        if (cityWallTexture.id != 0) UnloadTexture(cityWallTexture);
        if (cityLowBarrierTexture.id != 0) UnloadTexture(cityLowBarrierTexture);

        // Выгружаем текстуры для Forest
        if (forestJumpTexture.id != 0) UnloadTexture(forestJumpTexture);
        if (forestDuckTexture.id != 0) UnloadTexture(forestDuckTexture);
        if (forestWallTexture.id != 0) UnloadTexture(forestWallTexture);
        if (forestLowBarrierTexture.id != 0) UnloadTexture(forestLowBarrierTexture);

        // Выгружаем текстуры для Desert
        if (desertJumpTexture.id != 0) UnloadTexture(desertJumpTexture);
        if (desertDuckTexture.id != 0) UnloadTexture(desertDuckTexture);
        if (desertWallTexture.id != 0) UnloadTexture(desertWallTexture);
        if (desertLowBarrierTexture.id != 0) UnloadTexture(desertLowBarrierTexture);

        // Выгружаем текстуры для Winter
        if (winterJumpTexture.id != 0) UnloadTexture(winterJumpTexture);
        if (winterDuckTexture.id != 0) UnloadTexture(winterDuckTexture);
        if (winterWallTexture.id != 0) UnloadTexture(winterWallTexture);
        if (winterLowBarrierTexture.id != 0) UnloadTexture(winterLowBarrierTexture);
    }

    void LoadTextures() {
        // Загружаем текстуры для каждой локации
        LoadLocationTextures();

        // Создаем текстуры для усилений
        speedBoostTexture = CreateSpeedBoostTexture();
        invincibilityTexture = CreateInvincibilityTexture();
        magnetTexture = CreateMagnetTexture();
        doublePointsTexture = CreateDoublePointsTexture();

        // Загружаем текстуры персонажей как основные
        LoadCharacterTexture("runner.png", characterTexture1);   // Default
        LoadCharacterTexture("runner.png", characterTexture2);     // Ninja
        LoadCharacterTexture("runner.png", characterTexture3);     // Robot
        LoadCharacterTexture("runner.png", characterTexture4);      // Girl

        texturesLoaded = AreTexturesLoaded();

        TraceLog(LOG_INFO, "All textures loaded: %s", texturesLoaded ? "YES" : "NO");
    }

    void LoadLocationTextures() {
        // Загружаем текстуры для City
        LoadObstacleTexture("train.png", cityJumpTexture);
        LoadObstacleTexture("train.png", cityDuckTexture);
        LoadObstacleTexture("train.png", cityWallTexture);
        LoadObstacleTexture("train.png", cityLowBarrierTexture);

        // Загружаем текстуры для Forest
        LoadObstacleTexture("forest_jump.png", forestJumpTexture);
        LoadObstacleTexture("forest_duck.png", forestDuckTexture);
        LoadObstacleTexture("forest_wall.png", forestWallTexture);
        LoadObstacleTexture("forest_barrier.png", forestLowBarrierTexture);

        // Загружаем текстуры для Desert
        LoadObstacleTexture("desert_jump.png", desertJumpTexture);
        LoadObstacleTexture("desert_duck.png", desertDuckTexture);
        LoadObstacleTexture("desert_wall.png", desertWallTexture);
        LoadObstacleTexture("desert_barrier.png", desertLowBarrierTexture);

        // Загружаем текстуры для Winter
        LoadObstacleTexture("winter_jump.png", winterJumpTexture);
        LoadObstacleTexture("winter_duck.png", winterDuckTexture);
        LoadObstacleTexture("winter_wall.png", winterWallTexture);
        LoadObstacleTexture("winter_barrier.png", winterLowBarrierTexture);
    }

    void LoadObstacleTexture(const char* filepath, Texture2D& texture) {
        if (FileExists(filepath)) {
            Image image = LoadImage(filepath);
            if (image.data != NULL) {
                ImageFlipVertical(&image);
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
                ImageFlipVertical(&image); // Важно: переворачиваем текстуру
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

                // Глаза
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
        // Проверяем, что все основные текстуры загружены
        return cityJumpTexture.id != 0 && cityDuckTexture.id != 0 &&
            forestJumpTexture.id != 0 && forestDuckTexture.id != 0 &&
            speedBoostTexture.id != 0 && invincibilityTexture.id != 0;
    }

    Texture2D GetObstacleTexture(ObstacleType type) {
        // Возвращаем текстуру препятствия в зависимости от текущей локации и типа
        switch (menu.selectedLocation) {
        case 0: // City
            switch (type) {
            case ObstacleType::JUMP_OVER: return cityJumpTexture;
            case ObstacleType::DUCK_UNDER: return cityDuckTexture;
            case ObstacleType::WALL: return cityWallTexture;
            case ObstacleType::LOW_BARRIER: return cityLowBarrierTexture;
            default: return cityJumpTexture;
            }
        case 1: // Forest
            switch (type) {
            case ObstacleType::JUMP_OVER: return forestJumpTexture;
            case ObstacleType::DUCK_UNDER: return forestDuckTexture;
            case ObstacleType::WALL: return forestWallTexture;
            case ObstacleType::LOW_BARRIER: return forestLowBarrierTexture;
            default: return forestJumpTexture;
            }
        case 2: // Desert
            switch (type) {
            case ObstacleType::JUMP_OVER: return desertJumpTexture;
            case ObstacleType::DUCK_UNDER: return desertDuckTexture;
            case ObstacleType::WALL: return desertWallTexture;
            case ObstacleType::LOW_BARRIER: return desertLowBarrierTexture;
            default: return desertJumpTexture;
            }
        case 3: // Winter
            switch (type) {
            case ObstacleType::JUMP_OVER: return winterJumpTexture;
            case ObstacleType::DUCK_UNDER: return winterDuckTexture;
            case ObstacleType::WALL: return winterWallTexture;
            case ObstacleType::LOW_BARRIER: return winterLowBarrierTexture;
            default: return winterJumpTexture;
            }
        default: return cityJumpTexture;
        }
    }

    Texture2D GetCharacterTexture(int characterType) {
        // Возвращаем текстуру для выбранного персонажа
        switch (characterType) {
        case 0: return characterTexture1; // Default
        case 1: return characterTexture2; // Ninja
        case 2: return characterTexture3; // Robot
        case 3: return characterTexture4; // Girl
        default: return characterTexture1;
        }
    }

    // Функции создания текстур для усилений
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

        rlSetTexture(texture.id);
        rlBegin(RL_QUADS);
        rlColor4ub(color.r, color.g, color.b, color.a);

        rlNormal3f(0.0f, 0.0f, 1.0f);
        rlTexCoord2f(0.0f, 0.0f); rlVertex3f(x - width / 2, y - height / 2, z + length / 2);
        rlTexCoord2f(1.0f, 0.0f); rlVertex3f(x + width / 2, y - height / 2, z + length / 2);
        rlTexCoord2f(1.0f, 1.0f); rlVertex3f(x + width / 2, y + height / 2, z + length / 2);
        rlTexCoord2f(0.0f, 1.0f); rlVertex3f(x - width / 2, y + height / 2, z + length / 2);

        rlEnd();
        rlSetTexture(0);
    }


    void DrawObstacle(const Obstacle& obstacle) {
        if (obstacle.active) {
            if (texturesLoaded && obstacle.texture.id != 0) {
                DrawCubeTexture(obstacle.position, obstacle.size, obstacle.texture,RAYWHITE);
            }
            else {
                // Fallback - рисуем цветной куб если текстура не загружена
                //DrawCube(obstacle.position, obstacle.size.x, obstacle.size.y, obstacle.size.z, obstacle.color);
            }
            DrawCubeWires(obstacle.position, obstacle.size.x, obstacle.size.y, obstacle.size.z, BLACK);
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

        HandleInput();
        UpdatePlayer();
        UpdateObstacles();
        UpdateCoins();
        UpdatePowerUps();
        UpdateCamera();
        CheckCollisions();
        UpdatePowerUpEffects();

        // Обновляем параллакс-эффект
        environmentOffset += gameSpeed * 0.3f * GetFrameTime();
        if (environmentOffset > 50.0f) environmentOffset = 0.0f;

        // Увеличиваем счет
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
            ApplyLocationSettings();
            player.characterType = menu.selectedCharacter;
            menu.isActive = false;
        }

        if (oldLocation != menu.selectedLocation) {
            ApplyLocationSettings();
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

    void ApplyLocationSettings() {
        switch (menu.selectedLocation) {
        case 0: // City
            backgroundColor = SKYBLUE;
            groundColor = GRAY;
            break;
        case 1: // Forest
            backgroundColor = DARKGREEN;
            groundColor = GREEN;
            break;
        case 2: // Desert
            backgroundColor = { 240, 200, 150, 255 };
            groundColor = { 210, 180, 140, 255 };
            break;
        case 3: // Winter
            backgroundColor = { 200, 220, 240, 255 };
            groundColor = WHITE;
            break;
        }
    }

    void HandleInput() {
        // Движение влево-вправо
        if (IsKeyPressed(KEY_LEFT) && player.lane > 0) {
            player.lane--;
            player.position.x = lanePositions[player.lane];
        }
        if (IsKeyPressed(KEY_RIGHT) && player.lane < 2) {
            player.lane++;
            player.position.x = lanePositions[player.lane];
        }

        // Прыжок
        if ((IsKeyPressed(KEY_SPACE) || IsKeyPressed(KEY_UP)) && !player.isJumping && !player.isDucking) {
            player.isJumping = true;
            player.jumpVelocity = 8.0f;
        }

        // Приседание
        if (IsKeyDown(KEY_DOWN) && !player.isJumping) {
            player.isDucking = true;
            player.size.y = 1.0f;
            player.position.y = 0.5f;
        }
        else {
            player.isDucking = false;
            player.size.y = 2.0f;
            if (!player.isJumping) {
                player.position.y = 1.0f;
            }
        }
    }

    void UpdatePlayer() {
        // Обновление прыжка
        if (player.isJumping) {
            player.position.y += player.jumpVelocity * GetFrameTime();
            player.jumpVelocity -= player.gravity * GetFrameTime();

            if (player.position.y <= 1.0f) {
                player.position.y = 1.0f;
                player.isJumping = false;
                player.jumpVelocity = 0;
            }
        }
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

                if (obstacle.position.z > 10.0f) {
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
            obstacle.size = { 1.0f, 1.0f, 1.0f };
            obstacle.color = DARKGRAY;
            break;
        case 1:
            obstacle.type = ObstacleType::DUCK_UNDER;
            obstacle.size = { 1.0f, 1.0f, 1.0f };
            obstacle.color = BROWN;
            break;
        case 2:
            obstacle.type = ObstacleType::WALL;
            obstacle.size = { 1.0f, 3.0f, 1.0f };
            obstacle.color = MAROON;
            break;
        case 3:
            obstacle.type = ObstacleType::LOW_BARRIER;
            obstacle.size = { 1.0f, 2.0f, 1.0f };
            obstacle.color = { 150, 75, 0, 255 };
            break;
        }

        // Назначаем текстуру в зависимости от локации и типа препятствия
        obstacle.texture = GetObstacleTexture(obstacle.type);

        obstacle.position = { lanePositions[obstacle.lane], obstacle.size.y / 2, -20.0f };
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
                obstacle.size = { 1.0f, 1.0f, 1.0f };
                obstacle.color = DARKGRAY;
                break;
            case ObstacleType::DUCK_UNDER:
                obstacle.size = { 1.0f, 1.0f, 1.0f };
                obstacle.color = BROWN;
                break;
            case ObstacleType::WALL:
                obstacle.size = { 1.0f, 3.0f, 1.0f };
                obstacle.color = MAROON;
                break;
            case ObstacleType::LOW_BARRIER:
                obstacle.size = { 1.0f, 1.5f, 1.0f };
                obstacle.color = { 150, 75, 0, 255 };
                break;
            }

            // Назначаем текстуру в зависимости от локации и типа препятствия
            obstacle.texture = GetObstacleTexture(obstacle.type);

            obstacle.position = { lanePositions[obstacle.lane], obstacle.size.y / 2, -20.0f };
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
                coin.position.z += coin.speed * GetFrameTime();

                // Эффект магнита: монеты притягиваются к игроку
                if (HasPowerUp(PowerUpType::MAGNET)) {
                    float magnetRange = 5.0f + (shop.upgrades[2].level * 0.5f);
                    float dx = player.position.x - coin.position.x;
                    float dz = player.position.z - coin.position.z;
                    float distance = sqrt(dx * dx + dz * dz);

                    if (distance < magnetRange && distance > 0.5f) {
                        float pullStrength = 10.0f;
                        coin.position.x += (dx / distance) * pullStrength * GetFrameTime();
                        coin.position.z += (dz / distance) * pullStrength * GetFrameTime();
                    }
                }

                if (coin.position.z > 10.0f) {
                    coin.active = false;
                }
            }
        }

        coins.erase(std::remove_if(coins.begin(), coins.end(),
            [](const Coin& c) { return !c.active; }), coins.end());
    }

    void SpawnCoin() {
        Coin coin;
        coin.position = { lanePositions[GetRandomValue(0, 2)], 1.5f, -20.0f };
        coin.active = true;
        coin.speed = gameSpeed;

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
                powerUp.position.z += powerUp.speed * GetFrameTime();
                powerUp.rotation += 2.0f * GetFrameTime();

                if (powerUp.position.z > 10.0f) {
                    powerUp.active = false;
                }
            }
        }

        powerUps.erase(std::remove_if(powerUps.begin(), powerUps.end(),
            [](const PowerUp& p) { return !p.active; }), powerUps.end());
    }

    void SpawnPowerUp() {
        PowerUp powerUp;
        powerUp.position = { lanePositions[GetRandomValue(0, 2)], 1.5f, -20.0f };
        powerUp.active = true;
        powerUp.speed = gameSpeed;
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
            break;
        case PowerUpType::DOUBLE_POINTS:
            upgradeBonus = shop.upgrades[3].value;
            break;
        }

        float totalDuration = baseDuration + upgradeBonus;

        for (auto& activePowerUp : player.activePowerUps) {
            if (activePowerUp.type == type) {
                activePowerUp.timer = totalDuration;
                return;
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
        BoundingBox playerBox = {
            { player.position.x - player.size.x / 2, player.position.y - player.size.y / 2, player.position.z - player.size.z / 2 },
            { player.position.x + player.size.x / 2, player.position.y + player.size.y / 2, player.position.z + player.size.z / 2 }
        };

        for (auto& obstacle : obstacles) {
            if (obstacle.active && player.lane == obstacle.lane) {
                BoundingBox obstacleBox = {
                    { obstacle.position.x - obstacle.size.x / 2, obstacle.position.y - obstacle.size.y / 2, obstacle.position.z - obstacle.size.z / 2 },
                    { obstacle.position.x + obstacle.size.x / 2, obstacle.position.y + obstacle.size.y / 2, obstacle.position.z + obstacle.size.z / 2 }
                };

                if (CheckCollisionBoxes(playerBox, obstacleBox)) {
                    if (HasPowerUp(PowerUpType::INVINCIBILITY)) {
                        continue;
                    }

                    bool canAvoid = false;

                    switch (obstacle.type) {
                    case ObstacleType::JUMP_OVER:
                        canAvoid = player.isJumping;
                        break;
                    case ObstacleType::DUCK_UNDER:
                        canAvoid = player.isDucking;
                        break;
                    case ObstacleType::WALL:
                        canAvoid = false;
                        break;
                    case ObstacleType::LOW_BARRIER:
                        // НИЗКИЙ БАРЬЕР: нельзя перепрыгнуть, можно ТОЛЬКО пригнуться
                        canAvoid = player.isDucking && !player.isJumping;
                        break;
                    }

                    if (!canAvoid) {
                        gameOver = true;
                        return;
                    }
                }
            }
        }

        for (auto& coin : coins) {
            if (coin.active) {
                if (CheckCollisionBoxSphere(playerBox, coin.position, 0.5f)) {
                    coin.active = false;
                    coinsCollected++;
                    int coinValue = 100 + static_cast<int>(shop.upgrades[4].value);
                    score += HasPowerUp(PowerUpType::DOUBLE_POINTS) ? coinValue * 2 : coinValue;
                }
            }
        }

        for (auto& powerUp : powerUps) {
            if (powerUp.active) {
                if (CheckCollisionBoxSphere(playerBox, powerUp.position, 0.5f)) {
                    powerUp.active = false;
                    ApplyPowerUp(powerUp.type);
                }
            }
        }
    }

    void ResetGame() {
        player.position = { lanePositions[1], 1.0f, 0.0f };
        player.lane = 1;
        player.isJumping = false;
        player.isDucking = false;
        player.jumpVelocity = 0;
        player.speed = player.originalSpeed;

        obstacles.clear();
        coins.clear();
        powerUps.clear();
        player.activePowerUps.clear();

        score = 0;
        coinsCollected = 0;
        gameOver = false;
        environmentOffset = 0.0f;
    }

    void DrawPowerUp(Vector3 position, PowerUpType type) {
        Texture2D texture;
        Color glowColor = WHITE;

        switch (type) {
        case PowerUpType::SPEED_BOOST:
            texture = speedBoostTexture;
            glowColor = ORANGE;
            break;
        case PowerUpType::INVINCIBILITY:
            texture = invincibilityTexture;
            glowColor = GOLD;
            break;
        case PowerUpType::MAGNET:
            texture = magnetTexture;
            glowColor = RED;
            break;
        case PowerUpType::DOUBLE_POINTS:
            texture = doublePointsTexture;
            glowColor = GREEN;
            break;
        }

        float scale = 1.0f + 0.2f * sin(GetTime() * 5.0f);
        Vector3 scaledSize = { scale, scale, scale };

        DrawCubeTexture(position, scaledSize, texture,RAYWHITE);
        DrawSphereWires(position, 0.7f, 8, 8, glowColor);
    }

    void DrawEnvironment() {
        Color envColor;
        Vector3 envSize;

        switch (menu.selectedLocation) {
        case 0: // City - здания
            envColor = GRAY;
            envSize = { 3.0f, 8.0f, 3.0f };
            for (int i = -5; i <= 5; i++) {
                DrawCube({ -8.0f, 4.0f, i * 10.0f + environmentOffset }, envSize.x, envSize.y, envSize.z, envColor);
                DrawCube({ 8.0f, 4.0f, i * 10.0f + environmentOffset }, envSize.x, envSize.y, envSize.z, envColor);
            }
            break;
        case 1: // Forest - деревья
            envColor = GREEN;
            envSize = { 2.0f, 6.0f, 2.0f };
            for (int i = -5; i <= 5; i++) {
                DrawCube({ -6.0f, 3.0f, i * 8.0f + environmentOffset }, envSize.x, envSize.y, envSize.z, envColor);
                DrawCube({ 6.0f, 3.0f, i * 8.0f + environmentOffset }, envSize.x, envSize.y, envSize.z, envColor);
            }
            break;
        case 2: // Desert - камни
            envColor = BROWN;
            envSize = { 4.0f, 4.0f, 4.0f };
            for (int i = -5; i <= 5; i++) {
                DrawCube({ -7.0f, 2.0f, i * 12.0f + environmentOffset }, envSize.x, envSize.y, envSize.z, envColor);
                DrawCube({ 7.0f, 2.0f, i * 12.0f + environmentOffset }, envSize.x, envSize.y, envSize.z, envColor);
            }
            break;
        case 3: // Winter - домики
            envColor = WHITE;
            envSize = { 4.0f, 5.0f, 4.0f };
            for (int i = -5; i <= 5; i++) {
                DrawCube({ -7.0f, 2.5f, i * 15.0f + environmentOffset }, envSize.x, envSize.y, envSize.z, envColor);
                DrawCube({ 7.0f, 2.5f, i * 15.0f + environmentOffset }, envSize.x, envSize.y, envSize.z, envColor);
            }
            break;
        }
    }

    void Draw3DWorld() {
        DrawPlane({ 0.0f, 0.0f, 0.0f }, { 50.0f, 100.0f }, groundColor);
        DrawEnvironment();

        for (int i = 0; i < 3; i++) {
            Color laneColor = (i == 1) ? GRAY : DARKGRAY;
            DrawCube({ lanePositions[i], 0.01f, 0.0f }, laneWidth, 0.02f, 100.0f, laneColor);
        }

        // Рисуем препятствия с текстурами
        for (auto& obstacle : obstacles) {
            DrawObstacle(obstacle);
        }

        for (auto& coin : coins) {
            if (coin.active) {
                DrawSphere(coin.position, 0.5f, GOLD);
            }
        }

        for (auto& powerUp : powerUps) {
            if (powerUp.active) {
                DrawPowerUp(powerUp.position, powerUp.type);
            }
        }

        DrawPlayer();

        if (HasPowerUp(PowerUpType::MAGNET)) {
            float magnetRadius = 2.0f + (shop.upgrades[2].level * 0.3f);
            DrawSphereWires(player.position, magnetRadius, 8, 8, BLUE);
        }
    }

    void DrawPlayer() {
        Texture2D characterTexture = GetCharacterTexture(player.characterType);

        // Всегда рисуем персонажа с текстурой
        if (characterTexture.id != 0) {
            DrawCubeTexture(player.position, player.size, characterTexture,RAYWHITE);
        }
        else {
            // Если текстура не загрузилась, рисуем цветным кубом
            Color playerColor;
            switch (player.characterType) {
            case 0: playerColor = RED; break;
            case 1: playerColor = BLACK; break;
            case 2: playerColor = BLUE; break;
            case 3: playerColor = PINK; break;
            }

            if (HasPowerUp(PowerUpType::INVINCIBILITY) && ((int)(GetTime() * 10) % 2 == 0)) {
                playerColor = GOLD;
            }

            DrawCube(player.position, player.size.x, player.size.y, player.size.z, playerColor);
        }

        // Всегда рисуем контуры
        DrawCubeWires(player.position, player.size.x, player.size.y, player.size.z, BLACK);
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
        ClearBackground(backgroundColor);

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
            BeginDrawing();
            ClearBackground(RAYWHITE);

            BeginMode3D(camera);
            BeginBlendMode(BLEND_ALPHA);
            
            Draw3DWorld();

            EndBlendMode();
            EndMode3D();

            DrawText(TextFormat("Score: %d", score), 10, 10, 20, BLACK);
            DrawText(TextFormat("Coins: %d", coinsCollected), 10, 40, 20, BLACK);
            DrawText(TextFormat("Lane: %d", player.lane + 1), 10, 70, 20, BLACK);
            DrawText(TextFormat("Location: %s", menu.locations[menu.selectedLocation].c_str()), 10, 100, 15, DARKGRAY);
            DrawText(TextFormat("Character: %s", menu.characters[player.characterType].c_str()), 10, 120, 15, DARKGRAY);

            int powerUpY = 150;
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

            DrawText("JUMP: SPACE/UP", 10, powerUpY, 15, DARKGREEN);
            DrawText("DUCK: DOWN", 10, powerUpY + 20, 15, DARKBLUE);
            DrawText("MOVE: LEFT/RIGHT", 10, powerUpY + 40, 15, DARKPURPLE);
            DrawText("MENU: M", 10, powerUpY + 60, 15, DARKBROWN);

            DrawText("Obstacles:", 10, powerUpY + 90, 15, BLACK);
            DrawText("▲ - Jump Over", 10, powerUpY + 110, 12, DARKGREEN);
            DrawText("▼ - Duck Under", 10, powerUpY + 125, 12, DARKBLUE);
            DrawText("✕ - Wall (Avoid)", 10, powerUpY + 140, 12, RED);
            DrawText("▬ - Low Barrier (Duck)", 10, powerUpY + 155, 12, ORANGE);

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

        DrawText("RUNNER 3D WITH CHARACTER TEXTURES", screenWidth / 2 - MeasureText("RUNNER 3D WITH CHARACTER TEXTURES", 40) / 2, 50, 40, YELLOW);
        DrawText(TextFormat("Total Coins: %d", shop.totalCoins), screenWidth / 2 - MeasureText(TextFormat("Total Coins: %d", shop.totalCoins), 30) / 2, 100, 30, GOLD);

        DrawText("SELECT LOCATION:", screenWidth / 2 - MeasureText("SELECT LOCATION:", 30) / 2, 150, 30, WHITE);
        for (int i = 0; i < (int)menu.locations.size(); i++) {
            Color color = (i == menu.selectedLocation) ? GREEN : WHITE;
            DrawText(menu.locations[i].c_str(), screenWidth / 2 - MeasureText(menu.locations[i].c_str(), 25) / 2, 200 + i * 40, 25, color);
        }

        DrawText("SELECT CHARACTER: (A/D to change)", screenWidth / 2 - MeasureText("SELECT CHARACTER: (A/D to change)", 30) / 2, 350, 30, WHITE);
        for (int i = 0; i < (int)menu.characters.size(); i++) {
            Color color = (i == menu.selectedCharacter) ? GREEN : WHITE;
            DrawText(menu.characters[i].c_str(), screenWidth / 2 - MeasureText(menu.characters[i].c_str(), 25) / 2, 400 + i * 40, 25, color);
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