# Penrose CPU Visualization

Standalone CPU visualization for Penrose trajectories. Independent of the GPU raymarcher in `realtime/`.

Physics-agnostic rendering: this library never constructs metrics or solvers. Entry points run independent simulations, store trajectories, prepare renderable geometry, then draw.

## Pipeline

```text
SimulationRequest(s)  →  run_all  →  PhysicsTrajectory storage
                                          ↓
                               prepare_scene (Stage 2)
                                          ↓
                               Scene + Camera → render (Stage 3)
```

Stage 2 is currently a pass-through. `VisualizationPreparationSettings` reserves knobs for future interpolation / resampling without touching the physics engine.

## User-facing workflows

| Executable | Config | Command |
|------------|--------|---------|
| `physics_benchmark` | [`examples/benchmark/main.cpp`](../examples/benchmark/main.cpp) | `./build/physics_benchmark` |
| `visualization_viewer` | [`examples/viewer/main.cpp`](../examples/viewer/main.cpp) | `./build/visualization_viewer` |
| `visualization_export` | [`examples/export/main.cpp`](../examples/export/main.cpp) | `./build/visualization_export` |

Full guide: [`docs/VISUALIZATION_GUIDE.md`](../docs/VISUALIZATION_GUIDE.md) · Architecture: [`docs/ARCHITECTURE.md`](../docs/ARCHITECTURE.md)

## Module layout

```text
visualization/
├── Preparation/    StoredTrajectory + prepare_scene (Stage 2)
├── Trajectory/     Immutable samples + State adapters
├── Scene/          Scene graph, playback (multi-trajectory)
├── Camera/         Orbit / pan / zoom
├── Geometry/       Math, spherical→Cartesian, meshes
├── Renderer/       Headless CPU rasterizer (Stage 3)
├── Presentation/   VisualizationConfig + optional post-process
├── IO/             PPM export, output paths; CSV loader (analysis only)
├── Apps/           ViewerApp + DisplayBlit (internal)
└── Tests/          Unit tests (`-DPENROSE_BUILD_TESTS=ON`)
```

### In-memory API

```cpp
std::vector<Simulation::SimulationRequest> simulations = {orbit1, orbit2, orbit3};
auto trajectories = Simulation::run_all(simulations);

viz::VisualizationConfig viz;
// set viz.preparation / viz.scene / viz.camera / styles / ...

viz::Scene scene = viz::prepare_scene(trajectories, viz);
viz::Camera camera = viz::make_camera(scene, viz.camera);
viz::run_interactive_viewer(std::move(scene), camera, viz);
```

Per-particle styles: pass a parallel `std::span<const TrajectoryStyle>` to `prepare_scene`, or set `StoredTrajectory::use_style`.

Rules:

- Only stored trajectories cross into visualization preparation.
- Drawing / viz-resolution parameters come from `VisualizationConfig` in `main.cpp`.
- Trajectories are immutable after adaptation.
- Viewer and export entry points never load CSV.
