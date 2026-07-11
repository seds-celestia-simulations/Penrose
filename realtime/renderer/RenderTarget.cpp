#include "RenderTarget.h"
#include <iostream>

RenderTarget::RenderTarget(unsigned int w, unsigned int h)
    : width(w), height(h)
{
    glGenFramebuffers(1, &fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);

    // Empty texture: the black-hole shader will write pixels into this.
    glGenTextures(1, &colorTexture);
    glBindTexture(GL_TEXTURE_2D, colorTexture);

    glTexImage2D(
        GL_TEXTURE_2D,
        0,
        GL_RGB16F,      // good for bright disk / HDR-ish values
        width,
        height,
        0,
        GL_RGB,
        GL_FLOAT,
        nullptr
    );

    // This is the upscaling filter.
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glFramebufferTexture2D(
        GL_FRAMEBUFFER,
        GL_COLOR_ATTACHMENT0,
        GL_TEXTURE_2D,
        colorTexture,
        0
    );

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        std::cerr << "RenderTarget framebuffer incomplete\n";
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

RenderTarget::~RenderTarget()
{
    if (colorTexture) glDeleteTextures(1, &colorTexture);
    if (fbo) glDeleteFramebuffers(1, &fbo);
}

void RenderTarget::bind() const
{
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);
    glViewport(0, 0, width, height);
}

void RenderTarget::unbind() const
{
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}