#pragma once

#include "Math.h"

#include <vector>

namespace viz {

struct Triangle {
    Vec3 v0;
    Vec3 v1;
    Vec3 v2;
    Vec3 normal;
};

struct Mesh {
    std::vector<Triangle> triangles;
};

Mesh make_uv_sphere(int stacks, int slices, float radius);
Mesh make_annulus(float inner_radius, float outer_radius, int segments);

} // namespace viz
