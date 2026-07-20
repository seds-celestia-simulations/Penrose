#pragma once
#include <state/GeodesicState.h>
#include <Eigen/Dense>

namespace CoordinateChart {
    Eigen::Matrix4d sph_to_cart_Jacobian(double r, double theta, double phi);
    Eigen::Matrix4d cart_to_sph_Jacobian(double r, double theta, double phi);
    State cart_to_sphere(const State& cartState);
    State sph_to_cart(State& sphState);
}
