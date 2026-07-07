#define _USE_MATH_DEFINES
#include "orbital.h"
#include "../spacetime/SchwarzschildMetric.h"
#include "../dynamics/GeodesicDynamics.h"
#include "../simulation/TrajectorySolver.h"
#include "../simulation/TerminationPolicy.h"
#include <iostream>
#include <iomanip>
#include <fstream>
#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

constexpr double rs = 1.0;

void benchmark_orbital(double r0, double vr, double vph, double dt, int max_steps) {

    double f = 1.0 - rs / r0;

    // Compute vt from norm condition g_uv U^u U^v = -1
    // -(1-rs/r)*vt^2 + (1/(1-rs/r))*vr^2 + r^2*vph^2 = -1
    // => vt = sqrt((1 + r^2*vph^2 - (1/(1-rs/r))*vr^2) / (1-rs/r))
    double inner = (1.0 + r0 * r0 * vph * vph - (1.0 / f) * vr * vr) / f;
    if (inner <= 0.0) {
        std::cerr << "[!] Invalid initial conditions: vr/vph combination "
                  << "cannot satisfy norm condition. Reduce vr or vph.\n";
        return;
    }
    double vt = std::sqrt(inner);

    State s(
        Eigen::Vector4d(0.0, r0, M_PI / 2.0, 0.0),
        Eigen::Vector4d(vt, vr, 0.0, vph)
    );

    // ---- Diagnostics ----
    {
        double norm = -f * vt * vt + (1.0 / f) * vr * vr + r0 * r0 * vph * vph;
        double E    = f * vt;
        double L    = r0 * r0 * vph;

        std::cout << "\n=== Orbital Benchmark ===\n";
        std::cout << std::fixed << std::setprecision(8);
        std::cout << "r0               = " << r0        << "\n";
        std::cout << "rs               = " << rs        << "\n";
        std::cout << "dt               = " << dt        << "\n";
        std::cout << "vr               = " << vr        << "\n";
        std::cout << "vph              = " << vph       << "\n";
        std::cout << "vt (computed)    = " << vt        << "\n";
        std::cout << "max_steps        = " << max_steps << "\n";
        std::cout << "norm (should -1) = " << norm      << "\n";
        std::cout << "E  (conserved)   = " << E         << "\n";
        std::cout << "L  (conserved)   = " << L         << "\n\n";
    }

    Spacetime::SchwarzschildMetric metric(rs);
    Dynamics::GeodesicDynamics dynamics(metric);
    Simulation::HorizonTermination policy(rs, 1.0); // terminate exactly at rs

    std::vector<State> history = Simulation::TrajectorySolver::solve(s, dynamics, policy, dt, max_steps);

    // ---- CSV ----
    std::ofstream csv("src/benchmarking/data/orbital.csv");
    csv << "tau,r,phi,vt,vr,vph,norm\n";

    // ---- Terminal header ----
    std::cout << std::setw(10) << "step"
              << std::setw(12) << "tau"
              << std::setw(12) << "r"
              << std::setw(12) << "phi"
              << std::setw(12) << "vr"
              << std::setw(14) << "norm"
              << "\n";
    std::cout << std::string(72, '-') << "\n";

    for (size_t step = 0; step < history.size(); ++step) {
        const State& state = history[step];
        double tau  = step * dt;
        double r_   = state.X[1];
        double phi_ = state.X[3];
        double vt_  = state.U[0];
        double vr_  = state.U[1];
        double vph_ = state.U[3];
        
        // Don't calculate norm if r is singular
        double norm = 0.0;
        if (r_ > rs) {
            double f_   = 1.0 - rs / r_;
            norm = -f_ * vt_ * vt_ + (1.0 / f_) * vr_ * vr_ + r_ * r_ * vph_ * vph_;
        }

        csv << std::fixed << std::setprecision(8)
            << tau  << ","
            << r_   << ","
            << phi_ << ","
            << vt_  << ","
            << vr_  << ","
            << vph_ << ","
            << norm << "\n";

        if (step % 1000 == 0) {
            std::cout << std::setw(10) << step
                      << std::setw(12) << tau
                      << std::setw(12) << r_
                      << std::setw(12) << phi_
                      << std::setw(12) << vr_
                      << std::setw(14) << norm
                      << "\n";
        }
        
        if (r_ <= rs) {
            std::cout << "\n[Horizon crossed] step = " << step << "  tau = " << tau << "\n";
        }
        if (r_ > 1000.0) {
            std::cout << "\n[Escaped to infinity] step = " << step << "  tau = " << tau << "\n";
            break;
        }
    }

    if (history.size() - 1 == (size_t)max_steps) {
        std::cout << "\n[Max steps reached] tau = " << (history.size() - 1) * dt << "\n";
    }

    csv.close();
    std::cout << "\nCSV written to orbital.csv\n";
    std::cout << "=== Orbital complete ===\n";
}
