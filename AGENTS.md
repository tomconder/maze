Do not attribute AI in code comments, commit messages and pull request descriptions.

* Choose the simplest implementation that fully satisfies the requirements. Avoid speculative abstractions, configuration and indirection.
* Grow the system in layers, starting with the simplest version that works end to end, and add each new capability on top of a product that already works. Never trade a working product for unfinished complexity.
* Keep components modular and concerns clearly separated.
* Prefer established, well-maintained libraries and frameworks when they reduce overall complexity or improve reliability. Do not reimplement common functionality without a clear reason.
* Lean on the dependencies already in the project before writing your own implementation or adding packages. Do not assume a library lacks a capability without checking its documentation.
* Make architecture decisions for the long term. If the change in front of you only warrants a stopgap, take it and say so — name the ceiling and what would force the real fix. Flag the larger decision; do not fold it into the current change unasked.
* Study how established libraries and frameworks solve similar problems. Adopt their patterns and conventions rather than inventing an approach from scratch.

## Pull Requests

Also in `~/.claude/skills/writing-pr` (Claude Code skill); kept here so other agents see it.

* Don't write essays. Don't mention that you ran tests. Write a concise body.
* Use bullet points, code snippets, and Mermaid diagrams.
* For visual changes, show a before/after table with images.
* For benchmarks, show before/after tables (baseline from target branch).
* For large or high-risk changes, write it like a technical blog post.

## Compaction

When compacting, keep the current task goal, files changed, commands run,
failing builds and exact errors, and decisions made. Drop old exploration
paths and repeated logs.

## Architecture

* `game/` — maze application (layers, UI, cameras). CMake target `game`; Windows exe `maze.exe`.
* `sponge/src/{core,debug,event,input,layer,logging,scene,thread}` — platform-agnostic engine.
* `sponge/src/platform/` — GLFW, OpenGL, OS file I/O.
* `assets/shaders/slang/` — Slang sources. `assetconv --manifest` compiles every stage listed under `shaders` in `assets/manifest.yaml` to GLSL 450, column-major, into one `shaders/shaders.spnga`. `Shader` looks stages up by manifest name (`"pbr.vert"`), not by path.
* C++23, vcpkg manifest (`vcpkg.json`). Optional CMake flags: `ENABLE_IMGUI`, `ENABLE_PROFILING` (Tracy).
* No automated test suite. Verify with compile + run + `pre-commit.exe run --all-files`.

## Intent Layer

**Before modifying code in a subdirectory, read its AGENTS.md first** to understand local patterns and invariants.

* **Game**: `game/src/AGENTS.md` — maze layers, scenes, UI
* **Engine core**: `sponge/src/AGENTS.md` — platform-agnostic engine (layers, events, input types, Worker)
* **Platform backends**: `sponge/src/platform/AGENTS.md` — GLFW / OpenGL / OS-specific code

`CLAUDE.md` is a Claude Code pointer to this file. Do not duplicate rules there.

### Global Invariants

* `sponge/src/{core,debug,event,input,layer,logging,scene,thread}` must not call GLFW/GL/OS APIs — those go through `sponge/src/platform/`.
* No tool or persona names in code comments (`ponytail:`, agent names, etc.) — tag deliberate simplifications in plain words.
* Cluster grid and max-lights constants in `assets/shaders/slang/include/clustered.slang` must match `sponge/src/platform/opengl/scene/clusteredlights.hpp`.
* `maxCascades` in `assets/shaders/slang/include/shadows.slang` must match `ShadowMap::maxCascades` in `sponge/src/platform/opengl/scene/shadowmap.hpp`.
* Don't edit generated files in `out/` or 3rd-party files in `sponge/deps`.

## IDE Tooling

When the CLion MCP server is connected, prefer its tools for searching,
reading and patching files, and use it to open a file in the editor when the
user should see it.

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

These are configure presets. The only build presets are `windows-release`,
`osx-release` and `linux-release`, which build the matching `ci-*-release`
configuration. For any other configure preset, build its binary directory:

```
cmake --build out/build/ci-windows-debug
```

To use the preset on Windows:

```
cmake.exe -B build --preset windows-msvc-release
cmake.exe --build build --target game
```

`windows-msvc-debug`/`windows-msvc-release` use real `cl.exe` (`ci-windows-*`
use `clang-cl`, which warns differently) — run both commands from a Developer
Command Prompt, or `VsDevCmd.bat` first, so `cl.exe`/`rc.exe`/`INCLUDE`/`LIB`
are on `PATH`.

## Running

On Windows, the maze executable is in the build directory:

```
build\maze\maze.exe
```

Run it from its own directory. `File::getResourceDir()` returns the relative
path `assets` on Windows and Linux, so a different working directory loads no
assets and the window stays grey.

Or, for MacOS, the app bundle is in the build directory:

```
build/maze/maze.app
```

### Capture flags (debug builds only)

Two flags exist only when `NDEBUG` is not defined. A release build ignores
them and starts normally. Use them to check the render pipeline for NaN, Inf
and isolated bright pixels without a screen capture, which quantizes to 8 bits.

```
maze.exe --capture-frames 600
maze.exe --capture-frames=600 --dump-nan-stats
```

* `--capture-frames N` (or `=N`): skips the splash and intro screens, loads the
  maze, turns the camera once over N frames in a fixed 1600x900 window with
  vsync off, writes the last frame of each stage as a float `capture_<stage>_last.pfm`
  and exits. A missing, zero or non-numeric N leaves the normal start.
* `--dump-nan-stats`: scans every stage on every frame and writes
  `capture_stats.txt` (NaN, Inf, values over 1000, bright and dark specks,
  highest finite value, first hit), `capture_specks.txt` (frame and pixel of
  each scene-stage speck) and `capture_<stage>_first_hit.pfm`. N defaults to
  600 when `--capture-frames` is not given.
* `--capture-walk=D`: with `--capture-frames`, move the camera D world units
  per update along +X and keep its yaw fixed, instead of turning it. A turn
  never redraws a shadow cascade, so a walk is the way to count redraws:
  `gpu shadow calls` in `capture_timing.txt` is one per cascade redraw. A
  near cascade redraws when the camera has moved one texel of its map.
* `--capture-cascades=N`: use N shadow cascades (1 to 4, default 2). One is
  the whole-scene map alone, for an A/B against the cascades.
* `--capture-off=a,b`: turn passes off for the run. Names: `aa`, `bloom`,
  `ssao`, `ssr`, `shadow`, `probe`, `planar`. Two more names change the
  measurement: `shadowcache` draws the shadow map every frame (the cache would
  otherwise draw it once and hide its cost), and `gputimer` turns the GPU timer
  off. Without `--dump-nan-stats`, the run also writes `capture_timing.txt`
  (mean, p50, p95 frame time, a `gpu frame` line, and one `gpu` line per pass
  with its GPU time) from the frames after a 60 frame warm-up. Pass times come
  from `GL_TIME_ELAPSED` queries, so they hold in a debug build; only the CPU
  side is slower there. `gpu frame` has two GPU clock times: the span, from
  the start of `onRender()` to its last pass, and the period, from one frame
  start to the next. Period minus span is the time the GPU spends idle, in
  the swap, or in work outside `onRender()`. The `cpu` line gives the median
  wall time of each part of the frame loop (`Application::loopTimes`): the
  render task, the buffer swap inside it, the update task, and the time the
  main thread waits for the render and for the update. The task that the main
  thread waits on longest sets the frame rate.
* Stages: `scene` and `bloom` (RGB16F, linear radiance), `tonemap` and `taa`
  or `fxaa` input (0 to 1), `backbuffer` (8-bit). Files go to the working
  directory, so run from the build directory as above.
* The back buffer cannot show NaN: it reads as black. Trust the float stages.
* Each stage read stalls the GPU. Debug builds also run slower, so a 600-frame
  stats run takes minutes.

Before you commit changes run the pre-commit script:

```
pre-commit.exe run --all-files
```

## vcpkg port updates
- When bumping a port, update the version, SHA512, port-version, and versions/ database, AND raise the `version>=` floors in the consuming project's vcpkg.json.
- Run `vcpkg x-add-version <port>` for a single port only. Never use `--all`, because it has truncated baseline.json before. Check `git diff versions/baseline.json` afterward.
- Open upstream PRs against microsoft/vcpkg (`--repo microsoft/vcpkg`), not against my fork.
- If a fix applies to every platform, remove stale baseline/ci entries for all triplets, not just Windows.
- Don't declare a dependent build failure out of scope without asking. Find the root cause first.
