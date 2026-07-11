#include "SchwarzschildMetric.h"
#include <cmath>

namespace Spacetime {

    SchwarzschildMetric::SchwarzschildMetric(double rs) : rs_(rs) {}

    double SchwarzschildMetric::christoffel(int mu, int alpha, int beta, const Eigen::Vector4d& X) const {
        double r = X[1];
        double theta = X[2];

        double sinth = std::sin(theta);
        double costh = std::cos(theta);

        // Symmetric connection: \Gamma^\mu_{\alpha\beta} = \Gamma^\mu_{\beta\alpha}
        // Ensure alpha <= beta to simplify matching
        if (alpha > beta) {
            std::swap(alpha, beta);
        }

        // Time component (\mu = 0)
        if (mu == 0) {
            if (alpha == 0 && beta == 1) return rs_ / (2.0 * r * (r - rs_)); // \Gamma^t_{t r}
        }
        
        // Radial component (\mu = 1)
        else if (mu == 1) {
            if (alpha == 0 && beta == 0) return rs_ * (r - rs_) / (2.0 * r * r * r); // \Gamma^r_{t t}
            if (alpha == 1 && beta == 1) return -rs_ / (2.0 * r * (r - rs_)); // \Gamma^r_{r r}
            if (alpha == 2 && beta == 2) return -(r - rs_); // \Gamma^r_{\theta \theta}
            if (alpha == 3 && beta == 3) return -(r - rs_) * sinth * sinth; // \Gamma^r_{\phi \phi}
        }
        
        // Polar component (\mu = 2)
        else if (mu == 2) {
            if (alpha == 1 && beta == 2) return 1.0 / r; // \Gamma^\theta_{r \theta}
            if (alpha == 3 && beta == 3) return -sinth * costh; // \Gamma^\theta_{\phi \phi}
        }
        
        // Azimuthal component (\mu = 3)
        else if (mu == 3) {
            if (alpha == 1 && beta == 3) return 1.0 / r; // \Gamma^\phi_{r \phi}
            if (alpha == 2 && beta == 3) return costh / (sinth + 1e-8); // \Gamma^\phi_{\theta \phi}
        }

        return 0.0;
    }

}
