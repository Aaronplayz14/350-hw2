#pragma once

#include <cassert>
#include <cstddef>
#include <limits>
#include <type_traits>
#include <utility>
#include <vector>

namespace ECSEngine {

//-----------------------------------------------------------------------
/**
 * @brief Component storage and compile-time component type lookup for the ECS.
 *
 * EntityManager owns one ComponentStorage<T> for every registered component
 * type. This file supplies both the storage itself and TypeIndex, which maps a
 * component type to its position in the manager's type pack.
 *
 * Implemented with the help of AI.
 */
//-----------------------------------------------------------------------
//-----------------------------------------------------------------------
/**
 * @brief Compile-time lookup of a component type in the engine's type list.
 *
 * EntityManager stores one ComponentStorage per component type in a tuple.
 * A system asks for a component by type, so the manager needs to turn that
 * type into a tuple index. TypeIndex walks the Components... pack and stops
 * when Query matches First; the recursion therefore produces the zero-based
 * position of the requested type.
 *
 * A type that is not in the pack reaches the incomplete primary template and
 * fails to compile, which catches an accidental use of a component that was
 * never registered with the engine.
 */
//-----------------------------------------------------------------------
namespace detail {

template <typename Query, typename... Types>
struct TypeIndex;

template <typename Query, typename... Rest>
struct TypeIndex<Query, Query, Rest...> : std::integral_constant<std::size_t, 0> {};

template <typename Query, typename First, typename... Rest>
struct TypeIndex<Query, First, Rest...>
    : std::integral_constant<std::size_t, 1 + TypeIndex<Query, Rest...>::value> {};

}  // namespace detail

//-----------------------------------------------------------------------
/**
 * @brief Dense storage for one component type.
 *
 * Components live in mStorage and mValid records whether a slot is alive.
 * Removing a component does not shift the remaining components, because every
 * entity stores the slot index of each of its components. Shifting would make
 * those indices stale and would invalidate references held by systems. Instead
 * the slot is marked invalid and pushed onto mFreeList; the next Add reuses the
 * slot. That gives O(1) add, remove, and lookup while avoiding the permanent
 * gaps a plain vector would leave behind.
 *
 * The value in a removed slot is reset when T is default-constructible so that
 * resources held by the old value are released promptly. Callers must never
 * read a slot after it has been removed.
 *
 * Implemented with the help of AI.
 */
//-----------------------------------------------------------------------
template <typename T>
class ComponentStorage {
public:
    static constexpr std::size_t kNullIndex = std::numeric_limits<std::size_t>::max();

    std::vector<T> mStorage;
    std::vector<bool> mValid;            // Quickly track if a component is valid
    std::vector<std::size_t> mFreeList;  // Avoid scanning for free slots

    //-------------------------------------------------------------------
    /**
     * @brief Stores a copy of a component.
     * @param component Component value to copy into the storage.
     * @return The slot the new component occupies.
     */
    //-------------------------------------------------------------------
    std::size_t Add(const T& component) { return Insert(component); }

    //-------------------------------------------------------------------
    /**
     * @brief Stores a component by moving it into the storage.
     * @param component Component value to move into the storage.
     * @return The slot the new component occupies.
     */
    //-------------------------------------------------------------------
    std::size_t Add(T&& component) { return Insert(std::move(component)); }

    //-------------------------------------------------------------------
    /**
     * @brief Marks a slot as dead and makes it available for reuse.
     *
     * The slot is not erased from mStorage, so existing indices stay stable.
     * When T can be default-constructed, the old value is replaced with a
     * default value to release any resources it owned.
     *
     * Implemented with the help of AI.
     *
     * @param index Slot previously returned by Add.
     * @return None.
     */
    //-------------------------------------------------------------------
    void Remove(std::size_t index) {
        assert(IsValid(index));
        mValid[index] = false;
        // Release any resources held by the old value when the type allows it.
        if constexpr (std::is_default_constructible_v<T>) {
            mStorage[index] = T{};
        }
        mFreeList.push_back(index);
        --mCount;
    }

    //-------------------------------------------------------------------
    /**
     * @brief Returns true when index refers to a live component.
     * @param index Slot to test.
     * @return true if the slot is in range and currently occupied.
     */
    //-------------------------------------------------------------------
    [[nodiscard]] bool IsValid(std::size_t index) const {
        return index < mValid.size() && mValid[index];
    }

    //-------------------------------------------------------------------
    /**
     * @brief Returns the component in a live slot.
     *
     * Removal or replacement of this component invalidates the reference.
     * Adding another component of the same type may grow this storage's
     * vector and invalidate all references into it. Other component types
     * have separate storage and do not move this vector.
     *
     * @param index Slot previously returned by Add.
     * @return Reference to the stored component.
     */
    //-------------------------------------------------------------------
    T& Get(std::size_t index) {
        assert(IsValid(index));
        return mStorage[index];
    }

    /// @brief Const overload of Get(); the slot must already be live.
    const T& Get(std::size_t index) const {
        assert(IsValid(index));
        return mStorage[index];
    }

    /// @brief Number of live components, not the capacity of mStorage.
    [[nodiscard]] std::size_t Count() const { return mCount; }

    /// @brief Number of slots ever allocated, including invalid slots.
    [[nodiscard]] std::size_t Capacity() const { return mStorage.size(); }

    /// @brief True when no live component is stored.
    [[nodiscard]] bool Empty() const { return mCount == 0; }

private:
    //-------------------------------------------------------------------
    /**
     * @brief Shared implementation for the copy and move Add overloads.
     *
     * Reuses the most recently freed slot when one exists; otherwise appends
     * a new slot. mCount tracks live slots rather than vector size, so it is
     * updated here rather than derived from mStorage.size().
     *
     * Implemented with the help of AI.
     *
     * @param component Component to copy or move into the chosen slot.
     * @return The slot the component occupies.
     */
    //-------------------------------------------------------------------
    template <typename U>
    std::size_t Insert(U&& component) {
        std::size_t index;
        if (!mFreeList.empty()) {
            index = mFreeList.back();
            mFreeList.pop_back();
            mStorage[index] = std::forward<U>(component);
            mValid[index] = true;
        } else {
            index = mStorage.size();
            mStorage.push_back(std::forward<U>(component));
            mValid.push_back(true);
        }
        ++mCount;
        return index;
    }

    std::size_t mCount = 0;
};

}  // namespace ECSEngine
