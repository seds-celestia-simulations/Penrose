# Penrose

**Penrose is a modular General Relativity framework for simulating, analyzing, and visualizing particle and photon trajectories in curved spacetime.**

Built around a physics-first architecture, Penrose combines a validated CPU reference solver, scientific analysis tools, a CPU trajectory visualization engine, and a GPU real-time renderer into a unified framework for studying relativistic motion.

Originally developed for Schwarzschild black holes, the long-term goal is a general framework supporting arbitrary spacetime metrics, gravitational lensing, and relativistic optical systems such as the Solar Gravitational Lens (SGL).

---

## Physics

Penrose evolves **timelike** and **null geodesics** by numerically integrating the geodesic equation

$$\frac{d^2x^\mu}{d\tau^2}+\Gamma^\mu_{\alpha\beta}\frac{dx^\alpha}{d\tau}\frac{dx^\beta}{d\tau}=0\$$

using a modular spacetime abstraction.

The framework separates:

- spacetime geometry
- equations of motion
- numerical integration
- trajectory storage
- visualization preparation
- rendering

allowing new metrics to be introduced without modifying the core simulation pipeline, and allowing visualization quality to evolve without changing physics accuracy.

---

# Gallery

## Real-Time Schwarzschild Rendering

<img width="1920" height="675" alt="image" src="https://github.com/user-attachments/assets/09155160-bac0-4b90-a0d9-e39732910ce2" />


## Null Geodesic Evolution

<img width="1559" height="864" alt="image" src="https://github.com/user-attachments/assets/eefc82a2-d28a-4da2-b755-80b33b23bd14" />


---


# Capabilities

Penrose contains a complete scientific benchmarking pipeline.

Current validation scenarios include

- Stable bound orbits
- Radial freefall
- Null geodesic scattering
- Critical photon trajectories
- Integration convergence studies
- Conserved quantity monitoring

Outputs include

- CSV trajectory data
- Benchmark metrics
- Validation reports
- Publication-quality scientific figures
- Interactive visualization
- GPU real-time rendering

---

# Features

## Physics

- Schwarzschild metric implementation
- Modular spacetime interface
- Timelike and null geodesics
- Analytical Christoffel symbols
- RK4 geodesic integration
- Configurable termination policies
- Independent multi-particle simulations (`SimulationRequest` / `run_all`)
- Extensible metric architecture

## Scientific Computing

- Reference scientific implementation
- Scientific benchmark suite
- Automatic validation metrics
- CSV trajectory export
- Python analysis pipeline (`visualization/scientific/`)
- Deterministic simulations

## Visualization

### CPU Trajectory Visualization

- Three-stage pipeline: physics → storage → preparation → render
- Physics resolution independent of visualization resolution
- Interactive orbit viewer
- Multiple simultaneous trajectories (independent sims overlaid in prep)
- Presentation-oriented rendering
- Headless image export
- PPM animation sequences

### GPU Real-Time Renderer

- Interactive Schwarzschild renderer
- OpenGL + GLSL
- Real-time camera controls
- Frame capture (P key) → PPM → video
- High-performance visualization

---

# Architecture

Penrose consists of three complementary pipelines.

## 1. Scientific Physics Pipeline (CPU)

Reference implementation responsible for correctness.

```text
SimulationRequest (per particle)
    ↓
Metric + Dynamics + Integrator
    ↓
TrajectorySolver
    ↓
PhysicsTrajectory storage / Benchmarks / CSV
```

---

## 2. CPU Trajectory Visualization

Consumes stored trajectories; never constructs metrics or solvers.

```text
PhysicsTrajectory(s)
      ↓
prepare_scene          # Stage 2 (pass-through today; interpolation reserved)
      ↓
Scene + Camera
      ↓
CPU Rasterizer / Presentation
      ↓
Viewer / Images
```

Presentation effects are purely visual and never influence the underlying simulation.

---

## 3. GPU Real-Time Rendering

Interactive visualization of Schwarzschild spacetime.

```text
Engine / Camera
    ↓
Renderer
    ↓
Fragment Shader
    ↓
Geodesic March
    ↓
Frame (+ optional FrameCapture)
```

The CPU physics pipeline remains the scientific reference implementation.

The GPU renderer is optimized for interactive exploration.

Current architecture reference: [`docs/ARCHITECTURE.md`](docs/ARCHITECTURE.md).

---

# Repository Structure

```text
penrose/
├── run/                  # Config entry points: benchmark / viewer / export
├── physics/              # CPU reference solver
│   ├── metrics/          # Spacetime geometry (Schwarzschild)
│   ├── geodesics/        # Equations of motion
│   ├── integrators/      # RK4 integration
│   ├── simulation/       # TrajectorySolver, SimulationRequest, pipeline
│   ├── validation/       # Benchmark suite + BenchmarkRunner
│   └── export/           # Benchmark CSV I/O helpers
├── visualization/        # CPU trajectory renderer + scientific Python
│   ├── Preparation/      # Stage 2: StoredTrajectory, prepare_scene
│   ├── Apps/             # ViewerApp, DisplayBlit (internal)
│   ├── Renderer/         # Rasterization pipeline
│   ├── Scene/            # Scene composition
│   └── scientific/       # Analysis / plots / reports
├── realtime/             # GPU interactive renderer
│   ├── core/             # Engine, Window, FrameCapture
│   ├── render/           # OpenGL renderer
│   ├── scene/            # Camera, particles
│   ├── shaders/          # GLSL geodesic marching
│   ├── resources/        # Textures & assets
│   └── visualization/    # PPM → video helper
├── shared/               # State + Metric interface
├── docs/                 # Architecture & usage guides
├── outputs/              # Generated benchmarks, figures, renders, notebooks
├── vendor/               # Eigen, stb (header-only)
├── CMakeLists.txt
├── vcpkg.json
└── requirements.txt      # Python analysis deps
```

---

# Quick Start

## Build

```bash
git clone https://github.com/microsoft/vcpkg.git ~/vcpkg
~/vcpkg/bootstrap-vcpkg.sh

cmake -B build -S . \
  -DCMAKE_TOOLCHAIN_FILE=$HOME/vcpkg/scripts/buildsystems/vcpkg.cmake

cmake --build build
```

---

## Run

CPU visualization uses **three config-driven executables**. Edit the matching `run/*/main.cpp`, rebuild, and run — no CSV paths or CLI flags required.

| Goal | Edit | Command |
|------|------|---------|
| GPU renderer | — | `./build/Penrose` |
| Physics benchmarks | `run/benchmark/main.cpp` | `./build/physics_benchmark` |
| Interactive CPU viewer | `run/viewer/main.cpp` | `./build/visualization_viewer` |
| Export still / sequence | `run/export/main.cpp` | `./build/visualization_export` |

```bash
# Example: change initial conditions in run/viewer/main.cpp, then
cmake --build build --target visualization_viewer
./build/visualization_viewer
```

Complete walkthrough: [`docs/VISUALIZATION_GUIDE.md`](docs/VISUALIZATION_GUIDE.md) · Architecture: [`docs/ARCHITECTURE.md`](docs/ARCHITECTURE.md) · Run guide: [`docs/RUNNING.md`](docs/RUNNING.md)

---

# Outputs

Benchmark and render runs generate timestamped outputs under:

```text
outputs/
  benchmark_data/
  validation_figures/
  rendered_frames/
  analysis/
```

including

- trajectory CSVs
- benchmark metrics
- validation reports
- publication figures
- rendered images

GPU capture sessions write separately to `imagesequence/`.

---

# Documentation

| Document | Description |
|----------|-------------|
| [`docs/ARCHITECTURE.md`](docs/ARCHITECTURE.md) | Current architecture (sole reference) |
| [`docs/VISUALIZATION_GUIDE.md`](docs/VISUALIZATION_GUIDE.md) | CPU three-executable UX |
| [`docs/RUNNING.md`](docs/RUNNING.md) | Install / run all pipelines |
| [`visualization/README.md`](visualization/README.md) | CPU visualization module |
| [`docs/frame_capture/`](docs/frame_capture/) | GPU frame capture + PPM → video |
| [`docs/reviews/`](docs/reviews/) | Historical architecture reviews |
| [`docs/reports/`](docs/reports/) | Long-form science notes |

---

# Roadmap

Penrose is evolving toward a general relativistic simulation framework.

Planned capabilities include

- Solar Gravitational Lensing
- Kerr spacetime
- Numerical metrics
- Coupled multi-body dynamics (independent multi-particle overlay already supported)
- Photon bundles / ray ensembles
- Spline / adaptive visualization resampling
- Relativistic optical systems
- Additional visualization backends

---

# Current Status

Penrose currently provides a complete Schwarzschild simulation framework consisting of

- validated CPU physics
- scientific benchmarking
- three-stage trajectory visualization (including multi-particle overlay)
- GPU real-time rendering

Development is now focused on generalized spacetime support and expanding Penrose beyond Schwarzschild into a modular General Relativity research framework.

---
