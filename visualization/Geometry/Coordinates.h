#pragma once

#include "Math.h"

namespace viz {

// Schwarzschild spherical (t, r, theta, phi) -> Cartesian spatial (x, y, z).
// Matches the convention used in physics/metrics/CoordinateChart.cpp but is
// implemented locally so visualization never calls mutable physics APIs.
Vec3 spherical_to_cartesian(double r, double theta, double phi);

} // namespace viz
