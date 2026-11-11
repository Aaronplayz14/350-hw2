#pragma once

#include <cstddef>
#include <string>

// An entity is a lightweight handle: it owns an id, a display/type name and a
// validity bit.  All component data lives in the EntityManager's component
// storages, so copying an Entity never copies gameplay data.
//
// Entity ids are stable indices into the EntityManager's entity vector.  A
// removed entity leaves an invalid slot behind; iterators therefore visit both
// live and dead entities and callers must check IsValid() (or use the manager's
// IsValid(EntityID)) before touching a component.

namespace ECSEngine {

using EntityID = std::size_t;

class Entity {
public:
    Entity(const Entity&) = default;
    Entity(Entity&&) noexcept = default;
    Entity& operator=(const Entity&) = default;
    Entity& operator=(Entity&&) noexcept = default;

    EntityID GetID() const { return mID; }

    // The name supplied when the entity was created.  The manager assumes that
    // names are unique, but does not enforce it.
    const std::string& GetName() const { return mType; }

    // Alias kept for code that calls the name the entity's type.
    const std::string& GetType() const { return mType; }

    // False once the entity has been removed.  Removed ids may be reused by a
    // later CreateEntity call.
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
