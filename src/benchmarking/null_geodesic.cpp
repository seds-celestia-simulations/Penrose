#define _USE_MATH_DEFINES
#include "null_geodesic.h"
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

void benchmark_null_geodesic(double r0, double vr, double vph, double dt, int max_steps) {
    double f = 1.0 - rs / r0;

    // ---- Compute vt from null condition ----
    double term_r   = (vr * vr) / f;
    double term_phi = r0 * r0 * vph * vph;
    double vt       = std::sqrt((term_r + term_phi) / f);

    State s(
        Eigen::Vector4d(0.0, r0, M_PI / 2.0, 0.0),
        Eigen::Vector4d(vt, vr, 0.0, vph)
    );

    // ---- Initial invariants ----
    double E0 = f * vt;
    double L0 = r0 * r0 * vph;
    double b0 = L0 / E0;

    std::cout << "\n=== Null Geodesic Benchmark ===\n";
    std::cout << std::fixed << std::setprecision(15);
    std::cout << "r0 = " << r0 << " | rs = " << rs << " | dt = " << dt << "\n";
    std::cout << "vr = " << vr << " | vph = " << vph << "\n";
    std::cout << "vt = " << vt << "\n";
    std::cout << "Impact parameter b = " << b0 << "\n\n";

    // ---- CSV ----
    std::ofstream csv("src/benchmarking/data/null_b_" + std::to_string(b0) + ".csv");
    csv << "lambda,r,phi,vt,vr,vph,H,E,L,dE,dL,dvt,dvph,phi_total\n";

    // ---- Terminal header ----
    std::cout << std::setw(8)  << "step"
              << std::setw(12) << "lambda"
              << std::setw(12) << "r"
              << std::setw(12) << "phi"
              << std::setw(14) << "H_error"
              << std::setw(14) << "dE"
              << std::setw(14) << "dL"
              << "\n";
    std::cout << std::string(80, '-') << "\n";

    // ---- Configure Simulation ----
    Spacetime::SchwarzschildMetric metric(rs);
    Dynamics::GeodesicDynamics dynamics(metric);
    Simulation::HorizonTermination policy(rs, 1.0001); // terminate at rs * 1.0001

    auto projection = [](State& state, int step) {
        double r_  = state.X[1];
        if (r_ <= rs) return; // Don't project if inside or at horizon
        double f_  = 1.0 - rs / r_;
        double vr_  = state.U[1];
        double vph_ = state.U[3];

        double vt_new = std::sqrt((vr_*vr_/f_ + r_*r_*vph_*vph_) / f_);
        if (step % 1000 == 0) {
            state.U[0] = vt_new;
        }

        if (state.U[0] < 0.0) {
            state.U[0] = std::abs(state.U[0]);
        }
    };

    std::vector<State> history = Simulation::TrajectorySolver::solve(s, dynamics, policy, dt, max_steps, projection);

    // ---- Tracking variables ----
    double r_min = r0;
    double vt_prev = vt;
    double vph_prev = vph;
    double phi_total = 0.0;
    double phi_prev = s.X[3];

    for (size_t step = 0; step < history.size(); ++step) {
        const State& state = history[step];
        double lambda = step * dt;

        double r_   = state.X[1];
        double phi_ = state.X[3];
        double vt_  = state.U[0];
        double vr_  = state.U[1];
        double vph_ = state.U[3];

        double f_ = 1.0 - rs / r_;
        double dphi = phi_ - phi_prev;

        // unwrap
        if (dphi < -M_PI) dphi += 2*M_PI;
        if (dphi >  M_PI) dphi -= 2*M_PI;

        phi_total += dphi;
        phi_prev = phi_;

        // ---- Track minimum radius (observable) ----
        if (r_ < r_min) r_min = r_;

        // ---- Hamiltonian (null constraint) ----
        double H = -f_ * vt_ * vt_
                 + (1.0 / f_) * vr_ * vr_
                 + r_ * r_ * vph_ * vph_;

        double scale = std::abs(vt_*vt_) + std::abs(vr_*vr_) + std::abs(r_*r_*vph_*vph_) + 1e-12;
        double H_error = std::abs(H) / scale;

        // ---- Step-to-step drift (true numerical noise) ----
        double dvt  = vt_  - vt_prev;
        double dvph = vph_ - vph_prev;

        vt_prev  = vt_;
        vph_prev = vph_;

        // ---- Conserved quantities ----
        double E_ = f_ * vt_;
        double L_ = r_ * r_ * vph_;

        double dE = (E_ - E0) / E0;
        double dL = (L_ - L0) / L0;

        // ---- CSV write ----
        csv << std::scientific << std::setprecision(15)
            << lambda << ","
            << r_ << ","
            << phi_ << ","
            << vt_ << ","
            << vr_ << ","
            << vph_ << ","
            << H << ","
            << E_ << ","
            << L_ << ","
            << dE << ","
            << dL << ","
            << dvt << ","
            << dvph << ","
            << phi_total << "\n";

        // ---- Console output ----
        if (step % 1000 == 0) {
            std::cout << std::setw(8)  << step
                      << std::setw(12) << lambda
                      << std::setw(12) << r_
                      << std::setw(12) << phi_
                      << std::setw(14) << std::scientific << H_error << std::fixed
                      << std::setw(14) << dE
                      << std::setw(14) << dL
                      <<  " H=" << H
                      << " E=" << E_
                      << " L=" << L_ 
                      << "\n";
        }

        // ---- Termination ----
        if (r_ <= rs * 1.0001) {
            std::cout << "\n[Horizon crossed]\n";
            break;
        }

        if (r_ > 1000.0 && vr_>0) {
            std::cout << "\n[Escaped]\n";
            break;
        }
    }

    if (history.size() - 1 == (size_t)max_steps) {
        std::cout << "\n[Max steps reached]\n";
    }

    csv.close();

    std::cout << "\nMin radius reached: " << r_min << "\n";
    std::cout << "CSV written.\n";
}