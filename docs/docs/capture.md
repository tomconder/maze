# Capture flags (debug builds)

> These arguments exist only in debug builds. A release build ignores them and starts normally.

A capture run skips the splash and intro screens and loads the maze. It uses a fixed 1600x900 window with vsync off, so
runs compare. The camera turns once over the run. The run writes its results to the working directory and then exits.

| Argument                  | Description                                                                                                                                                     |
|---------------------------|-----------------------------------------------------------------------------------------------------------------------------------------------------------------|
| `--capture-frames N`      | Render `N` frames and exit. `--capture-frames=N` also works. A missing, zero or non-numeric `N` gives the normal start.                                         |
| `--dump-nan-stats`        | Scan every stage on every frame for NaN, Inf and isolated bright or dark pixels. `N` defaults to 600 when `--capture-frames` is not given.                      |
| `--capture-off=a,b`       | Turn passes off for the run. Use a comma-separated list of: `aa`, `bloom`, `ssao`, `ssr`, `shadow`, `probe`, `planar`. Also `shadowcache` (draw the shadow map every frame, so its cost shows in the timing) and `gputimer` (turn the GPU timer off). |

Examples:

```
maze.exe --capture-frames 600
maze.exe --capture-frames=600 --dump-nan-stats
maze.exe --capture-frames 1500 --capture-off=ssao,bloom
```

## Output files

| File                                    | Written when                                 | Content                                                                                                                                                              |
|-----------------------------------------|----------------------------------------------|----------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| `capture_<stage>_last.pfm`              | Always                                       | The last frame of each stage as a float image. Stages: `scene` and `bloom` (RGB16F, linear radiance), `tonemap` (the input of FXAA or TAA) and `taa` (0 to 1), `backbuffer` (8-bit). |
| `capture_timing.txt`                    | Without `--dump-nan-stats`                   | Mean, p50 and p95 frame time and FPS, then a `gpu frame` line (the GPU time of the render pass sequence, the span, and the time between two frame starts, the period) a `cpu` line (the median time of the render task, the buffer swap, the update task, and the main thread's waits for the render and the update) and one `gpu` line per pass: the number of calls, the median GPU time of one call and the GPU time per frame. The first 60 frames (warm-up) and the last frame (readback) are left out. |
| `capture_stats.txt`                     | Always                                       | One line per stage: NaN, Inf, values over 1000, bright and dark specks, highest finite value and the first hit.                                                      |
| `capture_specks.txt`                    | Always                                       | The frame and pixel of each speck in the `scene` stage.                                                                                                              |
| `capture_<stage>_first_hit.pfm`         | A stage has a hit                            | The first frame of that stage that has a NaN, Inf or speck.                                                                                                          |

Notes:

* The back buffer cannot show NaN: it reads as black. Use the float stages.
* A stage read stalls the GPU. Without `--dump-nan-stats`, only the last frame is read, so the frame times stay valid.
  With `--dump-nan-stats`, every frame stalls, and no timing file is written.
* Debug builds run slower than release builds, and the CPU cost is higher. Use a capture run to compare passes with each
  other, not to measure the speed of a release build. A 600-frame `--dump-nan-stats` run takes minutes.
