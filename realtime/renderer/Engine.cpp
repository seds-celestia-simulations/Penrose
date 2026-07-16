#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>
#include <vector>
#include <random>

// Rendering headers
#include "renderer/Shader.h"
#include "renderer/Window.h"
#include "renderer/Camera.h"
#include "renderer/Texture.h"
#include "renderer/Renderer.h"
#include "renderer/Particle.h"
#include "renderer/FrameCapture.h"
#include "renderer/RenderTarget.h"
#include "renderer/LutBaker.h"
#include "renderer/AccretionDisk.h"
#include "renderer/ParticleBuffer.h"
#include "renderer/Engine.h"


// Basically the old main ported 
Camera camera(glm::vec3(0.5f, 0.0f, 2.0f));
float deltaTime = 0.0f; 
float lastFrame = 0.0f; 
bool firstMouse = true;

Engine::Engine(unsigned int width, unsigned int height, const std::string& title)
    : width(width), height(height), rs(0.25f), 
      escapeWasDown(false), periodWasDown(false), mouseCaptured(true) 
{
    initWindow(title);
    initAssets();
}

Engine::~Engine() {
    glfwTerminate();
}

void Engine::initWindow(const std::string& title) {
   // 0. Register GLFW Error Callback
    glfwSetErrorCallback([](int error, const char* description) {
        std::cerr << "GLFW Error (" << error << "): " << description << std::endl;
    });

    // 1. Initialize GLFW
    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW\n";
        exit(-1);
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    // 2. Create the Window
    window = glfwCreateWindow(width, height, "Penrose: RK4 Black Hole", NULL, NULL);
    if (!window) {
        std::cerr << "Failed to create GLFW window\n";
        glfwTerminate();
        exit(-1);

    }
    glfwMakeContextCurrent(window);
    glfwSwapInterval(0);
    setupMouseControls(window, camera);
    
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

    // 3. Initialize GLAD
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cerr << "Failed to initialize GLAD\n";
        exit(-1);

    }

     // Print runtime OpenGL information
    std::cout << "========================================" << std::endl;
    std::cout << "OpenGL Version:  " << glGetString(GL_VERSION) << std::endl;
    std::cout << "OpenGL Vendor:   " << glGetString(GL_VENDOR) << std::endl;
    std::cout << "OpenGL Renderer: " << glGetString(GL_RENDERER) << std::endl;
    std::cout << "GLSL Version:    " << glGetString(GL_SHADING_LANGUAGE_VERSION) << std::endl;
    std::cout << "========================================" << std::endl;

}

void Engine::initAssets() {
    // 1. Bake LUT
    std::cout << "Baking LUT... This might take a second.\n";
    Physics::BakerConfig config;
    config.rs = rs;
    config.rMin = rs * 1.001f;
    std::vector<float> lutData = Physics::LutBaker::bakeSchwarzschildLUT(config);
    std::cout << "LUT Bake Complete!\n";

    glGenTextures(1, &geodesicLUT);
    glBindTexture(GL_TEXTURE_2D, geodesicLUT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB32F, config.lutSize, config.lutSize, 0, GL_RGB, GL_FLOAT, lutData.data());
    lutData.clear();

    // 2. Load Shaders & Textures
    skyboxTexture = loadTexture("realtime/resources/starfield_original.jpg");
    blackHoleShader = std::make_unique<Shader>("realtime/shaders/quad.vert", "realtime/shaders/reduced.frag");
    
    blackHoleShader->use();
    blackHoleShader->setInt("skybox", 0);
    blackHoleShader->setInt("uGeodesicLUT", 1);

    // 3. Initialize Core Systems
    renderer = std::make_unique<Renderer>();
    frameCapture = std::make_unique<FrameCapture>();
    particleBuffer = std::make_unique<ParticleBuffer>();
    accretionDisk = std::make_unique<Physics::AccretionDisk>(10);
    fallingSystem = std::make_unique<FallingParticleSystem>();
}

void Engine::update() {
    accretionDisk->update(deltaTime);
    fallingSystem->update(deltaTime, rs);

    std::vector<Particle> renderPayload;
    for (const auto& dp : accretionDisk->getParticles()) {
        Particle p;
        p.stateX = dp.position;
        p.color = dp.color;
        p.mass = dp.mass;
        p.radius = dp.radius;
        p.stateU = glm::vec4(0.0f);
        renderPayload.push_back(p);
    }

    const auto& drops = fallingSystem->getParticles();
    renderPayload.insert(renderPayload.end(), drops.begin(), drops.end());
    renderer->updateParticles(renderPayload); 
    //^^^^ COMMENT THIS IF YOU DONT WANT PARTICLES or want performance since they 
    // butcher your fps
}

void Engine::render() {
    int w, h;
    glfwGetFramebufferSize(window, &w, &h);
    
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(0, 0, w, h);
    glClear(GL_COLOR_BUFFER_BIT);

    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, geodesicLUT);
//    particleBuffer->bind(0);

    float currentFrame = (float)glfwGetTime();
    renderer->draw(*blackHoleShader, camera, skyboxTexture, currentFrame, w, h, true, false);
}

void Engine::run() {
    // 6. Main Render Loop
    while (!glfwWindowShouldClose(window)) {
        // --- TIMING MATH ---
        float currentFrame = (float)glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        // --- PROCESS KEYBOARD INPUT ---
        // uses the global camera and deltaTime(=dt by default)
        static bool escapeWasDown = false;
        processInput(window, camera, *frameCapture);
        bool escapeDown = glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS;
        //mouse
        if (escapeDown && !escapeWasDown) {
            static bool mouseCaptured = true;
            mouseCaptured = !mouseCaptured;

            glfwSetInputMode(window,GLFW_CURSOR,mouseCaptured ? GLFW_CURSOR_DISABLED : GLFW_CURSOR_NORMAL
            );

            firstMouse = true;
        }
        escapeWasDown = escapeDown;
        // Edge-detection to avoid multi-spawning frames from holding the key down
        static bool periodWasDown = false;
        bool periodDown = glfwGetKey(window, GLFW_KEY_PERIOD) == GLFW_PRESS;

        if (periodDown && !periodWasDown) {
            // FIX: Pass the global camera position into the spawn function!
            fallingSystem->spawnParticle(camera.Position); 
            std::cout << "Dynamic test sphere dropped into spherical coordinates pipeline!\n";
        }
        periodWasDown = periodDown;

        // --- CLEAR SCREEN ---
        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        update();
        render();

        // FPS 
        static double lastFPSTime = glfwGetTime();
        static int frames = 0;
        frames++;
        if (glfwGetTime() - lastFPSTime >= 1.0) {
            std::cout << "FPS: " << frames << "\n";
            frames = 0;
            lastFPSTime = glfwGetTime();
        }
// double t1= glfw

        // --- CAPTURE FRAME IF ENABLED ---
        if (frameCapture->getIsCapturing()) {
            std::string filePath = frameCapture->getNextFilePath();
            renderer->captureFrame(filePath, window);
        }

        glfwSwapBuffers(window);
        glfwPollEvents();
    }
}