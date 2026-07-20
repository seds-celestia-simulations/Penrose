# Realtime Modularization Plan

## Revised Architecture

```
                    shared/  (GR abstractions only)

        Metric interface, Observer, Geodesic State,
        Coordinate System, Physical Constants, Units

               ↙                          ↘

     physics/                          realtime/
                                        visualization/
    (implementations)                   (implementations)

    TrajectorySolver, RK4              LutBaker, ray marching,
    SchwarzschildMetric,               shader metrics, Keplerian
    validation benchmarks              particles, camera
```

---

## Phase 0: Populate `shared/` with GR Abstractions

Move existing abstractions and add new ones:

### Move from `physics/` -> `shared/`

- `physics/state/State.h` -> `shared/state/GeodesicState.h` — rename to clarify it's a shared concept. Uses Eigen types (as before). `shared/` declares Eigen as a public dependency. `physics/` and `visualization/` continue with no code change.
- `physics/metrics/Metric.h` -> `shared/spacetime/Metric.h` — the abstract `Spacetime::Metric` interface stays the same. Move only the abstraction, not the concrete `SchwarzschildMetric`.
- `physics/metrics/CoordinateChart.h` -> `shared/coordinates/CoordinateChart.h` — coordinate transforms are a shared definition, not physics-specific.

### Add new shared abstractions

- `shared/constants/PhysicalConstants.h` — `G` (gravitational constant), `c` (speed of light), `M_sun`, `rs_from_mass(M)`, geometric vs SI unit definitions.
- `shared/observer/Observer.h` — `Observer` interface: `position()`, `velocity()`, `frame()` — conceptual abstraction for where the camera/observer is and how they're moving.
- `shared/units/Units.h` — unit system definitions: `LengthUnit`, `TimeUnit`, conversion factors between geometric units (`G=c=1`) and SI.

### Design rule

`shared/` contains only abstract interfaces, type definitions, constants, and pure functions. No application logic (no RK4, no integrators, no solver loops). The litmus test: if you include a `shared/` header, you should not pull in any application-specific code.

### Build

```cmake
add_library(penrose_shared INTERFACE)
target_include_directories(penrose_shared INTERFACE shared/)
target_link_libraries(penrose_shared INTERFACE Eigen3::Eigen)
```

---

## Phase 1: Restructure `realtime/`

### Directory layout

```
realtime/
├── core/              # Shader, Window, Texture, Framebuffer, FrameCapture
│   ├── Engine.h/.cpp
│   ├── Window.h/.cpp
│   ├── Shader.h/.cpp
│   ├── ShaderManager.h/.cpp     # Metric-aware shader loading
│   ├── Framebuffer.h/.cpp       (was RenderTarget)
│   ├── Texture.h/.cpp
│   └── FrameCapture.h
│
├── render/            # Rendering pipeline (passes)
│   ├── Renderer.h/.cpp          # Fullscreen quad + pass dispatch
│   ├── GeodesicPass.h/.cpp      # Ray marching shader management
│   ├── ParticlePass.h/.cpp      # SSBO particle upload + draw
│   └── PostProcessPass.h/.cpp   # Future: bloom, tone mapping
│
├── spacetime/         # Metric implementations (uses shared/Metric.h)
│   ├── Metric.h                # Re-exports shared/Metric.h or adds GPU params
│   ├── Schwarzschild.h/.cpp    # Concrete Schwarzschild params + LUT baking
│   ├── Kerr.h/.cpp             # Future: Concrete Kerr params + LUT baking
│   ├── LutBaker.h/.cpp         # CPU geodesic LUT (Metric-agnostic, takes Metric&)
│   └── GeodesicODE.h/.cpp      # CPU ODE step (Metric-agnostic)
│
├── scene/             # Scene objects
│   ├── Camera.h/.cpp            # FPS camera (was in renderer/)
│   ├── AccretionDisk.h/.cpp     # Keplerian particle system
│   ├── FallingParticles.h/.cpp
│   ├── Particle.h/.cpp          # GPU-aligned particle struct
│   └── SceneBuilder.h/.cpp      # Assembles camera + particles + metric
│
├── shaders/           # GLSL only
│   ├── quad.vert
│   ├── common/
│   │   ├── orbit_ode.glsl
│   │   ├── raymarch.glsl
│   │   └── disk_intersection.glsl
│   ├── metrics/
│   │   ├── schwarzschild_orbit.glsl
│   │   └── kerr_orbit.glsl  (future)
│   ├── reduced.frag
│   ├── quad.frag
│   └── upscale.frag
│
├── resources/         # Skybox textures
├── gpu/               # GLAD loader
└── visualization/
    └── ppm_to_video.py
```

### Abstraction seam 1: Metric

The abstract `Metric` interface now lives in `shared/` and is used by both `physics/` and `realtime/`. `realtime/` depends on it for:

- `LutBaker` — calls `Metric::christoffel()` to drive geodesic integration
- `GeodesicODE` — encapsulates the orbit ODE in terms of a `Metric`

The concrete `SchwarzschildMetric` lives in `realtime/spacetime/` (not `shared/`). A future `KerrMetric` would live there too. These implement the shared `Metric` interface with their own numerical code.

Note: `physics/SchwarzschildMetric` and `realtime/spacetime/Schwarzschild` are **independent implementations** that both implement the shared `Metric` interface. They may use different numerical methods (Eigen vs GLM, double vs float) but produce the same physical results.

### Abstraction seam 2: ParticleSystem

```cpp
// realtime/scene/ParticleSystem.h
class ParticleSystem {
public:
    virtual ~ParticleSystem() = default;
    virtual void update(float deltaTime, const Spacetime::Metric& metric) = 0;
    virtual const std::vector<Particle>& getParticles() const = 0;
};
```

- `AccretionDisk` : `ParticleSystem` — current Keplerian approximation, self-contained.
- `FallingParticleSystem` : `ParticleSystem` — Newtonian gravity, self-contained.
- `GeodesicAccretionDisk` : `ParticleSystem` — future: uses the shared `Metric` interface to integrate full geodesics on CPU.

### Abstraction seam 3: RenderPass

```cpp
// realtime/render/RenderPass.h
class RenderPass {
public:
    virtual ~RenderPass() = default;
    virtual void execute(const Scene& scene, const Framebuffer& target) = 0;
    virtual const char* name() const = 0;
};
```

- `GeodesicPass` — fullscreen ray marching.
- `ParticlePass` — SSBO particle rendering.
- `PostProcessPass` — future bloom/tone mapping.

`Renderer` holds a `vector<unique_ptr<RenderPass>>` and calls each `execute()` in order. New passes can be added without modifying existing code.

### Build

```cmake
add_library(realtime STATIC
    realtime/core/Engine.cpp
    realtime/core/Window.cpp
    realtime/core/Shader.cpp
    realtime/core/Framebuffer.cpp
    realtime/core/Texture.cpp
    realtime/render/Renderer.cpp
    realtime/render/GeodesicPass.cpp
    realtime/render/ParticlePass.cpp
    realtime/spacetime/Schwarzschild.cpp
    realtime/spacetime/LutBaker.cpp
    realtime/scene/Camera.cpp
    realtime/scene/AccretionDisk.cpp
    realtime/scene/FallingParticles.cpp
)
target_include_directories(realtime PUBLIC realtime/)
target_link_libraries(realtime PUBLIC penrose_shared)
target_link_libraries(realtime PRIVATE glfw glad glm)

add_executable(Penrose realtime/main.cpp)
target_link_libraries(Penrose PRIVATE realtime)
```

Key difference from the original proposal: `realtime` links `penrose_shared` (not `physics`). This means:

- `realtime` has access to `State`, `Metric` interface, constants, observer concepts.
- `realtime` does **not** link `GeodesicDynamics`, `TrajectorySolver`, or any `physics/` implementation.
- `realtime` implements its own LutBaker, using only the shared Metric interface.

---

## Phase 2: Refactor `physics/`

### Remove moved files

Delete `physics/state/State.h`, `physics/metrics/Metric.h`, `physics/metrics/CoordinateChart.h` (now in `shared/`).

Update all `#include` paths in `physics/`:

```cpp
// Before:
#include "state/State.h"
#include "metrics/Metric.h"
#include "metrics/CoordinateChart.h"

// After:
#include <shared/state/GeodesicState.h>
#include <shared/spacetime/Metric.h>
#include <shared/coordinates/CoordinateChart.h>
```

### Build as a static library

Replace the `PHYSICS_ENGINE_SOURCES` variable with a proper library:

```cmake
add_library(physics STATIC
    physics/metrics/SchwarzschildMetric.cpp
    physics/geodesics/GeodesicDynamics.cpp
    physics/integrators/RK4Integrator.cpp
    physics/simulation/TrajectorySolver.cpp
    physics/simulation/TerminationPolicy.cpp
    physics/validation/freefall.cpp
    physics/validation/orbital.cpp
    physics/validation/null_geodesic.cpp
)
target_include_directories(physics PUBLIC physics/)
target_link_libraries(physics PUBLIC penrose_shared)
target_link_libraries(physics PUBLIC Eigen3::Eigen)
```

Now both `benchmark_test` and `visualization_example` link `physics` directly instead of listing sources:

```cmake
add_executable(benchmark_test physics/validation/main_benchmark.cpp)
target_link_libraries(benchmark_test PRIVATE physics)

add_executable(visualization_example examples/direct_integration/main.cpp)
target_link_libraries(visualization_example PRIVATE physics visualization)
```

### No behavioral changes

The `SchwarzschildMetric::christoffel()` implementation stays identical. The `TrajectorySolver::solve()` loop stays identical. The benchmark validation suite stays identical. Only include paths and build system change.

---

## Phase 3: Shader Modularization

### Extract common GLSL

Move shared raymarch infrastructure into `shaders/common/`:

**`shaders/common/orbit_ode.glsl`** — declares the function signature:

```glsl
void orbitDerivative(float u, float v, out float du, out float dv);
```

**`shaders/common/raymarch.glsl`** — shared loop logic (ray setup, skybox sampling, particle intersection, disk intersection). Calls `orbitDerivative()` without knowing which metric provides it.

**`shaders/common/disk_intersection.glsl`** — equatorial crossing detection, procedural noise, thermal color mapping.

### Per-metric GLSL in `shaders/metrics/`

**`shaders/metrics/schwarzschild_orbit.glsl`:**

```glsl
uniform float uRs;
void orbitDerivative(float u, float v, out float du, out float dv) {
    du = v;
    dv = -u + 1.5 * uRs * u * u;
}
```

**`shaders/metrics/kerr_orbit.glsl`** (future):

```glsl
uniform float uRs;
uniform float uSpin;
void orbitDerivative(float u, float v, out float du, out float dv) {
    du = v;
    float a = uSpin;
    dv = -u + 1.5 * uRs * u * u - a * a * u * u * u;
}
```

### ShaderManager

New class in `core/ShaderManager.h/.cpp` that handles metric-aware shader loading:

```cpp
class ShaderManager {
public:
    std::unique_ptr<Shader> loadReduced(const std::string& metricName);
    std::unique_ptr<Shader> loadFull(const std::string& metricName);
    void precompileAll(const std::vector<std::string>& metricNames);
    void switchMetric(const std::string& metricName);
};
```

The `ShaderManager` uses `GL_ARB_shading_language_include` (OpenGL 4.3+) or manual string concatenation to inject the metric-specific `orbitDerivative()` definition before the shared raymarch code.

### Fast path: LUT-based (optional)

A pure-LUT path (`shaders/reduced_lut.frag`) that replaces RK4 stepping with a single LUT texture lookup:

```glsl
uniform sampler2D uGeodesicLUT;  // baked by CPU using shared Metric
vec2 uv = vec2(r_normalized, gamma_normalized);
float deltaPhi = texture(uGeodesicLUT, uv).r;
```

This is **O(1)** per pixel (one texture lookup) instead of O(150) steps. Lower quality (LUT resolution limited) but completely metric-agnostic and much faster. The `GeodesicPass` selects between reduced and LUT paths.

### Research path: Christoffel Texture (future)

For the full `quad.frag` path with arbitrary metrics, upload Christoffel symbols as a 3D texture pre-computed on CPU using the shared `Metric` interface. The shader samples the texture rather than evaluating inline expressions.

---

## Phase 4: Update `visualization/`

### Include from shared/ instead of physics/

Change three files that currently include `physics/state/State.h`:

- `visualization/Scene/SceneBuilder.cpp`
- `visualization/Trajectory/TrajectoryAdapter.cpp`
- `visualization/Tests/TestTrajectory.cpp`

```cpp
// Before:
#include "../../physics/state/State.h"

// After:
#include <shared/state/GeodesicState.h>
```

### Build

```cmake
target_link_libraries(visualization PUBLIC penrose_shared)
```

No other changes needed — `visualization/` already builds as a static library and the State type is the same.

---

## Phase 5: CMake Top-Level Consolidation

After all phases:

```cmake
cmake_minimum_required(VERSION 3.22)
project(Penrose)

# ── Shared GR abstraction layer (header-only) ──
add_library(penrose_shared INTERFACE)
target_include_directories(penrose_shared INTERFACE shared/)
target_link_libraries(penrose_shared INTERFACE Eigen3::Eigen)

# ── Scientific Framework (physics pipeline) ──
add_library(physics STATIC
    physics/metrics/SchwarzschildMetric.cpp
    physics/geodesics/GeodesicDynamics.cpp
    physics/integrators/RK4Integrator.cpp
    physics/simulation/TrajectorySolver.cpp
    physics/simulation/TerminationPolicy.cpp
    physics/validation/freefall.cpp
    physics/validation/orbital.cpp
    physics/validation/null_geodesic.cpp
)
target_link_libraries(physics PUBLIC penrose_shared)

add_executable(benchmark_test physics/validation/main_benchmark.cpp)
target_link_libraries(benchmark_test PRIVATE physics)

# ── CPU Visualization ──
add_library(visualization STATIC ...)
target_link_libraries(visualization PUBLIC penrose_shared)

add_executable(visualization_export ...)
target_link_libraries(visualization_export PRIVATE visualization physics)

# ── GPU Real-Time Engine ──
add_library(realtime STATIC
    realtime/core/Engine.cpp
    realtime/core/Window.cpp
    realtime/core/Shader.cpp
    realtime/render/Renderer.cpp
    realtime/render/GeodesicPass.cpp
    realtime/spacetime/Schwarzschild.cpp
    realtime/spacetime/LutBaker.cpp
    realtime/scene/Camera.cpp
    realtime/scene/AccretionDisk.cpp
    ...
)
target_link_libraries(realtime PUBLIC penrose_shared)
target_link_libraries(realtime PRIVATE glfw glad glm)

add_executable(Penrose realtime/main.cpp)
target_link_libraries(Penrose PRIVATE realtime)
```

---

## Final Dependency Graph

```
                    ┌─────────────┐
                    │  penrose    │
                    │  _shared    │  (header-only: Metric, State, Constants,
                    │             │   CoordinateChart, Observer, Units)
                    └──────┬──────┘
                           │        Eigen3
               ┌───────────┼───────────┐
               │           │           │
        ┌──────▼────┐ ┌───▼────┐ ┌────▼─────┐
        │  physics  │ │  vis.  │ │ realtime  │
        │           │ │        │ │           │
        │ Metric    │ │ Scene  │ │ Camera    │
        │ Dynamics  │ │ Raster │ │ Particles │
        │ Solver    │ │ PPM    │ │ LutBaker  │
        │ Validate  │ │        │ │ Shaders   │
        └───────────┘ └────────┘ └───────────┘
              │                     │
        Eigen3 only           glfw, glad, glm
```

Key properties:

- No arrows between physics, visualization, and realtime. They are peers.
- `penrose_shared` is the only common dependency.
- Each app links its own framework dependencies (Eigen for physics, GLFW/GLAD/GLM for realtime).
- If `physics/` is removed, `Penrose` still compiles. If `realtime/` is removed, `benchmark_test` still compiles.

---

## Migration Order

| Phase | What | Depends On | Risk |
|---|---|---|---|
| **0** | Populate `shared/` | None | Low — new files only, existing code unchanged |
| **1a** | Restructure `realtime/` dirs | None | Medium — file moves break includes |
| **1b** | Add abstraction seams | Phase 0 | Low — interfaces only |
| **2a** | Move shared files from `physics/` | Phase 0 | Medium — include paths change |
| **2b** | Build `physics/` as static lib | Phase 2a | Low — CMake only |
| **1c** | Build `realtime/` as static lib | Phase 1a | Low — CMake only |
| **3** | Shader modularization | Phase 1a | Medium — GLSL restructuring |
| **4** | Update `visualization/` includes | Phase 0 | Low — 3 file changes |
| **5** | CMake consolidation | All above | Low — final wiring |

Each phase can be done independently and verified that the project still builds and runs.
