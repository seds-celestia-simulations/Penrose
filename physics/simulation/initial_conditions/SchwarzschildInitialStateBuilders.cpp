#include "SchwarzschildInitialStateBuilders.h"

#include "../SimulationConfig.h"

#include <cmath>
#include <stdexcept>

namespace Simulation::InitialStateBuilders {

State build_bound_orbit(const Spacetime::SchwarzschildParameters& metric,
                        const BoundOrbitInitialConditions& initial) {
    const double rs = metric.mass;
    const double r0 = initial.r0;
    const double f = 1.0 - rs / r0;
    if (f <= 0.0) {
        throw std::runtime_error("BoundOrbit: r0 must be greater than the Schwarzschild radius");
    }
    const double vt = std::sqrt((1.0 + r0 * r0 * initial.vphi * initial.vphi) / f);
    return State(Eigen::Vector4d(initial.t0, r0, initial.theta0, initial.phi0),
                 Eigen::Vector4d(vt, initial.vr, initial.vtheta, initial.vphi));
}

State build_radial_freefall(const Spacetime::SchwarzschildParameters& metric,
                            const RadialFreefallInitialConditions& initial) {
    const double rs = metric.mass;
    const double r0 = initial.r0;
    const double f = 1.0 - rs / r0;
    if (f <= 0.0) {
        throw std::runtime_error("RadialFreefall: r0 must be greater than the Schwarzschild radius");
    }
    const double vt = 1.0 / f;
    const double vr = -std::sqrt(rs / r0);
    return State(Eigen::Vector4d(initial.t0, r0, initial.theta0, initial.phi0),
                 Eigen::Vector4d(vt, vr, 0.0, 0.0));
}

State build_null_scatter(const Spacetime::SchwarzschildParameters& metric,
                         const NullScatterInitialConditions& initial) {
    const double rs = metric.mass;
    const double r0 = initial.r0;
    const double f = 1.0 - rs / r0;
    if (f <= 0.0) {
        throw std::runtime_error("NullScatter: r0 must be greater than the Schwarzschild radius");
    }

    const double b_crit = (3.0 * std::sqrt(3.0) / 2.0) * rs;
    const double b = (initial.impact_parameter > 0.0)
                         ? initial.impact_parameter
                         : (b_crit + initial.impact_parameter_offset);

    const double E = 1.0;
    const double L = b * E;
    const double vt = E / f;
    const double vph = L / (r0 * r0);
    const double inside = E * E - f * (L * L / (r0 * r0));
    if (inside < 0.0) {
        throw std::runtime_error("NullScatter: impact parameter yields imaginary radial velocity");
    }
    const double vr = -std::sqrt(inside);
    return State(Eigen::Vector4d(initial.t0, r0, initial.theta0, initial.phi0),
                 Eigen::Vector4d(vt, vr, 0.0, vph));
}

State build_custom(const SimulationConfig& config, const Spacetime::SchwarzschildParameters& metric,
                   const CustomInitialConditions& initial) {
    double vt = initial.vt;
    const double rs = metric.mass;
    const double r0 = initial.r0;
    const double f = 1.0 - rs / r0;
    if (f <= 0.0) {
        throw std::runtime_error("Custom: r0 must be greater than the Schwarzschild radius");
    }

    if (vt == 0.0) {
        if (config.geodesic == GeodesicKind::Null) {
            const double term_r = (initial.vr * initial.vr) / f;
            const double term_phi = r0 * r0 * initial.vphi * initial.vphi;
            vt = std::sqrt((term_r + term_phi) / f);
        } else {
            const double inner =
                (1.0 + r0 * r0 * initial.vphi * initial.vphi + (initial.vr * initial.vr) / f) / f;
            vt = std::sqrt(std::max(inner, 0.0));
        }
    }

    return State(Eigen::Vector4d(initial.t0, r0, initial.theta0, initial.phi0),
                 Eigen::Vector4d(vt, initial.vr, initial.vtheta, initial.vphi));
}

} // namespace Simulation::InitialStateBuilders
