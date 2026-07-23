# Penrose Architecture

This is the **sole current architecture reference** for Penrose.

Penrose is a modular General Relativity framework: a CPU reference geodesic solver, a trajectory visualization stack (GPU interactive viewer + headless CPU export), and a separate GPU real-time ray-march renderer. The design goal is to add new spacetimes (Kerr, SGL, FLRW, …) by extending physics modules—not by rewriting entry points or a catch-all config struct.

---

## 1. Pipelines

```text
                    shared/  (State, MetricKind, parameter vocabulary, units)
                           │
           ┌───────────────┼───────────────┐
           ▼               ▼               ▼
   penrose_physics/  visualization/     realtime/
     (CPU science)   (trajectory viz)   (GPU ray march)
```

| Pipeline | Role | Truth for |
|----------|------|-----------|
| **physics/** | Analytic metrics, dynamics, RK4, trajectory solve, validation | Scientific correctness |
| **visualization/** | Stored trajectories → preparation → scene → Stage 3 render backends | Publication / orbit illustrations |
| **realtime/** | OpenGL + GLSL null geodesic ray march | Interactive lensing imagery |

Pipelines are intentionally loosely coupled. Visualization consumes stored `State` histories (never constructs metrics or solvers). Realtime does **not** share the CPU metric implementation; it remains a separate shader-resident path. Trajectory-viz GPU code (`visualization_gpu`) also does **not** include or link `realtime/`.

---

## 2. User-facing CPU workflow

Three production executables. Each is configured by editing a small `main.cpp` under `run/`, then building and running—no CLI flags required for viewer/export.

| Executable | Config | Role |
|------------|--------|------|
| `physics_benchmark` | [`run/benchmark/main.cpp`](../run/benchmark/main.cpp) | Validation suite → CSV under `outputs/benchmark_data/` |
| `visualization_viewer` | [`run/viewer/main.cpp`](../run/viewer/main.cpp) | In-memory integrate → GPU polyline viewer |
| `visualization_export` | [`run/export/main.cpp`](../run/export/main.cpp) | In-memory integrate → CPU PPM still/sequence |

```text
SimulationRequest(s)          // one per independent particle
        ↓
run_all / run_simulation      // Stage 1: physics accuracy only
        ↓
PhysicsTrajectory storage     // SimulationResult histories
        ↓
store_trajectories (run/adapter)  // consumer bridge — not in visualization lib
        ↓
prepare_scene                 // Stage 2: StoredTrajectory → Scene geometry
        ↓
Viewer / Export               // Stage 3: TrajectoryRenderBackend (GPU viewer / CPU export)
```

Entry points never construct concrete metrics (`SchwarzschildMetric`, …). Metrics are created inside the simulation pipeline. Multiple particles are simulated independently and overlaid only in visualization preparation.

GPU ray-march app: `./build/Penrose` — see [`RUNNING.md`](RUNNING.md).

---

## 3. Configuration layers

### Layer 1 — physics settings (`SimulationConfig`)

Spacetime-agnostic controls for **numerical correctness only**:

- `spacetime`, `scenario`, `geodesic`
- `dt`, `max_steps`, `horizon_safety_factor`
- `name`

Visualization resolution does **not** live here.

### Layer 2 — physics parameters

Metric- and scenario-specific numbers live in dedicated types:

| Kind | Example | Location |
|------|---------|----------|
| Metric params | `SchwarzschildParameters` | `shared/metrics/` (POD vocabulary) |
| Initial conditions | `BoundOrbitInitialConditions`, … | `physics/simulation/initial_conditions/` |

Bundle them per particle as a `SimulationRequest` (`config` + `metric` + `initial` variant):

```cpp
std::vector<Simulation::SimulationRequest> simulations = {orbit1, orbit2};
auto trajectories = Simulation::run_all(simulations);
auto stored = viz::store_trajectories(trajectories);
auto scene = viz::prepare_scene(stored, viz);
// Or: viz::prepare_scene_from_results(trajectories, viz) in run/viewer and run/export
```

Adding Kerr should introduce `KerrParameters` and a new `run_simulation` overload—not new fields on `SimulationConfig`.

### Layer 3 — visualization preparation + drawing

| Concern | Type | Notes |
|---------|------|-------|
| Viz resolution | `VisualizationPreparationSettings` | `interpolation_method`, `render_samples_per_segment`, `trajectory_resolution` (pass-through today) |
| Drawing | `VisualizationConfig` | scene, camera, presentation, styles, render options |

Physics `dt` and visualization `trajectory_resolution` are independent concepts.

---

## 4. CPU scientific stack

```text
State { X, U }
    → Metric::christoffel(...)
    → GeodesicDynamics::compute_derivative(...)
    → Integrator::step(...)   // RK4 default
    → TrajectorySolver::solve(...)   // + TerminationPolicy
    → PhysicsTrajectory (SimulationResult + metadata)
```

Built as the `penrose_physics` static library (`CMakeLists.txt`). CPU consumers link it instead of duplicating source lists.

| Module | Responsibility |
|--------|----------------|
| `shared/state/GeodesicState.h` | `State` |
| `shared/spacetime/MetricKind.h` | `MetricKind`, `CoordinateChartKind` |
| `shared/metrics/SchwarzschildParameters.h` | Schwarzschild POD parameters |
| `shared/spacetime/Metric.h` | Narrow Christoffel evaluator interface (CPU today) |
| `physics/metrics/` | Concrete metrics + `CoordinateChart` transforms |
| `physics/geodesics/` | `DynamicsModel`, `GeodesicDynamics` |
| `physics/integrators/` | `Integrator` interface, RK4 default |
| `physics/simulation/` | `TrajectorySolver`, `TerminationPolicy`, `SimulationConfig`, `SimulationRequest`, `SimulationPipeline`, IC builders |
| `physics/validation/` | Benchmark drivers (consumers of `run_simulation`) + `BenchmarkRunner` |
| `physics/validation/observables/` | Reusable invariant / observable helpers |
| `physics/analysis/` | Python benchmark analysis / figures / reports |
| `physics/export/` | Benchmark CSV I/O helpers |

`SimulationPipeline` resolves metric + ICs from Layer 2 types and calls the solver. Validation drivers build `SimulationRequest` objects and call `run_simulation` — they no longer construct metrics/solvers directly. `run_all` integrates each `SimulationRequest` independently (no coupled multi-body integration).

---

## 5. Trajectory visualization stack

Three stages after physics:

```text
PhysicsTrajectory(s)          // storage — no render knobs
        ↓
store_trajectories (run/adapter)   // SimulationResult → StoredTrajectory + chart metadata
        ↓
prepare_scene(...)            // Stage 2: StoredTrajectory → Scene geometry
        ↓                     //   (interpolation / resampling hooks reserved)
make_camera(...)
        ↓
TrajectoryRenderBackend       // Stage 3: prepared geometry only
  ├── GpuPolylineBackend      // interactive viewer (OpenGL; visualization_gpu)
  └── CpuRasterizerBackend    // headless export (CPURasterizer + PostProcessor)
        ↓
GLFW window / PPM
```

| Module | Responsibility |
|--------|----------------|
| `visualization/Preparation/` | `StoredTrajectory` (with `CoordinateChartKind`), `prepare_scene` |
| `visualization/Trajectory/` | Chart-to-Cartesian `adapt_states` |
| `visualization/Scene/` | Multi-trajectory scene graph + playback |
| `visualization/Camera/` | Orbit / pan / zoom |
| `visualization/Renderer/` | `TrajectoryRenderBackend`, CPU rasterizer, `Gpu/` polyline backend |
| `visualization/Presentation/` | `VisualizationConfig`, bloom / cosmetic lensing (CPU export path) |
| `visualization/Apps/` | `ViewerApp` (GPU backend), `DisplayBlit` GLFW/GLAD window helper |
| `visualization/IO/` | PPM writer, output paths, starfield from `visualization/resources/` |
| `run/adapter/` | `store_trajectories`, `prepare_scene_from_results` (physics→viz bridge) |

The renderer never cares which integrator, spacetime, or step count produced a curve. Multiple independently simulated trajectories are overlaid into one `Scene`.

**Stage 3 backends:**

| Backend | Library / target | Used by | Output |
|---------|------------------|---------|--------|
| `GpuPolylineBackend` | `visualization_gpu` (viewer only) | `visualization_viewer` | OpenGL draw into GLFW framebuffer |
| `CpuRasterizerBackend` | `visualization` | `visualization_export` | `Framebuffer` → PPM (no OpenGL) |

GPU trajectory shaders and buffers live under `visualization/Renderer/Gpu/` (embedded sources). They are not shared with `realtime/`. Interactive viewer v1 draws starfield, opaque horizon disc + glow ring, solid trails with distance fall-off, and markers. CPU export retains optional `PostProcessor` bloom / cosmetic lensing; GPU bloom parity is deferred.

`auto_frame` distances the camera from scene extent and uses a tilted yaw/pitch (not edge-on to equatorial orbits). Playback scrub in the viewer advances at a time-based rate (~8% of duration per second while Left/Right are held).

Scene horizon radius is geometric (not a metric type):

```text
horizon_radius = max(VisualizationConfig.scene.horizon_radius,
                     max(horizon_radius / characteristic_radius over stored trajectories))
```

All drawing parameters are set in `run/*/main.cpp` via `VisualizationConfig`. Viewer/export **do not load CSV**; benchmark CSVs feed scientific analysis only.

Internal unit tests: `-DPENROSE_BUILD_TESTS=ON` (default **OFF**).

---

## 6. GPU realtime stack

```text
Engine → ShaderManager → RenderPass pipeline → GLSL fragment ray march → framebuffer
                ↘ FrameCapture (P key) → imagesequence/<timestamp>/
```

Owned under `realtime/`. Independent build target `Penrose`.

| Area | Location |
|------|----------|
| Engine / window / capture | `realtime/core/` |
| ShaderManager (metric loading, caching, switching) | `realtime/core/ShaderManager.h/.cpp` |
| RenderPass pipeline (GeodesicPass, UpscalePass) | `realtime/render/` |
| Renderer (fullscreen quad, particle SSBO) | `realtime/render/Renderer.h/.cpp` |
| Camera / particles / ParticleSystem interface | `realtime/scene/` |
| Spacetime FX helpers (LutBaker, AccretionDisk) | `realtime/spacetime/` |
| Shaders / resources | `realtime/shaders/`, `realtime/resources/` |
| PPM → video script | `realtime/visualization/ppm_to_video.py` |

**RenderPass pipeline:** Engine holds `vector<unique_ptr<RenderPass>>`. Each pass receives a `PassContext` (camera, time, dimensions, textures) and executes independently. `GeodesicPass` handles the fullscreen quad draw with the active metric shader. `UpscalePass` is a placeholder for future foveated rendering.

**ShaderManager:** Manages metric shader registration, lazy compilation, caching, and runtime switching. Metrics are registered via `loadMetric(type, vertPath, fragPath)` and switched via `setMetric(type)`.

**ParticleSystem:** Polymorphic interface implemented by `FallingParticleSystem` (set `rs` via `setRs()`) and `AccretionDisk` (returns `Particle` directly). Engine iterates all systems to merge particles into a single SSBO.

Docs: [`frame_capture/`](frame_capture/).

---

## 7. Repository map

```text
penrose/
├── run/
│   ├── benchmark/main.cpp      → physics_benchmark
│   ├── viewer/main.cpp         → visualization_viewer (GPU Stage 3)
│   ├── export/main.cpp         → visualization_export (CPU Stage 3)
│   └── adapter/                → SimulationResult → StoredTrajectory bridge
├── physics/                    # CPU reference solver (+ analysis Python)
├── visualization/              # Trajectory presentation (CPU export + GPU viewer)
├── realtime/                   # GPU ray-march product (independent)
├── shared/                     # GR vocabulary (State, MetricKind, parameters)
├── vendor/glad/                # Neutral OpenGL loader for trajectory viewer
├── outputs/                    # Generated CSVs, figures, PPMs, notebooks
├── docs/
│   ├── ARCHITECTURE.md         # this file (current)
│   ├── RUNNING.md              # install / run all pipelines
│   ├── VISUALIZATION_GUIDE.md  # trajectory viz three-executable UX
│   ├── frame_capture/          # GPU ray-march capture helpers
│   ├── reviews/                # historical architecture reviews
│   └── reports/                # long-form science notes (historical layout)
├── notes/                      # working notes / screenshots
└── CMakeLists.txt
```

---

## 8. Design rules

1. **Entry points configure; pipelines implement.** No concrete `*Metric` in `run/*/main.cpp`.
2. **Do not grow `SimulationConfig` into a spacetime kitchen sink.** New physics → new parameter structs + overloads.
3. **Physics resolution ≠ visualization resolution.** `dt` / integrator settings stay in physics; resampling / interpolation knobs stay in `VisualizationPreparationSettings`.
4. **Particles do not couple.** Independent `SimulationRequest`s; overlay only in `prepare_scene`.
5. **Visualization never mutates physics state.** Trajectories are immutable after adaptation. The visualization library does not include physics headers.
6. **Presentation ≠ GR.** Screen-space lensing / bloom are cosmetic.
7. **CPU solver is the reference.** Realtime ray-march and trajectory-viz GPU drawing are for interaction / presentation until shared abstractions catch up.
8. **Preserve validated math.** Architecture changes extract modules; they do not silently rewrite Christoffel formulas.
9. **Ownership boundaries:** `shared/` = vocabulary; `physics/` (`penrose_physics`) = CPU science; `visualization/` (+ `visualization_gpu` for the viewer) = presentation; `physics/analysis/` = benchmark analysis; `realtime/` = ray-march engine (independent).
10. **OpenGL stays out of headless export.** Only `visualization_viewer` links `visualization_gpu` / `penrose_glad` / GLFW for trajectory viz.

---

## 9. Extensibility sketch (future Kerr)

1. Implement `Spacetime::KerrMetric : Metric`.
2. Add `KerrParameters { mass, spin }`.
3. Add `run_simulation(config, KerrParameters, BoundOrbitInitialConditions)` (and IC recipes as needed).
4. In `run/*/main.cpp`, set `config.spacetime = Kerr` and construct `KerrParameters` on each `SimulationRequest`.

No structural change to viewer, export, or benchmark executables.

---

## 10. Related documentation

| Document | Status |
|----------|--------|
| [`ARCHITECTURE.md`](ARCHITECTURE.md) | **Current** — this file |
| [`RUNNING.md`](RUNNING.md) | **Current** — build / run all pipelines |
| [`VISUALIZATION_GUIDE.md`](VISUALIZATION_GUIDE.md) | **Current** — trajectory viz UX |
| [`frame_capture/`](frame_capture/) | **Current** — GPU ray-march capture |
| [`../visualization/README.md`](../visualization/README.md) | **Current** — visualization module |
| [`reports/Penrose_from_First_Principles.md`](reports/Penrose_from_First_Principles.md) | Science walkthrough (layout assumptions may be dated) |
| [`reviews/`](reviews/) | **Historical** reviews (not normative) |
