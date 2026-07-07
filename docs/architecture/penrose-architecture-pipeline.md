# Penrose Architecture Data Flow & Pipelines

This document expands on the Penrose Architecture Design Specification, detailing the data flow, module dependencies, and exact operational pipelines of the refined framework. It also explicitly contrasts this new architecture with the legacy implementation.

## 1. Data Flow Through the Architecture

Data in the new Penrose framework flows as a continuous pipeline of abstract state, driven by the Execution Backend but mathematically governed by the Spacetime modules.

1. **Initialization (Observer $\rightarrow$ Ray Pipeline)**: 
   - The Execution Backend (e.g., a GPU compute shader or CPU thread pool) queries the **Observer** for a given screen pixel or initial condition.
   - The Observer uses its local tetrad to output a **Trajectory State** (a four-position $x^\mu$ and four-momentum $u^\mu$).
2. **Evolution Loop (Ray Pipeline $\rightarrow$ Integrator $\rightarrow$ Scene)**:
   - The Ray Pipeline passes the Trajectory State to the **Integrator**.
   - The Integrator asks **Geodesic Dynamics** for the state derivative $\frac{d\mathbf{S}}{d\lambda}$.
   - **Geodesic Dynamics** queries the **Metric** for the connection $\Gamma^\mu_{\alpha\beta}$ at position $x^\mu$.
   - The Integrator uses numerical methods (e.g., RK4) to advance the State by a step $\Delta\lambda$.
   - The Integrator queries the **Coordinate Chart** to ensure the new state hasn't crossed a singularity or boundary.
   - The Ray Pipeline takes the updated State and queries the **Scene**.
3. **Termination & Output (Scene $\rightarrow$ Backend)**:
   - If the Scene reports an intersection (e.g., crossing an accretion disk or horizon), it returns a physical payload (e.g., color, temperature).
   - The Ray Pipeline terminates the evolution loop and returns the payload to the **Execution Backend**.
   - The Execution Backend writes the payload to the final memory buffer (e.g., a framebuffer).

## 2. Dependencies of Each Module

The dependency graph strictly enforces Dependency Inversion. 

- **Execution Backend**: Depends on `Ray Pipeline`, `Observer`, `Scene`. (Does *not* depend on `Metric` or `Integrator`).
- **Ray Pipeline**: Depends on `Observer`, `Integrator`, `Scene`.
- **Integrator**: Depends on `Trajectory State`, `Geodesic Dynamics`, `Coordinate Chart`.
- **Observer**: Depends on `Trajectory State`, `Coordinate Chart`.
- **Scene**: Depends on `Trajectory State`, `Coordinate Chart`.
- **Geodesic Dynamics**: Depends on `Trajectory State`, `Metric`.
- **Metric**: Independent. Computes tensor components from coordinates.
- **Coordinate Chart**: Independent. Defines numerical limits.
- **Trajectory State**: Independent. Pure data structure ($x^\mu, u^\mu$).

## 3. Differences From Current Architecture

The new architecture drastically shifts ownership away from execution files and into discrete, testable physics modules.

| Feature | Legacy Implementation | Refined Architecture | Exact File Impacted |
| :--- | :--- | :--- | :--- |
| **Physics Location** | Hardcoded inside GLSL fragment shaders (`shaders/reduced.frag`). | Backend-agnostic `Geodesic Dynamics` and `Metric` interfaces. | `shaders/reduced.frag` and `shaders/quad.frag` lose all physics math. |
| **Integrator Location** | Written procedurally in `reduced.frag` and `physics.cpp`. | Pluggable `Integrator` module queried by a `Ray Pipeline`. | `src/physics_engine/physics.cpp` RK4 logic is isolated into a standalone class. |
| **Scene/Disk Logic** | Hardcoded `$z=0$` plane crossing inside the shader's raymarch loop. | Pluggable `Scene` module queried independently of the integration step. | Shader `raymarch` loops are stripped of `if(hit_disk)` checks; delegated to a Scene interface. |
| **Camera Model** | Euclidean `glm::lookAt` inverse projection matrix in `Camera.h` and GLSL. | Relativistic `Observer` using local tetrads to generate valid four-momentum. | `src/render/Camera.h` is replaced/augmented by a relativistic `Observer` subsystem. |
| **Metric Independence** | Only Schwarzschild exists; equations scattered. | `Metric` is an explicit abstraction providing $\Gamma^\mu_{\alpha\beta}$. | `src/physics_engine/physics.cpp` `find_acceleration` is replaced by a `Metric` query. |
| **Code Unification** | GPU and CPU take completely different mathematical paths. | CPU and GPU execute the exact same abstract pipeline. | Benchmarks (`src/benchmarking/*`) and `Renderer` consume identical abstract interfaces. |

## 4. Detailed Physics Pipeline

The physics pipeline represents the mathematically immutable core of the simulation. It occurs strictly *inside* the Ray Pipeline's evolution loop and is entirely backend-agnostic.

1. **State Injection**: An initial `Trajectory State` $S = [x^\mu, u^\mu]$ is passed to the `Integrator`.
2. **Derivative Request**: The `Integrator` begins a stepping algorithm (e.g., RK4) and evaluates intermediate slopes $k_1, k_2, k_3, k_4$. For each evaluation, it passes the trial state to `Geodesic Dynamics`.
3. **Metric Evaluation**: `Geodesic Dynamics` reads $x^\mu$ from the trial state and passes it to the `Metric`. The `Metric` returns the Christoffel symbols $\Gamma^\mu_{\alpha\beta}$ evaluated precisely at $x^\mu$.
4. **Acceleration Computation**: `Geodesic Dynamics` computes the four-acceleration $a^\mu = -\Gamma^\mu_{\alpha\beta} u^\alpha u^\beta$ and returns the full derivative $dS = [u^\mu, a^\mu]$ to the `Integrator`.
5. **Chart Validation**: After summing the RK4 slopes to produce the next state $S_{n+1}$, the `Integrator` passes $S_{n+1}$ to the `Coordinate Chart`. The Chart checks if $r < r_{\text{horizon}}$ or if a polar singularity $\theta \to 0, \pi$ was crossed.
6. **State Update**: If valid (or corrected by the Chart), the `Integrator` overwrites the `Trajectory State` with $S_{n+1}$.

## 5. Detailed Visualization Pipeline

The visualization pipeline defines how the Execution Backend marshals the physics pipeline to produce an observable image on a screen or disk.

1. **Frame Setup**: The Host Application prepares the `Execution Backend` (e.g., an OpenGL compute shader or C++ thread pool), binding the `Observer` configuration and the `Scene` data (e.g., an SSBO of accretion disk particles).
2. **Work Dispatch**: The Backend spawns parallel workers (one per pixel or ray).
3. **Ray Generation**: Each worker passes its local screen coordinate $(u, v)$ to the `Observer`. The `Observer` constructs a primary `Trajectory State` originating at the observer's location with a tangent pointing toward the virtual $(u, v)$ coordinate on the local sky.
4. **Execution Loop**: The worker enters a `while(true)` loop:
   - Instructs the `Ray Pipeline` to step the state forward by calling the `Integrator`.
   - Passes the new state to the `Scene` intersection tester.
5. **Intersection & Shading**: 
   - If the `Scene` detects the state has crossed an object (e.g., the accretion disk), it calculates the local emission and returns a color payload.
   - If the state escapes to numerical infinity (queried via the `Coordinate Chart`), the `Scene` samples the background skybox and returns a color.
   - If the state falls into the event horizon, it returns black.
6. **Buffer Write**: The loop terminates, and the worker writes the final returned color payload to its designated $(x, y)$ index in the framebuffer.
7. **Display**: The Host Application swaps the framebuffer to the display or saves it to disk as a `.ppm` file.
