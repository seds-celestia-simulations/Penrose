# Penrose

**Penrose** is a modular General Relativity framework for simulating and visualizing particle and photon trajectories in curved spacetime.

Originally developed as a Schwarzschild black hole ray tracer, Penrose now combines a **CPU reference physics pipeline**, a **CPU scientific presentation renderer**, and a **GPU real-time ray tracer** behind a shared modular architecture.
<br>

**GPU-accelerated realtime simulation**

<img width="975" height="575" alt="penrose" src="https://github.com/user-attachments/assets/42979505-8273-47f0-a5e5-b9e5b6aa6c3e" />

---

## Features

- Schwarzschild spacetime (analytic metric, Christoffel symbols, geodesic dynamics)
- Fourth-order Runge–Kutta geodesic integration
- **CPU reference physics** — trajectory solving, benchmarking, CSV export
- **CPU presentation visualization** — orbit viewer, PPM export, screen-space lensing at Schwarzschild radius
- **GPU real-time ray tracing** — interactive OpenGL Schwarzschild renderer
- Scientific benchmarking, validation figures, and metrics (Python)
- Modular layout for future metrics (Kerr, FLRW, numerical fields, …)

---

## Quick start

### Prerequisites

| Tool | Notes |
|------|--------|
| CMake 3.22+ | Build system |
| C++20 compiler | GCC 11+, Clang 14+, or MSVC 2019+ |
| [vcpkg](https://github.com/microsoft/vcpkg) | Eigen3, GLM, and other deps |
| Python 3.9+ | Optional — validation plots and PPM→video |
| OpenGL + display | Required for interactive apps only |

### Build

From the repository root:

```bash
# One-time vcpkg bootstrap (if not already done)
git clone https://github.com/microsoft/vcpkg.git ~/vcpkg
~/vcpkg/bootstrap-vcpkg.sh

# Configure (auto-detects ./vcpkg if present)
cmake -B build -S . -DCMAKE_TOOLCHAIN_FILE=$HOME/vcpkg/scripts/buildsystems/vcpkg.cmake

# Compile
cmake --build build
```

On Windows, use `bootstrap-vcpkg.bat` and `--config Release` as needed.

### Run something immediately

| Goal | Command |
|------|---------|
| GPU ray tracer | `./build/Penrose` |
| Generate benchmark CSVs | `./build/benchmark_test` |
| Interactive orbit viewer | `./build/visualization_viewer --csv outputs/benchmark_data/<timestamp>/orbital.csv` |
| Viewer with built-in orbit (no CSV) | `./build/visualization_viewer_example` |
| Export a still PPM | `./build/visualization_export --csv outputs/benchmark_data/<timestamp>/orbital.csv` |
| Run visualization tests | `./build/visualization_test` |

**Full walkthrough (benchmarks → viewer → figures):** [`docs/frame_capture/VISUALIZATION_GUIDE.md`](docs/frame_capture/VISUALIZATION_GUIDE.md)

**GPU app details:** [`docs/RUNNING.md`](docs/RUNNING.md)

---

## Executables

| Target | Description |
|--------|-------------|
| `Penrose` | GPU real-time Schwarzschild ray marcher |
| `benchmark_test` | Integrates geodesics, writes CSVs to `outputs/benchmark_data/<timestamp>/` |
| `visualization_viewer` | Interactive CPU orbit viewer (GLFW); `--csv`, `--speed`, `--classic` |
| `visualization_viewer_example` | Same viewer with in-process orbit integration |
| `visualization_export` | Headless PPM still or frame sequence |
| `visualization_example` | Headless still from direct C++ integration |
| `visualization_benchmark` | Headless render timing (no display) |
| `visualization_test` | Unit tests for the CPU visualization module |

Build a single target:

```bash
cmake --build build --target visualization_viewer
```

Disable interactive viewers on headless machines:

```bash
cmake -B build -S . -DPENROSE_BUILD_VIEWER=OFF
```

---

## Architecture

Penrose has **three complementary pipelines** that share physics types but stay decoupled at render time.

### 1. Scientific framework (CPU)

Reference implementation for correctness:

```
State → Metric → Dynamics → Integrator → TrajectorySolver → CSV / benchmarks
```

### 2. CPU presentation visualization

Downstream of physics — reads `State` trajectories (in memory or CSV), never modifies the solver:

```
Trajectory → Scene → CPURasterizer → PostProcessor → PPM / viewer blit
```

Presentation-only effects (bloom, vignette, screen-space lensing at projected `rs`) are **not** GR ray tracing.

### 3. GPU real-time visualization

Interactive Schwarzschild ray marching in OpenGL/GLSL:

```
Camera → Renderer → Fragment shader → geodesic march → pixel color
```

The CPU physics pipeline is the reference; the GPU path is optimized for interaction.

---

## Repository structure

```text
penrose/
├── physics/                 # CPU scientific pipeline
│   ├── state/               # Geodesic state vector (shared with visualization)
│   ├── metrics/             # Spacetime metrics
│   ├── geodesics/           # Equations of motion
│   ├── integrators/         # RK4 and related methods
│   ├── simulation/          # TrajectorySolver
│   └── validation/          # Benchmark drivers
├── visualization/           # CPU presentation module
│   ├── Scene/               # Scene graph, playback
│   ├── Renderer/            # CPURasterizer, framebuffer
│   ├── Presentation/        # Post-process, cinematic defaults
│   ├── Camera/              # Orbit camera
│   ├── Trajectory/          # State → Cartesian adapter, CSV loader
│   ├── Apps/                # Viewer, export, benchmark entry points
│   └── scientific/          # Python: metrics, plots, validation report
├── examples/
│   ├── direct_integration/  # Headless integration + PPM
│   └── interactive_viewer/  # Integrated orbit + GLFW viewer
├── realtime/                # GPU ray tracer (Penrose app)
│   ├── main.cpp
│   ├── renderer/
│   ├── shaders/
│   └── visualization/       # Frame capture, ppm_to_video
├── outputs/                 # Timestamped benchmark, figure, and render outputs
├── docs/                    # Running guides, architecture, frame capture
├── CMakeLists.txt
└── README.md
```

---

## Outputs

All scripted runs write under **`outputs/`** with timestamped directories:

```text
outputs/
├── benchmark_data/<timestamp>/     # CSVs, metrics JSON, validation report
├── validation_figures/<timestamp>/ # PNG/PDF scientific plots
└── rendered_frames/<timestamp>/    # PPM stills and sequences
```

After `./build/benchmark_test`, find the latest folder:

```bash
ls -td outputs/benchmark_data/*/ | head -1
```

---

## Documentation

| Document | Contents |
|----------|----------|
| [`docs/frame_capture/VISUALIZATION_GUIDE.md`](docs/frame_capture/VISUALIZATION_GUIDE.md) | **Complete CPU viz guide** — CMake, all targets, viewer controls, export, Python validation, performance |
| [`docs/RUNNING.md`](docs/RUNNING.md) | GPU app install, run, capture, benchmark |
| [`docs/frame_capture/FRAME_CAPTURE.md`](docs/frame_capture/FRAME_CAPTURE.md) | GPU frame capture |
| [`visualization/README.md`](visualization/README.md) | CPU module API and design |
| [`docs/architecture/`](docs/architecture/) | Refactor plans and architecture notes |

---

## Interactive viewer tips

**Faster orbit evolution** (particle moves quicker):

```bash
./build/visualization_viewer --csv path/to/orbital.csv --speed 10
```

**Higher frame rate** (smoother camera):

```bash
./build/visualization_viewer --csv path/to/orbital.csv --classic --width 960 --height 540
```

See [Performance](docs/frame_capture/VISUALIZATION_GUIDE.md#12-performance-faster-playback-and-higher-fps) in the visualization guide for details.

---

## Roadmap

Long-term support for arbitrary relativistic geometries:

- Schwarzschild *(current)*
- Solar Gravitational Lens (SGL)
- Kerr, Reissner–Nordström, FLRW
- User-defined analytic and numerical metrics

New spacetime models are intended to plug in with minimal changes to the integrator and benchmark layers.

---

## Current status

Penrose is under **active development**. The Schwarzschild CPU physics pipeline, benchmark suite, CPU presentation viewer, and GPU ray tracer are all usable today. Work continues on modular metrics, additional geometries, and tighter scientific/visualization workflows.

---

## Pipeline diagram

[Physics and Rendering Pipeline (Mermaid)](https://mermaid.ai/live/edit#pako:eNqVVm1P4zgQ_itW0J6KtnBpXlqa45DYljcd7FYtvQ9HOeQmTuvDiSPbKRSW_37j2Cltl95pKyFlnJlnnpnxM-HViXlCnMiZCVzM0W1_kiP4ffqERmrJaD4zdsywlH2SooIWBI4JSilj0Z6X-IHfbkol-COJ9jrBtJum1jx4oomaR17x3Iw54yLaS9P0ty28hChMmbRwbb9NOt4KLk0Jxp2fgQPXMlbyY3ak1XHD8GfgZkVpodzuUUD8FZTrxiTu_g_UWisNq8Y4j3mek1iRBH3hz_vGQ5ZT0_0-Vtj4loJIdDdx9Al6P5o49yZE_0YtcDienpjXEbrBShGBGr3Pn_ePf52eHE_FyYLEASq4XD0vCNPPKeNYoQzK1Jb5WwEN8RI1Lq5H1_8JA1yiKLINN6xInqxVPZhjSVArQlc5VRQz-oIV5flWzZVXS9e6w9_iNHqD8f5G_YPW3UiRoo5APZwRgTWp1SU9ODhBA8-4edZtgIWiMSNy3XM3fwiDzGAsJY3lR-S9d_Kbzuia8wI1BjCUcwHktuh7hl5g6AXQ9z8CoKgIYOvCf-RXhQU67HsPM1BN8IwKQJcA8B0NfIPkAwnMYtSbCyoVBxEx9As6jWPCdkD66OAQMIcELlkujSvgBbu7AjkuoNAhvCNitSU2--K_92XbHQ3qCe3sTmC6E5qawgjmJiUa5zTlIpNQUDVMuKrVHEGoa7GhiW2b2LZNjpcZFvFcp9eD2QjbrLFvlxLOE3Re5rGeBqh3SGdzhUY0IdvCPdW4ilQirYOh-PXjGnSjyr6VcJ0k0oJDsSBYkYe4us8PGVaCxkQ29q1Me6A4kIfxvSA5OCmC_qTkqYkGgv9DKqgmNGhBBDS_Bjg0irUbd42E9xEJakUoyUNRK2ZF4XTN8ZQxHmsGUiVRBNtBcXFsdtEJwkLg5a7EaxR8Q-HufHw7Hp7BLhtf317dXg7PTvtXXy_u6z20lboWC4EVWfWgSo5G6Hd0B9uqqdfU_aEOvCmlQjHPihLc1oUhl9mU61ErRKBdy0qEGU0KTnO1TXyNcGB37xzDjV5dS0PLLCK9j6u-N1crB-6JbscHlfS4HpaCz-szkFIc-X2UUGFGWQcA4Nz2vCjYEn0tGdORsIMxsNVhkrMFfJuhCYu_1c6Bh4a8XvFae7OMQLQpJdrR6mGZV61heqWVUouoCk_KglE9_kTfs3lN9ZaIjOb4Pf4SxHq20HkuuaAvsNYbfYGf0BeG48d99G2IzmSMC2KPR4_LKXwfP6pgU6o3uCg0mZ75rmqhmneDVrXUQGLW9qztWTuwtm_t0Np26w3a1g6R04T_jmjiRPCtI00HhpthbTqv2nXiqDnJyMSJ4DHB4nHiTPI3iClw_hfnWR0meDmbO1GKmQSrLBJoWp9iuL_Z6lRUC7LHy1w5UdjxKxAnenWeneig2z70giPPbR2FrdDvePB2Ccde6zBww3b7qOv6nY7nhv5b03mpEns6wD3qgrPrtt2wG779Cwm6JS0)
