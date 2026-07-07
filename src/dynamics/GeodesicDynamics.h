#pragma once
#include "DynamicsModel.h"
#include "../spacetime/Metric.h"

namespace Dynamics {

    class GeodesicDynamics : public DynamicsModel {
    public:
        explicit GeodesicDynamics(const Spacetime::Metric& metric);

        State compute_derivative(const State& state) const override;

    private:
        const Spacetime::Metric& metric_;
    };

}
