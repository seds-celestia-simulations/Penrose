#pragma once

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <string>
#include <memory>
#include <vector>

#include "renderer/Renderer.h"
#include "renderer/Shader.h"
#include "renderer/FrameCapture.h"
#include "renderer/ParticleBuffer.h"
#include "renderer/AccretionDisk.h"
#include "renderer/ParticleBuffer.h"

class Engine {
public:
    Engine(unsigned int width, unsigned int height, const std::string& title);
    ~Engine();

    // The only function main() will ever call
    void run();

private:
    void initWindow(const std::string& title);
    void initAssets();
    void update();
    void render();

    // OS & Window State
    GLFWwindow* window;
    unsigned int width;
    unsigned int height;
    float rs; // Schwarzschild radius constant

    // Input Trackers
    bool escapeWasDown;
    bool periodWasDown;
    bool mouseCaptured;

    // --- SYSTEMS (Instantiated after OpenGL Context is ready) ---
    std::unique_ptr<Renderer> renderer;
    std::unique_ptr<Shader> blackHoleShader;
    std::unique_ptr<FrameCapture> frameCapture;
    std::unique_ptr<ParticleBuffer> particleBuffer;

    // --- PHYSICS ---
    std::unique_ptr<Physics::AccretionDisk> accretionDisk;
    std::unique_ptr<FallingParticleSystem> fallingSystem;

    // --- ASSETS ---
    unsigned int skyboxTexture;
    unsigned int geodesicLUT;
};