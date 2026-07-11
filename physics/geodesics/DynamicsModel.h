#pragma once
#include "../state/State.h"

namespace Dynamics {

    class DynamicsModel {
    public:
        virtual ~DynamicsModel() = default;

        // Computes the derivative of the state (i.e. acceleration and velocity)
        virtual State compute_derivative(const State& state) const = 0;
    };

}
