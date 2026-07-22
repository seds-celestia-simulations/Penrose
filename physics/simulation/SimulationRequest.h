#pragma once

#include "SimulationConfig.h"

#include <metrics/SchwarzschildParameters.h>

#include <span>
#include <variant>
#include <vector>

namespace Simulation {

using InitialConditions = std::variant<BoundOrbitInitialConditions, RadialFreefallInitialConditions,
                                       NullScatterInitialConditions, CustomInitialConditions>;

struct SimulationRequest {
    SimulationConfig config;
    Spacetime::SchwarzschildParameters metric{};
    InitialConditions initial = BoundOrbitInitialConditions{};
};

SimulationResult run_simulation(const SimulationRequest& request);
std::vector<SimulationResult> run_all(std::span<const SimulationRequest> requests);
std::vector<SimulationResult> run_all(const std::vector<SimulationRequest>& requests);

// Compatibility factory for Schwarzschild requests.
SimulationRequest make_schwarzschild_request(SimulationConfig config,
                                             const Spacetime::SchwarzschildParameters& metric,
                                             InitialConditions initial);

} // namespace Simulation
