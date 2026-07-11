#include "RK4Integrator.h"

namespace Integration {

State stepRK4(const State& s, double dt, const DerivativeFunc& derivFunc) {
    State k1 = derivFunc(s);    
    State k2 = derivFunc(s + k1 * (dt / 2.0));
    State k3 = derivFunc(s + k2 * (dt / 2.0));
    State k4 = derivFunc(s + k3 * dt);

    State s_final = s + (dt / 6.0) * (k1 + 2.0 * k2 + 2.0 * k3 + k4);

    return s_final;
}

} // namespace Integration
