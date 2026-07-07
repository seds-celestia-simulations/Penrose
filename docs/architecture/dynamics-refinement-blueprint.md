# Penrose Dynamics Architecture Refinement Blueprint

## 1. Responsibility Analysis of `LegacyDynamics`

The current `LegacyDynamics::compute_acceleration(const State& state)` function acts as a monolithic container for all physics execution. An inspection of its implementation reveals that it simultaneously performs four distinct conceptual tasks:

1. **Metric Parameter Resolution**: It globally accesses `rs` (the Schwarzschild radius) from `Constants.h`.
2. **Christoffel Symbol Evaluation**: It computes the non-zero connection coefficients ($\Gamma^\mu_{\alpha\beta}$) specific to a static, spherically symmetric Schwarzschild black hole (e.g., `Tr_tt`, `Tt_tr`, `Tph_thph`).
3. **Equations of Motion (Geodesic Contraction)**: It manually contracts the computed connection coefficients with the state's 4-velocity vector to solve the geodesic equation ($a^\mu = -\Gamma^\mu_{\alpha\beta} U^\alpha U^\beta$).
4. **Numerical Safeguards / Horizon Handling**: It checks if the radial coordinate is inside the event horizon ($r \le r_s \times 1.001$) and artificially zeroes the acceleration to prevent numerical explosion.

These four responsibilities span across geometry, kinematics, and boundary constraints. They do not belong in a single function.

---

## 2. Universal vs. Metric-Specific Logic

To decouple the architecture, we must distinguish between logic that applies to *all* of General Relativity and logic that applies only to a *specific* spacetime.

### Universal (Shared by all GR simulations)
- **The Geodesic Equation**: The tensor contraction $a^\mu = -\Gamma^\mu_{\alpha\beta} U^\alpha U^\beta$ is universally true for freely falling test particles in any spacetime.
- **State Derivative Interface**: The mapping of an acceleration $a^\mu$ back to the $(X^\mu, U^\mu)$ state space required by the `RK4Integrator`.
- **Termination Policies**: The concept of halting or altering a simulation based on state boundaries (e.g., crossing an event horizon).

### Metric-Specific (Applicable to a particular spacetime)
- **Spacetime Parameters**: Mass ($M$), Spin ($a$), Charge ($Q$).
- **Christoffel Symbols ($\Gamma^\mu_{\alpha\beta}$)**: The actual mathematical definitions of spacetime curvature.
- **Horizon Definitions**: The location and shape of event horizons (e.g., Schwarzschild is a simple sphere $r = r_s$, whereas Kerr has an outer horizon $r_+ = M + \sqrt{M^2 - a^2}$ and an ergosphere).

---

## 3. Proposed Dynamics Layer Architecture

The dynamics layer will be decomposed into three distinct architectural concepts: **Geometry**, **Dynamics**, and **Boundary Conditions**.

### Concept A: `Metric` (Geometry)
- **Owns**: The geometric properties of a specific spacetime. It provides the Christoffel symbols for any given position $X^\mu$.
- **Depends on**: Mathematical tensor representations.
- **Must never depend on**: Particle state vectors $(X^\mu, U^\mu)$, velocities, integration logic, or horizon handling.

### Concept B: `GeodesicDynamics` (Dynamics Model)
- **Owns**: The universal equations of motion. It asks a `Metric` for the local geometry, and contracts it with a particle's velocity.
- **Depends on**: A `Metric` instance and a `State`.
- **Must never depend on**: Specific metric parameters (like $r_s$) or specific Christoffel formulas.

### Concept C: `Boundary Conditions / Termination Policy` (Constraints)
- **Owns**: The logic to determine if a physical state violates a simulation constraint (e.g., crossing an event horizon or hitting a numerical singularity).
- **Depends on**: State and specific threshold parameters (which may be provided by the metric or simulation).
- **Must never depend on**: Equations of motion or integration math.

---

## 4. Physics Integration Model

This architecture guarantees that adding new physics happens through **extension**, not modification.

### Scenario: Implementing the Schwarzschild Metric
- **Implemented Modules**: `SchwarzschildMetric` (provides Schwarzschild $\Gamma$) and a Schwarzschild-specific Termination Policy (checks $r \le r_s$).
- **Untouched Modules**: `GeodesicDynamics`, `RK4Integrator`.
- **Shared Abstractions**: Conforms to the implicit `Metric` concept.

### Scenario: Implementing the Kerr Metric (Spinning Black Hole)
- **Implemented Modules**: `KerrMetric` (provides Kerr $\Gamma$) and a Kerr-specific Termination Policy (checks $r \le r_+$).
- **Untouched Modules**: `GeodesicDynamics`, `RK4Integrator`.
- **Shared Abstractions**: Reuses the exact same contraction logic in `GeodesicDynamics`.

By extracting the `Metric` and `Termination Policy`, developers can focus entirely on transcribing mathematical formulas from a textbook into code, completely free of simulation boilerplate.

---

## 5. Migration Strategy

To evolve `LegacyDynamics` into this architecture without rewriting the validated mathematics, we will execute a surgical, step-by-step extraction:

1. **Extract Christoffel Formulas (In-Place)**
   - Create a `SchwarzschildMetric` struct in `src/geometry/`.
   - Move the exact formulas for `Tr_tt`, `Tt_tr`, etc., from `LegacyDynamics` into this struct.
   
2. **Extract Boundary Conditions**
   - Move the horizon check into a distinct `TerminationPolicy` block or class in the simulation loop.

3. **Extract Geodesic Contraction**
   - Create a new `GeodesicDynamics` class in `src/dynamics/`.
   - Pass it a `State` and a `SchwarzschildMetric`.
   - Implement the $a^\mu = -\Gamma^\mu_{\alpha\beta} U^\alpha U^\beta$ contraction inside `GeodesicDynamics` using the outputs from the Metric.

4. **Redirect and Validate**
   - Update the benchmarks to construct a `SchwarzschildMetric`, bind it to `GeodesicDynamics`, and wrap it in the `TerminationPolicy`.
   - Verify that the CSV outputs remain bit-for-bit identical to the legacy implementation.

5. **Delete Legacy Code**
   - Remove `LegacyDynamics.h` and `LegacyDynamics.cpp`.
   - Remove global `Constants.h` (moving $r_s$ into the `SchwarzschildMetric` constructor).

---

## 6. Refined Repository Organization

After the migration, the architecture will map directly to physical concepts:

```text
src/
├── core/
│   └── State                   (Universal Phase Space)
├── geometry/
│   ├── CoordinateChart         (Mappings)
│   ├── Metric                  (Base Concept)
│   └── SchwarzschildMetric     (Specific Geometry)
├── dynamics/
│   ├── DynamicsModel           (Base Concept)
│   └── GeodesicDynamics        (Universal Motion)
└── integration/
    └── RK4Integrator           (Numerical Solver)
```

---

## 7. Architectural Rationale

This decomposition perfectly aligns the software architecture with the mathematics of General Relativity. 

By separating the **Metric** (geometry) from the **Termination Policy** (simulation constraints), we ensure that geometric models remain mathematically pure. A Metric should solely define curvature, while constraints (like event horizon clipping) belong higher up the chain in the simulation loop.

Furthermore, splitting `LegacyDynamics` into `Metric` and `GeodesicDynamics` maps foundational physics directly onto the C++ file structure. This approach minimizes duplicated logic. If we left `LegacyDynamics` as-is, a future `KerrDynamics` would be forced to completely duplicate the $a^\mu = -\Gamma^\mu_{\alpha\beta} U^\alpha U^\beta$ contraction. Under the proposed architecture, that logic is written exactly once, permanently solving particle motion for any arbitrary geometry.
