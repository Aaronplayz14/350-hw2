# Project 2 requirements and rubric check

Reviewed against the supplied Project 2 part-1 writeup and the rubric quoted
in chat. This records implementation evidence and verification limits, not a
guaranteed grade. The latest reviewed main update includes Travis's system-order
fix (`186f043`), window aspect-ratio fix (`1f7b154`), and movement tuning
(`504491d`), plus world-space debug drawing (`9f652d8`); this review preserves
those implementations and movement values.

## Rubric coverage

| Category | Implementation evidence | Verification / remaining work |
| --- | --- | --- |
| Resource managers | SoundManager, SpriteManager, WindowManager, and EntityManager are engine-owned. Systems borrow them through EngineView. Sound buffers/textures outlive sounds/sprites; heap ownership keeps media references stable; entity/component slots are reused. | Ownership and lifetime rules reviewed; existing resource/core checks pass. This is not a general memory-leak certification. Lab-machine verification remains external. |
| Game components | All 11 named rubric components have dedicated definition headers; RespawnComponent from the full assignment also has its own header. Components store values and non-owning resource ids. | All 12 headers compile independently with warnings treated as errors. Splitting headers does not change component data layouts or movement values. |
| Gameplay systems | Every named system has a separate Run function, except ProcessEvents, which is its own engine function. The pre-movement snapshot implements the CollisionSystemUpdate role under the historic name CollisionLateSystem. | Current registration follows Travis's latest required order; system order/exit and physics checks pass. Collision detection is a pairwise pass, not a spatially partitioned solver. |
| Gameplay behavior | Supplied sky.map/world.map load; player input, jumps/wall jumps, slow wall slides, stronger downward gravity, fast fall, variable jump release, dash, star spawning/bounces/collection, score sprites, event sounds, camera following/shake, and spike respawning are implemented. | Automated collision/input/score checks pass. A complete playthrough confirming that the first puzzle needs jumps and the second needs dashes remains an interactive check. |
| Documentation | Doxygen-style API comments, resource lifetime/access rules, README instructions, and ABOUT.TXT implementation/team/AI credits are present. Current dash timings and header organization are documented. | Reviewed against current source. Trav's recent tuning, wall-cling, order, and window API contributions are included in ABOUT.TXT. |
| Compilation | C++20 project builds locally with its configured warning flags; isolated component headers also compile with -Werror. | Local build emitted no compiler warnings/errors, and ECSTest passed. Course lab compilation/execution is not verified here. |

The quoted rubric says "11 systems" but names 10 entries, as does the supplied
writeup's system list. All 10 named roles are implemented; this checklist does
not invent an extra system to reconcile that numbering discrepancy. The
quoted component list has 11 entries, while the longer writeup also requires
RespawnComponent; both lists are covered.

## Dedicated component headers

| Header | Component data |
| --- | --- |
| `CollisionComponent.h` | Static/damage/contact bits, local/current and previous world collision bounds, star rest status |
| `CameraComponent.h` | Base camera pose, scale, follow bands and easing fraction |
| `CameraFollowerComponent.h` | Non-owning target EntityID |
| `CameraShakeComponent.h` | Remaining/total duration and directional amplitude |
| `InputComponent.h` | Current/previous key bitsets and action timers |
| `LocationComponent.h` | World origin |
| `MovementComponent.h` | Per-frame velocity |
| `GravityComponent.h` | Per-frame acceleration and reserved timer |
| `ScoreComponent.h` | Numeric score, numeral SpriteIDs and display EntityIDs |
| `SpawnComponent.h` | Owner/type/SpriteID, countdown, interval and remaining spawn budget |
| `SpriteComponent.h` | SpriteID, relative bounds and world/screen-space flag |
| `RespawnComponent.h` | World-space respawn origin (additional full-writeup component) |

`CameraFollower` and `CameraShake` remain aliases for the corresponding
`...Component` types. The old LocationComponent.h and CameraComponent.h imports
remain compatible, but the moved definitions each live in their own file.

## Detailed behavior check

- Arrow movement accelerates up to a cap, including while airborne. Z uses a
  fresh press for ground or falling wall jumps; releasing it cuts upward
  movement. Wall jumping still requires pressing into the contacted wall.
- Gravity is stronger on descent, and Down adds fast-fall acceleration.
  Wall-slide slowdown and its sound are triggered while pressing into a wall.
- Travis's current dash cooldown is 30 frames, its duration is 15 frames, and
  the wall-slide cap is 3 before gravity. At the 30 FPS cap the dash cooldown
  is about one second. Physics and timers remain frame-based, not delta-time.
- Star collisions add 10 points and remove the collected star. Numeral sprites
  display the score at the top left, clamped to the three display digits.
- The map has one S spawner. It emits stars in random directions with a finite
  five-event budget; stars bounce with energy loss and settle on floors.
  The spawner is solid for the player but ignored by stars.
- The extra test star now starts at (576, 192), outside the left boundary wall
  at x=640..704. Its ordinary gravity/bounce behavior is retained. Both supplied
  map files and all their dictionary/layout data remain unchanged.
- Spike/saw hitboxes retain the corrected tile clipping. Actual contact
  respawns the player; merely sliding beside the wall does not hit an invisible
  overhang. Regression cases cover spike contact from several directions.
- The camera retains Travis's fixed base y and horizontal hard/soft bands.
  Directional impact shake fades independently of that base position. Exact
  visual edge behavior and shake feel still merit interactive play checks.
- SpriteSystem draws the scene, then the engine passes world bounds to
  WindowManager, which converts and draws the F1 outlines. The engine presents
  once; there is one display() call in the frame loop,
  so toggling F1 no longer introduces a second limiter wait. Native-window
  automation could not attach to the raw game executable; visual F1 alignment
  was not interactively certified in this review.

## Validation and handoff

Local checks completed:

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build --parallel 2
./build/engine/ECSTest
git diff --check
```

Each component header was additionally compiled on its own with -Werror using
the project's real compiler/include settings. Source comparisons confirmed
no executable changes to InputSystem, GravitySystem, MovementSystem, or the
latest WindowManager implementation, and no changes to the system registration
order or supplied map files. The local game starts and reports both maps loaded
without asset-load warnings. Intentional missing-file messages in ECSTest are
resource-error test cases, not failed checks.

Remaining external checks: build/run on the lab machines, play both traversal
puzzles with the preserved movement tuning, and listen/check the live camera,
shake, and event sounds. The local automated checks do not prove all possible
high-speed/corner collisions or every complete level route.

`project_reqirements.txt` is a local reference and is excluded from the commit.
