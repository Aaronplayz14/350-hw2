#pragma once

#include <array>
#include <cassert>
#include <cstddef>
#include <limits>
#include <string>
#include <tuple>
#include <type_traits>
#include <utility>
#include <vector>

#include "Component.h"
#include "Entity.h"

namespace ECSEngine {

// Owns every entity and every component in the ECS world.
//
// Entity ids are indices into mEntities.  Components are stored densely, one
// ComponentStorage per type, and mEntityComponents maps (entity, component
// type) to the component's slot.  Removing an entity leaves an invalid slot in
// mEntities and puts its id on a free list; iterators still visit the invalid
// slot, so callers must check Entity::IsValid() (or EntityManager::IsValid)
// before using an entity.
//
// Typical iteration:
//
//     for (auto it = manager.begin(); it != manager.end(); ++it) {
//         if (!it->IsValid()) continue;
//         EntityID id = it->GetID();
//         if (!manager.HasComponent<LocationComponent>(id)) continue;
//         auto& location = manager.GetComponent<LocationComponent>(id);
//         // ...
//     }
template <typename... Components>
class EntityManager {
public:
    static constexpr std::size_t kNullComponent = std::numeric_limits<std::size_t>::max();

    using tEntity = std::vector<Entity>;
    using const_iterator = tEntity::const_iterator;

    // Creates a live entity and returns its id.  Ids from previously removed
    // entities are reused before the entity vector grows.  The name is stored
    // on the Entity and can be retrieved with Entity::GetName().
    [[nodiscard]] EntityID CreateEntity(const std::string& name) {
        EntityID id;
        if (!mFreeEntityIDs.empty()) {
            id = mFreeEntityIDs.back();
            mFreeEntityIDs.pop_back();
            mEntities[id] = Entity(id, name);
        } else {
            id = mEntities.size();
            mEntities.push_back(Entity(id, name));
            mEntityComponents.emplace_back();
        }

        // A reused entity starts with no components.
        mEntityComponents[id].fill(kNullComponent);
        return id;
    }

    // Removes the entity and all of its components.  The entity's slot is kept
    // (marked invalid) so that existing ids remain valid indices.
    void RemoveEntity(EntityID entity) {
        assert(IsValid(entity));
        if (!IsValid(entity)) {
            return;
        }

        // Remove every component the entity owns.  The comma fold is a no-op
        // for an empty Components pack.
        (RemoveComponent<Components>(entity), ...);

        mEntityComponents[entity].fill(kNullComponent);
        mEntities[entity].mValid = false;
        mFreeEntityIDs.push_back(entity);
    }

    // Adds (or replaces) component on entity.  Replacing a component reuses the
    // component storage's free list instead of growing it.
    template <typename T>
    void AddComponent(EntityID entity, T component) {
        assert(IsValid(entity));
        if (!IsValid(entity)) {
            return;
        }

        using StoredType = std::remove_cvref_t<T>;
        constexpr std::size_t index = detail::TypeIndex<StoredType, Components...>::value;
        auto& storage = std::get<index>(mComponentStorages);
        std::size_t& slot = mEntityComponents[entity][index];

        if (slot != kNullComponent) {
            storage.Remove(slot);
            slot = kNullComponent;
        }

        slot = storage.Add(std::move(component));
    }

    // Removes component from entity.  Does nothing when the entity does not
    // have that component.
    template <typename T>
    void RemoveComponent(EntityID entity) {
        if (!IsValid(entity)) {
            return;
        }

        using StoredType = std::remove_cvref_t<T>;
        constexpr std::size_t index = detail::TypeIndex<StoredType, Components...>::value;
        std::size_t& slot = mEntityComponents[entity][index];
        if (slot == kNullComponent) {
            return;
        }

        std::get<index>(mComponentStorages).Remove(slot);
        slot = kNullComponent;
    }

    // Returns true when entity is alive and owns component T.
    template <typename T>
    [[nodiscard]] bool HasComponent(EntityID entity) const {
        if (!IsValid(entity)) {
            return false;
        }

        using StoredType = std::remove_cvref_t<T>;
        constexpr std::size_t index = detail::TypeIndex<StoredType, Components...>::value;
        const std::size_t slot = mEntityComponents[entity][index];
        return slot != kNullComponent && std::get<index>(mComponentStorages).IsValid(slot);
    }

    // Returns the component owned by entity.  The reference stays valid until
    // that component is removed, but may be invalidated by operations on other
    // component types if their storage has to grow.
    template <typename T>
    T& GetComponent(EntityID entity) {
        assert(IsValid(entity));
        assert(HasComponent<T>(entity));

        using StoredType = std::remove_cvref_t<T>;
        constexpr std::size_t index = detail::TypeIndex<StoredType, Components...>::value;
        const std::size_t slot = mEntityComponents[entity][index];
        return std::get<index>(mComponentStorages).Get(slot);
    }

    template <typename T>
    const T& GetComponent(EntityID entity) const {
        assert(IsValid(entity));
        assert(HasComponent<T>(entity));

        using StoredType = std::remove_cvref_t<T>;
        constexpr std::size_t index = detail::TypeIndex<StoredType, Components...>::value;
        const std::size_t slot = mEntityComponents[entity][index];
        return std::get<index>(mComponentStorages).Get(slot);
    }

    // Direct access to a component storage, useful for systems that want to
    // iterate live components without first walking every entity.  The returned
    // reference is owned by this manager and is valid for the manager's
    // lifetime; individual component references may move when the storage grows.
    template <typename T>
    ComponentStorage<T>& GetComponentStorage() {
        constexpr std::size_t index = detail::TypeIndex<T, Components...>::value;
        return std::get<index>(mComponentStorages);
    }

    template <typename T>
    const ComponentStorage<T>& GetComponentStorage() const {
        constexpr std::size_t index = detail::TypeIndex<T, Components...>::value;
        return std::get<index>(mComponentStorages);
    }

    // Returns true when entity is alive (its component slots are meaningful).
    [[nodiscard]] bool IsValid(EntityID entity) const {
        return entity < mEntities.size() && mEntities[entity].IsValid();
    }

    // Lower-case alias matching the lab EntityManager API.
    [[nodiscard]] bool ValidEntity(EntityID entity) const { return IsValid(entity); }

    // Returns the entity for an id, including invalid entities, or nullptr when
    // the id has never been allocated.
    [[nodiscard]] Entity* GetEntity(EntityID entity) {
        return entity < mEntities.size() ? &mEntities[entity] : nullptr;
    }

    [[nodiscard]] const Entity* GetEntity(EntityID entity) const {
        return entity < mEntities.size() ? &mEntities[entity] : nullptr;
    }

    // Number of slots in the entity vector (live plus invalid/reusable slots).
    [[nodiscard]] std::size_t GetEntityCount() const { return mEntities.size(); }

    // Number of live entities.
    [[nodiscard]] std::size_t GetActiveEntityCount() const {
        std::size_t count = 0;
        for (const Entity& entity : mEntities) {
            if (entity.IsValid()) {
                ++count;
            }
        }
        return count;
    }

    const_iterator cbegin() const { return mEntities.begin(); }
    const_iterator cend() const { return mEntities.end(); }
    const_iterator begin() const { return mEntities.begin(); }
    const_iterator end() const { return mEntities.end(); }
    const_iterator begin() { return mEntities.begin(); }
    const_iterator end() { return mEntities.end(); }

private:
    using ComponentIndexArray = std::array<std::size_t, sizeof...(Components)>;

    tEntity mEntities;
    // Parallel to mEntities: maps each entity to one slot per component type.
    std::vector<ComponentIndexArray> mEntityComponents;
    std::tuple<ComponentStorage<Components>...> mComponentStorages;
    std::vector<EntityID> mFreeEntityIDs;
};

}  // namespace ECSEngine
