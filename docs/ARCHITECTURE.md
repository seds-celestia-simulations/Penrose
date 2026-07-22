# Penrose Architecture

This is the **sole current architecture reference** for Penrose.

Penrose is a modular General Relativity framework: a CPU reference geodesic solver, a CPU trajectory visualization stack, and a GPU real-time renderer. The design goal is to add new spacetimes (Kerr, SGL, FLRW, …) by extending physics modules—not by rewriting entry points or a catch-all config struct.

---

## 1. Pipelines

```text
                    shared/  (State, MetricKind, parameter vocabulary, units)
                           │
           ┌───────────────┼───────────────┐
           ▼               ▼               ▼
   penrose_physics/  visualization/     realtime/
     (CPU science)   (CPU presentation) (GPU interactive)
```

| Pipeline | Role | Truth for |
|----------|------|-----------|
| **physics/** | Analytic metrics, dynamics, RK4, trajectory solve, validation | Scientific correctness |
| **visualization/** | Stored trajectories → preparation → scene → CPU raster → optional post-process | Publication / orbit illustrations |
| **realtime/** | OpenGL + GLSL null geodesic ray march | Interactive lensing imagery |

Pipelines are intentionally loosely coupled. Visualization consumes stored `State` histories (never constructs metrics or solvers). Realtime does **not** yet share the CPU metric implementation; it remains a separate shader-resident path.

---

## 2. User-facing CPU workflow

Three production executables. Each is configured by editing a small `main.cpp` under `run/`, then building and running—no CLI flags required for viewer/export.

| Executable | Config | Role |
|------------|--------|------|
| `physics_benchmark` | [`run/benchmark/main.cpp`](../run/benchmark/main.cpp) | Validation suite → CSV under `outputs/benchmark_data/` |
| `visualization_viewer` | [`run/viewer/main.cpp`](../run/viewer/main.cpp) | In-memory integrate → interactive viewer |
| `visualization_export` | [`run/export/main.cpp`](../run/export/main.cpp) | In-memory integrate → PPM still/sequence |

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
Viewer / Export               // Stage 3: rendering of prepared geometry
```

Entry points never construct concrete metrics (`SchwarzschildMetric`, …). Metrics are created inside the simulation pipeline. Multiple particles are simulated independently and overlaid only in visualization preparation.

GPU app: `./build/Penrose` — see [`RUNNING.md`](RUNNING.md).

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

## 5. CPU visualization stack

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
CPURasterizer / PostProcessor // Stage 3: prepared geometry only
        ↓
Viewer blit / PPM
```

| Module | Responsibility |
|--------|----------------|
| `visualization/Preparation/` | `StoredTrajectory` (with `CoordinateChartKind`), `prepare_scene` |
| `visualization/Trajectory/` | Chart-to-Cartesian `adapt_states` |
| `visualization/Scene/` | Multi-trajectory scene graph + playback |
| `visualization/Camera/` | Orbit / pan / zoom |
| `visualization/Renderer/` | CPU rasterizer + framebuffer |
| `visualization/Presentation/` | `VisualizationConfig`, bloom / cosmetic lensing |
| `visualization/Apps/` | `ViewerApp`, `DisplayBlit` (links neutral `penrose_glad`, not `realtime/`) |
| `visualization/IO/` | PPM writer, output paths, starfield from `visualization/resources/` |
| `run/adapter/` | `store_trajectories`, `prepare_scene_from_results` (physics→viz bridge) |

The renderer never cares which integrator, spacetime, or step count produced a curve. Multiple independently simulated trajectories are overlaid into one `Scene`.

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
Engine → Window / Camera → Renderer → GLSL fragment ray march → framebuffer
                ↘ FrameCapture (P key) → imagesequence/<timestamp>/
```

Owned under `realtime/`. Independent build target `Penrose`.

| Area | Location |
|------|----------|
| Engine / window / capture | `realtime/core/` |
| Renderer | `realtime/render/` |
| Camera / particles | `realtime/scene/` |
| Spacetime FX helpers | `realtime/spacetime/` |
| Shaders / resources | `realtime/shaders/`, `realtime/resources/` |
| PPM → video script | `realtime/visualization/ppm_to_video.py` |

Docs: [`frame_capture/`](frame_capture/).

---

## 7. Repository map

```text
penrose/
├── run/
│   ├── benchmark/main.cpp      → physics_benchmark
│   ├── viewer/main.cpp         → visualization_viewer
│   ├── export/main.cpp         → visualization_export
│   └── adapter/                → SimulationResult → StoredTrajectory bridge
├── physics/                    # CPU reference solver (+ analysis Python)
├── visualization/              # CPU presentation renderer only
├── realtime/                   # GPU interactive renderer (unchanged)
├── shared/                     # GR vocabulary (State, MetricKind, parameters)
├── vendor/glad/                # Neutral OpenGL loader for CPU viewer
├── outputs/                    # Generated CSVs, figures, PPMs, notebooks
├── docs/
│   ├── ARCHITECTURE.md         # this file (current)
│   ├── RUNNING.md              # install / run all pipelines
│   ├── VISUALIZATION_GUIDE.md  # CPU three-executable UX
│   ├── frame_capture/          # GPU capture helpers
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
7. **CPU solver is the reference.** GPU shader math is for interaction until shared abstractions catch up.
8. **Preserve validated math.** Architecture changes extract modules; they do not silently rewrite Christoffel formulas.
9. **Ownership boundaries:** `shared/` = vocabulary; `physics/` (`penrose_physics`) = CPU science; `visualization/` = presentation; `physics/analysis/` = benchmark analysis consumers; `realtime/` = GPU engine (independent).

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
| [`VISUALIZATION_GUIDE.md`](VISUALIZATION_GUIDE.md) | **Current** — CPU UX |
| [`frame_capture/`](frame_capture/) | **Current** — GPU capture |
| [`../visualization/README.md`](../visualization/README.md) | **Current** — CPU viz module |
| [`reports/Penrose_from_First_Principles.md`](reports/Penrose_from_First_Principles.md) | Science walkthrough (layout assumptions may be dated) |
| [`reviews/`](reviews/) | **Historical** reviews (not normative) |
