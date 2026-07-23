# Penrose — AI Agent Guide

## What this project is

A black hole visualization engine with two independent pipelines:
- **CPU pipeline** (`physics/` + `visualization/`): scientific geodesic integration → trajectory scenes → PPM/export rendering
- **GPU pipeline** (`realtime/`): real-time GLSL ray marching with OpenGL 4.3

They share only `shared/` (GR type definitions). They do NOT share code or link each other.

## Tech stack

- C++20, CMake 3.22+
- OpenGL 4.3+ (GLSL 430, GLAD loader, GLFW 3.4)
- Eigen3 (CPU physics), GLM (GPU math)
- vcpkg for Eigen3 and GLM; GLFW via FetchContent

## Build (Windows)

```powershell
cmake -B build -S . -DCMAKE_TOOLCHAIN_FILE=D:\dev\cpp\tools\vcpkg\scripts\buildsystems\vcpkg.cmake
cmake --build build --config Debug
```

Adjust `D:\dev\cpp\tools\vcpkg` to your vcpkg install path. The Penrose.exe ends up in `build/Debug/`.

## File layout

```
shared/                  # Cross-module GR definitions (header-only, no .cpp)
  state/GeodesicState.h  # State, Particle, Light structs (Eigen types)
  spacetime/Metric.h     # Abstract Spacetime::Metric interface
  constants/             # G, c, schwarzschild_radius()
  observer/              # Observer stub
  units/                 # Placeholder for unit system

physics/                 # CPU scientific pipeline
  metrics/               # SchwarzschildMetric, CoordinateChart
  geodesics/             # GeodesicDynamics (implements DynamicsModel)
  integrators/           # RK4Integrator
  simulation/            # TrajectorySolver, TerminationPolicy
  validation/            # Benchmark tests (orbital, freefall, null_geodesic)

realtime/                # GPU real-time engine
  core/                  # Engine, Window, Shader, Texture, Framebuffer, FrameCapture
  render/                # Renderer (fullscreen quad), RenderPass interface
  scene/                 # Camera, Particle, ParticleBuffer, ParticleSystem interface
  spacetime/             # LutBaker, AccretionDisk
  shaders/               # GLSL shaders (see shader conventions below)
  gpu/                   # GLAD loader (glad.c, glad.h, KHR/)
  resources/             # Textures (starfield)

visualization/           # CPU headless visualization library
  Camera/                # viz::Camera
  Renderer/              # CPURasterizer, Framebuffer (CPU-side, not OpenGL)
  Scene/                 # Scene, SceneBuilder
  Trajectory/            # Trajectory, TrajectoryAdapter
  Presentation/          # PresentationPipeline (CPU rendering)
  IO/                    # PPM writer, CSV loader, output paths
  Geometry/              # Coordinates, Mesh
  Tests/                 # viz unit tests

examples/                # Standalone examples linking physics + visualization
  direct_integration/    # CPU integration → PPM output
  interactive_viewer/    # CPU integration → GLFW window

vendor/                  # stb_image.h, Eigen (vendored copy)
```

## Critical: Module dependency rules

```
penrose_shared (header-only)
    ↑
    ├── physics     (links: Eigen3)
    ├── visualization (links: Eigen3)
    └── realtime    (links: glfw, glad, glm)
```

- `physics/`, `visualization/`, `realtime/` are PEERS. They never link each other.
- They all link `penrose_shared` for shared types.
- `realtime/` implements its own metric-specific code (LutBaker, AccretionDisk) — it does NOT use `physics/SchwarzschildMetric`.

## When modifying code, always reference:

| Area | Read first |
|------|-----------|
| Any type that crosses modules | `shared/` headers |
| Adding a new metric | `docs/architecture/realtime_modularization_plan.md` (Phase 3) |
| Modifying physics solver | `physics/simulation/TrajectorySolver.h`, `physics/geodesics/GeodesicDynamics.h` |
| Modifying GPU rendering | `realtime/render/Renderer.h`, `realtime/core/Engine.cpp` |
| Modifying GLSL shaders | `realtime/shaders/` (see shader conventions below) |
| CPU visualization pipeline | `visualization/Presentation/PresentationPipeline.h` |
| CMake changes | `CMakeLists.txt` (top-level) — never create cross-module links |

## Shader conventions (`realtime/shaders/`)

- `#version 430 core` — GLSL 430, OpenGL 4.3 required
- `quad.vert` — shared fullscreen quad vertex shader, used by all fragment shaders
- `reduced.frag` — fast LUT-based ray marching (150 steps, alpha compositing)
- `quad.frag` — full Christoffel-symbol RK4 ray marching (500 steps, per-pixel integration)
- All fragment shaders are compiled standalone (no `#include` across shader files yet — see Phase 3 plan for modularization)
- Resource paths in `Engine.cpp::initAssets()` must match the CMake `POST_BUILD` copy destinations: `shaders/` and `resources/` (NOT `realtime/shaders/`)

## C++ conventions

- OpenGL objects use raw C API (no wrapper classes) — follow existing pattern
- Realtime classes: no namespaces (global scope, like `Renderer`, `Engine`, `Camera`)
- Physics classes: namespaced (`Spacetime::`, `Dynamics::`, `Simulation::`, `Physics::`)
- Visualization classes: `viz::` namespace
- Header-only where possible; `.cpp` files only when there's non-trivial implementation
- No comments unless asked; keep code self-documenting
- Use `std::make_unique` for heap-allocated objects
- Never add a `shared/` dependency to `physics/` that pulls in GL or GLFW headers
