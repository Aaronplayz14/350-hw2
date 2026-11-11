#pragma once

#include <string>
#include <vector>

#include <SFML/Graphics.hpp>
#include <SFML/System/Sleep.hpp>
#include <SFML/Window.hpp>

#include "CameraComponent.h"
#include "CollisionComponent.h"
#include "EntityManager.h"
#include "InputComponent.h"
#include "LocationComponent.h"
#include "MathUtil.h"
#include "ScoreComponent.h"
#include "SoundManager.h"
#include "SpawnComponent.h"
#include "SpriteComponent.h"
#include "SpriteManager.h"
#include "SystemManager.h"
#include "WindowManager.h"

namespace ECSEngine {

// The read/write view of the engine that systems receive.  Systems must access
// resources through these getters rather than through globals so that systems
// can be moved to another engine/host later.
template <typename... Components>
class EngineView {
public:
    virtual ~EngineView() = default;

    virtual SoundManager& GetSoundManager() = 0;
    virtual SpriteManager& GetSpriteManager() = 0;
    virtual EntityManager<Components...>& GetEntityManager() = 0;
    virtual WindowManager& GetWindowManager() = 0;
};

// Owns the four resource managers and runs the main game loop.
//
// The loop polls SFML events (updating every InputComponent), runs the systems
// in order, and lets SpriteSystem present the frame.  F1 toggles collision box
// debug drawing, P pauses/resumes, and N advances one frame while paused.
template <typename... Components>
class ECSEngine : public EngineView<Components...> {
public:
    ECSEngine(unsigned int width, unsigned int height, const std::string& name);
    ~ECSEngine() override;

    ECSEngine(const ECSEngine&) = delete;
    ECSEngine(ECSEngine&&) = delete;
    ECSEngine& operator=(const ECSEngine&) = delete;
    ECSEngine& operator=(ECSEngine&&) = delete;

    // Blocks until the window closes, a system returns kExit, or Escape is
    // pressed.
    void Run();

    SoundManager& GetSoundManager() override { return mSoundManager; }

    SpriteManager& GetSpriteManager() override { return mSpriteManager; }

    EntityManager<Components...>& GetEntityManager() override { return mEntityManager; }

    WindowManager& GetWindowManager() override { return mWindowManager; }

    SystemManager<Components...>& GetSystemManager() { return mSystemManager; }

    [[nodiscard]] bool IsDebugEnabled() const { return mDebug; }

private:
    // Polls all pending events, updates input components and handles the
    // engine's own debugging keys.  Returns false when the window has closed.
    bool ProcessEvents();

    // Applies a key transition to every InputComponent in the world.
    void SetKeyState(unsigned int scancode, bool down);

    // Draws collision boxes when debug mode is on.
    void DebugDraw();

    EntityManager<Components...> mEntityManager;
    SystemManager<Components...> mSystemManager;
    SpriteManager mSpriteManager;
    SoundManager mSoundManager;
    WindowManager mWindowManager;

    // Debugging / main-loop control.
    bool mRunning = true;
    bool mPaused = false;
    bool mStep = false;
    bool mDebug = false;
};

template <typename... Components>
ECSEngine<Components...>::ECSEngine(unsigned int width, unsigned int height,
                                    const std::string& name)
    : mWindowManager(width, height, name) {
    if (auto window = mWindowManager.GetWindow()) {
        // One key-down event per physical press is what InputComponent expects.
        window->setKeyRepeatEnabled(false);
        window->setFramerateLimit(60);
    }
}

template <typename... Components>
ECSEngine<Components...>::~ECSEngine() {
    if (auto window = mWindowManager.GetWindow()) {
        if (window->isOpen()) {
            window->close();
        }
    }
}

template <typename... Components>
void ECSEngine<Components...>::Run() {
    auto window = mWindowManager.GetWindow();
    if (!window) {
        return;
    }

    mRunning = true;
    while (mRunning && window->isOpen()) {
        if (!ProcessEvents()) {
            break;
        }

        // A paused engine keeps polling events (so it can be resumed) but does
        // not advance the simulation unless a single step was requested.
        if (mPaused && !mStep) {
            sf::sleep(sf::milliseconds(10));
            continue;
        }
        mStep = false;

        if (!mSystemManager.Run(this)) {
            mRunning = false;
            break;
        }

        if (mDebug) {
            DebugDraw();
        }
    }
}

template <typename... Components>
bool ECSEngine<Components...>::ProcessEvents() {
    auto window = mWindowManager.GetWindow();
    if (!window) {
        return false;
    }

    // Remember this frame's starting key state before applying new events.  The
    // InputSystem sees keydown as "now" and lastKeydown as "the previous frame".
    for (auto it = mEntityManager.begin(); it != mEntityManager.end(); ++it) {
        if (!it->IsValid()) {
            continue;
        }

        const EntityID id = it->GetID();
        if (mEntityManager.template HasComponent<InputComponent>(id)) {
            auto& input = mEntityManager.template GetComponent<InputComponent>(id);
            input.lastKeydown = input.keydown;
        }
    }

    while (const auto event = window->pollEvent()) {
        if (event->template is<sf::Event::Closed>()) {
            window->close();
            return false;
        }

        if (const auto* keyPressed = event->template getIf<sf::Event::KeyPressed>()) {
            const int scancode = static_cast<int>(keyPressed->scancode);
            if (scancode >= 0 && scancode < static_cast<int>(sf::Keyboard::ScancodeCount)) {
                SetKeyState(static_cast<unsigned int>(scancode), true);
            }

            switch (keyPressed->scancode) {
                case sf::Keyboard::Scancode::Escape:
                    window->close();
                    return false;
                case sf::Keyboard::Scancode::F1:
                    mDebug = !mDebug;
                    break;
                case sf::Keyboard::Scancode::P:
                    mPaused = !mPaused;
                    break;
                case sf::Keyboard::Scancode::N:
                    // Step exactly one frame, then remain paused.
                    mStep = true;
                    mPaused = true;
                    break;
                default:
                    break;
            }
        } else if (const auto* keyReleased = event->template getIf<sf::Event::KeyReleased>()) {
            const int scancode = static_cast<int>(keyReleased->scancode);
            if (scancode >= 0 && scancode < static_cast<int>(sf::Keyboard::ScancodeCount)) {
                SetKeyState(static_cast<unsigned int>(scancode), false);
            }
        }
    }

    return window->isOpen();
}

template <typename... Components>
void ECSEngine<Components...>::SetKeyState(unsigned int scancode, bool down) {
    for (auto it = mEntityManager.begin(); it != mEntityManager.end(); ++it) {
        if (!it->IsValid()) {
            continue;
        }

        const EntityID id = it->GetID();
        if (!mEntityManager.template HasComponent<InputComponent>(id)) {
            continue;
        }

        auto& input = mEntityManager.template GetComponent<InputComponent>(id);
        if (down) {
            input.keydown.set(scancode);
        } else {
            input.keydown.reset(scancode);
        }
    }
}

template <typename... Components>
void ECSEngine<Components...>::DebugDraw() {
    auto window = mWindowManager.GetWindow();
    if (!window) {
        return;
    }

    for (auto it = mEntityManager.begin(); it != mEntityManager.end(); ++it) {
        if (!it->IsValid()) {
            continue;
        }

        const EntityID id = it->GetID();
        if (!mEntityManager.template HasComponent<CollisionComponent>(id)) {
            continue;
        }

        // Collision boxes are relative to the entity location.
        Rect box = mEntityManager.template GetComponent<CollisionComponent>(id).collisionBox;
        if (mEntityManager.template HasComponent<LocationComponent>(id)) {
            const Point2D& location =
                mEntityManager.template GetComponent<LocationComponent>(id).loc;
            box.topLeft.x += location.x;
            box.topLeft.y += location.y;
        }

        mWindowManager.DebugFrameRect(box, 255, 0, 0, 255);
    }

    // SpriteSystem normally presents the frame; debug drawing happens after it,
    // so present once more to make the overlays visible.
    window->display();
}

}  // namespace ECSEngine
