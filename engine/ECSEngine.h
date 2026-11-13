#pragma once

#include <string>
#include <vector>

#include <SFML/Graphics.hpp>
#include <SFML/System/Sleep.hpp>
#include <SFML/Window.hpp>

#include "CameraComponent.h"
#include "CameraFollowerComponent.h"
#include "CameraShakeComponent.h"
#include "CollisionComponent.h"
#include "EntityManager.h"
#include "InputComponent.h"
#include "LocationComponent.h"
#include "MovementComponent.h"
#include "GravityComponent.h"
#include "RespawnComponent.h"
#include "MathUtil.h"
#include "ScoreComponent.h"
#include "SoundManager.h"
#include "SpawnComponent.h"
#include "SpriteComponent.h"
#include "SpriteManager.h"
#include "SystemManager.h"
#include "WindowManager.h"

namespace ECSEngine {

//-----------------------------------------------------------------------
/**
 * @brief Read/write view of the engine that systems receive.
 *
 * Systems access the engine's managers through these pure virtual getters
 * instead of through globals. That keeps systems testable with a small fake
 * view and leaves open the possibility of hosting the same systems in another
 * engine later.
 */
//-----------------------------------------------------------------------
template <typename... Components>
class EngineView {
public:
    virtual ~EngineView() = default;

    /// @brief Borrows the sound manager while the concrete view owns it.
    virtual SoundManager& GetSoundManager() = 0;
    /// @brief Borrows the sprite manager without transferring texture ownership.
    virtual SpriteManager& GetSpriteManager() = 0;
    /// @brief Borrows storage; individual entity/component lookups may expire sooner.
    virtual EntityManager<Components...>& GetEntityManager() = 0;
    /// @brief Borrows the window and coordinate-conversion manager.
    virtual WindowManager& GetWindowManager() = 0;
};

//-----------------------------------------------------------------------
/**
 * @brief Owns the resource managers and runs the main game loop.
 *
 * ECSEngine creates the window, stores the entity, system, sprite, sound, and
 * window managers, and repeatedly runs the registered systems. The loop polls
 * SFML events (updating every InputComponent), then runs the systems in order.
 * SpriteSystem draws the scene; the engine draws any debug overlays and then
 * presents the completed frame exactly once.
 *
 * Engine-level controls:
 * - Escape closes the window.
 * - F1 toggles collision-box debug drawing.
 * - P pauses or resumes the simulation.
 * - N advances exactly one frame while paused.
 *
 * The engine is non-copyable and non-movable because several managers hold
 * resources whose addresses must remain stable.
 */
//-----------------------------------------------------------------------
template <typename... Components>
class ECSEngine : public EngineView<Components...> {
public:
    ECSEngine(unsigned int width, unsigned int height, const std::string& name);
    ~ECSEngine() override;

    ECSEngine(const ECSEngine&) = delete;
    ECSEngine(ECSEngine&&) = delete;
    ECSEngine& operator=(const ECSEngine&) = delete;
    ECSEngine& operator=(ECSEngine&&) = delete;

    /// @brief Blocks until the window closes, a system returns kExit, or Escape is pressed.
    void Run();

    /// @brief Borrows the sound manager until engine destruction.
    SoundManager& GetSoundManager() override { return mSoundManager; }

    /// @brief Borrows the sprite manager until engine destruction.
    SpriteManager& GetSpriteManager() override { return mSpriteManager; }

    /// @brief Borrows the entity manager until engine destruction.
    EntityManager<Components...>& GetEntityManager() override { return mEntityManager; }

    /// @brief Borrows the window manager until engine destruction.
    WindowManager& GetWindowManager() override { return mWindowManager; }

    /// @brief Borrows the system manager; register systems before entering Run.
    SystemManager<Components...>& GetSystemManager() { return mSystemManager; }

    [[nodiscard]] bool IsDebugEnabled() const { return mDebug; }

private:
    //-------------------------------------------------------------------
    /**
     * @brief Polls events, updates input components, and handles debug keys.
     * @return false when the window has closed or Escape was pressed.
     */
    //-------------------------------------------------------------------
    bool ProcessEvents();

    //-------------------------------------------------------------------
    /**
     * @brief Applies a key transition to every InputComponent in the world.
     * @param scancode SFML scancode of the key.
     * @param down true for key-down, false for key-up.
     * @return None.
     */
    //-------------------------------------------------------------------
    void SetKeyState(unsigned int scancode, bool down);

    //-------------------------------------------------------------------
    /**
     * @brief Draws collision boxes when debug mode is on.
     *
     * Collision boxes are stored relative to the entity's location, so the
     * entity's LocationComponent (when present) is added before the box is
     * converted to window coordinates and drawn before the frame is presented.
     * @return None.
     */
    //-------------------------------------------------------------------
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

//-----------------------------------------------------------------------
/**
 * @brief Opens the window and prepares the engine for its main loop.
 *
 * Creates the window through WindowManager, disables key repeat so each
 * physical press produces one key-down event, and caps the loop at 30 frames
 * per second. Window creation happens before any systems are added, so a
 * failed window is visible immediately rather than after setup.
 *
 * Implemented with the help of AI.
 *
 * @param width Window width in pixels.
 * @param height Window height in pixels.
 * @param name Title shown in the window's title bar.
 */
//-----------------------------------------------------------------------
template <typename... Components>
ECSEngine<Components...>::ECSEngine(unsigned int width, unsigned int height,
                                    const std::string& name)
    : mWindowManager(width, height, name) {
    if (auto window = mWindowManager.GetWindow()) {
        // One key-down event per physical press is what InputComponent expects.
        window->setKeyRepeatEnabled(false);
        window->setFramerateLimit(30);
    }
}

//-----------------------------------------------------------------------
/**
 * @brief Closes the window if it is still open.
 *
 * SFML tears the window down on its own, but closing it explicitly releases
 * the drawing surface before the manager destructors run.
 */
//-----------------------------------------------------------------------
template <typename... Components>
ECSEngine<Components...>::~ECSEngine() {
    if (auto window = mWindowManager.GetWindow()) {
        if (window->isOpen()) {
            window->close();
        }
    }
}

//-----------------------------------------------------------------------
/**
 * @brief Runs the main game loop until the window closes.
 *
 * Each iteration polls events first so the input state is current before any
 * system runs. A paused engine keeps polling events (so it can be resumed) but
 * does not advance the simulation unless N requested a single step. Systems
 * run in the order established by the game setup; if a system returns kExit
 * the loop stops. When debug mode is on, collision boxes are drawn after the
 * normal render pass.
 *
 * Implemented with the help of AI.
 *
 * @return None. Blocks until the window is closed or the game exits.
 */
//-----------------------------------------------------------------------
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
        window->display();
    }
}

//-----------------------------------------------------------------------
/**
 * @brief Polls all pending events and updates input components.
 *
 * At the start of the frame every InputComponent copies keydown into
 * lastKeydown. Systems can then compare the two bitsets to detect a key that
 * was pressed this frame rather than held from an earlier frame.
 *
 * The event loop handles window close, key-down, and key-up. Key-down and
 * key-up are broadcast to every entity that owns an InputComponent, while the
 * engine's own debugging keys are handled once. Key repeat is disabled by the
 * constructor, so a held key is not repeatedly toggled.
 *
 * Implemented with the help of AI.
 *
 * @return false when the window closed or Escape was pressed; true otherwise.
 */
//-----------------------------------------------------------------------
template <typename... Components>
bool ECSEngine<Components...>::ProcessEvents() {
    auto window = mWindowManager.GetWindow();
    if (!window) {
        return false;
    }

    // Remember this frame's starting key state before applying new events. The
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

//-----------------------------------------------------------------------
/**
 * @brief Applies a key transition to every InputComponent in the world.
 *
 * Implemented with the help of AI.
 *
 * Each entity has its own input bitset so multiple controllable entities would
 * see the same physical keyboard state. The scancode is bounds-checked by the
 * caller before this function runs.
 *
 * @param scancode SFML scancode of the key.
 * @param down true to set the bit, false to clear it.
 * @return None.
 */
//-----------------------------------------------------------------------
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

//-----------------------------------------------------------------------
/**
 * @brief Draws collision boxes over the current frame when debug mode is on.
 *
 * A CollisionComponent's box is relative to its entity, so the entity's
 * LocationComponent is added first, then WindowManager converts the world
 * bounds using the same camera as SpriteSystem. Run presents the scene and
 * these overlays together, so F1 does not add another frame-rate-limit wait.
 *
 * Implemented with the help of AI.
 */
//-----------------------------------------------------------------------
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

        // WindowManager's debug API now accepts world bounds and converts
        // them internally, matching Travis's latest implementation.
        mWindowManager.DebugFrameRect(box, 255, 0, 0, 255);
    }

}

}  // namespace ECSEngine
