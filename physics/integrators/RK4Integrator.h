#pragma once
#include <state/GeodesicState.h>
#include <functional>

namespace Integration {

    // Represents the equation of motion for both position and velocity: d/dtau [X, U]
    using DerivativeFunc = std::function<State(const State&)>;

    // Advances the state by dt using standard 4th-order Runge-Kutta
    State stepRK4(const State& s, double dt, const DerivativeFunc& derivFunc);
}
