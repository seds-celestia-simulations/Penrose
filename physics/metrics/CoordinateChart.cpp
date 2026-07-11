#include "CoordinateChart.h"
#include <cmath>
#include <algorithm>

namespace CoordinateChart {

// Jacobian converts U between Polar and Cartesian
Eigen::Matrix4d sph_to_cart_Jacobian(double r, double theta, double phi) {
    Eigen::Matrix4d J = Eigen::Matrix4d::Zero();
    J(0, 0) = 1.0; // dt/dt is 1

    J(1, 1) = std::sin(theta) * std::cos(phi);
    J(1, 2) = r * std::cos(theta) * std::cos(phi);
    J(1, 3) = -r * std::sin(theta) * std::sin(phi);

    J(2, 1) = std::sin(theta) * std::sin(phi);
    J(2, 2) = r * std::cos(theta) * std::sin(phi);
    J(2, 3) = r * std::sin(theta) * std::cos(phi);

    J(3, 1) = std::cos(theta);
    J(3, 2) = -r * std::sin(theta);
    J(3, 3) = 0.0;
    return J;
}

// Exact same Jacobian but Eigen lets us calculate the inverse directly
Eigen::Matrix4d cart_to_sph_Jacobian(double r, double theta, double phi) {
    return sph_to_cart_Jacobian(r, theta, phi).inverse();
}

State cart_to_sphere(const State& cartState) {
    double t = cartState.X[0];
    double x = cartState.X[1];
    double y = cartState.X[2];
    double z = cartState.X[3];

    double r = std::sqrt(x*x + y*y + z*z);
    double phi = std::atan2(y, x);
    double theta = std::acos(std::clamp(z/(r + 1e-8), -1.0, 1.0));
    // +1e-8 prevents divide by zero incase particle accidently ends up at origin anc clamp makes sure input to cos-1 is in range

    return State(Eigen::Vector4d(t, r, theta, phi), cart_to_sph_Jacobian(r, theta, phi) * cartState.U);
}

State sph_to_cart(State& sphState) {
    double t = sphState.X[0];
    double r = sphState.X[1];
    double theta = sphState.X[2];
    double phi = sphState.X[3];

    double x = r * std::cos(phi) * std::sin(theta);
    double y = r * std::sin(phi) * std::sin(theta);
    double z = r * std::cos(theta);

    return State(Eigen::Vector4d(t, x, y, z), sph_to_cart_Jacobian(r, theta, phi) * sphState.U);
}

} // namespace CoordinateChart
