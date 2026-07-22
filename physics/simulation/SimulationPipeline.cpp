#include "SimulationConfig.h"
#include "SimulationRequest.h"
#include "TerminationPolicy.h"
#include "TrajectorySolver.h"

#include "../geodesics/GeodesicDynamics.h"
#include "../metrics/SchwarzschildMetric.h"

#include <cmath>
#include <memory>
#include <stdexcept>
#include <variant>
#include <vector>

namespace Simulation {
namespace {

void require_schwarzschild(const SimulationConfig& config, Scenario expected) {
    if (config.spacetime != SpacetimeKind::Schwarzschild) {
        throw std::runtime_error(
            "run_simulation: metric parameters do not match SimulationConfig::spacetime");
    }
    if (config.scenario != expected) {
        throw std::runtime_error(
            "run_simulation: initial-condition type does not match SimulationConfig::scenario");
    }
}

std::unique_ptr<Spacetime::Metric> make_schwarzschild_metric(
    const Spacetime::SchwarzschildParameters& params) {
    return std::make_unique<Spacetime::SchwarzschildMetric>(params.mass);
}

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

State build_freefall(const Spacetime::SchwarzschildParameters& metric,
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

SimulationResult integrate(const SimulationConfig& config,
                           std::unique_ptr<Spacetime::Metric> metric_impl,
                           double characteristic_radius, const State& initial) {
    Dynamics::GeodesicDynamics dynamics(*metric_impl);
    HorizonTermination policy(characteristic_radius, config.horizon_safety_factor);

    SimulationResult result;
    result.history =
        TrajectorySolver::solve(initial, dynamics, policy, config.dt, config.max_steps);
    result.characteristic_radius = characteristic_radius;
    result.name = config.name;
    result.spacetime = config.spacetime;
    return result;
}

} // namespace

SimulationResult run_simulation(const SimulationConfig& config,
                                const Spacetime::SchwarzschildParameters& metric,
                                const BoundOrbitInitialConditions& initial) {
    require_schwarzschild(config, Scenario::BoundOrbit);
    return integrate(config, make_schwarzschild_metric(metric), metric.mass,
                     build_bound_orbit(metric, initial));
}

SimulationResult run_simulation(const SimulationConfig& config,
                                const Spacetime::SchwarzschildParameters& metric,
                                const RadialFreefallInitialConditions& initial) {
    require_schwarzschild(config, Scenario::RadialFreefall);
    return integrate(config, make_schwarzschild_metric(metric), metric.mass,
                     build_freefall(metric, initial));
}

SimulationResult run_simulation(const SimulationConfig& config,
                                const Spacetime::SchwarzschildParameters& metric,
                                const NullScatterInitialConditions& initial) {
    require_schwarzschild(config, Scenario::NullScatter);
    return integrate(config, make_schwarzschild_metric(metric), metric.mass,
                     build_null_scatter(metric, initial));
}

SimulationResult run_simulation(const SimulationConfig& config,
                                const Spacetime::SchwarzschildParameters& metric,
                                const CustomInitialConditions& initial) {
    require_schwarzschild(config, Scenario::Custom);
    return integrate(config, make_schwarzschild_metric(metric), metric.mass,
                     build_custom(config, metric, initial));
}

SimulationResult run_simulation(const SimulationRequest& request) {
    return std::visit(
        [&](const auto& initial) -> SimulationResult {
            return run_simulation(request.config, request.metric, initial);
        },
        request.initial);
}

std::vector<SimulationResult> run_all(std::span<const SimulationRequest> requests) {
    std::vector<SimulationResult> results;
    results.reserve(requests.size());
    for (const SimulationRequest& request : requests) {
        results.push_back(run_simulation(request));
    }
    return results;
}

std::vector<SimulationResult> run_all(const std::vector<SimulationRequest>& requests) {
    return run_all(std::span<const SimulationRequest>(requests));
}

} // namespace Simulation
