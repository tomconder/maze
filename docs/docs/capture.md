# Capture flags (debug builds)

> These arguments exist only in debug builds. A release build ignores them and starts normally.

A capture run skips the splash and intro screens and loads the maze. (`--capture-menu` shows one menu screen instead, and `--dump-yoga` opens no window.) It uses a 1600x900 window by default, with vsync off, so
runs compare. The camera turns once over the run, unless `--capture-walk` moves it. The run writes its results to the working directory and then exits.

| Argument                  | Description                                                                                                                                                     |
|---------------------------|-----------------------------------------------------------------------------------------------------------------------------------------------------------------|
| `--capture-frames N`      | Render `N` frames and exit. `--capture-frames=N` also works. A missing, zero or non-numeric `N` gives the normal start.                                         |
| `--dump-nan-stats`        | Scan every stage on every frame for NaN, Inf and isolated bright or dark pixels. `N` defaults to 600 when `--capture-frames` is not given.                      |
| `--capture-off=a,b`       | Turn passes off for the run. Use a comma-separated list of: `aa`, `bloom`, `ssao`, `ssr`, `shadow`, `probe`, `planar`. Also `shadowcache` (draw the shadow map every frame, so its cost shows in the timing) and `gputimer` (turn the GPU timer off). |
| `--capture-walk=D`        | Move the camera `D` world units per update along +X and keep its yaw fixed, instead of turning it. A turn never redraws a shadow cascade, so a walk is how to count redraws: `gpu shadow calls` in `capture_timing.txt` is one per cascade redraw. A near cascade redraws when the camera has moved one texel of its map. |
| `--capture-cascades=N`    | Use `N` shadow cascades, 1 to 4. The default is 2. One is the whole-scene map alone, for an A/B against the cascades. |
| `--capture-size=WxH`      | Use a `W` by `H` window for any capture run, for example `3840x2160`. The default is 1600x900. A menu capture has no window frame, so the size can be larger than the screen. |
| `--capture-menu=name`     | Show one menu screen instead of the maze: `intro`, `exit`, `display`, `keyboard`, `audio`, `confirm` (the display menu with the graphical changes prompt open) or `reset` (the keyboard menu with the reset prompt open). The run waits two seconds, writes `capture_menu_<name>.png` and exits. Any other name gives the normal start. |
| `--dump-yoga=name`        | Print the Yoga layout tree of the `intro`, `exit`, `display`, `keyboard`, `audio` menu, or the `confirm` or `reset` prompt, as JSX and exit, with no window. The tab bar and the key hint bar are part of the tree. It uses the `--capture-size` window size. Paste the output into [yogalayout.dev/playground](https://yogalayout.dev/playground) to inspect or change the layout. |

Examples:

```
maze.exe --capture-frames 600
maze.exe --capture-frames=600 --dump-nan-stats
maze.exe --capture-frames 1500 --capture-off=ssao,bloom
maze.exe --capture-frames 600 --capture-walk=0.05 --capture-cascades=3
maze.exe --capture-menu=display --capture-size=3840x2160
maze.exe --capture-menu=exit
maze.exe --dump-yoga=intro --capture-size=1920x1080
```

## Output files

| File                                    | Written when                                 | Content                                                                                                                                                              |
|-----------------------------------------|----------------------------------------------|----------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| `capture_<stage>_last.pfm`              | Always                                       | The last frame of the `scene` and `bloom` stages as a float image (RGB16F, linear radiance). Values above 1 survive.                                                 |
| `capture_<stage>_last.png`              | Always                                       | The last frame of the 0 to 1 stages as an 8-bit image: `tonemap` (the input of FXAA or TAA), `taa` and `backbuffer`.                                                 |
| `capture_menu_<name>.png`               | With `--capture-menu`                        | The menu screen as an 8-bit image.                                                                                                                                   |
| `capture_timing.txt`                    | Without `--dump-nan-stats`                   | Mean, p50 and p95 frame time and FPS, then a `gpu frame` line (the GPU time of the render pass sequence, the span, and the time between two frame starts, the period) a `cpu` line (the median time of the render task, the buffer swap, the update task, and the main thread's waits for the render and the update) and one `gpu` line per pass: the number of calls, the median GPU time of one call and the GPU time per frame. The first 60 frames (warm-up) and the last frame (readback) are left out. |
| `capture_stats.txt`                     | Always                                       | One line per stage: NaN, Inf, values over 1000, bright and dark specks, highest finite value and the first hit.                                                      |
| `capture_specks.txt`                    | Always                                       | The frame and pixel of each speck in the `scene` stage.                                                                                                              |
| `capture_<stage>_first_hit.pfm`         | A stage has a hit                            | The first frame of that stage that has a NaN, Inf or speck.                                                                                                          |

Notes:

* The back buffer cannot show NaN: it reads as black. Use the float stages. PNG files hold 8 bits, so radiance above 1 and NaN
  do not show in them; the `.pfm` files keep them. Debug builds link libspng for the PNG files.
* A stage read stalls the GPU. Without `--dump-nan-stats`, only the last frame is read, so the frame times stay valid.
  With `--dump-nan-stats`, every frame stalls, and no timing file is written.
* Debug builds run slower than release builds, and the CPU cost is higher. Use a capture run to compare passes with each
  other, not to measure the speed of a release build. A 600-frame `--dump-nan-stats` run takes minutes.
