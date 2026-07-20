#pragma once
#include <glad/glad.h>

class Framebuffer {
public:
    unsigned int fbo = 0;
    unsigned int colorTexture = 0;
    unsigned int width = 0;
    unsigned int height = 0;

    Framebuffer(unsigned int w, unsigned int h);
    ~Framebuffer();

    void bind() const;
    void unbind() const;
};
