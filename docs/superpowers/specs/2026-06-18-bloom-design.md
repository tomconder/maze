# Bloom Post-Processing Effect

Date: 2026-06-18

## Overview

Add a Dual Kawase bloom post-processing effect. Bloom is toggleable and exposes
threshold and intensity as tunable parameters in the options UI.

## Architecture

Render order: scene → bloom extract+blur → FXAA → screen.

`Bloom` owns the scene-capture FBO so FXAA's begin/end API is unchanged. FXAA
gains an `applyWithBloom(sceneTexId, bloomTexId, intensity)` overload. When
bloom is off, `bloomTexId = 0` and the shader skips the additive pass —
FXAA-only behavior is identical to before.

### Render paths

**Bloom + FXAA:**

```
bloom->begin()
  renderSceneToDepthMap / renderGameObjects / renderLightCubes
bloom->end()
bloom->process()
fxaa->applyWithBloom(bloom->getSceneTexture(), bloom->getBloomTexture(), intensity)
```

**Bloom only (FXAA off):**

```
bloom->begin() → render → bloom->end() → bloom->process() → bloom->apply()
```

**FXAA only (unchanged):**

```
fxaa->begin() → render → fxaa->end() → fxaa->apply()
```

**Neither:** render directly to default framebuffer (unchanged).

## Components

### `Bloom` class

Location: `sponge/src/platform/opengl/scene/bloom.hpp/cpp`

Public interface (mirrors FXAA):

* `Bloom(uint32_t width, uint32_t height)`
* `begin()` — bind scene-capture FBO
* `end()` — unbind
* `process()` — extract → 5× downsample → 5× upsample
* `apply()` — composite scene + bloom to default FB (bloom-only path)
* `getSceneTexture()` / `getBloomTexture()` — textures for FXAA composite
* `resize(uint32_t w, uint32_t h)` — rebuild all FBOs
* `setEnabled(bool)` / `isEnabled()`
* `setThreshold(float)` / `getThreshold()`
* `setIntensity(float)` / `getIntensity()`

Internals:

* 1 scene-capture FBO + texture
* 10 mip-chain FBOs + textures (5 downsample levels, 5 upsample levels)
* Each level is half the resolution of the previous
* All FBOs checked with `glCheckFramebufferStatus`; `SPONGE_GL_CRITICAL` if incomplete

### Shaders

All reuse `blur.vert.glsl` as the vertex shader.

| File | Purpose |
|------|---------|
| `bloom_extract.frag.glsl` | Luminance threshold — outputs bright pixels only |
| `bloom_down.frag.glsl` | Dual Kawase downsample, RGB (adapts `blur.frag.glsl`) |
| `bloom_up.frag.glsl` | Dual Kawase upsample, RGB (adapts `blur_up.frag.glsl`) |
| `bloom_composite.frag.glsl` | `scene + bloom * intensity` — bloom-only composite path |

FXAA fragment shader (`fxaa.frag.glsl`) gains a `bloomTex` sampler and
`bloomIntensity` uniform; additive bloom is skipped when `bloomIntensity == 0`.

### `MazeLayer` changes

* Add `std::unique_ptr<sponge::platform::opengl::scene::Bloom> bloom`
* Add fields: `bloomEnabled` (bool), `bloomThreshold` (float), `bloomIntensity` (float)
* Add getters/setters for each
* `captureRenderFrame()` copies all three into `MazeRenderFrame`
* `onRender()` branches on `frame.bloomEnabled` and `frame.fxaaEnabled`
* `onWindowResize` defers bloom resize alongside FXAA resize

### `MazeRenderFrame` changes

Add: `bool bloomEnabled`, `float bloomThreshold`, `float bloomIntensity`.

### `OptionLayer` changes

Bloom section alongside existing FXAA toggle:

* Bloom enable/disable checkbox
* Threshold slider (e.g. 0.0–1.0)
* Intensity slider (e.g. 0.0–3.0)

## Error Handling

`glCheckFramebufferStatus` after each of the 11 FBO creations; `SPONGE_GL_CRITICAL`
if any is incomplete. On resize: destroy all FBOs, then recreate.

## Verification

Manual:

* Toggle bloom on/off in options — effect appears/disappears
* Threshold slider cuts off dim areas; bright areas always bloom
* Intensity slider scales the glow strength
* FXAA-only path (bloom off) is visually identical to pre-change behavior
* Both on together: bloom composites correctly before AA pass
