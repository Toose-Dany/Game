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
    Vector2 position;
    Vector2 size;
    Color color;
    float speed;
    int lane; // 0 - левая, 1 - средняя, 2 - правая
    bool isJumping;
    bool isDucking;
    float jumpVelocity;
    float gravity;
    int characterType; // 0 - стандартный, 1 - ниндзя, 2 - робот, 3 - девушка
};

// Структура для препятствий
struct Obstacle {
    Vector2 position;
    Vector2 size;
    Color color;
    int lane;
    bool active;
    float speed;
    ObstacleType type;
};

// Структура для монет
struct Coin {
    Vector2 position;
    float radius;
    Color color;
    int lane;
    bool active;
    float speed;
};

// Структура для следов
struct Trail {
    Vector2 position;
    Vector2 size;
    Color color;
    float lifetime;
    float maxLifetime;
};

// Структура для меню
struct Menu {
    bool isActive;
    int selectedLocation;
    int selectedMusic;
    int selectedCharacter;
    std::vector<std::string> locations;
    std::vector<std::string> musicTracks;
    std::vector<std::string> characters;

    Menu() {
        isActive = true;
        selectedLocation = 0;
        selectedMusic = 0;
        selectedCharacter = 0;
        locations = { "City", "Forest", "Desert", "Winter" };
        musicTracks = { "Track 1", "Track 2", "Track 3", "None" };
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
    std::vector<Trail> trails;

    float obstacleSpawnTimer;
    float coinSpawnTimer;
    float trailSpawnTimer;
    const float obstacleSpawnInterval = 1.5f;
    const float coinSpawnInterval = 2.0f;
    const float trailSpawnInterval = 0.1f;

    int score;
    int coinsCollected;
    bool gameOver;

    float laneWidth;
    float lanePositions[3]; // x позиции для трех дорожек

    Camera2D camera;
    float gameSpeed;

    // Новые переменные для меню и локаций
    Menu menu;
    Color backgroundColor;
    Color groundColor;
    Music currentMusic;
    bool musicPlaying;

    // Текстуры для краев экрана
    Texture2D leftEdgeTexture;
    Texture2D rightEdgeTexture;
    int edgeWidth; // Ширина краев

    // Текстуры для персонажей
    Texture2D characterDefault;
    Texture2D characterNinja;
    Texture2D characterRobot;
    Texture2D characterGirl;

    // Параллакс-эффект для боковых текстур
    float leftEdgeOffset;
    float rightEdgeOffset;

public:
    Game() {
        InitWindow(screenWidth, screenHeight, "Subway Surfers Clone");
        InitAudioDevice();

        // Настройка дорожек
        laneWidth = static_cast<float>(screenWidth) / 3.0f;
        lanePositions[0] = laneWidth * 0.5f;
        lanePositions[1] = laneWidth * 1.5f;
        lanePositions[2] = laneWidth * 2.5f;

        // Инициализация игрока
        player.size = { 40.0f, 80.0f };
        player.position = { lanePositions[1], static_cast<float>(screenHeight) - 150.0f };
        player.color = RED;
        player.speed = 5.0f;
        player.lane = 1; // Начинаем на средней дорожке
        player.isJumping = false;
        player.isDucking = false;
        player.jumpVelocity = 0;
        player.gravity = 0.5f;
        player.characterType = 0; // Стандартный персонаж

        // Инициализация камеры
        camera = { 0 };
        camera.target = { player.position.x, player.position.y };
        camera.offset = { screenWidth / 2.0f, screenHeight / 2.0f };
        camera.rotation = 0.0f;
        camera.zoom = 1.0f;

        // Таймеры
        obstacleSpawnTimer = 0;
        coinSpawnTimer = 0;
        trailSpawnTimer = 0;

        // Счет
        score = 0;
        coinsCollected = 0;
        gameOver = false;

        gameSpeed = 5.0f;

        // Настройки по умолчанию для локации
        backgroundColor = SKYBLUE;
        groundColor = GREEN;
        musicPlaying = false;

        // Инициализируем текстуры краев
        edgeWidth = 200; // Увеличили ширину краев для лучшего эффекта
        leftEdgeTexture = { 0 };
        rightEdgeTexture = { 0 };
        leftEdgeOffset = 0;
        rightEdgeOffset = 0;

        // Инициализируем текстуры персонажей
        characterDefault = { 0 };
        characterNinja = { 0 };
        characterRobot = { 0 };
        characterGirl = { 0 };

        // Загружаем все текстуры
        LoadAllTextures();

        SetTargetFPS(60);
    }

    ~Game() {
        if (musicPlaying) {
            StopMusicStream(currentMusic);
            UnloadMusicStream(currentMusic);
        }

        // Выгружаем текстуры
        if (leftEdgeTexture.id != 0) UnloadTexture(leftEdgeTexture);
        if (rightEdgeTexture.id != 0) UnloadTexture(rightEdgeTexture);
        if (characterDefault.id != 0) UnloadTexture(characterDefault);
        if (characterNinja.id != 0) UnloadTexture(characterNinja);
        if (characterRobot.id != 0) UnloadTexture(characterRobot);
        if (characterGirl.id != 0) UnloadTexture(characterGirl);

        CloseAudioDevice();
        CloseWindow();
    }

    void Run() {
        while (!WindowShouldClose()) {
            Update();
            Draw();
        }
    }

private:
    void LoadAllTextures() {
        LoadLocationTextures();
        LoadCharacterTextures();
    }

    void LoadLocationTextures() {
        // Выгружаем старые текстуры если они загружены
        if (leftEdgeTexture.id != 0) UnloadTexture(leftEdgeTexture);
        if (rightEdgeTexture.id != 0) UnloadTexture(rightEdgeTexture);

        // Пытаемся загрузить текстуры для выбранной локации
        switch (menu.selectedLocation) {
        case 0: // City
            leftEdgeTexture = LoadTexture("resources/city_left.png");
            rightEdgeTexture = LoadTexture("resources/city_right.png");
            break;
        case 1: // Forest
            leftEdgeTexture = LoadTexture("resources/forest_left.png");
            rightEdgeTexture = LoadTexture("resources/forest_right.png");
            break;
        case 2: // Desert
            leftEdgeTexture = LoadTexture("resources/desert_left.png");
            rightEdgeTexture = LoadTexture("resources/desert_right.png");
            break;
        case 3: // Winter
            leftEdgeTexture = LoadTexture("resources/winter_left.png");
            rightEdgeTexture = LoadTexture("resources/winter_right.png");
            break;
        }

        // Если текстуры не загрузились, создаем placeholder
        if (leftEdgeTexture.id == 0) {
            CreatePlaceholderTexture(true);
        }
        if (rightEdgeTexture.id == 0) {
            CreatePlaceholderTexture(false);
        }
    }

    void LoadCharacterTextures() {
        // Загружаем текстуры персонажей
        characterDefault = LoadTexture("resources/character_default.png");
        characterNinja = LoadTexture("resources/character_ninja.png");
        characterRobot = LoadTexture("resources/character_robot.png");
        characterGirl = LoadTexture("resources/character_girl.png");

        // Если текстуры не загрузились, создаем placeholder
        if (characterDefault.id == 0) CreateCharacterPlaceholder(0);
        if (characterNinja.id == 0) CreateCharacterPlaceholder(1);
        if (characterRobot.id == 0) CreateCharacterPlaceholder(2);
        if (characterGirl.id == 0) CreateCharacterPlaceholder(3);
    }

    void CreatePlaceholderTexture(bool isLeft) {
        // Создаем placeholder текстуру если файл не найден
        Image image = GenImageColor(edgeWidth, screenHeight * 2, BLANK); // Увеличили высоту для параллакса

        for (int y = 0; y < screenHeight * 2; y++) {
            for (int x = 0; x < edgeWidth; x++) {
                Color color;

                switch (menu.selectedLocation) {
                case 0: // City - серые здания с окнами
                    color = {
                        (unsigned char)(80 + x * 0.5f),
                        (unsigned char)(80 + x * 0.5f),
                        (unsigned char)(100 + x * 0.3f),
                        255
                    };
                    // Окна в зданиях
                    if ((x / 25) % 2 == 0 && (y / 35) % 3 == 0 && x > 10 && x < edgeWidth - 10) {
                        if ((x + y) % 7 < 4) {
                            color = YELLOW;
                        }
                        else {
                            color = { 40, 40, 60, 255 };
                        }
                    }
                    // Контуры зданий
                    if (x == edgeWidth - 1 || x == 0 || (x % 40 == 0 && y > 100)) {
                        color = DARKGRAY;
                    }
                    break;
                case 1: // Forest - зеленые деревья
                    color = {
                        (unsigned char)(20 + x * 0.2f),
                        (unsigned char)(60 + x * 0.4f),
                        (unsigned char)(20 + x * 0.1f),
                        255
                    };
                    // Текстура коры деревьев
                    if (x % 15 < 3) {
                        color = { 60, 40, 20, 255 };
                    }
                    // Листья
                    if ((x + y * 2) % 50 < 25 && x > 30) {
                        color = { 30, 100, 30, 255 };
                    }
                    break;
                case 2: // Desert - песочные дюны
                {
                    color = {
                        (unsigned char)(200 + x * 0.1f),
                        (unsigned char)(170 + x * 0.05f),
                        (unsigned char)(120 + x * 0.05f),
                        255
                    };
                    // Волны дюн
                    int duneHeight = (int)(sin(y * 0.02f) * 10 + cos(x * 0.05f) * 5);
                    if (x > edgeWidth - duneHeight - 10) {
                        color = { 190, 160, 110, 255 };
                    }
                    // Кактусы
                    if (x % 60 == 30 && y % 200 > 150 && y % 200 < 180) {
                        color = { 30, 100, 30, 255 };
                    }
                }
                break;
                case 3: // Winter - заснеженные здания
                    color = {
                        (unsigned char)(180 - x * 0.1f),
                        (unsigned char)(200 - x * 0.05f),
                        (unsigned char)(220 - x * 0.05f),
                        255
                    };
                    // Снег на крышах
                    if (y % 150 < 20) {
                        color = WHITE;
                    }
                    // Окна
                    if ((x / 20) % 2 == 1 && (y / 40) % 3 == 1 && x > 15 && x < edgeWidth - 15) {
                        color = { 200, 200, 100, 255 };
                    }
                    break;
                }

                ImageDrawPixel(&image, x, y, color);
            }
        }

        if (isLeft) {
            leftEdgeTexture = LoadTextureFromImage(image);
        }
        else {
            rightEdgeTexture = LoadTextureFromImage(image);
        }

        UnloadImage(image);
    }

    void CreateCharacterPlaceholder(int characterType) {
        Image image = GenImageColor(50, 80, BLANK);

        for (int y = 0; y < 80; y++) {
            for (int x = 0; x < 50; x++) {
                Color color;

                switch (characterType) {
                case 0: // Default - красный
                    color = RED;
                    if (y < 20) color = YELLOW; // голова
                    break;
                case 1: // Ninja - черный
                    color = BLACK;
                    if (y < 15) color = DARKGRAY; // маска
                    break;
                case 2: // Robot - серый
                    color = GRAY;
                    if (y < 20) color = LIGHTGRAY; // голова
                    if (x > 15 && x < 35 && y > 25 && y < 45) color = BLUE; // экран
                    break;
                case 3: // Girl - розовый
                    color = PINK;
                    if (y < 20) color = Color{ 255, 220, 177, 255 }; // лицо
                    if (x > 10 && x < 40 && y > 30 && y < 50) color = PURPLE; // платье
                    break;
                }

                ImageDrawPixel(&image, x, y, color);
            }
        }

        switch (characterType) {
        case 0: characterDefault = LoadTextureFromImage(image); break;
        case 1: characterNinja = LoadTextureFromImage(image); break;
        case 2: characterRobot = LoadTextureFromImage(image); break;
        case 3: characterGirl = LoadTextureFromImage(image); break;
        }

        UnloadImage(image);
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
        UpdateTrails();
        UpdateCamera();
        CheckCollisions();

        // Обновляем параллакс-смещение для боковых текстур
        leftEdgeOffset += gameSpeed * 0.3f; // Боковые текстуры движутся медленнее
        rightEdgeOffset += gameSpeed * 0.3f;

        // Зацикливаем смещение
        if (leftEdgeOffset > screenHeight * 2) leftEdgeOffset = 0;
        if (rightEdgeOffset > screenHeight * 2) rightEdgeOffset = 0;

        if (musicPlaying) {
            UpdateMusicStream(currentMusic);
        }

        // Увеличиваем счет
        score += 1;
    }

    void UpdateMenu() {
        int oldLocation = menu.selectedLocation;
        int oldCharacter = menu.selectedCharacter;

        // Выбор локации - UP/DOWN
        if (IsKeyPressed(KEY_UP)) {
            if (menu.selectedLocation > 0) menu.selectedLocation--;
        }
        if (IsKeyPressed(KEY_DOWN)) {
            if (menu.selectedLocation < (int)menu.locations.size() - 1) menu.selectedLocation++;
        }

        // Выбор музыки - LEFT/RIGHT
        if (IsKeyPressed(KEY_LEFT)) {
            if (menu.selectedMusic > 0) menu.selectedMusic--;
        }
        if (IsKeyPressed(KEY_RIGHT)) {
            if (menu.selectedMusic < (int)menu.musicTracks.size() - 1) menu.selectedMusic++;
        }

        // Выбор персонажа - A/D или Q/E
        if (IsKeyPressed(KEY_A) || IsKeyPressed(KEY_Q)) {
            if (menu.selectedCharacter > 0) menu.selectedCharacter--;
        }
        if (IsKeyPressed(KEY_D) || IsKeyPressed(KEY_E)) {
            if (menu.selectedCharacter < (int)menu.characters.size() - 1) menu.selectedCharacter++;
        }

        if (IsKeyPressed(KEY_ENTER)) {
            ApplyLocationSettings();
            player.characterType = menu.selectedCharacter;

            if (menu.selectedMusic < (int)menu.musicTracks.size() - 1) {
                LoadAndPlayMusic();
            }
            else {
                if (musicPlaying) {
                    StopMusicStream(currentMusic);
                    UnloadMusicStream(currentMusic);
                }
                musicPlaying = false;
            }
            menu.isActive = false;
        }

        // Если локация изменилась, загружаем новые текстуры
        if (oldLocation != menu.selectedLocation) {
            LoadLocationTextures();
        }

        // Если персонаж изменился, обновляем
        if (oldCharacter != menu.selectedCharacter) {
            // Текстуры уже загружены, просто меняем тип
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
        LoadLocationTextures();
    }

    void LoadAndPlayMusic() {
        if (musicPlaying) {
            StopMusicStream(currentMusic);
            UnloadMusicStream(currentMusic);
        }

        // Загружаем музыку в зависимости от выбора
        switch (menu.selectedMusic) {
        case 0:
            currentMusic = LoadMusicStream("resources/music_track1.mp3");
            break;
        case 1:
            currentMusic = LoadMusicStream("resources/music_track2.mp3");
            break;
        case 2:
            currentMusic = LoadMusicStream("resources/music_track3.mp3");
            break;
        }

        if (currentMusic.ctxData != NULL) {
            PlayMusicStream(currentMusic);
            musicPlaying = true;
        }
        else {
            musicPlaying = false;
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
            player.jumpVelocity = -15.0f;
        }

        // Приседание
        if (IsKeyDown(KEY_DOWN) && !player.isJumping) {
            player.isDucking = true;
            player.size.y = 40.0f;
            player.position.y = static_cast<float>(screenHeight) - 110.0f;
        }
        else {
            player.isDucking = false;
            player.size.y = 80.0f;
            if (!player.isJumping) {
                player.position.y = static_cast<float>(screenHeight) - 150.0f;
            }
        }
    }

    void UpdatePlayer() {
        // Обновление прыжка
        if (player.isJumping) {
            player.position.y += player.jumpVelocity;
            player.jumpVelocity += player.gravity;

            // Проверка приземления
            if (player.position.y >= static_cast<float>(screenHeight) - 150.0f) {
                player.position.y = static_cast<float>(screenHeight) - 150.0f;
                player.isJumping = false;
                player.jumpVelocity = 0;
            }
        }
    }

    void UpdateObstacles() {
        // Спавн препятствий СВЕРХУ
        obstacleSpawnTimer += GetFrameTime();
        if (obstacleSpawnTimer >= obstacleSpawnInterval) {
            SpawnObstacle();
            obstacleSpawnTimer = 0;
        }

        // Обновление позиций препятствий (движутся ВНИЗ)
        for (auto& obstacle : obstacles) {
            if (obstacle.active) {
                obstacle.position.y += obstacle.speed;

                // Создание следов от препятствий
                if (GetRandomValue(0, 100) < 10) {
                    Trail trail;
                    trail.position = { obstacle.position.x, obstacle.position.y + obstacle.size.y / 2 };
                    trail.size = { obstacle.size.x * 0.8f, 5.0f };
                    trail.color = Fade(obstacle.color, 0.5f);
                    trail.lifetime = 0.0f;
                    trail.maxLifetime = 1.0f;
                    trails.push_back(trail);
                }

                // Деактивация вышедших за экран препятствий (СНИЗУ)
                if (obstacle.position.y > screenHeight + 100) {
                    obstacle.active = false;
                }
            }
        }

        // Удаление неактивных препятствий
        obstacles.erase(std::remove_if(obstacles.begin(), obstacles.end(),
            [](const Obstacle& o) { return !o.active; }), obstacles.end());
    }

    void UpdateCoins() {
        // Спавн монет СВЕРХУ
        coinSpawnTimer += GetFrameTime();
        if (coinSpawnTimer >= coinSpawnInterval) {
            SpawnCoin();
            coinSpawnTimer = 0;
        }

        // Обновление позиций монет (движутся ВНИЗ)
        for (auto& coin : coins) {
            if (coin.active) {
                coin.position.y += coin.speed;

                // Деактивация вышедших за экран монет (СНИЗУ)
                if (coin.position.y > screenHeight + 100) {
                    coin.active = false;
                }
            }
        }

        // Удаление неактивных монет
        coins.erase(std::remove_if(coins.begin(), coins.end(),
            [](const Coin& c) { return !c.active; }), coins.end());
    }

    void UpdateTrails() {
        // Создание следов от игрока
        trailSpawnTimer += GetFrameTime();
        if (trailSpawnTimer >= trailSpawnInterval && !player.isJumping) {
            Trail trail;
            trail.position = { player.position.x, player.position.y + player.size.y / 2 - 10 };
            trail.size = { player.size.x * 0.6f, 8.0f };

            // Цвет следа зависит от персонажа
            switch (player.characterType) {
            case 0: trail.color = Fade(RED, 0.3f); break;
            case 1: trail.color = Fade(BLACK, 0.3f); break;
            case 2: trail.color = Fade(BLUE, 0.3f); break;
            case 3: trail.color = Fade(PINK, 0.3f); break;
            }

            trail.lifetime = 0.0f;
            trail.maxLifetime = 2.0f;
            trails.push_back(trail);
            trailSpawnTimer = 0;
        }

        // Обновление времени жизни следов
        for (auto& trail : trails) {
            trail.lifetime += GetFrameTime();
        }

        // Удаление старых следов
        trails.erase(std::remove_if(trails.begin(), trails.end(),
            [](const Trail& t) { return t.lifetime >= t.maxLifetime; }), trails.end());
    }

    void UpdateCamera() {
        // Камера следует за игроком по вертикали
        float targetY = player.position.y - 200.0f;

        // Плавное движение камеры
        camera.target.y += (targetY - camera.target.y) * 0.1f;
        camera.target.x = screenWidth / 2.0f;
    }

    void SpawnObstacle() {
        Obstacle obstacle;
        obstacle.lane = GetRandomValue(0, 2);

        // Случайный выбор типа препятствия
        int obstacleType = GetRandomValue(0, 2);
        switch (obstacleType) {
        case 0:
            obstacle.type = ObstacleType::JUMP_OVER;
            obstacle.size = { 50.0f, 80.0f };
            obstacle.color = DARKGRAY;
            break;
        case 1:
            obstacle.type = ObstacleType::DUCK_UNDER;
            obstacle.size = { 70.0f, 40.0f };
            obstacle.color = BROWN;
            break;
        case 2:
            obstacle.type = ObstacleType::WALL;
            obstacle.size = { 60.0f, 120.0f };
            obstacle.color = MAROON;
            break;
        }

        obstacle.position = {
            lanePositions[obstacle.lane],
            -100.0f
        };
        obstacle.active = true;
        obstacle.speed = gameSpeed + (static_cast<float>(score) / 1000.0f);

        obstacles.push_back(obstacle);
    }

    void SpawnCoin() {
        Coin coin;
        coin.radius = 15.0f;
        coin.lane = GetRandomValue(0, 2);
        coin.position = {
            lanePositions[coin.lane],
            -50.0f
        };
        coin.color = GOLD;
        coin.active = true;
        coin.speed = gameSpeed;

        coins.push_back(coin);
    }

    void CheckCollisions() {
        Rectangle playerRect = {
            player.position.x - player.size.x / 2,
            player.position.y - player.size.y,
            player.size.x,
            player.size.y
        };

        // Проверка столкновений с препятствиями
        for (auto& obstacle : obstacles) {
            if (obstacle.active && player.lane == obstacle.lane) {
                Rectangle obstacleRect = {
                    obstacle.position.x - obstacle.size.x / 2,
                    obstacle.position.y - obstacle.size.y,
                    obstacle.size.x,
                    obstacle.size.y
                };

                if (CheckCollisionRecs(playerRect, obstacleRect)) {
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
                if (CheckCollisionCircleRec(coin.position, coin.radius, playerRect)) {
                    coin.active = false;
                    coinsCollected++;
                    score += 100;
                }
            }
        }
    }

    void ResetGame() {
        player.position = { lanePositions[1], static_cast<float>(screenHeight) - 150.0f };
        player.lane = 1;
        player.isJumping = false;
        player.isDucking = false;
        player.jumpVelocity = 0;
        player.size = { 40.0f, 80.0f };

        obstacles.clear();
        coins.clear();
        trails.clear();

        camera.target = { screenWidth / 2.0f, player.position.y - 200.0f };

        obstacleSpawnTimer = 0;
        coinSpawnTimer = 0;
        trailSpawnTimer = 0;

        score = 0;
        coinsCollected = 0;
        gameOver = false;

        // Сбрасываем параллакс-смещение
        leftEdgeOffset = 0;
        rightEdgeOffset = 0;
    }

    void DrawPlayer() {
        Texture2D currentTexture;

        // Выбираем текстуру в зависимости от типа персонажа
        switch (player.characterType) {
        case 0: currentTexture = characterDefault; break;
        case 1: currentTexture = characterNinja; break;
        case 2: currentTexture = characterRobot; break;
        case 3: currentTexture = characterGirl; break;
        }

        // Если текстура загружена, рисуем её
        if (currentTexture.id != 0) {
            Rectangle sourceRect = { 0, 0, (float)currentTexture.width, (float)currentTexture.height };
            Rectangle destRect = {
                player.position.x,
                player.position.y,
                player.size.x,
                player.size.y
            };
            Vector2 origin = { player.size.x / 2, player.size.y };

            DrawTexturePro(currentTexture, sourceRect, destRect, origin, 0.0f, WHITE);
        }
        else {
            // Fallback - рисуем цветной прямоугольник
            DrawRectanglePro(
                Rectangle{ player.position.x, player.position.y, player.size.x, player.size.y },
                Vector2{ player.size.x / 2, player.size.y }, 0.0f, player.color);
        }
    }

    void Draw() {
        BeginDrawing();

        if (menu.isActive) {
            DrawMenu();
        }
        else {
            BeginMode2D(camera);

            ClearBackground(backgroundColor);

            // РИСУЕМ КРАЯ ЭКРАНА С КАРТИНКАМИ (ПАРАЛЛАКС-ЭФФЕКТ)
            float cameraTop = camera.target.y - screenHeight / 2.0f;

            // Левый край с параллакс-эффектом
            if (leftEdgeTexture.id != 0) {
                // Рисуем две копии текстуры для бесшовного скроллинга
                float textureY = cameraTop * 0.3f - leftEdgeOffset;
                DrawTextureRec(
                    leftEdgeTexture,
                    Rectangle{ 0, 0, (float)edgeWidth, (float)screenHeight },
                    Vector2{ camera.target.x - screenWidth / 2.0f - edgeWidth, textureY },
                    WHITE
                );
                // Вторая копия для непрерывного скроллинга
                DrawTextureRec(
                    leftEdgeTexture,
                    Rectangle{ 0, 0, (float)edgeWidth, (float)screenHeight },
                    Vector2{ camera.target.x - screenWidth / 2.0f - edgeWidth, textureY - screenHeight },
                    WHITE
                );
            }

            // Правый край с параллакс-эффектом
            if (rightEdgeTexture.id != 0) {
                float textureY = cameraTop * 0.3f - rightEdgeOffset;
                DrawTextureRec(
                    rightEdgeTexture,
                    Rectangle{ 0, 0, (float)edgeWidth, (float)screenHeight },
                    Vector2{ camera.target.x + screenWidth / 2.0f, textureY },
                    WHITE
                );
                // Вторая копия для непрерывного скроллинга
                DrawTextureRec(
                    rightEdgeTexture,
                    Rectangle{ 0, 0, (float)edgeWidth, (float)screenHeight },
                    Vector2{ camera.target.x + screenWidth / 2.0f, textureY - screenHeight },
                    WHITE
                );
            }

            // Рисуем игровую область между краями
            float gameAreaLeft = camera.target.x - screenWidth / 2.0f;
            float gameAreaWidth = screenWidth;

            // Рисуем землю
            DrawRectangle(
                (int)gameAreaLeft,
                screenHeight - 100,
                (int)gameAreaWidth,
                100,
                groundColor
            );

            // Рисуем дорожки
            for (int i = 0; i < 3; i++) {
                float x = lanePositions[i] - laneWidth / 2;
                for (int y = -1000; y < 1000; y += 100) {
                    DrawRectangleLines((int)x, y, (int)laneWidth, 100, Fade(BLACK, 0.3f));
                }
            }

            // Рисуем следы
            for (auto& trail : trails) {
                float alpha = 1.0f - (trail.lifetime / trail.maxLifetime);
                Color trailColor = Fade(trail.color, alpha * 0.5f);
                DrawRectanglePro(
                    Rectangle{ trail.position.x, trail.position.y, trail.size.x, trail.size.y },
                    Vector2{ trail.size.x / 2, trail.size.y / 2 }, 0.0f, trailColor);
            }

            // Рисуем препятствия
            for (auto& obstacle : obstacles) {
                if (obstacle.active) {
                    Color obstacleColor = obstacle.color;

                    switch (obstacle.type) {
                    case ObstacleType::JUMP_OVER:
                        DrawRectanglePro(
                            Rectangle{ obstacle.position.x, obstacle.position.y, obstacle.size.x, obstacle.size.y },
                            Vector2{ obstacle.size.x / 2, obstacle.size.y / 2 }, 0.0f, obstacleColor);
                        DrawTriangle(
                            Vector2{ obstacle.position.x - 15, obstacle.position.y - obstacle.size.y / 2 + 10 },
                            Vector2{ obstacle.position.x + 15, obstacle.position.y - obstacle.size.y / 2 + 10 },
                            Vector2{ obstacle.position.x, obstacle.position.y - obstacle.size.y / 2 - 10 },
                            YELLOW);
                        break;

                    case ObstacleType::DUCK_UNDER:
                        DrawRectanglePro(
                            Rectangle{ obstacle.position.x, obstacle.position.y, obstacle.size.x, obstacle.size.y },
                            Vector2{ obstacle.size.x / 2, obstacle.size.y / 2 }, 0.0f, obstacleColor);
                        DrawTriangle(
                            Vector2{ obstacle.position.x - 15, obstacle.position.y + obstacle.size.y / 2 - 10 },
                            Vector2{ obstacle.position.x + 15, obstacle.position.y + obstacle.size.y / 2 - 10 },
                            Vector2{ obstacle.position.x, obstacle.position.y + obstacle.size.y / 2 + 10 },
                            YELLOW);
                        break;

                    case ObstacleType::WALL:
                        DrawRectanglePro(
                            Rectangle{ obstacle.position.x, obstacle.position.y, obstacle.size.x, obstacle.size.y },
                            Vector2{ obstacle.size.x / 2, obstacle.size.y / 2 }, 0.0f, obstacleColor);
                        DrawLineEx(
                            Vector2{ obstacle.position.x - obstacle.size.x / 3, obstacle.position.y - obstacle.size.y / 3 },
                            Vector2{ obstacle.position.x + obstacle.size.x / 3, obstacle.position.y + obstacle.size.y / 3 },
                            3, RED);
                        DrawLineEx(
                            Vector2{ obstacle.position.x + obstacle.size.x / 3, obstacle.position.y - obstacle.size.y / 3 },
                            Vector2{ obstacle.position.x - obstacle.size.x / 3, obstacle.position.y + obstacle.size.y / 3 },
                            3, RED);
                        break;
                    }

                    DrawRectangleLinesEx(
                        Rectangle{
                            obstacle.position.x - obstacle.size.x / 2,
                            obstacle.position.y - obstacle.size.y / 2,
                            obstacle.size.x,
                            obstacle.size.y
                        },
                        2,
                        BLACK
                    );
                }
            }

            // Рисуем монеты
            for (auto& coin : coins) {
                if (coin.active) {
                    DrawCircle(coin.position.x, coin.position.y, coin.radius, coin.color);
                    DrawCircleLines(coin.position.x, coin.position.y, coin.radius, YELLOW);
                    DrawCircle(coin.position.x - 3, coin.position.y - 3, coin.radius / 3, YELLOW);
                }
            }

            // РИСУЕМ ИГРОКА С ВЫБРАННЫМ ПЕРСОНАЖЕМ
            DrawPlayer();

            EndMode2D();

            // Рисуем UI поверх всего
            DrawText(TextFormat("Score: %d", score), 10, 10, 20, BLACK);
            DrawText(TextFormat("Coins: %d", coinsCollected), 10, 40, 20, BLACK);
            DrawText(TextFormat("Lane: %d", player.lane + 1), 10, 70, 20, BLACK);
            DrawText(TextFormat("Location: %s", menu.locations[menu.selectedLocation].c_str()), 10, 100, 15, DARKGRAY);
            DrawText(TextFormat("Music: %s", menu.musicTracks[menu.selectedMusic].c_str()), 10, 120, 15, DARKGRAY);
            DrawText(TextFormat("Character: %s", menu.characters[player.characterType].c_str()), 10, 140, 15, DARKGRAY);

            // Подсказки по управлению
            DrawText("JUMP: SPACE/UP", 10, 170, 15, DARKGREEN);
            DrawText("DUCK: DOWN", 10, 190, 15, DARKBLUE);
            DrawText("MOVE: LEFT/RIGHT", 10, 210, 15, DARKPURPLE);
            DrawText("MENU: M", 10, 230, 15, DARKBROWN);

            if (gameOver) {
                DrawRectangle(0, 0, screenWidth, screenHeight, Fade(BLACK, 0.5f));
                DrawText("GAME OVER", screenWidth / 2 - MeasureText("GAME OVER", 40) / 2, screenHeight / 2 - 50, 40, RED);
                DrawText(TextFormat("Final Score: %d", score), screenWidth / 2 - MeasureText(TextFormat("Final Score: %d", score), 20) / 2, screenHeight / 2, 20, WHITE);
                DrawText("Press R to restart", screenWidth / 2 - MeasureText("Press R to restart", 20) / 2, screenHeight / 2 + 30, 20, WHITE);
                DrawText("Press M for menu", screenWidth / 2 - MeasureText("Press M for menu", 20) / 2, screenHeight / 2 + 60, 20, WHITE);
            }
        }

        EndDrawing();
    }

    void DrawMenu() {
        ClearBackground(DARKBLUE);

        // Заголовок
        DrawText("SUBWAY SURFERS CLONE", screenWidth / 2 - MeasureText("SUBWAY SURFERS CLONE", 40) / 2, 50, 40, YELLOW);

        // Выбор локации
        DrawText("SELECT LOCATION:", screenWidth / 2 - MeasureText("SELECT LOCATION:", 30) / 2, 150, 30, WHITE);
        for (int i = 0; i < (int)menu.locations.size(); i++) {
            Color color = (i == menu.selectedLocation) ? GREEN : WHITE;
            DrawText(menu.locations[i].c_str(), screenWidth / 2 - MeasureText(menu.locations[i].c_str(), 25) / 2, 200 + i * 40, 25, color);
        }

        // Выбор музыки
        DrawText("SELECT MUSIC:", screenWidth / 2 - MeasureText("SELECT MUSIC:", 30) / 2, 350, 30, WHITE);
        for (int i = 0; i < (int)menu.musicTracks.size(); i++) {
            Color color = (i == menu.selectedMusic) ? GREEN : WHITE;
            DrawText(menu.musicTracks[i].c_str(), screenWidth / 2 - MeasureText(menu.musicTracks[i].c_str(), 25) / 2, 400 + i * 40, 25, color);
        }

        // Выбор персонажа
        DrawText("SELECT CHARACTER: (A/D to change)", screenWidth / 2 - MeasureText("SELECT CHARACTER: (A/D to change)", 30) / 2, 500, 30, WHITE);
        for (int i = 0; i < (int)menu.characters.size(); i++) {
            Color color = (i == menu.selectedCharacter) ? GREEN : WHITE;
            DrawText(menu.characters[i].c_str(), screenWidth / 2 - MeasureText(menu.characters[i].c_str(), 25) / 2, 550 + i * 40, 25, color);
        }

        // Инструкции
        DrawText("USE ARROWS TO NAVIGATE, ENTER TO START", screenWidth / 2 - MeasureText("USE ARROWS TO NAVIGATE, ENTER TO START", 20) / 2, 750, 20, LIGHTGRAY);
    }
};

int main() {
    Game game;
    game.Run();
    return 0;
}