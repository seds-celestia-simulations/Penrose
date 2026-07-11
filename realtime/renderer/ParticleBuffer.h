#pragma once

#include <glad/glad.h>
#include <vector>
#include "Particle.h"

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