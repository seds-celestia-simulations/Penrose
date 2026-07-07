# Penrose Architecture Refinement — Post-Refactor Review

## 1. Executive Summary
The initial refactor successfully dismantled the `physics_engine` monolith, extracting a loosely coupled framework of Core, Spacetime, Dynamics, and Integration modules. Crucially, the mathematical foundation and numerical outputs (verified by bit-for-bit identical benchmarks) have remained pristine. 

However, this review reveals that while the **files** have been separated, some **concepts** remain conflated. The most significant unresolved architectural friction lies in the `LegacyDynamics` module (which currently intertwines metric definitions with the geodesic equation) and the `Core` module (which pollutes phase-space states with rendering attributes). 

**Verdict**: The current architecture is **not** mature enough to implement new physics (like a Kerr metric). A targeted refinement pass is required to properly isolate the `Metric` concept from `Dynamics` before new features are added.

---

## 2. Current Architectural Assessment
The architecture has successfully achieved **Dependency Inversion** regarding numerical integration. By passing a `DerivativeFunc` lambda to the RK4 Integrator, the solver is now completely ignorant of General Relativity. This is a massive victory.

However, the architecture has not yet achieved **Domain Isolation** within the physics modules themselves. Spacetime geometry (Metrics) and particle motion (Geodesics) are still tightly coupled within the same functions.

---

## 3. Subsystem Review

### `Core` (`src/core`)
- **Owns**: The definition of a physical state $(X^\mu, U^\mu)$.
- **Should never own**: Rendering properties or entity lifetimes.
- **Friction**: The `State.h` file defines `Particle` and `Light` structs which contain `float mass` and `Vector3f color`. `color` is a rendering concern and constitutes an architectural leak from the visualizer into the mathematical core.

### `Integration` (`src/integration`)
- **Owns**: Numerical methods for advancing ODEs.
- **Should never own**: Physics equations, metrics, or coordinate transforms.
- **Status**: Perfectly defined. The `RK4Integrator` is agnostic to the physics being solved.

### `Dynamics` (`src/dynamics`)
- **Owns**: The equations of motion (how a state changes given a geometry).
- **Should never own**: The definition of the geometry (the Metric) itself.
- **Friction**: `LegacyDynamics.cpp` hardcodes the Schwarzschild Christoffel symbols directly into the acceleration calculation. It owns both "what is the geometry" and "how do things move in it". 

### `Spacetime` (`src/spacetime`)
- **Owns**: Coordinate charts and transformations.
- **Should never own**: State evolution.
- **Friction**: Currently only owns Jacobians. Ideally, this subsystem should own the concept of Spacetime Metrics.

---

## 4. Abstraction Review

- **`State`**: A genuine domain concept (Phase Space). Needs refinement to strip out rendering data.
- **`LegacyDynamics`**: A transitional abstraction. It serves as a quarantine zone for the old `find_acceleration` function and must be split.
- **`RK4Integrator`**: A genuine, long-term domain concept.
- **`CoordinateChart`**: A genuine domain concept, but implemented as a procedural namespace. It should eventually evolve into a formal interface if multiple coordinate systems (e.g., Boyer-Lindquist) are introduced.

---

## 5. Responsibility Analysis
The most egregious violation of the Single Responsibility Principle is `LegacyDynamics::compute_acceleration(const State& state)`. 

Mathematically, the geodesic equation is:
$$ a^\mu = -\Gamma^\mu_{\alpha\beta} U^\alpha U^\beta $$

Currently, `compute_acceleration` manually calculates $\Gamma^\mu_{\alpha\beta}$ (the Christoffel symbols for a Schwarzschild black hole) AND performs the contraction with the velocity vector $U$ in a single block of code. The architectural boundary between **Metric** (which provides $\Gamma$) and **Dynamics** (which applies the Geodesic Equation) remains entirely implicit.

---

## 6. Dependency Review
- **Current Direction**: `Benchmark -> Dynamics`, `Benchmark -> Integration`. `Integration` knows nothing about `Dynamics`.
- **Unnecessary Dependencies**: `LegacyDynamics` depends globally on `src/Constants.h` for `rs` (the Schwarzschild radius). Constants should be encapsulated inside a specific Metric instantiation, not globally floated.

---

## 7. Naming & Repository Organization Review
- **Folders**: Communicate responsibility well (`core`, `dynamics`, `integration`, `spacetime`).
- **`LegacyDynamics`**: The name indicates implementation history rather than scientific responsibility. It should not survive the next refinement pass.
- **`Constants.h`**: A global dumping ground. Needs to be removed in favor of parameterized metric classes.
- **Browsability**: A new contributor looking for the "Schwarzschild Metric" will not find it in `spacetime/` where it belongs, but hidden inside `dynamics/LegacyDynamics.cpp`. 

---

## 8. CPU / GPU Alignment Review
- **Alignment**: Poor.
- **Explanation**: The CPU pipeline uses exact RK4 integration over a procedurally defined metric. The GPU pipeline (in `shaders/`) relies on GLSL raymarching that duplicates relativistic logic. 
- **Risk**: If a developer adds a `KerrMetric` to the C++ architecture, they must manually rewrite the equivalent GPU shader logic from scratch. While unifying them is out of scope for a C++ refactor, the conceptual inconsistency means "Penrose" currently acts as two separate simulation engines.

---

## 9. Future Extension Evaluation

### Extension A: Adding a new Integrator (e.g., RK45)
- **Impact**: Flawless. We simply drop `RK45Integrator.h` into `integration/` and pass the existing Lambda. No other modules are touched. The architecture shines here.

### Extension B: Adding the Kerr Metric
- **Impact**: High Friction. Because the geodesic equation is hardcoded alongside the Schwarzschild symbols in `LegacyDynamics`, a developer would have to write a completely new `compute_kerr_acceleration` function from scratch, duplicating the contraction math. 
- **Conclusion**: The architecture fails the Metric extension test.

---

## 10. Architectural Debt

| Issue | Severity | Classification | Description |
| :--- | :--- | :--- | :--- |
| **Global Constants** | Minor | Encapsulation | `Constants.h` is a global state. Should wait to be fixed during Metric extraction. |
| **UI Leakage in Core** | Moderate | Separation of Concerns | `State.h` contains `color`. Should be fixed by separating `Particle` (Render) from `State` (Math). |
| **Metric/Dynamics Conflation** | Major | Missing Abstraction | `LegacyDynamics` mixes Geometry and Equations of Motion. Must be fixed before any new physics are added. |
| **CPU/GPU Duplication** | Major | Code Duplication | Shaders and C++ do not share physical definitions. Should intentionally wait until later (requires heavy graphics architecture work). |

---

## 11. Prioritized Refinement Roadmap

To prepare the framework for new physics, the following small, isolated refinements must be executed:

### Phase 1: Strip UI Leakage from Core
- **Task**: Remove `color` and rendering concepts from `State.h` (e.g., into `render/Particle.h`).
- **Reasoning**: Ensures `core/` remains a pure mathematical representation.

### Phase 2: Encapsulate Constants
- **Task**: Remove global `Constants.h`. Pass the Schwarzschild radius (`rs`) as a parameter or state variable into the physics models.
- **Reasoning**: Prepares the system for metrics with different parameters (like spin $a$ for Kerr).

### Phase 3: Extract the Metric Interface
- **Task**: Create `spacetime/Metric.h` defining a standard way to retrieve Christoffel symbols.
- **Task**: Extract the $\Gamma$ calculations from `LegacyDynamics` into a `SchwarzschildMetric` implementation.

### Phase 4: Generalize Dynamics
- **Task**: Create `dynamics/GeodesicEquation.h` that accepts a `State` and a `Metric`, performing the $a^\mu = -\Gamma^\mu_{\alpha\beta} U^\alpha U^\beta$ contraction.
- **Task**: Delete `LegacyDynamics`.

---

## 12. Final Assessment

> **Is the current architecture mature enough to begin implementing new physics (e.g. Kerr)?**

**No.** Another refinement pass must be completed first. 

While the extraction of the Integrator is a massive success, the `LegacyDynamics` module is currently a bottleneck. It hardcodes Schwarzschild geometry directly into the equations of motion. Attempting to add Kerr physics right now would result in duplicating the geodesic contraction logic, leading to divergent code paths. 

By executing the proposed Refinement Roadmap, we will establish a formal `Metric` abstraction. Once `Metric` and `GeodesicEquation` are separated, adding the Kerr metric will be as trivial as adding a new Integrator.
