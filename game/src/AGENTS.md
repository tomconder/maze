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
* `capture/framecapture.hpp` / `.cpp` - the `--capture-frames` and
  `--dump-nan-stats` debug scan (see the root `AGENTS.md`). Parsed in
  `createApplication()` under `#ifndef NDEBUG`; `MazeLayer::recordCapture()`
  feeds it float readbacks on the render thread. Test NaN and Inf by bits,
  through a pointer: the build uses fast math, so `std::isnan`, `std::isinf`
  and a bit test on a float passed by value all fold to false.
* `capture/menucapturelayer.hpp` / `.cpp` - the `--capture-menu` debug capture.
  `Maze::onUserCreate()` pushes it after every other layer, so it reads a
  frame all menus have drawn, and shows the named screen through
  `introLayer` or `ui::showOptionTab()`. It writes a PNG with libspng, which
  only debug builds link (`game/CMakeLists.txt`).
* `layer/buttonmenulayer.hpp` / `.cpp` - the base of the intro and exit menus:
  a column of buttons with keyboard, gamepad and mouse selection. A subclass
  gives a `ButtonMenuStyle` (labels, key hints, colours, `MenuLayoutSpec`) and
  overrides `onSelect()`, and `onBack()` if it has a back action. The layout is
  a `ui::MenuLayout` member (`ui/menulayout.hpp`), which owns the Yoga tree and
  frees it. `--dump-yoga=intro|exit|display|keyboard|audio` prints that tree as JSX for
  yogalayout.dev/playground (`ui/yogadump.hpp`).
* `layer/scrollmenulayer.hpp` / `.cpp` - the base of an options tab with rows
  under the tab bar. It owns the camera, font, `ui::MenuLayout`, `MenuScroll`,
  selection, hover, tab switching, wheel and scroll bar handling. A subclass
  gives a `ScrollMenuStyle` and overrides `renderRow()`, plus `onRowKeys()`,
  `onRowPressed()` (return true to drag), `onRowDrag()`, `onRowReleased()`,
  `onConfirm()`, `onFontSizeChanged()`, `onResized()`, `onOpened()`,
  `onClosed()`, `isSelectable()` and `takesInput()` as needed. A row that is
  not selectable is a label: no highlight, hover or click, and the selection
  hops over it. A style can name one clickable key hint (`actionHint`, handled
  in `onActionHint()`) and the row selected on open (`firstRow`). All three
  option tabs use it.
* `layer/introlayer.hpp`, `layer/optionlayer.hpp`, `layer/keymaplayer.hpp`,
  `layer/audiolayer.hpp`, `layer/exitlayer.hpp`, `layer/splashscreenlayer.hpp`
  - other screens in the layer stack. The options screen is one layer per tab:
  `OptionLayer` is Display, `KeyMapLayer` is Keyboard, `AudioLayer` is Audio.
  Only one is ever active; `ui/tabbar.hpp` draws the shared tab strip and owns
  the swap (`showOptionTab`) and the wraparound next/prev order
  (`cycleTab`). The tab strip is a row of five nodes at the top of the Yoga root (`MenuLayoutSpec::optionTab`):
  the two prompt icons around one node per label. Label sizes come from the font (`setTabLabelSizes()`), and `renderTabBar()` draws where `MenuLayout::tabBarRects()` says. `--dump-yoga` measures them with `ui/fontmetrics.hpp`, which needs no GL. `KeyMapLayer` rebinds keys through
  `InputManager::requestRebind()`; the key capture and the settings write both
  happen on the main thread, never in the layer. `AudioLayer`'s volume rows
  are `ui::Slider` (drag, click-to-jump, clicks on the arrows, and
  keyboard/gamepad step), applied live to `sponge::platform::audio::Audio` and
  written to `Settings` on every change, but only flushed to disk on
  drag-release or a discrete step, not on every mouse-move frame of a drag.
  The three layers scroll through `ui/menuscroll.hpp`. The setting rows are
  fixed-height Yoga column nodes laid out once; the scroll offset is subtracted from
  their computed y in `ScrollMenuLayer::rowRect()`. No screen has a
  pinned row now. Esc is the only way back; there is no Return row.
  The key hint bar (`ui/keyhints.hpp`) is a row of Yoga nodes below the menu
  (an icon and a label per hint, sized by `keyHintSizes()`, drawn where
  `MenuLayout::keyHintRects()` says). It puts Back at the left margin, then any
  hint with `leftAligned` set, and right-aligns the rest. The layers lay out
  again when the active device changes, because the prompt sizes differ. Reset to Defaults is
  a left-aligned hint on the Keyboard tab: the `MenuReset` action (R, gamepad
  Y), or a click on the hint (`hitTestKeyHints()`), opens a `ConfirmLayer`
  prompt with Cancel selected and no countdown. Only Accept calls
  `requestResetBindings()`. The Keyboard tab ignores its keys while that
  prompt is open.
  The target and the drawn offset are in fractional rows, so a row can rest
  half in view; the offset eases toward the target
  (`MenuScroll::step()`, which relays out each frame it moves). Every render
  loop skips rows with `shows()` and draws between `beginClip()` and
  `endClip()`, which use `RendererAPI::setScissor()` (the ortho camera and the
  viewport share one pixel size). Hover and click loops use `canHit()`.
  Follow the selection only when it changes (`followSelection()`), or the
  wheel snaps back.
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
* Shadow quality is `ShadowQuality` (Low, Medium, High), saved as
  `video.shadowQuality`. Each level sets the map size and the cascade count
  together (`shadowPresets` in `mazelayer.cpp`); the options row and the debug
  combo both call `MazeLayer::setShadowQuality()`. The scene file has no
  shadow size. The debug UI keeps only Cascade Split as a free control.
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

* The Display tab has three sections, General, Quality and Advanced. Each
  starts with a heading row (`isHeading()` in `optionlayer.cpp`), which takes a
  scroll slot but is never selected, hovered or clicked.
* The Display tab has no Apply row: every row applies the moment it changes.
  A Display Mode change also opens `layer/confirmlayer.hpp` ("Confirm
  Graphical Changes", 15 second countdown, Accept selected), which draws a
  `ui::ConfirmDialog`. It is the top overlay and modal: it takes the mouse
  events first, and `OptionLayer` skips its keys while it is active. It runs
  its callback after it goes inactive, and ignores the key press that opened
  it. `--capture-menu=confirm` shows it over the Display tab, and
  `--capture-menu=reset` shows the Reset to Defaults prompt over the Keyboard tab. `show()` takes the
  title, message, countdown seconds (0 or less: no countdown, no timeout) and
  `defaultAccept`, so other screens can reuse it; give a destructive action
  `defaultAccept = false`. Accept saves
  `video.displayMode`; Cancel, Esc or the countdown ending puts back the mode
  held in `previousDisplayMode`. Nothing is saved before Accept, so a crash
  with a bad mode open starts with the old one. Resolution, Aspect Ratio (which
  applies the first resolution of the new ratio), Anti-aliasing and Shadow
  Quality apply and save at once with no prompt. Mode and size changes reach the window
  asynchronously on the main thread, so `onWindowResize()` re-syncs the rows.
* The Display tab's Ambient Occlusion, Bloom and Subsurface Scattering
  rows apply on the spot and save `video.ssaoEnabled`,
  `video.bloomEnabled` and `video.translucencyEnabled`. Subsurface Scattering is the PBR shader's diffuse
  transmission; the `translucencyEnabled` uniform zeroes its factor, so it is
  a shader uniform and not part of the render snapshot. The options screen
  opens before the maze loads, so `MazeLayer` reads these settings in its
  constructor, and its setters must skip a null `Mesh::getShader()`;
  `finishLoading()` sends the stored values to the shader.
* Refraction has no option and is off (`MazeLayer::refractionEnabled`, false).
  `captureRenderFrame()` blanks `objectRefraction`, so glass takes the opaque
  path (prepass, shadows, opaque color) and the glass pass is skipped. The
  shadow cache is cleared if the flag changes, because glass then joins or
  leaves the casters.

* Menu scale follows the window width. `ui::menuFontSizeForWidth()` returns the
  baked font size nearest to `32 * width / 1920`, and the option rows, tab
  bar and key hints derive from it. Checked with `--capture-menu` (all four
  screens) at 1024x768 and 3840x2160, and by screenshot at 1280x720 and
  1920x1080. `ui::Slider` scales its track, handle and gaps from its font size
  (its `SliderCreateInfo` sizes are pixels at the 32 px font). These do not
  scale yet:
  * The 10 px margin in `MenuLayout`.
  Scale these from the font size when one of them needs the fix.
* The font snaps to the nearest baked size, about 10 to 15% apart, so a width
  between two steps gets the nearest one. Exact scaling needs `BitmapFont` to
  scale a baked size at draw time.

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
