# Project Penrose: Technical Documentation
## Real-Time General Relativistic Raymarching Solver

---

### 1. Introduction & Motivation

Modeling the propagation of light in strong gravitational fields is a fundamental requirement in computational astrophysics and relativistic visualization. Near compact objects like black holes, space and time are warped to such an extent that approximate lensing methods such as the weak-field limit or geometric deflection angles fail to accurately capture physical phenomena. Approximations struggle at resolving higher-order relativistic effects, including infinite-order Einstein rings, the circular orbit of photons at the photon sphere, and coordinate capture at the event horizon. 

Project Penrose addresses this limitation by executing real-time numerical integration of the null geodesic equations derived from the Einstein Field Equations. By formulating the geodesic equations as a system of coupled first-order ordinary differential equations (ODEs), the simulator tracks individual photon trajectories through curved Schwarzschild spacetime.

To achieve real-time frame rates, Penrose implements this integration directly within a GPU fragment shader (GLSL 4.30) via screen-space raymarching. Each pixel on the screen corresponds to a single photon path, which is solved independently and in parallel. This implementation enables interactive exploration of general relativistic lensing and horizon shadows without sacrificing physical correctness.

---

### 2. High-Level Architecture

The framework is partitioned into a dual-pipeline architecture to decouple offline verification from real-time visualization:

```
+-----------------------------------------------------------------------------+
|                            Physics Pipeline (CPU)                            |
|  - Define initial conditions (Position/Velocity) via Eigen3                 |
|  - Compute Schwarzschild Christoffel Symbols                                |
|  - Evolve state using 4th-Order Runge-Kutta (RK4) Integrator                |
|  - Export trajectories to CSV for analytical validation & plotting          |
+-----------------------------------------------------------------------------+
                                       |
                                       v
+-----------------------------------------------------------------------------+
|                         Visualization Pipeline (GPU)                        |
|  - Render screen-space Quad (Vertex Shader)                                 |
|  - Un-project pixels to Cartesian world-space rays using InvProjView matrix |
|  - Convert Cartesian coordinates to Spherical coordinates                   |
|  - Initialize null geodesic velocity vector using Metric constraint         |
|  - March coordinates using parallelized GPU RK4 numerical solver            |
|  - Evaluate horizon crossing, particle buffer (SSBO) collision, or escape    |
|  - Write fragment colors, capture framebuffers to PPM, compile to MP4       |
+-----------------------------------------------------------------------------+
```

---

### 3. Physical Model

The physical model is governed by the geometry of a static, spherically symmetric, non-rotating mass $M$ in vacuum, described by General Relativity. Natural units are adopted throughout the system ($G = c = 1$).

#### 3.1 The Schwarzschild Metric
Spacetime geometry is defined by the Schwarzschild metric tensor $g_{\mu\nu}$. In spherical coordinates $x^\mu = (t, r, \theta, \phi)$, the line element $ds^2$ is:

$$ds^2 = -\left(1 - \frac{r_s}{r}\right) dt^2 + \left(1 - \frac{r_s}{r}\right)^{-1} dr^2 + r^2 d\theta^2 + r^2 \sin^2\theta d\phi^2$$

where $r_s = 2M$ is the Schwarzschild radius representing the event horizon.

#### 3.2 Null Geodesics and the Metric Constraint
Light rays travel along null geodesics ($ds^2 = 0$). To initialize a physical light ray, the time component of the velocity vector ($u^t$) is solved from the spatial velocity components:

$$u^t = \frac{1}{\sqrt{1 - \frac{r_s}{r}}} \sqrt{ \frac{(u^r)^2}{1 - \frac{r_s}{r}} + r^2 (u^\theta)^2 + r^2 \sin^2\theta (u^\phi)^2 }$$

#### 3.3 The Geodesic Equation
The equations of motion for particles in curved spacetime are governed by the geodesic equation. The four-acceleration vector $a^\mu = \frac{du^\mu}{d\lambda}$ (where $\lambda$ is an affine parameter) is calculated as:

$$a^\mu = -\Gamma^\mu_{\alpha\beta} u^\alpha u^\beta$$

where $\Gamma^\mu_{\alpha\beta}$ are the Christoffel symbols of the second kind.

#### 3.4 Christoffel Symbols
The non-zero Christoffel symbols for the Schwarzschild geometry are structured as follows:

| Component ($\mu$) | Symbol | Analytic Expression |
| :--- | :--- | :--- |
| **Time ($t$)** | $\Gamma^t_{tr} = \Gamma^t_{rt}$ | $\frac{r_s}{2r(r - r_s)}$ |
| **Radial ($r$)** | $\Gamma^r_{tt}$ | $\frac{r_s(r - r_s)}{2r^3}$ |
| | $\Gamma^r_{rr}$ | $-\frac{r_s}{2r(r - r_s)}$ |
| | $\Gamma^r_{\theta\theta}$ | $-(r - r_s)$ |
| | $\Gamma^r_{\phi\phi}$ | $-(r - r_s)\sin^2\theta$ |
| **Polar ($\theta$)** | $\Gamma^\theta_{r\theta} = \Gamma^\theta_{\theta r}$ | $\frac{1}{r}$ |
| | $\Gamma^\theta_{\phi\phi}$ | $-\sin\theta\cos\theta$ |
| **Azimuthal ($\phi$)** | $\Gamma^\phi_{r\phi} = \Gamma^\phi_{\phi r}$ | $\frac{1}{r}$ |
| | $\Gamma^\phi_{\theta\phi} = \Gamma^\phi_{\phi\theta}$ | $\cot\theta$ |

---

### 4. Numerical Methods

#### 4.1 State Vector Definition
The dynamical variables evolved by the integration solver are represented by the $8$-dimensional state vector $\mathbf{S}$:

$$\mathbf{S} = \begin{pmatrix} x^\mu \\ u^\mu \end{pmatrix} = \begin{pmatrix} t \\ r \\ \theta \\ \phi \\ u^t \\ u^r \\ u^\theta \\ u^\phi \end{pmatrix}$$

The evolution of the state vector with respect to the affine parameter $\lambda$ is governed by the system of first-order differential equations:

$$\frac{d\mathbf{S}}{d\lambda} = \begin{pmatrix} u^\mu \\ -\Gamma^\mu_{\alpha\beta} u^\alpha u^\beta \end{pmatrix}$$

#### 4.2 Coordinate Representation and Transformations
While numerical integration is evaluated in spherical coordinates $(t, r, \theta, \phi)$, boundary conditions are represented in Cartesian coordinates $(x,y,z)$. 

Velocity transformations between systems are mapped using the Jacobian transformation:

$$u^{\mu}_{\text{cart}} = J_{\text{sph}\to\text{cart}} \cdot u^{\mu}_{\text{sph}}$$

where $J_{\text{sph}\to\text{cart}}$ is the spherical-to-Cartesian Jacobian matrix. Mapping from Cartesian camera rays to spherical state vectors uses its inverse $J_{\text{cart}\to\text{sph}}$.

#### 4.3 RK4 Integration Scheme
The state vector is evolved using a 4th-order Runge-Kutta (RK4) integration step size $h$ :

$$\mathbf{k}_1 = f(\mathbf{S}_n)$$
$$\mathbf{k}_2 = f\left(\mathbf{S}_n + \frac{h}{2}\mathbf{k}_1\right)$$
$$\mathbf{k}_3 = f\left(\mathbf{S}_n + \frac{h}{2}\mathbf{k}_2\right)$$
$$\mathbf{k}_4 = f(\mathbf{S}_n + h\mathbf{k}_3)$$
$$\mathbf{S}_{n+1} = \mathbf{S}_n + \frac{h}{6}(\mathbf{k}_1 + 2\mathbf{k}_2 + 2\mathbf{k}_3 + \mathbf{k}_4)$$

#### 4.4 Numerical Choices and Stability
* **Solver Selection**: The RK4 method is selected because its local truncation error scale is $\mathcal{O}(h^5)$, and global accumulation error is $\mathcal{O}(h^4)$. Compared to lower-order Euler integration ($\mathcal{O}(h)$), RK4 significantly limits the numerical drift of conserved invariants over long integration steps.
* **Timestep Strategy**: A fixed-timestep integration scheme is selected for the GPU implementation. While adaptive steps (such as Runge-Kutta-Fehlberg RKF45) reduce computation in flat spacetime, they introduce branch divergence across GPU threads, reducing parallel warp efficiency.
* **Singularity Mitigation**: Near coordinate singularities (the horizon $r \to r_s$ and poles $\theta \to 0, \pi$), numerical divisions by $(r - r_s)$ and $\sin\theta$ diverge. The polar coordinate singularity is resolved via reflection boundary conditions: if $\theta < 0$, the state is updated to $-\theta$ and $\phi$ is shifted by $\pi$, negating the velocity component $u^\theta$. To prevent horizon divergence, integration terminates once the ray crosses $r < 1.1 r_s$.
* **Precision Constraints**: Shader execution is limited to 32-bit single-precision floating-point (fp32) values. This limits spatial resolution near the horizon due to round-off error accumulation over $\sim 1000$ steps.

---

### 5. GPU Rendering Pipeline

The rendering engine processes screen-space rays through parallel GPU raymarching:

1. **Ray Projection**: For each pixel, the fragment shader un-projects screen coordinates to construct a world-space Cartesian ray direction.
2. **Raymarching Loop**: The ray is converted to spherical coordinates and advanced step-by-step using the GPU RK4 solver. At each step, the engine evaluates key boundary conditions:
   * **Horizon Absorption**: If $r < 1.1 r_s$, the ray is marked as captured and renders black.
   * **SSBO Particle Hit**: Cartesian ray positions are checked against an SSBO buffer of disk particles. A hit immediately returns the particle's color.
   * **Escape to Skybox**: If $r > 20.0$, the final direction vector samples the background environment map.

---

### 6. Computational Characteristics

* **Parallel Workload**: The algorithm executes an embarrassingly parallel workload, where each pixel computes an independent geodesic. 
* **GPU Suitability**: The Single Instruction, Multiple Threads (SIMT) architecture of modern GPUs is suited for this workload. Adjacent rays follow similar trajectories through spacetime, which maintains instruction coherency. Branch divergence occurs primarily at the boundary of the black hole shadow, where adjacent threads diverge between horizon capture and escape.
* **Complexity**: The computational complexity per pixel is $\mathcal{O}(N_{\text{steps}})$, where $N_{\text{steps}} \le 1000$. Each step requires evaluation of 16 Christoffel components and 4 matrix multiplications.
* **Memory Footprint**: The implementation has a memory footprint of $< 50$ MB. The geometry is generated procedurally, and physical particles are streamed via a single SSBO.

---

### 7. Validation & Benchmarking

The numerical engine is validated by executing test scenarios and comparing results against analytical constraints.

#### 7.1 Radial Free-Fall Validation
A test particle is released from rest at $r_0 = 10.0$ with energy $E = 1$ in Schwarzschild space ($r_s = 1.0$). The analytical proper time $\tau$ to reach the event horizon is given by:

$$\tau_{\text{analytical}} = \frac{2}{3 \sqrt{r_s}} \left( r_0^{3/2} - r_s^{3/2} \right) \approx 20.4151844$$

The C++ integration suite steps through this trajectory using the RK4 solver. At $dt = 0.01$, the horizon is crossed at $\tau_{\text{numerical}} \approx 20.41$, demonstrating a relative error of $< 0.05\%$.

#### 7.2 Conservation of Invariants
 Schwarzschild spacetime exhibits two Killing vectors, $\xi^\mu = (1, 0, 0, 0)$ and $\eta^\mu = (0, 0, 0, 1)$, corresponding to time-translation and azimuthal symmetries. These symmetries guarantee the conservation of Energy ($E$) and Angular Momentum ($L$):

$$E = -g_{tt} u^t = \left(1 - \frac{r_s}{r}\right) u^t$$
$$L = g_{\phi\phi} u^\phi = r^2 \sin^2\theta u^\phi$$

The geodesic norm constraint ($g_{\mu\nu} u^\mu u^\nu = \eta$) is also conserved, where $\eta = -1$ for massive particles and $\eta = 0$ for photons. The circular orbit benchmark ($r_0 = 6.0, u^\phi = 0.05556$) monitors these invariants. Over $5 \times 10^6$ steps, numerical drift of $E$, $L$, and the norm constraint remains bounded within $10^{-6}$ at $dt=0.01$, demonstrating numerical stability.

```
=== Orbital Benchmark ===
r0               = 6.00000000
rs               = 1.00000000
dt               = 0.01000000
norm (should -1) = -1.00000000
E  (conserved)   = 0.94280904
L  (conserved)   = 2.00016000
```

---

### 8. Current Limitations

* **Symmetry Constraints**: The physical model is restricted to a non-rotating Schwarzschild black hole. Rotating black holes are not supported.
* **Precision Limits**: The GPU solver uses 32-bit single-precision floats (fp32). Near the singularity, rounding accumulation degraded energy conservation.
* **Timestep Limitations**: The integration uses a fixed spatial step size. Rays near the photon sphere require higher step density to avoid coordinate drift.
* **Simplified Disk representation**: The accretion disk is modeled as kinematic point particles, neglecting optical depth, absorption coefficients, or temperature gradients.
* **Spectral Shift Absence**: The visualization pipeline does not calculate relativistic Doppler boosting or gravitational redshift.

---

### 9. Future Development

* **Kerr Spacetime Integration**: Future revisions will implement the Kerr metric to simulate rotating black holes. This requires evaluating non-zero off-diagonal metric components $g_{t\phi}$ and a more complex set of Christoffel symbols.
* **Numerical Upgrades**: Implementing GPU double-precision variables (fp64) to eliminate coordinate drift, and adaptive Runge-Kutta-Fehlberg (RKF45) integration.
* **Advanced Relativistic Effects**: Implementing relativistic Doppler boosting and gravitational redshift by tracking frequency shifts along the null geodesic.
* **Physical Accretion Disk Modeling**: Replacing the simple kinematic particle disk with physical profiles like a Novikov-Thorne thin disk or datasets from General Relativistic Magnetohydrodynamics (GRMHD) simulations.

---

### 10. Conclusion

Project Penrose demonstrates the feasibility of executing real-time general relativistic raymarching directly on consumer GPU architectures. By solving the null geodesic equations using a parallelized 4th-order Runge-Kutta solver in a GLSL fragment shader, the framework produces physically motivated visualizations of Schwarzschild lensing without relying on weak-field approximations. Validation via radial free-fall and orbital benchmarks confirms the numerical accuracy and conservation properties of the solver. Future extensions to rotating Kerr spacetimes and double-precision numeric schemes will further enhance the simulation's physical fidelity and utility as a computational physics research tool.
