# Penrose Visualization — User Guide

CPU visualization exposes **three** production executables. Edit a `main.cpp` under `run/`, build, run. No CSV paths or CLI flags for viewer/export.

Architecture overview: [`ARCHITECTURE.md`](ARCHITECTURE.md).

---

## Workflows

| Executable | Config | Purpose |
|------------|--------|---------|
| `physics_benchmark` | [`run/benchmark/main.cpp`](../run/benchmark/main.cpp) | Validation → `outputs/benchmark_data/<timestamp>/` |
| `visualization_viewer` | [`run/viewer/main.cpp`](../run/viewer/main.cpp) | Integrate in memory → interactive viewer |
| `visualization_export` | [`run/export/main.cpp`](../run/export/main.cpp) | Integrate in memory → PPM still/sequence |

```text
Open run/<workflow>/main.cpp
        ↓
Edit SimulationRequest(s) + VisualizationConfig
        ↓
cmake --build build --target <executable>
        ↓
./build/<executable>
```

### Pipeline stages

```text
Physics (SimulationRequest per particle)
        ↓
Trajectory storage (SimulationResult)
        ↓
Visualization preparation (prepare_scene — pass-through today)
        ↓
Rendering (viewer / PPM)
```

### Three config layers

```cpp
// Layer 1+2 — one SimulationRequest per independent particle
Simulation::SimulationRequest orbit;
orbit.config.spacetime = Simulation::SpacetimeKind::Schwarzschild;
orbit.config.scenario  = Simulation::Scenario::BoundOrbit;
orbit.config.dt = 0.01;          // physics resolution
orbit.metric.mass = 1.0;
orbit.initial = Simulation::BoundOrbitInitialConditions{ .r0 = 6.0, .vphi = 0.06 };

std::vector<Simulation::SimulationRequest> simulations = {orbit};
auto trajectories = Simulation::run_all(simulations);

// Layer 3 — visualization preparation + drawing (independent of physics dt)
viz::VisualizationConfig viz;
viz.preparation.interpolation_method = viz::InterpolationMethod::PassThrough;
viz.preparation.trajectory_resolution = 1.0f; // reserved for future resampling
// ... camera, presentation, trajectory_style, render ...

viz::Scene scene = viz::prepare_scene_from_results(trajectories, viz);
viz::Camera camera = viz::make_camera(scene, viz.camera);
```

Do **not** put metric-specific fields on `SimulationConfig`. Physics and visualization resolutions are separate. Multiple particles are simulated independently and overlaid only in `prepare_scene`. The visualization library accepts `StoredTrajectory` only — conversion from `SimulationResult` lives in `run/adapter/`. See [`ARCHITECTURE.md`](ARCHITECTURE.md) §2–§5.

Per-particle styles: pass a parallel `std::span<const TrajectoryStyle>` to `prepare_scene_from_results`, or set `StoredTrajectory::use_style`.

---

## Build once

```bash
cmake -B build -S . -DCMAKE_TOOLCHAIN_FILE=$HOME/vcpkg/scripts/buildsystems/vcpkg.cmake
cmake --build build --target physics_benchmark visualization_viewer visualization_export
```

| Option | Default | Effect |
|--------|---------|--------|
| `PENROSE_BUILD_VIEWER` | `ON` | Build interactive viewer |
| `PENROSE_BUILD_TESTS` | `OFF` | Internal unit tests |

---

## 1. Physics benchmark

```bash
cmake --build build --target physics_benchmark
./build/physics_benchmark
```

Writes CSVs under `outputs/benchmark_data/<timestamp>/`. Optional analysis:

```bash
python -m physics.analysis.analyze_benchmarks
python -m physics.analysis.plot_benchmarks
python -m physics.analysis.generate_report
```

(`python -m visualization.scientific.*` remains as a compatibility shim.)

---

## 2. Interactive viewer

```bash
cmake --build build --target visualization_viewer
./build/visualization_viewer
```

`run/viewer/main.cpp` currently demonstrates **multiple independent null geodesics** overlaid in one scene.

| Input | Action |
|-------|--------|
| Left drag | Orbit |
| Middle drag | Pan |
| Scroll | Zoom |
| Space | Play / pause |
| Left / Right | Scrub |
| 1–9 | Select trajectory highlight |
| Escape | Quit |

Raise `playback_speed` or set `presentation.enabled = false` in `run/viewer/main.cpp` for responsiveness.

---

## 3. Export

```bash
cmake --build build --target visualization_export
./build/visualization_export
```

`frame_count = 1` → still; `>1` → `frame_XXXXXX.ppm` under `outputs/rendered_frames/<timestamp>/`.

GPU capture PPMs use a different tree (`imagesequence/`). Helpers: [`frame_capture/`](frame_capture/).

---

## Troubleshooting

| Problem | Fix |
|---------|-----|
| No display for viewer | Use export, or `-DPENROSE_BUILD_VIEWER=OFF` |
| Want freefall / bound orbit | Change `scenario` + IC type on each `SimulationRequest` in `run/viewer/main.cpp` |
| Add another particle | Push another `SimulationRequest` into the `simulations` vector |
| Benchmark CSVs missing | Run `./build/physics_benchmark` |
| Build fails looking for `examples/` | Entry points live under `run/` — reconfigure CMake from a current checkout |
