# Penrose System Design & Implementation Blueprint

This authoritative blueprint dictates the architectural migration of Penrose from its current monolithic implementation into a modular scientific computing framework. 

The core philosophy of this document is **preservation**. The current implementation contains years of validated scientific mathematics and rendering logic. The architecture must be *extracted* from these working implementations, rather than replacing them. We will isolate, move, and wrap existing logic. Rewriting validated code is forbidden unless architecturally unavoidable.

---

# Part I — System Design Specification

## 1. Framework Responsibilities

The framework is partitioned into orthogonal subsystems based entirely on the physics of general relativistic ray evolution.

- **Metric**: Owns the intrinsic spacetime geometry. Computes metric tensors ($g_{\mu\nu}$) and Christoffel symbols ($\Gamma^\mu_{\alpha\beta}$) for any valid event.
- **Coordinate**: Owns domain limits, Jacobians, and singularity detection. Prevents numerical evolution into undefined coordinate regions.
- **Dynamics**: Owns the equations of motion. Evaluates accelerations (e.g., the geodesic equation) by querying the Metric.
- **State**: The universal data payload. Represents the four-position and four-momentum of a ray or particle.
- **Integration**: Owns the numerical stepping algorithms (e.g., RK4). Knows nothing of gravity; only knows how to step a State given a derivative from Dynamics.
- **Observer**: Owns the local tetrad frame. Generates initial States based on virtual sensor geometry.
- **Scene**: Owns non-gravitational entities. Evaluates geometric intersections and material interactions (e.g., accretion disks, skyboxes) at queried spacetime coordinates.
- **Simulation**: Orchestrates the Ray Evolution Pipeline. Loops the Integrator and checks against the Scene and Coordinate limits.
- **Backend**: Hardware execution (CPU threads, GPU compute). Marshals the Simulation pipeline but never defines the physics.

## 2. Ownership Model

Strict ownership guarantees a cohesive architecture:
- **Geometry** is owned exclusively by the `Metric`.
- **Numerical Stepping** is owned exclusively by `Integration`.
- **Ray State** is a transient payload. It is owned by the `Backend` worker thread, modified by the `Integration`, and read by the `Scene`.
- **Ray Initialization** is owned by the `Observer`.
- **Rendering Logic** is owned by the `Scene` and `Backend`. It must never own or dictate physical evolution.

## 3. Data Flow

1. The **Backend** worker queries the **Observer** for an initial **State**.
2. The Backend initiates the **Simulation** loop.
3. The **Simulation** passes the **State** to the **Integration**.
4. The **Integration** queries **Dynamics** for a derivative.
5. **Dynamics** queries the **Metric** for connection coefficients.
6. The **Integration** updates the **State** and queries the **Coordinate** system for boundary violations.
7. The **Simulation** passes the updated **State** to the **Scene** to test for collisions.
8. If a collision occurs, the **Scene** returns an observable payload to the **Backend**, terminating the loop.

## 4. Communication Paths

Subsystems communicate exclusively through clearly defined conceptual contracts. We do not prescribe specific C++ mechanisms (e.g., virtual inheritance vs. templates) prematurely; the requirement is strict boundary enforcement.

- **Integration $\leftrightarrow$ Dynamics**: Integration requests a state derivative. Dynamics returns $[u^\mu, a^\mu]$.
- **Dynamics $\leftrightarrow$ Metric**: Dynamics requests a connection. Metric returns a mathematical array of symbols.
- **Simulation $\leftrightarrow$ Scene**: Simulation provides a State path segment. Scene returns a boolean intersection and an optional color/physical payload.

## 5. Dependency Graph

Dependencies must form a strict Directed Acyclic Graph (DAG):
- `Backend` $\rightarrow$ `Simulation`, `Observer`, `Scene`
- `Simulation` $\rightarrow$ `Integration`, `Scene`, `Coordinate`
- `Integration` $\rightarrow$ `Dynamics`, `Coordinate`, `State`
- `Dynamics` $\rightarrow$ `Metric`, `State`
- `Observer`, `Scene`, `Metric`, `Coordinate` $\rightarrow$ independent (they only depend on core mathematical utilities and `State`).

## 6. Architectural Invariants

These permanent rules govern all future contributions:
1. **Metrics own geometry.** Only the metric evaluates Christoffel symbols.
2. **Dynamics own equations of motion.** Only dynamics evaluates accelerations.
3. **Integrators own numerical stepping.** Integrators never know about physical constants like $r_s$ or metrics.
4. **Rendering never owns physics.** Scene logic only queries coordinates; it never alters trajectory mathematics.
5. **Backends execute physics but do not define it.** 
6. **State is the universal data.** Subsystems communicate by exchanging trajectory states.

---

# Part II — Implementation Blueprint

## 1. Source File Analysis & Responsibility Decomposition

Before moving code, we must understand the responsibilities mixed within existing files.

### `src/physics_engine/physics.cpp`
- `find_acceleration(const State& state)`
  - **Geometry**: Hardcodes Schwarzschild Christoffel symbols (`rs`).
  - **Dynamics**: Computes the 4-acceleration $a^\mu$.
  - **Coordinate Handling**: Enforces a coordinate boundary (`r <= rs * 1.001`).
- `Integrator(const State& s)`
  - **Numerical Stepping**: Pure 4th-order Runge-Kutta math.
- `cart_to_sphere`, `sph_to_cart_Jacobian`
  - **Coordinate Handling**: Pure geometric mapping.

### `shaders/reduced.frag`
- `orbitDerivative(float u, float v)`
  - **Geometry & Dynamics**: Calculates the geodesic derivative for the reduced 2D plane ($1.5 r_s u^2$).
- `orbitRK4()`
  - **Numerical Stepping**: Integrates the reduced $u, v$ variables.
- `raymarchReduced()`
  - **Simulation**: The `for (int i = 0; i < MAX_ORBIT_STEPS; ++i)` loop.
  - **Scene**: The plane-crossing test (`(z0 * z1) <= 0.0`) and color grading.
  - **Coordinate Handling**: Horizon capture and escape limit checks.
- `main()`
  - **Observer**: NDC-to-world un-projection.

## 2. Source File Migration Map

| Existing Component | Current Role | Conceptual Target | Action Strategy |
| :--- | :--- | :--- | :--- |
| `physics.cpp::find_acceleration` | Hardcoded Schwarzschild math | `Metric` + `Dynamics` | **Isolate & Split** into discrete metric and dynamics blocks. |
| `physics.cpp::Integrator` | RK4 integration loop | `Integration` | **Isolate** into a generic integration wrapper. |
| `physics.cpp::cart_to_sphere` | Cartesian/Spherical mapping | `Coordinate` | **Move** to coordinate utilities. |
| `reduced.frag::raymarchReduced` | Monolithic simulation pipeline | `Simulation`, `Scene` | **Wrap** the disk-crossing logic into a standalone GLSL `Scene` function. |
| `reduced.frag::main` | Ray generation | `Observer` | **Wrap** NDC logic into an `Observer` function. |
| `src/render/Camera.h` | Euclidean viewing frustum | `Observer` | **Generalize** to generate relativistic states. |

## 3. Repository Organization

The repository hierarchy must communicate the architecture immediately.

### Proposed Directory Structure
- `src/`
  - `spacetime/`: **Responsibility:** Geometry and domains. **Owns:** Metrics, Coordinate charts. **Forbidden:** Dynamics, rendering.
  - `dynamics/`: **Responsibility:** Equations of motion. **Owns:** Geodesic evaluations. **Forbidden:** Numerical integration logic.
  - `integration/`: **Responsibility:** Numerical solvers. **Owns:** RK4, RKF45. **Forbidden:** Physics, metrics.
  - `scene/`: **Responsibility:** Spatial environments. **Owns:** Accretion disks, volumes. **Forbidden:** Core trajectory mutation.
  - `observer/`: **Responsibility:** Sensors and initial states. **Owns:** Cameras, tetrads. **Forbidden:** Scene rendering.
  - `simulation/`: **Responsibility:** Orchestration. **Owns:** The ray pipeline loops. **Forbidden:** Hardware driver code.
  - `backend/`: **Responsibility:** Hardware execution. **Owns:** Main loops, CPU threads, OpenGL setup. **Forbidden:** Physics definitions.
  - `core/`: **Responsibility:** Fundamental structures. **Owns:** Math utilities, `State` definition.

### File Naming Strategy
Filenames must reflect architectural concepts rather than vague categories.
- Avoid: `physics.cpp`, `utils.cpp`, `engine.cpp`
- Prefer: `SchwarzschildMetric.cpp`, `RK4Integrator.cpp`, `BoyerLindquistChart.cpp`, `StandardObserver.cpp`.

## 4. Migration Phases

The migration is organized into safe, incremental refactors. The repository must compile and function correctly at every step.

### Phase 1: In-Place Isolation (CPU)
**Action**: Without creating new files, restructure the contents of `physics.cpp` into explicit namespaces/structs representing the architecture (e.g., `namespace Integration { State stepRK4(...) }`). 
**Success Criteria**: 
- Responsibilities are conceptually separated within the file.
- `find_acceleration` logic is split into a discrete Metric call and a Dynamics computation.
- Existing benchmarks compile and run.
- Numerical outputs match the reference implementation identically.

### Phase 2: File Movement & Extraction (CPU)
**Action**: Create the new repository folder hierarchy (`src/spacetime`, `src/integration`, etc.). Move the isolated blocks from `physics.cpp` into their appropriately named new files (e.g., `SchwarzschildMetric.cpp`, `RK4Integrator.cpp`). Update benchmark includes.
**Success Criteria**:
- Subsystem boundaries are enforced by file inclusion paths.
- The `physics_engine` directory is emptied of core math.
- Compilation succeeds and numerical output remains identically stable.

### Phase 3: GLSL Internal Isolation (GPU)
**Action**: Without moving GLSL files, refactor `reduced.frag`. Wrap the $z=0$ crossing logic into a distinct `intersectScene(vec3 prev, vec3 curr)` function. Wrap the orbit math into `Dynamics` and `Integration` functions. 
**Success Criteria**:
- The monolithic `raymarchReduced` loop is simplified into a sequence of abstract function calls.
- The visualizer compiles and renders a pixel-perfect match to the legacy version.
- Frame rates remain identical (no optimizations lost).

### Phase 4: Observer Generalization
**Action**: Extend `src/render/Camera.h` into the new `observer/` directory, generalizing its API to return `State` objects rather than Euclidean view matrices.
**Success Criteria**:
- The CPU benchmarks and GPU backends can both construct rays using the exact same Observer logic.

## 5. Final Cleanup Phase
**Action**: 
- Delete the legacy `physics.cpp` and `physics.h` files.
- Remove the empty `src/physics_engine/` directory.
- Strip any legacy compatibility wrappers used during the migration.
**Success Criteria**:
- The repository perfectly mirrors the new architecture.
- No legacy physics files remain.
- The project builds cleanly from scratch.

## 6. Rewrite Justification Analysis

**Are there any rewrites scheduled in this blueprint?**
**No.** 
- The RK4 arithmetic is perfectly preserved.
- The Schwarzschild Christoffel evaluation is perfectly preserved.
- The GLSL disk intersection test (`z0 * z1 <= 0.0`) is perfectly preserved.

*Conclusion*: Rewriting is entirely unnecessary. The architectural goals can be fully met through isolation, wrapping, and physical relocation of existing logic.

## 7. Risk Analysis

- **Mathematical Regression Risk**: Near zero. Because no algorithms are being rewritten, the numerical behavior is guaranteed to remain static.
- **Architectural Coupling Risk**: If interfaces are poorly designed in Phase 1, dependencies may leak. *Mitigation*: Strictly enforce the Architectural Invariants during the isolation step.
- **GPU Performance Risk**: Extracting GLSL functions may defeat compiler optimizations if not careful. *Mitigation*: Ensure the GPU math relies on the identical 1D reduced state parameterization it uses today; only wrap the logic conceptually, do not alter the data structures the GPU relies on.
