# Penrose Visualization — User Guide

CPU visualization exposes **three** production executables. Edit a `main.cpp`, build, run. No CSV paths or CLI flags for viewer/export.

Architecture overview: [`ARCHITECTURE.md`](ARCHITECTURE.md).

---

## Workflows

| Executable | Config | Purpose |
|------------|--------|---------|
| `physics_benchmark` | [`examples/benchmark/main.cpp`](../examples/benchmark/main.cpp) | Validation → `outputs/benchmark_data/<timestamp>/` |
| `visualization_viewer` | [`examples/viewer/main.cpp`](../examples/viewer/main.cpp) | Integrate in memory → interactive viewer |
| `visualization_export` | [`examples/export/main.cpp`](../examples/export/main.cpp) | Integrate in memory → PPM still/sequence |

```text
Open examples/<workflow>/main.cpp
        ↓
Edit SimulationConfig + metric params + ICs (and render settings)
        ↓
cmake --build build --target <executable>
        ↓
./build/<executable>
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

viz::Scene scene = viz::prepare_scene(trajectories, viz);
viz::Camera camera = viz::make_camera(scene, viz.camera);
```

Do **not** put metric-specific fields on `SimulationConfig`. Physics and visualization resolutions are separate. Multiple particles are simulated independently and overlaid only in `prepare_scene`. See [`ARCHITECTURE.md`](ARCHITECTURE.md) §2–§5.

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
python -m visualization.scientific.analyze_benchmarks
python -m visualization.scientific.plot_benchmarks
python -m visualization.scientific.generate_report
```

---

## 2. Interactive viewer

```bash
cmake --build build --target visualization_viewer
./build/visualization_viewer
```

| Input | Action |
|-------|--------|
| Left drag | Orbit |
| Middle drag | Pan |
| Scroll | Zoom |
| Space | Play / pause |
| Left / Right | Scrub |
| Escape | Quit |

Raise `playback_speed` or set `presentation.enabled = false` in `examples/viewer/main.cpp` for responsiveness.

---

## 3. Export

```bash
cmake --build build --target visualization_export
./build/visualization_export
```

`frame_count = 1` → still; `>1` → `frame_XXXXXX.ppm` under `outputs/rendered_frames/<timestamp>/`.

GPU PPM→video helpers: [`frame_capture/`](frame_capture/).

---

## Troubleshooting

| Problem | Fix |
|---------|-----|
| No display for viewer | Use export, or `-DPENROSE_BUILD_VIEWER=OFF` |
| Want freefall / null in viewer | Change `scenario` + IC type in `examples/viewer/main.cpp` |
| Benchmark CSVs missing | Run `./build/physics_benchmark` |
