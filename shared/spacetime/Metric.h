#pragma once
#include <Eigen/Dense>

namespace Spacetime {

    class Metric {
    public:
        virtual ~Metric() = default;

        virtual double christoffel(int mu, int alpha, int beta, const Eigen::Vector4d& X) const = 0;
    };

}
