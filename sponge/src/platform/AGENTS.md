# Platform Backends

Everything that talks to the OS, the window, or the GPU directly. Game code
(`game/src`) and the rest of the engine (`sponge/src/{core,debug,event,input,
layer,logging,scene,thread}`) should stay platform-agnostic and go through these backends rather
than calling GLFW/OpenGL/OS APIs directly.

## Layout

* `glfw/core/` - `Application` (main loop, layer stack, window/vsync/mouse
  state), `Window`, `InputManager`. This is the only place that owns the GLFW
  window and drives the update/render worker threads.
* `audio/` - miniaudio playback (`Audio::init` / `shutdown` / `play` /
  `setMasterVolume` / `setSfxVolume` / `setMusicVolume`). Init and shutdown
  from `Application`; game code calls `play` and the volume setters.
  `init` seeds all three volumes from `Settings` (`audio.masterVolume` /
  `audio.sfxVolume` / `audio.musicVolume`, 0-100, default 100) — it runs after
  `startupCore()` has loaded `Settings`, so the values are already on disk by
  then. `play` routes through the sfx sound group, not the engine's master
  sound directly, so sfx and music volume can be controlled independently of
  each other and of master.
* `glfw/imgui/` - `GLFWManager` (real ImGui backend, built when
  `ENABLE_IMGUI`) vs `NoopManager` (stub for release builds).
* `opengl/renderer/` - GL primitives: `Context`, `RendererAPI`, buffers
  (`vertexbuffer`, `indexbuffer`, `vertexarray`, `ssbo`), `Shader`, `Texture`,
  `AssetManager`.
* `opengl/scene/` - render features built on the primitives: `Model`, `Mesh`,
  `Cube`, `Sprite`, `BitmapFont`, `Quad`, `ClusteredLights`, `ShadowMap`,
  `SceneTarget`, `Ssao`, `Ssr`, `PlanarReflection`, `OcclusionCuller`,
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
* The pipeline is: depth prepass, then the mirrored scene render for the
  planar mirror (if any), then opaque scene to a linear HDR `SceneTarget`,
  then the light cubes, then the planar composite draws the mirror over them
  from its mirrored render, then the full-screen `Ssr` pass blends
  reflections into that target, then refractive objects into that same
  target, then bloom extracts and blurs in linear ->
  `SceneTarget::resolve()` composites bloom, tone maps, gamma encodes. `Ssr`
  runs only when SSR is on and at least one object other than the active
  planar mirror has `reflective > 0`. SSR takes the mirror over when the
  planar path is inactive: the Planar toggle is off, the mirror is culled,
  or the camera is on or behind its plane. Nothing upstream of resolve() may
  tone map, or the bloom threshold stops being a radiance value. Refractive
  objects stay out of the depth prepass, so the color behind them exists to
  sample. The glass pass depth-tests
  against the scene depth renderbuffer (the blitted opaque depth) and blits
  that depth back to the prepass texture. Do not sample the prepass depth
  while it is the bound depth attachment: that feedback is undefined.
  `Ssr` and the glass pass each sample `SceneTarget::copyColor()`, never the
  color attachment they draw into. The prepass normal alpha is the SSR
  strength mask.
* Dither exactly once, at the final 8-bit write, and keep every intermediate
  target float (`GL_RGB16F`). An 8-bit intermediate quantises undithered and
  then quantises again at the real output — the banding `dither8` exists to
  prevent. resolve() dithers only when it writes the back buffer directly;
  when FXAA or TAA follows, each dithers on its own final write instead.
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

* Threading (Worker, double-buffered frame snapshots): `../thread/`.
* Game-side usage of these backends: `../../../game/src/AGENTS.md`.
