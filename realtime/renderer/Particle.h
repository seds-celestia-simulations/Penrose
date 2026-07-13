#pragma once
#include <glm/glm.hpp>

struct Particle {
    glm::vec4 stateX;      // x.x = time, x.y = r, x.z = theta, x.w = phi (matches your ray struct)
    glm::vec4 stateU;      // u.x = vt, u.y = vr, u.z = vtheta, u.w = vphi
    glm::vec3 color;       // RGB color
    float mass;            // Mass of particle
    float radius;          // Radius for collision/rendering
    float _pad0, _pad1, _pad2;  // Padding to align to 16 bytes per field
};