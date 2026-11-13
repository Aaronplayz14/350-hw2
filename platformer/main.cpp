#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <unordered_map>

#include "CameraSystem.h"
#include "CollisionComponent.h"
#include "CollisionLateSystem.h"
#include "CollisionSystem.h"
#include "ECSEngine.h"
#include "EntityManager.h"
#include "GravitySystem.h"
#include "InputComponent.h"
#include "InputSystem.h"
#include "LocationComponent.h"
#include "MathUtil.h"
#include "MovementSystem.h"
#include "ScoreComponent.h"
#include "ScoreSystem.h"
#include "SpawnComponent.h"
#include "SpawnSystem.h"
#include "SpriteComponent.h"
#include "SpriteManager.h"
#include "SpriteSystem.h"

std::string gResourcePath = "../assets/";
#define COMPONENTS                                                                             \
    ECSEngine::LocationComponent, ECSEngine::MovementComponent, ECSEngine::CollisionComponent, \
        ECSEngine::SpriteComponent, ECSEngine::SpawnComponent, ECSEngine::CameraComponent,     \
        ECSEngine::CameraFollower, ECSEngine::InputComponent, ECSEngine::GravityComponent,     \
        ECSEngine::CameraShake, ECSEngine::ScoreComponent, ECSEngine::RespawnComponent
using Engine = ECSEngine::ECSEngine<COMPONENTS>;

namespace {

// Information copied out of a map file's header, used to place the player and
// camera without hard-coding the level dimensions in main().
struct MapInfo {
    int type = 0;
    int width = 0;
    int height = 0;
    float tileWidth = 64.0f;
    float tileHeight = 64.0f;
    ECSEngine::Point2D origin;
};

// One character from a map file's dictionary. A dictionary entry maps a map
// character to a sprite-sheet rectangle and a collision rectangle.
struct TileDefinition {
    std::string texturePath;
    ECSEngine::Rect spriteRect;
    ECSEngine::Rect boundRect;
    ECSEngine::SpriteID spriteID = 0;
};

std::string JoinPath(const std::string& directory, const std::string& file) {
    if (directory.empty()) {
        return file;
    }
    if (directory.back() == '/' || directory.back() == '\\') {
        return directory + file;
    }
    return directory + "/" + file;
}

//-----------------------------------------------------------------------
/**
 * @brief Loads one Canvas-style dictionary map into the ECS world.
 *
 * The file starts with a dictionary that maps single characters to a texture
 * path, a source rectangle, and a collision rectangle. The map header then
 * supplies the world origin, tile size, and map dimensions. Each non-empty
 * cell becomes an entity with LocationComponent and SpriteComponent. For map
 * type 2 (objects), the entity also gets a static CollisionComponent. A cell
 * marked with 'S' additionally gets a SpawnComponent so the level's spawner
 * tiles are preserved for the SpawnSystem.
 *
 * Implemented with the help of AI.
 *
 * @param engine Engine that owns the sprite manager and entity manager.
 * @param path Resource directory containing the map and its textures.
 * @param file Map filename relative to path.
 * @param info Receives the map header information.
 * @param starSprite Sprite used by stars created from 'S' spawner cells.
 * @return true when the map was loaded; false when the file is missing or
 *         malformed.
 */
//-----------------------------------------------------------------------
bool LoadMap(Engine& engine, const std::string& path, const std::string& file, MapInfo& info,
             ECSEngine::SpriteID starSprite) {
    const std::string fullPath = JoinPath(path, file);
    std::ifstream input(fullPath);
    if (!input.is_open()) {
        std::cerr << "WARNING: could not open map file: " << fullPath << '\n';
        return false;
    }

    std::string line;
    if (!std::getline(input, line)) {
        std::cerr << "WARNING: empty map file: " << fullPath << '\n';
        return false;
    }

    std::istringstream dictionaryHeader(line);
    std::string dictionaryKeyword;
    if (!(dictionaryHeader >> dictionaryKeyword >> info.type) || dictionaryKeyword != "dictionary") {
        std::cerr << "WARNING: missing dictionary header in " << fullPath << '\n';
        return false;
    }

    std::unordered_map<char, TileDefinition> definitions;
    bool foundMapHeader = false;

    while (std::getline(input, line)) {
        if (line.empty()) {
            continue;
        }

        std::istringstream stream(line);
        std::string firstToken;
        stream >> firstToken;

        if (firstToken == "map") {
            std::string keyword;
            stream >> keyword >> info.origin.x >> info.origin.y;
            stream >> keyword >> info.tileWidth >> info.tileHeight;
            stream >> keyword >> info.width >> info.height;
            foundMapHeader = true;
            break;
        }

        if (firstToken.size() != 1) {
            continue;
        }

        TileDefinition definition;
        stream >> definition.texturePath;
        stream >> definition.spriteRect.topLeft.x >> definition.spriteRect.topLeft.y >>
            definition.spriteRect.width >> definition.spriteRect.height;
        stream >> definition.boundRect.topLeft.x >> definition.boundRect.topLeft.y >>
            definition.boundRect.width >> definition.boundRect.height;

        if (!stream) {
            std::cerr << "WARNING: malformed dictionary entry in " << fullPath << ": " << line
                      << '\n';
            continue;
        }

        definitions[firstToken[0]] = definition;
    }

    if (!foundMapHeader) {
        std::cerr << "WARNING: missing map header in " << fullPath << '\n';
        return false;
    }

    // Register each dictionary sprite once and let every matching tile share
    // the resulting SpriteID. SpriteSystem rewrites the sprite transform for
    // each entity before drawing, so sharing is safe and avoids hundreds of
    // duplicate sprites on a full background layer.
    ECSEngine::SpriteManager& sprites = engine.GetSpriteManager();
    for (auto& [symbol, definition] : definitions) {
        (void)symbol;
        definition.spriteID =
            sprites.RegisterTexture(JoinPath(path, definition.texturePath), definition.spriteRect);
    }

    auto& entities = engine.GetEntityManager();
    int row = 0;
    while (row < info.height && std::getline(input, line)) {
        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }
        if (line.empty()) {
            continue;
        }

        if (static_cast<int>(line.size()) < info.width) {
            std::cerr << "WARNING: map row " << row << " in " << fullPath << " is only "
                      << line.size() << " characters; expected " << info.width << '\n';
        }

        for (int column = 0; column < info.width && column < static_cast<int>(line.size());
             ++column) {
            const char symbol = line[column];
            if (symbol == '.' || symbol == ' ') {
                continue;
            }

            const auto found = definitions.find(symbol);
            if (found == definitions.end()) {
                std::cerr << "WARNING: no dictionary entry for '" << symbol << "' at " << row
                          << ',' << column << " in " << fullPath << '\n';
                continue;
            }

            const TileDefinition& definition = found->second;
            const ECSEngine::Point2D location(info.origin.x + column * info.tileWidth,
                                              info.origin.y + row * info.tileHeight);

            const ECSEngine::EntityID entity =
                entities.CreateEntity(std::string("tile ") + symbol);
            entities.AddComponent<ECSEngine::LocationComponent>(
                entity, ECSEngine::LocationComponent{location});

            ECSEngine::SpriteComponent sprite;
            sprite.textureID = definition.spriteID;
            sprite.bounds = ECSEngine::Rect(0.0f, 0.0f, info.tileWidth, info.tileHeight);
            sprite.worldSpace = true;
            entities.AddComponent<ECSEngine::SpriteComponent>(entity, sprite);

            if (info.type == 2) {
                ECSEngine::CollisionComponent collision{};
                collision.staticEntity = 1;
                collision.doesDamage = symbol == 'X' ? 1 : 0;
                collision.collisionBox = definition.boundRect;
                collision.previousCollisionBox = definition.boundRect;
                entities.AddComponent<ECSEngine::CollisionComponent>(entity, collision);
            }

            if (symbol == 'S') {
                ECSEngine::SpawnComponent spawn{};
                spawn.entityID = entity;
                spawn.spawnType = "star";
                spawn.spawnTextureID = starSprite;
                spawn.timeToNextSpawn = 60;
                spawn.timeBetweenSpawns = 120;
                spawn.totalSpawns = 5;
                entities.AddComponent<ECSEngine::SpawnComponent>(entity, spawn);
            }
        }

        ++row;
    }

    std::cout << "Loaded " << file << " (" << info.width << 'x' << info.height << " tiles, type "
              << info.type << ")\n";
    return true;
}

}  // namespace

int main(int argc, char* argv[]) {
    if (argc >= 3 && argv[1] == std::string("-path")) {
        gResourcePath = argv[2];
    }
    std::cout << "Usage: " << argv[0] << " [-path resource_path]\n";
    std::cout << "Using resource path: " << gResourcePath << "\n";

    // Game setup below builds the initial scene: sounds, level maps, player,
    // score display, camera, and systems.
    // Implemented with the help of AI.
    Engine engine(1024, 768, "Test Engine");

    // Load sounds. Gameplay systems can request these by name later.
    try {
        ECSEngine::SoundManager& sounds = engine.GetSoundManager();
        sounds.RegisterSound(JoinPath(gResourcePath, "sfx_jump.ogg"), "jump");
        sounds.RegisterSound(JoinPath(gResourcePath, "sfx_jump-high.ogg"), "jump-high");
        sounds.RegisterSound(JoinPath(gResourcePath, "sfx_gem.ogg"), "gem");
        sounds.RegisterSound(JoinPath(gResourcePath, "laserLarge_000.ogg"), "laser");
        sounds.RegisterSound(JoinPath(gResourcePath, "error_008.ogg"), "error");
        sounds.RegisterSound(JoinPath(gResourcePath, "footstep_grass_003.ogg"), "footstep-grass");
        sounds.RegisterSound(JoinPath(gResourcePath, "footstep_snow_001.ogg"), "footstep-snow");
    } catch (const std::exception& error) {
        std::cerr << "WARNING: could not load one or more sounds: " << error.what() << '\n';
    }

    // Load the background layer first so it is drawn beneath the object layer.
    // The level maps are reconstructed from the object-map example in the
    // project requirements because the Canvas map files are not in the repo.
    ECSEngine::SpriteManager& sprites = engine.GetSpriteManager();

    // The star and spawner both come from the provided tile sheet. The star is
    // the yellow collectable tile at grid cell (11, 6); the spawner tile keeps
    // whatever sprite its map dictionary entry supplies.
    const ECSEngine::SpriteID starSprite = sprites.RegisterTexture(
        JoinPath(gResourcePath, "spritesheet-tiles-default.png"),
        ECSEngine::Rect(704.0f, 384.0f, 64.0f, 64.0f));

    MapInfo backgroundInfo;
    MapInfo objectInfo;
    try {
        LoadMap(engine, gResourcePath, "map-background.txt", backgroundInfo, starSprite);
        LoadMap(engine, gResourcePath, "map-objects.txt", objectInfo, starSprite);
    } catch (const std::exception& error) {
        std::cerr << "WARNING: could not load the level maps: " << error.what() << '\n';
    }

    auto& entities = engine.GetEntityManager();
    // Start over the middle of the map so the initial camera shows the full
    // width of the left half of the level instead of off-map space.
    const ECSEngine::Point2D playerSpawn(512.0f, 448.0f);

    // Place the player above the floor in an open column.
    const ECSEngine::EntityID player = entities.CreateEntity("player");
    entities.AddComponent<ECSEngine::LocationComponent>(player,
                                                        ECSEngine::LocationComponent{playerSpawn});
    entities.AddComponent<ECSEngine::MovementComponent>(
        player, ECSEngine::MovementComponent{ECSEngine::Point2D(0.0f, 0.0f)});
    entities.AddComponent<ECSEngine::GravityComponent>(
        player, ECSEngine::GravityComponent(ECSEngine::Point2D(0.0f, 2.0f)));
    entities.AddComponent<ECSEngine::InputComponent>(player,
                                                     ECSEngine::InputComponent(player));
    entities.AddComponent<ECSEngine::RespawnComponent>(player,
                                                       ECSEngine::RespawnComponent{playerSpawn});

    ECSEngine::CollisionComponent playerCollision{};
    playerCollision.staticEntity = 0;
    playerCollision.doesDamage = 0;
    playerCollision.collisionBox = ECSEngine::Rect(8.0f, 0.0f, 48.0f, 64.0f);
    playerCollision.previousCollisionBox = playerCollision.collisionBox;
    entities.AddComponent<ECSEngine::CollisionComponent>(player, playerCollision);

    ECSEngine::SpriteComponent playerSprite;
    playerSprite.textureID = sprites.RegisterTexture(
        JoinPath(gResourcePath, "spritesheet-characters-default.png"),
        ECSEngine::Rect(0.0f, 0.0f, 128.0f, 128.0f));
    playerSprite.bounds = ECSEngine::Rect(0.0f, 0.0f, 64.0f, 64.0f);
    playerSprite.worldSpace = true;
    entities.AddComponent<ECSEngine::SpriteComponent>(player, playerSprite);

    entities.AddComponent<ECSEngine::CameraFollower>(player,
                                                     ECSEngine::CameraFollower{player});

    // Hand-place one star above the left-hand wall section. The requirement
    // allows a test star outside the normal spawner path so wall jumping over
    // the wall can be exercised immediately.
    ECSEngine::SpawnSystem<COMPONENTS>::CreateStar(
        entities, starSprite, ECSEngine::Point2D(704.0f, 384.0f),
        ECSEngine::Point2D(0.0f, 0.0f));

    // Set up three screen-space digit entities for the score display. The
    // numeral sprites are a small generated sheet so the score reads correctly
    // without relying on tiles that only look like numbers.
    ECSEngine::ScoreComponent score{};
    score.score = 0;
    for (int digit = 0; digit < 10; ++digit) {
        score.digitTextureID[digit] = sprites.RegisterTexture(
            JoinPath(gResourcePath, "spritesheet-digits-default.png"),
            ECSEngine::Rect(static_cast<float>(digit * 64), 0.0f, 64.0f, 64.0f));
    }

    for (int digit = 0; digit < 3; ++digit) {
        const ECSEngine::EntityID digitEntity =
            entities.CreateEntity(std::string("score digit ") + std::to_string(digit));
        entities.AddComponent<ECSEngine::LocationComponent>(
            digitEntity,
            ECSEngine::LocationComponent{ECSEngine::Point2D(20.0f + digit * 34.0f, 20.0f)});

        ECSEngine::SpriteComponent digitSprite;
        digitSprite.textureID = score.digitTextureID[0];
        digitSprite.bounds = ECSEngine::Rect(0.0f, 0.0f, 32.0f, 32.0f);
        digitSprite.worldSpace = false;
        entities.AddComponent<ECSEngine::SpriteComponent>(digitEntity, digitSprite);
        score.digitID[digit] = digitEntity;
    }

    const ECSEngine::EntityID scoreEntity = entities.CreateEntity("score");
    entities.AddComponent<ECSEngine::ScoreComponent>(scoreEntity, score);

    // Setup Camera. The x position follows the player; y stays centered on the
    // map so the full 12-tile height is visible in the 768px window.
    //
    // TEMP (Travis): The exclusion rectangles and speed are left at their
    // neutral values here on purpose. They belong to the camera pass that still
    // needs the 10% hard-follow band, the 10-30% slow-follow bands, and the
    // center-40% settle behavior from the project requirements.
    ECSEngine::WindowManager& windows = engine.GetWindowManager();
    windows.SetWorldScale(1.0f);

    ECSEngine::CameraComponent camera{};
    camera.position = ECSEngine::Point2D(
        playerSpawn.x,
        objectInfo.origin.y + objectInfo.height * objectInfo.tileHeight * 0.5f);
    camera.worldUnitsPerPixel = 1.0f;
    camera.hardExclusion = ECSEngine::Rect(ECSEngine::Point2D(windows.GetWindow()->getSize().x * 0.1f, 0.0f), 
                                           ECSEngine::Point2D(windows.GetWindow()->getSize().x * 0.9f, windows.GetWindow()->getSize().y));
    camera.softExclusion = ECSEngine::Rect(ECSEngine::Point2D(windows.GetWindow()->getSize().x * 0.3f, 0.0f), 
                                           ECSEngine::Point2D(windows.GetWindow()->getSize().x * 0.7f, windows.GetWindow()->getSize().y));
    camera.speed = 0.1f;

    const ECSEngine::EntityID cameraEntity = entities.CreateEntity("camera");
    entities.AddComponent<ECSEngine::CameraComponent>(cameraEntity, camera);
    windows.SetWorldCenter(camera.position);

    std::cout << "Player placed at " << playerSpawn << ", camera centered at "
              << camera.position << " with " << entities.GetEntityCount() << " entities\n";
    std::cout << std::flush;

    // Add all systems. SpriteSystem must be last because it clears and presents
    // the frame.
    auto& sm = engine.GetSystemManager();
    sm.AddSystem(std::make_unique<ECSEngine::CollisionLateSystem<COMPONENTS>>());
    sm.AddSystem(std::make_unique<ECSEngine::InputSystem<COMPONENTS>>());
    sm.AddSystem(std::make_unique<ECSEngine::GravitySystem<COMPONENTS>>());
    sm.AddSystem(std::make_unique<ECSEngine::MovementSystem<COMPONENTS>>());
    sm.AddSystem(std::make_unique<ECSEngine::CollisionSystem<COMPONENTS>>());
    sm.AddSystem(std::make_unique<ECSEngine::SpawnSystem<COMPONENTS>>());
    sm.AddSystem(std::make_unique<ECSEngine::ScoreSystem<COMPONENTS>>());
    sm.AddSystem(std::make_unique<ECSEngine::CameraSystem<COMPONENTS>>());
    sm.AddSystem(std::make_unique<ECSEngine::SpriteSystem<COMPONENTS>>());

    engine.Run();
    return 0;
}
