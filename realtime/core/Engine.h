#pragma once

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <string>
#include <memory>
#include <vector>

#include "render/Renderer.h"
#include "core/Shader.h"
#include "core/FrameCapture.h"
#include "scene/ParticleBuffer.h"
#include "spacetime/AccretionDisk.h"

class Engine {
public:
    Engine(unsigned int width, unsigned int height, const std::string& title);
    ~Engine();

    void run();

private:
    void initWindow(const std::string& title);
    void initAssets();
    void update();
    void render();

    GLFWwindow* window;
    unsigned int width;
    unsigned int height;
    float rs;

    bool escapeWasDown;
    bool periodWasDown;
    bool mouseCaptured;

    std::unique_ptr<Renderer> renderer;
    std::unique_ptr<Shader> blackHoleShader;
    std::unique_ptr<FrameCapture> frameCapture;
    std::unique_ptr<ParticleBuffer> particleBuffer;

    std::unique_ptr<Physics::AccretionDisk> accretionDisk;
    std::unique_ptr<FallingParticleSystem> fallingSystem;

    unsigned int skyboxTexture;
    unsigned int geodesicLUT;
};
