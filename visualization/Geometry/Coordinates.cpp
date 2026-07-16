#include "Coordinates.h"

#include <cmath>

namespace viz {

Vec3 spherical_to_cartesian(double r, double theta, double phi) {
    const double sin_theta = std::sin(theta);
    const double cos_theta = std::cos(theta);
    const double sin_phi = std::sin(phi);
    const double cos_phi = std::cos(phi);

    return Vec3(static_cast<float>(r * cos_phi * sin_theta),
              static_cast<float>(r * sin_phi * sin_theta),
              static_cast<float>(r * cos_theta));
}

} // namespace viz
