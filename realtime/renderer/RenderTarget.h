#pragma once
#include <glad/glad.h>

class RenderTarget {
public:
    unsigned int fbo = 0;
    unsigned int colorTexture = 0;
    unsigned int width = 0;
    unsigned int height = 0;

    RenderTarget(unsigned int w, unsigned int h);
    ~RenderTarget();

    void bind() const;
    void unbind() const;
};