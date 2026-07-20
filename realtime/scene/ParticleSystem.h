#pragma once
#include <vector>
#include "scene/Particle.h"

class ParticleSystem {
public:
    virtual ~ParticleSystem() = default;

    virtual void update(float deltaTime) = 0;
    virtual const std::vector<Particle>& getParticles() const = 0;
};
