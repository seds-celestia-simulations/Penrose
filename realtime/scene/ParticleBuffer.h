#pragma once

#include <glad/glad.h>
#include <vector>
#include "Particle.h"
#include <vector>
#include <glm/glm.hpp>
#include <random>

class ParticleBuffer {
private:
    unsigned int SSBO;
    size_t particleCount;
    
public:
    ParticleBuffer();
    ~ParticleBuffer();
    
    // Upload particles to GPU
    void uploadParticles(const std::vector<Particle>& particles);
    
    // Bind SSBO to shader binding point
    void bind(unsigned int bindingPoint = 0);
    
    // Get particle count
    size_t getParticleCount() const { return particleCount; }
};

class FallingParticleSystem {
public:
    void update(float deltaTime, float rs) {
        float GM = rs * 0.5f; // Mass relationship from Schwarzschild radius

        for (auto it = m_particles.begin(); it != m_particles.end(); ) {
            // Treat stateX strictly as Cartesian (X, Y, Z) to match the shader
            glm::vec3 pos = glm::vec3(it->stateX);
            glm::vec3 vel = glm::vec3(it->stateU);
            float r = glm::length(pos);

            // 1. Event Horizon Check
            if (r <= rs * 1.001f) {
                it = m_particles.erase(it);
                continue;
            }

            // 2. Physics Step: Standard 3D Cartesian Gravity
            glm::vec3 acceleration = -(GM / (r * r * r)) * pos;
            vel += acceleration * deltaTime;
            pos += vel * deltaTime;

            // 3. Write back to the struct
            it->stateX = glm::vec4(pos, r);
            it->stateU = glm::vec4(vel, 0.0f);

            ++it;
        }
    }

    void spawnParticle(glm::vec3 cameraPosition) {
        Particle p;
        
        // Spawn right in front of the camera, slightly above the disk plane
        // (Your camera is at z = 2.0, black hole is at z = 0.0)
        glm::vec3 spawnPos = glm::vec3(0.0f, 0.5f, 1.0f); 
        
        p.stateX = glm::vec4(spawnPos, glm::length(spawnPos));
        
        // Push it hard along the X-axis (sideways), with a gentle nudge inward (Z-axis)
        p.stateU = glm::vec4(0.5f, 0.0f, -0.1f, 0.0f);

        // Keep your massive blue test settings!
        p.color = glm::vec3(0.0f, 0.0f, 1.0f); 
        p.mass = 1.0f;
        p.radius = 0.3f; 

        // Zero out the padding to keep the GPU memory strictly aligned
        p._pad0 = 0.0f;
        p._pad1 = 0.0f;
        p._pad2 = 0.0f;

        m_particles.push_back(p);
    }

    const std::vector<Particle>& getParticles() const { return m_particles; }

private:
    std::vector<Particle> m_particles;
};