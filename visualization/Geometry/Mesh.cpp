#include "Mesh.h"

#include <cmath>

namespace viz {

namespace {

Triangle make_triangle(const Vec3& a, const Vec3& b, const Vec3& c) {
    const Vec3 n = (b - a).cross(c - a).normalized();
    return {a, b, c, n};
}

} // namespace

Mesh make_uv_sphere(int stacks, int slices, float radius) {
    Mesh mesh;
    if (stacks < 2 || slices < 3) {
        return mesh;
    }

    for (int i = 0; i < stacks; ++i) {
        const float t0 = static_cast<float>(i) / static_cast<float>(stacks);
        const float t1 = static_cast<float>(i + 1) / static_cast<float>(stacks);
        const float theta0 = t0 * 3.14159265359f;
        const float theta1 = t1 * 3.14159265359f;

        for (int j = 0; j < slices; ++j) {
            const float p0 = static_cast<float>(j) / static_cast<float>(slices);
            const float p1 = static_cast<float>(j + 1) / static_cast<float>(slices);
            const float phi0 = p0 * 2.0f * 3.14159265359f;
            const float phi1 = p1 * 2.0f * 3.14159265359f;

            auto sph = [radius](float theta, float phi) {
                const float st = std::sin(theta);
                return Vec3(radius * st * std::cos(phi), radius * st * std::sin(phi), radius * std::cos(theta));
            };

            const Vec3 a = sph(theta0, phi0);
            const Vec3 b = sph(theta1, phi0);
            const Vec3 c = sph(theta1, phi1);
            const Vec3 d = sph(theta0, phi1);

            mesh.triangles.push_back(make_triangle(a, b, c));
            mesh.triangles.push_back(make_triangle(a, c, d));
        }
    }

    return mesh;
}

Mesh make_annulus(float inner_radius, float outer_radius, int segments) {
    Mesh mesh;
    if (segments < 3 || outer_radius <= inner_radius) {
        return mesh;
    }

    for (int i = 0; i < segments; ++i) {
        const float a0 = static_cast<float>(i) / static_cast<float>(segments) * 2.0f * 3.14159265359f;
        const float a1 = static_cast<float>(i + 1) / static_cast<float>(segments) * 2.0f * 3.14159265359f;

        const Vec3 i0(inner_radius * std::cos(a0), inner_radius * std::sin(a0), 0.0f);
        const Vec3 i1(inner_radius * std::cos(a1), inner_radius * std::sin(a1), 0.0f);
        const Vec3 o0(outer_radius * std::cos(a0), outer_radius * std::sin(a0), 0.0f);
        const Vec3 o1(outer_radius * std::cos(a1), outer_radius * std::sin(a1), 0.0f);

        mesh.triangles.push_back(make_triangle(i0, o0, o1));
        mesh.triangles.push_back(make_triangle(i0, o1, i1));
    }

    return mesh;
}

} // namespace viz
