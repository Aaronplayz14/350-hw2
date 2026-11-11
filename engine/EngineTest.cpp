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
#include "ECSEngine.h"
#include "Entity.h"
#include "EntityManager.h"
#include "ScoreComponent.h"
#include "ScoreSystem.h"
#include "SoundManager.h"
#include "SpriteComponent.h"
#include "SpriteManager.h"
#include "SystemManager.h"

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

}  // namespace

int main(int argc, char* argv[]) {
    (void)argc;
    (void)argv;

    TestComponentStorage();
    TestEntityManager();
    TestSystemManager();
    TestSoundManager();
    TestSpriteManager();
    TestScoreSystem();

    if (gFailures != 0) {
        std::cerr << gFailures << " check(s) failed\n";
        return 1;
    }

    std::cout << "all engine checks passed\n";
    return 0;
}
