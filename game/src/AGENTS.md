# Game

The `maze` game itself: layers, scenes, UI, and the game-specific render frame.
Built on top of the sponge engine (`sponge/src`); does not implement rendering
primitives, windowing, or platform backends itself — see
`../../sponge/src/AGENTS.md` and `../../sponge/src/platform/AGENTS.md`.

## Entry Points

* `maze.hpp` / `maze.cpp` - `game::Maze`, the `sponge::platform::glfw::core::Application`
  subclass. Owns the layer stack (intro, maze, options, exit, imgui) and
  exposes the anti-aliasing mode and bloom toggles the UI reads.
* `layer/mazelayer.hpp` / `.cpp` - the core gameplay layer: camera, lighting,
  shadow map, SSAO, occlusion culling, bloom/FXAA/TAA post-processing, and
  the update/render split.
* `layer/framecapture.hpp` / `.cpp` - the `--capture-frames` and
  `--dump-nan-stats` debug scan (see the root `AGENTS.md`). Parsed in
  `createApplication()` under `#ifndef NDEBUG`; `MazeLayer::recordCapture()`
  feeds it float readbacks on the render thread. Test NaN and Inf by bits,
  through a pointer: the build uses fast math, so `std::isnan`, `std::isinf`
  and a bit test on a float passed by value all fold to false.
* `layer/introlayer.hpp`, `layer/optionlayer.hpp`, `layer/keymaplayer.hpp`,
  `layer/audiolayer.hpp`, `layer/exitlayer.hpp`, `layer/splashscreenlayer.hpp`
  - other screens in the layer stack. The options screen is one layer per tab:
  `OptionLayer` is Display, `KeyMapLayer` is Keyboard, `AudioLayer` is Audio.
  Only one is ever active; `ui/tabbar.hpp` draws the shared tab strip and owns
  the swap (`showOptionTab`) and the wraparound next/prev order
  (`cycleTab`), so all three layers reserve `tabBarHeight()` at the top of
  their Yoga root. `KeyMapLayer` rebinds keys through
  `InputManager::requestRebind()`; the key capture and the settings write both
  happen on the main thread, never in the layer. `AudioLayer`'s volume rows
  are `ui::Slider` (drag, click-to-jump, and keyboard/gamepad step), applied
  live to `sponge::platform::audio::Audio` and written to `Settings` on every
  change, but only flushed to disk on drag-release or a discrete step — not
  on every mouse-move frame of a drag.
* `layer/imgui/imguilayer.hpp` / `.cpp` - the debug UI, compiled only when
  `ENABLE_IMGUI`. `layer/imgui/noopimguilayer.hpp` defines an inert
  `ImGuiLayer` with the same name for release builds, picked by the single
  `#ifdef` in `maze.hpp`, so no other file needs one. Mirrors
  `GLFWManager`/`NoopManager` in `../../sponge/src/platform/glfw/imgui/`.
  Never link imgui into release: keep the `imgui::imgui` link and the
  `layer/imgui/*.cpp` glob behind `ENABLE_IMGUI`.
* `scene/scenefile.hpp` / `.cpp` - reads `assets/scenes/maze.yaml` into a
  `Scene`: camera, lighting and the object list. Parsed with fkYAML in the
  `MazeLayer` constructor, which `startupCore()` guarantees runs after
  logging and settings are up.
* `scene/refraction.hpp` - `SceneRefraction`, a scene object's glass values.
  It has its own header so the render frame does not include the scene
  loader.
* A scene object's `reflective` (0 to 1) turns on screen-space reflection
  for it. The loader clamps it, and sets it to 0 on a `refractive` object,
  because glass is not in the depth prepass that carries the mask.
* `planar` (bool, default false) draws the object with a mirrored scene
  render instead of SSR. It needs `reflective > 0`. The plane is the
  object's top face (local y = 1). Only the first planar object in a scene
  is used; the others fall back to SSR with a warning.
* `probe` (top-level, optional) sets one reflection probe: `position`,
  `boxMin`, `boxMax`. The loader drops it with a warning unless boxMin is
  below boxMax on every axis and the position is inside the box. With no
  probe, ambient is the flat `ambientStrength` term.

## Contracts & Invariants

* Only `MazeLayer` opts into `runsOnUpdateThread()`. Its `onUpdate()` runs on
  the update thread and must issue **no GL calls**; `onRender()` runs on the
  render thread and owns all GL commands. Other game layers keep the default
  (`false`) so `onUpdate()` runs on the render thread. See the double-buffered
  `renderFrames` / `renderReadIndex` in `mazelayer.hpp`.
* TAA's previous-frame state — `prevCameraViewProj` and
  `prevObjectModelMatrices` — is carried in the snapshot, not held on the
  render thread, so the camera history and the object history always describe
  the same past frame. Never read the live `camera` at resolve time:
  `render[N]` reads `update[N-1]`, so it is frames ahead and the history
  smears.
* `onFrameSync()` runs on the main thread and publishes the slot from the last
  completed `captureRenderFrame()`, so render\[N] always reads update\[N-1]'s
  snapshot — never read a slot the update thread might still be writing.
* Settings touched by both ImGui (render thread) and `captureRenderFrame()`
  (update thread) — lights, directional light, anti-aliasing and bloom
  params — go through `settingsMutex`.
* Deferred state that must apply on a specific thread (viewport and
  post-process resize, shadow map FBO rebuild) is set via an atomic flag +
  payload and consumed in `onRender()` on the GL thread; don't apply it inline
  where it's requested.
* Every key in `assets/scenes/maze.yaml` is optional and falls back to the
  defaults in `scene/scenefile.hpp` and `scene/refraction.hpp`, so a partial
  file still yields a complete scene. A missing or malformed file logs and
  leaves the defaults standing; it must never take startup down.
* The scene file supplies the shadow map resolution *default* only. A
  `video.shadowRes` saved from the options screen overrides it — keep the
  scene value as the fallback argument to `Settings::getUInt32()`, or a
  saved resolution silently reverts.
* Point lights are placed by the seeded spiral in `setNumLights()`, not
  authored one by one, so the debug slider can change the count at run
  time. The scene file carries the generator's inputs, not positions.
* Scene object paths come from YAML, so a typo reaches run time.
  `loadScene()` drops entries with an empty name or path, which is what
  keeps `objectModels`, `objectModelMatrices` and `objectEmissives`
  index-locked for `renderGameObjects()`.
* `bloomThreshold` and `bloomIntensity` operate on linear radiance and are
  applied before tone mapping. The bloom texture holds unbounded values, so
  useful threshold values run past 1.0 and intensity stays small. Tune both
  from a texture readback. Their defaults live in the `bloom` section of
  `assets/scenes/maze.yaml`, not in settings; the debug sliders change them
  for the current run only.
* `onRender()` calls `RendererAPI::flush()` after the prepass and after the
  opaque pass. The driver holds queued commands until the swap, so without
  them the GPU starts only after the CPU has recorded the whole frame. A
  release capture run showed 24% lower frame time with them, in a windowed
  window. Keep them when reordering passes, and re-measure with the `cpu` line
  of `capture_timing.txt`.
* Each shadow cascade is drawn again only when its own matrix in
  `lightSpaceMatrices` changes or the map is rebuilt (`shadowCached`). A
  camera that only turns changes no matrix, so nothing redraws. This holds
  only while shadow casters are static. An object that moves must also
  invalidate the cache, or its shadow freezes.
* A refractive object is left out of the shadow map and the depth prepass.
  The glass pass runs after the opaque color and the light cubes, and before
  bloom, still in linear radiance. It samples a copy of the scene color.
  Moving it back into the prepass deletes the image behind the glass.

## Anti-patterns

* Don't call GLFW or raw GL from this tree; go through sponge platform wrappers.
* Don't put engine primitives here (buffers, shaders, Worker) — those belong in
  `sponge/src`.
* Don't apply viewport / post-process / shadow-FBO changes on the update thread.
* Don't put scene content back into the source. Object list, camera placement
  and light parameters belong in `assets/scenes/maze.yaml`.

## Related Context

* Engine core (Layer, Worker, GameAction): `../../sponge/src/AGENTS.md`
* Clustered lighting, shadows, AA: `../../sponge/src/platform/AGENTS.md`
