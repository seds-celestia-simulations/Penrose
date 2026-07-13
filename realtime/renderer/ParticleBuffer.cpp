#include "ParticleBuffer.h"

ParticleBuffer::ParticleBuffer() : SSBO(0), particleCount(0) {
    glGenBuffers(1, &SSBO);
}

ParticleBuffer::~ParticleBuffer() {
    glDeleteBuffers(1, &SSBO);
}

void ParticleBuffer::uploadParticles(const std::vector<Particle>& particles) {
    particleCount = particles.size();
    
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, SSBO);
    glBufferData(GL_SHADER_STORAGE_BUFFER, 
                 particles.size() * sizeof(Particle), 
                 particles.data(), 
                 GL_DYNAMIC_DRAW);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
}

void ParticleBuffer::bind(unsigned int bindingPoint) {
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, bindingPoint, SSBO);
}