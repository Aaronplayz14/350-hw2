# Engine APIs and resource usage

This document describes the implementation in this repository. The in-source
API comments give parameters, return values, and local preconditions.

## Ownership and access

`ECSEngine<Components...>` owns the entity, sound, sprite, window, and system
managers. Systems receive a borrowed `EngineView<Components...>*` and use its
getters to access resources. The engine also exposes `GetSystemManager()` for
setup; systems are added before the main loop. A system must not retain a
manager/view beyond the owning engine's lifetime or delete a borrowed object.

| API | Ownership and lifetime contract |
| --- | --- |
| Engine manager getters | Borrowed references valid until engine destruction. Access does not transfer ownership. |
| `EntityManager::CreateEntity` | Returns an integer id, valid for the created entity's lifetime. Removed ids can identify a different entity after reuse. |
| `GetEntity` | Borrowed record pointer, possibly to an invalid slot; null for an out-of-range id. Entity-vector growth can invalidate pointers and iterators. Removal/reuse ends the old entity's identity. |
| Entity `GetName` / `GetType` | Borrowed string reference into the entity record; reacquire after vector growth, removal, or id reuse. Names are type labels and need not be unique. |
| `GetComponent<T>` / storage `Get` | Borrowed reference to a live component. Removal/replacement ends its validity; growth of the same component-type vector can invalidate every reference into that storage. |
| `GetComponentStorage<T>` | Borrowed storage object, valid while the manager exists. Its individual elements have the shorter lifetimes above. |
| `SpriteManager::GetSprite` | Borrowed sprite reference, stable across later registrations because sprites are held indirectly. Valid until manager destruction. |
| `SoundManager::GetSound` | Borrowed sound reference, stable across unrelated registrations. Re-registering the same name or destroying the manager invalidates it. |
| `WindowManager::GetWindow` | A copied `shared_ptr`, sharing ownership of the window object. Closing it affects all users even while references remain. Normal game code lets the engine manage shutdown. |
| Camera/coordinate getters | Independent scalar, point, or rectangle values; changing a returned copy does not change the manager. |

## Entity and component storage

Register every component type in the engine's template parameter list before
using it in systems. `TypeIndex` selects its storage at compile time; asking
for an unregistered type is a compile error. Entity ids and component slots
are separate indices. `AddComponent<T>` replaces an existing value of that
type instead of adding a duplicate; `RemoveEntity` removes all its components.

Iteration includes invalid entity slots. Check `IsValid()` before reading an
entity's components, then check `HasComponent<T>()` for every optional type.
`GetComponent<T>` and storage `Get` require a valid live target; debug builds
assert those preconditions. `GetEntityCount()` counts allocated slots,
including holes; `GetActiveEntityCount()` counts only live entities.

Creation can invalidate entity iterators and grow component storage. Follow
SpawnSystem's pattern: collect spawner ids before creating new entities and
re-fetch components afterward. Do not keep a component reference while adding
another component of that same type. Removing a different slot does not compact
the vectors, but invalidated slots must never be accessed as live components.
The public ComponentStorage vectors are implementation data; change storage
through its methods so its validity flags, free list, and count agree.

## Media managers

`RegisterSound(path, name)` owns both the loaded sound buffer and its `sf::Sound`.
Buffers outlive sounds, and heap storage keeps the buffer addresses stable.
Replacing a name destroys its old sound before its old buffer. `PlaySound`
restarts the named sound; `SetVolume` expects a value in SFML's 0-100 range.
An empty registration name throws `invalid_argument`; a load failure throws
`runtime_error`; requesting an unknown sound throws `out_of_range`.

`RegisterTexture(path, sourceRect)` caches a texture by path and returns a new
`SpriteID` each time. Rectangles are texture-pixel coordinates; a nonpositive
width or height selects the whole image. Textures outlive sprites and do not
move when their map grows. Empty paths, failed loads, and invalid sprite ids
throw `invalid_argument`, `runtime_error`, and `out_of_range`, respectively.
There is no individual sprite-removal API.

SpriteSystem borrows a registered sprite, changes its position/scale for each
entity, draws it immediately, and repeats. Multiple entities may share a
SpriteID, but should not assume the sprite retains their transform afterward.
Components hold ids, not ownership of the corresponding textures or sounds.

## Coordinates and component data

`LocationComponent::loc` is the entity's world-space origin. Sprite bounds and
the current collision box are offsets from that origin. The saved previous
collision box is already in world coordinates. World positions use positive
x to the right and positive y downward. Movement is world units per frame,
acceleration is velocity change per frame, and gameplay timers count frames.
The 60 FPS limit is a target, not a delta-time simulation.

WindowManager's point/rectangle conversions include center translation and
scale; scalar conversions scale lengths only. Set a positive world-units-per-
pixel value before conversions. `SetWorldRect` expects the window's aspect
ratio. Debug draw methods consume window coordinates and do not present the
frame. Normal sprite rendering converts world-space rectangles first;
screen-space score sprites bypass that conversion.

| Component group | Data and consumer |
| --- | --- |
| Location, Movement, Gravity, Respawn | Origin, per-frame velocity/acceleration, respawn location; grouped in LocationComponent.h. `disabledTimer` is currently reserved and unused. |
| Collision | Static/damage tags, contact flags, current local and previous world bounds, star resting status. Input reads the preceding collision pass's flags. |
| Input | Current/previous key bitsets plus dash and wall-slide sound timers. Events update bitsets; InputSystem owns countdown updates. |
| Camera, CameraFollower, CameraShake | Base pose and screen-space bands, non-owning target id, directional shake amplitude/duration. Grouped in CameraComponent.h. |
| Sprite | Non-owning SpriteID, local draw bounds, world/screen-space flag. |
| Score | Score value, numeral SpriteIDs, and three display EntityIDs. ScoreSystem skips missing display entities. |
| Spawn | Spawner EntityID, supported type string, SpriteID, countdown, interval, and remaining event budget. |

## System execution

`SystemManager` takes unique ownership in `AddSystem` and calls `Run` in insertion
order. `kExit` stops the remaining systems and causes the main loop to finish;
`ClearSystems` destroys the registered systems. Do not change the system list
while it is being traversed. Systems do not own resources supplied by the view.

The current game executes the following sequence:

1. Engine event processing updates current/previous key state and debug controls.
2. CollisionLateSystem saves world bounds before movement (the requirements'
   CollisionSystemUpdate role; the historic class name is retained).
3. InputSystem uses old contacts and key transitions to update movement/actions.
4. GravitySystem applies acceleration and limits star downward speed.
5. MovementSystem adds velocity to location.
6. CollisionSystem resolves overlaps, collects stars, updates flags, and triggers effects.
7. SpawnSystem creates ready stars after existing entities' physics pass.
8. ScoreSystem changes numeric display sprite ids.
9. CameraSystem updates horizontal following and samples a fading display shake.
10. SpriteSystem clears, draws, and presents; engine debug overlays can present again.

This documents the actual registration order. The requirements place SpawnSystem
after SpriteSystem; the current game registers it earlier so new stars render
immediately. No order change is made by this documentation pass.

The engine pauses simulation while continuing to poll events; single-step runs
one complete system sequence. Audio is accessed by registered names with
`HasSound` checks in gameplay, so an unavailable optional sound does not stop a
system. Resource loading during setup can warn or throw; a partial map load is
not rolled back. See ABOUT.TXT for current behavior and validation limits.
