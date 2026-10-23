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

* Don't write essays. Write a concise body.
* Use bullet points, code snippets, and Mermaid diagrams.
* For visual changes, show a before/after table with images.
* For benchmarks, show before/after tables (baseline from target branch).
* Merge with a merge commit. Every commit stays in history, so write each commit message to stand alone.
* For large or high-risk changes, write it like a technical blog post.
* Set the body with `gh pr create/edit --body-file <file>`. `--body -` does not read stdin; it sets the body to a literal `-`. Read the body back with `gh pr view <n> --json body -q .body`.

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
* `evsmExponent` in `assets/shaders/slang/include/shadows.slang` must match `ShadowMap::evsmExponent` in `sponge/src/platform/opengl/scene/shadowmap.hpp`.
* `bakedFontSizes` in `game/src/ui/menufontsize.hpp` must match `fonts[].sizes` in `assets/manifest.yaml`. The font holds only those sizes; a text size outside the list draws nothing.
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
| `windows-clang-debug`  | Windows clang debug build            |
| `windows-clang-release`| Windows clang release build          |

These are configure presets. The only build presets are `windows-release`,
`osx-release` and `linux-release`, which build the matching `ci-*-release`
configuration. For any other configure preset, build its binary directory:

```
cmake --build out/build/ci-windows-debug
```

To use the preset on Windows:

```
cmake.exe -B build --preset windows-clang-release
cmake.exe --build build --target game
```

All Windows presets use LLVM `clang` (the GNU-style driver, not `clang-cl`) —
run both commands from a Developer Command Prompt, or `VsDevCmd.bat` first, so
`rc.exe`/`INCLUDE`/`LIB` are on `PATH`.

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
  vsync off, writes the last frame of each stage and exits. The `scene` and
  `bloom` stages hold radiance and are float `capture_<stage>_last.pfm`; the
  0 to 1 stages (`tonemap`, `taa`, `backbuffer`) are 8-bit
  `capture_<stage>_last.png`. A missing, zero or non-numeric N leaves the
  normal start.
* `--dump-nan-stats`: scans every stage on every frame and writes
  `capture_stats.txt` (NaN, Inf, values over 1000, bright and dark specks,
  highest finite value, first hit), `capture_specks.txt` (frame and pixel of
  each scene-stage speck) and `capture_<stage>_first_hit.pfm`. N defaults to
  600 when `--capture-frames` is not given.
* `--capture-walk=D`: with `--capture-frames`, move the camera D world units
  per update along +X and keep its yaw fixed, instead of turning it. A turn
  never redraws a shadow cascade, so a walk is the way to count redraws:
  `gpu shadow calls` in `capture_timing.txt` is one per cascade redraw. A
  near cascade redraws when the camera has moved `ShadowMap::snapTexels`
  texels of its map.
* `--capture-cascades=N`: use N shadow cascades (1 to 4, default 2). One is
  the whole-scene map alone, for an A/B against the cascades.
* `--capture-scene=<name>`: load `assets/scenes/<name>.yaml` in place of
  `maze.yaml`. `lodtest` is the mesh LOD test scene.
* `--capture-menu=intro|exit|display|keyboard|audio|confirm|reset`: skip the splash, show that
  menu screen for two seconds, write the back buffer as
  `capture_menu_<name>.png` (8-bit RGB) and exit. The window has no frame, so
  `--capture-size=WxH` can be larger than the screen; it also sets the window
  size of any other capture run (default 1600x900). Use it to check menu layout
  at a resolution the monitor cannot show, such as
  `--capture-menu=display --capture-size=3840x2160`. Debug builds link libspng
  for the PNG; release builds do not.
* `--dump-yoga`: with `--capture-menu=<name>`, write that menu's Yoga tree as
  JSX for yogalayout.dev/playground to `capture_menu_<name>.jsx` at the
  `--capture-size` window size (default 1600x900) and exit. No window or
  OpenGL is used. The tree has every row part (sliders, select lists,
  checkboxes, buttons). The display tab lists three aspect ratios and four
  resolutions in place of what a monitor reports, so its dot counts and
  value width can differ from a run on a real monitor. Debug builds only.
* `--capture-off=a,b`: turn passes off for the run. Names: `aa`, `bloom`,
  `ssao`, `ssr`, `shadow`, `probe`, `gi` (flat ambient diffuse in place of
  the probe irradiance), `planar`, `lod` (draw LOD0 for every
  mesh, for an A/B of the cut). Two more names change the measurement:
  `shadowcache` draws the shadow map every frame (the cache would otherwise
  draw it once and hide its cost), and `gputimer` turns the GPU timer off. Without
  `--dump-nan-stats`, the run also writes `capture_timing.txt` (mean, p50, p95 frame time, a `gpu frame` line, and one `gpu` line per pass
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

### Leak checking (Linux)

The `linux-sanitize` preset builds Debug with AddressSanitizer, LeakSanitizer
and UBSan (`ENABLE_SANITIZERS`). Run the game from its build directory and
quit normally; leaks print at exit. Windows has no leak checker, and
`ENABLE_SANITIZERS` stops the configure on Windows.

```
cmake --preset linux-sanitize
cmake --build out/build/linux-sanitize --target game
cd out/build/linux-sanitize/maze && ASAN_OPTIONS=detect_leaks=1 ./maze
```

Driver and GLFW leaks can show up as noise; suppress them with
`LSAN_OPTIONS=suppressions=<file>` once seen.

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
