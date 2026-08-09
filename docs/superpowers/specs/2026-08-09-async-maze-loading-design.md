# Async maze loading + loading screen

## Goal

Show a loading screen (plain background + progress bar, no art asset) between
the intro menu and the maze scene, driven by a real background thread, while
the maze's models load. Replaces the current behavior where model loading
happens lazily but synchronously in `MazeLayer::onRender()` on first show
(a one-frame stall).

## Scope

Narrowed to `Model` only — no changes to `Mesh`, `Texture`, `VertexArray`, or
`AssetManager`'s generic caching internals (beyond one small addition, below).
Their constructors already take pre-decoded data (`Mesh` takes vertex/index
vectors + built `Texture` handles; `Texture` already builds from raw decoded
pixels via `TextureCreateInfo::data`), so the CPU/GPU boundary already exists
at those signatures. Only `Model`'s glTF/OBJ loading path needs to split into
"parse" (CPU-only) and "build" (GL calls) phases.

Why not a shared GL context on the loader thread instead: VAOs are never
shared across GL contexts even within a share group (hard OpenGL rule, not an
implementation gap), and `Mesh` builds a VAO in its constructor. A CPU-only
loader thread sidesteps this entirely — no second context, no hidden window,
no VAO hazard — because the actually-slow work (file I/O, glTF/obj parsing,
`stb_image` decode) is pure CPU already.

## Design

### `Model`: parse/build split

* `ModelData` (new, in `model.hpp`): a plain CPU-side struct — one entry per
  mesh primitive, each holding `vector<Vertex>`, `vector<uint32_t>` indices,
  up to 5 optional decoded images (albedo/normal/occlusion/emissive/
  metallic-roughness — raw pixels + dimensions, no GL handles), material
  scalars, and `MeshUVTransforms`. No `shared_ptr<Texture>`, no `Mesh`, no
  `AssetManager` calls anywhere in this struct or its construction path.
* `Model::parse(const ModelCreateInfo&) -> ModelData` (new static): the CPU
  half of today's `loadGltf`/`loadObj` — same parsing, same
  `stbi_load(_from_memory)` decode, but stops before texture/mesh
  construction. Safe to call from any thread.
* `Model::build(const ModelCreateInfo&, ModelData&&)` (new): the GL half —
  walks the parsed data, calls `AssetManager::createTexture(...)` per decoded
  image (unchanged call, still name-keyed and cache-deduped exactly as
  today), constructs `Mesh` objects (unchanged ctor), accumulates
  vertex/index counts. Must run on the GL thread.
* `Model(const ModelCreateInfo&)` (existing ctor, unchanged behavior): now
  implemented as `build(createInfo, parse(createInfo))` — same eager,
  synchronous, single-thread behavior for any other caller.
* `Model(const ModelCreateInfo&, ModelData&&)` (new ctor): thin wrapper over
  `build()`, used by the render-thread finalize step below.

Dropped: the existing cross-material image dedup (`AssetManager::getTextures()`
lookup during decode) doesn't carry over to the parse phase, since parse must
not touch `AssetManager` (it would race the render thread's own asset
creation on an unsynchronized map). Duplicate materials referencing the same
image are decoded more than once during parse (background thread, CPU-only,
does not block a frame). `AssetManager::createTexture`'s existing name-keyed
cache still dedups the actual GL upload during build, so there's no GPU
memory or correctness regression — only a bounded amount of redundant CPU
decode work for models with many materials sharing one image (sponza).
`// ponytail:` comment marks this at the call site.

### `AssetManager`: preserve the debug models table

`ImGuiLayer::showModelsTable()` reads `AssetManager::getModels()`. Since the
new lazy-built `Model` no longer goes through `AssetHandler::load()` (which
assumes single-shot `make_shared<T>(createInfo)`), add
`AssetHandler<T,C>::insert(name, asset)` (2 lines) and
`AssetManager::registerModel(name, model)` so the render-thread finalize step
can still register the built model for the debug table. No change to the
generic `load()` path or other asset types.

### `MazeLayer`: expose load requests, accept finished models

* `MazeLayer::getModelLoadRequests() const -> vector<ModelCreateInfo>`: built
  from the existing file-local `gameObjects` array, one entry per object.
* `MazeLayer::finishLoading(vector<shared_ptr<Model>> builtModels)`: takes
  models in the same order as `getModelLoadRequests()`. Does everything
  `loadResources()` does today *except* the model-loading loop: computes
  `objectModelMatrices`/`objectEmissives` from `gameObjects` (unchanged, no
  GL), sets `objectModels = builtModels`, creates camera/shader/shadow
  map/cube/FXAA/bloom/clustered lights/depth-prepass FBO (unchanged, still
  GL calls — this part stays synchronous, it's already fast), populates both
  `renderFrames` slots, sets `resourcesReady = true`, moves the existing
  "activate ImGui if `isImguiOpen`" logic here (previously in
  `IntroLayer::activateSelected`, now that `MazeLayer` isn't activated
  directly by `IntroLayer` — see below), and finally `setActive(true)`.
* The `resourcesReady` gate from the current lazy-load change stays exactly
  as-is: `onUpdate()`/`onRender()` still no-op until it's set. It's the
  correctness backstop if activation ordering ever skews by a frame.

### `LoadingLayer` (new): orchestration + progress UI

Render-thread-only layer (`runsOnUpdateThread() == false`), same shape as
`SplashScreenLayer`: its own `OrthoCamera`, a background `Quad`, and two more
`Quad`s for the progress bar (track + fill), projection rebound each frame.
No art asset — solid-color rectangles only, matching the "plain background"
choice.

Progress model: fixed, monotonic step count — `2 * numRequests + 1` (parse
step + upload step per model, plus one step for `finishLoading()`'s fast
setup). Coarse but never rescales backward, which matters more than
precision for a bar the player watches for a few seconds.

State machine, driven from `onUpdate()` (called every frame while active,
same as any render-thread layer):

1. `setActive(true)` (called from `IntroLayer::activateSelected` instead of
   activating `MazeLayer` directly): snapshot
   `getModelLoadRequests()`, reset `completedSteps` to 0, pre-size a
   `vector<ModelData>` to `requests.size()` (disjoint-index writes, no
   mutex needed), and spawn a plain `std::thread` — not the `Worker` class,
   which is built around per-frame blocking `kick()`/`waitForComplete()` and
   doesn't support "poll without blocking" — that calls `Model::parse()` for
   each request in order, writing into its own index and incrementing an
   `std::atomic<uint32_t> completedSteps` after each, then sets
   `std::atomic<bool> parseDone = true`.
2. Every frame: draw background + bar at `completedSteps / totalSteps`.
3. Once `parseDone` is observed true: join the thread (once), then build one
   model per frame via `Model::build`/the new ctor (spreads the GL upload
   cost across frames instead of one hitch), incrementing `completedSteps`
   each time.
4. Once all models are built: call
   `Maze::get().getMazeLayer()->finishLoading(std::move(builtModels))`,
   then `setActive(false)` on itself.

### Wiring

* `Maze` gets a `loadingLayer` member + `getLoadingLayer()`, pushed into the
  layer stack and `setActive(false)` at startup alongside the other menu
  layers.
* `IntroLayer::activateSelected()`'s `NewGame` case: `setActive(false)` on
  itself, `setActive(true)` on `LoadingLayer` (was: directly activating
  `MazeLayer`, including the ImGui-enable check, which moves into
  `MazeLayer::finishLoading()` as noted above).
* `ExitLayer`'s "return to main menu" path is unaffected — `MazeLayer` stays
  loaded (`resourcesReady` stays `true`), so re-entering the maze from the
  exit menu skips `LoadingLayer` entirely and goes straight back to
  `MazeLayer::setActive(true)`.

## Error handling

Matches existing behavior: a failed parse (bad path, unsupported primitive)
logs and produces an empty `ModelData`/`Model` for that entry, same as today
— no new error UI, no retry. Not a regression; the current code has no
recovery path either.

## Testing

No automated test harness for GL-dependent rendering in this codebase
(confirmed: no `test_*` targets under `game/` or `sponge/`). Verification is
manual: launch, start a new game, confirm the loading screen appears and its
bar advances monotonically to 100%, then the maze scene renders normally.
Also verify returning to the main menu via the exit screen and starting a new
game a second time does not re-show the loading screen or reload resources
(covered by the pre-existing `resourcesReady` gate).
