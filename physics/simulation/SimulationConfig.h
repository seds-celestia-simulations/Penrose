#pragma once

#include "initial_conditions/InitialConditions.h"

#include "../metrics/parameters/SchwarzschildParameters.h"

#include <state/GeodesicState.h>

#include <string>
#include <vector>

namespace Simulation {

// High-level spacetime selection for the orchestration layer.
enum class SpacetimeKind {
    Schwarzschild,
    // Future: Kerr, ReissnerNordstrom, FLRW, SolarGravitationalLens, ...
};

enum class Scenario {
    BoundOrbit,
    RadialFreefall,
    NullScatter,
    Custom,
};

enum class GeodesicKind {
    Timelike,
    Null,
};

// Layer 1 — physics settings / orchestration only.
// Exists solely for numerical correctness (timestep, step limit, termination).
// Metric- and scenario-specific numbers live in dedicated physics parameter types.
// Visualization resolution is NOT configured here — see VisualizationPreparationSettings.
struct SimulationConfig {
    SpacetimeKind spacetime = SpacetimeKind::Schwarzschild;
    Scenario scenario = Scenario::BoundOrbit;
    GeodesicKind geodesic = GeodesicKind::Timelike;

    double dt = 0.01;
    int max_steps = 100000;
    double horizon_safety_factor = 1.0;

    std::string name = "trajectory";
};

// Physics → Trajectory Storage boundary.
// Immutable record of one independently integrated particle. No render knobs.
struct SimulationResult {
    std::vector<State> history;
    double characteristic_radius = 1.0;
    std::string name;
    SpacetimeKind spacetime = SpacetimeKind::Schwarzschild;
};

// Alias clarifying the storage role in the Physics → Storage → Viz prep pipeline.
using PhysicsTrajectory = SimulationResult;

// Layer 2 is passed separately: metric parameters + initial conditions.
// Adding Kerr/FLRW/SGL means new overloads + parameter structs — not new fields here.
SimulationResult run_simulation(const SimulationConfig& config,
                                const Spacetime::SchwarzschildParameters& metric,
                                const BoundOrbitInitialConditions& initial);

SimulationResult run_simulation(const SimulationConfig& config,
                                const Spacetime::SchwarzschildParameters& metric,
                                const RadialFreefallInitialConditions& initial);

SimulationResult run_simulation(const SimulationConfig& config,
                                const Spacetime::SchwarzschildParameters& metric,
                                const NullScatterInitialConditions& initial);

SimulationResult run_simulation(const SimulationConfig& config,
                                const Spacetime::SchwarzschildParameters& metric,
                                const CustomInitialConditions& initial);

} // namespace Simulation
