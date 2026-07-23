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
cmake -B build -S . -DCMAKE_TOOLCHAIN_FILE=path\to\vcpkg.cmake
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
  core/                  # Engine, Window, Shader, ShaderManager, Texture, Framebuffer, FrameCapture
  render/                # Renderer (fullscreen quad), RenderPass, GeodesicPass, UpscalePass
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
| Adding a new metric | Create `.glsl` in `realtime/shaders/metrics/`, register in `Engine::initAssets()` via `shaderManager->loadMetric()` |
| Modifying physics solver | `physics/simulation/TrajectorySolver.h`, `physics/geodesics/GeodesicDynamics.h` |
| Modifying GPU rendering | `realtime/render/Renderer.h`, `realtime/core/Engine.cpp`, `realtime/render/GeodesicPass.cpp` |
| Modifying GLSL shaders | `realtime/shaders/` (see shader conventions below) |
| Adding a new render pass | `realtime/render/RenderPass.h` (interface), create new `*Pass.h/.cpp` in `realtime/render/`, add to `Engine::passes` in `initAssets()` |
| Adding a new metric | Create `.glsl` in `realtime/shaders/metrics/`, register in `Engine::initAssets()` via `shaderManager->loadMetric()` |
| CPU visualization pipeline | `visualization/Presentation/PresentationPipeline.h` |
| CMake changes | `CMakeLists.txt` (top-level) — never create cross-module links |

## Shader conventions (`realtime/shaders/`)

- `#version 430 core` — GLSL 430, OpenGL 4.3 required
- `quad.vert` — shared fullscreen quad vertex shader, used by all fragment shaders
- Shaders are assembled at runtime by ShaderManager from modular `.glsl` files
- Use `#include "relative/path.glsl"` in any `.glsl` file — ShaderManager resolves includes recursively with circular-dependency protection
- Resource paths in `Engine.cpp::initAssets()` must match the CMake `POST_BUILD` copy destinations: `shaders/` and `resources/` (NOT `realtime/shaders/`)

### Module layout

```
shaders/
  common/
    reduced_header.glsl    # #version + uniforms for reduced metric
    quad_header.glsl       # #version + uniforms for quad metric
    particle.glsl          # Particle struct + SSBO + intersection helpers
    noise.glsl             # hash, valueNoise, fBm
    skybox.glsl            # DirectionToUV, PI constant
    disk.glsl              # accumulateDisk (disk-ray intersection)
    reduced_main.glsl      # main() for reduced orbit ray march
    quad_main.glsl         # raymarch() + main() for full Christoffel
  metrics/
    schwarzschild_reduced.glsl  # orbit ODE (includes noise, skybox, disk)
    schwarzschild_full.glsl     # Christoffel symbols (includes skybox)
  quad.vert
```

### How assembly works

ShaderManager concatenates: `header + metric + main`. Each file can `#include` other `.glsl` modules. Example:

- `schwarzschild_reduced.glsl` includes `../common/noise.glsl`, `../common/skybox.glsl`, `../common/disk.glsl`
- `schwarzschild_full.glsl` includes `../common/skybox.glsl` (provides PI)
- Headers include `particle.glsl` (Particle struct + SSBO)

Result is one GLSL string compiled via `glShaderSource`. The old assembled `reduced.frag` / `quad.frag` are kept as reference but are no longer the source of truth.

## RenderPass pipeline

- `RenderPass` (interface) — `execute(PassContext&)`, `name()`
- `GeodesicPass` — fullscreen quad draw with metric shader from ShaderManager, handles camera/LUT/skybox uniforms and particle SSBO binding
- `UpscalePass` — placeholder for future foveated rendering
- Engine holds `vector<unique_ptr<RenderPass>> passes`, iterates in `render()`
- `PassContext` carries camera, time, dimensions, textures to each pass

## ParticleSystem interface

- `ParticleSystem` (interface) — `update(float dt)`, `getParticles() const`
- `FallingParticleSystem` implements `ParticleSystem` (set `rs` via `setRs()`)
- `AccretionDisk` implements `ParticleSystem` (returns `Particle` directly)
- Engine holds `vector<ParticleSystem*> particleSystems` for polymorphic iteration

## ShaderManager

- Manages metric shader loading, caching, and switching
- `loadMetric(type, vertPath, headerPath, metricPath, mainPath)` — register a metric's shader parts
- `setMetric(type)` — switch active metric (lazy-compile on first use)
- `getActive()` — returns current `Shader*`
- `reloadAll()` — recompiles all cached shaders from disk
- `resolveIncludes(filePath, visited)` — recursively resolves `#include "path"` directives, protects against circular includes

## C++ conventions

- OpenGL objects use raw C API (no wrapper classes) — follow existing pattern
- Realtime classes: no namespaces (global scope, like `Renderer`, `Engine`, `Camera`)
- Physics classes: namespaced (`Spacetime::`, `Dynamics::`, `Simulation::`, `Physics::`)
- Visualization classes: `viz::` namespace
- Header-only where possible; `.cpp` files only when there's non-trivial implementation
- No comments unless asked; keep code self-documenting
- Use `std::make_unique` for heap-allocated objects
- Never add a `shared/` dependency to `physics/` that pulls in GL or GLFW headers
