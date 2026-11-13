#pragma once

#include <cstddef>
#include <string>

namespace ECSEngine {

using EntityID = std::size_t;

//-----------------------------------------------------------------------
/**
 * @brief Lightweight handle to an entity managed by EntityManager.
 *
 * An Entity owns only an id, a display/type name, and a validity bit. All
 * gameplay data lives in the EntityManager's component storages, so copying an
 * Entity never copies components or sprites.
 *
 * Entity ids are stable indices into the manager's entity vector. Removing an
 * entity marks its slot invalid instead of erasing it, which keeps every
 * existing id a valid index. Iterators therefore visit both live and dead
 * slots, and callers must check IsValid() (or EntityManager::IsValid) before
 * reading a component.
 *
 * Construction is private because only EntityManager may create handles;
 * gameplay code receives an EntityID and asks the manager for the data.
 */
//-----------------------------------------------------------------------
class Entity {
public:
    Entity(const Entity&) = default;
    Entity(Entity&&) noexcept = default;
    Entity& operator=(const Entity&) = default;
    Entity& operator=(Entity&&) noexcept = default;

    /// @brief Stable id used to look the entity up in the manager.
    EntityID GetID() const { return mID; }

    /// @brief Name supplied when the entity was created.
    ///
    /// The manager assumes names are unique, but does not enforce it; names are
    /// intended for debugging and for distinguishing objects such as "player"
    /// and "star".
    const std::string& GetName() const { return mType; }

    /// @brief Alias kept for code that calls the name the entity's type.
    const std::string& GetType() const { return mType; }

    /// @brief False once the entity has been removed.
    ///
    /// A removed id may be reused by a later CreateEntity call, at which point
    /// the slot becomes valid again with the new entity's name and no
    /// components.
    bool IsValid() const { return mValid; }

private:
    Entity() = default;
    Entity(EntityID eid, const std::string& name) : mID(eid), mType(name) {}
    EntityID mID = 0;
    std::string mType;
    bool mValid = true;

    template <typename... T>
    friend class EntityManager;
};

}  // namespace ECSEngine
