#include "TerminationPolicy.h"

namespace Simulation {

    HorizonTermination::HorizonTermination(double event_horizon_radius, double safety_factor) 
        : r_horizon_(event_horizon_radius * safety_factor) {}

    bool HorizonTermination::should_terminate(const State& state) const {
        return state.X[1] <= r_horizon_;
    }

}
