#include "freefall.h"
#include "orbital.h"
#include "null_geodesic.h"
#include <iostream>
#include <cmath>

constexpr double rs = 1.0;

static void run_null_at_b(double r0, double b, double dt, int max_steps) {
    double f = 1.0 - rs / r0;
    double E = 1.0;
    double L = b * E;

    double vt  = E / f;
    double vph = L / (r0 * r0);
    double vr  = -std::sqrt(E * E - f * (L * L / (r0 * r0)));

    std::cout << "\n=== Running b = " << b << " (dt = " << dt << ") ===\n";
    benchmark_null_geodesic(r0, vr, vph, dt, max_steps);
}

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
    double r0_orb  = 6.0;
    double vr_orb  = 0.0;
    double vph_orb = 0.06;
    double dt_orb  = 0.01;
    int max_steps  = 100000;

    benchmark_orbital(r0_orb, vr_orb, vph_orb, dt_orb, max_steps);

    // ----------------------------------------------------------------
    // BENCHMARK 3: Null geodesic (representative set)
    // Three cases around b_crit = (3*sqrt(3)/2) * r_s:
    //   escape (b > b_crit), capture (b < b_crit), near-critical.
    // ----------------------------------------------------------------
    double r0 = 10.0;
    double b_crit = (3.0 * std::sqrt(3.0) / 2.0) * rs;
    constexpr double null_dt = 0.0005;
    constexpr int null_max_steps = 200000;

    run_null_at_b(r0, b_crit + 1e-3,  null_dt, null_max_steps);  // escape
    run_null_at_b(r0, b_crit - 1e-3,  null_dt, null_max_steps);  // capture
    run_null_at_b(r0, b_crit + 1e-5,  null_dt, null_max_steps);  // near-critical

    return 0;
}
