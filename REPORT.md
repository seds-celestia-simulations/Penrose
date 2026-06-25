# Penrose: GPU-Accelerated Schwarzschild Black Hole Renderer

## Overview

Penrose is a real-time GPU-based black hole visualization that renders light
rays bending around a Schwarzschild black hole via relativistic ray marching.
The raymarching is computed entirely on the GPU (GLSL 4.30 fragment shader)
using a 4th-order Runge-Kutta integrator for null geodesics. The skybox is
rendered from an equirectangular environment map, and particles (accretion disk
tracers) are uploaded via SSBO for ray intersection.

**Repository**: <https://github.com/AMVS24/Penrose>

---

## Repository Structure

```
Penrose/
├── CMakeLists.txt               # CMake build (C++20, OpenGL 4.3 Core)
├── vcpkg.json                   # Dependencies: glfw3, glad, glm, eigen3
├── README.md                    # Build instructions (vcpkg-based)
├── FRAME_CAPTURE.md             # PPM frame capture documentation
├── PPM_TO_VIDEO_README.md       # PPM→MP4 conversion guide
├── ppm_to_video.py              # Python script: PPM sequence → MP4
├── REPORT.md                    # This file
├── .gitignore                   # Ignores build/
├── .vscode/settings.json        # Empty
├── shaders/
│   ├── quad.vert                # Full-screen quad vertex shader
│   ├── quad.frag                # Black hole raymarching fragment shader
│   └── starfield_original.jpg   # Equirectangular skybox environment map
├── src/
│   ├── Constants.h              # Global constants
│   ├── main.cpp                 # Entry point, GLFW init, render loop
│   ├── physics_engine/
│   │   ├── physics.h            # State/Particle/Light structs (Eigen)
│   │   └── physics.cpp          # CPU-side coordinate transforms, Christoffel symbols, RK4
│   └── render/
│       ├── Camera.h             # FPS-style camera (WASD + arrow keys)
│       ├── FrameCapture.h       # PPM frame capture (P key toggle)
│       ├── Particle.h           # GPU-compatible particle struct
│       ├── ParticleBuffer.cpp/h # SSBO management
│       ├── Renderer.cpp/h       # Render loop, drawing, frame capture
│       ├── Shader.h             # Shader compilation and uniforms
│       ├── Texture.cpp/h        # Skybox texture loading (stb_image)
│       └── Window.cpp/h         # Input processing
├── vendor/
│   ├── stb_image.h              # Image loading (used by Texture.cpp)
│   └── stb_image_write.h        # Image writing (vendored but unused)
├── notes/
│   ├── NOTES.md                 # Bug report: texture byte alignment
│   ├── actual_bg_img.png        # Expected skybox output
│   └── bg_issue_img.jpg         # Distorted (bugged) output
└── build/                       # MSVC build artifacts (not committed)
```

---

## Git History

**31 commits** across 3 branches (`dev`, `fix`, `main`). No tags.

### Contributors

| Contributor | Commits | Email |
|---|---|---|
| = (akshaykaruvathil@gmail.com) |
| AMVS24 (Aditya Melinkeri) |
| avsyde@gmail.com |
| inhumanetrial (Hardik Bisht) |

### Timeline: 17 March 2026 — 20 May 2026

#### Phase 1: Foundation (17-18 March)
| Commit | Description |
|---|---|
| `476f278` | **Initial commit** — empty repo |
| `f9d04f6` | README pipeline link added |
| `be47c1d` | Initial setup commit |
| `5bea915` / `ee63c1d` | Build folder cleanup |
| `19e1c6d` | README with build instructions |
| `728befb` | **Basic shader with parameters** — first shader code |
| `ed734a8` | **Camera + skybox** — equirectangular skybox rendering |
| `8b1740b` | **Refactor** — split monolithic main into render engine files, Eigen3 added |
| `82ef6ac` | **Physics engine skeleton** — `State` struct, Jacobian transforms, stub `Integrator()` |

#### Phase 2: Raymarching (18-19 March)
| Commit | Description |
|---|---|
| `73ce47c` | **Basic raymarching loop** — 100 steps, sphere hit test, `dT=0.1` |
| `cf80d30` | Switched from `vec3` to `vec4` position/velocity |
| `ba70522` | Miscellaneous |
| `1764eda` | Coordinate conversion methods |
| `4456b45` | Intermediate null constraint implementation |
| `a08e740` | **Null geodesic constraint** — `u^t` computed from metric |
| `693d0dd` | **Null constraint working** — Jacobian transpose fix, `dT→dL` rename |
| `d878e28` | **RK4 integrator + Christoffel symbols** (by inhumanetrial) — CPU side |
| `0d405ae` | Integrator added on GPU, but broken |
| `1bdaf74` | **"WE HAVE A BLACK HOLE"** — working black hole with precision issues noted |

Key changes in the breakthrough commit:
- Jacobian transpose bug fixed (`transpose()` removed)
- `rs` changed from `1.0` to `0.25` in shader (divergence from `Constants.h`)
- `theta` clamped to `[1e-6, π-1e-6]` to avoid pole singularities
- `Tph_thph` denominator restructured: `1.0/(tan(theta) + 1e-8)` → erroneous formulation
- **Pole crossing fix** — theta bounce-back logic added
- Normalized ray direction for skybox lookup
- Camera moved to `(0.5, 0.5, 0.5)` (close to the black hole)

#### Phase 3: Bug Fixes (24 March)
| Commit | Description |
|---|---|
| `c61e9ad` | Miscellaneous |
| `e832fd4` | Merge branch `fix` |
| `91eb879` | **Byte alignment bug fix** — `glPixelStorei(GL_UNPACK_ALIGNMENT, 1)` in Texture.cpp |
| `9d075a6` | Bug report documentation committed |
| `55e0db4` | Uncommented debug-bypassed code |
| `0d216d1` | **Black hole re-enabled** (`rs` restored from 0 to 0.25, skybox restored) |

The texture bug: RGB images with width not a multiple of 4 caused diagonal skew
due to OpenGL's default 4-byte unpack alignment (documented in `notes/NOTES.md`).

#### Phase 4: Particles (25-26 March)
| Commit | Description |
|---|---|
| `198651d` | **Particle system** — `Particle.h`, `ParticleBuffer` (SSBO), ray-particle hit detection |
| `a8a790e` | **Improved particles** — proper spherical→cartesian conversion, simplified hit test, `N_STEPS=1000`, `rs=0.25` |

#### Phase 5: Frame Capture (20 May)
| Commit | Description |
|---|---|
| `661b266` | Particle animation (`deltaTime * 0.05` theta rotation) |
| `ed013b5` | **Frame capture system** — `FrameCapture.h`, `Renderer::captureFrame()`, PPM output, Python→MP4 converter |

---

## Physics

### Schwarzschild Metric

The Schwarzschild metric (spherical coordinates, G=c=1):

```
ds² = -(1 - rs/r) dt² + (1 - rs/r)⁻¹ dr² + r² dθ² + r² sin²θ dφ²
```

where `rs = 2M` is the Schwarzschild radius (event horizon).

### Null Geodesics

Light follows null geodesics (`ds² = 0`), giving the constraint:

```
u^t = sqrt( (u^r)²/(1-rs/r) + r²(u^θ)² + r² sin²θ (u^φ)² ) / sqrt(1-rs/r)
```

### Christoffel Symbols

Implemented identically in both CPU (`physics.cpp`) and GPU (`quad.frag`):

| Symbol | Expression | Component |
|---|---|---|
| Γᵗ_tr | rs / (2r(r-rs)) | Time |
| Γʳ_tt | rs(r-rs) / (2r³) | Radial |
| Γʳ_rr | -rs / (2r(r-rs)) | Radial |
| Γʳ_θθ | -(r-rs) | Radial |
| Γʳ_φφ | -(r-rs) sin²θ | Radial |
| Γθ_rθ | 1/r | Polar |
| Γθ_φφ | -sinθ cosθ | Polar |
| Γφ_rφ | 1/r | Azimuthal |
| Γφ_θφ | cotθ | Azimuthal |

### Geodesic Equation

```
d²x^μ / dλ² + Γ^μ_αβ (dx^α/dλ)(dx^β/dλ) = 0
```

Implemented as 4 acceleration components in `find_acceleration()`.

### Integration

**GPU** (active, in `quad.frag`):
- RK4 integrator, `dL = 0.01`, up to `N_STEPS = 1000`
- State: `ray { vec4 x (t,r,θ,φ), vec4 u (u^t,u^r,u^θ,u^φ) }`
- Integration in spherical coordinates; converted to cartesian for particle hits

**CPU** (passive, in `physics.cpp`):
- RK4 integrator, `dt = 0.01` (from `Constants.h`)
- State: `{ Vector4d X, Vector4d U }` via Eigen
- **Not called from main.cpp** — the CPU physics engine is development scaffolding

---

## Rendering Pipeline

1. **Vertex Shader** (`quad.vert`): Passes through a full-screen quad with texture
   coordinates. No geometry processing.

2. **Fragment Shader** (`quad.frag`):
   - Un-project pixel from NDC to world ray using inverse projection×view matrix
   - `raymarch(ro, rd)`:
     1. Initialize ray state `(x=0, ro)` and `(u=0, rd)`
     2. Convert to spherical coordinates via Jacobian
     3. Apply null constraint to set `u^t`
     4. For up to 1000 steps:
        - Check if `r < rs×1.1` → black hole (return black)
        - Convert to cartesian for particle hit detection
        - Check all particles for intersection via sphere test
        - If `r > 20.0` → escape (break)
        - Integrate one RK4 step
        - Handle pole crossing (theta bounds)
     5. Convert final ray to cartesian, look up skybox via `DirectionToUV()`

3. **Post-render**: Optional PPM frame capture via `glReadPixels(GL_FRONT)`.

---

## Known Issues

### 1. Precision Artifacts (Primary Issue)

The raymarching is computed entirely in **GLSL `float` (32-bit single precision)**.
This is the likely cause of visible rendering artifacts:

| Location | Issue | Impact |
|---|---|---|
| Near `r ≈ rs` | Coordinate singularity; `1/(r-rs)` terms diverge | Noise/tearing near the event horizon |
| Near `θ ≈ 0, π` | Pole singularity; `cotθ` diverges | Artifacts near polar axis |
| `cart_to_sph_Jacobian` inverse | GLSL `inverse(mat4)` in fp32 | Jacobian accuracy degrades |
| 1000-step RK4 accumulation | Floating-point error accumulation | Ray drift / missed photon orbits |
| Null constraint normalization | Ratios of small fp32 quantities near horizon | Incorrect `u^t` → wrong bending |
| Coordinate round-trip (`sph→cart→sph`) | Repeated lossy conversions | Cumulative position/velocity error |

### 2. `rs` Mismatch

- `Constants.h`: `rs = 1.0` (CPU physics)
- `quad.frag`: `rs = 0.25` (GPU raymarching)

The CPU physics engine and GPU renderer are configured for different black hole
sizes. The CPU code is unused, but if reconnected it would produce inconsistent
results.

### 3. Duplicate Operator Definitions

`src/physics_engine/physics.h:20-24` defines `operator+` and `operator*` twice:

```cpp
State operator+(const State& other) const { ... }
State operator*(double scalar) const { ... }
// then again:
State operator+(const State& other) const { ... }
State operator*(double scalar) const { ... }
```

The second pair is redundant and causes compiler warnings (or errors with
`-Werror` / `/W4`).

### 4. Unused `stb_image_write.h`

`vendor/stb_image_write.h` is vendored but the project writes PPM files directly
via `std::ofstream` in `Renderer::captureFrame()`. The implemention in
`stb_image_write.h` is also an incomplete/simplified stub that would produce
invalid PNG files.

### 5. Unused CPU Physics Engine

`src/physics_engine/physics.cpp` contains a complete RK4 integrator and
Christoffel symbol computation, but `main.cpp` never includes or calls it.
Particle updates in the render loop are purely kinematic (`stateX.z += dt`).

### 6. Debug Code in History

Commit `91eb879` ("bug fixed") bypassed the entire raymarching shader with:
```glsl
FragColor = texture(skybox, TexCoords);
return;
```
This was reverted in the next commit but remains in history.

### 7. `Tph_thph` Denominator Evolution

The `Tph_thph = cotθ` term has been through multiple formulations in git history:
- `1.0/tan(theta) + 1e-8`
- `1.0/(tan(theta) + 1e-8)`
- `1.0/(tan_theta)` (current — no epsilon, relies on theta clamp)

The current version has no epsilon, making it purely dependent on the theta
clamp for pole protection.

### 8. Window Title Mismatch

The window title says "Penrose: RK4 Black Hole" but the simulation uses CPU
timestep `dt` from `Constants.h` for camera movement (via `Window.cpp:12`),
not the shader's `dL`. Camera speed is tied to the physics timestep constant
rather than frame time.

---

## Building

Requires: C++20 compiler, CMake 3.22+, vcpkg with glfw3/glad/glm/eigen3.

```bash
cmake -B build -S . -DCMAKE_TOOLCHAIN_FILE=[vcpkg]/scripts/buildsystems/vcpkg.cmake
cmake --build build --config Debug
# Run: build/Debug/Penrose.exe  (on Windows)
```

### Controls
| Key | Action |
|---|---|
| WASD | Move camera |
| Arrow keys | Rotate camera |
| P | Toggle PPM frame capture |
| Escape | Exit |
