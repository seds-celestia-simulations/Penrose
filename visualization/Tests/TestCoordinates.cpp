#include "TestDeclarations.h"
#include "../Geometry/Coordinates.h"

#include <cmath>

namespace viz::test {

bool test_spherical_to_cartesian_on_axis() {
    const Vec3 p = spherical_to_cartesian(2.0, 0.0, 0.0);
    return std::abs(p.x) < 1e-5f && std::abs(p.y) < 1e-5f && std::abs(p.z - 2.0f) < 1e-5f;
}

bool test_spherical_to_cartesian_equatorial() {
    const Vec3 p = spherical_to_cartesian(3.0, 1.5707963267948966, 0.0);
    return std::abs(p.x - 3.0f) < 1e-5f && std::abs(p.y) < 1e-5f && std::abs(p.z) < 1e-5f;
}

} // namespace viz::test
