#define _USE_MATH_DEFINES
#include "freefall.h"
#include "benchmark_io.h"
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

// Exact proper time from r0 to rs for radial free-fall with E=1.
// Derived from (dr/dtau)^2 = rs/r:
//   tau = (2/3) * (1/sqrt(rs)) * (r0^(3/2) - rs^(3/2))
static double analytical_freefall_time(double r0) {
    return (2.0 / 3.0) * (1.0 / std::sqrt(rs))
           * (std::pow(r0, 1.5) - std::pow(rs, 1.5));
}

void benchmark_freefall(double r0, double dt) {

    // Initial 4-velocity for E=1 radial free-fall
    // vt = 1/(1 - rs/r0),  vr = -sqrt(rs/r0)
    double f  = 1.0 - rs / r0;
    double vt = 1.0 / f;
    double vr = -std::sqrt(rs / r0);

    State s(
        Eigen::Vector4d(0.0, r0, M_PI / 2.0, 0.0),
        Eigen::Vector4d(vt, vr, 0.0, 0.0)
    );

    double tau_analytical = analytical_freefall_time(r0);

    // ---- Diagnostics ----
    {
        double norm = -f * vt * vt + (1.0 / f) * vr * vr;
        double E    = f * vt;
        std::cout << "\n=== Free-Fall Benchmark ===\n";
        std::cout << std::fixed << std::setprecision(8);
        std::cout << "r0                       = " << r0             << "\n";
        std::cout << "rs                       = " << rs             << "\n";
        std::cout << "dt                       = " << dt             << "\n";
        std::cout << "vt                       = " << vt             << "\n";
        std::cout << "vr                       = " << vr             << "\n";
        std::cout << "norm     (should be -1)  = " << norm           << "\n";
        std::cout << "E        (should be  1)  = " << E              << "\n";
        std::cout << "tau_analytical           = " << tau_analytical << "\n\n";
    }

    Spacetime::SchwarzschildMetric metric(rs);
    Dynamics::GeodesicDynamics dynamics(metric);
    Simulation::HorizonTermination policy(rs, 1.0); // terminate exactly at rs

    std::vector<State> history = Simulation::TrajectorySolver::solve(s, dynamics, policy, dt, 100000);

    // ---- CSV ----
    std::ofstream csv = open_benchmark_csv("freefall.csv");
    if (!csv.is_open()) {
        return;
    }
    csv << "tau,r,vt,vr\n";

    // ---- Terminal header ----
    std::cout << std::setw(10) << "step"
              << std::setw(14) << "tau"
              << std::setw(12) << "r"
              << std::setw(14) << "vt"
              << std::setw(14) << "vr"
              << "\n";
    std::cout << std::string(64, '-') << "\n";

    for (size_t step = 0; step < history.size(); ++step) {
        const State& state = history[step];
        double tau = step * dt;
        double r_  = state.X[1];
        double vt_ = state.U[0];
        double vr_ = state.U[1];

        // Write every step to CSV
        csv << std::fixed << std::setprecision(8)
            << tau << "," << r_ << "," << vt_ << "," << vr_ << "\n";

        // Print every 500 steps to terminal
        if (step % 500 == 0) {
            std::cout << std::setw(10) << step
                      << std::setw(14) << tau
                      << std::setw(12) << r_
                      << std::setw(14) << vt_
                      << std::setw(14) << vr_
                      << "\n";
        }

        // DIAGNOSTIC 2: Stop if the math breaks (NaN)
        if (std::isnan(r_)) {
            std::cout << "TERMINATED: r became NaN at step " << step << "\n";
            break;
        }

        if (r_ <= rs) {
            double error         = std::abs(tau - tau_analytical);
            double error_percent = 100.0 * error / tau_analytical;
            std::cout << "\n--- Horizon crossed at step " << step << " ---\n";
            std::cout << "tau_numerical  = " << tau            << "\n";
            std::cout << "tau_analytical = " << tau_analytical << "\n";
            std::cout << "absolute error = " << error          << "\n";
            std::cout << "relative error = " << error_percent  << " %\n";
            break;
        }
    }
    
    if (history.size() == 100000 + 1) { // 100k steps plus initial state
        const State& last_state = history.back();
        std::cout << "TERMINATED: Reached 100k steps. r is currently: " << last_state.X[1] << "\n";
    }

    csv.close();
    std::cout << "\nCSV written to " << (benchmark_data_dir() / "freefall.csv") << "\n";
    std::cout << "=== Free-fall complete ===\n";
}
