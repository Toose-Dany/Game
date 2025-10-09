#include "raylib.h"
#include <vector>
#include <string>
#include <random>
#include <algorithm>

// Типы препятствий
enum class ObstacleType {
    JUMP_OVER,    // Можно перепрыгнуть
    DUCK_UNDER,   // Можно пригнуться
    WALL          // Нельзя пройти
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

class Game {
private:
    const int screenWidth = 1100;
    const int screenHeight = 800;

    Player player;
    std::vector<Obstacle> obstacles;
    std::vector<Coin> coins;

    float obstacleSpawnTimer;
    float coinSpawnTimer;
    const float obstacleSpawnInterval = 1.5f; // Вернули обычный интервал
    const float coinSpawnInterval = 2.0f;

    int score;
    int coinsCollected;
    bool gameOver;

    float laneWidth;
    float lanePositions[3];

    Camera3D camera;
    float gameSpeed;

    Menu menu;
    Color backgroundColor;
    Color groundColor;

    // Текстуры для препятствий
    Texture2D jumpObstacleTexture;
    Texture2D duckObstacleTexture;
    Texture2D wallObstacleTexture;

    bool texturesLoaded;
    float environmentOffset;

    // Модели кубов для текстур
    Model cubeModel;

public:
    Game() {
        InitWindow(screenWidth, screenHeight, "Runner 3D");

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

        // Инициализация 3D камеры
        camera.position = { 0.0f, 5.0f, 10.0f };
        camera.target = { player.position.x, player.position.y, player.position.z };
        camera.up = { 0.0f, 1.0f, 0.0f };
        camera.fovy = 45.0f;
        camera.projection = CAMERA_PERSPECTIVE;

        // Таймеры
        obstacleSpawnTimer = 0;
        coinSpawnTimer = 0;

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

        // Создаем модель куба
        cubeModel = LoadModelFromMesh(GenMeshCube(1.0f, 1.0f, 1.0f));

        // Загружаем текстуры
        LoadTextures();

        SetTargetFPS(60);
    }

    ~Game() {
        // Выгружаем текстуры и модель
        if (texturesLoaded) {
            UnloadTexture(jumpObstacleTexture);
            UnloadTexture(duckObstacleTexture);
            UnloadTexture(wallObstacleTexture);
        }
        UnloadModel(cubeModel);
        CloseWindow();
    }

    void Run() {
        while (!WindowShouldClose()) {
            Update();
            Draw();
        }
    }

private:
    void LoadTextures() {
        // Создаем текстуры программно
        jumpObstacleTexture = CreateJumpObstacleTexture();
        duckObstacleTexture = CreateDuckObstacleTexture();
        wallObstacleTexture = CreateWallObstacleTexture();
        texturesLoaded = true;
    }

    Texture2D CreateJumpObstacleTexture() {
        // Текстура для прыжка - высокая с стрелкой вверх
        Image image = GenImageColor(64, 128, BLANK);

        // Основной цвет - темно-серый
        for (int y = 0; y < 128; y++) {
            for (int x = 0; x < 64; x++) {
                Color color = DARKGRAY;

                // Стрелка вверх в центре
                if (y < 40 && abs(x - 32) <= 10 - y / 4) {
                    color = YELLOW;
                }

                // Контуры
                if (x == 0 || x == 63 || y == 0 || y == 127) {
                    color = BLACK;
                }

                ImageDrawPixel(&image, x, y, color);
            }
        }

        Texture2D texture = LoadTextureFromImage(image);
        UnloadImage(image);
        return texture;
    }

    Texture2D CreateDuckObstacleTexture() {
        // Текстура для приседания - низкая с большой стрелкой вниз
        Image image = GenImageColor(64, 64, BLANK);

        // Основной цвет - коричневый
        for (int y = 0; y < 64; y++) {
            for (int x = 0; x < 64; x++) {
                Color color = BROWN;

                // Большая стрелка вниз по центру
                int arrowTop = 15;
                int arrowBottom = 45;
                int centerX = 32;

                // Рисуем стрелку вниз
                if (y >= arrowTop && y <= arrowBottom) {
                    int width = 8 + (y - arrowTop) / 2; // Стрелка расширяется к низу
                    if (abs(x - centerX) <= width) {
                        color = YELLOW;
                    }
                }

                // Наконечник стрелки
                if (y > arrowBottom && y <= 55) {
                    int tipWidth = 12 - (y - arrowBottom);
                    if (tipWidth > 0 && abs(x - centerX) <= tipWidth) {
                        color = YELLOW;
                    }
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

    Texture2D CreateWallObstacleTexture() {
        // Текстура для стены - высокая с крестом
        Image image = GenImageColor(64, 128, BLANK);

        // Основной цвет - темно-красный
        for (int y = 0; y < 128; y++) {
            for (int x = 0; x < 64; x++) {
                Color color = MAROON;

                // Красный крест
                if (abs(x - 32) <= 5 || abs(y - 64) <= 5) {
                    if (abs(x - 32) <= 5 && abs(y - 64) <= 5) {
                        color = RED;
                    }
                }

                // Контуры
                if (x == 0 || x == 63 || y == 0 || y == 127) {
                    color = BLACK;
                }

                ImageDrawPixel(&image, x, y, color);
            }
        }

        Texture2D texture = LoadTextureFromImage(image);
        UnloadImage(image);
        return texture;
    }

    void Update() {
        if (menu.isActive) {
            UpdateMenu();
            return;
        }

        if (gameOver) {
            if (IsKeyPressed(KEY_R)) {
                ResetGame();
            }
            if (IsKeyPressed(KEY_M)) {
                menu.isActive = true;
            }
            return;
        }

        HandleInput();
        UpdatePlayer();
        UpdateObstacles();
        UpdateCoins();
        UpdateCamera();
        CheckCollisions();

        // Обновляем параллакс-эффект
        environmentOffset += gameSpeed * 0.3f * GetFrameTime();
        if (environmentOffset > 50.0f) environmentOffset = 0.0f;

        // Увеличиваем счет
        score += 1;
    }

    void UpdateMenu() {
        int oldLocation = menu.selectedLocation;

        // Выбор локации - UP/DOWN
        if (IsKeyPressed(KEY_UP)) {
            if (menu.selectedLocation > 0) menu.selectedLocation--;
        }
        if (IsKeyPressed(KEY_DOWN)) {
            if (menu.selectedLocation < (int)menu.locations.size() - 1) menu.selectedLocation++;
        }

        // Выбор персонажа - A/D
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

        // Если локация изменилась, применяем настройки
        if (oldLocation != menu.selectedLocation) {
            ApplyLocationSettings();
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

            // Проверка приземления
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
            // Случайный выбор: одиночное препятствие или группа
            if (GetRandomValue(0, 100) < 40) { // 40% chance для группы
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

                // Деактивация прошедших препятствий
                if (obstacle.position.z > 10.0f) {
                    obstacle.active = false;
                }
            }
        }

        // Удаление неактивных препятствий
        obstacles.erase(std::remove_if(obstacles.begin(), obstacles.end(),
            [](const Obstacle& o) { return !o.active; }), obstacles.end());
    }

    void SpawnSingleObstacle() {
        Obstacle obstacle;
        obstacle.lane = GetRandomValue(0, 2);

        // Случайный выбор типа препятствия
        int obstacleType = GetRandomValue(0, 2);
        switch (obstacleType) {
        case 0:
            obstacle.type = ObstacleType::JUMP_OVER;
            obstacle.size = { 1.0f, 2.0f, 1.0f }; // Высокое препятствие
            obstacle.color = DARKGRAY;
            obstacle.texture = jumpObstacleTexture;
            break;
        case 1:
            obstacle.type = ObstacleType::DUCK_UNDER;
            obstacle.size = { 1.0f, 1.0f, 1.0f }; // Низкое препятствие
            obstacle.color = BROWN;
            obstacle.texture = duckObstacleTexture;
            break;
        case 2:
            obstacle.type = ObstacleType::WALL;
            obstacle.size = { 1.0f, 3.0f, 1.0f }; // Очень высокое препятствие
            obstacle.color = MAROON;
            obstacle.texture = wallObstacleTexture;
            break;
        }

        obstacle.position = { lanePositions[obstacle.lane], obstacle.size.y / 2, -20.0f };
        obstacle.active = true;
        obstacle.speed = gameSpeed + (static_cast<float>(score) / 1000.0f);

        obstacles.push_back(obstacle);
    }

    void SpawnObstacleGroup() {
        // Создаем препятствия на всех трех дорожках
        bool hasPassableLane = false;
        std::vector<ObstacleType> laneTypes(3);

        // Гарантируем, что хотя бы одна дорожка будет проходимой
        do {
            for (int lane = 0; lane < 3; lane++) {
                int obstacleType = GetRandomValue(0, 2);
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
                }
            }

            // Проверяем, есть ли хотя бы одна проходимая дорожка
            for (int lane = 0; lane < 3; lane++) {
                if (laneTypes[lane] != ObstacleType::WALL) {
                    hasPassableLane = true;
                    break;
                }
            }
        } while (!hasPassableLane); // Повторяем, пока не получим валидную комбинацию

        // Создаем препятствия для каждой дорожки
        for (int lane = 0; lane < 3; lane++) {
            Obstacle obstacle;
            obstacle.lane = lane;
            obstacle.type = laneTypes[lane];

            switch (obstacle.type) {
            case ObstacleType::JUMP_OVER:
                obstacle.size = { 1.0f, 2.0f, 1.0f }; // Высокое препятствие
                obstacle.color = DARKGRAY;
                obstacle.texture = jumpObstacleTexture;
                break;
            case ObstacleType::DUCK_UNDER:
                obstacle.size = { 1.0f, 1.0f, 1.0f }; // Низкое препятствие
                obstacle.color = BROWN;
                obstacle.texture = duckObstacleTexture;
                break;
            case ObstacleType::WALL:
                obstacle.size = { 1.0f, 3.0f, 1.0f }; // Очень высокое препятствие
                obstacle.color = MAROON;
                obstacle.texture = wallObstacleTexture;
                break;
            }

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

    void UpdateCamera() {
        camera.target = { player.position.x, player.position.y, player.position.z };
        camera.position = { player.position.x, player.position.y + 3.0f, player.position.z + 8.0f };
    }

    void CheckCollisions() {
        BoundingBox playerBox = {
            { player.position.x - player.size.x / 2, player.position.y - player.size.y / 2, player.position.z - player.size.z / 2 },
            { player.position.x + player.size.x / 2, player.position.y + player.size.y / 2, player.position.z + player.size.z / 2 }
        };

        // Проверка столкновений с препятствиями
        for (auto& obstacle : obstacles) {
            if (obstacle.active && player.lane == obstacle.lane) {
                BoundingBox obstacleBox = {
                    { obstacle.position.x - obstacle.size.x / 2, obstacle.position.y - obstacle.size.y / 2, obstacle.position.z - obstacle.size.z / 2 },
                    { obstacle.position.x + obstacle.size.x / 2, obstacle.position.y + obstacle.size.y / 2, obstacle.position.z + obstacle.size.z / 2 }
                };

                if (CheckCollisionBoxes(playerBox, obstacleBox)) {
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
                    }

                    if (!canAvoid) {
                        gameOver = true;
                        return;
                    }
                }
            }
        }

        // Проверка сбора монет
        for (auto& coin : coins) {
            if (coin.active) {
                if (CheckCollisionBoxSphere(playerBox, coin.position, 0.5f)) {
                    coin.active = false;
                    coinsCollected++;
                    score += 100;
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

        obstacles.clear();
        coins.clear();

        score = 0;
        coinsCollected = 0;
        gameOver = false;
        environmentOffset = 0.0f;
    }

    void DrawTexturedCube(Vector3 position, Vector3 size, Texture2D texture) {
        // Рисуем куб с текстурой используя модель
        Model model = cubeModel;

        // Применяем текстуру к модели
        model.materials[0].maps[MATERIAL_MAP_DIFFUSE].texture = texture;

        // Рисуем модель с масштабированием
        DrawModelEx(model, position, { 0, 1, 0 }, 0.0f, size, WHITE);
    }

    void DrawEnvironment() {
        // Рисуем окружение в зависимости от локации простыми кубами
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
        // Рисуем землю
        DrawPlane({ 0.0f, 0.0f, 0.0f }, { 50.0f, 100.0f }, groundColor);

        // Рисуем окружение
        DrawEnvironment();

        // Рисуем дорожки
        for (int i = 0; i < 3; i++) {
            Color laneColor = (i == 1) ? GRAY : DARKGRAY;
            DrawCube({ lanePositions[i], 0.01f, 0.0f }, laneWidth, 0.02f, 100.0f, laneColor);
        }

        // Рисуем препятствия с текстурами
        for (auto& obstacle : obstacles) {
            if (obstacle.active && texturesLoaded) {
                // Рисуем куб с текстурой
                DrawTexturedCube(obstacle.position, obstacle.size, obstacle.texture);

                // Дополнительно рисуем контур для лучшей видимости
                DrawCubeWires(obstacle.position, obstacle.size.x, obstacle.size.y, obstacle.size.z, BLACK);
            }
            else if (obstacle.active) {
                // Fallback: рисуем обычный куб если текстуры не загружены
                DrawCube(obstacle.position, obstacle.size.x, obstacle.size.y, obstacle.size.z, obstacle.color);
                DrawCubeWires(obstacle.position, obstacle.size.x, obstacle.size.y, obstacle.size.z, BLACK);
            }
        }

        // Рисуем монеты
        for (auto& coin : coins) {
            if (coin.active) {
                DrawSphere(coin.position, 0.5f, GOLD);
            }
        }

        // Рисуем игрока с цветом в зависимости от персонажа
        Color playerColor;
        switch (player.characterType) {
        case 0: playerColor = RED; break;
        case 1: playerColor = BLACK; break;
        case 2: playerColor = BLUE; break;
        case 3: playerColor = PINK; break;
        }
        DrawCube(player.position, player.size.x, player.size.y, player.size.z, playerColor);
    }

    void Draw() {
        BeginDrawing();
        ClearBackground(backgroundColor);

        if (menu.isActive) {
            DrawMenu();
        }
        else if (gameOver) {
            // Экран Game Over
            DrawRectangle(0, 0, screenWidth, screenHeight, Fade(BLACK, 0.5f));
            DrawText("GAME OVER", screenWidth / 2 - MeasureText("GAME OVER", 40) / 2, screenHeight / 2 - 50, 40, RED);
            DrawText(TextFormat("Final Score: %d", score), screenWidth / 2 - MeasureText(TextFormat("Final Score: %d", score), 20) / 2, screenHeight / 2, 20, WHITE);
            DrawText("Press R to restart", screenWidth / 2 - MeasureText("Press R to restart", 20) / 2, screenHeight / 2 + 30, 20, WHITE);
            DrawText("Press M for menu", screenWidth / 2 - MeasureText("Press M for menu", 20) / 2, screenHeight / 2 + 60, 20, WHITE);
        }
        else {
            BeginMode3D(camera);

            Draw3DWorld();

            EndMode3D();

            // Рисуем UI
            DrawText(TextFormat("Score: %d", score), 10, 10, 20, BLACK);
            DrawText(TextFormat("Coins: %d", coinsCollected), 10, 40, 20, BLACK);
            DrawText(TextFormat("Lane: %d", player.lane + 1), 10, 70, 20, BLACK);
            DrawText(TextFormat("Location: %s", menu.locations[menu.selectedLocation].c_str()), 10, 100, 15, DARKGRAY);
            DrawText(TextFormat("Character: %s", menu.characters[player.characterType].c_str()), 10, 120, 15, DARKGRAY);

            // Подсказки по управлению
            DrawText("JUMP: SPACE/UP", 10, 150, 15, DARKGREEN);
            DrawText("DUCK: DOWN", 10, 170, 15, DARKBLUE);
            DrawText("MOVE: LEFT/RIGHT", 10, 190, 15, DARKPURPLE);
            DrawText("MENU: M", 10, 210, 15, DARKBROWN);

            // Подсказки для препятствий
            DrawText("Obstacles:", 10, 240, 15, BLACK);
            DrawText("▲ - Jump Over", 10, 260, 12, DARKGREEN);
            DrawText("▼ - Duck Under", 10, 275, 12, DARKBLUE);
            DrawText("✕ - Wall (Avoid)", 10, 290, 12, RED);

            // Информация о случайных препятствиях
            DrawText("Obstacles spawn randomly!", 10, 320, 15, DARKPURPLE);
            DrawText("Sometimes in groups of 3", 10, 340, 12, DARKGREEN);
        }

        EndDrawing();
    }

    void DrawMenu() {
        ClearBackground(DARKBLUE);

        // Заголовок
        DrawText("RUNNER 3D", screenWidth / 2 - MeasureText("RUNNER 3D", 40) / 2, 50, 40, YELLOW);

        // Выбор локации
        DrawText("SELECT LOCATION:", screenWidth / 2 - MeasureText("SELECT LOCATION:", 30) / 2, 150, 30, WHITE);
        for (int i = 0; i < (int)menu.locations.size(); i++) {
            Color color = (i == menu.selectedLocation) ? GREEN : WHITE;
            DrawText(menu.locations[i].c_str(), screenWidth / 2 - MeasureText(menu.locations[i].c_str(), 25) / 2, 200 + i * 40, 25, color);
        }

        // Выбор персонажа
        DrawText("SELECT CHARACTER: (A/D to change)", screenWidth / 2 - MeasureText("SELECT CHARACTER: (A/D to change)", 30) / 2, 350, 30, WHITE);
        for (int i = 0; i < (int)menu.characters.size(); i++) {
            Color color = (i == menu.selectedCharacter) ? GREEN : WHITE;
            DrawText(menu.characters[i].c_str(), screenWidth / 2 - MeasureText(menu.characters[i].c_str(), 25) / 2, 400 + i * 40, 25, color);
        }

        // Инструкции
        DrawText("PRESS ENTER TO START", screenWidth / 2 - MeasureText("PRESS ENTER TO START", 30) / 2, 550, 30, YELLOW);
        DrawText("USE ARROWS TO NAVIGATE", screenWidth / 2 - MeasureText("USE ARROWS TO NAVIGATE", 20) / 2, 600, 20, LIGHTGRAY);
    }
};

int main() {
    Game game;
    game.Run();
    return 0;
}