#pragma once

#include <vector>
#include <glm/glm.hpp>

namespace Physics {

struct DiskParticleState {
    glm::vec4 position; // x, y, z, and w (radius tracker)
    glm::vec3 color;
    float mass;
    float radius;
};

class AccretionDisk {
public:
    AccretionDisk(int numParticles);

    // Updates the orbital physics step for all particles based on deltaTime
    void update(float deltaTime);

    // Exposes the read-only particle array to the renderer
    const std::vector<DiskParticleState>& getParticles() const { return m_particles; }

private:
    int m_numParticles;
    std::vector<DiskParticleState> m_particles;
    std::vector<glm::vec3> m_sphericalCoords; // Internal physics tracking (r, theta, phi)
};

} // namespace Physics