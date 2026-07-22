#pragma once

#include "SimulationConfig.h"

#include <span>
#include <variant>
#include <vector>

namespace Simulation {

// One independently simulated particle. Sims never couple — each request owns its
// config, metric parameters, and initial conditions.
using InitialConditions = std::variant<BoundOrbitInitialConditions, RadialFreefallInitialConditions,
                                       NullScatterInitialConditions, CustomInitialConditions>;

struct SimulationRequest {
    SimulationConfig config;
    Spacetime::SchwarzschildParameters metric{};
    InitialConditions initial = BoundOrbitInitialConditions{};
};

// Physics stage — numerically accurate trajectories only (no visualization decisions).
SimulationResult run_simulation(const SimulationRequest& request);
std::vector<SimulationResult> run_all(std::span<const SimulationRequest> requests);
std::vector<SimulationResult> run_all(const std::vector<SimulationRequest>& requests);

} // namespace Simulation
