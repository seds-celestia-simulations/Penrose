#include "TestDeclarations.h"
#include "../Camera/Camera.h"

#include <cmath>

namespace viz::test {

bool test_camera_view_projection_finite() {
    Camera camera;
    const Mat4 vp = camera.view_projection(16.0f / 9.0f);
    const Vec3 p = vp.transform_point(camera.target());
    return std::isfinite(p.x) && std::isfinite(p.y) && std::isfinite(p.z);
}

} // namespace viz::test
