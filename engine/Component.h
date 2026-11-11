#pragma once

#include <cassert>
#include <cstddef>
#include <limits>
#include <type_traits>
#include <utility>
#include <vector>

namespace ECSEngine {

namespace detail {

// Maps a component type to its position inside the Components... pack.  This
// lets the EntityManager store one dense ComponentStorage per component type in
// a std::tuple while still looking the storage up by type.
template <typename Query, typename... Types>
struct TypeIndex;

template <typename Query, typename... Rest>
struct TypeIndex<Query, Query, Rest...> : std::integral_constant<std::size_t, 0> {};

template <typename Query, typename First, typename... Rest>
struct TypeIndex<Query, First, Rest...>
    : std::integral_constant<std::size_t, 1 + TypeIndex<Query, Rest...>::value> {};

}  // namespace detail

// Dense storage for a single component type.
//
// Components are stored in mStorage and mValid says whether a slot is alive.
// Removing a component does not shift the remaining components (which would
// invalidate references and every entity's slot index); the slot is put on
// mFreeList and reused by the next add.  This keeps both iteration and removal
// O(1) while avoiding the gaps that would be left by a plain vector.
template <typename T>
class ComponentStorage {
public:
    static constexpr std::size_t kNullIndex = std::numeric_limits<std::size_t>::max();

    std::vector<T> mStorage;
    std::vector<bool> mValid;       // Quickly track if a component is valid
    std::vector<std::size_t> mFreeList;  // Avoid scanning for free slots

    // Stores a copy of component and returns the slot it occupies.  A slot from
    // the free list is reused before the storage grows.
    std::size_t Add(const T& component) { return Insert(component); }

    // Stores a moved component and returns the slot it occupies.
    std::size_t Add(T&& component) { return Insert(std::move(component)); }

    // Marks a slot as dead and makes it available for reuse.  The value itself
    // is left in place so that non-default-constructible components are cheap
    // to store; callers must never read a slot after removing it.
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

    // Returns true when index refers to a live component.
    [[nodiscard]] bool IsValid(std::size_t index) const {
        return index < mValid.size() && mValid[index];
    }

    // Returns the component in a live slot.  The reference remains valid until
    // the component is removed (adding to the free list reuses existing slots;
    // vector growth may move other components, so do not keep references to
    // different slots across Add calls).
    T& Get(std::size_t index) {
        assert(IsValid(index));
        return mStorage[index];
    }

    const T& Get(std::size_t index) const {
        assert(IsValid(index));
        return mStorage[index];
    }

    // Number of live components (not the capacity of mStorage).
    [[nodiscard]] std::size_t Count() const { return mCount; }

    // Number of slots ever allocated, including invalid slots.
    [[nodiscard]] std::size_t Capacity() const { return mStorage.size(); }

    // True when no live component is stored.
    [[nodiscard]] bool Empty() const { return mCount == 0; }

private:
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
