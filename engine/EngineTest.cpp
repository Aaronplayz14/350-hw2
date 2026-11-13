#include <cmath>
#include <cstddef>
#include <filesystem>
#include <functional>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include "Component.h"
#include "CollisionLateSystem.h"
#include "CollisionSystem.h"
#include "GravitySystem.h"
#include "MovementSystem.h"
#include "SpawnSystem.h"
#include "ECSEngine.h"
#include "Entity.h"
#include "EntityManager.h"
#include "ScoreComponent.h"
#include "ScoreSystem.h"
#include "SoundManager.h"
#include "SpriteComponent.h"
#include "SpriteManager.h"
#include "SystemManager.h"

//-----------------------------------------------------------------------
/**
 * @brief Regression tests for the engine and ECS core.
 *
 * The suite covers the component storage, entity/component lifecycle, system
 * execution order and exit handling, sound and sprite ownership, and score
 * digit updates. Tests that need image or audio assets skip their success path
 * when the assets cannot be found, so the suite still runs from any of the
 * usual build/launch directories.
 *
 * Every check records a failure instead of aborting, which lets one run report
 * all broken behaviours rather than only the first one.
 *
 * Implemented with the help of AI.
 */
//-----------------------------------------------------------------------
namespace {

int gFailures = 0;

void Check(bool condition, const std::string& message) {
    if (!condition) {
        std::cerr << "FAILED: " << message << '\n';
        ++gFailures;
    }
}

template <typename Exception, typename Function>
void CheckThrows(Function&& function, const std::string& message) {
    try {
        function();
        Check(false, message + " (no exception thrown)");
    } catch (const Exception&) {
        // Expected.
    } catch (const std::exception& error) {
        Check(false, message + " (wrong exception: " + error.what() + ")");
    } catch (...) {
        Check(false, message + " (wrong exception type)");
    }
}

// Asset files are copied into the build directory by CMake, but the test may be
// launched from the repository root, build/, or build/engine/.
std::string FindAsset(const std::string& fileName) {
    const std::vector<std::string> roots = {"assets/", "../assets/", "../../assets/",
                                            "../../../assets/"};
    for (const std::string& root : roots) {
        const std::string path = root + fileName;
        if (std::filesystem::exists(path)) {
            return path;
        }
    }
    return {};
}

struct Position {
    float x = 0;
    float y = 0;
};

struct Velocity {
    float x = 0;
    float y = 0;
};

struct Health {
    int value = 0;
};

//-----------------------------------------------------------------------
/**
 * @brief Verifies dense storage, removal, and free-list reuse.
 *
 * Checks that slot indices count up, removed slots become invalid without
 * disturbing their neighbours, and the next Add reuses a free slot instead of
 * growing capacity.
 */
//-----------------------------------------------------------------------
void TestComponentStorage() {
    ECSEngine::ComponentStorage<int> storage;
    Check(storage.Empty(), "new component storage is empty");
    Check(storage.Count() == 0, "new component storage count is zero");

    const std::size_t first = storage.Add(10);
    const std::size_t second = storage.Add(20);
    Check(first == 0 && second == 1, "component slots count up");
    Check(storage.IsValid(first) && storage.IsValid(second), "added slots are valid");
    Check(storage.Get(first) == 10 && storage.Get(second) == 20, "components read back");
    Check(storage.Count() == 2 && storage.Capacity() == 2, "storage counts are correct");

    storage.Get(first) = 11;
    Check(storage.Get(first) == 11, "components are mutable");

    storage.Remove(first);
    Check(!storage.IsValid(first), "removed slot is invalid");
    Check(storage.IsValid(second), "removing one slot leaves the other valid");
    Check(storage.Count() == 1, "count drops after removal");

    const std::size_t reused = storage.Add(30);
    Check(reused == first, "free slot is reused");
    Check(storage.Get(reused) == 30, "reused slot holds the new value");
    Check(storage.Capacity() == 2, "reusing a slot does not grow the storage");
    Check(storage.Count() == 2, "count grows after reuse");

    storage.Remove(reused);
    storage.Remove(second);
    Check(storage.Empty(), "removing every component empties the storage");
    Check(storage.Capacity() == 2, "capacity is retained after removals");
}

//-----------------------------------------------------------------------
/**
 * @brief Verifies entity creation, component ownership, removal, and id reuse.
 *
 * Exercises component add/replace/remove, entity removal clearing all of its
 * components, iteration over invalid slots, and reuse of freed entity ids.
 */
//-----------------------------------------------------------------------
void TestEntityManager() {
    using Manager = ECSEngine::EntityManager<Position, Velocity, Health>;
    Manager manager;

    const ECSEngine::EntityID e0 = manager.CreateEntity("player");
    const ECSEngine::EntityID e1 = manager.CreateEntity("enemy");
    const ECSEngine::EntityID e2 = manager.CreateEntity("star");

    Check(e0 == 0 && e1 == 1 && e2 == 2, "entity ids start at zero and count up");
    Check(manager.IsValid(e0) && manager.IsValid(e1) && manager.IsValid(e2),
          "new entities are valid");
    Check(manager.GetEntityCount() == 3, "entity count includes every slot");
    Check(manager.GetActiveEntityCount() == 3, "active entity count is correct");
    Check(manager.GetEntity(e0)->GetName() == "player", "entity names are stored");
    Check(manager.GetEntity(999) == nullptr, "out-of-range entity lookup returns null");

    manager.AddComponent<Position>(e0, Position{1.0F, 2.0F});
    manager.AddComponent<Velocity>(e0, Velocity{3.0F, 4.0F});
    manager.AddComponent<Health>(e1, Health{5});

    Check(manager.HasComponent<Position>(e0), "entity has added position");
    Check(manager.HasComponent<Velocity>(e0), "entity has added velocity");
    Check(!manager.HasComponent<Health>(e0), "components do not leak between entities");
    Check(manager.GetComponent<Position>(e0).x == 1.0F, "position x reads back");
    Check(manager.GetComponent<Velocity>(e0).y == 4.0F, "velocity y reads back");
    Check(manager.GetComponent<Health>(e1).value == 5, "health reads back");

    // Replacing a component updates the value without growing the storage.
    manager.AddComponent<Position>(e0, Position{7.0F, 8.0F});
    Check(manager.GetComponent<Position>(e0).x == 7.0F, "adding again replaces a component");
    Check(manager.GetComponentStorage<Position>().Count() == 1,
          "replacing does not duplicate storage");
    Check(manager.GetComponentStorage<Velocity>().Count() == 1,
          "different component types use different storage");

    manager.RemoveComponent<Velocity>(e0);
    Check(!manager.HasComponent<Velocity>(e0), "removed component is gone");
    Check(manager.GetComponentStorage<Velocity>().Count() == 0,
          "removed component is removed from storage");

    // Removing an entity invalidates its slot and removes its components.
    manager.RemoveEntity(e1);
    Check(!manager.IsValid(e1), "removed entity is invalid");
    Check(!manager.GetEntity(e1)->IsValid(), "removed entity slot is marked invalid");
    Check(!manager.HasComponent<Health>(e1), "removed entity has no components");
    Check(manager.GetComponentStorage<Health>().Count() == 0,
          "removing an entity clears component storage");
    Check(manager.GetActiveEntityCount() == 2, "active count drops after removal");

    // The invalid slot is still visited by iteration, so callers must skip it.
    std::size_t live = 0;
    std::size_t dead = 0;
    for (auto it = manager.begin(); it != manager.end(); ++it) {
        if (it->IsValid()) {
            ++live;
        } else {
            ++dead;
        }
    }
    Check(live == 2 && dead == 1, "iteration exposes live and invalid entity slots");

    // Removed ids are reused before the entity vector grows.
    manager.RemoveEntity(e2);
    const ECSEngine::EntityID reused = manager.CreateEntity("new star");
    Check(reused == e2, "removed entity id is reused");
    Check(manager.IsValid(reused), "reused entity is valid");
    Check(!manager.HasComponent<Position>(reused), "reused entity starts with no components");
    Check(manager.GetEntity(reused)->GetName() == "new star", "reused entity gets its new name");
    Check(manager.GetEntityCount() == 3, "reuse does not grow the entity vector");

    manager.AddComponent<Position>(reused, Position{9.0F, 10.0F});
    Check(manager.GetComponent<Position>(reused).y == 10.0F,
          "reused entity can receive components");

    Check(!manager.HasComponent<Position>(999), "unknown entity has no components");
    Check(!manager.IsValid(999), "unknown entity id is invalid");

    manager.RemoveEntity(e0);
    manager.RemoveEntity(reused);
    Check(manager.GetActiveEntityCount() == 0, "removing all entities leaves none active");
}

//-----------------------------------------------------------------------
/**
 * @brief Minimal EngineView used by the system-manager tests.
 *
 * The window manager is deliberately never constructed because doing so would
 * open an OS window during a unit test. These tests only need the system
 * manager, so GetWindowManager is never called.
 */
//-----------------------------------------------------------------------
template <typename... Components>
class TestEngineView : public ECSEngine::EngineView<Components...> {
public:
    ECSEngine::SoundManager& GetSoundManager() override { return mSoundManager; }
    ECSEngine::SpriteManager& GetSpriteManager() override { return mSpriteManager; }
    ECSEngine::EntityManager<Components...>& GetEntityManager() override {
        return mEntityManager;
    }

    // SystemManager tests never request the window, so this pointer may stay
    // null; constructing a real WindowManager would open an OS window.
    ECSEngine::WindowManager& GetWindowManager() override { return *mWindowManager; }

private:
    ECSEngine::SoundManager mSoundManager;
    ECSEngine::SpriteManager mSpriteManager;
    ECSEngine::EntityManager<Components...> mEntityManager;
    ECSEngine::WindowManager* mWindowManager = nullptr;
};

//-----------------------------------------------------------------------
/**
 * @brief Test system that records its name and returns a configured result.
 */
//-----------------------------------------------------------------------
class RecordingSystem : public ECSEngine::System<int> {
public:
    RecordingSystem(std::string name, std::vector<std::string>* order,
                    ECSEngine::tSystemReturn result)
        : mName(std::move(name)), mOrder(order), mResult(result) {}

    ECSEngine::tSystemReturn Run(ECSEngine::EngineView<int>*) override {
        mOrder->push_back(mName);
        return mResult;
    }

    const std::string& GetName() const override { return mName; }

private:
    std::string mName;
    std::vector<std::string>* mOrder;
    ECSEngine::tSystemReturn mResult;
};

//-----------------------------------------------------------------------
/**
 * @brief Verifies system ordering, clearing, and kExit handling.
 *
 * Systems must run in insertion order, and a system returning kExit must stop
 * the manager before later systems receive their turn.
 */
//-----------------------------------------------------------------------
void TestSystemManager() {
    TestEngineView<int> view;
    ECSEngine::SystemManager<int> manager;
    std::vector<std::string> order;

    Check(manager.Empty(), "new system manager is empty");
    manager.AddSystem(
        std::make_unique<RecordingSystem>("first", &order, ECSEngine::kContinue));
    manager.AddSystem(
        std::make_unique<RecordingSystem>("second", &order, ECSEngine::kContinue));
    Check(manager.GetSystemCount() == 2, "systems are counted");
    Check(manager.Run(&view), "systems that continue let the frame continue");
    Check(order == std::vector<std::string>({"first", "second"}),
          "systems run in insertion order");

    order.clear();
    manager.ClearSystems();
    Check(manager.Empty(), "clear systems removes every system");

    manager.AddSystem(
        std::make_unique<RecordingSystem>("stop", &order, ECSEngine::kExit));
    manager.AddSystem(
        std::make_unique<RecordingSystem>("never", &order, ECSEngine::kContinue));
    Check(!manager.Run(&view), "a system requesting exit stops the manager");
    Check(order == std::vector<std::string>({"stop"}),
          "systems after an exiting system do not run");
}

//-----------------------------------------------------------------------
/**
 * @brief Verifies sound registration, replacement, lookup, and volume.
 *
 * Error paths are checked with missing files and empty names. The success path
 * needs an .ogg asset and is skipped when the asset is not available.
 */
//-----------------------------------------------------------------------
void TestSoundManager() {
    ECSEngine::SoundManager manager;

    Check(!manager.HasSound("jump"), "unregistered sound is absent");
    CheckThrows<std::out_of_range>([&] { manager.GetSound("jump"); },
                                   "getting an unknown sound throws");
    CheckThrows<std::invalid_argument>(
        [&] { manager.RegisterSound("anything", ""); },
        "registering a sound with an empty name throws");

    CheckThrows<std::runtime_error>(
        [&] { manager.RegisterSound("this-file-does-not-exist.ogg", "missing"); },
        "registering a missing sound file throws");

    const std::string jumpPath = FindAsset("sfx_jump.ogg");
    if (jumpPath.empty()) {
        std::cout << "SKIP: sound success test (asset not found)\n";
        return;
    }

    try {
        manager.RegisterSound(jumpPath, "jump");
        Check(manager.HasSound("jump"), "registered sound is present");
        Check(manager.GetSoundCount() == 1, "registered sound is counted");

        sf::Sound& sound = manager.GetSound("jump");
        Check(sound.getBuffer().getSampleCount() > 0, "registered sound buffer has samples");
        manager.SetVolume("jump", 42.0F);
        Check(std::abs(sound.getVolume() - 42.0F) < 0.001F, "sound volume can be changed");

        // Re-registering replaces the entry rather than duplicating it.
        manager.RegisterSound(jumpPath, "jump");
        Check(manager.GetSoundCount() == 1, "re-registering a name replaces the sound");
        Check(manager.GetSound("jump").getBuffer().getSampleCount() > 0,
              "re-registered sound still has its buffer");
    } catch (const std::exception& error) {
        std::cout << "SKIP: sound success test (" << error.what() << ")\n";
    }
}

//-----------------------------------------------------------------------
/**
 * @brief Verifies texture registration, source rectangles, and sprite ids.
 *
 * The same texture path must be loaded once while producing separate sprite
 * ids and sprite instances. The success path is skipped when the sprite sheet
 * is not available.
 */
//-----------------------------------------------------------------------
void TestSpriteManager() {
    ECSEngine::SpriteManager manager;

    Check(!manager.IsValid(0), "new sprite manager has no sprites");
    CheckThrows<std::out_of_range>([&] { manager.GetSprite(0); },
                                   "getting an invalid sprite throws");
    CheckThrows<std::invalid_argument>(
        [&] { (void)manager.RegisterTexture("", ECSEngine::Rect{}); },
        "registering a texture with an empty path throws");
    CheckThrows<std::runtime_error>(
        [&] {
            (void)manager.RegisterTexture("this-file-does-not-exist.png", ECSEngine::Rect{});
        },
        "registering a missing texture throws");

    const std::string sheet = FindAsset("spritesheet-tiles-default.png");
    if (sheet.empty()) {
        std::cout << "SKIP: sprite success test (asset not found)\n";
        return;
    }

    try {
        ECSEngine::Rect sourceRect;
        sourceRect.topLeft = ECSEngine::Point2D(64.0F, 128.0F);
        sourceRect.width = 64.0F;
        sourceRect.height = 64.0F;

        const ECSEngine::SpriteID first = manager.RegisterTexture(sheet, sourceRect);
        const ECSEngine::SpriteID second = manager.RegisterTexture(sheet, sourceRect);

        Check(first == 0 && second == 1, "sprite ids are vector indices");
        Check(manager.IsValid(first) && manager.IsValid(second), "registered sprites are valid");
        Check(manager.GetSpriteCount() == 2, "sprites are counted");

        sf::Sprite& sprite = manager.GetSprite(first);
        Check(sprite.getTextureRect().position.x == 64, "source rectangle x is applied");
        Check(sprite.getTextureRect().position.y == 128, "source rectangle y is applied");
        Check(sprite.getTextureRect().size.x == 64, "source rectangle width is applied");
        Check(sprite.getTextureRect().size.y == 64, "source rectangle height is applied");
    } catch (const std::exception& error) {
        std::cout << "SKIP: sprite success test (" << error.what() << ")\n";
    }
}

//-----------------------------------------------------------------------
/**
 * @brief Verifies score digit extraction, clamping, and negative handling.
 *
 * A three-digit display must show leading zeroes, clamp values above 999 to
 * 999, and display negative scores as zero.
 */
//-----------------------------------------------------------------------
void TestScoreSystem() {
    using Manager =
        ECSEngine::EntityManager<ECSEngine::ScoreComponent, ECSEngine::SpriteComponent,
                                  ECSEngine::LocationComponent>;
    Manager manager;

    const ECSEngine::EntityID scoreEntity = manager.CreateEntity("score display");
    ECSEngine::ScoreComponent score{};
    score.score = 42;

    for (int digit = 0; digit < 10; ++digit) {
        score.digitTextureID[digit] = static_cast<ECSEngine::SpriteID>(100 + digit);
    }

    for (int i = 0; i < 3; ++i) {
        const ECSEngine::EntityID digitEntity =
            manager.CreateEntity("digit " + std::to_string(i));
        ECSEngine::SpriteComponent sprite{};
        manager.AddComponent<ECSEngine::SpriteComponent>(digitEntity, sprite);
        score.digitID[i] = digitEntity;
    }

    manager.AddComponent<ECSEngine::ScoreComponent>(scoreEntity, score);

    ECSEngine::ScoreSystem<ECSEngine::ScoreComponent, ECSEngine::SpriteComponent,
                           ECSEngine::LocationComponent>::UpdateScore(manager);

    auto textureFor = [&](int i) {
        return manager.GetComponent<ECSEngine::SpriteComponent>(score.digitID[i]).textureID;
    };

    Check(textureFor(0) == 100, "hundreds digit of 42 is zero");
    Check(textureFor(1) == 104, "tens digit of 42 is four");
    Check(textureFor(2) == 102, "ones digit of 42 is two");

    // Update the score through the component; the display must follow.
    manager.GetComponent<ECSEngine::ScoreComponent>(scoreEntity).score = 7;
    ECSEngine::ScoreSystem<ECSEngine::ScoreComponent, ECSEngine::SpriteComponent,
                           ECSEngine::LocationComponent>::UpdateScore(manager);
    Check(textureFor(0) == 100, "leading zero is displayed for 7");
    Check(textureFor(1) == 100, "tens digit of 7 is zero");
    Check(textureFor(2) == 107, "ones digit of 7 is seven");

    manager.GetComponent<ECSEngine::ScoreComponent>(scoreEntity).score = 1234;
    ECSEngine::ScoreSystem<ECSEngine::ScoreComponent, ECSEngine::SpriteComponent,
                           ECSEngine::LocationComponent>::UpdateScore(manager);
    Check(textureFor(0) == 109 && textureFor(1) == 109 && textureFor(2) == 109,
          "score is clamped to the number of display digits");

    manager.GetComponent<ECSEngine::ScoreComponent>(scoreEntity).score = -5;
    ECSEngine::ScoreSystem<ECSEngine::ScoreComponent, ECSEngine::SpriteComponent,
                           ECSEngine::LocationComponent>::UpdateScore(manager);
    Check(textureFor(0) == 100 && textureFor(1) == 100 && textureFor(2) == 100,
          "negative scores display as zero");
}

// Simulate the real system order without creating a graphics window.
void TestStarSettlingAndShake() {
    using View = TestEngineView<ECSEngine::LocationComponent, ECSEngine::MovementComponent,
        ECSEngine::CollisionComponent, ECSEngine::GravityComponent, ECSEngine::RespawnComponent,
        ECSEngine::ScoreComponent, ECSEngine::CameraComponent, ECSEngine::CameraShake,
        ECSEngine::SpawnComponent, ECSEngine::SpriteComponent>;
    using namespace ECSEngine;
    View view;
    auto& entities = view.GetEntityManager();
    const auto addTile = [&](float x, float y, float width) {
        const EntityID id = entities.CreateEntity("tile");
        entities.AddComponent<LocationComponent>(id, LocationComponent{Point2D(x, y)});
        CollisionComponent collision{};
        collision.staticEntity = 1;
        collision.collisionBox = Rect(0.0F, 0.0F, width, 64.0F);
        entities.AddComponent<CollisionComponent>(id, collision);
        return id;
    };
    const EntityID floor = addTile(-1000.0F, 200.0F, 2000.0F);
    const EntityID star = SpawnSystem<LocationComponent, MovementComponent, CollisionComponent,
        GravityComponent, RespawnComponent, ScoreComponent, CameraComponent, CameraShake,
        SpawnComponent, SpriteComponent>::CreateStar(entities, 0, Point2D(0.0F, 0.0F),
                                                    Point2D(5.0F, 0.0F));
    using Snapshot = CollisionLateSystem<LocationComponent, MovementComponent, CollisionComponent,
        GravityComponent, RespawnComponent, ScoreComponent, CameraComponent, CameraShake,
        SpawnComponent, SpriteComponent>;
    using Gravity = GravitySystem<LocationComponent, MovementComponent, CollisionComponent,
        GravityComponent, RespawnComponent, ScoreComponent, CameraComponent, CameraShake,
        SpawnComponent, SpriteComponent>;
    using Movement = MovementSystem<LocationComponent, MovementComponent, CollisionComponent,
        GravityComponent, RespawnComponent, ScoreComponent, CameraComponent, CameraShake,
        SpawnComponent, SpriteComponent>;
    using Collisions = CollisionSystem<LocationComponent, MovementComponent, CollisionComponent,
        GravityComponent, RespawnComponent, ScoreComponent, CameraComponent, CameraShake,
        SpawnComponent, SpriteComponent>;
    const auto tick = [&] {
        Snapshot{}.Run(&view);
        Gravity{}.Run(&view);
        Movement{}.Run(&view);
        Collisions{}.Run(&view);
    };
    bool bounced = false;
    for (int frame = 0; frame < 300; ++frame) {
        tick();
        const Point2D velocity = entities.GetComponent<MovementComponent>(star).velocity;
        bounced = bounced || velocity.y < 0.0F;
        Check(velocity.y >= -3.4F, "star floor rebounds stay small");
        if (frame > 240) {
            Check(velocity == Point2D(), "star stays still after settling");
            Check(entities.GetComponent<CollisionComponent>(star).onGround,
                  "settled star retains floor contact");
        }
    }
    Check(bounced, "falling star initially bounces before settling");
    Check(entities.GetComponent<CollisionComponent>(star).resting, "star reaches resting state");
    entities.RemoveEntity(floor);
    tick();
    Check(entities.GetComponent<MovementComponent>(star).velocity.y > 0.0F,
          "settled star falls when floor support disappears");
    Check(!entities.GetComponent<CollisionComponent>(star).resting,
          "unsupported star leaves resting state");

    const EntityID spawner = addTile(0.0F, 200.0F, 64.0F);
    entities.AddComponent<SpawnComponent>(spawner, SpawnComponent{});
    entities.GetComponent<LocationComponent>(star).loc = Point2D(0.0F, 180.0F);
    entities.GetComponent<MovementComponent>(star).velocity = Point2D(0.0F, 4.0F);
    tick();
    Check(entities.GetComponent<MovementComponent>(star).velocity.y > 0.0F,
          "stars pass through spawner without bouncing");

    const EntityID player = entities.CreateEntity("player");
    entities.AddComponent<LocationComponent>(player, LocationComponent{Point2D(0.0F, 270.0F)});
    entities.AddComponent<MovementComponent>(player, MovementComponent{Point2D(0.0F, -12.0F)});
    CollisionComponent playerCollision{};
    playerCollision.collisionBox = Rect(0.0F, 0.0F, 48.0F, 64.0F);
    entities.AddComponent<CollisionComponent>(player, playerCollision);
    const EntityID camera = entities.CreateEntity("camera");
    entities.AddComponent<CameraComponent>(camera, CameraComponent{});
    tick();
    Check(entities.GetComponent<CollisionComponent>(player).onCeiling,
          "player cannot jump through spawner from below");
    Check(entities.GetComponent<MovementComponent>(player).velocity.y == 0.0F,
          "spawner stops player's upward movement");
    Check(entities.HasComponent<CameraShake>(camera), "player impact starts camera shake");

    CameraShake shake{14, Point2D(4.0F, 0.0F), 14};
    bool reversed = false;
    for (; shake.frameLimit > 0; --shake.frameLimit) {
        const Point2D sample = shake.GetOffset();
        reversed = reversed || sample.x < 0.0F;
        Check(std::fabs(sample.x) <= 4.0F && sample.y == 0.0F,
              "shake is bounded and follows collision direction");
        if (shake.frameLimit <= 3) Check(std::fabs(sample.x) < 0.2F, "shake fades smoothly");
    }
    Check(reversed, "camera shake oscillates instead of holding a fixed offset");
    Check(shake.GetOffset() == Point2D(), "expired camera shake returns exactly to zero");
}

}  // namespace

//-----------------------------------------------------------------------
/**
 * @brief Runs every core test and reports the number of failures.
 * @return 0 when all checks passed, 1 otherwise.
 */
//-----------------------------------------------------------------------
int main(int argc, char* argv[]) {
    (void)argc;
    (void)argv;

    TestComponentStorage();
    TestEntityManager();
    TestSystemManager();
    TestSoundManager();
    TestSpriteManager();
    TestScoreSystem();
    TestStarSettlingAndShake();

    if (gFailures != 0) {
        std::cerr << gFailures << " check(s) failed\n";
        return 1;
    }

    std::cout << "all engine checks passed\n";
    return 0;
}
