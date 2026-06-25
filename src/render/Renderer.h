#pragma once

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "Shader.h"
#include "Camera.h"
#include "ParticleBuffer.h"

class Renderer {
private:
    unsigned int quadVAO, quadVBO;
    ParticleBuffer particleBuffer;

public:
    Renderer();
    ~Renderer();

    // draw function called by main
    void draw(Shader& shader, GLFWwindow* window, Camera& camera, unsigned int skyboxTexture, float currentFrame);

    void updateParticles(const std::vector<Particle>& particles);
    
    // capture the current framebuffer to a PNG file
    bool captureFrame(const std::string& filePath, GLFWwindow* window);
};