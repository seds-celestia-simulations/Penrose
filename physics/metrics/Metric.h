#pragma once
#include <Eigen/Dense>

namespace Spacetime {

    // Represents a Spacetime Geometry, defining its Christoffel symbols.
    class Metric {
    public:
        virtual ~Metric() = default;

        // Evaluates the Christoffel symbol \Gamma^\mu_{\alpha\beta} at a given position X.
        virtual double christoffel(int mu, int alpha, int beta, const Eigen::Vector4d& X) const = 0;
    };

}
