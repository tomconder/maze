# Engine Core

Platform-agnostic sponge engine: layer stack, events, input *types*, settings, logging, Worker, CPU-side scene types.
Does NOT own windowing, GL, or OS file shims — see `platform/AGENTS.md`.
Does NOT own maze gameplay — see `../../game/src/AGENTS.md`.

## Entry Points

* `entrypoint.hpp` — `main` / `WinMain` → `createApplication()` (defined in game).
* `layer/layer.hpp` — `Layer` lifecycle: `onAttach` / `onDetach` / `onEvent` / `onUpdate` / `onRender` / `onFrameSync`.
* `layer/layerstack.hpp` — `pushLayer` (game) vs `pushOverlay` (on top).
* `thread/worker.hpp` — `Worker`: `kick` then `waitForComplete`. Application owns two: update + render.
* `core/settings.hpp` — process-global key/value store; not thread-safe.
* `input/gameaction.hpp`, `input/inputsnapshot.hpp` — game-facing actions. Physical devices bind in GLFW `InputManager`.

`scene/` here is CPU-side (font atlas, mesh data). GPU types live in `platform/opengl/scene/`.

## Contracts & Invariants

* No GLFW, OpenGL, or OS APIs in this tree.
* `Layer::runsOnUpdateThread()` defaults to `false`: `onUpdate()` runs on the **render** thread (GL context current; `onRender()` unused). Opt in with `true` only when update work must be GL-free and overlap render; then `onUpdate()` must issue **no GL calls** and `onRender()` does the GPU work.
* `onFrameSync()` runs on the **main** thread while both workers are idle. Publish the snapshot render[N] will read here — never a slot the update thread might still write.
* `Worker` is kick-then-wait. Do not kick a worker that still has a task; the Application loop owns that discipline.
* New `GameAction` values go in `input/gameaction.hpp` and get bound in `platform/glfw/core/inputmanager.*`. Game code reads `InputSnapshot`, not raw keys.

## Patterns

New layer: subclass `sponge::layer::Layer`. Override `runsOnUpdateThread()` only if update must run in parallel with render. Push via `Application::pushLayer` / `pushOverlay`.

New setting: `Settings::get*` / `set` with a dotted key; persist with `Settings::save()`. Cross-thread readers need their own mutex (see MazeLayer).

## Anti-patterns

* Don't call GLFW/GL from this tree; extend `platform/` instead.
* Don't put maze-specific types here (`MazeRenderFrame` belongs in `game/src/thread/`).
* Don't treat `scene/` as the renderer — that's `platform/opengl/scene/`.
* Don't assume `onUpdate()` is GL-free; it isn't unless `runsOnUpdateThread()` is true.

## Related Context

* Platform backends: `platform/AGENTS.md`
* Game usage: `../../game/src/AGENTS.md`
