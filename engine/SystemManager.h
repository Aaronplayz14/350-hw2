#pragma once

#include <cstddef>
#include <iostream>
#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace ECSEngine {

enum tSystemReturn { kContinue, kExit };

template <typename... Components>
class EngineView;

//-----------------------------------------------------------------------
/**
 * @brief Base class for every game system.
 *
 * A system receives the engine view so that it can access the resource
 * managers and the entity manager. Systems do not own those resources; the
 * ECSEngine owns them and keeps them alive for the whole game.
 *
 * The manager calls Run once per frame in the order the systems were added.
 */
//-----------------------------------------------------------------------
template <typename... Components>
class System {
public:
    virtual ~System() = default;

    /// @brief Runs one frame of this system.
    ///
    /// Returning kExit asks the SystemManager (and therefore the main loop) to
    /// stop after this system finishes.
    virtual tSystemReturn Run(EngineView<Components...>* scene) = 0;

    /// @brief Borrowed diagnostic name; implementations keep it valid through Run.
    virtual const std::string& GetName() const = 0;
};

//-----------------------------------------------------------------------
/**
 * @brief Runs systems in a fixed, caller-defined order.
 *
 * The execution order is the order in which systems were added. This matters
 * for gameplay: input must update velocity before gravity and movement, and
 * collision resolution must see the same frame's positions. Systems are added
 * before Run() starts and are executed once per frame until one returns
 * kExit.
 */
//-----------------------------------------------------------------------
template <typename... Components>
class SystemManager {
public:
    //-------------------------------------------------------------------
    /**
     * @brief Runs every system once.
     *
     * A null system is skipped so a failed construction does not crash the
     * frame. If any system returns kExit, the remaining systems are not run
     * and the caller is told to stop the game loop.
     *
     * Implemented with the help of AI.
     *
     * @param scene Engine view passed to every system.
     *              Borrowed for this call; systems must not outlive its owner.
     * @return true when the frame completed, false when a system requested exit.
     */
    //-------------------------------------------------------------------
    bool Run(EngineView<Components...>* scene) {
        for (auto& system : mSystems) {
            if (!system) {
                continue;
            }

            if (system->Run(scene) == kExit) {
                std::cout << system->GetName() << " requested exit from system manager\n";
                return false;
            }
        }
        return true;
    }

    //-------------------------------------------------------------------
    /**
     * @brief Takes ownership of a system and appends it to the execution order.
     * @param system System to add; ownership moves into the manager.
     * @return None.
     */
    //-------------------------------------------------------------------
    void AddSystem(std::unique_ptr<System<Components...>>&& system) {
        mSystems.push_back(std::move(system));
    }

    /// @brief Removes every system; the systems are destroyed with the manager.
    void ClearSystems() { mSystems.clear(); }

    /// @brief Number of systems currently registered.
    [[nodiscard]] std::size_t GetSystemCount() const { return mSystems.size(); }

    /// @brief True when no systems are registered.
    [[nodiscard]] bool Empty() const { return mSystems.empty(); }

private:
    std::vector<std::unique_ptr<System<Components...>>> mSystems;
};

}  // namespace ECSEngine
