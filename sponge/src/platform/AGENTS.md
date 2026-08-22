# Platform Backends

Everything that talks to the OS, the window, or the GPU directly. Game code
(`game/src`) and the rest of the engine (`sponge/src/{core,event,layer,scene,
thread}`) should stay platform-agnostic and go through these backends rather
than calling GLFW/OpenGL/OS APIs directly.

## Layout

* `glfw/core/` - `Application` (main loop, layer stack, window/vsync/mouse
  state), `Window`, `InputManager`. This is the only place that owns the GLFW
  window and drives the update/render worker threads.
* `audio/` - miniaudio playback (`Audio::init` / `shutdown` / `play`). Init and
  shutdown from `Application`; game code only calls `play`.
* `glfw/imgui/` - `GLFWManager` (real ImGui backend, built when
  `ENABLE_IMGUI`) vs `NoopManager` (stub for release builds).
* `opengl/renderer/` - GL primitives: `Context`, `RendererAPI`, buffers
  (`vertexbuffer`, `indexbuffer`, `vertexarray`, `ssbo`, `framebuffer`),
  `Shader`, `Texture`, `AssetManager`.
* `opengl/scene/` - render features built on the primitives: `Model`, `Mesh`,
  `Cube`, `Sprite`, `BitmapFont`, `Quad`, `ClusteredLights`, `ShadowMap`,
  `Bloom`, `FXAA`, `TAA`.
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
* Anti-aliasing is a three-way mode (`AntiAliasing::None/Fxaa/Taa`), not a
  toggle. `FXAA` is single-pass; `TAA` jitters the camera with Halton(2,3),
  accumulates into a ping-pong `GL_RGB16F` history, and reprojects it through
  the depth prepass texture. The history must stay float — an 8-bit target
  quantises every sub-LSB increment to zero and never converges.
* TAA reprojects through an RG16F velocity buffer (`current UV - previous UV`)
  written as a second attachment on the depth prepass FBO, so a moving object
  reprojects correctly. Depth 1.0 is the coverage mask — background pixels,
  which the prepass never draws, fall back to reconstructing the world
  position from depth and reprojecting with the camera alone.
* The prepass draws the light cubes as well as the models, so they carry
  motion vectors. That also puts their depth in the buffer, which means the
  cube pass must run `GL_LEQUAL`: under `GL_LESS` every cube fragment is
  rejected by its own prepass depth and the cubes vanish.
* The velocity pass must run with `GL_BLEND` disabled. Blending is enabled
  globally in `RendererAPI` and the prepass shader writes no alpha, so motion
  vectors get blended against the clear colour and never reach the texture.
  Clear that attachment with `glClearBufferfv` to zero, never `glClear` — the
  global clear colour is grey and reads back as ~22 pixels of bogus motion.
* Motion is measured with unjittered matrices while rasterization uses the
  jittered one; mixing them makes the TAA jitter itself read as movement.
* The TAA history is resampled with a Catmull-Rom filter, not the bilinear
  `Sample()` the hardware gives you. `prevUV` rarely lands on a texel centre
  while the camera moves, so the history is refiltered every frame; bilinear
  compounded over a ~10-frame tail is a low-pass filter and the image goes
  soft in motion. Do not "simplify" it back to a single `Sample()`.

## Anti-patterns

* Don't call GLFW or raw GL functions from `game/src` or engine-core code;
  add/extend a wrapper here instead.
* Don't add OS-specific code outside `windows/`, `osx/`, `linux/` — those
  three directories exist so the rest of the tree stays portable.

## Related Context

* Threading (Worker, double-buffered frame snapshots): `../../thread/`.
* Game-side usage of these backends: `../../../../game/src/AGENTS.md`.
