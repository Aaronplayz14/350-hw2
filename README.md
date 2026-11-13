# CMPUT 350 HW 2

Project 2 platformer built with a C++20 entity-component-system engine and
SFML 3.1.0. The game loads `assets/sky.map` for the background and
`assets/world.map` for the harder playable level.

## Build and run

Run these commands from the repository root. CMake 3.28 or newer and a C++20
compiler are required. The version-8 `CMakePresets.json` is also supported by
CMake 3.28 or newer; the commands below work without using presets.

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build --parallel 2
./build/platformer/platformer -path ./build/assets
```

CMake copies `assets/` into the build directory during configuration. Run the
configure command again after adding or changing assets. An explicit `-path`
avoids dependence on the launch directory; the game's default is `../assets/`.
Use a quoted path if the resource directory contains spaces.

On Linux x86-64 with GNU 9.4.0, the project imports the bundled lab SFML
libraries from `extern/sfml-3.1.0-linux-x86_64-ubuntu20.04`. Keep the entire
bundle and its existing relative directory layout. Other configurations fetch
and build SFML 3.1.0, so their initial configuration needs network access and
the platform's compiler, graphics, and audio development dependencies.

## Controls

| Key | Action |
| --- | --- |
| Left / Right | Accelerate horizontally, including in the air |
| Z | Jump; release early for a shorter jump |
| Z while pressing into a wall and falling | Wall jump |
| C | Dash with a 30-frame cooldown (roughly one second at 30 FPS) |
| Down | Faster falling |
| Escape | Close the game |
| F1 | Toggle collision-box debug drawing |
| P | Pause/resume simulation |
| N | Advance one simulation frame and remain paused |

Movement and timers are frame-based, with a target of 30 FPS. One spawner
creates stars with random initial directions. Stars pass through its tile,
but the player collides with it. Each collected star adds 10 points; three
screen-space digit entities display a value clamped to 000-999. Stars lose
energy on collisions, slide to a stop on floors, and fall if support is lost.
The camera follows horizontally using hard/soft bands and applies a short,
directional, fading shake on sufficiently strong new player impacts.
The hand-placed extra star starts at (576, 192), outside the left boundary
wall. It uses normal star physics and can be collected after getting over
that wall. The supplied map files are unchanged.

Rendering presents one completed frame per update, including when F1 is on.
Collision outlines use the same camera conversion as world-space sprites.

## Checks

```sh
./build/engine/ECSTest
```

Run from the repository root so asset-dependent checks can find `assets/`.
The test executable covers component storage, entity lifetimes and id reuse,
system order/exit handling, sound/sprite ownership, score digits, star
settling, lost support, spawner interactions, spike-side wall slides and
spike respawning from several directions, floor-seam landings and direction
changes after landing, and shake samples. It returns
nonzero on a failed check. Expected missing-file messages exercise resource
failure handling. Some asset success checks skip when assets cannot be found;
inspect the output as well as the exit status.

The executable is not registered with CTest; `ctest` alone does not run it.

## Documentation

- [ABOUT.TXT](ABOUT.TXT): team contributions, implementation notes, and AI/resource credits.
- [Resource usage and engine APIs](docs/RESOURCE_USAGE.md): ownership, reference
  lifetimes, system order, components, and extension rules.
- Headers and implementation files: API comments describing parameters,
  return values, resource access, and data lifetimes.

The legacy `map-background.txt` and `map-objects.txt` remain in `assets/` but
are not loaded by the current game. Keep the supplied `.map` layouts intact.
