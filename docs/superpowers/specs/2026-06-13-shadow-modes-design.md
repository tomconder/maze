# Shadow Modes Design — PCF / PCSS / DPCF / EVSM

**Date:** 2026-06-13

## Goal

Add three additional shadow filtering techniques — PCSS, DPCF, and EVSM — alongside the existing PCF implementation, selectable at runtime via the ImGui panel.

## Enum

```cpp
enum class ShadowMode { PCF, PCSS, DPCF, EVSM };
```

Location: `sponge/src/platform/opengl/scene/shadowmode.hpp` (new file).

## Architecture

Single ubershader approach: all four shadow functions live in `pbr.frag.glsl`; a `uniform int shadowMode` selects which runs. This mirrors the existing `AntiAliasingMode` enum pattern.

## ShadowMap changes (`sponge/src/platform/opengl/scene/shadowmap.{hpp,cpp}`)

* Add `setMode(ShadowMode)` — recreates GPU texture and FBO when switching to/from EVSM.
* Add `momentMap` texture (`GL_RG32F`, same resolution as depth map) — allocated only when mode == `EVSM`.
* Add a two-pass separable Gaussian blur applied to the moment map after the depth pass (ping-pong FBO). Without it, EVSM aliasing at shadow edges is severe.
* Replace `activateAndBindDepthMap(unit)` with `activateAndBindShadowTexture(unit)` — binds the depth map for PCF/PCSS/DPCF, or the blurred moment map for EVSM.
* Load a second shadow-pass fragment shader (`shadowmap_evsm.frag.glsl`) when mode == `EVSM`. This shader writes `vec2(depth, depth * depth)` to the RG32F target instead of discarding.

## New shader files

| File | Purpose |
|------|---------|
| `assets/shaders/glsl/shadowmap_evsm.frag.glsl` | Writes `(depth, depth²)` moments to RG32F FBO |
| `assets/shaders/glsl/blur.vert.glsl` | Full-screen quad vert for Gaussian blur passes |
| `assets/shaders/glsl/blur.frag.glsl` | Separable Gaussian blur (horizontal + vertical) |

## pbr.frag.glsl changes

Add uniforms:

```glsl
uniform int       shadowMode;  // 0=PCF, 1=PCSS, 2=DPCF, 3=EVSM
uniform sampler2D momentMap;   // RG32F (EVSM only)
```

Four shadow functions:

**`shadowPCF`** — existing 3×3 kernel, unchanged.

**`shadowPCSS`** — two-pass:

1. Blocker search: sample depth map in a square region proportional to `lightSize`; average the depths of samples shallower than the receiver.
2. Penumbra estimation: `penumbraWidth = (receiver - avgBlocker) / avgBlocker * lightSize`.
3. Variable-radius PCF using the penumbra width.

**`shadowDPCF`** — Poisson disk PCF:

* 16-sample Poisson disk pattern (baked constants).
* Fixed soft radius (configurable via `pcfRadius` uniform).
* Faster than PCSS; softer than basic PCF; no blocker search.

**`shadowEVSM`** — moment-based:

1. Sample `momentMap` to get `(M1, M2)`.
2. Compute variance: `var = M2 - M1 * M1`.
3. Apply Chebyshev's upper bound: `p = var / (var + (depth - M1)²)`.
4. Clamp light bleeding: `shadow = max(p, threshold)` where `threshold ≈ 0.2`.

`main()` selection:

```glsl
if (shadowMode == 0)      shadow = shadowPCF(...);
else if (shadowMode == 1) shadow = shadowPCSS(...);
else if (shadowMode == 2) shadow = shadowDPCF(...);
else                      shadow = shadowEVSM(...);
```

## MazeRenderFrame changes (`game/src/thread/mazeframe.hpp`)

Add:

```cpp
ShadowMode shadowMode{ ShadowMode::PCF };
```

## MazeLayer changes (`game/src/layer/mazelayer.{hpp,cpp}`)

* Add `shadowMode` member (default `ShadowMode::PCF`).
* Add `getShadowMode()` / `setShadowMode(ShadowMode)`.
  * `setShadowMode` calls `shadowMap->setMode()` and updates `uniform int shadowMode` on the PBR shader.
* `captureRenderFrame()` copies `shadowMode` into the snapshot.
* `renderGameObjects()` binds the correct texture unit (`shadowMap->activateAndBindShadowTexture(1)`) and sets the `shadowMode` uniform.
* `renderSceneToDepthMap()` uses the EVSM shadow-pass shader + blur when `frame.shadowMode == EVSM`; otherwise uses the existing `shadowmap` shader.

## ImGui changes (`game/src/layer/imgui/imguilayer.cpp`)

Add a "Shadow Mode" row to the directional lights table, above the existing "Cast Shadow" row:

```cpp
const char* shadowModes[] = { "PCF", "PCSS", "DPCF", "EVSM" };
int currentMode = static_cast<int>(mazeLayer->getShadowMode());
if (ImGui::Combo("##shadowmode", &currentMode, shadowModes, 4)) {
    mazeLayer->setShadowMode(static_cast<ShadowMode>(currentMode));
}
```

## New uniforms exposed (all on the PBR shader)

| Uniform | Type | Purpose |
|---------|------|---------|
| `shadowMode` | `int` | Selects active shadow function |
| `momentMap` | `sampler2D` | EVSM moment texture (unit 2) |
| `pcssLightSize` | `float` | Light size for PCSS blocker search |
| `pcfRadius` | `float` | Disk radius for DPCF |
| `evsmBleedThreshold` | `float` | Light bleed clamp for EVSM |

## File change summary

| File | Change |
|------|--------|
| `sponge/src/platform/opengl/scene/shadowmode.hpp` | New — `ShadowMode` enum |
| `sponge/src/platform/opengl/scene/shadowmap.hpp` | Add `setMode`, moment texture, blur FBO |
| `sponge/src/platform/opengl/scene/shadowmap.cpp` | Implement above |
| `assets/shaders/glsl/pbr.frag.glsl` | Add 3 shadow functions + mode uniform |
| `assets/shaders/glsl/shadowmap_evsm.frag.glsl` | New — writes moments |
| `assets/shaders/glsl/blur.vert.glsl` | New — fullscreen quad |
| `assets/shaders/glsl/blur.frag.glsl` | New — separable Gaussian |
| `game/src/thread/mazeframe.hpp` | Add `shadowMode` field |
| `game/src/layer/mazelayer.hpp` | Add mode getter/setter |
| `game/src/layer/mazelayer.cpp` | Wire mode through render path |
| `game/src/layer/imgui/imguilayer.cpp` | Add Shadow Mode combo |
