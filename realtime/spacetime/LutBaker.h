#pragma once

#include <vector>

namespace Physics {

struct BakerConfig {
    int lutSize = 512;
    float rs = 0.25f;
    float rMin = 0.25f * 1.001f;
    float rMax = 20.0f;
    int maxSteps = 5000;
    float dPsi = 0.005f;
};

class LutBaker {
public:
    static std::vector<float> bakeSchwarzschildLUT(const BakerConfig& config);
};

} 