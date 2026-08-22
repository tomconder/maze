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
  shadow map, bloom/FXAA/TAA post-processing, and the update/render split.
* `layer/introlayer.hpp`, `layer/optionlayer.hpp`, `layer/exitlayer.hpp`,
  `layer/splashscreenlayer.hpp` - other screens in the layer stack.
* `resourcemanager.hpp` / `.cpp` - asset path resolution.

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
* `bloomIntensity` compensates the soft-knee bloom extract (passes only
  above-threshold energy); don't "simplify" it back toward the old hard
  threshold without re-deriving the constant.

## Anti-patterns

* Don't call GLFW or raw GL from this tree; go through sponge platform wrappers.
* Don't put engine primitives here (buffers, shaders, Worker) — those belong in
  `sponge/src`.
* Don't apply viewport / post-process / shadow-FBO changes on the update thread.

## Related Context

* Engine core (Layer, Worker, GameAction): `../../sponge/src/AGENTS.md`
* Clustered lighting, shadows, AA: `../../sponge/src/platform/AGENTS.md`
