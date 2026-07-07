#pragma once
#include "../core/State.h"
#include "../dynamics/DynamicsModel.h"
#include "../integration/RK4Integrator.h"
#include "TerminationPolicy.h"
#include <vector>

namespace Simulation {

    class TrajectorySolver {
    public:
        // Solves the trajectory from an initial state up to max_steps, checking termination conditions.
        // Returns the history of the trajectory.
        static std::vector<State> solve(
            const State& initial_state, 
            const Dynamics::DynamicsModel& dynamics, 
            const TerminationPolicy& termination_policy,
            double dt, 
            int max_steps,
            std::function<void(State&, int)> post_step = nullptr
        );
    };

}
