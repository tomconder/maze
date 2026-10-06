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
  `SceneTarget`, `DepthPrepass`, `Ssao`, `Ssr`, `PlanarReflection`,
  `ReflectionProbe`, `OcclusionCuller`, `Bloom`, `FXAA`, `TAA`.
  `RendererAPI` also has the static state calls game code needs between
  passes (`setDepth`, `setAlphaBlend`, `clearColor`, `bindTexture`).
* `opengl/debug/` - GL diagnostics/profiler, debug-build only.
* `windows/`, `osx/`, `linux/` `core/*file.*` - the only OS-specific file I/O
  shims; everything else is GLFW-portable.

## Contracts & Invariants

* The context is OpenGL 4.5 core. Create GL objects with `glCreate*` and
  set them up with the named (direct state access) calls:
  `glTextureStorage2D`, `glNamedFramebufferTexture`, `glNamedBufferStorage`,
  `glVertexArrayVertexBuffer` and so on. Bind only to draw: a framebuffer
  as the target, a VAO, and textures with `glBindTextureUnit`. Never mix in
  `glGen*` + bind-to-edit: a DSA call on a name from `glGen*` that was never
  bound is `GL_INVALID_OPERATION`. Texture storage is immutable, so a resize
  deletes and recreates the texture (`renderer::createRenderTarget`).
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
* `ShadowMap` uses EVSM (exponentially warped depth, positive and negative
  warp, so four moments in an RGBA32F layer) with a Dual Kawase blur; shadow resolution is a
  discrete quality setting (`video.shadowRes`), not a continuous slider, and
  FBO rebuilds are deferred to the render thread the same way viewport resize
  is (pending-flag pattern above) — never rebuild the FBO from the thread
  that requested the change.
* `ShadowMap` is a stack of cascades, one layer each of a
  `GL_TEXTURE_2D_ARRAY`. Cascade 0 is the finest; the last cascade in use
  covers the whole scene, so it is the fallback and never changes while the
  light is still. The lit shaders sample the array and pick the finest cascade
  whose light-space box holds the point (`shadowCascaded()`), blending into
  the next near its edge. They pick by position, not by camera depth, so the
  mirrored view and the reflection probe stay correct. All
  `maxCascades` (4) layers are always allocated, and one cascade is at most
  `ShadowMap::maxResolution` (2048) on a side, so the 4096 quality setting
  builds a 2048 map. The blur reads a 2D view per layer. A view does not inherit the parent's sampler state, so set
  the filter, wrap and border colour on the view. Its name must come from
  `glGenTextures`, because `glTextureView` rejects a name that already has a
  target.
* A near cascade is a box around the camera eye, not around the middle of the
  frustum slice: a centre in front of the camera moves when the camera
  turns, and that would redraw the cascade on every turn. Its size comes from
  the split distances only, and its centre snaps to whole texels, so it
  changes only when the camera moves a texel. All cascades share one light
  view and one light-space depth range; keep that, or the depths stop
  comparing and the EVSM minimum spread needs a scale per cascade.
* The warp exponent `c` is at most about 44: the map holds `exp(2c * depth)`,
  which overflows float32 past about 88. The moments are data, so the shadow
  draw and the blur run with `GL_BLEND` off, and the map is cleared to the
  far-plane moments (`farMoments`), never to the global clear colour. The
  bleed threshold (`evsmBleedThreshold`) trims the low end of the lit factor;
  raise it only if a lit outline shows where two occluders overlap.
* `ShadowMap::fitLightSpace()` is static and touches no GL state: the update
  thread calls it while the render thread may be replacing the `ShadowMap`.
  The shadow pass has no object-level occlusion query: only per-mesh frustum
  culling per cascade.
* The pipeline is: the reflection probe capture (first frame only, after the
  shadow map), depth prepass, then the mirrored scene render for the
  planar mirror (if any), then opaque scene to a linear HDR `SceneTarget`,
  then the light cubes, then the planar composite draws the mirror over them
  from its mirrored render, then the full-screen `Ssr` pass blends
  reflections into that target, then refractive objects into that same
  target, then bloom extracts and blurs in linear ->
  `SceneTarget::resolve()` composites bloom, tone maps, gamma encodes. `Ssr`
  runs only when SSR is on and at least one object other than the active
  planar mirror has `reflective > 0`. SSR takes the mirror over when the
  planar path is inactive: the Planar toggle is off, the mirror is culled,
  or the camera is on or behind its plane. The probe is bound on unit 12 and
  feeds the PBR ambient specular term; the mirrored render and the capture
  itself run with it off. Unit 12 holds the probe cube map (PBR) and the
  depth prepass 2D texture (refraction); never call
  `glBindTextureUnit(12, 0)` between the two passes. Nothing upstream of
  resolve() may tone map, or the bloom threshold stops being a radiance
  value. Refractive objects stay out of the depth prepass, so the color
  behind them exists to sample. The glass pass depth-tests against the
  scene depth renderbuffer (the blitted
  opaque depth) and blits
  that depth back to the prepass texture. Do not sample the prepass depth
  while it is the bound depth attachment: that feedback is undefined.
  `Ssr` and the glass pass each sample `SceneTarget::copyColor()`, never the
  color attachment they draw into. The prepass normal alpha is the SSR
  strength mask.
* resolve() takes a `ToneMapper` (Reinhard, Filmic, ACES, AgX). Every
  operator outputs linear display light and shares the one gamma encode, so
  AgX includes its own inverse transfer step. Keep `ToneMapper` in
  `scenetarget.hpp` in step with the `tone*` constants in `tonemap.slang`.
* The glTF `baseColorFactor` is in the baked mesh entry as a linear `float4`
  and multiplies the albedo in `pbr.slang`. With no albedo texture it is the
  albedo.
* glTF `alphaMode` MASK is supported: the baked mesh entry carries
  `alphaMode` and `alphaCutoff` (the cutoff is 0 unless the mode is Mask), and a fragment whose albedo alpha
  (`texture.a * baseColorFactor.a`) is below it is discarded. `pbr.slang`, the
  depth prepass and the shadow pass run the same test, so their depth agrees;
  a mismatch leaves holes or z-fighting in the scene pass. The prepass and
  shadow passes each have an opaque and a masked program (`*_masked.frag`), so
  a discard does not turn off early-Z for opaque meshes: `Model::render` draws
  `AlphaPass::Opaque` with one and `AlphaPass::Masked` with the other. The
  masked program reads the albedo on unit 0, and `Mesh::draw` sets its inputs.
  BLEND is in the baked entry but draws as OPAQUE. `assetconv` scales each mip's alpha of a masked
  albedo so the share of texels passing the cutoff matches level 0; the
  cutoff is part of the baked pixels, so changing a material's cutoff needs a
  rebake.
* Clear coat (`KHR_materials_clearcoat`) carries the two factors only; the
  three coat textures are not read. The coat uses
  the geometric normal, a fixed F0 of 0.04, and takes its Fresnel share off
  the base light and the base ambient. It adds its own probe reflection,
  because that is most of what a coat looks like. A factor of 0 skips all of
  it. The factors are in the baked mesh entry, so changing them changes
  `asset::version`.
* Diffuse transmission (`KHR_materials_diffuse_transmission`) carries the
  color and factor, packed in one `float4` (`rgb` color, `w` factor), and the
  strength texture (alpha channel, slot 6, texture unit 14, linear BC7). The
  color texture is not read. The factor, times that alpha, moves that share
  of the Lambert lobe from the front to the back of the surface
  (`evaluateTransmission`). The back term is not shadowed, since a back-lit pixel is in its own shadow, so
  objects in between do not block it. Fixing that needs a thickness
  (`KHR_materials_volume`). A factor of 0 skips all of it.
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
  Clear that attachment with `glClearNamedFramebufferfv` to zero, never `glClear` — the
  global clear colour is grey and reads back as ~22 pixels of bogus motion.
* Clear a framebuffer before you bind a shader program, not after. On NVIDIA,
  a `glClear` with a program bound recompiles that program's vertex shader
  and logs `API PERFORMANCE [131218] ... is being recompiled based on GL
  state`. `captureProbe()` binds the PBR program only after
  `ReflectionProbe::beginFace()` has cleared the face.
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
