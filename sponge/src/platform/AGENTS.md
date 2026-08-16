# Platform Backends

Everything that talks to the OS, the window, or the GPU directly. Game code
(`game/src`) and the rest of the engine (`sponge/src/{core,event,layer,scene,
thread}`) should stay platform-agnostic and go through these backends rather
than calling GLFW/OpenGL/OS APIs directly.

## Layout

* `glfw/core/` - `Application` (main loop, layer stack, window/vsync/mouse
  state), `Window`, `InputManager`. This is the only place that owns the GLFW
  window and drives the update/render worker threads.
* `glfw/imgui/` - `GLFWManager` (real ImGui backend, built when
  `ENABLE_IMGUI`) vs `NoopManager` (stub for release builds).
* `opengl/renderer/` - GL primitives: `Context`, `RendererAPI`, buffers
  (`vertexbuffer`, `indexbuffer`, `vertexarray`, `ssbo`, `framebuffer`),
  `Shader`, `Texture`, `AssetManager`.
* `opengl/scene/` - render features built on the primitives: `Model`, `Mesh`,
  `Cube`, `Sprite`, `BitmapFont`, `Quad`, `ClusteredLights`, `ShadowMap`,
  `Bloom`, `FXAA`.
* `opengl/debug/` - GL diagnostics/profiler, debug-build only.
* `windows/`, `osx/`, `linux/` `core/*file.*` - the only OS-specific file I/O
  shims; everything else is GLFW-portable.

## Contracts & Invariants

* `Application` owns the GLFW window and the GL context. Anything that must
  run on the thread owning the window/context (vsync, mouse visibility,
  fullscreen toggle, resolution change, viewport resize) is requested via an
  atomic pending-flag pair from other threads and applied inside
  `Application`/`onRender()` — never call the GLFW function directly from a
  layer's update thread.
* SSBOs: do not use `row_major mat4` layout for driver-uploaded matrices — it
  is broken on at least one driver in this project's history. Pass matrices
  as individual float4 rows instead. See `opengl/renderer/ssbo.*` and
  `opengl/scene/clusteredlights.cpp`.
* `ClusteredLights::maxLightsPerCluster` is defined as `= maxLights`
  (currently 128) so per-cluster truncation can never occur; don't split it
  back into its own literal or it silently truncates instead of erroring.
* `ShadowMap` uses EVSM with a Dual Kawase blur; shadow resolution is a
  discrete quality setting (`video.shadowRes`), not a continuous slider, and
  FBO rebuilds are deferred to the render thread the same way viewport resize
  is (pending-flag pattern above) — never rebuild the FBO from the thread
  that requested the change.
* Anti-aliasing is `FXAA` (single-pass) only. No TAA exists in this codebase
  despite older docs/memory claiming it — verified no ping-pong history
  buffer, no Halton jitter anywhere in `sponge/src` or `game/src` as of
  2026-08-16.

## Anti-patterns

* Don't call GLFW or raw GL functions from `game/src` or engine-core code;
  add/extend a wrapper here instead.
* Don't add OS-specific code outside `windows/`, `osx/`, `linux/` — those
  three directories exist so the rest of the tree stays portable.

## Related Context

* Threading (Worker, double-buffered frame snapshots): `../../thread/`.
* Game-side usage of these backends: `../../../../game/src/AGENTS.md`.
