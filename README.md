# Penrose

**A General Relativity framework for simulating, analyzing, and visualizing particle and photon trajectories in curved spacetime.**

Penrose combines a validated CPU reference solver, scientific benchmarking tools, a modular trajectory visualization pipeline, and a GPU real-time renderer to study particle and photon motion in curved spacetime.

---

## Real-Time Schwarzschild Rendering

<img width="1920" height="675" alt="image" src="https://github.com/user-attachments/assets/09155160-bac0-4b90-a0d9-e39732910ce2" />


## Null Geodesic Evolution

<img width="1559" height="864" alt="image" src="https://github.com/user-attachments/assets/eefc82a2-d28a-4da2-b755-80b33b23bd14" />

---

## Physics

Penrose evolves **timelike** and **null geodesics** by numerically integrating the geodesic equation on arbitrary spacetime metrics.

$$\frac{d^2x^\mu}{d\tau^2}+\Gamma^\mu_{\alpha\beta}\frac{dx^\alpha}{d\tau}\frac{dx^\beta}{d\tau}=0\$$

The simulation engine separates spacetime geometry, equations of motion, numerical integration, data storage, and visualization, allowing physical models and rendering systems to evolve independently.

---

## Current Capabilities

### Physics

* Schwarzschild spacetime
* Timelike and null geodesic evolution
* Analytical Christoffel symbols
* RK4 geodesic integration
* Configurable termination policies
* Extensible spacetime interface for future metrics

### Scientific Analysis

* Reference CPU implementation
* Validation benchmark suite
* Conserved quantity monitoring
* Integration convergence studies
* CSV trajectory export
* Python-based analysis and figure generation

### Trajectory Visualization

A modular visualization pipeline decoupled from the physics engine.

* Interactive GPU trajectory viewer
* Headless CPU rasterization and image export
* Multiple simultaneous trajectories
* Resolution-independent rendering pipeline
* Animation sequence generation

### Real-Time Rendering

An independent GPU renderer for interactive exploration of relativistic optics.

* Real-time Schwarzschild ray marching
* OpenGL / GLSL renderer
* Interactive camera controls
* Frame capture and image export
* High-performance gravitational lensing visualization

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

## 2. Trajectory Visualization

Consumes stored trajectories; never constructs metrics or solvers.

```text
PhysicsTrajectory(s)
      ↓
prepare_scene          # Stage 2 (pass-through today; interpolation reserved)
      ↓
Scene + Camera
      ↓
TrajectoryRenderBackend
  ├── GpuPolylineBackend     → interactive viewer
  └── CpuRasterizerBackend   → headless PPM export
```

Presentation effects are purely visual and never influence the underlying simulation. The interactive viewer and the `realtime/` ray marcher are independent OpenGL apps.

---

## 3. GPU Real-Time Rendering

Interactive visualization of Schwarzschild spacetime via fragment ray marching.

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

The GPU ray-march renderer is optimized for interactive lensing exploration.

Current architecture reference: [`docs/ARCHITECTURE.md`](docs/ARCHITECTURE.md).

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

Trajectory visualization uses **config-driven executables**. Edit the matching `run/*/main.cpp`, rebuild, and run — no CSV paths or CLI flags required for viewer/export.

| Goal | Edit | Command |
|------|------|---------|
| GPU ray-march renderer | — | `./build/Penrose` |
| Physics benchmarks | `run/benchmark/main.cpp` | `./build/physics_benchmark` |
| Interactive trajectory viewer (GPU) | `run/viewer/main.cpp` | `./build/visualization_viewer` |
| Export still / sequence (CPU) | `run/export/main.cpp` | `./build/visualization_export` |

```bash
# Example: change initial conditions in run/viewer/main.cpp, then
cmake --build build --target visualization_viewer
./build/visualization_viewer
```

Complete walkthrough: [`docs/VISUALIZATION_GUIDE.md`](docs/VISUALIZATION_GUIDE.md) · Architecture: [`docs/ARCHITECTURE.md`](docs/ARCHITECTURE.md) · Run guide: [`docs/RUNNING.md`](docs/RUNNING.md)

---

# Documentation

| Document | Description |
|----------|-------------|
| [`docs/ARCHITECTURE.md`](docs/ARCHITECTURE.md) | Current architecture (sole reference) |
| [`docs/VISUALIZATION_GUIDE.md`](docs/VISUALIZATION_GUIDE.md) | Trajectory viz three-executable UX |
| [`docs/RUNNING.md`](docs/RUNNING.md) | Install / run all pipelines |

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
- GPU post-process parity with CPU export (bloom / cosmetic lensing)

---

# Current Status

Penrose currently provides a complete Schwarzschild simulation framework consisting of

- validated CPU physics
- scientific benchmarking and Python analysis
- three-stage trajectory visualization with dual Stage 3 backends (GPU viewer / CPU export)
- GPU real-time ray-march rendering

Development is now focused on generalized spacetime support and expanding Penrose beyond Schwarzschild into a modular General Relativity research framework.

---
