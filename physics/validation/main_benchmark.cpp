#include "freefall.h"
#include "orbital.h"
#include "null_geodesic.h"
#include <iostream>
#include <vector>
#include <cmath>

constexpr double rs = 1.0;

int main() {
    double dt = 0.001;
    // ----------------------------------------------------------------
    // BENCHMARK 1: Radial free-fall
    // Drops a particle from r0 with E=1 and measures horizon crossing
    // time against the exact analytical result.
    // ----------------------------------------------------------------
    benchmark_freefall(10.0, dt);

    // ----------------------------------------------------------------
    // BENCHMARK 2: General orbit
    // Set r0, vr, vph and dt freely.
    // vt is computed automatically from the norm condition.
    //
    // Examples:
    //   Spiral infall:    r0=1.4, vr=0.0,  vph=0.05556, dt=0.0001
    //   Circular orbit:   r0=6.0, vr=0.0,  vph=0.05556, dt=0.01
    //   Radial plunge:    r0=10.0, vr=-0.5, vph=0.0,    dt=0.001
    // ----------------------------------------------------------------
    // --- BENCHMARK 2: STABLE ORBIT (MASSIVE PARTICLE) ---
double r0_orb   = 6.0;   // Start well outside the ISCO (r=3.0)
double vr_orb   = 0.0;   // No radial speed (start at the "peak" of the orbit)
double vph_orb  = 0.06;  // Tangential velocity for a precessing ellipse
double dt_orb   = 0.01;
int max_steps   = 100000; // Increase this to see the spirograph pattern

benchmark_orbital(r0_orb, vr_orb, vph_orb, dt_orb, max_steps);

    // --- BENCHMARK 3: PHOTON SPHERE (NULL GEODESIC) ---
    // (Add this new block to test the unstable light orbit)
double r0 = 10.0;
double f  = 1.0 - rs / r0;

// --- physics ---
double b_critical = (3.0 * std::sqrt(3.0) / 2.0) * rs;

// std::vector<double> scales = { 1.001,1.0005,1.0002,1.0001,1.00005};

// for (double scale : scales) {
//     double b = scale * b_critical;

//     double f = 1.0 - rs / r0;
//     double E = 1.0;

//     double L = b * E;
//     double vph = L / (r0 * r0);
//     double vt  = E / f;

//     double vr = -std::sqrt(E*E - f * (L*L / (r0*r0)));

//     std::cout << "\n=== Running b = " << b << " ===\n";

//     benchmark_null_geodesic(
//         r0,
//         vr,
//         vph,
//         0.0005,
//         1000000
//     );
// }

double b = 1.000005   * b_critical;   // start here
double E = 1.0;
double L = b * E;

// --- velocities ---
double vt  = E / f;
double vph = L / (r0 * r0);

double vr = -std::sqrt(
    E*E - f * (L*L / (r0*r0))
);

// ---- B SWEEP (Photon Sphere Exploration) ----
double b_crit = (3.0 * std::sqrt(3.0) / 2.0) * rs;

// log-spaced epsilons (above critical → escape side)
std::vector<double> epsilons = {
    1e-2,
    5e-3,
    2e-3,
    1e-3,
    5e-4,
    2e-4,
    1e-4,
    5e-5,
    2e-5
};

for (double eps : epsilons) {
    double b = b_crit + eps;

    double L = b * E;

    double f = 1.0 - rs / r0;

    double vt  = E / f;
    double vph = L / (r0 * r0);

    double vr = -std::sqrt(E*E - f * (L*L / (r0*r0)));

    std::cout << "\n=== Running b = " << b << " ===\n";

    benchmark_null_geodesic(
        r0,
        vr,
        vph,
        0.0005,
        500000   // increase if needed
    );
}

// ---- BELOW CRITICAL (capture side) ----
std::vector<double> eps_neg = {
    1e-3,
    5e-4,
    1e-4
};

for (double eps : eps_neg) {
    double b = b_crit - eps;

    double L = b * E;

    double f = 1.0 - rs / r0;

    double vt  = E / f;
    double vph = L / (r0 * r0);

    double vr = -std::sqrt(E*E - f * (L*L / (r0*r0)));

    std::cout << "\n=== Running b = " << b << " ===\n";

    benchmark_null_geodesic(
        r0,
        vr,
        vph,
        0.0005,
        500000
    );
}
//---- DT SWEEP ----
std::vector<double> dts = {0.02, 0.01, 0.005, 0.0025};

for (double dt_test : dts) {
    std::cout << "\nRunning dt = " << dt_test << "\n";

    benchmark_null_geodesic(
        r0,
        vr,
        vph,
        dt_test,
        1000000
    );
}
benchmark_null_geodesic(r0, vr, vph, 0.0005, 1000000);

    return 0;
}
