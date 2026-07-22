# Penrose CPU Visualization

Standalone CPU visualization for Penrose trajectories. Independent of the GPU raymarcher in `realtime/`.

Physics-agnostic rendering: this library never constructs metrics or solvers and does not include physics headers. Entry points run simulations via `penrose_physics`, convert results in `run/adapter/`, then call `prepare_scene`.

## Pipeline

```text
SimulationRequest(s)  →  run_all  →  SimulationResult
                                          ↓
                          store_trajectories (run/adapter)
                                          ↓
                               prepare_scene (Stage 2)
                                          ↓
                               Scene + Camera → render (Stage 3)
```

Stage 2 is currently a pass-through. `VisualizationPreparationSettings` reserves knobs for future interpolation / resampling without touching the physics engine.

## User-facing workflows

| Executable | Config | Command |
|------------|--------|---------|
| `physics_benchmark` | [`run/benchmark/main.cpp`](../run/benchmark/main.cpp) | `./build/physics_benchmark` |
| `visualization_viewer` | [`run/viewer/main.cpp`](../run/viewer/main.cpp) | `./build/visualization_viewer` |
| `visualization_export` | [`run/export/main.cpp`](../run/export/main.cpp) | `./build/visualization_export` |

Full guide: [`docs/VISUALIZATION_GUIDE.md`](../docs/VISUALIZATION_GUIDE.md) · Architecture: [`docs/ARCHITECTURE.md`](../docs/ARCHITECTURE.md)

## Module layout

```text
visualization/
├── Preparation/    StoredTrajectory + prepare_scene (Stage 2)
├── Trajectory/     Chart-to-Cartesian adapt_states
├── Scene/          Scene graph, playback (multi-trajectory)
├── Camera/         Orbit / pan / zoom
├── Geometry/       Math, spherical→Cartesian, meshes
├── Renderer/       Headless CPU rasterizer (Stage 3)
├── Presentation/   VisualizationConfig + optional post-process
├── IO/             PPM export, starfield (visualization/resources/)
├── Apps/           ViewerApp + DisplayBlit (penrose_glad)
├── scientific/     Compatibility shims → physics/analysis/
└── Tests/          Unit tests (`-DPENROSE_BUILD_TESTS=ON`)

run/adapter/        SimulationResult → StoredTrajectory bridge (linked by viewer/export)
```

### In-memory API (entry points)

```cpp
std::vector<Simulation::SimulationRequest> simulations = {orbit1, orbit2, orbit3};
auto trajectories = Simulation::run_all(simulations);

viz::VisualizationConfig viz;
// set viz.preparation / viz.scene / viz.camera / styles / ...

viz::Scene scene = viz::prepare_scene_from_results(trajectories, viz);
viz::Camera camera = viz::make_camera(scene, viz.camera);
viz::run_interactive_viewer(std::move(scene), camera, viz);
```

Per-particle styles: pass a parallel `std::span<const TrajectoryStyle>` to `prepare_scene_from_results`, or set `StoredTrajectory::use_style`.

Rules:

- Only `StoredTrajectory` crosses into the visualization library.
- Drawing / viz-resolution parameters come from `VisualizationConfig` in `run/*/main.cpp`.
- Trajectories are immutable after adaptation.
- Viewer and export entry points never load CSV.
- Benchmark analysis lives in `physics/analysis/` (not presentation).
