#pragma once

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "core/Shader.h"
#include "scene/Camera.h"
#include "scene/ParticleBuffer.h"

class Renderer {
private:
    unsigned int quadVAO, quadVBO;
    ParticleBuffer particleBuffer;

public:
    Renderer();
    ~Renderer();

    void draw(Shader& shader, Camera& camera, unsigned int texture, float currentFrame,   
     int renderWidth, int renderHeight, bool blackHolePass, bool highQualityPass);

    void updateParticles(const std::vector<Particle>& particles);
    
    bool captureFrame(const std::string& filePath, GLFWwindow* window);
};
