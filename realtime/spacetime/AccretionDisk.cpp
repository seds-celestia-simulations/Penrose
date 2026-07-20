#include "AccretionDisk.h"
#include <random>
#include <cmath>

namespace Physics {

AccretionDisk::AccretionDisk(int numParticles) 
    : m_numParticles(numParticles), m_particles(numParticles), m_sphericalCoords(numParticles) 
{
    std::random_device rd;
    std::mt19937 gen(rd());
    const float PI = 3.14159265359f;

    std::uniform_real_distribution<float> radDist(3.0f, 15.0f);
    std::uniform_real_distribution<float> phiDist(0.0f, 2.0f * PI);
    std::normal_distribution<float> thetaDist(PI / 2.0f, 0.05f); 

    for (int i = 0; i < m_numParticles; i++) {
        float r = radDist(gen);
        float theta = thetaDist(gen);
        float phi = phiDist(gen);

        // Store internally for tracking the integration state
        m_sphericalCoords[i] = glm::vec3(r, theta, phi);

        // Compute initial Cartesian coordinates
        m_particles[i].position.x = r * std::sin(theta) * std::cos(phi);
        m_particles[i].position.y = r * std::sin(theta) * std::sin(phi);
        m_particles[i].position.z = r * std::cos(theta);
        m_particles[i].position.w = r; 

        // Procedural thermal coloring (hotter/brighter near the event horizon)
        float heat = 1.0f - ((r - 3.0f) / 12.0f); 
        m_particles[i].color = glm::vec3(1.0f, heat * 0.8f, heat * 0.2f); 
        m_particles[i].mass = 1.0f;
        m_particles[i].radius = 0.03f; 
    }
}

void AccretionDisk::update(float deltaTime) {
    for (int i = 0; i < m_numParticles; i++) {
        float r = m_sphericalCoords[i].x;
        float theta = m_sphericalCoords[i].y;
        float phi = m_sphericalCoords[i].z;

        // Keplerian-style orbital speed falloff relative to distance from singularity
        float orbitSpeed = 2.0f / std::pow(r, 1.5f); 
        phi += orbitSpeed * deltaTime;
        
        m_sphericalCoords[i].z = phi;

        // Convert updated angular frames back to Cartesian space
        m_particles[i].position.x = r * std::sin(theta) * std::cos(phi);
        m_particles[i].position.y = r * std::sin(theta) * std::sin(phi);
        m_particles[i].position.z = r * std::cos(theta);
    }
}

} // namespace Physics