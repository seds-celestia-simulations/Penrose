# Penrose Architecture Design Specification (Refined)

## 1. Revised Framework Identity

Penrose is fundamentally a **relativistic ray evolution framework**. It is not primarily a renderer, nor is it a monolithic black hole visualizer. Its core scientific competency is integrating trajectory states through arbitrary curved spacetimes. 

Image generation (rendering) is merely one possible consumer of this evolution pipeline, alongside numerical benchmarking and invariant validation. By redefining the framework around ray evolution rather than pixel color, Penrose establishes an architecture capable of supporting both real-time interactive exploration and high-fidelity offline scientific research.

## 2. Revised Fundamental Concepts

To build a minimum viable architecture that supports the next phase of development (such as Kerr spacetime), the following concepts represent the absolute minimum necessary abstractions. Speculative features (such as volumetric media and complex spectral observables) have been intentionally omitted from this core.

### Metric
- **Purpose**: Defines the intrinsic geometry of spacetime.
- **Responsibilities**: Provides the metric tensor components ($g_{\mu\nu}$), the inverse metric, and the connection coefficients (Christoffel symbols, $\Gamma^\mu_{\alpha\beta}$) for any valid spacetime event.
- **Owned Data**: Physical parameters characterizing the spacetime (e.g., mass $M$, spin $a$).
- **Lifetime**: Bound to the simulation context as the ground truth of gravity.

### Coordinate Chart
- **Purpose**: Maps abstract spacetime manifold events to numerical coordinate vectors.
- **Responsibilities**: Provides coordinate transformations, Jacobians, and actively identifies coordinate singularities (e.g., poles or coordinate horizons).
- **Owned Data**: Coordinate domain boundaries.
- **Lifetime**: Bound to the simulation context, often paired closely with a specific Metric.

### Geodesic Dynamics
- **Purpose**: Evaluates the equations of motion for a given state.
- **Responsibilities**: Computes the state derivative (the right-hand side of the geodesic equation) by querying the Metric's connection and enforcing physical constraints.
- **Owned Data**: Stateless.
- **Lifetime**: Evaluates purely functionally during the integration step.

### Trajectory State (Ray)
- **Purpose**: Represents a point particle or photon traversing spacetime.
- **Responsibilities**: Tracks the evolution of spacetime position ($x^\mu$) and tangent four-momentum ($u^\mu$) along an affine parameter.
- **Owned Data**: Spacetime position, tangent, and trajectory type.
- **Invariants**: The norm of the four-momentum ($g_{\mu\nu}u^\mu u^\nu$) remains mathematically constant (0 for null rays).

### Integrator
- **Purpose**: Advances the Trajectory State step-by-step through the Coordinate Chart.
- **Responsibilities**: Manages numerical stepping (e.g., fixed RK4), error estimation, and bounds checking against the Coordinate Chart.
- **Owned Data**: Step sizes and numerical state buffers.

### Observer
- **Purpose**: Defines the relativistic frame of reference for initialization.
- **Responsibilities**: Generates initial Trajectory States from a local tetrad (local inertial frame) based on virtual sensor geometry.
- **Owned Data**: Four-position, four-velocity, and spatial orientation basis vectors.

### Scene
- **Purpose**: Represents non-gravitational physical entities within the spacetime.
- **Responsibilities**: Answers intersection queries along trajectories (e.g., crossing the accretion disk plane) and provides physical termination states (e.g., hitting a surface or escaping to a skybox).
- **Owned Data**: Geometric boundaries.

### Execution Backend
- **Purpose**: Orchestrates the hardware execution and parallelization of the simulation.
- **Responsibilities**: Manages memory, parallelizes ray evolution (GPU threads, CPU threads), and formats final output buffers.
- **Owned Data**: Device buffers and framebuffers.

## 3. Foundation vs Future Layers

The architecture explicitly segregates what must be built now from what may be built years from now.

### Foundation (Minimum Viable Architecture)
Required before any meaningful refactor is considered complete.
- `Metric`, `Coordinate Chart`, `Geodesic Dynamics`
- `State`, `Integrator`
- `Observer`, `Scene`, `Simulation Pipeline`
- `Execution Backend`

### First Expansion
Required for supporting multiple metrics.
- Additional `Metric` and `Coordinate Chart` modules (e.g., `KerrMetric`, `BoyerLindquistChart`). 
- **Note**: The Foundation layer fully supports this expansion without requiring architectural modifications.

### Advanced Framework (Future Work)
Only needed for sophisticated future capabilities. Do not allow these to complicate the Foundation.
- `Media`: For integrating through volumetric density fields (GRMHD).
- `Observable (Radiance)`: For carrying full radiative transfer payloads (polarization, Doppler shifts, spectra) instead of simple RGB/hit results.
- `Solar Gravitational Lens Geometry`: Extremely specialized observer-source geometries requiring high-precision double-float backends.

## 4. Revised Ownership Model

Ownership boundaries enforce low coupling and high cohesion:

- **Spacetime Geometry**: Owned by the **Metric** subsystem.
- **Coordinate Limits & Jacobians**: Owned by the **Coordinate Chart**.
- **Equations of Motion**: Owned by **Geodesic Dynamics**.
- **Ray State Progression**: Owned by the **Ray Evolution Pipeline**. The ray is a payload; it does not own its own evolution.
- **Numerical Math**: Owned by the **Integrator** subsystem, entirely decoupled from physics.
- **Scene Representation**: Owned by the **Scene** subsystem.
- **Ray Initialization**: Owned by the **Observer**.
- **Hardware Orchestration**: Owned by the **Host Application** and **Execution Backend**.

## 5. Revised Dependency Graph

Dependencies flow downward from the backend to abstract interfaces.

1. **Host Application** $\rightarrow$ Execution Backend, Observer, Scene, Metric, Coordinate Chart
2. **Execution Backend** $\rightarrow$ Ray Pipeline (Drives the loop)
3. **Ray Pipeline** $\rightarrow$ Observer (Init), Integrator (Step), Scene (Hit)
4. **Integrator** $\rightarrow$ Geodesic Dynamics (Get derivative), Coordinate Chart (Check bounds)
5. **Geodesic Dynamics** $\rightarrow$ Metric (Get connection)

*Crucial Architecture Check*: The Execution Backend (e.g., an OpenGL shader) does not depend on the Metric or the Geodesic Dynamics. The Backend executes the pipeline; the pipeline delegates to the math. 

## 6. Revised Migration Strategy

Instead of immediately attempting to extract physics out of the GLSL shaders—which risks destabilizing the live visualizer before the new architecture is proven—the migration strategy prioritizes understanding and architectural stability:

1. **Define CPU Interfaces First**: Establish the `Metric`, `Coordinate Chart`, `Geodesic Dynamics`, `Integrator`, and `State` contracts in the C++ CPU physics engine.
2. **Validate CPU Boundaries**: Migrate the existing `orbital.cpp` and `null_geodesic.cpp` benchmarks to use these new abstractions. Prove that an abstract RK4 Integrator can step an abstract Ray using the Schwarzschild Metric interface.
3. **Draft the Scene and Observer CPU Models**: Replace Euclidean camera logic in the CPU tests with relativistic Observers, and implement the disk intersection as a CPU Scene.
4. **Backend Extraction**: Only *after* the C++ abstractions are proven stable and correct, adapt the GPU shaders to match this established architecture. Architecture precedes execution.

## 7. Validation Against Schwarzschild and Kerr

If this architecture is correct, adding Kerr should not require changing the framework core.

### Schwarzschild Implementation
- **Modules Implemented**: `SchwarzschildMetric`, `SchwarzschildChart`, `StandardGeodesicDynamics`, `RK4Integrator`, `StandardObserver`, `EquatorialDiskScene`.
- **Untouched**: The `Simulation Pipeline` and `Execution Backend`.

### Kerr Implementation
- **Modules Implemented**: `KerrMetric` (provides Kerr Christoffel symbols), `BoyerLindquistChart` (provides Kerr coordinate bounds).
- **Untouched**: The `Integrator`, `Observer`, `Scene`, `Simulation Pipeline`, and `Execution Backend`.
- **Architectural Change Required**: **None.** The Integrator simply asks the `Geodesic Dynamics` for the derivative, which transparently asks the new `KerrMetric` for the connection. The architecture fully supports this extension out-of-the-box.

## 8. Summary of Changes from the Previous Specification
1. **Refocused Identity**: Shifted from a "scientific rendering framework" to a "relativistic ray evolution framework," properly centering the physics pipeline.
2. **Pruned Concepts**: Removed `Media` and `Observable (Radiance)`. These are speculative, advanced features that would over-engineer the Foundation.
3. **Retained Modularity**: Kept `Metric`, `Coordinate Chart`, and `Geodesic Dynamics` as distinct modules per explicit architectural necessity, ensuring mathematical clarity.
4. **Revised Migration**: Flipped the execution order. CPU contracts will be defined and validated before touching the fragile GLSL execution backend.

## 9. Conclusion

> *If the framework were frozen immediately after implementing only the Foundation layer, would it already possess the correct architectural shape for future growth?*

**Yes.** By retaining the modularity of the `Metric`, `Coordinate Chart`, and `Geodesic Dynamics`, the Foundation mathematically models the exact tenets of differential geometry. General Relativity guarantees that free fall is entirely described by the geodesic equation (Dynamics) given a connection (Metric) mapped to coordinates (Chart). 

Because the Integrator only depends on Dynamics, and the Pipeline only depends on the Integrator, this structural shape is mathematically universal. A future update requiring charged particles (Lorentz force) simply swaps the `Geodesic Dynamics` module for an `Electrodynamics` module without touching the `Integrator` or `Metric`. A future update requiring Kerr simply swaps the `Metric` without touching the `Integrator` or `Dynamics`. Therefore, the minimal Foundation is not a dead end; it is the correct, permanent shape of the framework.
