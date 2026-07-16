# Penrose Visualization — Complete User Guide

This guide explains **every visualization-related feature** in Penrose: benchmark data generation, scientific analysis plots, CPU presentation rendering, interactive viewing, and direct C++ integration.

You do **not** need to understand C++ or CMake internals to follow the command-line workflows below. Copy the commands from your terminal, run them from the **repository root** (the folder that contains `CMakeLists.txt`), and use the paths printed in the console.

---

## Table of contents

1. [What Penrose visualization includes](#1-what-penrose-visualization-includes)
2. [Prerequisites and first-time setup](#2-prerequisites-and-first-time-setup)
3. [Building everything (CMake cheat sheet)](#3-building-everything-cmake-cheat-sheet)
4. [Where outputs are saved](#4-where-outputs-are-saved)
5. [Benchmark generation (physics CSVs)](#5-benchmark-generation-physics-csvs)
6. [Validation and scientific figures (Python)](#6-validation-and-scientific-figures-python)
7. [CPU presentation renderer](#7-cpu-presentation-renderer)
8. [Interactive viewer](#8-interactive-viewer)
9. [Headless export (still images and animations)](#9-headless-export-still-images-and-animations)
10. [Example programs (no CSV required)](#10-example-programs-no-csv-required)
11. [Direct C++ integration (for developers)](#11-direct-c-integration-for-developers)
12. [Performance: faster playback and higher FPS](#12-performance-faster-playback-and-higher-fps)
13. [Architecture summary](#13-architecture-summary)
14. [Troubleshooting](#14-troubleshooting)

---



## 1. What Penrose visualization includes

Penrose has **three separate visualization paths**:


| Path                         | What it does                                                                    | Typical executable                                         |
| ---------------------------- | ------------------------------------------------------------------------------- | ---------------------------------------------------------- |
| **GPU real-time ray tracer** | Interactive Schwarzschild ray marching in OpenGL                                | `build/Penrose`                                            |
| **CPU presentation module**  | Scene-based orbit viewer with screen-space lensing, starfield, PPM export       | `build/visualization_viewer`, `build/visualization_export` |
| **Scientific analysis**      | Reads benchmark CSVs, produces metrics JSON, PNG/PDF figures, validation report | Python modules under `visualization/scientific/`           |


This guide focuses on the **CPU presentation module**, benchmarks, and analysis tooling. For the GPU app, see `[docs/RUNNING.md](../RUNNING.md)`.

The CPU renderer:

- Draws a **black hole shadow** (flat black disc at Schwarzschild radius `rs = 1`)
- Applies **simple screen-space gravitational lensing** aligned to the projected `rs`
- Shows **trajectory trails** and an **orbiting particle** with playback
- Does **not** run GR ray tracing or modify physics state at render time

---



## 2. Prerequisites and first-time setup



### Required software


| Tool                 | Minimum version                        | Purpose                                  |
| -------------------- | -------------------------------------- | ---------------------------------------- |
| **Git**              | any recent                             | Clone the repository                     |
| **CMake**            | 3.22+                                  | Configure builds                         |
| **C++ compiler**     | C++20 (GCC 11+, Clang 14+, MSVC 2019+) | Compile Penrose                          |
| **Python**           | 3.9+                                   | Validation scripts and PPM→video helpers |
| **OpenGL / display** | —                                      | Interactive viewer and GPU app only      |




### vcpkg (dependency manager)

Penrose uses [vcpkg](https://github.com/microsoft/vcpkg) for libraries such as **Eigen3** and **GLM**.

**One-time vcpkg install (Linux/macOS example):**

```bash
git clone https://github.com/microsoft/vcpkg.git ~/vcpkg
~/vcpkg/bootstrap-vcpkg.sh
```

If you clone vcpkg **inside** the Penrose repo as `vcpkg/`, CMake will detect it automatically. Otherwise pass the toolchain explicitly (see below).

### Clone Penrose and configure the build

```bash
git clone <your-penrose-repo-url> penrose
cd penrose
```

**Configure** (creates the `build/` folder with Make/Ninja files):

```bash
# If vcpkg is NOT at ./vcpkg — set your path:
cmake -B build -S . -DCMAKE_TOOLCHAIN_FILE=$HOME/vcpkg/scripts/buildsystems/vcpkg.cmake

# If vcpkg is at ./vcpkg (auto-detected):
cmake -B build -S .
```

You only need to run `cmake -B build -S .` again when `CMakeLists.txt` or dependencies change.

### Compile

```bash
cmake --build build
```

On Windows with Visual Studio generators you may use:

```powershell
cmake --build build --config Release
```

All executables appear under `build/` (or `build/Debug/` on some Windows setups). Throughout this guide we assume `./build/<name>` on Linux/macOS.

---



## 3. Building everything (CMake cheat sheet)

You do **not** need to build the whole project every time. Build only the target you need:

```bash
cmake --build build --target <TARGET_NAME>
```



### All visualization-related targets


| Target                         | Type           | What you get                                       |
| ------------------------------ | -------------- | -------------------------------------------------- |
| `benchmark_test`               | executable     | Generates physics benchmark CSVs                   |
| `visualization`                | static library | CPU renderer core (used by other targets)          |
| `visualization_test`           | executable     | Unit tests (determinism, CSV, camera)              |
| `visualization_export`         | executable     | Headless PPM still / frame sequence                |
| `visualization_viewer`         | executable     | Interactive GLFW viewer                            |
| `visualization_viewer_example` | executable     | Viewer with built-in integrated orbit (no `--csv`) |
| `visualization_example`        | executable     | Headless still from in-memory integration          |
| `visualization_benchmark`      | executable     | Headless frame-time benchmark (no display)         |
| `Penrose`                      | executable     | GPU real-time ray tracer                           |




### Build the full visualization stack in one go

```bash
cmake --build build --target \
  benchmark_test \
  visualization_test \
  visualization_export \
  visualization_viewer \
  visualization_viewer_example \
  visualization_example \
  visualization_benchmark
```



### CMake options


| Option                 | Default | Effect                                                                                          |
| ---------------------- | ------- | ----------------------------------------------------------------------------------------------- |
| `PENROSE_BUILD_VIEWER` | `ON`    | Build `visualization_viewer` and `visualization_viewer_example`. Set `OFF` on headless servers. |


```bash
cmake -B build -S . -DPENROSE_BUILD_VIEWER=OFF
cmake --build build --target visualization_export
```

---



## 4. Where outputs are saved

All scripted outputs use **timestamped run directories** under `outputs/`:

```text
outputs/
├── benchmark_data/YYYY-MM-DD_HH-MM-SS/
│   ├── freefall.csv
│   ├── orbital.csv
│   ├── null_b_*.csv
│   ├── benchmark_metrics.json      (after analyze_benchmarks)
│   └── benchmark_validation.md     (after generate_report)
├── validation_figures/YYYY-MM-DD_HH-MM-SS/
│   └── *.png, *.pdf
└── rendered_frames/YYYY-MM-DD_HH-MM-SS/
    └── frame.ppm  or  frame_000000.ppm, ...
```

**Finding the latest benchmark folder:**

```bash
ls -td outputs/benchmark_data/*/ | head -1
```

Use that path wherever this guide writes `outputs/benchmark_data/<timestamp>/`.

---



## 5. Benchmark generation (physics CSVs)

Benchmarks integrate geodesics in Schwarzschild spacetime and write CSV trajectories. The visualization viewer reads these CSVs.

### Build and run

```bash
cmake --build build --target benchmark_test
./build/benchmark_test | tee build/benchmark_run.log
```

Runtime is typically **under a minute** (null-geodesic sweeps were reduced for faster iteration).

### Expected CSV files

Each run creates a new folder, for example:

```text
outputs/benchmark_data/2026-07-17_03-00-00/
├── freefall.csv
├── orbital.csv
├── null_b_2.597076.csv
├── null_b_2.598086.csv
└── null_b_2.599076.csv
```



### What to do next

Use `orbital.csv` for orbit animations, `freefall.csv` for radial infall, and `null_b_*.csv` for photon paths. Pass any of them to the viewer or export tool with `--csv`.

---



## 6. Validation and scientific figures (Python)

Read-only Python tooling lives in `visualization/scientific/`. It **never modifies** physics C++ sources.

### Install Python dependencies

From the repo root (use a venv if you prefer):

```bash
pip install numpy matplotlib pandas
```



### Step 1 — Analyze metrics

```bash
python -m visualization.scientific.analyze_benchmarks
```

Reads the **latest** `outputs/benchmark_data/<timestamp>/` by default.

**Output:** `outputs/benchmark_data/<timestamp>/benchmark_metrics.json`

**Use a specific run:**

```bash
python -m visualization.scientific.analyze_benchmarks \
  --data-dir outputs/benchmark_data/2026-07-17_03-00-00
```



### Step 2 — Plot figures

```bash
python -m visualization.scientific.plot_benchmarks
```

**Output:** new directory `outputs/validation_figures/<timestamp>/` with PNG and PDF pairs (orbital top view, invariants, freefall, null geodesics, etc.).

### Step 3 — Validation report

```bash
python -m visualization.scientific.generate_report
```

**Output:** `outputs/benchmark_data/<latest-timestamp>/benchmark_validation.md`

### Full validation workflow (copy-paste)

```bash
./build/benchmark_test | tee build/benchmark_run.log
python -m visualization.scientific.analyze_benchmarks
python -m visualization.scientific.plot_benchmarks
python -m visualization.scientific.generate_report
```

---



## 7. CPU presentation renderer



### What gets drawn


| Element      | Description                                       |
| ------------ | ------------------------------------------------- |
| Starfield    | Procedural background stars                       |
| Black hole   | Pure black disc at projected Schwarzschild radius |
| Trajectory   | Gold trail up to current playback time            |
| Particle     | Soft glow at current playback position            |
| Post-process | Bloom, vignette, lensing warp, white ring at `rs` |


Use `--classic` on export/viewer to **disable post-processing** (faster, debug-style look).

### Presentation vs classic


| Mode                   | Post-processing          | Typical use            |
| ---------------------- | ------------------------ | ---------------------- |
| Default (presentation) | Lensing, bloom, vignette | Figures, viewer        |
| `--classic`            | Off                      | Debugging, maximum FPS |


---



## 8. Interactive viewer



### Build

```bash
cmake --build build --target visualization_viewer
```

Requires a graphical display (X11/Wayland on Linux, Windows/macOS desktop).

### Run with benchmark CSV

Replace `<timestamp>` with your actual folder name:

```bash
./build/visualization_viewer \
  --csv outputs/benchmark_data/<timestamp>/orbital.csv
```

Multiple trajectories:

```bash
./build/visualization_viewer \
  --csv outputs/benchmark_data/<timestamp>/orbital.csv \
  --csv outputs/benchmark_data/<timestamp>/freefall.csv
```

Playback **starts automatically** when a CSV is loaded.

### All viewer command-line options


| Option       | Default | Description                                 |
| ------------ | ------- | ------------------------------------------- |
| `--width W`  | `1280`  | Internal render width (pixels)              |
| `--height H` | `720`   | Internal render height (pixels)             |
| `--csv PATH` | —       | Load trajectory CSV (repeatable)            |
| `--classic`  | off     | Disable lensing / bloom post-process        |
| `--speed S`  | `1.0`   | Playback speed multiplier (orbit evolution) |
| `--help`     | —       | Show usage                                  |


**Examples:**

```bash
# Smaller window, faster orbit evolution
./build/visualization_viewer \
  --csv outputs/benchmark_data/<timestamp>/orbital.csv \
  --width 960 --height 540 \
  --speed 8

# Maximum rendering speed (no lensing)
./build/visualization_viewer \
  --csv outputs/benchmark_data/<timestamp>/orbital.csv \
  --classic
```



### Keyboard and mouse controls


| Input                  | Action                                                 |
| ---------------------- | ------------------------------------------------------ |
| **Left drag**          | Orbit camera around the black hole                     |
| **Middle drag**        | Pan camera                                             |
| **Scroll**             | Zoom                                                   |
| **Space**              | Play / pause animation                                 |
| **Left / Right arrow** | Scrub backward / forward (~1% of orbit per frame held) |
| **Home**               | Jump to start of trajectory                            |
| **End**                | Jump to end of trajectory                              |
| **1 – 9**              | Select which loaded trajectory to highlight            |
| **Escape**             | Quit                                                   |


The console prints **FPS** and **milliseconds per frame** about once per second.

### Interactive example (no CSV, integrated orbit)

Integrates an orbit in-process and opens the viewer immediately:

```bash
cmake --build build --target visualization_viewer_example
./build/visualization_viewer_example
```

Source: `[examples/interactive_viewer/main.cpp](../../examples/interactive_viewer/main.cpp)`

---



## 9. Headless export (still images and animations)



### Build

```bash
cmake --build build --target visualization_export
```



### Single still (default path)

```bash
./build/visualization_export \
  --csv outputs/benchmark_data/<timestamp>/orbital.csv
```

Writes:

```text
outputs/rendered_frames/YYYY-MM-DD_HH-MM-SS/frame.ppm
```

By default a **still** shows the trajectory at **final playback time** (complete orbit drawn).

### Custom output path and resolution

```bash
./build/visualization_export \
  --csv outputs/benchmark_data/<timestamp>/orbital.csv \
  --output outputs/rendered_frames/my_orbit.ppm \
  --width 1920 --height 1080
```



### Frame sequence (animation)

```bash
./build/visualization_export \
  --csv outputs/benchmark_data/<timestamp>/orbital.csv \
  --frames 120 \
  --width 1280 --height 720
```

Writes:

```text
outputs/rendered_frames/<timestamp>/frame_000000.ppm
outputs/rendered_frames/<timestamp>/frame_000001.ppm
...
```

Frames interpolate playback time from start to end.

### All export command-line options


| Option                | Default                                 | Description                 |
| --------------------- | --------------------------------------- | --------------------------- |
| `--csv PATH`          | —                                       | Trajectory CSV (repeatable) |
| `--output PATH`       | auto timestamped                        | Output PPM for single frame |
| `--output-dir PATH`   | auto timestamped                        | Directory for sequences     |
| `--frames N`          | `1`                                     | Number of frames            |
| `--width W`           | `1280`                                  | Image width                 |
| `--height H`          | `720`                                   | Image height                |
| `--camera-distance D` | cinematic default                       | Camera distance             |
| `--camera-yaw Y`      | cinematic default                       | Camera yaw (radians)        |
| `--camera-pitch P`    | cinematic default                       | Camera pitch (radians)      |
| `--time T`            | end (stills) / interpolated (sequences) | Explicit playback time      |
| `--classic`           | off                                     | Disable post-processing     |
| `--help`              | —                                       | Show usage                  |




### Convert PPM sequence to video

```bash
python realtime/visualization/ppm_to_video.py
```

See also `[PPM_TO_VIDEO_README.md](PPM_TO_VIDEO_README.md)` and `[FRAME_CAPTURE.md](FRAME_CAPTURE.md)`.

---



## 10. Example programs (no CSV required)



### Headless integration example

Integrates an orbit with the scientific engine and writes one PPM:

```bash
cmake --build build --target visualization_example
./build/visualization_example
```

Source: `[examples/direct_integration/main.cpp](../../examples/direct_integration/main.cpp)`

Output: `outputs/rendered_frames/<timestamp>/direct_integration.ppm`

### Interactive integration example

```bash
cmake --build build --target visualization_viewer_example
./build/visualization_viewer_example
```

Source: `[examples/interactive_viewer/main.cpp](../../examples/interactive_viewer/main.cpp)`

---



## 11. Direct C++ integration (for developers)

If you are embedding the CPU renderer in your own C++ code:

### Minimal headless flow

```cpp
#include "simulation/TrajectorySolver.h"
#include "Scene/Scene.h"
#include "Trajectory/TrajectoryAdapter.h"
#include "Presentation/PresentationDefaults.h"
#include "Presentation/PresentationPipeline.h"
#include "Camera/Camera.h"
#include "Renderer/Framebuffer.h"
#include "IO/PpmWriter.h"

std::vector<State> history = Simulation::TrajectorySolver::solve(
    initial_state, dynamics, termination_policy, dt, max_steps);

viz::Scene scene(viz::cinematic_scene_settings());
viz::TrajectoryAdapterOptions options;
options.name = "orbital";
scene.add_trajectory(viz::adapt_states(history, options));
scene.playback().time = 0.0;
scene.playback().playing = true;
scene.playback().speed = 4.0f;  // faster evolution

viz::Camera camera;
viz::apply_cinematic_camera(camera);

viz::Framebuffer framebuffer(1280, 720);
viz::PresentationPipeline pipeline;
pipeline.render(scene, camera, framebuffer);

viz::write_ppm(framebuffer, "frame.ppm");
```



### Rules

- Only `physics/state/State.h` crosses the physics/visualization boundary.
- Trajectories are **immutable** after adaptation.
- `adapt_states` converts `(t,r,θ,φ)` → Cartesian `(x,y,z)` locally.
- Multiple trajectories: call `scene.add_trajectory(...)` repeatedly.
- CSV is optional: `viz::load_trajectory_csv(path)`.



### Key types


| Component                         | Role                                                   |
| --------------------------------- | ------------------------------------------------------ |
| `CPURasterizer`                   | Deterministic geometry + starfield rasterization       |
| `PresentationPipeline`            | Rasterizer + optional `PostProcessor`                  |
| `PostProcessor`                   | Lensing at projected `rs`, bloom, vignette, white ring |
| `project_schwarzschild_horizon()` | Shared screen projection for BH and lensing            |
| `cinematic_scene_settings()`      | Default scene flags                                    |
| `apply_cinematic_camera()`        | Default orbit camera                                   |


Module design details: `[visualization/README.md](../../visualization/README.md)`

---



## 12. Performance: faster playback and higher FPS

The interactive viewer is **CPU-bound** (software rasterization + post-process). There are two separate “speed” concerns:

### A. Faster orbit evolution (particle moves quicker)

The viewer advances playback as:

```text
playback.time += delta_time × playback.speed
```

**Ways to speed up evolution:**


| Method                     | How                                                        |
| -------------------------- | ---------------------------------------------------------- |
| `--speed` **flag**         | `./build/visualization_viewer --csv ... --speed 10`        |
| **Scrub with Right arrow** | Hold Right to advance quickly (pauses auto-play)           |
| **In your own code**       | Set `scene.playback().speed = 4.0f` before the render loop |


Example:

```bash
./build/visualization_viewer \
  --csv outputs/benchmark_data/<timestamp>/orbital.csv \
  --speed 20
```



### B. Higher frame rate (smoother interaction)


| Method               | Effect                                            |
| -------------------- | ------------------------------------------------- |
| `--classic`          | Disables post-processing (~2–3× faster)           |
| **Lower resolution** | `--width 960 --height 540` or smaller             |
| **Release build**    | `cmake --build build --config Release`            |
| **Measure first**    | `./build/visualization_benchmark` prints ms/frame |


Typical 1280×720 timings (order of magnitude, machine-dependent):


| Mode                          | Approx. FPS   |
| ----------------------------- | ------------- |
| Presentation (lensing on)     | ~1–4 fps      |
| `--classic` (no post-process) | ~5–8 fps      |
| 960×540 + `--classic`         | often >10 fps |


The viewer prints live FPS in the terminal every second.

---



## 13. Architecture summary

```text
Scientific Engine                    CPU Visualization (downstream)
─────────────────                    ─────────────────────────────
TrajectorySolver::solve()
        │
        ▼
std::vector<State>  ──►  adapt_states()  ──►  Scene  ──►  CPURasterizer
        │                                                      │
        └── (optional CSV)                              PostProcessor
                                                               │
                                                               ▼
                                                    PPM export / Viewer blit
```

Lensing is **non-physical** screen-space warp centered on the **projected Schwarzschild radius**, not a numerical GR ray trace.

---



## 14. Troubleshooting


| Problem                                  | What to try                                                                              |
| ---------------------------------------- | ---------------------------------------------------------------------------------------- |
| `cmake: command not found`               | Install CMake 3.22+                                                                      |
| vcpkg / Eigen not found                  | Pass `-DCMAKE_TOOLCHAIN_FILE=.../vcpkg.cmake`                                            |
| `benchmark_test` CSV missing             | Run `./build/benchmark_test` first                                                       |
| Viewer: `Failed to create viewer window` | No display available; use SSH with X forwarding or run `visualization_export` headless   |
| Viewer builds disabled                   | Reconfigure with `-DPENROSE_BUILD_VIEWER=ON`                                             |
| Black screen in viewer                   | Wait for first frame; check terminal for errors                                          |
| Lensing not centered on BH               | Should be fixed via shared `HorizonProjection`; rebuild `visualization`                  |
| PPM won’t open                           | Use `display frame.ppm` (ImageMagick), GIMP, or `ppm_to_video.py`                        |
| Python module not found                  | Run from repo root; use `python -m visualization.scientific....`                         |
| Very slow viewer                         | See [§12 Performance](#12-performance-faster-playback-and-higher-fps)                    |
| `visualization_test` fails               | Rebuild: `cmake --build build --target visualization_test && ./build/visualization_test` |


---



## Quick reference — end-to-end demo

```bash
# 1. Configure once
cmake -B build -S .

# 2. Build tools
cmake --build build --target benchmark_test visualization_viewer visualization_export

# 3. Generate data
./build/benchmark_test

# 4. Find latest CSV folder
BENCH=$(ls -td outputs/benchmark_data/*/ | head -1)
echo "Using $BENCH"

# 5. Interactive viewer (fast evolution)
./build/visualization_viewer --csv "${BENCH}orbital.csv" --speed 8

# 6. Or export a still
./build/visualization_export --csv "${BENCH}orbital.csv"

# 7. Scientific figures
python -m visualization.scientific.plot_benchmarks
```

For GPU ray tracing, see `[docs/RUNNING.md](../RUNNING.md)`.