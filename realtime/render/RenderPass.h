#pragma once
#include <string>
#include "scene/Camera.h"

struct PassContext {
    Camera& camera;
    float time;
    int width, height;
    unsigned int skyboxTexture;
    unsigned int geodesicLUT;
};

class RenderPass {
public:
    virtual ~RenderPass() = default;

    virtual void execute(const PassContext& ctx) = 0;
    virtual const std::string& name() const = 0;
};
