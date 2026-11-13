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

//-----------------------------------------------------------------------
/**
 * @brief Owns every entity and every component in the ECS world.
 *
 * Entity ids are indices into mEntities. Components are stored densely, one
 * ComponentStorage per component type, and mEntityComponents maps each entity
 * to one component slot per type. Removing an entity leaves an invalid slot in
 * mEntities and puts its id on a free list; iterators still visit the invalid
 * slot, so callers must check Entity::IsValid() (or EntityManager::IsValid)
 * before using an entity.
 *
 * The manager deliberately separates entity lifetime from component lifetime:
 * components are removed independently, and adding a component to an entity
 * that already has one replaces the old value instead of duplicating it.
 *
 * Typical iteration:
 *
 *     for (auto it = manager.begin(); it != manager.end(); ++it) {
 *         if (!it->IsValid()) continue;
 *         EntityID id = it->GetID();
 *         if (!manager.HasComponent<LocationComponent>(id)) continue;
 *         auto& location = manager.GetComponent<LocationComponent>(id);
 *         // ...
 *     }
 *
 * Implemented with the help of AI.
 */
//-----------------------------------------------------------------------
template <typename... Components>
class EntityManager {
public:
    static constexpr std::size_t kNullComponent = std::numeric_limits<std::size_t>::max();

    using tEntity = std::vector<Entity>;
    using const_iterator = tEntity::const_iterator;

    //-------------------------------------------------------------------
    /**
     * @brief Creates a live entity and returns its id.
     *
     * Ids from previously removed entities are reused before the entity vector
     * grows. A reused slot starts with no components so stale component links
     * cannot leak into the new object. The name is stored on the Entity and can
     * be retrieved with Entity::GetName().
     *
     * Implemented with the help of AI.
     *
     * @param name Display/type name for the entity.
     * @return Id valid for this entity's lifetime; removed ids may be reused.
     */
    //-------------------------------------------------------------------
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

    //-------------------------------------------------------------------
    /**
     * @brief Removes an entity and every component it owns.
     *
     * The entity's slot is kept and marked invalid so existing ids remain
     * valid indices. The id is pushed onto the free list for CreateEntity to
     * reuse.
     *
     * Implemented with the help of AI.
     *
     * @param entity Id of the entity to remove.
     * @return None.
     */
    //-------------------------------------------------------------------
    void RemoveEntity(EntityID entity) {
        assert(IsValid(entity));
        if (!IsValid(entity)) {
            return;
        }

        // Remove every component the entity owns. The comma fold is a no-op
        // for an empty Components pack.
        (RemoveComponent<Components>(entity), ...);

        mEntityComponents[entity].fill(kNullComponent);
        mEntities[entity].mValid = false;
        mFreeEntityIDs.push_back(entity);
    }

    //-------------------------------------------------------------------
    /**
     * @brief Adds a component to an entity, replacing any existing one.
     *
     * Replacing a component reuses the component storage's free list instead
     * of growing it, so repeated AddComponent calls do not leak slots.
     *
     * Implemented with the help of AI.
     *
     * @param entity Target entity; must be alive.
     * @param component Component value to copy or move into storage.
     * @return None.
     */
    //-------------------------------------------------------------------
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

    //-------------------------------------------------------------------
    /**
     * @brief Removes a component from an entity.
     *
     * Does nothing when the entity is invalid or does not own that component,
     * which makes cleanup code safe to run without extra HasComponent checks.
     *
     * @param entity Target entity.
     * @return None.
     */
    //-------------------------------------------------------------------
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

    //-------------------------------------------------------------------
    /**
     * @brief Returns true when entity is alive and owns component T.
     * @param entity Entity id to test.
     * @return true only when both the entity and the component slot are valid.
     */
    //-------------------------------------------------------------------
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

    //-------------------------------------------------------------------
    /**
     * @brief Returns the component owned by an entity.
     *
     * The reference stays valid until that component is removed, but may be
     * invalidated by operations that grow its storage. In debug builds the
     * assertions catch an invalid entity or a missing component.
     *
     * @param entity Entity that owns the component.
     * @return Mutable reference to the stored component.
     */
    //-------------------------------------------------------------------
    template <typename T>
    T& GetComponent(EntityID entity) {
        assert(IsValid(entity));
        assert(HasComponent<T>(entity));

        using StoredType = std::remove_cvref_t<T>;
        constexpr std::size_t index = detail::TypeIndex<StoredType, Components...>::value;
        const std::size_t slot = mEntityComponents[entity][index];
        return std::get<index>(mComponentStorages).Get(slot);
    }

    /// @brief Const overload of GetComponent(); the entity must own the component.
    template <typename T>
    const T& GetComponent(EntityID entity) const {
        assert(IsValid(entity));
        assert(HasComponent<T>(entity));

        using StoredType = std::remove_cvref_t<T>;
        constexpr std::size_t index = detail::TypeIndex<StoredType, Components...>::value;
        const std::size_t slot = mEntityComponents[entity][index];
        return std::get<index>(mComponentStorages).Get(slot);
    }

    //-------------------------------------------------------------------
    /**
     * @brief Direct access to one component storage.
     *
     * Useful for systems that want to iterate live components without first
     * walking every entity. The storage reference is owned by this manager and
     * remains valid for the manager's lifetime; individual component
     * references may move when the storage grows.
     *
     * @return Reference to the dense storage for T.
     */
    //-------------------------------------------------------------------
    template <typename T>
    ComponentStorage<T>& GetComponentStorage() {
        constexpr std::size_t index = detail::TypeIndex<T, Components...>::value;
        return std::get<index>(mComponentStorages);
    }

    /// @brief Const overload of GetComponentStorage().
    template <typename T>
    const ComponentStorage<T>& GetComponentStorage() const {
        constexpr std::size_t index = detail::TypeIndex<T, Components...>::value;
        return std::get<index>(mComponentStorages);
    }

    /// @brief Returns true when the id refers to a live entity.
    [[nodiscard]] bool IsValid(EntityID entity) const {
        return entity < mEntities.size() && mEntities[entity].IsValid();
    }

    /// @brief Lower-case alias matching the lab EntityManager API.
    [[nodiscard]] bool ValidEntity(EntityID entity) const { return IsValid(entity); }

    //-------------------------------------------------------------------
    /**
     * @brief Returns the entity record for an id, including invalid entities.
     * @param entity Entity id to look up.
     * @return Borrowed pointer, or nullptr when the id was never allocated.
     *         CreateEntity may invalidate pointers and iterators by growing
     *         the entity vector. Removal/reuse ends the old entity's lifetime.
     */
    //-------------------------------------------------------------------
    [[nodiscard]] Entity* GetEntity(EntityID entity) {
        return entity < mEntities.size() ? &mEntities[entity] : nullptr;
    }

    /// @brief Const overload of GetEntity().
    [[nodiscard]] const Entity* GetEntity(EntityID entity) const {
        return entity < mEntities.size() ? &mEntities[entity] : nullptr;
    }

    /// @brief Number of slots in the entity vector, including reusable slots.
    [[nodiscard]] std::size_t GetEntityCount() const { return mEntities.size(); }

    /// @brief Number of live entities, excluding removed slots.
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
