# Penrose Architecture

This is the **sole current architecture reference** for Penrose.

Penrose is a modular General Relativity framework: a CPU reference geodesic solver, a CPU trajectory visualization stack, and a GPU real-time renderer. The design goal is to add new spacetimes (Kerr, SGL, FLRW, …) by extending physics modules—not by rewriting entry points or a catch-all config struct.

---

## 1. Pipelines

```text
                    shared/  (State, Metric interface, units stubs)
                           │
           ┌───────────────┼───────────────┐
           ▼               ▼               ▼
       physics/      visualization/     realtime/
     (CPU science)   (CPU presentation) (GPU interactive)
```

| Pipeline | Role | Truth for |
|----------|------|-----------|
| **physics/** | Analytic metrics, dynamics, RK4, trajectory solve, validation | Scientific correctness |
| **visualization/** | Immutable trajectories → scene → CPU raster → optional post-process | Publication / orbit illustrations |
| **realtime/** | OpenGL + GLSL null geodesic ray march | Interactive lensing imagery |

Pipelines are intentionally loosely coupled. Visualization consumes `State` histories (or adapters). Realtime does **not** yet share the CPU metric implementation; it remains a separate shader-resident path.

---

## 2. User-facing CPU workflow

Three production executables. Each is configured by editing a small `main.cpp`, then building and running—no CLI flags required for viewer/export.

| Executable | Config | Role |
|------------|--------|------|
| `physics_benchmark` | [`examples/benchmark/main.cpp`](../examples/benchmark/main.cpp) | Validation suite → CSV under `outputs/benchmark_data/` |
| `visualization_viewer` | [`examples/viewer/main.cpp`](../examples/viewer/main.cpp) | In-memory integrate → interactive viewer |
| `visualization_export` | [`examples/export/main.cpp`](../examples/export/main.cpp) | In-memory integrate → PPM still/sequence |

```text
SimulationRequest(s)          // one per independent particle
        ↓
run_all / run_simulation      // physics accuracy only
        ↓
PhysicsTrajectory storage     // SimulationResult histories
        ↓
prepare_scene                 // visualization preparation (pass-through today)
        ↓
Viewer / Export               // rendering of prepared geometry
```

Entry points never construct concrete metrics (`SchwarzschildMetric`, …). Metrics are created inside the simulation pipeline. Multiple particles are simulated independently and overlaid only in visualization.

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
| Metric params | `SchwarzschildParameters` | `physics/metrics/parameters/` |
| Initial conditions | `BoundOrbitInitialConditions`, … | `physics/simulation/initial_conditions/` |

```cpp
std::vector<Simulation::SimulationRequest> simulations = {orbit1, orbit2};
auto trajectories = Simulation::run_all(simulations);
auto scene = viz::prepare_scene(trajectories, viz);
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
    → stepRK4(...)
    → TrajectorySolver::solve(...)   // + TerminationPolicy
    → PhysicsTrajectory (SimulationResult)
```

| Module | Responsibility |
|--------|----------------|
| `shared/state/GeodesicState.h` | `State` |
| `shared/spacetime/Metric.h` | Abstract metric interface |
| `physics/metrics/` | Concrete metrics + parameter structs + `CoordinateChart` |
| `physics/geodesics/` | `DynamicsModel`, `GeodesicDynamics` |
| `physics/integrators/` | RK4 |
| `physics/simulation/` | `TrajectorySolver`, `TerminationPolicy`, `SimulationConfig`, `SimulationRequest`, `SimulationPipeline` |
| `physics/validation/` | Freefall / orbital / null drivers + `BenchmarkRunner` |
| `physics/export/` | Benchmark CSV I/O helpers |

`SimulationPipeline` resolves metric + ICs from Layer 2 types and calls the existing solver. `run_all` integrates each `SimulationRequest` independently (no coupled multi-body integration). Validation drivers remain Schwarzschild-specific implementations behind `BenchmarkRunner`.

---

## 5. CPU visualization stack

Three stages after physics:

```text
PhysicsTrajectory(s)          // storage — no render knobs
        ↓
prepare_scene(...)            // Stage 2: StoredTrajectory → Scene geometry
        ↓                     //   (interpolation / resampling hooks reserved)
make_camera(...)
        ↓
CPURasterizer / PostProcessor // Stage 3: prepared geometry only
        ↓
Viewer blit / PPM
```

The renderer never cares which integrator, spacetime, or step count produced a curve. Multiple independently simulated trajectories are overlaid into one `Scene`.

`SceneSettings.horizon_radius` is a geometric absorbing radius from stored trajectories — not a concrete metric type.

All drawing parameters are set in `examples/*/main.cpp` via `VisualizationConfig`.

---

## 6. GPU realtime stack

```text
Window / Camera → Renderer → GLSL fragment ray march → framebuffer
```

Owned under `realtime/`. Independent build target `Penrose`. Frame capture utilities: `realtime/visualization/`, docs in [`frame_capture/`](frame_capture/).

---

## 7. Repository map

```text
penrose/
├── examples/
│   ├── benchmark/main.cpp      → physics_benchmark
│   ├── viewer/main.cpp         → visualization_viewer
│   └── export/main.cpp         → visualization_export
├── physics/                    # CPU reference solver
├── visualization/              # CPU presentation renderer
├── realtime/                   # GPU interactive renderer
├── shared/                     # State + Metric interface
├── outputs/                    # Generated CSVs, figures, PPMs
├── docs/
│   ├── ARCHITECTURE.md         # this file (current)
│   ├── RUNNING.md              # install / GPU run
│   ├── VISUALIZATION_GUIDE.md  # CPU three-executable UX
│   ├── frame_capture/          # GPU capture helpers
│   ├── reviews/                # architecture reviews
│   ├── legacy/                 # superseded plans & old docs
│   └── reports/                # long-form science notes
├── notes/                      # working notes / screenshots
└── CMakeLists.txt
```

---

## 8. Design rules

1. **Entry points configure; pipelines implement.** No concrete `*Metric` in `examples/*/main.cpp`.
2. **Do not grow `SimulationConfig` into a spacetime kitchen sink.** New physics → new parameter structs + overloads.
3. **Visualization never mutates physics state.** Trajectories are immutable after adaptation.
4. **Presentation ≠ GR.** Screen-space lensing / bloom are cosmetic.
5. **CPU solver is the reference.** GPU shader math is for interaction until shared abstractions catch up.
6. **Preserve validated math.** Architecture changes extract modules; they do not silently rewrite Christoffel formulas.

---

## 9. Extensibility sketch (future Kerr)

1. Implement `Spacetime::KerrMetric : Metric`.
2. Add `KerrParameters { mass, spin }`.
3. Add `run_simulation(config, KerrParameters, BoundOrbitInitialConditions)` (and IC recipes as needed).
4. In `examples/*/main.cpp`, set `sim.spacetime = Kerr` and construct `KerrParameters`.

No structural change to viewer, export, or benchmark executables.

---

## 10. Related documentation

| Document | Status |
|----------|--------|
| [`ARCHITECTURE.md`](ARCHITECTURE.md) | **Current** — this file |
| [`RUNNING.md`](RUNNING.md) | **Current** — build / GPU |
| [`VISUALIZATION_GUIDE.md`](VISUALIZATION_GUIDE.md) | **Current** — CPU UX |
| [`frame_capture/`](frame_capture/) | **Current** — GPU capture |
| [`reports/Penrose_from_First_Principles.md`](reports/Penrose_from_First_Principles.md) | Science reference |
| [`reviews/`](reviews/) | Reviews (not normative) |
| [`legacy/`](legacy/) | Historical plans / superseded architecture notes |
