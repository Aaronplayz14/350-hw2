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

// Base class for every game system.  A system receives the engine view so that
// it can access the resource managers and entities; it must not own those
// resources itself.
template <typename... Components>
class System {
public:
    virtual ~System() = default;

    // Runs one frame of this system.  Returning kExit asks the SystemManager
    // (and therefore the main loop) to stop.
    virtual tSystemReturn Run(EngineView<Components...>* scene) = 0;

    // Name used for diagnostics.
    virtual const std::string& GetName() const = 0;
};

// Runs systems in the order they were added.  Systems are added before Run()
// starts and are executed once per frame until one returns kExit.
template <typename... Components>
class SystemManager {
public:
    // Runs every system once.  Returns false if a system requested an exit.
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

    // Takes ownership of a system and appends it to the execution order.
    void AddSystem(std::unique_ptr<System<Components...>>&& system) {
        mSystems.push_back(std::move(system));
    }

    // Removes every system; the systems are destroyed with the manager.
    void ClearSystems() { mSystems.clear(); }

    [[nodiscard]] std::size_t GetSystemCount() const { return mSystems.size(); }
    [[nodiscard]] bool Empty() const { return mSystems.empty(); }

private:
    std::vector<std::unique_ptr<System<Components...>>> mSystems;
};

}  // namespace ECSEngine
