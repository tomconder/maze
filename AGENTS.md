Do not attribute AI in code comments, commit messages and pull request descriptions.

* Choose the simplest implementation that fully satisfies the requirements. Avoid speculative abstractions, configuration and indirection.
* Grow the system in layers, starting with the simplest version that works end to end, and add each new capability on top of a product that already works. Never trade a working product for unfinished complexity.
* Keep components modular and concerns clearly separated.
* Prefer established, well-maintained libraries and frameworks when they reduce overall complexity or improve reliability. Do not reimplement common functionality without a clear reason.
* Lean on the dependencies already in the project before writing your own implementation or adding packages. Do not assume a library lacks a capability without checking its documentation.
* Make architecture decisions for the long term. Do not accept a stopgap that only works for now and is meant to be replaced later.
* Study how established libraries and frameworks solve similar problems. Adopt their patterns and conventions rather than inventing an approach from scratch.

# When compacting, preserve:

* current task goal, files changed, commands run
* failing tests and exact errors, decisions made

# Drop: old exploration paths, repeated logs

* Do not edit generated files in `out/`
* Do not edit 3rd-party files in `sponge/deps`
* App code: `game/` `sponge/`

## Architecture

* `game/` — maze application (layers, UI, cameras). CMake target `game`; Windows exe `maze.exe`.
* `sponge/src/{core,event,input,layer,logging,scene,thread}` — platform-agnostic engine.
* `sponge/src/platform/` — GLFW, OpenGL, OS file I/O.
* `assets/shaders/slang/` — Slang sources. CMake (`cmake/CompileSlang.cmake`) compiles to GLSL 450 with `-matrix-layout-column-major`. Runtime loads compiled GLSL from the build tree, not `.slang`.
* C++23, vcpkg manifest (`vcpkg.json`). Optional CMake flags: `ENABLE_IMGUI`, `ENABLE_PROFILING` (Tracy).
* No automated test suite. Verify with compile + run + `pre-commit.exe run --all-files`.

## Intent Layer

**Before modifying code in a subdirectory, read its AGENTS.md first** to understand local patterns and invariants.

* **Game**: `game/src/AGENTS.md` — maze layers, scenes, UI
* **Engine core**: `sponge/src/AGENTS.md` — platform-agnostic engine (layers, events, input types, Worker)
* **Platform backends**: `sponge/src/platform/AGENTS.md` — GLFW / OpenGL / OS-specific code

`CLAUDE.md` is a Claude Code pointer to this file. Do not duplicate rules there.

### Global Invariants

* `sponge/src/{core,event,input,layer,logging,scene,thread}` must not call GLFW/GL/OS APIs — those go through `sponge/src/platform/`.
* Cluster grid and max-lights constants in `assets/shaders/slang/include/clustered.slang` must match `sponge/src/platform/opengl/scene/clusteredlights.hpp`.
* Don't edit generated files in `out/` or 3rd-party files in `sponge/deps`.

## Building

Use a configuration preset to compile `maze`. Possible values are:

| Preset                 | Description                          |
|------------------------|--------------------------------------|
| `ci-linux-debug`       | Linux debug build                    |
| `ci-linux-release`     | Linux release build                  |
| `ci-osx-debug`         | MacOS debug build                    |
| `ci-osx-release`       | MacOS release build                  |
| `ci-windows-debug`     | Windows debug build (uses sccache)   |
| `ci-windows-release`   | Windows release build (uses sccache) |
| `windows-msvc-debug`   | Windows MSVC debug build             |
| `windows-msvc-release` | Windows MSVC release build           |

To use the preset on Windows:

```
cmake.exe -B build --preset windows-msvc-release
cmake.exe --build build --target game --config Release
```

`windows-msvc-debug`/`windows-msvc-release` use real `cl.exe` (`ci-windows-*`
use `clang-cl`, which warns differently) — run both commands from a Developer
Command Prompt, or `VsDevCmd.bat` first, so `cl.exe`/`rc.exe`/`INCLUDE`/`LIB`
are on `PATH`.

## Running

On Windows, the maze executable is in the build directory:

```
build\maze\Release\maze.exe
```

Or, for MacOS, the app bundle is in the build directory:

```
build/maze/maze.app
```

Before you commit changes run the pre-commit script:

```
pre-commit.exe run --all-files
```
