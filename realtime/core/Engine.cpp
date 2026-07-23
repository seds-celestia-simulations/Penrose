#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>
#include <vector>
#include <random>

#include "core/ShaderManager.h"
#include "core/Window.h"
#include "scene/Camera.h"
#include "core/Texture.h"
#include "render/Renderer.h"
#include "render/GeodesicPass.h"
#include "render/UpscalePass.h"
#include "scene/Particle.h"
#include "core/FrameCapture.h"
#include "core/Framebuffer.h"
#include "spacetime/LutBaker.h"
#include "spacetime/AccretionDisk.h"
#include "scene/ParticleBuffer.h"
#include "core/Engine.h"


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
    glfwSetErrorCallback([](int error, const char* description) {
        std::cerr << "GLFW Error (" << error << "): " << description << std::endl;
    });

    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW\n";
        exit(-1);
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

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

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cerr << "Failed to initialize GLAD\n";
        exit(-1);

    }

    std::cout << "========================================" << std::endl;
    std::cout << "OpenGL Version:  " << glGetString(GL_VERSION) << std::endl;
    std::cout << "OpenGL Vendor:   " << glGetString(GL_VENDOR) << std::endl;
    std::cout << "OpenGL Renderer: " << glGetString(GL_RENDERER) << std::endl;
    std::cout << "GLSL Version:    " << glGetString(GL_SHADING_LANGUAGE_VERSION) << std::endl;
    std::cout << "========================================" << std::endl;

}

void Engine::initAssets() {
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

    skyboxTexture = loadTexture("resources/starfield_original.jpg");

    shaderManager = std::make_unique<ShaderManager>();
    shaderManager->loadMetric(MetricType::SCHWARZSCHILD_REDUCED,
        "shaders/quad.vert",
        "shaders/common/reduced_header.glsl",
        "shaders/metrics/schwarzschild_reduced.glsl",
        "shaders/common/reduced_main.glsl");
    shaderManager->loadMetric(MetricType::SCHWARZSCHILD_FULL,
        "shaders/quad.vert",
        "shaders/common/quad_header.glsl",
        "shaders/metrics/schwarzschild_full.glsl",
        "shaders/common/quad_main.glsl");
    shaderManager->setMetric(MetricType::SCHWARZSCHILD_REDUCED);

    Shader* activeShader = shaderManager->getActive();
    if (activeShader) {
        activeShader->use();
        activeShader->setInt("skybox", 0);
        activeShader->setInt("uGeodesicLUT", 1);
    }

    renderer = std::make_unique<Renderer>();
    frameCapture = std::make_unique<FrameCapture>();

    accretionDisk = std::make_unique<Physics::AccretionDisk>(10);
    fallingSystem = std::make_unique<FallingParticleSystem>();
    fallingSystem->setRs(rs);

    particleSystems.push_back(accretionDisk.get());
    particleSystems.push_back(fallingSystem.get());

    passes.push_back(std::make_unique<GeodesicPass>(*renderer, *shaderManager));
}

void Engine::update() {
    for (auto* ps : particleSystems) {
        ps->update(deltaTime);
    }

    std::vector<Particle> renderPayload;
    for (auto* ps : particleSystems) {
        const auto& particles = ps->getParticles();
        renderPayload.insert(renderPayload.end(), particles.begin(), particles.end());
    }
    renderer->updateParticles(renderPayload);
}

void Engine::render() {
    int w, h;
    glfwGetFramebufferSize(window, &w, &h);
    
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(0, 0, w, h);
    glClear(GL_COLOR_BUFFER_BIT);

    float currentFrame = (float)glfwGetTime();

    PassContext ctx {
        camera,
        currentFrame,
        w, h,
        skyboxTexture,
        geodesicLUT
    };

    for (auto& pass : passes) {
        pass->execute(ctx);
    }
}

void Engine::run() {
    while (!glfwWindowShouldClose(window)) {
        float currentFrame = (float)glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        static bool escapeWasDown = false;
        processInput(window, camera, *frameCapture);
        bool escapeDown = glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS;
        if (escapeDown && !escapeWasDown) {
            static bool mouseCaptured = true;
            mouseCaptured = !mouseCaptured;

            glfwSetInputMode(window,GLFW_CURSOR,mouseCaptured ? GLFW_CURSOR_DISABLED : GLFW_CURSOR_NORMAL
            );

            firstMouse = true;
        }
        escapeWasDown = escapeDown;
        static bool periodWasDown = false;
        bool periodDown = glfwGetKey(window, GLFW_KEY_PERIOD) == GLFW_PRESS;

        if (periodDown && !periodWasDown) {
            fallingSystem->spawnParticle(camera.Position); 
            std::cout << "Dynamic test sphere dropped into spherical coordinates pipeline!\n";
        }
        periodWasDown = periodDown;

        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        update();
        render();

        static double lastFPSTime = glfwGetTime();
        static int frames = 0;
        frames++;
        if (glfwGetTime() - lastFPSTime >= 1.0) {
            std::cout << "FPS: " << frames << "\n";
            frames = 0;
            lastFPSTime = glfwGetTime();
        }

        if (frameCapture->getIsCapturing()) {
            std::string filePath = frameCapture->getNextFilePath();
            renderer->captureFrame(filePath, window);
        }

        glfwSwapBuffers(window);
        glfwPollEvents();
    }
}
