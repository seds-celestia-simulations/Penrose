#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>
#include <vector>
#include <random>

// Physics headers
#include "Constants.h"

// Rendering headers
#include "render/Shader.h"
#include "render/Window.h"
#include "render/Camera.h"
#include "render/Texture.h"
#include "render/Renderer.h"
#include "render/Particle.h"
#include "render/FrameCapture.h"
#include "render/RenderTarget.h"
#ifdef _WIN32
extern "C" {
    __declspec(dllexport) unsigned long NvOptimusEnablement = 0x00000001;
    __declspec(dllexport) int AmdPowerXpressRequestHighPerformance = 1;
}
#endif

const unsigned int SCR_WIDTH = 800;
const unsigned int SCR_HEIGHT = 600;

Camera camera(glm::vec3(0.5f, 0.0f, 2.0f));

float deltaTime = 0.0f; 
float lastFrame = 0.0f; 
bool firstMouse = true;
float lastMouseX = SCR_WIDTH  * 0.5f;
float lastMouseY = SCR_HEIGHT * 0.5f;

int main() {
    // 0. Register GLFW Error Callback
    glfwSetErrorCallback([](int error, const char* description) {
        std::cerr << "GLFW Error (" << error << "): " << description << std::endl;
    });

    // 1. Initialize GLFW
    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW\n";
        return -1;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    // 2. Create the Window
    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "Penrose: RK4 Black Hole", NULL, NULL);
    if (!window) {
        std::cerr << "Failed to create GLFW window\n";
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    setupMouseControls(window, camera);
    
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

    // 3. Initialize GLAD
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cerr << "Failed to initialize GLAD\n";
        return -1;
    }

    // Print runtime OpenGL information
    std::cout << "========================================" << std::endl;
    std::cout << "OpenGL Version:  " << glGetString(GL_VERSION) << std::endl;
    std::cout << "OpenGL Vendor:   " << glGetString(GL_VENDOR) << std::endl;
    std::cout << "OpenGL Renderer: " << glGetString(GL_RENDERER) << std::endl;
    std::cout << "GLSL Version:    " << glGetString(GL_SHADING_LANGUAGE_VERSION) << std::endl;
    std::cout << "========================================" << std::endl;

    // 4. Load the Shader Program & Textures
    const unsigned int RENDER_WIDTH = 400;
    const unsigned int RENDER_HEIGHT = 300;

    RenderTarget lowResTarget(RENDER_WIDTH, RENDER_HEIGHT);

    Shader blackHoleShader("shaders/quad.vert", "shaders/reduced.frag");
    Shader upscaleShader("shaders/quad.vert", "shaders/upscale.frag");

    upscaleShader.use();
    upscaleShader.setInt("uLowResImage", 0);

    unsigned int skyboxTexture = loadTexture("shaders/starfield_original.jpg");
    if (skyboxTexture == 0) return -1; // Fail if image didn't load

    // 5. Setup the Full-Screen Quad Geometry
    Renderer renderer; // The constructor silently builds the VAO/VBO here!

// --- PROCEDURAL ACCRETION DISK GENERATION ---
    std::random_device rd;
    std::mt19937 gen(rd());
    const float PI = 3.14159265359f;

    std::uniform_real_distribution<float> radDist(3.0f, 15.0f);
    std::uniform_real_distribution<float> phiDist(0.0f, 2.0f * PI);
    std::normal_distribution<float> thetaDist(PI / 2.0f, 0.05f); 

    int NUM_PARTICLES = 10;
    std::vector<Particle> particles(NUM_PARTICLES);
    
    // We store the spherical coords parallel to the GPU particles to calculate orbits
    std::vector<glm::vec3> sphericalCoords(NUM_PARTICLES); 

    for (int i = 0; i < NUM_PARTICLES; i++) {
        float r = radDist(gen);
        float theta = thetaDist(gen);
        float phi = phiDist(gen);

        // Store spherical coords for the CPU update loop (x=r, y=theta, z=phi)
        sphericalCoords[i] = glm::vec3(r, theta, phi);

        // Calculate initial Cartesian coords for the GPU
        particles[i].stateX.x = r * std::sin(theta) * std::cos(phi);
        particles[i].stateX.y = r * std::sin(theta) * std::sin(phi);
        particles[i].stateX.z = r * std::cos(theta);
        particles[i].stateX.w = r; 

        particles[i].stateU = glm::vec4(0.0f);

        // Procedural Color (Hotter near the black hole)
        float heat = 1.0f - ((r - 3.0f) / 12.0f); 
        particles[i].color = glm::vec3(1.0f, heat * 0.8f, heat * 0.2f); 
        particles[i].mass = 1.0f;
        particles[i].radius = 0.03f; 
    }

    // Upload initial state to renderer
    renderer.updateParticles(particles);
    
    blackHoleShader.use();
    blackHoleShader.setInt("skybox", 0);

    // Initialize frame capture
    FrameCapture frameCapture;

    // 6. Main Render Loop
    while (!glfwWindowShouldClose(window)) {
        // --- TIMING MATH ---
        float currentFrame = (float)glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        // --- PROCESS KEYBOARD INPUT ---
        // uses the global camera and deltaTime(=dt by default)
        static bool escapeWasDown = false;
        processInput(window, camera, frameCapture);
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

        // --- CLEAR SCREEN ---
        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        // --- UPDATE ---
        // Update particle data if needed
for (size_t i = 0; i < NUM_PARTICLES; i++) {
            float r = sphericalCoords[i].x;
            float theta = sphericalCoords[i].y;
            float phi = sphericalCoords[i].z;

            // Calculate orbital speed (faster closer to the black hole)
            float orbitSpeed = 2.0f / std::pow(r, 1.5f); 
            
            // Increment the azimuthal angle (phi) based on time and speed
            phi += orbitSpeed * deltaTime;
            
            // Save the updated angle back to our CPU tracker
            sphericalCoords[i].z = phi;

            // Convert the new position to Cartesian for the GPU
            particles[i].stateX.x = r * std::sin(theta) * std::cos(phi);
            particles[i].stateX.y = r * std::sin(theta) * std::sin(phi);
            particles[i].stateX.z = r * std::cos(theta);
        }
        
        // Push the new Cartesian frames to the SSBO
        renderer.updateParticles(particles);

        // --- DRAW Fn ---
        int width, height;
        glfwGetFramebufferSize(window, &width, &height);
        // PASS 1: trace black hole into 400x300 texture
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
glViewport(0, 0, width, height);
glClear(GL_COLOR_BUFFER_BIT);

renderer.draw(blackHoleShader, camera, skyboxTexture, currentFrame, width, height, true, false);
        // FPS
                static double lastFPSTime = glfwGetTime();
        static int frames = 0;
        frames++;
        double now = glfwGetTime();
        if (now - lastFPSTime >= 1.0)
        {
            std::cout << "FPS: " << frames << "\n";
            frames = 0;
            lastFPSTime = now;
        }
//double t1 = glfw

        // --- CAPTURE FRAME IF ENABLED ---
        if (frameCapture.getIsCapturing()) {
            std::string filePath = frameCapture.getNextFilePath();
            renderer.captureFrame(filePath, window);
        }

        // --- SWAP BUFFERS ---
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    // 7. Cleanup
    glfwTerminate();
    return 0;
}