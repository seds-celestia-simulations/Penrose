#include "TrajectorySolver.h"
#include <functional>

namespace Simulation {

    std::vector<State> TrajectorySolver::solve(
        const State& initial_state, 
        const Dynamics::DynamicsModel& dynamics, 
        const TerminationPolicy& termination_policy,
        double dt, 
        int max_steps,
        std::function<void(State&, int)> post_step
    ) {
        std::vector<State> history;
        history.reserve(std::min(max_steps, 100000)); // Reasonable reservation
        
        State current = initial_state;
        history.push_back(current);

        Integration::DerivativeFunc derivative = [&dynamics](const State& s) {
            return dynamics.compute_derivative(s);
        };

        for (int i = 0; i < max_steps; ++i) {
            if (termination_policy.should_terminate(current)) {
                break;
            }

            current = Integration::stepRK4(current, dt, derivative);
            if (post_step) {
                post_step(current, i);
            }
            history.push_back(current);
        }

        return history;
    }

}
