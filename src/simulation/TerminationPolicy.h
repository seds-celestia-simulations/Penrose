#pragma once
#include "../core/State.h"
#include "../spacetime/Metric.h"

namespace Simulation {

    class TerminationPolicy {
    public:
        virtual ~TerminationPolicy() = default;

        // Returns true if the simulation should halt
        virtual bool should_terminate(const State& state) const = 0;
    };

    // A specific policy for stopping particles that cross the Schwarzschild horizon
    class HorizonTermination : public TerminationPolicy {
    public:
        explicit HorizonTermination(double event_horizon_radius, double safety_factor = 1.001);

        bool should_terminate(const State& state) const override;

    private:
        double r_horizon_;
    };

}
