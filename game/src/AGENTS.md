# Game

The `maze` game itself: layers, scenes, UI, and the game-specific render frame.
Built on top of the sponge engine (`sponge/src`); does not implement rendering
primitives, windowing, or platform backends itself — see
`../../sponge/src/platform/AGENTS.md` for those.

## Entry Points

* `maze.hpp` / `maze.cpp` - `game::Maze`, the `sponge::platform::glfw::core::Application`
  subclass. Owns the layer stack (intro, maze, options, exit, imgui) and
  exposes the FXAA/bloom toggles the UI reads.
* `layer/mazelayer.hpp` / `.cpp` - the core gameplay layer: camera, lighting,
  shadow map, bloom/FXAA post-processing, and the update/render split.
* `layer/introlayer.hpp`, `layer/optionlayer.hpp`, `layer/exitlayer.hpp`,
  `layer/splashscreenlayer.hpp` - other screens in the layer stack.
* `resourcemanager.hpp` / `.cpp` - asset path resolution.

## Contracts & Invariants

* `MazeLayer::onUpdate()` runs on the update thread and must issue **no GL
  calls**; `onRender()` runs on the render thread and owns all GL commands.
  See `runsOnUpdateThread()` override and the double-buffered
  `renderFrames` / `renderReadIndex` in `mazelayer.hpp`.
* `onFrameSync()` runs on the main thread and publishes the slot from the last
  completed `captureRenderFrame()`, so render\[N] always reads update\[N-1]'s
  snapshot — never read a slot the update thread might still be writing.
* Settings touched by both ImGui (render thread) and `captureRenderFrame()`
  (update thread) — lights, directional light, fxaa/bloom params — go through
  `settingsMutex`.
* Deferred state that must apply on a specific thread (viewport/FXAA resize,
  shadow map FBO rebuild) is set via an atomic flag + payload and consumed in
  `onRender()` on the GL thread; don't apply it inline where it's requested.
* `bloomIntensity` compensates the soft-knee bloom extract (passes only
  above-threshold energy); don't "simplify" it back toward the old hard
  threshold without re-deriving the constant.

## Related Context

* Threading model (Worker, double-buffering, kick discipline):
  see project memory `project_threading` / `sponge/src/thread/`.
* Clustered lighting, shadows, AA: `../../sponge/src/platform/AGENTS.md`.
